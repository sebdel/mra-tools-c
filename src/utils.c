#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "utils.h"

#define MAX_DATA_SIZE (4l * 1024l * 1024l)

static int read_hex_char(char c) {
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? 10 + c - 'a' : c >= 'A' && c <= 'F' ? 10 + c - 'A' : -1;
}

#if defined(_WIN32) || defined(_WIN64)

#include <math.h>

// strndup() is not available on Windows
char *strndup(const char *s1, size_t n) {
    char *copy = (char *)malloc(n + 1);
    memcpy(copy, s1, n);
    copy[n] = 0;
    return copy;
};
#endif

char *dos_clean_basename(char *filename, int uppercase) {
    char bad_chars[] = " ()[]{}.!@%^*~<>|:?'\"";
    char *clean_name = (char *)malloc(8 + 1);
    int i;

    memcpy(clean_name, filename, 5);                                    // str_left(filename, 5)
    memcpy(clean_name + 5, filename + strnlen(filename, 1024) - 3, 3);  // str_right(filename, 3)
    clean_name[8] = '\0';
    if (uppercase)
        clean_name = str_toupper(clean_name);

    for (i = 0; i < strlen(bad_chars); i++) {
        char *p;
        if (p = strchr(clean_name, bad_chars[i])) {
            *p = '_';
        }
    }
    printf("filename: %s, clean_name: %s\n", filename, clean_name);

    return clean_name;
}

char *get_path(char *filename) {
    char *path = strndup(filename, 1024);
    char *last_slash = NULL;
    char *p = path;
    char c;

    while (c = *p) {
        if (c == '/')
            last_slash = p;
        p++;
    }
    // Return '.' if no '/' was found in filename
    if (last_slash) {
        *last_slash = '\0';
    } else {
        strncpy(path, ".", 2);
    }
    return path;
}

char *get_basename(char *filename, int strip_extension) {
    char *basename = strndup(filename, 1024);
    char *p = basename;
    char *last_dot = NULL;
    char *last_slash = NULL;
    char c;
    int i = 0;

    while ((c = *p) && (i++ < 1024)) {
        if (c == '.') last_dot = p;
        if (c == '/') last_slash = p;
        p++;
    }
    if (strip_extension && last_dot) *last_dot = '\0';
    return last_slash ? last_slash + 1 : basename;
}

char *get_filename(char *path, char *basename, char *extension) {
    char *filename;
    int n = strnlen(path, 1024) + 1 + strnlen(basename, 1024) + 1;
    char has_separator = path[strnlen(path, 1024) - 1] == '/';

    if (extension) {
        n += strnlen(extension, 1024) + 1;
    }
    filename = (char *)malloc(sizeof(char) * n);
    if (extension) {
        if (has_separator) {
            snprintf(filename, n, "%s%s.%s", path, basename, extension);
        } else {
            snprintf(filename, n, "%s/%s.%s", path, basename, extension);
        }
    } else {
        if (has_separator) {
            snprintf(filename, n, "%s%s", path, basename);
        } else {
            snprintf(filename, n, "%s/%s", path, basename);
        }
    }
    return filename;
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

char *str_toupper(char *src) {
    char *dest = strndup(src, 256);
    char *p = dest;
    while (*p) {
        char c = *p;
        *p++ = toupper(c);
    }
    return dest;
}

char *str_trimleft(char *src) {
    char *p;
    for (p = src; *p && isspace(*p); p++)
        ;
    return p;
}

int parse_hex_string(char *hexstr, unsigned char **data, size_t *length) {
    char *ptr = hexstr;
    char c;
    int state;
    int lsq, msq;

    // We prealloc (hexstr / 2 + 1) bytes, which is in the ball park of what we actually need.
    // It's at least big enough and we will downsize it anyway.
    size_t size = strnlen(hexstr, MAX_DATA_SIZE) / 2 + 1;
    *data = (unsigned char *)malloc(sizeof(unsigned char) * size);
    *length = 0;
    state = 0;  // 0: wait for MSQ, 1: wait for LSQ
    while (c = *ptr++) {
        if (state == 0) {
            msq = read_hex_char(c);
            if (msq != -1) {
                state = 1;
            }
        } else {
            lsq = read_hex_char(c);
            if (lsq != -1) {
                (*data)[(*length)++] = (msq << 4) + lsq;
            } else {
                (*data)[(*length)++] = msq;
            }
            state = 0;
        }
    }
    if (state == 1) {
        (*data)[(*length)++] = msq;
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
    char *token = element;

    while (token = strtok(token, "|")) {
        list->n_elements++;
        list->elements = (char **)realloc(list->elements, sizeof(char *) * list->n_elements);
        list->elements[list->n_elements - 1] = strndup(token, 1024);

        token = NULL;
    }

    return list->elements[list->n_elements - 1];
}
