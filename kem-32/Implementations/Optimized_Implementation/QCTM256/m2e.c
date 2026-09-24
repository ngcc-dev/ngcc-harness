#include <stdlib.h>
#include <string.h>

#include "m2e.h"
#include "rng.h"

static int load_sigma1_candidate(const unsigned char *bits, int j)
{
    size_t bit = (size_t)PARAM_SIGMA1 * (size_t)j;
    size_t byte = bit / 8U;
    unsigned shift = (unsigned)(bit % 8U);
    uint32_t x = (uint32_t)bits[byte] |
                 ((uint32_t)bits[byte + 1U] << 8) |
                 ((uint32_t)bits[byte + 2U] << 16);

    return (int)((x >> shift) & ((1U << PARAM_SIGMA1) - 1U));
}

static int fixed_weight_from_bits(const unsigned char *bits, int *error)
{
    int count = 0;
    int j;
    unsigned char seen[BITS_TO_BYTES(LENGTH)];

    if (bits == NULL || error == NULL) {
        return FAIL;
    }

    for (j = 0; j < PARAM_RHO && count < NB_ERRORS; j++) {
        int d = load_sigma1_candidate(bits, j);

        if (d < LENGTH) {
            error[count++] = d;
        }
    }

    if (count < NB_ERRORS) {
        return FAIL;
    }

    memset(seen, 0, sizeof(seen));
    for (j = 0; j < NB_ERRORS; j++) {
        int bit = error[j];
        unsigned char mask = (unsigned char)(1U << (bit % 8));

        if ((seen[bit / 8] & mask) != 0) {
            return FAIL;
        }
        seen[bit / 8] |= mask;
    }
    return SUCCESS;
}

int fixed_weight_random(OUT int *error)
{
    unsigned char bits[FIXED_WEIGHT_BYTES];
    int status;

    do {
        randombytes(bits, FIXED_WEIGHT_BYTES);
        status = fixed_weight_from_bits(bits, error);
    } while (status != SUCCESS);

    return SUCCESS;
}

int m2error(IN unsigned char *m, OUT int *error)
{
    return fixed_weight_from_bits(m, error);
}
