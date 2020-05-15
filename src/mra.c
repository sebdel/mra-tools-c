#include "mra.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

int read_patch(XMLNode *node, t_patch *patch) {
    int j;

    memset(patch, 0, sizeof(t_patch));
    for (j = 0; j < node->n_attributes; j++) {
        if (strncmp(node->attributes[j].name, "offset", 7) == 0) {
            // offset can be decimal or hexa with 0x prefix
            patch->offset = strtoul(strndup(node->attributes[j].value, 256), NULL, 0);
        }
    }
    if (node->text != NULL) {
        char *trimmed_text = str_trimleft(node->text);
        if (*trimmed_text) {
            if (parse_hex_string(trimmed_text, &(patch->data), &(patch->data_length))) {
                printf("warning: failed to decode patch data. Data dropped.\n");
            }
        }
    }
}

void get_pattern_from_map(char *map, uint8_t **pattern, int *map_index) {
    int i, j;
    int n = strnlen(map, 256);
    char first_found = 0;

    *pattern = (char *)malloc(n + 1);

    for (i = n - 1, j = 0; i >= 0; i--) {
        if (map[i] != '0') {
            if (!first_found) {
                first_found = -1;
                (*map_index) = n - i - 1;
            }
            (*pattern)[j++] = map[i] - 1;
        }
    }
    (*pattern)[j] = '\0';

    if (trace > 0) printf("map=0x%s => pattern=\"%s\"\n", map, *pattern);
}

int read_part(XMLNode *node, t_part *part, int parent_index) {
    int j;

    memset(part, 0, sizeof(t_part));

    for (j = 0; j < node->n_attributes; j++) {
        if (strncmp(node->attributes[j].name, "crc", 4) == 0) {
            // CRC is read as hexa no matter what
            part->p.crc32 = strtoul(strndup(node->attributes[j].value, 256), NULL, 16);
        } else if (strncmp(node->attributes[j].name, "name", 5) == 0) {
            part->p.name = strndup(node->attributes[j].value, 256);
        } else if (strncmp(node->attributes[j].name, "zip", 4) == 0) {
            part->p.zip = strndup(node->attributes[j].value, 256);
        } else if (strncmp(node->attributes[j].name, "repeat", 7) == 0) {
            // repeat can be decimal or hexa with 0x prefix
            part->p.repeat = strtoul(strndup(node->attributes[j].value, 256), NULL, 0);
        } else if (strncmp(node->attributes[j].name, "offset", 7) == 0) {
            // offset can be decimal or hexa with 0x prefix
            part->p.offset = strtoul(strndup(node->attributes[j].value, 256), NULL, 0);
        } else if (strncmp(node->attributes[j].name, "length", 7) == 0 ||
                   strncmp(node->attributes[j].name, "size", 5) == 0) {
            // length/size can be decimal or hexa with 0x prefix
            part->p.length = strtoul(strndup(node->attributes[j].value, 256), NULL, 0);
        } else if (strncmp(node->attributes[j].name, "pattern", 8) == 0) {
            part->p.pattern = strndup(node->attributes[j].value, 256);
            part->p._map_index = parent_index;
        } else if (strncmp(node->attributes[j].name, "map", 4) == 0) {
            get_pattern_from_map(node->attributes[j].value, &(part->p.pattern), &(part->p._map_index));
        } else {
            printf("warning: unknown attribute for regular part: %s\n", node->attributes[j].name);
        }
    }
    if (node->text != NULL) {
        char *trimmed_text = str_trimleft(node->text);
        if (*trimmed_text) {
            if (part->p.name) {
                printf("warning: part %s has a name and data. Data dropped.\n", part->p.name);
            } else {
                if (parse_hex_string(trimmed_text, &(part->p.data), &(part->p.data_length))) {
                    printf("warning: failed to decode part data. Data dropped.\n");
                } else {
                }
            }
        }
    }
    return 0;
}

