#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "utils.h"

#define MAX_DATA_SIZE (4l * 1024l * 1024l)

static int read_hex_char(char c) {
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? 10 + c - 'a' : c >= 'A' && c <= 'F' ? 10 + c - 'A' : -1;
}

char *get_path(char *filename) {
    char *path = strndup(filename, 1024);
    char *last_slash = path;
    char *p = path;
    char c;

    while (c = *p) {
        if (c == '/')
            last_slash = p;
        p++;
    }
    *last_slash = '\0';

    return path;
}

int file_exists(char *filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

void sprintf_md5(char *dest, unsigned char *md5) {
    snprintf(dest, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
             md5[0], md5[1], md5[2], md5[3], md5[4], md5[5], md5[6], md5[7],
             md5[8], md5[9], md5[10], md5[11], md5[12], md5[13], md5[14], md5[15]);
}

int parse_hex_string(char *hexstr, unsigned char **data, size_t *length) {
    char *ptr = hexstr;
    char c;

    // We prealloc (hexstr / 2 + 1) bytes, which is in the ball park of what we actually need.
    // It's at least big enough and we will downsize it anyway.
    size_t size = strnlen(hexstr, MAX_DATA_SIZE) / 2 + 1;
    *data = (unsigned char *)malloc(sizeof(unsigned char) * size);
    *length = 0;
    while (c = *ptr++) {
        static int state = 0;  // 0: wait for MSQ, 1: wait for LSQ
        int lsq, msq;

        if (state == 0) {
            msq = read_hex_char(c);
            if (msq != -1) {
                state = 1;
            }
        } else {
            lsq = read_hex_char(c);
            if (lsq != -1) {
                (*data)[(*length)++] = (msq << 4) + lsq;
                state = 0;
            } else {
                // single digit value, we don't support that (we could)
                printf("single digit value, we don't support that!\n");

                free(*data);
                *data = NULL;
                *length = 0;
                return -1;
            }
        }
    }
    *data = (unsigned char *)realloc(*data, sizeof(unsigned char *) * *length);
    return 0;
}

t_string_list *string_list_new(char *element) {
    t_string_list *new_list = (t_string_list *)calloc(sizeof(t_string_list), 1);

    if (element) {
        string_list_add(new_list, element);
    }
    return new_list;
}

char *string_list_add(t_string_list *list, char *element) {
    list->n_elements++;
    list->elements = (char **)realloc(list->elements, sizeof(char *) * list->n_elements);
    list->elements[list->n_elements - 1] = strndup(element, 1024);

    return list->elements[list->n_elements - 1];
}
