#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sxmlc.h"

typedef struct s_rom
{
    char *index;
    char *zip;
    char *md5;
    char *type;
} t_rom;

void print_usage() {
    printf("Usage: mra-tools <my_file.mra>\n");
}

void extract_rom(XMLNode *node, t_rom *rom) {
    int i;

    if (strncmp(node->tag, "rom", 4) == 0) {
        int j;

        for (j = 0; j < node->n_attributes; j++) {
            if (strncmp(node->attributes[j].name, "index", 4) == 0) {
                rom->index = strndup(node->attributes[j].value, 256);
            } else if (strncmp(node->attributes[j].name, "zip", 3) == 0) {
                rom->zip = strndup(node->attributes[j].value, 256);
            } else if (strncmp(node->attributes[j].name, "md5", 3) == 0) {
                rom->md5 = strndup(node->attributes[j].value, 256);
            } else if (strncmp(node->attributes[j].name, "type", 4) == 0) {
                rom->type = strndup(node->attributes[j].value, 256);
            }
        }
    } else {
        for (i = 0; i < node->n_children; i++) {
            extract_rom(node->children[i], rom);
        }
    }
}

void extract_parts(XMLNode *node, char ***parts, int *n_parts) {
    int i;

    if (strncmp(node->tag, "part", 4) == 0) {
        if (node->n_attributes > 0 && strncmp(node->attributes[0].name, "name", 4) == 0) {
            *parts = (char **) realloc(*parts, sizeof(char *) * (*n_parts + 1));
            (*parts)[(*n_parts)++] = strndup(node->attributes[0].value, 256);
        }
    }
    for (i = 0; i < node->n_children; i++) {
        extract_parts(node->children[i], parts, n_parts);
    }
}

void main(int argc, char **argv) {
    XMLDoc mra;
    t_rom rom;
    int i, res;
    char **parts;
    int n_parts;

    if (argc != 2) {
        print_usage();
        exit(-1);
    }

    printf("Parsing %s\n", argv[1]);
    
    XMLDoc_init(&mra);
    res = XMLDoc_parse_file(argv[1], &mra);
    if (res != 1) {
        printf("%s is not a valid xml file\n", argv[1]);
        exit(-1);
    }
    if (mra.i_root >= 0 && strncmp(mra.nodes[mra.i_root]->tag, "misterromdescription", 20) != 0) {
        printf("%s is not a valid MRA file\n", argv[1]);
        exit(-1);
    }

    memset(&rom, 0, sizeof(t_rom));
    extract_rom(mra.nodes[mra.i_root], &rom);
    printf("rom.index = %s\n", rom.index);
    printf("rom.md5 = %s\n", rom.md5);
    printf("rom.type = %s\n", rom.type);
    printf("rom.zip = %s\n", rom.zip);

    n_parts = 0;
    parts = NULL;
    extract_parts(mra.nodes[mra.i_root], &parts, &n_parts);
    printf("%d parts found:\n", n_parts);
    for (i = 0; i < n_parts; i++) {
        printf("%s\n", parts[i]);
    }

    printf("done!\n");
}