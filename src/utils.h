#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdlib.h>

int parse_hex_string(char *hexstr, unsigned char **data, size_t *length);
void sprintf_md5(char *dest, unsigned char *md5);
int file_exists(char *filename);

#endif