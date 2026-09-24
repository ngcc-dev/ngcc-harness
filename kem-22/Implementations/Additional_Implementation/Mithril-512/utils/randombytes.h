/* https://github.com/pq-crystals/kyber/blob/main/ref/randombytes.h */

#ifndef RANDOMBYTES_H
#define RANDOMBYTES_H

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    char dummy;
} DRNG_ctx; /* Not used */

int init_random_number(DRNG_ctx *drng, const unsigned char *seed, unsigned long long seed_len_bytes);
int get_random_number(DRNG_ctx *drng, unsigned char *random_number, unsigned long long random_number_len_bits);
void randombytes(uint8_t *out, size_t outlen);

#endif