t_part *read_group(XMLNode *node, t_part *part) {
    int i, j;

    memset(part, 0, sizeof(t_part));
    part->is_group = -1;
    part->g.width = 8;            // default width = 8 bits
    part->g.is_interleaved = -1;  // groups are interleaved by default
    for (j = 0; j < node->n_attributes; j++) {
        if (strncmp(node->attributes[j].name, "width", 6) == 0) {
            part->g.width = atoi(strndup(node->attributes[j].value, 256));
        } else if (strncmp(node->attributes[j].name, "output", 7) == 0) {
            part->g.width = atoi(strndup(node->attributes[j].value, 256));
        } else if (strncmp(node->attributes[j].name, "repeat", 7) == 0) {
            part->g.repeat = atoi(strndup(node->attributes[j].value, 256));
        } else if (strncmp(node->attributes[j].name, "interleaved", 12) == 0) {
            part->g.is_interleaved = atoi(strndup(node->attributes[j].value, 256));
        } else {
            printf("warning: unsupported attribute for group: %s\n", node->attributes[j].name);
        }
    }
    if (node->text != NULL) {
        char *trimmed_text = str_trimleft(node->text);
        if (*trimmed_text) {
            printf("warning: groups cannot have embedded data. (%s)\n", node->text);
        }
    }
    return part;
}

static int cmp_part_map(const void *p1, const void *p2) {

    return ((t_part *)p1)->p._map_index - ((t_part *)p2)->p._map_index;
}

int read_parts(XMLNode *node, t_part **parts, int *n_parts) {
    int i;
    char *part_types[] = {"part", "group", "interleave"};
    t_part *part;

    for (i = 0; i < 3; i++)
        if (strncmp(node->tag, part_types[i], 20) == 0)
            break;

    if (i < 3) {
        (*n_parts)++;
        *parts = (t_part *)realloc(*parts, sizeof(t_part) * (*n_parts));
        part = (*parts) + (*n_parts) - 1;

        switch (i) {
            case 0:  // part
                read_part(node, part, (*n_parts) - 1);
                break;
            case 1:  // group
            case 2:  // interleave
            {
                t_part *group = read_group(node, part);
                for (i = 0; i < node->n_children; i++) {
                    read_parts(node->children[i], &(group->g.parts), &(group->g.n_parts));
                }
                qsort(group->g.parts, group->g.n_parts, sizeof(t_part), cmp_part_map);
            } break;
            default:  // won't happen
                break;
        }
    } else if (node->tag_type != TAG_COMMENT) {
        printf("warning: unexpected token in rom node: %s\n", node->tag);
        return -1;
    }

    return 0;
}

void read_rom(XMLNode *node, t_rom *rom) {
    int i, j;

    memset(rom, 0, sizeof(t_rom));

    for (j = 0; j < node->n_attributes; j++) {
        if (strncmp(node->attributes[j].name, "index", 6) == 0) {
            rom->index = atoi(strndup(node->attributes[j].value, 256));
        } else if (strncmp(node->attributes[j].name, "zip", 4) == 0) {
            string_list_add(&rom->zip, strndup(node->attributes[j].value, 256));
        } else if (strncmp(node->attributes[j].name, "md5", 4) == 0) {
            if (strncmp(node->attributes[j].value, "none", 256) != 0) {
                rom->md5 = strndup(node->attributes[j].value, 256);
            }
        } else if (strncmp(node->attributes[j].name, "type", 5) == 0) {
            string_list_add(&rom->type, strndup(node->attributes[j].value, 256));
        }
    }
    for (i = 0; i < node->n_children; i++) {
        if (strncmp(node->children[i]->tag, "patch", 6) == 0) {
            rom->n_patches++;
            rom->patches = (t_patch *)realloc(rom->patches, sizeof(t_patch) * rom->n_patches);
            read_patch(node->children[i], rom->patches + rom->n_patches - 1);
        } else {
            read_parts(node->children[i], &rom->parts, &rom->n_parts);
        }
    }
}

