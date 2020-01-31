#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdlib.h>

typedef struct s_string_list {
  char **elements;
  int n_elements;
} t_string_list;

int parse_hex_string(char *hexstr, unsigned char **data, size_t *length);
void sprintf_md5(char *dest, unsigned char *md5);
int file_exists(char *filename);
char *get_path(char *filename);

t_string_list *string_list_new(char *element);
char *string_list_add(t_string_list *list, char *element);

#endif