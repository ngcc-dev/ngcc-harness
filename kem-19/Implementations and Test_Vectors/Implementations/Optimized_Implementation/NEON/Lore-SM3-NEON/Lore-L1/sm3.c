#include "sm3.h"
#include "auxfunc.h"

void sm3(uint8_t *out, const uint8_t *in, size_t inlen)
{
    (void)sm3hash(256, in, (unsigned long long)inlen * 8ULL, out);
}

void sm3_kdf(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen)
{
    (void)pseudoXOF((unsigned long long)outlen * 8ULL,
                    in,
                    (unsigned long long)inlen * 8ULL,
                    out);
}
