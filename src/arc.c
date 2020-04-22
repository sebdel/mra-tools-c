#include "arc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "utils.h"

#define MAX_LINE_LENGTH 256

char *format_bits(t_dip *dip, int base) {
    char buffer[256] = "O";
    int n = 1;
    char *token = dip->bits;

    // Parse bits first
    while (token = strtok(token, ",")) {
        char c = atoi(token) + (char)base;
        if (c > 31) {
            printf("error while parsing dip switch: required bit position exceeds 31.\n");
            return NULL;
        }
        buffer[n++] = (c < 10) ? ('0' + c) : 'A' + c - 10;
        token = NULL;
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
    int mod = 0;

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
    if (n >= MAX_LINE_LENGTH) printf("%s:%d: warning: line was truncated while writing in ARC file!\n", __FILE__, __LINE__);
    fwrite(buffer, 1, n, out);
    // Write rbf
    if (mra->rbf.name) {
        char *rbf = str_toupper(mra->rbf.alt_name ? mra->rbf.alt_name : mra->rbf.name);
        if (strnlen(rbf, 32) > 8) printf("warning: RBF file name may be too long for MiST\n");

        n = snprintf(buffer, MAX_LINE_LENGTH, "RBF=%s\n", rbf);
        if (n >= MAX_LINE_LENGTH) printf("%s:%d: warning: line was truncated while writing in ARC file!\n", __FILE__, __LINE__);
        fwrite(buffer, 1, n, out);

        if (mod != -1) {
            n = snprintf(buffer, MAX_LINE_LENGTH, "MOD=%d\n", mod);
            if (n >= MAX_LINE_LENGTH) printf("%s:%d: warning: line was truncated while writing in ARC file!\n", __FILE__, __LINE__);
            fwrite(buffer, 1, n, out);
        }
    }
    n = snprintf(buffer, MAX_LINE_LENGTH, "NAME=%s\n", str_toupper(rom_basename));
    if (n >= MAX_LINE_LENGTH) printf("%s:%d: warning: line was truncated while writing in ARC file!\n", __FILE__, __LINE__);
    fwrite(buffer, 1, n, out);

    if (mra->switches.n_dips) {
        n = snprintf(buffer, MAX_LINE_LENGTH, "DEFAULT=0x%08X\n", mra->switches.defaults << mra->switches.base);
        fwrite(buffer, 1, n, out);
    }
    for (i = 0; i < mra->switches.n_dips; i++) {
        t_dip *dip = mra->switches.dips + i;
        if (dip->ids) {
            n = snprintf(buffer, MAX_LINE_LENGTH, "CONF=\"%s,%s,%s\"\n", format_bits(dip, mra->switches.base), dip->name, dip->ids);
        } else {
            n = snprintf(buffer, MAX_LINE_LENGTH, "CONF=\"%s,%s\"\n", format_bits(dip, mra->switches.base), dip->name);
        }
        if (n >= MAX_LINE_LENGTH) printf("%s:%d: warning: line was truncated while writing in ARC file!\n", __FILE__, __LINE__);
        fwrite(buffer, 1, n, out);
    }
    fclose(out);
    return 0;
}
