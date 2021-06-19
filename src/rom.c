#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "md5.h"
#include "rom.h"
#include "unzip.h"
#include "utils.h"

t_file *files = NULL;
int n_files = 0;

int get_file_by_crc(t_file *files, int n_files, uint32_t crc) {
    int i;

    if (trace > 0) {
        printf("looking for crc: %08x\n", crc);
    }

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
    static size_t global_offset = 0;
    int i;

    if (data) {
        if (part->is_group) {
            int n_writes = part->g.repeat ? part->g.repeat : 1;
            for (i = 0; i < n_writes; i++) {
                fwrite(data, 1, data_length, out);
                MD5_Update(md5_ctx, data, data_length);
                if (verbose) {
                    printf("writing %lu bytes @ %08lX\n", data_length, global_offset);
                }
                global_offset += data_length;
            }
        } else {
            if (part->p.offset >= data_length) {
                printf("warning: offset set past the part size. Skipping part.\n");
                return 0;
            } else {
                int n_writes = part->p.repeat ? part->p.repeat : 1;
                size_t length = (part->p.length && (part->p.length < (data_length - part->p.offset))) ? part->p.length : (data_length - part->p.offset);
                if (verbose) {
                    printf("writing %lu bytes @ %08lX\n", length * n_writes, global_offset);
                }
                for (i = 0; i < n_writes; i++) {
                    fwrite(data + part->p.offset, 1, length, out);
                    MD5_Update(md5_ctx, data + part->p.offset, length);
                    global_offset += length;
                }
            }
        }
    }

    return 0;
}

int get_data(t_part *part, uint8_t **data, size_t *size) {
    int n;

    if (part->p.zip) {
        printf("Support of part with zip attributes is not implemented!\n");
        return -1;
    }

    n = -1;
    if (part->p.name || part->p.crc32) {
        if (part->p.crc32) {  // First, try to identify file by crc
            n = get_file_by_crc(files, n_files, part->p.crc32);
            if (n >= 0) {
                if (verbose) {
                    printf("part selected by CRC (%08X)\n", part->p.crc32);
                }
            }
        }
        if (n == -1 && part->p.name) {  // then by name
            n = get_file_by_name(files, n_files, part->p.name);
            if (n >= 0) {
                if (verbose) {
                    printf("part selected by name (%s)\n", part->p.name);
                }
            }
        }
    }
    if (n == -1 && !part->p.data) {  // no file, no data => part not found
        printf("part not found in zip: %s (%08x)\n", part->p.name, part->p.crc32);
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
        *data = part->p.data;
        *size = part->p.data_length;
    }

    return 0;
}

