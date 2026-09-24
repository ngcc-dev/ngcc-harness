#include <stddef.h>
#include <stdint.h>

#include "verify.h"

int verify(
    const uint8_t *a, 
    const uint8_t *b, 
    size_t len_bytes)
{
    size_t i;
    uint8_t diff = 0;

    for (i = 0; i < len_bytes; i++) {
        diff |= a[i] ^ b[i];
    }

    return (-(uint64_t)diff) >> 63; 
}

void cmov(
    uint8_t *r, 
    const uint8_t *x, 
    size_t len_bytes, 
    uint8_t b)
{
    size_t i;
    b = -b; 

    for (i = 0; i < len_bytes; i++) {
        r[i] ^= b & (x[i] ^ r[i]); /* if b is 1, r[i] = x[i]; if b is 0, r[i] = r[i] */
    }
}