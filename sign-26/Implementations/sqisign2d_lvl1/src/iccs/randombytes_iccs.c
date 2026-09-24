#include <limits.h>
#include <stddef.h>

#include <rng.h>

#include "drng.h"

extern DRNG_ctx drng_algorithm;

int
randombytes(unsigned char *out, unsigned long long out_len_bytes)
{
    int ret;

    if (out_len_bytes == 0) {
        return 0;
    }

    if (out == NULL) {
        return -1;
    }

    if (out_len_bytes > ULLONG_MAX / 8ULL) {
        return -1;
    }

    ret = get_random_number(&drng_algorithm, out, out_len_bytes * 8ULL);
    return ret == 0 ? 0 : -1;
}

void
randombytes_init(unsigned char *entropy_input,
                 unsigned char *personalization_string,
                 int security_strength)
{
    (void)entropy_input;
    (void)personalization_string;
    (void)security_strength;
}
