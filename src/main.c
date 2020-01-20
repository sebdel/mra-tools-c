#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sxmlc.h"
#include "junzip.h"

// make vscode happy
extern char *optarg;
extern int optind, opterr, optopt;

typedef struct s_files
{
    int n_files;
    char **file_names;
    unsigned char **data;
    long *data_size;
} t_files;

typedef struct s_rom
{
    char *index;
    char *zip;
    char *md5;
    char *type;
} t_rom;

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

void mra_read_rom(XMLNode *node, t_rom *rom)
{
    int i;

    if (strncmp(node->tag, "rom", 4) == 0)
    {
        int j;

        for (j = 0; j < node->n_attributes; j++)
        {
            if (strncmp(node->attributes[j].name, "index", 4) == 0)
            {
                rom->index = strndup(node->attributes[j].value, 256);
            }
            else if (strncmp(node->attributes[j].name, "zip", 3) == 0)
            {
                rom->zip = strndup(node->attributes[j].value, 256);
            }
            else if (strncmp(node->attributes[j].name, "md5", 3) == 0)
            {
                rom->md5 = strndup(node->attributes[j].value, 256);
            }
            else if (strncmp(node->attributes[j].name, "type", 4) == 0)
            {
                rom->type = strndup(node->attributes[j].value, 256);
            }
        }
    }
    else
    {
        for (i = 0; i < node->n_children; i++)
        {
            mra_read_rom(node->children[i], rom);
        }
    }
}

void mra_read_files(XMLNode *node, t_files *files)
{
    int i;

    if (strncmp(node->tag, "part", 4) == 0)
    {
        if (node->n_attributes > 0 && strncmp(node->attributes[0].name, "name", 4) == 0)
        {
            files->file_names = (char **)realloc(files->file_names, sizeof(char *) * (files->n_files + 1));
            files->file_names[files->n_files++] = strndup(node->attributes[0].value, 256);
        }
    }
    for (i = 0; i < node->n_children; i++)
    {
        mra_read_files(node->children[i], files);
    }
}

int processFile(JZFile *zip, t_files *files)
{
    JZFileHeader header;
    char filename[1024];

    if (jzReadLocalFileHeader(zip, &header, filename, sizeof(filename)))
    {
        printf("Couldn't read local file header!");
        return -1;
    }

    // look for index of filename in files->filenames
    int i, n;
    for (i = 0, n = strlen(filename); i < files->n_files && strncmp(files->file_names[i], filename, n); i++)
        ;
    if (i < files->n_files)
    {
        if ((files->data[i] = (unsigned char *)malloc(header.uncompressedSize)) == NULL)
        {
            printf("Couldn't allocate memory!");
            return -1;
        }
        files->data_size[i] = header.uncompressedSize;

        if (trace > 0)
        {
            printf("%s, %d / %d bytes at offset %08X\n", filename,
                   header.compressedSize, header.uncompressedSize, header.offset);
        }

        if (jzReadData(zip, &header, files->data[i]) != Z_OK)
        {
            printf("Couldn't read file data!");
            free(files->data[i]);
            files->data[i] = NULL;
            return -1;
        }
    }

    return 0;
}

int recordCallback(JZFile *zip, int idx, JZFileHeader *header, char *filename, void *user_data)
{
    long offset;

    offset = zip->tell(zip); // store current position

    if (zip->seek(zip, header->offset, SEEK_SET))
    {
        printf("Cannot seek in zip file!");
        return 0; // abort
    }

    processFile(zip, (t_files *)user_data); // alters file offset

    zip->seek(zip, offset, SEEK_SET); // return to position

    return 1; // continue
}

int unzip_file(char *file, t_files *files)
{
    FILE *fp;
    int retval = -1;
    JZEndRecord endRecord;

    JZFile *zip;

    if (!(fp = fopen(file, "rb")))
    {
        printf("Couldn't open \"%s\"!", file);
        return -1;
    }

    zip = jzfile_from_stdio_file(fp);

    if (jzReadEndRecord(zip, &endRecord))
    {
        printf("Couldn't read ZIP file end record.");
        goto endClose;
    }

    if (jzReadCentralDirectory(zip, &endRecord, recordCallback, files))
    {
        printf("Couldn't read ZIP file central record.");
        goto endClose;
    }

    retval = 0;

endClose:
    zip->close(zip);

    return retval;
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
