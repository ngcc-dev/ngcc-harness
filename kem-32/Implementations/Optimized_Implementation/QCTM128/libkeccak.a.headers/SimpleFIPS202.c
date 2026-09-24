#include "SimpleFIPS202.h"

#include <stdint.h>
#include <string.h>

static const uint64_t keccakf_rndc[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL,
    0x800000000000808aULL, 0x8000000080008000ULL,
    0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008aULL, 0x0000000000000088ULL,
    0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL,
    0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL,
    0x8000000080008081ULL, 0x8000000000008080ULL,
    0x0000000080000001ULL, 0x8000000080008008ULL
};

static const int keccakf_rotc[24] = {
    1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14,
    27, 41, 56, 8, 25, 43, 62, 18, 39, 61, 20, 44
};

static const int keccakf_piln[24] = {
    10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4,
    15, 23, 19, 13, 12, 2, 20, 14, 22, 9, 6, 1
};

static uint64_t rol64(uint64_t x, int n)
{
    return (x << n) | (x >> (64 - n));
}

static uint64_t load64(const unsigned char *x)
{
    uint64_t r = 0;
    int i;

    for (i = 0; i < 8; ++i)
        r |= (uint64_t)x[i] << (8 * i);

    return r;
}

static void store64(unsigned char *x, uint64_t u)
{
    int i;

    for (i = 0; i < 8; ++i)
        x[i] = (unsigned char)(u >> (8 * i));
}

static void keccakf1600(uint64_t st[25])
{
    uint64_t bc[5];
    uint64_t t;
    int round;
    int i;
    int j;

    for (round = 0; round < 24; ++round) {
        for (i = 0; i < 5; ++i)
            bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];

        for (i = 0; i < 5; ++i) {
            t = bc[(i + 4) % 5] ^ rol64(bc[(i + 1) % 5], 1);
            for (j = 0; j < 25; j += 5)
                st[j + i] ^= t;
        }

        t = st[1];
        for (i = 0; i < 24; ++i) {
            j = keccakf_piln[i];
            bc[0] = st[j];
            st[j] = rol64(t, keccakf_rotc[i]);
            t = bc[0];
        }

        for (j = 0; j < 25; j += 5) {
            for (i = 0; i < 5; ++i)
                bc[i] = st[j + i];
            for (i = 0; i < 5; ++i)
                st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        st[0] ^= keccakf_rndc[round];
    }
}

static int keccak(unsigned char *out, size_t outlen,
                  const unsigned char *in, size_t inlen,
                  size_t rate, unsigned char domain)
{
    uint64_t st[25];
    unsigned char block[200];
    size_t i;
    size_t n;

    memset(st, 0, sizeof st);

    while (inlen >= rate) {
        for (i = 0; i < rate / 8; ++i)
            st[i] ^= load64(in + 8 * i);
        keccakf1600(st);
        in += rate;
        inlen -= rate;
    }

    memset(block, 0, rate);
    memcpy(block, in, inlen);
    block[inlen] = domain;
    block[rate - 1] |= 0x80;

    for (i = 0; i < rate / 8; ++i)
        st[i] ^= load64(block + 8 * i);
    keccakf1600(st);

    while (outlen > 0) {
        for (i = 0; i < rate / 8; ++i)
            store64(block + 8 * i, st[i]);

        n = outlen < rate ? outlen : rate;
        memcpy(out, block, n);
        out += n;
        outlen -= n;

        if (outlen > 0)
            keccakf1600(st);
    }

    return 0;
}

int SHAKE128(unsigned char *output, size_t outputByteLen,
             const unsigned char *input, size_t inputByteLen)
{
    return keccak(output, outputByteLen, input, inputByteLen, 168, 0x1f);
}

int SHAKE256(unsigned char *output, size_t outputByteLen,
             const unsigned char *input, size_t inputByteLen)
{
    return keccak(output, outputByteLen, input, inputByteLen, 136, 0x1f);
}

int SHA3_224(unsigned char *output, const unsigned char *input, size_t inputByteLen)
{
    return keccak(output, 28, input, inputByteLen, 144, 0x06);
}

int SHA3_256(unsigned char *output, const unsigned char *input, size_t inputByteLen)
{
    return keccak(output, 32, input, inputByteLen, 136, 0x06);
}

int SHA3_384(unsigned char *output, const unsigned char *input, size_t inputByteLen)
{
    return keccak(output, 48, input, inputByteLen, 104, 0x06);
}

int SHA3_512(unsigned char *output, const unsigned char *input, size_t inputByteLen)
{
    return keccak(output, 64, input, inputByteLen, 72, 0x06);
}
