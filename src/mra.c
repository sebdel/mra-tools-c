#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mra.h"
#include "utils.h"

char *strtrimleft(char *src) {
    char *p;
    for (p = src; *p && isspace(*p); p++)
        ;
    return p;
}

int read_rpart(XMLNode *node, t_part *part) {
    int j;

    memset(part, 0, sizeof(t_part));

    for (j = 0; j < node->n_attributes; j++) {
        if (strncmp(node->attributes[j].name, "crc", 4) == 0) {
            part->r.crc32 = strtoul(strndup(node->attributes[j].value, 256), (char **)0, 0);
        } else if (strncmp(node->attributes[j].name, "name", 5) == 0) {
            part->r.name = strndup(node->attributes[j].value, 256);
        } else if (strncmp(node->attributes[j].name, "zip", 4) == 0) {
            part->r.zip = strndup(node->attributes[j].value, 256);
        } else if (strncmp(node->attributes[j].name, "repeat", 7) == 0) {
            part->r.repeat = atoi(strndup(node->attributes[j].value, 256));
        } else if (strncmp(node->attributes[j].name, "offset", 7) == 0) {
            part->r.offset = atol(strndup(node->attributes[j].value, 256));
        } else if (strncmp(node->attributes[j].name, "length", 7) == 0) {
            part->r.length = atol(strndup(node->attributes[j].value, 256));
        } else if (strncmp(node->attributes[j].name, "pattern", 8) == 0) {
            part->r.pattern = strndup(node->attributes[j].value, 256);
        } else {
            printf("warning: unknown attribute for regular part: %s\n", node->attributes[j].name);
        }
    }
    if (node->text != NULL) {
        char *trimmed_text = strtrimleft(node->text);
        if (*trimmed_text) {
            if (part->r.name) {
                printf("warning: part %s has a name and data. Data dropped.\n", part->r.name);
            } else {
                if (parse_hex_string(trimmed_text, &(part->r.data), &(part->r.data_length))) {
                    printf("warning: failed to decode part data. Data dropped.\n");
                } else {
                }
            }
        }
    }
    return 0;
}

t_part *read_ipart(XMLNode *node, t_part *part) {
    int i, j;

    memset(part, 0, sizeof(t_part));
    part->is_interleaved = -1;
    part->i.width = 8; // default width = 8 bits

    for (j = 0; j < node->n_attributes; j++) {
        if (strncmp(node->attributes[j].name, "width", 6) == 0) {
            part->i.width = atoi(strndup(node->attributes[j].value, 256));
        } else {
            printf("warning: unknown attribute for interleaved part: %s\n", node->attributes[j].name);
        }
    }
    if (node->text != NULL) {
        char *trimmed_text = strtrimleft(node->text);
        if (*trimmed_text) {
            printf("warning: interleaved part cannot have embedded data. (%s)\n", node->text);
        }
    }
    return part;
}

int read_parts(XMLNode *node, t_part **parts, int *n_parts) {
    int i;

    if (strncmp(node->tag, "part", 5) == 0) {
        (*n_parts)++;
        *parts = (t_part *)realloc(*parts, sizeof(t_part) * (*n_parts));
        read_rpart(node, (*parts) + (*n_parts) - 1);
    } else if (strncmp(node->tag, "group", 5) == 0) {
        (*n_parts)++;
        *parts = (t_part *)realloc(*parts, sizeof(t_part) * (*n_parts));
        t_part *ipart = read_ipart(node, (*parts) + (*n_parts) - 1);
        for (i = 0; i < node->n_children; i++) {
            read_parts(node->children[i], &(ipart->i.parts), &(ipart->i.n_parts));
        }
    } else {
        printf("warning: unexpected token: %s\n", node->tag);
        return -1;
    }
    return 0;
}

void read_rom(XMLNode *node, t_rom *rom) {
    int i, j;

    memset(rom, 0, sizeof(t_rom));

    for (j = 0; j < node->n_attributes; j++) {
        if (strncmp(node->attributes[j].name, "index", 4) == 0) {
            rom->index = atoi(strndup(node->attributes[j].value, 256));
        } else if (strncmp(node->attributes[j].name, "zip", 3) == 0) {
            rom->zip = strndup(node->attributes[j].value, 256);
        } else if (strncmp(node->attributes[j].name, "md5", 3) == 0) {
            rom->md5 = strndup(node->attributes[j].value, 256);
        } else if (strncmp(node->attributes[j].name, "type", 4) == 0) {
            rom->type = strndup(node->attributes[j].value, 256);
        }
    }
    for (i = 0; i < node->n_children; i++) {
        read_parts(node->children[i], &rom->parts, &rom->n_parts);
    }
}

