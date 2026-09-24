#include "rng.h"
#include "drng.h"

DRNG_ctx drng;

void init_randombytes(const unsigned char *seed, unsigned long long seed_len_bytes)
{
    init_random_number(&drng, seed, seed_len_bytes);
}

void get_randombytes(unsigned char *x, unsigned long long xlen)
{
    get_random_number(&drng, x, xlen * 8);
}