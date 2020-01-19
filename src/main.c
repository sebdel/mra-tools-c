#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sxmlc.h"
#include "junzip.h"

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
    printf("Usage: mra-tools <my_file.mra>\n");
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
    int i, res;
    int mra_dump = 0;
    int verbose = 0;

    if (argc != 2)
    {
        print_usage();
        exit(-1);
    }

    rom_filename = strndup(argv[1], 1024);
    rom_filename[strnlen(rom_filename, 1024) - 4] = 0;
    strncat(rom_filename, ".rom", 4);

    if (verbose)
    {
        printf("Parsing %s to %s\n", argv[1], rom_filename);
    }

    XMLDoc_init(&mra);
    res = XMLDoc_parse_file(argv[1], &mra);
    if (res != 1)
    {
        printf("%s is not a valid xml file\n", argv[1]);
        exit(-1);
    }
    if (mra.i_root >= 0 && strncmp(mra.nodes[mra.i_root]->tag, "misterromdescription", 20) != 0)
    {
        printf("%s is not a valid MRA file\n", argv[1]);
        exit(-1);
    }

    memset(&rom, 0, sizeof(t_rom));
    memset(&files, 0, sizeof(t_files));

    mra_read_rom(mra.nodes[mra.i_root], &rom);
    if (mra_dump)
    {
        printf("rom.index = %s\n", rom.index);
        printf("rom.md5 = %s\n", rom.md5);
        printf("rom.type = %s\n", rom.type);
        printf("rom.zip = %s\n", rom.zip);
    }

    mra_read_files(mra.nodes[mra.i_root], &files);
    if (mra_dump)
    {
        printf("%d file parts found:\n", files.n_files);
        for (i = 0; i < files.n_files; i++)
        {
            printf("%s\n", files.file_names[i]);
        }
    }

    files.data = (unsigned char **)malloc(sizeof(unsigned char *) * files.n_files);
    files.data_size = (long *)malloc(sizeof(long) * files.n_files);
    memset(files.data, 0, sizeof(unsigned char *) * files.n_files);

    res = unzip_file(rom.zip, &files);
    if (res != 0)
    {
        printf("Failed to unzip file: %s\n", rom.zip);
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

    if (verbose)
    {
        printf("done!\n");
    }
}