int read_roms(XMLNode *node, t_rom **roms, int *n_roms) {
    int i;

    if (strncmp(node->tag, "rom", 4) == 0) {
        (*n_roms)++;
        *roms = (t_rom *)realloc(*roms, sizeof(t_rom) * (*n_roms));
        read_rom(node, (*roms) + (*n_roms) - 1);
    } else {
        for (i = 0; i < node->n_children; i++) {
            read_roms(node->children[i], roms, n_roms);
        }
    }
}

int read_root(XMLNode *root, t_mra *mra) {
    int i;

    for (i = 0; i < root->n_children; i++) {
        XMLNode *node = root->children[i];

        if (strncmp(node->tag, "name", 5) == 0) {
            mra->name = strndup(node->text, 1024);
        } else if (strncmp(node->tag, "mratimestamp", 13) == 0) {
            mra->mratimestamp = strndup(node->text, 1024);
        } else if (strncmp(node->tag, "mameversion", 12) == 0) {
            mra->mameversion = strndup(node->text, 1024);
        } else if (strncmp(node->tag, "setname", 8) == 0) {
            mra->setname = strndup(node->text, 1024);
        } else if (strncmp(node->tag, "year", 5) == 0) {
            mra->year = strndup(node->text, 1024);
        } else if (strncmp(node->tag, "manufacturer", 13) == 0) {
            mra->manufacturer = strndup(node->text, 1024);
        } else if (strncmp(node->tag, "rbf", 4) == 0) {
            mra->rbf = strndup(node->text, 1024);
        }
        // TODO: parse categories and n_categories
    }
}

int mra_load(char *filename, t_mra *mra) {
    int res;
    XMLDoc *doc = &(mra->_xml_doc);
    XMLNode *root = NULL;

    memset(mra, 0, sizeof(t_mra));

    XMLDoc_init(doc);
    res = XMLDoc_parse_file(filename, doc);
    if (res != 1 || doc->i_root < 0) {
        printf("%s is not a valid xml file\n", filename);
        return -1;
    }
    root = doc->nodes[doc->i_root];
    if (strncmp(root->tag, "misterromdescription", 20) != 0) {
        printf("%s is not a valid MRA file\n", filename);
        return -1;
    }

    read_root(root, mra);
    read_roms(root, &mra->roms, &mra->n_roms);
}

void dump_part(t_part *part) {
    int i;

    if (part->is_interleaved) {
        printf("**** interleaved part start\n");
        printf("    width: %u\n", part->i.width);
        for (i = 0; i < part->i.n_parts; i++) {
            printf("[%d]: \n", i);
            dump_part(part->i.parts + i);
        }
        printf("**** interleaved part end\n");
    } else {
        if (part->r.crc32) printf("    crc32: %u\n", part->r.crc32);
        if (part->r.name) printf("    name: %s\n", part->r.name);
        if (part->r.zip) printf("    zip: %s\n", part->r.zip);
        if (part->r.pattern) printf("    pattern: %s\n", part->r.pattern);
        if (part->r.repeat) printf("    repeat: %d\n", part->r.repeat);
        if (part->r.offset) printf("    offset: %ld\n", part->r.offset);
        if (part->r.length) printf("    length: %ld\n", part->r.length);
        if (part->r.data_length) printf("    data_length: %lu\n", part->r.data_length);
    }
}

int mra_dump(t_mra *mra) {
    int i;

    printf("name: %s\n", mra->name);
    printf("mratimestamp: %s\n", mra->mratimestamp);
    printf("mameversion: %s\n", mra->mameversion);
    printf("setname: %s\n", mra->setname);
    printf("year: %s\n", mra->year);
    printf("manufacturer: %s\n", mra->manufacturer);
    printf("rbf: %s\n", mra->rbf);
    for (i = 0; i < mra->n_categories; i++) {
        printf("category[%d]: %s\n", i, mra->name);
    }

    printf("nb roms: %d\n", mra->n_roms);
    for (i = 0; i < mra->n_roms; i++) {
        int j;
        t_rom *rom = mra->roms + i;

        printf("rom[%d]:\n", i);
        printf("  index: %d\n", rom->index);
        printf("  zip: %s\n", rom->zip);
        printf("  md5: %s\n", rom->md5);
        printf("  type: %s\n", rom->type);

        for (j = 0; j < rom->n_parts; j++) {
            printf("  part[%d]:\n", j);
            dump_part(rom->parts + j);
        }
    }
}

int mra_get_next_rom0(t_mra *mra, int start_index) {
    int i;

    if (start_index >= mra->n_roms) {
        return -1;
    }
    for (i = start_index; i < mra->n_roms; i++) {
        if (mra->roms[i].index == 0) {
            return i;
        }
    }
    return -1;  // ROM 0 not found
}
