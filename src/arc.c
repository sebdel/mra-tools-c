#include <stdio.h>
#include <stdlib.h>

#include "arc.h"

#define MAX_LINE_LENGTH 256

int write_arc(t_mra *mra, char *filename) {
    FILE *out;
    char buffer[MAX_LINE_LENGTH + 1];
    int i, n;

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
        if (mra->mod != -1) {
            n = snprintf(buffer, MAX_LINE_LENGTH, "MOD=\"%d\"\n", mra->mod);
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
