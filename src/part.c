#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "md5.h"
#include "part.h"
#include "unzip.h"

t_file *files = NULL;
int n_files = 0;

void sprintf_md5(char *dest, unsigned char *md5) {
    snprintf(dest, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
             md5[0], md5[1], md5[2], md5[3], md5[4], md5[5], md5[6], md5[7],
             md5[8], md5[9], md5[10], md5[11], md5[12], md5[13], md5[14], md5[15]);
}

int get_file_by_crc(t_file *files, int n_files, uint32_t crc) {
    int i;

    for (i = 0; i < n_files; i++) {
        if (files[i].crc32 == crc) {
            if (trace > 0) {
                printf("crc matches for file: %s\n", files[i].name);
            }
            return i;
        }
    }
    return -1;
}

int get_file_by_name(t_file *files, int n_files, char *name) {
    int i;

    for (i = 0; i < n_files; i++) {
        if (strncmp(files[i].name, name, 1024) == 0) {
            if (trace > 0) {
                printf("name matches for file: %s\n", files[i].name);
            }
            return i;
        }
    }
    return -1;
}

int write_to_rom(FILE *out, MD5_CTX *md5_ctx, uint8_t *data, size_t data_length, t_part *part) {
    if (data) {
        if (part->is_interleaved) {
            fwrite(data, 1, data_length, out);
            MD5_Update(md5_ctx, data, data_length);
        } else {
            if (part->r.offset >= data_length) {
                printf("warning: offset set past the part size. Skipping part.\n");
                return 0;
            } else {
                int i;
                int n_writes = part->r.repeat ? part->r.repeat : 1;
                size_t length = (part->r.length && (part->r.length < (data_length - part->r.offset))) ? part->r.length : (data_length - part->r.offset);
                for (i = 0; i < n_writes; i++) {
                    fwrite(data + part->r.offset, 1, length, out);
                    MD5_Update(md5_ctx, data + part->r.offset, length);
                }
            }
        }
    }

    return 0;
}

int get_data(t_part *part, uint8_t **data, size_t *size) {
    int n;

    if (part->r.zip) {
        printf("Support of part with zip attributes is not implemented!\n");
        return -1;
    }

    n = -1;
    if (part->r.crc32)  // First, try to identify file by crc
    {
        n = get_file_by_crc(files, n_files, part->r.crc32);
    }
    if (n == -1 && part->r.name)  // then by name
    {
        n = get_file_by_name(files, n_files, part->r.name);
    }
    if (n == -1 && !part->r.data) {  // no file, no data => part not found
        printf("part not found in zip: %s\n", part->r.name);
        return -1;
    }
    if (n != -1 && trace > 0) {
        printf("file:\n");
        printf("  name: %s\n", files[n].name);
        printf("  size: %d\n", files[n].size);
    }

    if (n != -1) {
        *data = files[n].data;
        *size = files[n].size;
    } else {
        *data = part->r.data;
        *size = part->r.data_length;
    }

    return 0;
}

int write_rpart(FILE *out, MD5_CTX *md5_ctx, t_part *part) {
    int res;
    uint8_t *data;
    size_t size;

    res = get_data(part, &data, &size);
    if (res) {
        return res;
    }

    if (write_to_rom(out, md5_ctx, data, size, part)) {
        return -1;
    }
}

int parse_pattern(char *pattern, int **byte_offsets, int *n_src_bytes) {

    if (!pattern) {
        printf("warning: pattern not set, defaulting to \"0\" (8 bits)\n");
        *n_src_bytes = 1;
        *byte_offsets = (int *)calloc(1, sizeof(int));
        (*byte_offsets)[0] = 0;
        return 0;
    }

    *n_src_bytes = strnlen(pattern, 1024);
    *byte_offsets = (int *)calloc((*n_src_bytes), sizeof(int));

    for (int i = 0; i < (*n_src_bytes); i++) {
        int offset = pattern[i] - '0';
        if (offset >= (*n_src_bytes)) {
            printf("error: invalid pattern, offset > length. (\"%s\")\n", pattern);
            return -1;
        }
        (*byte_offsets)[i] = offset;
    }
    return 0;
}

