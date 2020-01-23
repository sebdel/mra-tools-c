#include <string.h>
#include <stdint.h> // define SIZE_MAX
#include <stdio.h>

#include "utils.h"

const int read_hex_char(char c)
{
  return c >= '0' && c <= '9' ? c - '0' :
         c >= 'a' && c <= 'f' ? 10 + c - 'a' :
         c >= 'A' && c <= 'F' ? 10 + c - 'A' :
         -1;
}

int parse_hex_string(char *hexstr, unsigned char **data, size_t *length)
{
  char *ptr = hexstr;
  char c;

  // We prealloc (hexstr / 2 + 1) bytes, which is in the ball park of what we actually need.
  // It's at least big enough and we will downsize it anyway.
  size_t size = strnlen(hexstr, SIZE_MAX) / 2 + 1;
  *data = (unsigned char *)malloc(sizeof(unsigned char) * size);
  *length = 0;
  while (c = *ptr++)
  {
    static int state = 0; // 0: wait for MSQ, 1: wait for LSQ
    int lsq, msq;

    if (state == 0)
    {
      msq = read_hex_char(c);
      if (msq != -1) {
        state = 1;
      }
    } else {
      lsq = read_hex_char(c);
      if (lsq != -1) {
        (*data)[(*length)++] = (msq << 4) + lsq;
        state = 0;
      } else {
        // single digit value, we don't support that (we could)
        printf("single digit value, we don't support that!\n");

        free(*data);
        *data = NULL;
        *length = 0;
        return -1;
      }
    }
  }
  *data = (unsigned char *)realloc(*data, sizeof(unsigned char *) * *length);
  return 0;
}
