#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arc.h"
#include "mra.h"
#include "rom.h"
#include "utils.h"

// make vscode happy
extern char *optarg;
extern int optind, opterr, optopt;

extern char *sha1;

int trace = 0;
int verbose = 0;
char *rom_basename = NULL;

void print_usage() {
    printf("\nUsage:\n\tmra [-vlzoOaA] <my_file.mra>\n");
    printf("\nConvert a MRA file to a ROM file for use on MiST arcade cores.\nOptionally creates the associated ARC file.\n");
    printf("For more informations, visit http://www.atari-forum.com/viewforum.php?f=115\n\n");
    printf("Options:\n\t-h\t\tthis help.\n");
    printf("\t-v\t\twhen it is the only parameter, display version information and exit. Otherwise, set Verbose on (default: off).\n");
    printf("\t-l\t\tlist MRA content instead of creating the ROM file.\n");
    printf("\t-z directory\tadd directory to include zip files. Directories added with -z have priority over the current dir.\n");
    printf("\t-o filename\tset the output ROM file name. Overrides the internal generation of the filename.\n");
    printf("\t-O directory\tset the output directory. By default, ROM and ARC files are created in the current directory.\n");
    printf("\t-a filename\tset the output ARC file name. Overrides the internal generation of the filename.\n");
    printf("\t-A\t\tcreate ARC file. This is done in addition to creating the ROM file.\n");
    printf("\t-s\t\tskip ROM creation. This is useful if only the ARC file is required.\n");
}

void print_version() {
    printf("MRA Tool (%s) (%s)\n", sha1, __DATE__);
}

void main(int argc, char **argv) {
    char *rom_filename = NULL;
    char *arc_filename = NULL;
    char *output_dir = NULL;
    char *mra_filename;
    char *mra_basename;
    t_string_list *dirs = string_list_new(NULL);
    int i, res;
    int dump_mra = 0;
    int dump_rom = -1;
    int create_arc = 0;

    if (trace > 0) {
        for (i = 0; i < argc; i++) {
            printf("argv[%d]: %s\n", i, argv[i]);
        }
    }

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
    while ((opt = getopt(argc, argv, ":vlhAo:a:O:z:s")) != -1) {
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
                string_list_add(dirs, replace_backslash(optarg));
                break;
            case 'O':
                output_dir = replace_backslash(strndup(optarg, 1024));
                break;
            case 'o':
                rom_filename = replace_backslash(strndup(optarg, 1024));
                break;
            case 'a':
                arc_filename = replace_backslash(strndup(optarg, 1024));
                break;
            case 's':
                dump_rom = 0;
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

    if( argc-optind > 1 ) {
        free( rom_filename );
        free( arc_filename );
        rom_filename = NULL;
        arc_filename = NULL;
    }

    for( int name_idx=optind; name_idx<argc; name_idx++) {
        t_mra mra;

        mra_filename = replace_backslash(strndup(argv[name_idx], 1024));
        if (!file_exists(mra_filename)) {
            printf("error: file not found (%s)\n", mra_filename);
            exit(-1);
        }

        if (trace > 0 )
            printf("mra: %s\n", mra_filename);

        char *mra_path = get_path(mra_filename);
        if (mra_path) {
            string_list_add(dirs, mra_path);
        }
        string_list_add(dirs, ".");

        if (verbose) {
            if (dirs->n_elements) {
                int i;
                printf("zip include dirs: ");
                for (i = 0; i < dirs->n_elements; i++) {
                    printf("%s%s/", i ? ", " : "", dirs->elements[i]);
                }
                printf("\n");
            }
        }

        if (mra_load(mra_filename, &mra)) {
            exit(-1);
        }

        mra_basename = get_basename(mra_filename, 1);
        if (rom_filename) {
            rom_basename = get_basename(rom_filename, 1);
            if (output_dir) {
                rom_filename = get_filename(output_dir, rom_basename, "rom");
            }
        } else {
            rom_basename = dos_clean_basename(mra.setname ? mra.setname : mra_basename, 0);
            rom_filename = get_filename(output_dir ? output_dir : ".", rom_basename, "rom");
        }

        if (trace > 0) printf("MRA loaded...\n");

        if (dump_mra) {
            if (trace > 0) printf("dumping MRA content...\n");
            mra_dump(&mra);
        } else {
            if (create_arc) {
                if (trace > 0) printf("create_arc set...\n");

                if (arc_filename) {
                    if (output_dir) {
                        arc_filename = get_filename(output_dir, get_basename(arc_filename, 1), "arc");
                    }
                    make_fat32_compatible(arc_filename, 0);
                } else {
                    char *arc_mra_filename = strdup(mra.name ? mra.name : mra_basename);
                    make_fat32_compatible(arc_mra_filename, 1);
                    arc_filename = get_filename(output_dir ? output_dir : ".", arc_mra_filename, "arc");
                    free(arc_mra_filename);
                }
                if (verbose) {
                    printf("Creating ARC file %s\n", arc_filename);
                }
                res = write_arc(&mra, arc_filename);
                if (res != 0) {
                    printf("Writing ARC file failed with error code: %d\n. Retry without -A if you still want to create the ROM file.\n", res);
                    exit(-1);
                }
                arc_filename = NULL;
            }
            if( dump_rom ) {
                if (trace > 0) printf("creating ROM...\n");
                res = write_rom(&mra, dirs, rom_filename);
                if (res != 0) {
                    printf("Writing ROM failed with error code: %d\n", res);
                    exit(-1);
                }
            }
            free( rom_filename );
            rom_filename = NULL;
        }
    }
    if (verbose) {
        printf("done!\n");
    }
}