int write_ipart(FILE *out, MD5_CTX *md5_ctx, t_part *part) {
    int i;

    if (part->i.n_parts == 0) {
        printf("warning: empty ipart\n");
        return 0;
    }

    // Allocate, load data sources and parse patterns for children of the ipart
    int **byte_offsets = (int **)calloc(part->i.n_parts, sizeof(int *));
    int *n_src_bytes = (int *)calloc(part->i.n_parts, sizeof(int));
    uint8_t **data = (uint8_t **)calloc(part->i.n_parts, sizeof(uint8_t *));
    size_t *size = (size_t *)calloc(part->i.n_parts, sizeof(size_t));

    int n_dest_bytes = part->i.width >> 3;  // number of bytes per value defined by width attribute

    for (i = 0; i < part->i.n_parts; i++) {
        int res;

        res = get_data(part->i.parts + i, data + i, size + i);
        if (res) {
            return res;
        }
        res = parse_pattern(part->i.parts[i].r.pattern, byte_offsets + i, n_src_bytes + i);
        if (res) {
            return res;
        }
    }

    // Sanity checks on the data/width/pattern combinations
    int n_bytes_value = 0;                       // number of bytes per value accumulated over patterns
    size_t n_values = size[0] / n_src_bytes[0];  // number of values defined by part #0
    for (i = 0; i < part->i.n_parts; i++) {
        if (trace > 0) printf("size[%d] = %lu\n", i, size[i]);
        if (trace > 0) printf("n_src_bytes[%d] = %d\n", i, n_src_bytes[i]);
        if (trace > 0) printf("bytes_offsets[%d][0] = %d\n", i, byte_offsets[i][0]);
        if (n_values != (size[i] / n_src_bytes[i])) {
            printf("error: interleaved part size mismatch. (%lu vs. %lu)\n", n_values, (size[i] / n_src_bytes[i]));
            return -1;
        }
        n_bytes_value += n_src_bytes[i];
    }
    if (n_bytes_value != n_dest_bytes) {
        printf("error: interleaved group width do not match total bytes in children patterns.\n");
        return -1;
    }

    // Allocate final interleaved buffer
    size_t total_bytes = n_values * n_bytes_value;
    uint8_t *buffer = (uint8_t *)malloc(sizeof(uint8_t) * total_bytes);

    uint8_t *dest = buffer;
    for (i = 0; i < n_values; i++) {                    // iterate over values
        for (int j = 0; j < part->i.n_parts; j++) {     // for each value, iterate over parts
            for (int k = 0; k < n_src_bytes[j]; k++) {  // for each part, iterate over the pattern
                size_t byte_offset = i * n_src_bytes[j] + byte_offsets[j][k];
                if (trace > 0) printf("i, j, k, offset: %d , %d, %d, %lu\n", i, j, k, byte_offset);
                *dest++ = data[j][byte_offset];
            }
        }
    }

    if (write_to_rom(out, md5_ctx, buffer, total_bytes, part)) {
        return -1;
    }

    return 0;
}

int write_rom(t_mra *mra, char *zip_dir, char *rom_filename) {
    char *zip_filename;
    t_rom *rom;
    int rom_index;
    int i, res;

    rom_index = mra_get_next_rom0(mra, rom_index);
    if (rom_index == -1) {
        printf("ROM0 not found in MRA.\n");
        return -1;
    }
    rom = mra->roms + rom_index;

    if (*zip_dir) {
        int length = strnlen(zip_dir, 1024) + strnlen(rom->zip, 1024);
        zip_filename = (char *)malloc(sizeof(char) * (length + 2));

        snprintf(zip_filename, 2050, "%s/%s", zip_dir, rom->zip);
    } else {
        zip_filename = rom->zip;
    }
    if (verbose) {
        printf("Reading zip file: %s\n", zip_filename);
    }

    res = unzip_file(zip_filename, &files, &n_files);
    if (res != 0) {
        printf("\nFailed to unzip file: %s\n", zip_filename);
        return -1;
    }
    if (trace > 0) {
        for (i = 0; i < n_files; i++) {
            printf("%s\t%d\t%X\n", files[i].name, files[i].size, files[i].crc32);
        }
    }

    FILE *out;
    MD5_CTX md5_ctx;
    unsigned char md5[16];
    char md5_string[33];

    out = fopen(rom_filename, "wb");
    MD5_Init(&md5_ctx);

    if (out == NULL) {
        fprintf(stderr, "Couldn't open %s for writing!\n", rom_filename);
        return -1;
    }

    for (i = 0; i < rom->n_parts; i++) {
        int j, n;
        t_part *part = rom->parts + i;

        if (part->is_interleaved) {
            write_ipart(out, &md5_ctx, part);
        } else {
            write_rpart(out, &md5_ctx, part);
        }
    }
    fclose(out);
    MD5_Final(md5, &md5_ctx);
    sprintf_md5(md5_string, md5);
    if (verbose) {
        printf("%s\t%s\n", md5_string, rom_filename);
    }
    if (rom->md5) {
        if (strncmp(rom->md5, md5_string, 33)) {
            printf("warning: md5 mismatch! (found: %s, expected: %s)\n", md5_string, rom->md5);
        } else if (verbose) {
            printf("MD5s match! (%s)\n", rom->md5);
        }
    }
    return 0;
}
