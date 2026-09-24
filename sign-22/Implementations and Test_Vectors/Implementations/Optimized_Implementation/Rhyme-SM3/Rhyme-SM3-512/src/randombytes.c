#include <stdio.h>
#include <stdlib.h>
#include "randombytes.h"
void randombytes(uint8_t *out, size_t outlen) {
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f || fread(out, 1, outlen, f) != outlen) { abort(); }
    fclose(f);
}