void read_dip_switch(XMLNode *node, t_dip *dip_switch) {
    int i;

    memset(dip_switch, 0, sizeof(t_dip));
    for (i = 0; i < node->n_attributes; i++) {
        if (strncmp(node->attributes[i].name, "bits", 5) == 0) {
            dip_switch->bits = strndup(node->attributes[i].value, 256);
        } else if (strncmp(node->attributes[i].name, "name", 5) == 0) {
            dip_switch->name = strndup(node->attributes[i].value, 256);
        } else if (strncmp(node->attributes[i].name, "ids", 5) == 0) {
            dip_switch->ids = strndup(node->attributes[i].value, 256);
        }
    }
}

int read_switches(XMLNode *node, t_switches *switches) {
    int i;

    memset(switches, 0, sizeof(t_switches));
    switches->defaults = ~0;

    // Read all attributes
    for (i = 0; i < node->n_attributes; i++) {
        XMLAttribute *attr = &node->attributes[i];
        if (strncmp(attr->name, "base", 10) == 0) {
            switches->base = strtol(strndup(attr->value, 256), NULL, 0);
        } else if (strncmp(attr->name, "default", 8) == 0) {
            int a, b, c, d, n;  // up to three values
            n = sscanf(attr->value, "%X,%X,%X,%X", &a, &b, &c, &d);
            if (n-- > 0) switches->defaults &= (a | 0xffffff00);
            if (n-- > 0) switches->defaults = (switches->defaults << 8) | b;
            if (n-- > 0) switches->defaults = (switches->defaults << 8) | c;
            if (n-- > 0) switches->defaults = (switches->defaults << 8) | d;
        } else if (strncmp(attr->name, "page_id", 8) == 0) {
            switches->page_id = strtol(strndup(attr->value, 256), NULL, 0);
        } else if (strncmp(attr->name, "page_name", 10) == 0) {
            switches->page_name = strndup(attr->value, 26);
        }
    }

    // Read DIPs
    for (i = 0; i < node->n_children; i++) {
        XMLNode *child = node->children[i];
        if (strncmp(child->tag, "dip", 4) == 0) {
            switches->n_dips++;
            switches->dips = (t_dip *)realloc(switches->dips, sizeof(t_dip) * (switches->n_dips));
            read_dip_switch(child, switches->dips + switches->n_dips - 1);
        }
    }
    return 0;
}

void read_roms(XMLNode *node, t_rom **roms, int *n_roms) {
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

void read_rbf(XMLNode *node, t_rbf *rbf) {
    memset(rbf, 0, sizeof(t_rbf));

    for (int i = 0; i < node->n_attributes; i++) {
        XMLAttribute *attr = &node->attributes[i];
        if (strcmp(attr->name, "alt") == 0) {
            rbf->alt_name = strndup(attr->value, 9);
        }
    }
    rbf->name = strndup(node->text, 1024);
}

void read_root(XMLNode *root, t_mra *mra) {
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
            read_rbf(node, &mra->rbf);
        } else if (strncmp(node->tag, "category", 9) == 0) {
            string_list_add(&mra->categories, node->text);
        } else if (strncmp(node->tag, "switches", 9) == 0) {
            read_switches(node, &mra->switches);
        }
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

    return 0;
}

