#ifndef _MRA_H_
#define _MRA_H_

#include <stdint.h>
#include "globals.h"
#include "sxmlc.h"

typedef struct s_part {
    int is_interleaved;
    union {
        struct s_regular_part {
            char *name;
            char *zip;
            uint32_t crc32;
            int repeat;
            long offset;
            long length;
            unsigned char *pattern;
            unsigned char *data;
            size_t data_length;
        } r;
        struct s_interleaved_part {
            int width;
            struct s_part *parts;
            int n_parts;
        } i;
    };
} t_part;

typedef struct s_rom {
    int index;
    char *zip;
    char *md5;
    char *type;

    t_part *parts;
    int n_parts;
} t_rom;

typedef struct s_mra {
    XMLDoc _xml_doc;

    char *name;
    char *mratimestamp;
    char *mameversion;
    char *setname;
    char *year;
    char *manufacturer;
    char **categories;
    int n_categories;
    char *rbf;

    t_rom *roms;
    int n_roms;
} t_mra;

int mra_load(char *filename, t_mra *mra);
int mra_dump(t_mra *mra);
int mra_get_next_rom0(t_mra *mra, int start_index);

#endif