int write_part(FILE *out, MD5_CTX *md5_ctx, t_part *part) {
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

/*
    The pattern

    This is a key concept for advanced byte swapping (interleaving parts, manipulating endianness, etc...)

    First, the patterns are defined within the context of the group. A group has a width attribute defining the length of a value (in bits).
    All parts inside the group have a pattern that define how they willl contribute to rebuilding the value.
    Each character in the pattern attribute means "read 1 byte from this part at that offset". So, the number of chars in the pattern
    defines how many bytes are provided on each pass. So, the total number of chars in patterns in a group must be equal to the group width / 8.
    
    Note1: If group width is not specified, it's set to 8 by default.
           If a pattern is not specified it actually means pattern="0" (ie. read 1 byte at a time).

    Examples:
    <group width="16">
        <part name="file1" />
        <part name="file2" />
    </group>
    Will write a stream of 16 bits words, with file1 providing bytes at even offsets and file2 providing bytes at odd offsets.

    <group width="32">
        <part name="file1" pattern="01" />
        <part name="file2" pattern="10" />
    </group>
    Will write a stream of 32 bits long words, reading 16 bits from each file, byte-swapping file2.
*/
int parse_pattern(char *pattern, int **byte_offsets, int *n_src_bytes) {
    if (!pattern) {
        if (trace > 0) printf("pattern not set, defaulting to \"0\" (8 bits)\n");
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

int write_group(FILE *out, MD5_CTX *md5_ctx, t_part *part) {
    int i;

    if (!part->g.is_interleaved) {
        printf("%s:%d: error: non interleaved groups are not implemented\n", __FILE__, __LINE__);
        return -1;
    }
    if (part->g.n_parts == 0) {
        printf("warning: empty group\n");
        return 0;
    }

    // Allocate, load data sources and parse patterns for children of the group
    int **byte_offsets = (int **)calloc(part->g.n_parts, sizeof(int *));
    int *n_src_bytes = (int *)calloc(part->g.n_parts, sizeof(int));
    uint8_t **data = (uint8_t **)calloc(part->g.n_parts, sizeof(uint8_t *));
    size_t *size = (size_t *)calloc(part->g.n_parts, sizeof(size_t));

    int n_dest_bytes = part->g.width >> 3;  // number of bytes per value defined by width attribute

    for (i = 0; i < part->g.n_parts; i++) {
        int res;
        t_part *p_part = part->g.parts + i; 

        res = get_data(p_part, data + i, size + i);
        // apply offset and length attribute
        if (p_part->p.offset + p_part->p.length > size[i]) {
            printf("%s:%d: error: part offset and length exceeds data size\n", __FILE__, __LINE__);
            return -2;
        }
        data[i] += p_part->p.offset;
        if (p_part->p.length) {
            size[i] = p_part->p.length; 
        }

        if (res) {
            return res;
        }
        res = parse_pattern(part->g.parts[i].p.pattern, byte_offsets + i, n_src_bytes + i);
        if (res) {
            return res;
        }
    }

    // Sanity checks on the data/width/pattern combinations
    int n_bytes_value = 0;                       // number of bytes per value accumulated over patterns
    size_t n_values = size[0] / n_src_bytes[0];  // number of values defined by part #0
    for (i = 0; i < part->g.n_parts; i++) {
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
        for (int j = 0; j < part->g.n_parts; j++) {     // for each value, iterate over parts
            for (int k = 0; k < n_src_bytes[j]; k++) {  // for each part, iterate over the pattern
                size_t byte_offset = i * n_src_bytes[j] + byte_offsets[j][k];
                if (trace > 1) printf("i, j, k, offset: %d , %d, %d, %lu\n", i, j, k, byte_offset);
                *dest++ = data[j][byte_offset];
            }
        }
    }

    if (write_to_rom(out, md5_ctx, buffer, total_bytes, part)) {
        return -1;
    }

    return 0;
}

static char *get_zip_filename(char *filename, t_string_list *dirs) {
    int i;

    for (i = 0; i < dirs->n_elements; i++) {
        char *result;
        int length = strnlen(dirs->elements[i], 1024) + strnlen(filename, 1024);
        result = (char *)malloc(sizeof(char) * (length + 2));
        snprintf(result, 2050, "%s/%s", dirs->elements[i], filename);

        if (file_exists(result)) {
            return result;
        }
        free(result);
    }

    if (file_exists(filename)) {
        return strndup(filename, 1024);
    }

    return NULL;
}

int write_rom(t_mra *mra, t_string_list *dirs, char *rom_filename) {
    char *zip_filename;
    t_rom *rom;
    int rom_index=0;
    int i, res;

    // Look for first ROM with index 0
    rom_index = mra_get_next_rom0(mra, rom_index);
    if (rom_index == -1) {
        printf("%s:%d: error: ROM0 not found in MRA.\n", __FILE__, __LINE__);
        return -1;
    }
    rom = mra->roms + rom_index;

    // Load all zip files
    for (i = 0; i < rom->zip.n_elements; i++) {
        char *zip_filename;

        // Look for zip file (first in user defined dir, then in current dir)
        zip_filename = get_zip_filename(rom->zip.elements[i], dirs);
        if (!zip_filename) {
            printf("warning: zip file not found: %s\n", rom->zip.elements[i]);
            continue;
        }
        if (verbose) {
            printf("Uncompressing zip file: %s\n", zip_filename);
        }
        res = unzip_file(zip_filename, &files, &n_files);
        if (res != 0) {
            printf("warning: failed to unzip file: %s\n", zip_filename);
        }
    }

    if (verbose) {
        printf("FILE\t\tSIZE\tCRC\n");
        printf("----\t\t----\t---\n");
        for (i = 0; i < n_files; i++) {
            printf("%s\t\t%d\t%X\n", files[i].name, files[i].size, files[i].crc32);
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

        if (part->is_group) {
            write_group(out, &md5_ctx, part);
        } else {
            write_part(out, &md5_ctx, part);
        }
    }

    // Apply patches before we close the file
    for(i = 0; i < rom->n_patches; i++) {
        fseek(out, rom->patches[i].offset, SEEK_SET);
        fwrite(rom->patches[i].data, 1, rom->patches[i].data_length, out);
    }

    // Done
    fclose(out);
    MD5_Final(md5, &md5_ctx);
    sprintf_md5(md5_string, md5);
    if (verbose) {
        printf("%s\t%s\n", md5_string, rom_filename);
    }
    if (rom->md5) {
        if( strncmp(rom->md5,"None",5)!=0 ) {
            if (strncmp(rom->md5, md5_string, 33)) {
                printf("warning: md5 mismatch! (found: %s, expected: %s)\n", md5_string, rom->md5);
            } else if (verbose) {
                printf("MD5s match! (%s)\n", rom->md5);
            }
        }
    }
    return 0;
}
