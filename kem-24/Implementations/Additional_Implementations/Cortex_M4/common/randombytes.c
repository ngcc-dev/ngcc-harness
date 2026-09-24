#include "randombytes.h"
#include "drng.h"

extern DRNG_ctx drng_algorithm;

int randombytes(uint8_t *buf, size_t xlen)
{
    if (xlen == 0) {
        return 0;
    }

    return get_random_number(&drng_algorithm, buf, (unsigned long long)xlen * 8);
}
