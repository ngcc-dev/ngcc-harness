#include "simd/shake_avx2.h"

#ifdef LORE_USE_AVX2_SHAKE

#include "fips202.h"

#include <immintrin.h>
#include <string.h>

static const uint64_t keccak_round_constants[24] = {
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

static const unsigned int keccak_rho_offsets[25] = {
     0,  1, 62, 28, 27,
    36, 44,  6, 55, 20,
     3, 10, 43, 25, 39,
    41, 45, 15, 21,  8,
    18,  2, 61, 56, 14
};

static uint64_t load64(const uint8_t in[8])
{
    uint64_t r;
    memcpy(&r, in, sizeof(r));
    return r;
}

static void store64(uint8_t out[8], uint64_t v)
{
    memcpy(out, &v, sizeof(v));
}

static __m256i rol64(__m256i x, unsigned int n)
{
    if (n == 0) {
        return x;
    }

    return _mm256_or_si256(_mm256_sll_epi64(x, _mm_cvtsi32_si128((int)n)),
                           _mm256_srl_epi64(x, _mm_cvtsi32_si128((int)(64 - n))));
}

static void keccak_permute4x(__m256i a[25])
{
    __m256i c[5];
    __m256i d[5];
    __m256i b[25];

    for (unsigned int round = 0; round < 24; round++) {
        for (unsigned int x = 0; x < 5; x++) {
            c[x] = _mm256_xor_si256(
                _mm256_xor_si256(a[x], a[x + 5]),
                _mm256_xor_si256(
                    _mm256_xor_si256(a[x + 10], a[x + 15]),
                    a[x + 20]));
        }

        for (unsigned int x = 0; x < 5; x++) {
            d[x] = _mm256_xor_si256(c[(x + 4) % 5], rol64(c[(x + 1) % 5], 1));
        }

        for (unsigned int y = 0; y < 5; y++) {
            for (unsigned int x = 0; x < 5; x++) {
                a[x + 5 * y] = _mm256_xor_si256(a[x + 5 * y], d[x]);
            }
        }

        for (unsigned int y = 0; y < 5; y++) {
            for (unsigned int x = 0; x < 5; x++) {
                const unsigned int src = x + 5 * y;
                const unsigned int dst = y + 5 * ((2 * x + 3 * y) % 5);
                b[dst] = rol64(a[src], keccak_rho_offsets[src]);
            }
        }

        for (unsigned int y = 0; y < 5; y++) {
            for (unsigned int x = 0; x < 5; x++) {
                const unsigned int idx = x + 5 * y;
                a[idx] = _mm256_xor_si256(
                    b[idx],
                    _mm256_andnot_si256(b[((x + 1) % 5) + 5 * y],
                                        b[((x + 2) % 5) + 5 * y]));
            }
        }

        a[0] = _mm256_xor_si256(a[0], _mm256_set1_epi64x((long long)keccak_round_constants[round]));
    }
}

static void absorb_full_block(__m256i state[25],
                              const uint8_t *in0,
                              const uint8_t *in1,
                              const uint8_t *in2,
                              const uint8_t *in3,
                              unsigned int rate)
{
    const unsigned int words = rate / 8;

    for (unsigned int i = 0; i < words; i++) {
        const __m256i v = _mm256_set_epi64x((long long)load64(in3 + 8 * i),
                                            (long long)load64(in2 + 8 * i),
                                            (long long)load64(in1 + 8 * i),
                                            (long long)load64(in0 + 8 * i));
        state[i] = _mm256_xor_si256(state[i], v);
    }
    keccak_permute4x(state);
}

static void absorb_final_block(__m256i state[25],
                               const uint8_t *in0,
                               const uint8_t *in1,
                               const uint8_t *in2,
                               const uint8_t *in3,
                               size_t inlen,
                               unsigned int rate,
                               uint8_t domain)
{
    uint8_t block0[SHAKE128_RATE] = {0};
    uint8_t block1[SHAKE128_RATE] = {0};
    uint8_t block2[SHAKE128_RATE] = {0};
    uint8_t block3[SHAKE128_RATE] = {0};

    memcpy(block0, in0, inlen);
    memcpy(block1, in1, inlen);
    memcpy(block2, in2, inlen);
    memcpy(block3, in3, inlen);

    block0[inlen] ^= domain;
    block1[inlen] ^= domain;
    block2[inlen] ^= domain;
    block3[inlen] ^= domain;
    block0[rate - 1] ^= 0x80;
    block1[rate - 1] ^= 0x80;
    block2[rate - 1] ^= 0x80;
    block3[rate - 1] ^= 0x80;

    absorb_full_block(state, block0, block1, block2, block3, rate);
}

static void squeeze_block(uint8_t *out0,
                          uint8_t *out1,
                          uint8_t *out2,
                          uint8_t *out3,
                          const __m256i state[25],
                          unsigned int rate)
{
    const unsigned int words = rate / 8;
    uint64_t lanes[4];

    for (unsigned int i = 0; i < words; i++) {
        _mm256_storeu_si256((__m256i *)lanes, state[i]);
        store64(out0 + 8 * i, lanes[0]);
        store64(out1 + 8 * i, lanes[1]);
        store64(out2 + 8 * i, lanes[2]);
        store64(out3 + 8 * i, lanes[3]);
    }
}

void lore_shake128x4_absorb_once_squeezeblocks(uint8_t *out0,
                                               uint8_t *out1,
                                               uint8_t *out2,
                                               uint8_t *out3,
                                               size_t outblocks,
                                               const uint8_t *in0,
                                               const uint8_t *in1,
                                               const uint8_t *in2,
                                               const uint8_t *in3,
                                               size_t inlen)
{
    __m256i state[25];
    const unsigned int rate = SHAKE128_RATE;

    if (outblocks == 0) {
        return;
    }

    memset(state, 0, sizeof(state));

    while (inlen >= rate) {
        absorb_full_block(state, in0, in1, in2, in3, rate);
        in0 += rate;
        in1 += rate;
        in2 += rate;
        in3 += rate;
        inlen -= rate;
    }

    absorb_final_block(state, in0, in1, in2, in3, inlen, rate, 0x1f);

    for (size_t block = 0; block < outblocks; block++) {
        squeeze_block(out0 + block * rate,
                      out1 + block * rate,
                      out2 + block * rate,
                      out3 + block * rate,
                      state,
                      rate);
        if (block + 1 < outblocks) {
            keccak_permute4x(state);
        }
    }
}

#endif
