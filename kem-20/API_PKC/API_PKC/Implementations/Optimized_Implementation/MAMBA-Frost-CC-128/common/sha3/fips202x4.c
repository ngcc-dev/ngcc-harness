/*
 * API_PKC auxiliary XOF adapter for the optional 4-way Frost SHAKE interface.
 *
 * The optimized API_PKC default build uses the AES128 matrix backend and does
 * not compile Keccak times4. If the SHAKE128 matrix backend is enabled later,
 * this one-shot wrapper preserves the public shake128_4x API while routing each
 * stream to the official API_PKC pseudoXOF-backed shake128 adapter.
 */
#include "fips202.h"
#include "fips202x4.h"

void shake128_absorb4x(__m256i *s, const unsigned char *in0,
                       const unsigned char *in1, const unsigned char *in2,
                       const unsigned char *in3, unsigned long long inlen)
{
    (void)s;
    (void)in0;
    (void)in1;
    (void)in2;
    (void)in3;
    (void)inlen;
}

void shake128_squeezeblocks4x(unsigned char *output0, unsigned char *output1,
                              unsigned char *output2, unsigned char *output3,
                              unsigned long long outlen, __m256i *s)
{
    (void)output0;
    (void)output1;
    (void)output2;
    (void)output3;
    (void)outlen;
    (void)s;
}

void shake128_4x(unsigned char *output0, unsigned char *output1,
                 unsigned char *output2, unsigned char *output3,
                 unsigned long long outlen, const unsigned char *in0,
                 const unsigned char *in1, const unsigned char *in2,
                 const unsigned char *in3, unsigned long long inlen)
{
    shake128(output0, outlen, in0, inlen);
    shake128(output1, outlen, in1, inlen);
    shake128(output2, outlen, in2, inlen);
    shake128(output3, outlen, in3, inlen);
}
