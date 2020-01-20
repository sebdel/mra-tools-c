#ifndef _GLOBALS_H_
#define _GLOBALS_H_

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

extern int trace;

#endif
