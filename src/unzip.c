#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "junzip.h"
#include "unzip.h"

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

