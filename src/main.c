#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mra.h"
#include "part.h"

// make vscode happy
extern char *optarg;
extern int optind, opterr, optopt;

int trace = 0;
int verbose = 0;

void print_usage() {
    printf("Usage: mra [-vlz] <my_file.mra>\n");
    printf("\n");
    printf("\t-h\tHelp.\n");
    printf("\t-v\tVerbose on (default: off)\n");
    printf("\t-l\tLists MRA content instead of creating the ROM\n");
    printf("\t-z dir\tSets directory to include zip files. This directory has priority over the current dir.\n");
}

void main(int argc, char **argv) {
    t_mra mra;
    char *rom_filename;
    char *mra_filename;
    char *zip_dir = "";
    int i, res;
    int dump_mra = 0;

    // Parse command line
    int opt;
    // put ':' in the starting of the
    // string so that program can
    //distinguish between '?' and ':'
    while ((opt = getopt(argc, argv, ":vlhz:")) != -1) {
        switch (opt) {
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

    if (optind == argc) {
        print_usage();
        exit(-1);
    }

    mra_filename = strndup(argv[optind], 1024);
    if (trace > 0)
        printf("mra: %s\n", mra_filename);

    rom_filename = strndup(mra_filename, 1024);
    rom_filename[strnlen(rom_filename, 1024) - 4] = 0;
    strncat(rom_filename, ".rom", 4);

    if (verbose) {
        printf("Parsing %s to %s\n", mra_filename, rom_filename);
        if (*zip_dir) {
            printf("ROMS zip dir: %s\n", zip_dir);
        }
    }

    mra_load(mra_filename, &mra);

    if (dump_mra) {
#if 1  // Use that to create test files
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

    } else {
        res = write_rom(&mra, zip_dir, rom_filename);
        if (res != 0) {
            printf("Writing ROM failed with error code: %d\n", res);
            exit(-1);
        }
    }

    if (verbose) {
        printf("done!\n");
    }
}
