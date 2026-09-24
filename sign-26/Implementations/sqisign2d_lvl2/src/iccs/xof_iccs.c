#include <limits.h>
#include <stddef.h>

#include <sqisign_xof.h>

#include "auxfunc.h"

int
sqisign_xof(unsigned char *out,
            size_t out_len_bytes,
            const unsigned char *in,
            size_t in_len_bytes)
{
    if ((out == NULL && out_len_bytes != 0) || (in == NULL && in_len_bytes != 0)) {
        return -1;
    }

    if (out_len_bytes > ULLONG_MAX / 8ULL || in_len_bytes > ULLONG_MAX / 8ULL) {
        return -1;
    }

    return pseudoXOF((unsigned long long)out_len_bytes * 8ULL,
                     in,
                     (unsigned long long)in_len_bytes * 8ULL,
                     out);
}
