/*
 * fips202x8.c -- 8-way SHAKE128/SHAKE256 (AVX-512).
 *
 * Derived from the XKCP 8-way Keccak-f[1600] code path
 *   https://github.com/XKCP/XKCP .
 * The absorb does a gathered 64-bit XOR over the 8 input streams; the
 * squeeze extracts the rate lanes from each 128-bit sub-lane of the
 * __m512i state. The permutation is keccakf1600x8.c
 * (KeccakP1600times8_PermuteAll_24rounds).
 *
 * MODIFICATIONS: re-namespaced lithium_fips202x8_avx512_*; dropped the
 * dead main()/debug prints and the unused <stdio.h>/<string.h>
 * includes.
 */
#include "fips202x8.h"

#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>

#include "fips202.h"

/* 8-way permutation from keccakf1600x8.c (XKCP, AVX-512). */
#define KeccakF1600_StatePermute8x \
    FIPS202X8_NAMESPACE(KeccakP1600times8_PermuteAll_24rounds)
extern void KeccakF1600_StatePermute8x(__m512i *s);

static void keccakx8_absorb_once(__m512i s[25], unsigned int r,
                                 const uint8_t *in0, const uint8_t *in1,
                                 const uint8_t *in2, const uint8_t *in3,
                                 const uint8_t *in4, const uint8_t *in5,
                                 const uint8_t *in6, const uint8_t *in7,
                                 size_t inlen, uint8_t p)
{
    size_t i;
    uint64_t pos = 0;
    __m512i t, idx;

    for (i = 0; i < 25; ++i)
        s[i] = _mm512_setzero_si512();

    /* idx holds the 8 base pointers; the gather reads 8 bytes from each
     * stream at byte offset `pos` (scale 1). */
    idx = _mm512_set_epi64((long long)in7, (long long)in6, (long long)in5,
                           (long long)in4, (long long)in3, (long long)in2,
                           (long long)in1, (long long)in0);
    while (inlen >= r) {
        for (i = 0; i < r / 8; ++i) {
            t = _mm512_i64gather_epi64(idx, (long long *)pos, 1);
            s[i] = _mm512_xor_si512(s[i], t);
            pos += 8;
        }
        inlen -= r;
        KeccakF1600_StatePermute8x(s);
    }

    for (i = 0; i < inlen / 8; ++i) {
        t = _mm512_i64gather_epi64(idx, (long long *)pos, 1);
        s[i] = _mm512_xor_si512(s[i], t);
        pos += 8;
    }
    inlen -= 8 * i;

    if (inlen) {
        t = _mm512_i64gather_epi64(idx, (long long *)pos, 1);
        idx = _mm512_set1_epi64((1ULL << (8 * inlen)) - 1);
        t = _mm512_and_si512(t, idx);
        s[i] = _mm512_xor_si512(s[i], t);
    }

    t = _mm512_set1_epi64((uint64_t)p << 8 * inlen);
    s[i] = _mm512_xor_si512(s[i], t);
    t = _mm512_set1_epi64(1ULL << 63);
    s[r / 8 - 1] = _mm512_xor_si512(s[r / 8 - 1], t);
}

static void keccakx8_squeezeblocks(uint8_t *out0, uint8_t *out1,
                                   uint8_t *out2, uint8_t *out3,
                                   uint8_t *out4, uint8_t *out5,
                                   uint8_t *out6, uint8_t *out7,
                                   size_t nblocks, unsigned int r,
                                   __m512i s[25])
{
    unsigned int i;
    __m128d t;

    while (nblocks > 0) {
        KeccakF1600_StatePermute8x(s);
        for (i = 0; i < r / 8; ++i) {
            t = _mm_castsi128_pd(_mm512_castsi512_si128(s[i]));
            _mm_storel_pd(
                (__attribute__((__may_alias__)) double *)&out0[8 * i], t);
            _mm_storeh_pd(
                (__attribute__((__may_alias__)) double *)&out1[8 * i], t);
            t = _mm_castsi128_pd(_mm512_extracti64x2_epi64(s[i], 1));
            _mm_storel_pd(
                (__attribute__((__may_alias__)) double *)&out2[8 * i], t);
            _mm_storeh_pd(
                (__attribute__((__may_alias__)) double *)&out3[8 * i], t);
            t = _mm_castsi128_pd(_mm512_extracti64x2_epi64(s[i], 2));
            _mm_storel_pd(
                (__attribute__((__may_alias__)) double *)&out4[8 * i], t);
            _mm_storeh_pd(
                (__attribute__((__may_alias__)) double *)&out5[8 * i], t);
            t = _mm_castsi128_pd(_mm512_extracti64x2_epi64(s[i], 3));
            _mm_storel_pd(
                (__attribute__((__may_alias__)) double *)&out6[8 * i], t);
            _mm_storeh_pd(
                (__attribute__((__may_alias__)) double *)&out7[8 * i], t);
        }
        out0 += r;
        out1 += r;
        out2 += r;
        out3 += r;
        out4 += r;
        out5 += r;
        out6 += r;
        out7 += r;
        --nblocks;
    }
}

void shake128x8_absorb_once(keccakx8_state *state, const uint8_t *in0,
                            const uint8_t *in1, const uint8_t *in2,
                            const uint8_t *in3, const uint8_t *in4,
                            const uint8_t *in5, const uint8_t *in6,
                            const uint8_t *in7, size_t inlen)
{
    keccakx8_absorb_once(state->s, SHAKE128_RATE, in0, in1, in2, in3, in4,
                         in5, in6, in7, inlen, 0x1F);
}

