#include "arc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "utils.h"

#define MAX_LINE_LENGTH 256
#define MAX_CONTENT_LENGTH 25
#define MAX_CONF_OPT_LENGTH 128

char *format_bits( t_mra* mra, t_dip *dip ) {
    char buffer[256] = "O";
    int start = 1;
    int n;

    int base = mra->switches.base;
    int page_id = mra->switches.page_id;

    if (page_id >= 1 && page_id <= 9) {
        snprintf(buffer, 256, "P%dO", page_id);
        start = 3;
    }
    n = start;

    char *token = dip->bits;

    // Parse bits first
    while (token = strtok(token, ",")) {
        char c = atoi(token) + (char)base;
        if (c > 61) {
            printf("error while parsing dip switch (%s): required bit position exceeds 61.\n", mra->setname);
            return NULL;
        }
        buffer[n++] = (c < 10) ? ('0' + c) : (c < 36) ? ('A' + c - 10) : ('a' + c - 36);
        token = NULL;
    }
    buffer[n] = '\0';

    if (!dip->ids) {
        if (n - 1 > start) {
            printf("error (%s) while parsing \"%s\" dip switch: number of bits > 1 but no ids defined.\n", mra->setname, dip->name);
            return NULL;
        }
        buffer[start - 1] = 'T';
    }

    return strdup(buffer);
}

int check_ids_len(t_dip *dip) {
    int nlen;
    int tlen;
    char copy[MAX_LINE_LENGTH];
    char *tok;

    nlen = strnlen(dip->name, MAX_LINE_LENGTH);
    strncpy(copy, dip->ids, MAX_LINE_LENGTH);
    tok = strtok(copy, ",");
    tlen = nlen;
    while (tok) {
        int j = strlen(tok);
        tlen += j+1;
        if (tlen > MAX_CONF_OPT_LENGTH) return 1;
        if (nlen + j > MAX_CONTENT_LENGTH) return 1;
        tok = strtok(NULL, ",");
    }
    return 0;
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

    if (mra->switches.n_dips && mra->switches.defaults) {
        n = snprintf(buffer, MAX_LINE_LENGTH, "DEFAULT=0x%llX\n", mra->switches.defaults << mra->switches.base);
        fwrite(buffer, 1, n, out);
    }
    if (mra->switches.page_id && mra->switches.page_name) {
        n = snprintf(buffer, MAX_LINE_LENGTH, "CONF=\"P%d,%s\"\n", mra->switches.page_id, mra->switches.page_name);
        fwrite(buffer, 1, n, out);
    }

    for (i = 0; i < mra->switches.n_dips; i++) {
        t_dip *dip = mra->switches.dips + i;
        if (!strstr(str_tolower(dip->name), "unused")) {
            if (dip->ids) {
                if (check_ids_len(dip)) {
                    printf("warning (%s): dip_content too long for MiST (%s):\n\t%s\t%s\n\n", mra->setname, mra->name, dip->name, dip->ids);
                    continue;
                }
                n = snprintf(buffer, MAX_LINE_LENGTH, "CONF=\"%s,%s,%s\"\n", format_bits( mra, dip ), dip->name, dip->ids);
                strnlen(dip->ids, MAX_LINE_LENGTH);
            } else {
                n = snprintf(buffer, MAX_LINE_LENGTH, "CONF=\"%s,%s\"\n", format_bits( mra, dip ), dip->name);
            }
            if (n >= MAX_LINE_LENGTH) {
                printf("%s:%d: warning (%s): line was truncated while writing in ARC file!\n", __FILE__, __LINE__, mra->setname);
                continue;
            }
            fwrite(buffer, 1, n, out);
        } else {
            printf("warning (%s): \"%s\" dip setting skipped (unused)\n", mra->setname, dip->name);
        }
    }
    fclose(out);
    return 0;
}
