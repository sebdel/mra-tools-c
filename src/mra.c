#include <string.h>
#include <stdlib.h>

#include "mra.h"
#include "utils.h"

int read_part(XMLNode *node, t_part *part)
{
    int j;

    memset(part, 0, sizeof(t_part));

    for (j = 0; j < node->n_attributes; j++)
    {
        if (strncmp(node->attributes[j].name, "crc", 4) == 0)
        {
            part->crc32 = strtoul(strndup(node->attributes[j].value, 256), (char **)0, 0);
        }
        else if (strncmp(node->attributes[j].name, "name", 5) == 0)
        {
            part->name = strndup(node->attributes[j].value, 256);
        }
        else if (strncmp(node->attributes[j].name, "zip", 4) == 0)
        {
            part->zip = strndup(node->attributes[j].value, 256);
        }
        else if (strncmp(node->attributes[j].name, "repeat", 7) == 0)
        {
            part->repeat = atoi(strndup(node->attributes[j].value, 256));
        }
        else if (strncmp(node->attributes[j].name, "offset", 7) == 0)
        {
            part->offset = atol(strndup(node->attributes[j].value, 256));
        }
        else if (strncmp(node->attributes[j].name, "length", 7) == 0)
        {
            part->length = atol(strndup(node->attributes[j].value, 256));
        }
    }
    if (node->text != NULL)
    {
        if (part->name) {
            printf("warning: part %s has a name and data. Data dropped.\n", part->name);
        } else {
            if (parse_hex_string(node->text, &(part->data), &(part->data_length))) {
                printf("warning: failed to decode part data. Data dropped.\n");
            } else {

            }
        }
    }
    return 0;
}

int read_parts(XMLNode *node, t_part **parts, int *n_parts)
{
    int i;

    if (strncmp(node->tag, "part", 5) == 0)
    {
        (*n_parts)++;
        *parts = (t_part *)realloc(*parts, sizeof(t_part) * (*n_parts));
        read_part(node, (*parts) + (*n_parts) - 1);
    }
    else
    {
        for (i = 0; i < node->n_children; i++)
        {
            read_parts(node->children[i], parts, n_parts);
        }
    }
}

void read_rom(XMLNode *node, t_rom *rom)
{
    int j;

    memset(rom, 0, sizeof(t_rom));

    for (j = 0; j < node->n_attributes; j++)
    {
        if (strncmp(node->attributes[j].name, "index", 4) == 0)
        {
            rom->index = atoi(strndup(node->attributes[j].value, 256));
        }
        else if (strncmp(node->attributes[j].name, "zip", 3) == 0)
        {
            rom->zip = strndup(node->attributes[j].value, 256);
        }
        else if (strncmp(node->attributes[j].name, "md5", 3) == 0)
        {
            rom->md5 = strndup(node->attributes[j].value, 256);
        }
        else if (strncmp(node->attributes[j].name, "type", 4) == 0)
        {
            rom->type = strndup(node->attributes[j].value, 256);
        }
    }
    read_parts(node, &rom->parts, &rom->n_parts);
}

int read_roms(XMLNode *node, t_rom **roms, int *n_roms)
{
    int i;

    if (strncmp(node->tag, "rom", 4) == 0)
    {
        (*n_roms)++;
        *roms = (t_rom *)realloc(*roms, sizeof(t_rom) * (*n_roms));
        read_rom(node, (*roms) + (*n_roms) - 1);
    }
    else
    {
        for (i = 0; i < node->n_children; i++)
        {
            read_roms(node->children[i], roms, n_roms);
        }
    }
}

int read_root(XMLNode *root, t_mra *mra)
{
    int i;

    for (i = 0; i < root->n_children; i++)
    {
        XMLNode *node = root->children[i];

        if (strncmp(node->tag, "name", 5) == 0)
        {
            mra->name = strndup(node->text, 1024);
        }
        else if (strncmp(node->tag, "mratimestamp", 13) == 0)
        {
            mra->mratimestamp = strndup(node->text, 1024);
        }
        else if (strncmp(node->tag, "mameversion", 12) == 0)
        {
            mra->mameversion = strndup(node->text, 1024);
        }
        else if (strncmp(node->tag, "setname", 8) == 0)
        {
            mra->setname = strndup(node->text, 1024);
        }
        else if (strncmp(node->tag, "year", 5) == 0)
        {
            mra->year = strndup(node->text, 1024);
        }
        else if (strncmp(node->tag, "manufacturer", 13) == 0)
        {
            mra->manufacturer = strndup(node->text, 1024);
        }
        else if (strncmp(node->tag, "rbf", 4) == 0)
        {
            mra->rbf = strndup(node->text, 1024);
        }
        // TODO: parse categories and n_categories
    }
}

int mra_load(char *filename, t_mra *mra)
{
    int res;
    XMLDoc *doc = &(mra->_xml_doc);
    XMLNode *root = NULL;

    memset(mra, 0, sizeof(t_mra));

    XMLDoc_init(doc);
    res = XMLDoc_parse_file(filename, doc);
    if (res != 1 || doc->i_root < 0)
    {
        printf("%s is not a valid xml file\n", filename);
        return -1;
    }
    root = doc->nodes[doc->i_root];
    if (strncmp(root->tag, "misterromdescription", 20) != 0)
    {
        printf("%s is not a valid MRA file\n", filename);
        return -1;
    }

    read_root(root, mra);
    read_roms(root, &mra->roms, &mra->n_roms);
}

int mra_dump(t_mra *mra)
{
    int i;

    printf("name: %s\n", mra->name);
    printf("mratimestamp: %s\n", mra->mratimestamp);
    printf("mameversion: %s\n", mra->mameversion);
    printf("setname: %s\n", mra->setname);
    printf("year: %s\n", mra->year);
    printf("manufacturer: %s\n", mra->manufacturer);
    printf("rbf: %s\n", mra->rbf);
    for (i = 0; i < mra->n_categories; i++)
    {
        printf("category[%d]: %s\n", i, mra->name);
    }

    printf("nb roms: %d\n", mra->n_roms);
    for (i = 0; i < mra->n_roms; i++)
    {
        int j;
        t_rom *rom = mra->roms + i;

        printf("rom[%d]:\n", i);
        printf("  index: %d\n", rom->index);
        printf("  zip: %s\n", rom->zip);
        printf("  md5: %s\n", rom->md5);
        printf("  type: %s\n", rom->type);

        for (j = 0; j < rom->n_parts; j++)
        {
            t_part *part = rom->parts + j;
            printf("  part[%d]:\n", j);
            printf("    crc32: %u\n", part->crc32);
            printf("    name: %s\n", part->name);
            printf("    zip: %s\n", part->zip);
            printf("    repeat: %d\n", part->repeat);
            printf("    offset: %ld\n", part->offset);
            printf("    length: %ld\n", part->length);
            printf("    data_length: %lu\n", part->data_length);
        }
    }
}

int mra_get_next_rom0(t_mra *mra, int start_index)
{
    int i;

    if (start_index >= mra->n_roms)
    {
        return -1;
    }
    for (i = start_index; i < mra->n_roms; i++)
    {
        if (mra->roms[i].index == 0)
        {
            return i;
        }
    }
    return -1; // ROM 0 not found
}