void shake128x8_squeezeblocks(uint8_t *out0, uint8_t *out1, uint8_t *out2,
                              uint8_t *out3, uint8_t *out4, uint8_t *out5,
                              uint8_t *out6, uint8_t *out7, size_t nblocks,
                              keccakx8_state *state)
{
    keccakx8_squeezeblocks(out0, out1, out2, out3, out4, out5, out6, out7,
                           nblocks, SHAKE128_RATE, state->s);
}

void shake256x8_absorb_once(keccakx8_state *state, const uint8_t *in0,
                            const uint8_t *in1, const uint8_t *in2,
                            const uint8_t *in3, const uint8_t *in4,
                            const uint8_t *in5, const uint8_t *in6,
                            const uint8_t *in7, size_t inlen)
{
    keccakx8_absorb_once(state->s, SHAKE256_RATE, in0, in1, in2, in3, in4,
                         in5, in6, in7, inlen, 0x1F);
}

void shake256x8_squeezeblocks(uint8_t *out0, uint8_t *out1, uint8_t *out2,
                              uint8_t *out3, uint8_t *out4, uint8_t *out5,
                              uint8_t *out6, uint8_t *out7, size_t nblocks,
                              keccakx8_state *state)
{
    keccakx8_squeezeblocks(out0, out1, out2, out3, out4, out5, out6, out7,
                           nblocks, SHAKE256_RATE, state->s);
}

void shake128x8(uint8_t *out0, uint8_t *out1, uint8_t *out2, uint8_t *out3,
                uint8_t *out4, uint8_t *out5, uint8_t *out6, uint8_t *out7,
                size_t outlen, const uint8_t *in0, const uint8_t *in1,
                const uint8_t *in2, const uint8_t *in3, const uint8_t *in4,
                const uint8_t *in5, const uint8_t *in6, const uint8_t *in7,
                size_t inlen)
{
    unsigned int i;
    size_t nblocks = outlen / SHAKE128_RATE;
    uint8_t t[8][SHAKE128_RATE];
    keccakx8_state state;

    shake128x8_absorb_once(&state, in0, in1, in2, in3, in4, in5, in6, in7,
                           inlen);
    shake128x8_squeezeblocks(out0, out1, out2, out3, out4, out5, out6,
                             out7, nblocks, &state);

    out0 += nblocks * SHAKE128_RATE;
    out1 += nblocks * SHAKE128_RATE;
    out2 += nblocks * SHAKE128_RATE;
    out3 += nblocks * SHAKE128_RATE;
    out4 += nblocks * SHAKE128_RATE;
    out5 += nblocks * SHAKE128_RATE;
    out6 += nblocks * SHAKE128_RATE;
    out7 += nblocks * SHAKE128_RATE;
    outlen -= nblocks * SHAKE128_RATE;

    if (outlen) {
        shake128x8_squeezeblocks(t[0], t[1], t[2], t[3], t[4], t[5], t[6],
                                 t[7], 1, &state);
        for (i = 0; i < outlen; ++i) {
            out0[i] = t[0][i];
            out1[i] = t[1][i];
            out2[i] = t[2][i];
            out3[i] = t[3][i];
            out4[i] = t[4][i];
            out5[i] = t[5][i];
            out6[i] = t[6][i];
            out7[i] = t[7][i];
        }
    }
}

void shake256x8(uint8_t *out0, uint8_t *out1, uint8_t *out2, uint8_t *out3,
                uint8_t *out4, uint8_t *out5, uint8_t *out6, uint8_t *out7,
                size_t outlen, const uint8_t *in0, const uint8_t *in1,
                const uint8_t *in2, const uint8_t *in3, const uint8_t *in4,
                const uint8_t *in5, const uint8_t *in6, const uint8_t *in7,
                size_t inlen)
{
    unsigned int i;
    size_t nblocks = outlen / SHAKE256_RATE;
    uint8_t t[8][SHAKE256_RATE];
    keccakx8_state state;

    shake256x8_absorb_once(&state, in0, in1, in2, in3, in4, in5, in6, in7,
                           inlen);
    shake256x8_squeezeblocks(out0, out1, out2, out3, out4, out5, out6,
                             out7, nblocks, &state);

    out0 += nblocks * SHAKE256_RATE;
    out1 += nblocks * SHAKE256_RATE;
    out2 += nblocks * SHAKE256_RATE;
    out3 += nblocks * SHAKE256_RATE;
    out4 += nblocks * SHAKE256_RATE;
    out5 += nblocks * SHAKE256_RATE;
    out6 += nblocks * SHAKE256_RATE;
    out7 += nblocks * SHAKE256_RATE;
    outlen -= nblocks * SHAKE256_RATE;

    if (outlen) {
        shake256x8_squeezeblocks(t[0], t[1], t[2], t[3], t[4], t[5], t[6],
                                 t[7], 1, &state);
        for (i = 0; i < outlen; ++i) {
            out0[i] = t[0][i];
            out1[i] = t[1][i];
            out2[i] = t[2][i];
            out3[i] = t[3][i];
            out4[i] = t[4][i];
            out5[i] = t[5][i];
            out6[i] = t[6][i];
            out7[i] = t[7][i];
        }
    }
}
