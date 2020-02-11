#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arc.h"
#include "utils.h"

#define MAX_LINE_LENGTH 256

char *format_bits(t_dip_switch *dip) {
    char buffer[256] = "O";
    int n = 1;
    char *token;

    // Parse bits first
    while (token = strtok(n == 1 ? dip->bits : NULL, ",")) {
        char c = atoi(token);
        buffer[n++] = (c < 10) ? ('0' + c) : 'A' + c - 10;
    }
    buffer[n] = '\0';

    if (!dip->ids) {
        if (n - 1 > 1) {
            printf("error while parsing dip switch: number of bits > 1 but no ids defined.\n");
            return NULL;
        }
        buffer[0] = 'T';
    }

    return strdup(buffer);
}

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
        n = snprintf(buffer, MAX_LINE_LENGTH, "RBF=%s\n", str_toupper(mra->rbf));
        fwrite(buffer, 1, n, out);
        if (mod != -1) {
            n = snprintf(buffer, MAX_LINE_LENGTH, "MOD=%d\n", mod);
            fwrite(buffer, 1, n, out);
        }
    }
    for (i = 0; i < mra->n_switches; i++) {
        if (mra->switches[i].ids) {
            n = snprintf(buffer, MAX_LINE_LENGTH, "CONF=\"%s,%s,%s\"\n", format_bits(mra->switches + i), mra->switches[i].name, mra->switches[i].ids);
        } else {
            n = snprintf(buffer, MAX_LINE_LENGTH, "CONF=\"%s,%s\"\n", format_bits(mra->switches + i), mra->switches[i].name);
        }
        fwrite(buffer, 1, n, out);
    }
    fclose(out);
    return 0;
}
