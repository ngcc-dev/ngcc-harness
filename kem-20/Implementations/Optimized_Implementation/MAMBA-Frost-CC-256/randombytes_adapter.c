/* Adapt Frost's byte-oriented RNG request to the official API_PKC DRNG. */
#include <limits.h>
#include "drng.h"

extern DRNG_ctx drng_algorithm;

int randombytes(unsigned char *output, unsigned long long output_len_bytes)
{
    if (output == 0 || output_len_bytes > ULLONG_MAX / 8ULL) {
        return 1;
    }
    return get_random_number(&drng_algorithm, output, output_len_bytes * 8ULL);
}
