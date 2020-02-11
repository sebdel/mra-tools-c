#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arc.h"
#include "mra.h"
#include "part.h"
#include "utils.h"

// make vscode happy
extern char *optarg;
extern int optind, opterr, optopt;

extern char *sha1;

int trace = 0;
int verbose = 0;

void print_usage() {
    printf("Usage: mra [-vlz] <my_file.mra>\n");
    printf("\n");
    printf("\t-h\tHelp.\n");
    printf("\t-v\tVersion. Only when it is the only parameter, otherwise set Verbose on (default: off).\n");
    printf("\t-l\tLists MRA content instead of creating the ROM file.\n");
    printf("\t-z dir\tSets directory to include zip files. This directory has priority over the current dir.\n");
    printf("\t-A\tCreate ARC file. This is done in addition to creating the ROM file.\n");
}

void print_version() {
    printf("MRA Tool (%s) (%s)\n", sha1, __DATE__);
}

void main(int argc, char **argv) {
    t_mra mra;
    char *rom_filename;
    char *mra_filename;
    char *arc_filename;
    t_string_list *dirs = string_list_new(NULL);
    int i, res;
    int dump_mra = 0;
    int create_arc = 0;

    // Parse command line
    // Looks bad but mkfs does it so why not me ?
    if (argc == 2 && !strcmp(argv[1], "-v")) {
        print_version();
        exit(0);
    }

    int opt;
    // put ':' in the starting of the
    // string so that program can
    //distinguish between '?' and ':'
    while ((opt = getopt(argc, argv, ":vlhAz:")) != -1) {
        switch (opt) {
            case 'v':
                verbose = -1;
                break;
            case 'l':
                dump_mra = -1;
                break;
            case 'A':
                create_arc = -1;
                break;
            case 'z':
                string_list_add(dirs, optarg);
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
    strncat(rom_filename, ".rom", 5);

    arc_filename = strndup(mra_filename, 1024);
    arc_filename[strnlen(rom_filename, 1024) - 4] = 0;
    strncat(arc_filename, ".arc", 5);

    char *mra_path = get_path(mra_filename);
    if (*mra_path)
        string_list_add(dirs, mra_path);
    string_list_add(dirs, ".");

    if (verbose) {
        printf("Parsing %s to %s\n", mra_filename, rom_filename);
        if (dirs->n_elements) {
            int i;
            printf("zip include dirs: ");
            for (i = 0; i < dirs->n_elements; i++) {
                printf("%s%s/", i ? ", " : "", dirs->elements[i]);
            }
            printf("\n");
        }
    }

    mra_load(mra_filename, &mra);
    if (trace > 0) printf("MRA loaded...\n");

    if (dump_mra) {
        if (trace > 0) printf("dumping MRA content...\n");
        mra_dump(&mra);
    } else {
        if (create_arc) {
            if (trace > 0) printf("create_arc set...\n");
            if (verbose) {
                printf("Creating ARC file!\n");
            }
            res = write_arc(&mra, arc_filename);
            if (res != 0) {
                printf("Writing ARC file failed with error code: %d\n. Retry without -A if you still want to create the ROM file.\n", res);
                exit(-1);
            }
        }
        if (trace > 0) printf("creating ROM...\n");
        res = write_rom(&mra, dirs, rom_filename);
        if (res != 0) {
            printf("Writing ROM failed with error code: %d\n", res);
            exit(-1);
        }
    }
    if (verbose) {
        printf("done!\n");
    }
}
