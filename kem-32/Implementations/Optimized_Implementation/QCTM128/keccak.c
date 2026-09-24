#include <limits.h>
#include <stddef.h>

#include "libkeccak.a.headers/SimpleFIPS202.h"

void FIPS202_SHAKE128(const unsigned char *input, unsigned int inputByteLen,
                      unsigned char *output, int outputByteLen)
{
    if (outputByteLen < 0) {
        return;
    }
    (void)SHAKE128(output, (size_t)outputByteLen, input, (size_t)inputByteLen);
}

void FIPS202_SHAKE256(const unsigned char *input, unsigned int inputByteLen,
                      unsigned char *output, int outputByteLen)
{
    if (outputByteLen < 0) {
        return;
    }
    (void)SHAKE256(output, (size_t)outputByteLen, input, (size_t)inputByteLen);
}

void FIPS202_SHA3_224(const unsigned char *input, unsigned int inputByteLen,
                      unsigned char *output)
{
    (void)SHA3_224(output, input, (size_t)inputByteLen);
}

void FIPS202_SHA3_256(const unsigned char *input, unsigned int inputByteLen,
                      unsigned char *output)
{
    (void)SHA3_256(output, input, (size_t)inputByteLen);
}

void FIPS202_SHA3_384(const unsigned char *input, unsigned int inputByteLen,
                      unsigned char *output)
{
    (void)SHA3_384(output, input, (size_t)inputByteLen);
}

void FIPS202_SHA3_512(const unsigned char *input, unsigned int inputByteLen,
                      unsigned char *output)
{
    (void)SHA3_512(output, input, (size_t)inputByteLen);
}
