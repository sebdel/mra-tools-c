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

void main(int argc, char **argv)
{
    XMLDoc mra;
    t_rom rom;
    t_files files;
    char *rom_filename;
    char *mra_filename;
    char *zip_dir = "";
    int i, res;
    int mra_dump = 0;
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
            mra_dump = -1;
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

    XMLDoc_init(&mra);
    res = XMLDoc_parse_file(mra_filename, &mra);
    if (res != 1)
    {
        printf("%s is not a valid xml file\n", mra_filename);
        exit(-1);
    }
    if (mra.i_root >= 0 && strncmp(mra.nodes[mra.i_root]->tag, "misterromdescription", 20) != 0)
    {
        printf("%s is not a valid MRA file\n", mra_filename);
        exit(-1);
    }

    memset(&rom, 0, sizeof(t_rom));
    memset(&files, 0, sizeof(t_files));

    mra_read_rom(mra.nodes[mra.i_root], &rom);
    mra_read_files(mra.nodes[mra.i_root], &files);

    if (mra_dump)
    {
        printf("rom.index = %s\n", rom.index);
        printf("rom.md5 = %s\n", rom.md5);
        printf("rom.type = %s\n", rom.type);
        printf("rom.zip = %s\n", rom.zip);
        printf("%d file parts found:\n", files.n_files);
        for (i = 0; i < files.n_files; i++)
        {
            printf("%s\n", files.file_names[i]);
        }
    }
    else
    {
        char *zip_filename;
        if (*zip_dir)
        {
            int length = strnlen(zip_dir, 1024) + strnlen(rom.zip, 1024);
            zip_filename = (char *)malloc(sizeof(char) * (length + 2));

            snprintf(zip_filename, 2050, "%s/%s", zip_dir, rom.zip);
        }
        else
        {
            zip_filename = rom.zip;
        }
        if (verbose)
            printf("Reading zip file: %s\n", zip_filename);

        files.data = (unsigned char **)malloc(sizeof(unsigned char *) * files.n_files);
        files.data_size = (long *)malloc(sizeof(long) * files.n_files);
        memset(files.data, 0, sizeof(unsigned char *) * files.n_files);

        res = unzip_file(zip_filename, &files);
        if (res != 0)
        {
            printf("\nFailed to unzip file: %s\n", rom.zip);
            exit(-1);
        }

        FILE *out;
        out = fopen(rom_filename, "wb");

        if (out == NULL)
        {
            fprintf(stderr, "Couldn't open %s for writing!\n", rom_filename);
            exit(-1);
        }

        for (i = 0; i < files.n_files; i++)
        {
            if (files.data[i])
            {
                fwrite(files.data[i], 1, files.data_size[i], out);
            }
            else
            {
                printf("%s data not found !\n", files.file_names[i]);
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
