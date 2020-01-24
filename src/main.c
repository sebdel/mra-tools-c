#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "globals.h"
#include "unzip.h"
#include "mra.h"
#include "sxmlc.h"
#include "md5.h"

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

void sprintf_md5(char *dest, unsigned char *md5)
{
    snprintf(dest, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
             md5[0], md5[1], md5[2], md5[3], md5[4], md5[5], md5[6], md5[7],
             md5[8], md5[9], md5[10], md5[11], md5[12], md5[13], md5[14], md5[15]);
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

int write_to_rom(FILE *out, MD5_CTX *md5_ctx, uint8_t *data, size_t data_length, t_part *part)
{
    if (data)
    {
        if (part->offset >= data_length)
        {
            printf("warning: offset set past the part size. Skipping part.\n");
            return 0;
        }
        else
        {
            int i;
            int n_writes = part->repeat ? part->repeat : 1;
            size_t length = (part->length && (part->length < (data_length - part->offset))) ? part->length : (data_length - part->offset);
            for (i = 0; i < n_writes; i++)
            {
                fwrite(data + part->offset, 1, length, out);
                MD5_Update(md5_ctx, data + part->offset, length);
            }
        }
    }

    return 0;
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
#if 1 // Use that to create test files
        mra_dump(&mra);
#else
        int i;
        uint8_t buffer[16];

        FILE *out;
        out = fopen("01.dat", "wb");
        memset(buffer, 1, 16);
        fwrite(buffer, 1, 16, out);
        fclose(out);
        out = fopen("02.dat", "wb");
        memset(buffer, 2, 16);
        fwrite(buffer, 1, 16, out);
        fclose(out);
        out = fopen("03.dat", "wb");
        memset(buffer, 3, 16);
        fwrite(buffer, 1, 16, out);
        fclose(out);
        out = fopen("s16.dat", "wb");
        for (i = 0; i < 16; i++)
            buffer[i] = i;
        fwrite(buffer, 1, 16, out);
        fclose(out);
#endif
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
        MD5_CTX md5_ctx;
        unsigned char md5[16];
        char md5_string[33];

        out = fopen(rom_filename, "wb");
        MD5_Init(&md5_ctx);

        if (out == NULL)
        {
            fprintf(stderr, "Couldn't open %s for writing!\n", rom_filename);
            exit(-1);
        }

        for (i = 0; i < rom->n_parts; i++)
        {
            int j, n;
            t_part *part = rom->parts + i;

            if (part->zip)
            {
                printf("Support of part with zip attributes is not implemented!\n");
                exit(-1);
            }

            n = -1;
            if (part->crc32) // First, try to identify file by crc
            {
                n = get_file_by_crc(files, n_files, part->crc32);
            }
            if (n == -1 && part->name) // then by name
            {
                n = get_file_by_name(files, n_files, part->name);
            }
            if (n == -1 && !part->data)
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

            if (n != -1)
            {
                if (write_to_rom(out, &md5_ctx, files[n].data, files[n].size, part))
                {
                    exit(-1);
                }
            }
            else
            {
                if (write_to_rom(out, &md5_ctx, part->data, part->data_length, part))
                {
                    exit(-1);
                }
            }
        }
        fclose(out);
        MD5_Final(md5, &md5_ctx);
        sprintf_md5(md5_string, md5);
        if (verbose)
        {
            printf("%s\t%s\n", md5_string, rom_filename);
        }
        if (rom->md5)
        {
            if (strncmp(rom->md5, md5_string, 33))
            {
                printf("warning: md5 mismatch! (found: %s, expected: %s)\n", md5_string, rom->md5);
            }
            else if (verbose)
            {
                printf("MD5s match! (%s)\n", rom->md5);
            }
        }
    }

    if (verbose)
    {
        printf("done!\n");
    }
}
