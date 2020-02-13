#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdlib.h>
#include <ctype.h>

typedef struct s_string_list {
  char **elements;
  int n_elements;
} t_string_list;

#if defined(_WIN32) || defined(_WIN64)
// strndup() is not available on Windows
char *strndup( const char *s1, size_t n);
#endif

char *str_toupper(char *src);

int parse_hex_string(char *hexstr, unsigned char **data, size_t *length);
void sprintf_md5(char *dest, unsigned char *md5);
int file_exists(char *filename);

char *get_path(char *filename);
char *get_basename(char *filename, int strip_extension);
char *get_filename(char *path, char *basename, char *extension);
char *dos_clean_basename(char *filename);

t_string_list *string_list_new(char *element);
char *string_list_add(t_string_list *list, char *element);

#endif