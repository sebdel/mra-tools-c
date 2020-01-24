#ifndef _UNZIP_H_
#define _UNZIP_H_

#include <stdint.h>
#include "globals.h"

typedef struct s_file {
    char *name;
    uint32_t crc32;
    unsigned char *data;
    int size;
} t_file;

int unzip_file(char *file, t_file **files, int *n_files);

#endif