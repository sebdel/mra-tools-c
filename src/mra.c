#include <string.h>
#include <stdlib.h>

#include "mra.h"

void mra_read_rom(XMLNode *node, t_rom *rom)
{
    int i;

    if (strncmp(node->tag, "rom", 4) == 0)
    {
        int j;

        for (j = 0; j < node->n_attributes; j++)
        {
            if (strncmp(node->attributes[j].name, "index", 4) == 0)
            {
                rom->index = strndup(node->attributes[j].value, 256);
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
    }
    else
    {
        for (i = 0; i < node->n_children; i++)
        {
            mra_read_rom(node->children[i], rom);
        }
    }
}

void mra_read_files(XMLNode *node, t_files *files)
{
    int i;

    if (strncmp(node->tag, "part", 4) == 0)
    {
        if (node->n_attributes > 0 && strncmp(node->attributes[0].name, "name", 4) == 0)
        {
            files->file_names = (char **)realloc(files->file_names, sizeof(char *) * (files->n_files + 1));
            files->file_names[files->n_files++] = strndup(node->attributes[0].value, 256);
        }
    }
    for (i = 0; i < node->n_children; i++)
    {
        mra_read_files(node->children[i], files);
    }
}
