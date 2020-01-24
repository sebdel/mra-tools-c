#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "junzip.h"
#include "unzip.h"

struct s_callback_data {
    t_file **files;
    int *n_files;
};

int processFile(JZFile *zip, t_file *file) {
    JZFileHeader header;
    char filename[1024];

    if (jzReadLocalFileHeader(zip, &header, filename, sizeof(filename))) {
        printf("Couldn't read local file header!");
        return -1;
    }

    file->name = strndup(filename, 1024);
    file->crc32 = header.crc32;
    file->size = header.uncompressedSize;
    if ((file->data = (unsigned char *)malloc(header.uncompressedSize)) == NULL) {
        printf("Couldn't allocate memory!");
        return -1;
    }
    // look for index of filename in files->filenames
    /*    int i, n;
    for (i = 0, n = strlen(filename); i < files->n_files && strncmp(files->file_names[i], filename, n); i++)
        ;*/
    if (trace > 0) {
        printf("%s, %d / %d bytes at offset %08X\n", filename,
               header.compressedSize, header.uncompressedSize, header.offset);
    }

    if (jzReadData(zip, &header, file->data) != Z_OK) {
        printf("Couldn't read file data!");
        free(file->data);
        file->data = NULL;
        return -1;
    }

    return 0;
}

int recordCallback(JZFile *zip, int idx, JZFileHeader *header, char *filename, void *user_data) {
    long offset;
    t_file **files = ((struct s_callback_data *)user_data)->files;
    int *n_files = ((struct s_callback_data *)user_data)->n_files;

    offset = zip->tell(zip);  // store current position

    if (zip->seek(zip, header->offset, SEEK_SET)) {
        printf("Cannot seek in zip file!");
        return 0;  // abort
    }

    (*n_files)++;
    *files = (t_file *)realloc(*files, sizeof(t_file) * (*n_files));

    processFile(zip, (*files) + (*n_files) - 1);  // alters file offset

    zip->seek(zip, offset, SEEK_SET);  // return to position

    return 1;  // continue
}

int unzip_file(char *file, t_file **files, int *n_files) {
    FILE *fp;
    int retval = -1;
    JZEndRecord endRecord;
    JZFile *zip;
    struct s_callback_data user_data = {files, n_files};

    if (!(fp = fopen(file, "rb"))) {
        printf("Couldn't open \"%s\"!", file);
        return -1;
    }

    zip = jzfile_from_stdio_file(fp);

    if (jzReadEndRecord(zip, &endRecord)) {
        printf("Couldn't read ZIP file end record.");
        goto endClose;
    }

    if (jzReadCentralDirectory(zip, &endRecord, recordCallback, &user_data)) {
        printf("Couldn't read ZIP file central record.");
        goto endClose;
    }

    retval = 0;

endClose:
    zip->close(zip);

    return retval;
}
