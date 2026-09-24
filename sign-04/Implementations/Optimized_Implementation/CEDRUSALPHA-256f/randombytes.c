/*
 * Modified for NGCC/ICCS KAT evaluation.
 * Replaced /dev/urandom with the provided deterministic random number generator (DRNG).
 */

#include <stdio.h>
#include "drng.h"

extern DRNG_ctx drng_algorithm;

void randombytes(unsigned char *x, unsigned long long xlen)
{
    unsigned long long xlen_bits = xlen * 8;

    int ret = get_random_number(&drng_algorithm, x, xlen_bits);

    if (ret != 0) {
        fprintf(stderr, "FATAL ERROR: get_random_number failed in randombytes()!\n");
    }
}