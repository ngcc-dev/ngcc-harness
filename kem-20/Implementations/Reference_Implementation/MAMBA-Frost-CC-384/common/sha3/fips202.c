/*
 * API_PKC auxiliary XOF adapter for Frost/Frost-CC.
 *
 * The Frost core calls SHAKE-style byte APIs named shake128/shake256. In the
 * API_PKC submission tree these entry points are intentionally routed to the
 * official auxiliary pseudoXOF() function instead of the original FIPS 202
 * implementation, so KEM code uses the API_PKC hash/XOF auxiliary layer.
 */
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include "fips202.h"
#include "../../auxfunc.h"

static void clear_output(unsigned char *output, unsigned long long outlen)
{
    if (output == 0 || outlen > (unsigned long long)SIZE_MAX) {
        return;
    }
    memset(output, 0, (size_t)outlen);
}

static void api_pseudo_xof(unsigned char *output, unsigned long long outlen,
                           const unsigned char *input, unsigned long long inlen)
{
    if (output == 0 || (input == 0 && inlen != 0) ||
        outlen > (ULLONG_MAX / 8ULL) || inlen > (ULLONG_MAX / 8ULL) ||
        outlen > (unsigned long long)SIZE_MAX) {
        clear_output(output, outlen);
        return;
    }

    if (pseudoXOF(outlen * 8ULL, input, inlen * 8ULL, output) != 0) {
        clear_output(output, outlen);
    }
}

void shake128(unsigned char *output, unsigned long long outlen,
              const unsigned char *input, unsigned long long inlen)
{
    api_pseudo_xof(output, outlen, input, inlen);
}

void shake256(unsigned char *output, unsigned long long outlen,
              const unsigned char *input, unsigned long long inlen)
{
    api_pseudo_xof(output, outlen, input, inlen);
}
