#include <stdio.h>
#include <stdlib.h>

#include "arc.h"

#define MAX_LINE_LENGTH 256

int write_arc(t_mra *mra, char *filename) {
    FILE *out;
    char buffer[MAX_LINE_LENGTH + 1];
    int i, n;
    int mod = -1;

    /* let's be strict about mod:
        it has to be a single byte in a single part in a ROM with index = "1".
    */
    i = mra_get_rom_by_index(mra, 1, 0);
    if (i != -1 &&
        mra->roms[i].n_parts == 1 &&
        !mra->roms[i].parts[0].is_group &&
        mra->roms[i].parts[0].p.data_length == 1) {
        mod = mra->roms[i].parts[0].p.data[0];
    }

    out = fopen(filename, "wb");
    if (out == NULL) {
        fprintf(stderr, "Couldn't open %s for writing!\n", filename);
        return -1;
    }

    n = snprintf(buffer, MAX_LINE_LENGTH, "[ARC]\n");
    fwrite(buffer, 1, n, out);
    if (mra->rbf) {
        n = snprintf(buffer, MAX_LINE_LENGTH, "RBF=\"%s\"\n", mra->rbf);
        fwrite(buffer, 1, n, out);
        if (mod != -1) {
            n = snprintf(buffer, MAX_LINE_LENGTH, "MOD=\"%d\"\n", mod);
            fwrite(buffer, 1, n, out);
        }
    }
    for (i = 0; i < mra->n_configurations; i++) {
        n = snprintf(buffer, MAX_LINE_LENGTH, "CONF=\"%s,%s,%s\"\n", mra->configurations[i].bits, mra->configurations[i].name, mra->configurations[i].ids);
        fwrite(buffer, 1, n, out);
    }
    fclose(out);
    return 0;
}
