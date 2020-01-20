#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "globals.h"
#include "unzip.h"
#include "mra.h"
#include "sxmlc.h"

// make vscode happy
extern char *optarg;
extern int optind, opterr, optopt;

int trace = 0;

void print_usage()
{
    printf("Usage: mra-tools [-vlz] <my_file.mra>\n");
    printf("\n");
    printf("\t-h\tHelp.\n");
    printf("\t-v\tVerbose on (default: off)\n");
    printf("\t-l\tList MRA content instead of creating the ROM\n");
    printf("\t-z dir\tSet directory where to look for the rom zip file\n");
}

int get_file_by_crc(t_file *files, int n_files, uint32_t crc)
{
    int i;

    for (i = 0; i < n_files; i++)
    {
        if (files[i].crc32 == crc)
        {
            if (trace > 0)
            {
                printf("crc matches for file: %s\n", files[i].name);
            }
            return i;
        }
    }
    return -1;
}

int get_file_by_name(t_file *files, int n_files, char *name)
{
    int i;

    for (i = 0; i < n_files; i++)
    {
        if (strncmp(files[i].name, name, 1024) == 0)
        {
            if (trace > 0)
            {
                printf("name matches for file: %s\n", files[i].name);
            }
            return i;
        }
    }
    return -1;
}

void main(int argc, char **argv)
{
    t_mra mra;
    char *rom_filename;
    char *mra_filename;
    char *zip_dir = "";
    int i, res;
    int dump_mra = 0;
    int verbose = 0;

    // Parse command line
    int opt;
    // put ':' in the starting of the
    // string so that program can
    //distinguish between '?' and ':'
    while ((opt = getopt(argc, argv, ":vlhz:")) != -1)
    {
        switch (opt)
        {
        case 'v':
            verbose = -1;
            break;
        case 'l':
            dump_mra = -1;
            break;
        case 'z':
            zip_dir = strndup(optarg, 1024);
            break;
        case 'h':
            print_usage();
            exit(0);

        case ':':
            printf("option needs a value\n");
            print_usage();
            exit(-1);

        case '?':
            printf("unknown option: %c\n", optopt);
            print_usage();
            exit(-1);
        }
    }

    if (optind == argc)
    {
        print_usage();
        exit(-1);
    }

    mra_filename = strndup(argv[optind], 1024);
    if (trace > 0)
        printf("mra: %s\n", mra_filename);

    rom_filename = strndup(mra_filename, 1024);
    rom_filename[strnlen(rom_filename, 1024) - 4] = 0;
    strncat(rom_filename, ".rom", 4);

    if (verbose)
    {
        printf("Parsing %s to %s\n", mra_filename, rom_filename);
        if (*zip_dir)
        {
            printf("ROMS zip dir: %s\n", zip_dir);
        }
    }

    mra_load(mra_filename, &mra);

    if (dump_mra)
    {
        mra_dump(&mra);
    }
    else
    {
        char *zip_filename;
        t_rom *rom;
        t_file *files = NULL;
        int n_files = 0;
        int rom_index;
        int i;

        rom_index = mra_get_next_rom0(&mra, rom_index);
        if (rom_index == -1)
        {
            printf("ROM0 not found in MRA.\n");
            exit(-1);
        }
        rom = mra.roms + rom_index;

        if (*zip_dir)
        {
            int length = strnlen(zip_dir, 1024) + strnlen(rom->zip, 1024);
            zip_filename = (char *)malloc(sizeof(char) * (length + 2));

            snprintf(zip_filename, 2050, "%s/%s", zip_dir, rom->zip);
        }
        else
        {
            zip_filename = rom->zip;
        }
        if (verbose)
        {
            printf("Reading zip file: %s\n", zip_filename);
        }

        // files.data = (unsigned char **)malloc(sizeof(unsigned char *) * files.n_files);
        // files.data_size = (long *)malloc(sizeof(long) * files.n_files);
        // memset(files.data, 0, sizeof(unsigned char *) * files.n_files);

        res = unzip_file(zip_filename, &files, &n_files);
        if (res != 0)
        {
            printf("\nFailed to unzip file: %s\n", zip_filename);
            exit(-1);
        }
        if (trace > 0)
        {
            for (i = 0; i < n_files; i++)
            {
                printf("%s\t%d\t%X\n", files[i].name, files[i].size, files[i].crc32);
            }
        }

        FILE *out;
        out = fopen(rom_filename, "wb");

        if (out == NULL)
        {
            fprintf(stderr, "Couldn't open %s for writing!\n", rom_filename);
            exit(-1);
        }

        for (i = 0; i < rom->n_parts; i++)
        {
            int n;
            t_part *part = rom->parts + i;

            n = -1;
            if (part->crc32)
            {
                n = get_file_by_crc(files, n_files, part->crc32);
            }
            if (n == -1 && part->name)
            {
                n = get_file_by_name(files, n_files, part->name);
            }
            if (n == -1)
            {
                printf("part not found in zip: %s\n", part->name);
                exit(-1);
            }
            if (trace > 0)
            {
                printf("file:\n");
                printf("  name: %s\n", files[n].name);
                printf("  size: %d\n", files[n].size);
            }

            if (files[n].data)
            {
                fwrite(files[n].data, 1, files[n].size, out);
            }
            else
            {
                printf("%s data not found !\n", files[n].name);
                exit(-1);
            }
        }
        fclose(out);
    }

    if (verbose)
    {
        printf("done!\n");
    }
}