void dump_part(t_part *part) {
    int i;

    if (part->is_group) {
        printf("**** group start\n");
        printf("    is_interleaved: %s\n", part->g.is_interleaved ? "true" : "false");
        printf("    width: %u\n", part->g.width);
        printf("    repeat: %d\n", part->g.repeat);
        for (i = 0; i < part->g.n_parts; i++) {
            printf("[%d]: \n", i);
            dump_part(part->g.parts + i);
        }
        printf("**** group end\n");
    } else {
        if (part->p.crc32) printf("    crc32: %08x\n", part->p.crc32);
        if (part->p.name) printf("    name: %s\n", part->p.name);
        if (part->p.zip) printf("    zip: %s\n", part->p.zip);
        if (part->p.pattern) {
            printf("    pattern: %s\n", part->p.pattern);
            printf("    _map_index: %d\n", part->p._map_index);
        }
        if (part->p.repeat) printf("    repeat: %u (0x%04x)\n", part->p.repeat, part->p.repeat);
        if (part->p.offset) printf("    offset: %u (0x%04x)\n", part->p.offset, part->p.offset);
        if (part->p.length) printf("    length: %u (0x%04x)\n", part->p.length, part->p.length);
        if (part->p.data_length) printf("    data_length: %lu\n", part->p.data_length);
    }
}

int mra_dump(t_mra *mra) {
    int i;

    if (mra->name) printf("name: %s\n", mra->name);
    if (mra->mratimestamp) printf("mratimestamp: %s\n", mra->mratimestamp);
    if (mra->mameversion) printf("mameversion: %s\n", mra->mameversion);
    if (mra->setname) printf("setname: %s\n", mra->setname);
    if (mra->year) printf("year: %s\n", mra->year);
    if (mra->manufacturer) printf("manufacturer: %s\n", mra->manufacturer);
    if (mra->rbf.name) printf("rbf name: %s\n", mra->rbf.name);
    if (mra->rbf.alt_name) printf("rbf alternative name: %s\n", mra->rbf.alt_name);
    for (i = 0; i < mra->categories.n_elements; i++) {
        printf("category[%d]: %s\n", i, mra->categories.elements[i]);
    }
    printf("switches: default=0x%08X, base=%d\n", mra->switches.defaults, mra->switches.base);
    printf("nb dips: %d\n", mra->switches.n_dips);
    for (i = 0; i < mra->switches.n_dips; i++) {
        printf("  dip[%d]: %s,%s,%s\n", i, mra->switches.dips[i].bits, mra->switches.dips[i].name, mra->switches.dips[i].ids);
    }

    for (i = 0; i < mra->n_roms; i++) {
        int j;
        t_rom *rom = mra->roms + i;

        printf("\nrom[%d]:\n", i);
        printf("  index: %d\n", rom->index);
        if (rom->md5) printf("  md5: %s\n", rom->md5);
        if (rom->type.n_elements) printf("  ============\n");
        for (j = 0; j < rom->type.n_elements; j++) {
            printf("  type[%d]: %s\n", j, rom->type.elements[j]);
        }
        if (rom->zip.n_elements) printf("  ============\n");
        for (j = 0; j < rom->zip.n_elements; j++) {
            printf("  zip[%d]: %s\n", j, rom->zip.elements[j]);
        }
        if (rom->n_parts) printf("  ============\n");
        for (j = 0; j < rom->n_parts; j++) {
            printf("  part[%d]:\n", j);
            dump_part(rom->parts + j);
        }
        if (rom->n_patches) printf("  ============\n");
        for (j = 0; j < rom->n_patches; j++) {
            printf("  patch[%d]:\n", j);
            printf("    offset: %u (0x%08x)\n", rom->patches[j].offset, rom->patches[j].offset);
            printf("    data_length: %lu (0x%08lx)\n", rom->patches[j].data_length, rom->patches[j].data_length);
        }
    }
}

int mra_get_next_rom0(t_mra *mra, int start_index) {
    return mra_get_rom_by_index(mra, 0, start_index);
}

int mra_get_rom_by_index(t_mra *mra, int index, int start_pos) {
    int i;

    if (start_pos >= mra->n_roms) {
        return -1;
    }
    for (i = start_pos; i < mra->n_roms; i++) {
        if (mra->roms[i].index == index) {
            return i;
        }
    }
    return -1;  // ROM not found
}
