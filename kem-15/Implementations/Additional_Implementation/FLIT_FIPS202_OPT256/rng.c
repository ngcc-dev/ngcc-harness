/*
 * SHAKE256-based deterministic random number generator.
 * Replaces the OpenSSL AES256-CTR-DRBG with a fast, self-contained
 * implementation that uses only the FIPS202 SHAKE256 XOF.
 */

#include <string.h>
#include <stdint.h>
#include "fips202.h"

#define RNG_SUCCESS      0
#define RNG_BAD_MAXLEN  -1
#define RNG_BAD_OUTBUF  -2
#define RNG_BAD_REQ_LEN -3

static uint8_t  rng_seed[32] = {0};
static uint64_t rng_ctr   = 0;

void randombytes_init(unsigned char *entropy_input,
                      unsigned char *personalization_string,
                      int security_strength)
{
    (void)security_strength;
    /* XOR personalization string into seed if provided */
    memcpy(rng_seed, entropy_input, 32);
    if (personalization_string)
        for (int i = 0; i < 32; i++)
            rng_seed[i] ^= personalization_string[i];
    rng_ctr = 0;
}

int randombytes(unsigned char *x, unsigned long long xlen)
{
    uint8_t input[40];  /* 32-byte seed || 8-byte counter */
    memcpy(input, rng_seed, 32);
    for (int i = 0; i < 8; i++)
        input[32 + i] = (uint8_t)(rng_ctr >> (56 - 8*i));
    rng_ctr++;

    shake256(x, xlen, input, sizeof(input));
    return RNG_SUCCESS;
}
