#ifdef _MSC_VER
#pragma warning(disable: 4819)
#endif
#include "random_sampling.h"
#include "auxfunc.h"
#include "dke_hash.h"
#include "parameters.h"
#include "poly.h"
#include "polyvec.h"
#include "sm3.h"
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

// CBD helpers ---------------------------------------------------------------

#if DKE_CBD_ETA == 3

static uint32_t load24_littleendian(const uint8_t x[3]) {
    uint32_t r;
    r  = (uint32_t)x[0];
    r |= (uint32_t)x[1] << 8;
    r |= (uint32_t)x[2] << 16;
    return r;
}

#if defined(DKE_USE_AARCH64) && defined(DKE_NEON_OPT)
#include <arm_neon.h>
/* NEON CBD eta=3: process 4 groups of 3 bytes (16 coefficients) per iteration. */
static void centered_binomial(poly *pol, const unsigned char coins[DKE_CBD_BYTES]) {
    unsigned int i;
    const uint32x4_t mask249 = vdupq_n_u32(0x00249249);
    const uint32x4_t mask7   = vdupq_n_u32(7);

    for (i = 0; i < DKE_N / 16; i++) {
        /* Load 4 groups of 3 bytes into 4 x uint32 */
        uint32_t t0 = load24_littleendian(coins + 12*i);
        uint32_t t1 = load24_littleendian(coins + 12*i + 3);
        uint32_t t2 = load24_littleendian(coins + 12*i + 6);
        uint32_t t3 = load24_littleendian(coins + 12*i + 9);
        uint32_t tv[4] = {t0, t1, t2, t3};
        uint32x4_t t = vld1q_u32(tv);

        /* Popcount in groups of 3 bits: d = popcount3(t) per 3-bit field */
        uint32x4_t d = vandq_u32(t, mask249);
        d = vaddq_u32(d, vandq_u32(vshrq_n_u32(t, 1), mask249));
        d = vaddq_u32(d, vandq_u32(vshrq_n_u32(t, 2), mask249));

        /* Extract 4 coefficients per uint32: a = d[6j:6j+2], b = d[6j+3:6j+5] */
        /* Process each of the 4 uint32 values to extract 4 coefficients */
        uint32_t dv[4];
        vst1q_u32(dv, d);
        unsigned int k;
        for (k = 0; k < 4; k++) {
            uint32_t dk = dv[k];
            pol->coeffs[16*i + 4*k + 0] = (int16_t)((dk >> 0) & 7) - (int16_t)((dk >> 3) & 7);
            pol->coeffs[16*i + 4*k + 1] = (int16_t)((dk >> 6) & 7) - (int16_t)((dk >> 9) & 7);
            pol->coeffs[16*i + 4*k + 2] = (int16_t)((dk >> 12) & 7) - (int16_t)((dk >> 15) & 7);
            pol->coeffs[16*i + 4*k + 3] = (int16_t)((dk >> 18) & 7) - (int16_t)((dk >> 21) & 7);
        }
    }
}
#else
static void centered_binomial(poly *pol, const unsigned char coins[DKE_CBD_BYTES]) {
    unsigned int i, j;
    uint32_t t, d;
    int16_t a, b;
    for (i = 0; i < DKE_N / 4; i++) {
        t = load24_littleendian(coins + 3 * i);
        d  = t & 0x00249249;
        d += (t >> 1) & 0x00249249;
        d += (t >> 2) & 0x00249249;
        for (j = 0; j < 4; j++) {
            a = (d >> (6*j + 0)) & 0x7;
            b = (d >> (6*j + 3)) & 0x7;
            pol->coeffs[4*i+j] = a - b;
        }
    }
}
#endif

#else /* DKE_CBD_ETA == 2 */

static uint32_t load32_littleendian(const uint8_t x[4]) {
    uint32_t r;
    r  = (uint32_t)x[0];
    r |= (uint32_t)x[1] << 8;
    r |= (uint32_t)x[2] << 16;
    r |= (uint32_t)x[3] << 24;
    return r;
}

#if defined(DKE_USE_AARCH64) && defined(DKE_NEON_OPT)
/* NEON CBD eta=2: process 4 groups of 4 bytes (32 coefficients) per iteration. */
static void centered_binomial(poly *pol, const unsigned char coins[DKE_CBD_BYTES]) {
    unsigned int i, k;
    const uint32x4_t mask55 = vdupq_n_u32(0x55555555);
    const uint32x4_t mask3  = vdupq_n_u32(3);

    for (i = 0; i < DKE_N / 32; i++) {
        /* Load 16 bytes = 4 x uint32 */
        uint32x4_t t = vld1q_u32((const uint32_t *)(coins + 16*i));
        uint32x4_t d = vandq_u32(t, mask55);
        d = vaddq_u32(d, vandq_u32(vshrq_n_u32(t, 1), mask55));

        /* Extract 8 coefficients per uint32 */
        uint32_t dv[4];
        vst1q_u32(dv, d);
        for (k = 0; k < 4; k++) {
            uint32_t dk = dv[k];
            unsigned int j;
            for (j = 0; j < 8; j++) {
                int16_t a = (dk >> (4*j)) & 3;
                int16_t b = (dk >> (4*j + 2)) & 3;
                pol->coeffs[32*i + 8*k + j] = a - b;
            }
        }
    }
}
#else
static void centered_binomial(poly *pol, const unsigned char coins[DKE_CBD_BYTES]) {
    unsigned int i, j;
    uint32_t t, d;
    int16_t a, b;
    for (i = 0; i < DKE_N / 8; i++) {
        t = load32_littleendian(coins + 4 * i);
        d  = t & 0x55555555;
        d += (t >> 1) & 0x55555555;
        for (j = 0; j < 8; j++) {
            a = (d >> (4*j + 0)) & 0x3;
            b = (d >> (4*j + 2)) & 0x3;
            pol->coeffs[8*i+j] = a - b;
        }
    }
}

#endif

#endif /* DKE_CBD_ETA */

// Secret/error sampling via CBD + XOF -----------------------------------------

#if defined(DKE_USE_OPT_C)

#if defined(DKE_HASH_SHAKE)
/* SHAKE mode: use SHAKE128 incremental API for CBD coin generation. */
#include "fips202.h"
void DKE_getsecretA(poly *pol, const unsigned char rand[DKE_SEEDBYTES], const uint8_t nonce) {
    uint8_t coins[DKE_CBD_BYTES];
    uint8_t extseed[DKE_SEEDBYTES + 1];
    keccak_state state;

    memcpy(extseed, rand, DKE_SEEDBYTES);
    extseed[DKE_SEEDBYTES] = nonce;
    shake128_absorb_once(&state, extseed, DKE_SEEDBYTES + 1);
    shake128_squeeze(coins, DKE_CBD_BYTES, &state);
    centered_binomial(pol, coins);
}
#else
/* SM3 mode: incremental hash with counter */
void DKE_getsecretA(poly *pol, const unsigned char rand[DKE_SEEDBYTES], const uint8_t nonce) {
    uint8_t msg_ct[DKE_SEEDBYTES + 1 + 4];
    uint8_t coins[DKE_CBD_BYTES];
    unsigned int ct = 1;
    size_t generated = 0;
    size_t msg_len = DKE_SEEDBYTES + 1;

    memcpy(msg_ct, rand, DKE_SEEDBYTES);
    msg_ct[DKE_SEEDBYTES] = nonce;

    while (generated + 32 <= DKE_CBD_BYTES) {
        msg_ct[msg_len + 0] = (uint8_t)(ct >> 24);
        msg_ct[msg_len + 1] = (uint8_t)(ct >> 16);
        msg_ct[msg_len + 2] = (uint8_t)(ct >> 8);
        msg_ct[msg_len + 3] = (uint8_t)(ct);
        dke_hash256(msg_ct, (unsigned long long)(msg_len + 4) * 8ULL,
                     coins + generated);
        generated += 32;
        ct++;
    }
    if (generated < DKE_CBD_BYTES) {
        uint8_t tmp[32];
        msg_ct[msg_len + 0] = (uint8_t)(ct >> 24);
        msg_ct[msg_len + 1] = (uint8_t)(ct >> 16);
        msg_ct[msg_len + 2] = (uint8_t)(ct >> 8);
        msg_ct[msg_len + 3] = (uint8_t)(ct);
        dke_hash256(msg_ct, (unsigned long long)(msg_len + 4) * 8ULL, tmp);
        memcpy(coins + generated, tmp, DKE_CBD_BYTES - generated);
    }
    centered_binomial(pol, coins);
}
#endif /* DKE_HASH */

#else

void DKE_getsecretA(poly *pol, const unsigned char rand[DKE_SEEDBYTES], const uint8_t nonce) {
    uint8_t msg[DKE_SEEDBYTES + 1];
    uint8_t coins[DKE_CBD_BYTES];
    memcpy(msg, rand, DKE_SEEDBYTES);
    msg[DKE_SEEDBYTES] = nonce;
    dke_xof(DKE_CBD_BYTES * 8, msg, (DKE_SEEDBYTES + 1) * 8, coins);
    centered_binomial(pol, coins);
}

#endif /* DKE_USE_OPT_C */

void DKE_geterrorA(poly *pol, const unsigned char rand[DKE_SEEDBYTES], const uint8_t nonce) {
    DKE_getsecretA(pol, rand, nonce);
}

void DKE_getsecretB(poly *pol, const unsigned char rand[DKE_SEEDBYTES], const uint8_t nonce) {
    DKE_getsecretA(pol, rand, nonce);
}

void DKE_geterrorB(poly *pol, const unsigned char rand[DKE_SEEDBYTES], const uint8_t nonce) {
    DKE_getsecretA(pol, rand, nonce);
}

// Optimized noise sampling -----------------------------------------------------

#if defined(DKE_USE_OPT_C) && defined(DKE_HASH_SHAKE)
#include "fips202.h"

/*
 * 2x interleaved noise sampling using two independent scalar SHAKE128 states.
 */
void DKE_getnoise_2x(poly *r0, poly *r1,
                      const uint8_t seed[DKE_SEEDBYTES],
                      uint8_t nonce0, uint8_t nonce1) {
    uint8_t buf0[DKE_CBD_BYTES];
    uint8_t buf1[DKE_CBD_BYTES];
    uint8_t ext0[DKE_SEEDBYTES + 1];
    uint8_t ext1[DKE_SEEDBYTES + 1];
    keccak_state state0, state1;

    memcpy(ext0, seed, DKE_SEEDBYTES); ext0[DKE_SEEDBYTES] = nonce0;
    memcpy(ext1, seed, DKE_SEEDBYTES); ext1[DKE_SEEDBYTES] = nonce1;

    shake128_absorb_once(&state0, ext0, DKE_SEEDBYTES + 1);
    shake128_absorb_once(&state1, ext1, DKE_SEEDBYTES + 1);

    shake128_squeeze(buf0, DKE_CBD_BYTES, &state0);
    shake128_squeeze(buf1, DKE_CBD_BYTES, &state1);

    centered_binomial(r0, buf0);
    centered_binomial(r1, buf1);
}

#endif /* DKE_USE_OPT_C && DKE_HASH_SHAKE */

#if defined(DKE_USE_AVX2) && defined(DKE_HASH_SHAKE)
#include "fips202.h"
#include "fips202x4.h"
#include <immintrin.h>

/*
 * AVX2 aligned buffer type (matches ML-KEM's ALIGNED_UINT8).
 */
#define DKE_NOISE_NBLOCKS ((DKE_CBD_BYTES + SHAKE128_RATE - 1) / SHAKE128_RATE)
#define DKE_NOISE_BUFLEN  (DKE_NOISE_NBLOCKS * SHAKE128_RATE)

typedef union {
    uint8_t coeffs[DKE_NOISE_BUFLEN];
    __m256i vec[(DKE_NOISE_BUFLEN + 31) / 32];
} dke_aligned_noisebuf;

/*
 * AVX2 vectorized CBD3 (eta=3): 24 bytes → 32 coefficients per iteration.
 * Borrowed from ML-KEM AVX2 cbd3 implementation.
 */
#if DKE_CBD_ETA == 3
static void cbd3_avx2(poly *r, const uint8_t buf[DKE_CBD_BYTES + 8]) {
    unsigned int i;
    __m256i f0, f1, f2, f3;
    const __m256i mask249 = _mm256_set1_epi32(0x249249);
    const __m256i mask6DB = _mm256_set1_epi32(0x6DB6DB);
    const __m256i mask07  = _mm256_set1_epi32(7);
    const __m256i mask70  = _mm256_set1_epi32(7 << 16);
    const __m256i mask3   = _mm256_set1_epi16(3);
    const __m256i shufbidx = _mm256_set_epi8(
        -1,15,14,13, -1,12,11,10, -1, 9, 8, 7, -1, 6, 5, 4,
        -1,11,10, 9, -1, 8, 7, 6, -1, 5, 4, 3, -1, 2, 1, 0);

    for (i = 0; i < DKE_N / 32; i++) {
        f0 = _mm256_loadu_si256((__m256i *)&buf[24 * i]);
        f0 = _mm256_permute4x64_epi64(f0, 0x94);
        f0 = _mm256_shuffle_epi8(f0, shufbidx);

        f1 = _mm256_srli_epi32(f0, 1);
        f2 = _mm256_srli_epi32(f0, 2);
        f0 = _mm256_and_si256(mask249, f0);
        f1 = _mm256_and_si256(mask249, f1);
        f2 = _mm256_and_si256(mask249, f2);
        f0 = _mm256_add_epi32(f0, f1);
        f0 = _mm256_add_epi32(f0, f2);

        f1 = _mm256_srli_epi32(f0, 3);
        f0 = _mm256_add_epi32(f0, mask6DB);
        f0 = _mm256_sub_epi32(f0, f1);

        f1 = _mm256_slli_epi32(f0, 10);
        f2 = _mm256_srli_epi32(f0, 12);
        f3 = _mm256_srli_epi32(f0, 2);
        f0 = _mm256_and_si256(f0, mask07);
        f1 = _mm256_and_si256(f1, mask70);
        f2 = _mm256_and_si256(f2, mask07);
        f3 = _mm256_and_si256(f3, mask70);
        f0 = _mm256_add_epi16(f0, f1);
        f1 = _mm256_add_epi16(f2, f3);
        f0 = _mm256_sub_epi16(f0, mask3);
        f1 = _mm256_sub_epi16(f1, mask3);

        f2 = _mm256_unpacklo_epi32(f0, f1);
        f3 = _mm256_unpackhi_epi32(f0, f1);

        f0 = _mm256_permute2x128_si256(f2, f3, 0x20);
        f1 = _mm256_permute2x128_si256(f2, f3, 0x31);

        _mm256_store_si256((__m256i *)&r->coeffs[32*i +  0], f0);
        _mm256_store_si256((__m256i *)&r->coeffs[32*i + 16], f1);
    }
}
#endif

/*
 * AVX2 vectorized CBD2 (eta=2): 32 bytes → 64 coefficients per iteration.
 * Borrowed from ML-KEM AVX2 cbd2 implementation.
 */
#if DKE_CBD_ETA == 2
static void cbd2_avx2(poly *r, const uint8_t *buf) {
    unsigned int i;
    __m256i f0, f1, f2, f3;
    const __m256i mask55 = _mm256_set1_epi32(0x55555555);
    const __m256i mask33 = _mm256_set1_epi32(0x33333333);
    const __m256i mask03 = _mm256_set1_epi32(0x03030303);
    const __m256i mask0F = _mm256_set1_epi32(0x0F0F0F0F);

    for (i = 0; i < DKE_N / 64; i++) {
        f0 = _mm256_loadu_si256((__m256i *)&buf[32 * i]);

        f1 = _mm256_srli_epi16(f0, 1);
        f0 = _mm256_and_si256(mask55, f0);
        f1 = _mm256_and_si256(mask55, f1);
        f0 = _mm256_add_epi8(f0, f1);

        f1 = _mm256_srli_epi16(f0, 2);
        f0 = _mm256_and_si256(mask33, f0);
        f1 = _mm256_and_si256(mask33, f1);
        f0 = _mm256_add_epi8(f0, mask33);
        f0 = _mm256_sub_epi8(f0, f1);

        f1 = _mm256_srli_epi16(f0, 4);
        f0 = _mm256_and_si256(mask0F, f0);
        f1 = _mm256_and_si256(mask0F, f1);
        f0 = _mm256_sub_epi8(f0, mask03);
        f1 = _mm256_sub_epi8(f1, mask03);

        f2 = _mm256_unpacklo_epi8(f0, f1);
        f3 = _mm256_unpackhi_epi8(f0, f1);

        f0 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(f2));
        f1 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(f2, 1));
        f2 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(f3));
        f3 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(f3, 1));

        _mm256_store_si256((__m256i *)&r->coeffs[64*i +  0], f0);
        _mm256_store_si256((__m256i *)&r->coeffs[64*i + 16], f2);
        _mm256_store_si256((__m256i *)&r->coeffs[64*i + 32], f1);
        _mm256_store_si256((__m256i *)&r->coeffs[64*i + 48], f3);
    }
}
#endif

static void cbd_avx2(poly *r, const uint8_t *buf) {
#if DKE_CBD_ETA == 3
    cbd3_avx2(r, buf);
#elif DKE_CBD_ETA == 2
    cbd2_avx2(r, buf);
#else
#error "cbd_avx2 requires DKE_CBD_ETA in {2, 3}"
#endif
}

/*
 * 4x parallel noise sampling using AVX2 4-way Keccak + AVX2 CBD.
 * Matches ML-KEM AVX2 poly_getnoise_eta1_4x structure:
 *   - AVX2 seed broadcast
 *   - shake128x4 absorb + squeezeblocks
 *   - AVX2 vectorized CBD
 */
void DKE_getnoise_4x(poly *r0, poly *r1, poly *r2, poly *r3,
                      const uint8_t seed[DKE_SEEDBYTES],
                      uint8_t nonce0, uint8_t nonce1,
                      uint8_t nonce2, uint8_t nonce3) {
    dke_aligned_noisebuf buf[4];
#if defined(_MSC_VER)
    __declspec(align(32)) keccakx4_state state;
#else
    keccakx4_state state __attribute__((aligned(32)));
#endif

    /* AVX2 seed broadcast (32 bytes at a time) */
#if DKE_SEEDBYTES == 32
    {
        __m256i f = _mm256_loadu_si256((const __m256i *)seed);
        _mm256_store_si256(buf[0].vec, f);
        _mm256_store_si256(buf[1].vec, f);
        _mm256_store_si256(buf[2].vec, f);
        _mm256_store_si256(buf[3].vec, f);
    }
#elif DKE_SEEDBYTES == 64
    {
        __m256i f0 = _mm256_loadu_si256((const __m256i *)seed);
        __m256i f1 = _mm256_loadu_si256((const __m256i *)(seed + 32));
        _mm256_store_si256(&buf[0].vec[0], f0); _mm256_store_si256(&buf[0].vec[1], f1);
        _mm256_store_si256(&buf[1].vec[0], f0); _mm256_store_si256(&buf[1].vec[1], f1);
        _mm256_store_si256(&buf[2].vec[0], f0); _mm256_store_si256(&buf[2].vec[1], f1);
        _mm256_store_si256(&buf[3].vec[0], f0); _mm256_store_si256(&buf[3].vec[1], f1);
    }
#else
    memcpy(buf[0].coeffs, seed, DKE_SEEDBYTES);
    memcpy(buf[1].coeffs, seed, DKE_SEEDBYTES);
    memcpy(buf[2].coeffs, seed, DKE_SEEDBYTES);
    memcpy(buf[3].coeffs, seed, DKE_SEEDBYTES);
#endif
    buf[0].coeffs[DKE_SEEDBYTES] = nonce0;
    buf[1].coeffs[DKE_SEEDBYTES] = nonce1;
    buf[2].coeffs[DKE_SEEDBYTES] = nonce2;
    buf[3].coeffs[DKE_SEEDBYTES] = nonce3;

    shake128x4_absorb_once(&state, buf[0].coeffs, buf[1].coeffs,
                           buf[2].coeffs, buf[3].coeffs, DKE_SEEDBYTES + 1);
    shake128x4_squeezeblocks(buf[0].coeffs, buf[1].coeffs,
                             buf[2].coeffs, buf[3].coeffs, DKE_NOISE_NBLOCKS, &state);

    cbd_avx2(r0, buf[0].coeffs);
    cbd_avx2(r1, buf[1].coeffs);
    cbd_avx2(r2, buf[2].coeffs);
    cbd_avx2(r3, buf[3].coeffs);
}

/*
 * 5x noise sampling: 4x via shake128x4 + 1x via scalar shake128.
 * Runs both in parallel to avoid a separate SHAKE128 invocation for r4.
 * Used in CPA_enc (K=2): r0=sB[0], r1=sB[1], r2=eB[0], r3=eB[1], r4=e.
 */
void DKE_getnoise_5x(poly *r0, poly *r1, poly *r2, poly *r3, poly *r4,
                      const uint8_t seed[DKE_SEEDBYTES],
                      uint8_t nonce0, uint8_t nonce1,
                      uint8_t nonce2, uint8_t nonce3, uint8_t nonce4) {
#define DKE_NOISE_NBLOCKS5 ((DKE_CBD_BYTES + SHAKE128_RATE - 1) / SHAKE128_RATE)
#define DKE_NOISE_BUFLEN5  (DKE_NOISE_NBLOCKS5 * SHAKE128_RATE)
    dke_aligned_noisebuf buf[4];
    /* r4 uses a scalar state — absorb starts immediately, overlapping 4x squeeze */
#if defined(_MSC_VER)
    __declspec(align(32)) uint8_t buf4[DKE_NOISE_BUFLEN5];
    __declspec(align(32)) keccakx4_state state4x;
#else
    uint8_t buf4[DKE_NOISE_BUFLEN5] __attribute__((aligned(32)));
    keccakx4_state state4x __attribute__((aligned(32)));
#endif
    keccak_state state1;
    uint8_t ext4[DKE_SEEDBYTES + 1];

    /* Absorb r4 seed first (scalar) — overlaps with 4x setup below */
    memcpy(ext4, seed, DKE_SEEDBYTES);
    ext4[DKE_SEEDBYTES] = nonce4;
    shake128_absorb_once(&state1, ext4, DKE_SEEDBYTES + 1);

    /* 4x: broadcast seed + set nonces */
#if DKE_SEEDBYTES == 32
    {
        __m256i f = _mm256_loadu_si256((const __m256i *)seed);
        _mm256_store_si256(buf[0].vec, f);
        _mm256_store_si256(buf[1].vec, f);
        _mm256_store_si256(buf[2].vec, f);
        _mm256_store_si256(buf[3].vec, f);
    }
#elif DKE_SEEDBYTES == 64
    {
        __m256i f0 = _mm256_loadu_si256((const __m256i *)seed);
        __m256i f1 = _mm256_loadu_si256((const __m256i *)(seed + 32));
        _mm256_store_si256(&buf[0].vec[0], f0); _mm256_store_si256(&buf[0].vec[1], f1);
        _mm256_store_si256(&buf[1].vec[0], f0); _mm256_store_si256(&buf[1].vec[1], f1);
        _mm256_store_si256(&buf[2].vec[0], f0); _mm256_store_si256(&buf[2].vec[1], f1);
        _mm256_store_si256(&buf[3].vec[0], f0); _mm256_store_si256(&buf[3].vec[1], f1);
    }
#else
    memcpy(buf[0].coeffs, seed, DKE_SEEDBYTES);
    memcpy(buf[1].coeffs, seed, DKE_SEEDBYTES);
    memcpy(buf[2].coeffs, seed, DKE_SEEDBYTES);
    memcpy(buf[3].coeffs, seed, DKE_SEEDBYTES);
#endif
    buf[0].coeffs[DKE_SEEDBYTES] = nonce0;
    buf[1].coeffs[DKE_SEEDBYTES] = nonce1;
    buf[2].coeffs[DKE_SEEDBYTES] = nonce2;
    buf[3].coeffs[DKE_SEEDBYTES] = nonce3;

    shake128x4_absorb_once(&state4x, buf[0].coeffs, buf[1].coeffs,
                           buf[2].coeffs, buf[3].coeffs, DKE_SEEDBYTES + 1);
    shake128x4_squeezeblocks(buf[0].coeffs, buf[1].coeffs,
                             buf[2].coeffs, buf[3].coeffs, DKE_NOISE_NBLOCKS5, &state4x);

    /* Squeeze r4 (scalar) — CPU can overlap with CBD below */
    shake128_squeeze(buf4, DKE_CBD_BYTES, &state1);

    cbd_avx2(r0, buf[0].coeffs);
    cbd_avx2(r1, buf[1].coeffs);
    cbd_avx2(r2, buf[2].coeffs);
    cbd_avx2(r3, buf[3].coeffs);
    cbd_avx2(r4, buf4);
#undef DKE_NOISE_NBLOCKS5
#undef DKE_NOISE_BUFLEN5
}

/*
 * DKE_getnoise_absorb / DKE_getnoise_squeeze:
 * Split interface for overlapping absorb with gen_at.
 *
 * Usage in CPA_enc (K=2):
 *   DKE_getnoise_absorb(&st, coins, 4);   // before gen_at
 *   gen_at(matt, seed);                   // ~3500 cycles, hides absorb
 *   DKE_getnoise_4x(...);                 // 4x noise
 *   DKE_getnoise_squeeze(&e, &st);        // squeeze + AVX2 CBD
 */
void DKE_getnoise_absorb(keccak_state *state,
                          const uint8_t seed[DKE_SEEDBYTES],
                          uint8_t nonce) {
    uint8_t ext[DKE_SEEDBYTES + 1];
    memcpy(ext, seed, DKE_SEEDBYTES);
    ext[DKE_SEEDBYTES] = nonce;
    shake128_absorb_once(state, ext, DKE_SEEDBYTES + 1);
}

void DKE_getnoise_squeeze(poly *r, keccak_state *state) {
#define DKE_NOISE_NBLOCKS_SQ ((DKE_CBD_BYTES + SHAKE128_RATE - 1) / SHAKE128_RATE)
#define DKE_NOISE_BUFLEN_SQ  (DKE_NOISE_NBLOCKS_SQ * SHAKE128_RATE)
#if defined(_MSC_VER)
    __declspec(align(32)) uint8_t buf[DKE_NOISE_BUFLEN_SQ];
#else
    uint8_t buf[DKE_NOISE_BUFLEN_SQ] __attribute__((aligned(32)));
#endif
    shake128_squeeze(buf, DKE_CBD_BYTES, state);
    cbd_avx2(r, buf);
#undef DKE_NOISE_NBLOCKS_SQ
#undef DKE_NOISE_BUFLEN_SQ
}

#undef DKE_NOISE_BUFLEN
#undef DKE_NOISE_NBLOCKS

#elif defined(DKE_USE_OPT_C) && defined(DKE_HASH_SHAKE)
#include "fips202.h"

#if defined(DKE_USE_AARCH64_NATIVE)
/* AArch64 native: fused Keccak x4 noise sampling.
 * Inlines absorb + squeeze to avoid wrapper overhead:
 * - No intermediate ext[4] seed copies (build padding directly in state)
 * - Single memset for 4 states
 * - Direct squeeze to output buffers */
#include "aarch64-native/fips202x4_aarch64.h"
#define DKE_NOISE_NBLOCKS ((DKE_CBD_BYTES + 168 - 1) / 168)

extern void dke_keccak_f1600_x4(uint64_t state[100], const uint64_t rc[24]);
extern const uint64_t dke_keccakf1600_round_constants[];

void DKE_getnoise_4x(poly *r0, poly *r1, poly *r2, poly *r3,
                      const uint8_t seed[DKE_SEEDBYTES],
                      uint8_t nonce0, uint8_t nonce1,
                      uint8_t nonce2, uint8_t nonce3) {
    uint8_t buf[4][DKE_NOISE_NBLOCKS * 168];
    keccakx4_state state;
    unsigned int i;
    const uint64_t *seed64 = (const uint64_t *)seed;

    /* Fast absorb: seed is short (SEEDBYTES+1 < 168), single block.
     * Build 4 sequential states directly, avoiding ext[] and wrapper calls. */
    memset(&state, 0, sizeof(state));

    /* XOR seed (32 or 64 bytes) into all 4 lanes */
    for (i = 0; i < DKE_SEEDBYTES / 8; i++) {
        state.s[0*25 + i] = seed64[i];
        state.s[1*25 + i] = seed64[i];
        state.s[2*25 + i] = seed64[i];
        state.s[3*25 + i] = seed64[i];
    }

    /* XOR nonce byte + SHAKE128 padding (0x1F at byte[SEEDBYTES], 0x80 at byte[167]) */
    {
        unsigned int nonce_word = DKE_SEEDBYTES / 8;
        unsigned int nonce_shift = (DKE_SEEDBYTES % 8) * 8;
        state.s[0*25 + nonce_word] ^= ((uint64_t)nonce0 | ((uint64_t)0x1F << 8)) << nonce_shift;
        state.s[1*25 + nonce_word] ^= ((uint64_t)nonce1 | ((uint64_t)0x1F << 8)) << nonce_shift;
        state.s[2*25 + nonce_word] ^= ((uint64_t)nonce2 | ((uint64_t)0x1F << 8)) << nonce_shift;
        state.s[3*25 + nonce_word] ^= ((uint64_t)nonce3 | ((uint64_t)0x1F << 8)) << nonce_shift;
        /* 0x80 at byte 167 = word 20, byte 7 */
        state.s[0*25 + 20] ^= (uint64_t)0x80 << 56;
        state.s[1*25 + 20] ^= (uint64_t)0x80 << 56;
        state.s[2*25 + 20] ^= (uint64_t)0x80 << 56;
        state.s[3*25 + 20] ^= (uint64_t)0x80 << 56;
    }

    /* Squeeze NBLOCKS blocks directly */
    for (i = 0; i < DKE_NOISE_NBLOCKS; i++) {
        dke_keccak_f1600_x4(state.s, dke_keccakf1600_round_constants);
        memcpy(buf[0] + i*168, &state.s[0*25], 168);
        memcpy(buf[1] + i*168, &state.s[1*25], 168);
        memcpy(buf[2] + i*168, &state.s[2*25], 168);
        memcpy(buf[3] + i*168, &state.s[3*25], 168);
    }

    centered_binomial(r0, buf[0]);
    centered_binomial(r1, buf[1]);
    centered_binomial(r2, buf[2]);
    centered_binomial(r3, buf[3]);
}
#else
/*
 * 4x noise sampling fallback: 4 independent scalar SHAKE128 states.
 */
void DKE_getnoise_4x(poly *r0, poly *r1, poly *r2, poly *r3,
                      const uint8_t seed[DKE_SEEDBYTES],
                      uint8_t nonce0, uint8_t nonce1,
                      uint8_t nonce2, uint8_t nonce3) {
    uint8_t buf[4][DKE_CBD_BYTES];
    uint8_t ext[4][DKE_SEEDBYTES + 1];
    keccak_state st0, st1, st2, st3;

    memcpy(ext[0], seed, DKE_SEEDBYTES); ext[0][DKE_SEEDBYTES] = nonce0;
    memcpy(ext[1], seed, DKE_SEEDBYTES); ext[1][DKE_SEEDBYTES] = nonce1;
    memcpy(ext[2], seed, DKE_SEEDBYTES); ext[2][DKE_SEEDBYTES] = nonce2;
    memcpy(ext[3], seed, DKE_SEEDBYTES); ext[3][DKE_SEEDBYTES] = nonce3;

    shake128_absorb_once(&st0, ext[0], DKE_SEEDBYTES + 1);
    shake128_absorb_once(&st1, ext[1], DKE_SEEDBYTES + 1);
    shake128_absorb_once(&st2, ext[2], DKE_SEEDBYTES + 1);
    shake128_absorb_once(&st3, ext[3], DKE_SEEDBYTES + 1);

    shake128_squeeze(buf[0], DKE_CBD_BYTES, &st0);
    shake128_squeeze(buf[1], DKE_CBD_BYTES, &st1);
    shake128_squeeze(buf[2], DKE_CBD_BYTES, &st2);
    shake128_squeeze(buf[3], DKE_CBD_BYTES, &st3);

    centered_binomial(r0, buf[0]);
    centered_binomial(r1, buf[1]);
    centered_binomial(r2, buf[2]);
    centered_binomial(r3, buf[3]);
}
#endif /* DKE_USE_AARCH64_NATIVE */

#if defined(DKE_USE_AARCH64_NATIVE)
/* AArch64 native: fused 5x = x4 parallel + 1x scalar */
void DKE_getnoise_5x(poly *r0, poly *r1, poly *r2, poly *r3, poly *r4,
                      const uint8_t seed[DKE_SEEDBYTES],
                      uint8_t nonce0, uint8_t nonce1,
                      uint8_t nonce2, uint8_t nonce3, uint8_t nonce4) {
    uint8_t buf[5][DKE_NOISE_NBLOCKS * 168];
    keccakx4_state state4x;
    keccak_state st4;
    uint8_t ext4[DKE_SEEDBYTES + 1];
    unsigned int i;
    const uint64_t *seed64 = (const uint64_t *)seed;

    /* Fast x4 absorb (inline, no wrapper) */
    memset(&state4x, 0, sizeof(state4x));
    for (i = 0; i < DKE_SEEDBYTES / 8; i++) {
        state4x.s[0*25+i] = seed64[i]; state4x.s[1*25+i] = seed64[i];
        state4x.s[2*25+i] = seed64[i]; state4x.s[3*25+i] = seed64[i];
    }
    {
        unsigned int nw = DKE_SEEDBYTES / 8, ns = (DKE_SEEDBYTES % 8) * 8;
        state4x.s[0*25+nw] ^= ((uint64_t)nonce0 | ((uint64_t)0x1F << 8)) << ns;
        state4x.s[1*25+nw] ^= ((uint64_t)nonce1 | ((uint64_t)0x1F << 8)) << ns;
        state4x.s[2*25+nw] ^= ((uint64_t)nonce2 | ((uint64_t)0x1F << 8)) << ns;
        state4x.s[3*25+nw] ^= ((uint64_t)nonce3 | ((uint64_t)0x1F << 8)) << ns;
        state4x.s[0*25+20] ^= (uint64_t)0x80 << 56;
        state4x.s[1*25+20] ^= (uint64_t)0x80 << 56;
        state4x.s[2*25+20] ^= (uint64_t)0x80 << 56;
        state4x.s[3*25+20] ^= (uint64_t)0x80 << 56;
    }

    /* 1x scalar for r4 (start absorb before x4 squeeze) */
    memcpy(ext4, seed, DKE_SEEDBYTES); ext4[DKE_SEEDBYTES] = nonce4;
    shake128_absorb_once(&st4, ext4, DKE_SEEDBYTES + 1);

    /* x4 squeeze */
    for (i = 0; i < DKE_NOISE_NBLOCKS; i++) {
        dke_keccak_f1600_x4(state4x.s, dke_keccakf1600_round_constants);
        memcpy(buf[0]+i*168, &state4x.s[0*25], 168);
        memcpy(buf[1]+i*168, &state4x.s[1*25], 168);
        memcpy(buf[2]+i*168, &state4x.s[2*25], 168);
        memcpy(buf[3]+i*168, &state4x.s[3*25], 168);
    }
    shake128_squeeze(buf[4], DKE_CBD_BYTES, &st4);

    centered_binomial(r0, buf[0]); centered_binomial(r1, buf[1]);
    centered_binomial(r2, buf[2]); centered_binomial(r3, buf[3]);
    centered_binomial(r4, buf[4]);
}
#else
/*
 * 5x noise sampling fallback: 5 independent scalar SHAKE128 states.
 */
void DKE_getnoise_5x(poly *r0, poly *r1, poly *r2, poly *r3, poly *r4,
                      const uint8_t seed[DKE_SEEDBYTES],
                      uint8_t nonce0, uint8_t nonce1,
                      uint8_t nonce2, uint8_t nonce3, uint8_t nonce4) {
    uint8_t buf[5][DKE_CBD_BYTES];
    uint8_t ext[5][DKE_SEEDBYTES + 1];
    keccak_state st0, st1, st2, st3, st4;

    memcpy(ext[0], seed, DKE_SEEDBYTES); ext[0][DKE_SEEDBYTES] = nonce0;
    memcpy(ext[1], seed, DKE_SEEDBYTES); ext[1][DKE_SEEDBYTES] = nonce1;
    memcpy(ext[2], seed, DKE_SEEDBYTES); ext[2][DKE_SEEDBYTES] = nonce2;
    memcpy(ext[3], seed, DKE_SEEDBYTES); ext[3][DKE_SEEDBYTES] = nonce3;
    memcpy(ext[4], seed, DKE_SEEDBYTES); ext[4][DKE_SEEDBYTES] = nonce4;

    shake128_absorb_once(&st0, ext[0], DKE_SEEDBYTES + 1);
    shake128_absorb_once(&st1, ext[1], DKE_SEEDBYTES + 1);
    shake128_absorb_once(&st2, ext[2], DKE_SEEDBYTES + 1);
    shake128_absorb_once(&st3, ext[3], DKE_SEEDBYTES + 1);
    shake128_absorb_once(&st4, ext[4], DKE_SEEDBYTES + 1);

    shake128_squeeze(buf[0], DKE_CBD_BYTES, &st0);
    shake128_squeeze(buf[1], DKE_CBD_BYTES, &st1);
    shake128_squeeze(buf[2], DKE_CBD_BYTES, &st2);
    shake128_squeeze(buf[3], DKE_CBD_BYTES, &st3);
    shake128_squeeze(buf[4], DKE_CBD_BYTES, &st4);

    centered_binomial(r0, buf[0]);
    centered_binomial(r1, buf[1]);
    centered_binomial(r2, buf[2]);
    centered_binomial(r3, buf[3]);
    centered_binomial(r4, buf[4]);
}
#endif /* DKE_USE_AARCH64_NATIVE */

#endif /* DKE_USE_AVX2/OPT_C && DKE_HASH_SHAKE -- getnoise_4x */




// Rejection sampling in Zq ---------------------------------------------------

#if DKE_MODE == 512

#if defined(DKE_USE_AVX2) && !defined(DKE_AVX2_NTT_INTRINSIC)
#include <immintrin.h>

/* pshufb lookup table for 8-value compaction (same as 12-bit version) */
static const uint8_t rej_idx_512[256][8] = {
  {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, { 0,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
  { 2,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 2,0xff,0xff,0xff,0xff,0xff,0xff},
  { 4,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 4,0xff,0xff,0xff,0xff,0xff,0xff},
  { 2, 4,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 2, 4,0xff,0xff,0xff,0xff,0xff},
  { 6,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 6,0xff,0xff,0xff,0xff,0xff,0xff},
  { 2, 6,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 2, 6,0xff,0xff,0xff,0xff,0xff},
  { 4, 6,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 4, 6,0xff,0xff,0xff,0xff,0xff},
  { 2, 4, 6,0xff,0xff,0xff,0xff,0xff}, { 0, 2, 4, 6,0xff,0xff,0xff,0xff},
  { 8,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 8,0xff,0xff,0xff,0xff,0xff,0xff},
  { 2, 8,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 2, 8,0xff,0xff,0xff,0xff,0xff},
  { 4, 8,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 4, 8,0xff,0xff,0xff,0xff,0xff},
  { 2, 4, 8,0xff,0xff,0xff,0xff,0xff}, { 0, 2, 4, 8,0xff,0xff,0xff,0xff},
  { 6, 8,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 6, 8,0xff,0xff,0xff,0xff,0xff},
  { 2, 6, 8,0xff,0xff,0xff,0xff,0xff}, { 0, 2, 6, 8,0xff,0xff,0xff,0xff},
  { 4, 6, 8,0xff,0xff,0xff,0xff,0xff}, { 0, 4, 6, 8,0xff,0xff,0xff,0xff},
  { 2, 4, 6, 8,0xff,0xff,0xff,0xff}, { 0, 2, 4, 6, 8,0xff,0xff,0xff},
  {10,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, { 0,10,0xff,0xff,0xff,0xff,0xff,0xff},
  { 2,10,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 2,10,0xff,0xff,0xff,0xff,0xff},
  { 4,10,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 4,10,0xff,0xff,0xff,0xff,0xff},
  { 2, 4,10,0xff,0xff,0xff,0xff,0xff}, { 0, 2, 4,10,0xff,0xff,0xff,0xff},
  { 6,10,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 6,10,0xff,0xff,0xff,0xff,0xff},
  { 2, 6,10,0xff,0xff,0xff,0xff,0xff}, { 0, 2, 6,10,0xff,0xff,0xff,0xff},
  { 4, 6,10,0xff,0xff,0xff,0xff,0xff}, { 0, 4, 6,10,0xff,0xff,0xff,0xff},
  { 2, 4, 6,10,0xff,0xff,0xff,0xff}, { 0, 2, 4, 6,10,0xff,0xff,0xff},
  { 8,10,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 8,10,0xff,0xff,0xff,0xff,0xff},
  { 2, 8,10,0xff,0xff,0xff,0xff,0xff}, { 0, 2, 8,10,0xff,0xff,0xff,0xff},
  { 4, 8,10,0xff,0xff,0xff,0xff,0xff}, { 0, 4, 8,10,0xff,0xff,0xff,0xff},
  { 2, 4, 8,10,0xff,0xff,0xff,0xff}, { 0, 2, 4, 8,10,0xff,0xff,0xff},
  { 6, 8,10,0xff,0xff,0xff,0xff,0xff}, { 0, 6, 8,10,0xff,0xff,0xff,0xff},
  { 2, 6, 8,10,0xff,0xff,0xff,0xff}, { 0, 2, 6, 8,10,0xff,0xff,0xff},
  { 4, 6, 8,10,0xff,0xff,0xff,0xff}, { 0, 4, 6, 8,10,0xff,0xff,0xff},
  { 2, 4, 6, 8,10,0xff,0xff,0xff}, { 0, 2, 4, 6, 8,10,0xff,0xff},
  {12,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, { 0,12,0xff,0xff,0xff,0xff,0xff,0xff},
  { 2,12,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 2,12,0xff,0xff,0xff,0xff,0xff},
  { 4,12,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 4,12,0xff,0xff,0xff,0xff,0xff},
  { 2, 4,12,0xff,0xff,0xff,0xff,0xff}, { 0, 2, 4,12,0xff,0xff,0xff,0xff},
  { 6,12,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 6,12,0xff,0xff,0xff,0xff,0xff},
  { 2, 6,12,0xff,0xff,0xff,0xff,0xff}, { 0, 2, 6,12,0xff,0xff,0xff,0xff},
  { 4, 6,12,0xff,0xff,0xff,0xff,0xff}, { 0, 4, 6,12,0xff,0xff,0xff,0xff},
  { 2, 4, 6,12,0xff,0xff,0xff,0xff}, { 0, 2, 4, 6,12,0xff,0xff,0xff},
  { 8,12,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 8,12,0xff,0xff,0xff,0xff,0xff},
  { 2, 8,12,0xff,0xff,0xff,0xff,0xff}, { 0, 2, 8,12,0xff,0xff,0xff,0xff},
  { 4, 8,12,0xff,0xff,0xff,0xff,0xff}, { 0, 4, 8,12,0xff,0xff,0xff,0xff},
  { 2, 4, 8,12,0xff,0xff,0xff,0xff}, { 0, 2, 4, 8,12,0xff,0xff,0xff},
  { 6, 8,12,0xff,0xff,0xff,0xff,0xff}, { 0, 6, 8,12,0xff,0xff,0xff,0xff},
  { 2, 6, 8,12,0xff,0xff,0xff,0xff}, { 0, 2, 6, 8,12,0xff,0xff,0xff},
  { 4, 6, 8,12,0xff,0xff,0xff,0xff}, { 0, 4, 6, 8,12,0xff,0xff,0xff},
  { 2, 4, 6, 8,12,0xff,0xff,0xff}, { 0, 2, 4, 6, 8,12,0xff,0xff},
  {10,12,0xff,0xff,0xff,0xff,0xff,0xff}, { 0,10,12,0xff,0xff,0xff,0xff,0xff},
  { 2,10,12,0xff,0xff,0xff,0xff,0xff}, { 0, 2,10,12,0xff,0xff,0xff,0xff},
  { 4,10,12,0xff,0xff,0xff,0xff,0xff}, { 0, 4,10,12,0xff,0xff,0xff,0xff},
  { 2, 4,10,12,0xff,0xff,0xff,0xff}, { 0, 2, 4,10,12,0xff,0xff,0xff},
  { 6,10,12,0xff,0xff,0xff,0xff,0xff}, { 0, 6,10,12,0xff,0xff,0xff,0xff},
  { 2, 6,10,12,0xff,0xff,0xff,0xff}, { 0, 2, 6,10,12,0xff,0xff,0xff},
  { 4, 6,10,12,0xff,0xff,0xff,0xff}, { 0, 4, 6,10,12,0xff,0xff,0xff},
  { 2, 4, 6,10,12,0xff,0xff,0xff}, { 0, 2, 4, 6,10,12,0xff,0xff},
  { 8,10,12,0xff,0xff,0xff,0xff,0xff}, { 0, 8,10,12,0xff,0xff,0xff,0xff},
  { 2, 8,10,12,0xff,0xff,0xff,0xff}, { 0, 2, 8,10,12,0xff,0xff,0xff},
  { 4, 8,10,12,0xff,0xff,0xff,0xff}, { 0, 4, 8,10,12,0xff,0xff,0xff},
  { 2, 4, 8,10,12,0xff,0xff,0xff}, { 0, 2, 4, 8,10,12,0xff,0xff},
  { 6, 8,10,12,0xff,0xff,0xff,0xff}, { 0, 6, 8,10,12,0xff,0xff,0xff},
  { 2, 6, 8,10,12,0xff,0xff,0xff}, { 0, 2, 6, 8,10,12,0xff,0xff},
  { 4, 6, 8,10,12,0xff,0xff,0xff}, { 0, 4, 6, 8,10,12,0xff,0xff},
  { 2, 4, 6, 8,10,12,0xff,0xff}, { 0, 2, 4, 6, 8,10,12,0xff},
  {14,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, { 0,14,0xff,0xff,0xff,0xff,0xff,0xff},
  { 2,14,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 2,14,0xff,0xff,0xff,0xff,0xff},
  { 4,14,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 4,14,0xff,0xff,0xff,0xff,0xff},
  { 2, 4,14,0xff,0xff,0xff,0xff,0xff}, { 0, 2, 4,14,0xff,0xff,0xff,0xff},
  { 6,14,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 6,14,0xff,0xff,0xff,0xff,0xff},
  { 2, 6,14,0xff,0xff,0xff,0xff,0xff}, { 0, 2, 6,14,0xff,0xff,0xff,0xff},
  { 4, 6,14,0xff,0xff,0xff,0xff,0xff}, { 0, 4, 6,14,0xff,0xff,0xff,0xff},
  { 2, 4, 6,14,0xff,0xff,0xff,0xff}, { 0, 2, 4, 6,14,0xff,0xff,0xff},
  { 8,14,0xff,0xff,0xff,0xff,0xff,0xff}, { 0, 8,14,0xff,0xff,0xff,0xff,0xff},
  { 2, 8,14,0xff,0xff,0xff,0xff,0xff}, { 0, 2, 8,14,0xff,0xff,0xff,0xff},
  { 4, 8,14,0xff,0xff,0xff,0xff,0xff}, { 0, 4, 8,14,0xff,0xff,0xff,0xff},
  { 2, 4, 8,14,0xff,0xff,0xff,0xff}, { 0, 2, 4, 8,14,0xff,0xff,0xff},
  { 6, 8,14,0xff,0xff,0xff,0xff,0xff}, { 0, 6, 8,14,0xff,0xff,0xff,0xff},
  { 2, 6, 8,14,0xff,0xff,0xff,0xff}, { 0, 2, 6, 8,14,0xff,0xff,0xff},
  { 4, 6, 8,14,0xff,0xff,0xff,0xff}, { 0, 4, 6, 8,14,0xff,0xff,0xff},
  { 2, 4, 6, 8,14,0xff,0xff,0xff}, { 0, 2, 4, 6, 8,14,0xff,0xff},
  {10,14,0xff,0xff,0xff,0xff,0xff,0xff}, { 0,10,14,0xff,0xff,0xff,0xff,0xff},
  { 2,10,14,0xff,0xff,0xff,0xff,0xff}, { 0, 2,10,14,0xff,0xff,0xff,0xff},
  { 4,10,14,0xff,0xff,0xff,0xff,0xff}, { 0, 4,10,14,0xff,0xff,0xff,0xff},
  { 2, 4,10,14,0xff,0xff,0xff,0xff}, { 0, 2, 4,10,14,0xff,0xff,0xff},
  { 6,10,14,0xff,0xff,0xff,0xff,0xff}, { 0, 6,10,14,0xff,0xff,0xff,0xff},
  { 2, 6,10,14,0xff,0xff,0xff,0xff}, { 0, 2, 6,10,14,0xff,0xff,0xff},
  { 4, 6,10,14,0xff,0xff,0xff,0xff}, { 0, 4, 6,10,14,0xff,0xff,0xff},
  { 2, 4, 6,10,14,0xff,0xff,0xff}, { 0, 2, 4, 6,10,14,0xff,0xff},
  { 8,10,14,0xff,0xff,0xff,0xff,0xff}, { 0, 8,10,14,0xff,0xff,0xff,0xff},
  { 2, 8,10,14,0xff,0xff,0xff,0xff}, { 0, 2, 8,10,14,0xff,0xff,0xff},
  { 4, 8,10,14,0xff,0xff,0xff,0xff}, { 0, 4, 8,10,14,0xff,0xff,0xff},
  { 2, 4, 8,10,14,0xff,0xff,0xff}, { 0, 2, 4, 8,10,14,0xff,0xff},
  { 6, 8,10,14,0xff,0xff,0xff,0xff}, { 0, 6, 8,10,14,0xff,0xff,0xff},
  { 2, 6, 8,10,14,0xff,0xff,0xff}, { 0, 2, 6, 8,10,14,0xff,0xff},
  { 4, 6, 8,10,14,0xff,0xff,0xff}, { 0, 4, 6, 8,10,14,0xff,0xff},
  { 2, 4, 6, 8,10,14,0xff,0xff}, { 0, 2, 4, 6, 8,10,14,0xff},
  {12,14,0xff,0xff,0xff,0xff,0xff,0xff}, { 0,12,14,0xff,0xff,0xff,0xff,0xff},
  { 2,12,14,0xff,0xff,0xff,0xff,0xff}, { 0, 2,12,14,0xff,0xff,0xff,0xff},
  { 4,12,14,0xff,0xff,0xff,0xff,0xff}, { 0, 4,12,14,0xff,0xff,0xff,0xff},
  { 2, 4,12,14,0xff,0xff,0xff,0xff}, { 0, 2, 4,12,14,0xff,0xff,0xff},
  { 6,12,14,0xff,0xff,0xff,0xff,0xff}, { 0, 6,12,14,0xff,0xff,0xff,0xff},
  { 2, 6,12,14,0xff,0xff,0xff,0xff}, { 0, 2, 6,12,14,0xff,0xff,0xff},
  { 4, 6,12,14,0xff,0xff,0xff,0xff}, { 0, 4, 6,12,14,0xff,0xff,0xff},
  { 2, 4, 6,12,14,0xff,0xff,0xff}, { 0, 2, 4, 6,12,14,0xff,0xff},
  { 8,12,14,0xff,0xff,0xff,0xff,0xff}, { 0, 8,12,14,0xff,0xff,0xff,0xff},
  { 2, 8,12,14,0xff,0xff,0xff,0xff}, { 0, 2, 8,12,14,0xff,0xff,0xff},
  { 4, 8,12,14,0xff,0xff,0xff,0xff}, { 0, 4, 8,12,14,0xff,0xff,0xff},
  { 2, 4, 8,12,14,0xff,0xff,0xff}, { 0, 2, 4, 8,12,14,0xff,0xff},
  { 6, 8,12,14,0xff,0xff,0xff,0xff}, { 0, 6, 8,12,14,0xff,0xff,0xff},
  { 2, 6, 8,12,14,0xff,0xff,0xff}, { 0, 2, 6, 8,12,14,0xff,0xff},
  { 4, 6, 8,12,14,0xff,0xff,0xff}, { 0, 4, 6, 8,12,14,0xff,0xff},
  { 2, 4, 6, 8,12,14,0xff,0xff}, { 0, 2, 4, 6, 8,12,14,0xff},
  {10,12,14,0xff,0xff,0xff,0xff,0xff}, { 0,10,12,14,0xff,0xff,0xff,0xff},
  { 2,10,12,14,0xff,0xff,0xff,0xff}, { 0, 2,10,12,14,0xff,0xff,0xff},
  { 4,10,12,14,0xff,0xff,0xff,0xff}, { 0, 4,10,12,14,0xff,0xff,0xff},
  { 2, 4,10,12,14,0xff,0xff,0xff}, { 0, 2, 4,10,12,14,0xff,0xff},
  { 6,10,12,14,0xff,0xff,0xff,0xff}, { 0, 6,10,12,14,0xff,0xff,0xff},
  { 2, 6,10,12,14,0xff,0xff,0xff}, { 0, 2, 6,10,12,14,0xff,0xff},
  { 4, 6,10,12,14,0xff,0xff,0xff}, { 0, 4, 6,10,12,14,0xff,0xff},
  { 2, 4, 6,10,12,14,0xff,0xff}, { 0, 2, 4, 6,10,12,14,0xff},
  { 8,10,12,14,0xff,0xff,0xff,0xff}, { 0, 8,10,12,14,0xff,0xff,0xff},
  { 2, 8,10,12,14,0xff,0xff,0xff}, { 0, 2, 8,10,12,14,0xff,0xff},
  { 4, 8,10,12,14,0xff,0xff,0xff}, { 0, 4, 8,10,12,14,0xff,0xff},
  { 2, 4, 8,10,12,14,0xff,0xff}, { 0, 2, 4, 8,10,12,14,0xff},
  { 6, 8,10,12,14,0xff,0xff,0xff}, { 0, 6, 8,10,12,14,0xff,0xff},
  { 2, 6, 8,10,12,14,0xff,0xff}, { 0, 2, 6, 8,10,12,14,0xff},
  { 4, 6, 8,10,12,14,0xff,0xff}, { 0, 4, 6, 8,10,12,14,0xff},
  { 2, 4, 6, 8,10,12,14,0xff}, { 0, 2, 4, 6, 8,10,12,14}
};

/*
 * AVX2-accelerated 13-bit rejection sampling for Q=7681.
 * Extraction is scalar (irregular 13-bit layout), but comparison
 * and compaction use AVX2 for the 8-value batch.
 */
unsigned int rej_uniform(int16_t *res,
                         unsigned int len,
                         const unsigned char *buf,
                         unsigned int buflen) {
    unsigned int ctr, pos, k;
    uint16_t val;
    int16_t t[8];
    const __m128i bound = _mm_set1_epi16((int16_t)DKE_Q);
    const __m128i ones  = _mm_set1_epi8(1);
    ctr = pos = 0;

    /* Main loop: extract 8 x 13-bit values from 13 bytes, AVX compare+compact */
    while (ctr + 8 <= len && pos + 13 <= buflen) {
        /* Scalar 13-bit extraction (irregular bit layout) */
        t[0] = (int16_t)(((buf[pos+ 0] >> 0) | ((uint16_t)buf[pos+ 1] << 8))         & 0x1FFF);
        t[1] = (int16_t)(((buf[pos+ 1] >> 5) | ((uint16_t)buf[pos+ 2] << 3)
                                               | ((uint16_t)buf[pos+ 3] << 11))        & 0x1FFF);
        t[2] = (int16_t)(((buf[pos+ 3] >> 2) | ((uint16_t)buf[pos+ 4] << 6))         & 0x1FFF);
        t[3] = (int16_t)(((buf[pos+ 4] >> 7) | ((uint16_t)buf[pos+ 5] << 1)
                                               | ((uint16_t)buf[pos+ 6] << 9))         & 0x1FFF);
        t[4] = (int16_t)(((buf[pos+ 6] >> 4) | ((uint16_t)buf[pos+ 7] << 4)
                                               | ((uint16_t)buf[pos+ 8] << 12))        & 0x1FFF);
        t[5] = (int16_t)(((buf[pos+ 8] >> 1) | ((uint16_t)buf[pos+ 9] << 7))         & 0x1FFF);
        t[6] = (int16_t)(((buf[pos+ 9] >> 6) | ((uint16_t)buf[pos+10] << 2)
                                               | ((uint16_t)buf[pos+11] << 10))        & 0x1FFF);
        t[7] = (int16_t)(((buf[pos+11] >> 3) | ((uint16_t)buf[pos+12] << 5))         & 0x1FFF);
        pos += 13;

        /* SSE compare + pshufb compact */
        __m128i v = _mm_loadu_si128((__m128i *)t);
        __m128i cmp = _mm_cmpgt_epi16(bound, v);
        unsigned int good = (unsigned int)_mm_movemask_epi8(cmp);

        /* Extract one bit per 16-bit lane */
        good &= 0x5555;
        good = (good | (good >> 1)) & 0x3333;
        good = (good | (good >> 2)) & 0x0F0F;
        good = (good | (good >> 4)) & 0x00FF;

        __m128i pilo = _mm_loadl_epi64((__m128i *)&rej_idx_512[good]);
        __m128i pihi = _mm_add_epi8(pilo, ones);
        pilo = _mm_unpacklo_epi8(pilo, pihi);
        v = _mm_shuffle_epi8(v, pilo);
        _mm_storeu_si128((__m128i *)&res[ctr], v);
        ctr += _mm_popcnt_u32(good);
    }

    /* Scalar tail */
    while (ctr < len && pos + 13 <= buflen) {
        int16_t tt[8];
        tt[0] = (int16_t)(((buf[pos+ 0] >> 0) | ((uint16_t)buf[pos+ 1] << 8))         & 0x1FFF);
        tt[1] = (int16_t)(((buf[pos+ 1] >> 5) | ((uint16_t)buf[pos+ 2] << 3)
                                                | ((uint16_t)buf[pos+ 3] << 11))        & 0x1FFF);
        tt[2] = (int16_t)(((buf[pos+ 3] >> 2) | ((uint16_t)buf[pos+ 4] << 6))         & 0x1FFF);
        tt[3] = (int16_t)(((buf[pos+ 4] >> 7) | ((uint16_t)buf[pos+ 5] << 1)
                                                | ((uint16_t)buf[pos+ 6] << 9))         & 0x1FFF);
        tt[4] = (int16_t)(((buf[pos+ 6] >> 4) | ((uint16_t)buf[pos+ 7] << 4)
                                                | ((uint16_t)buf[pos+ 8] << 12))        & 0x1FFF);
        tt[5] = (int16_t)(((buf[pos+ 8] >> 1) | ((uint16_t)buf[pos+ 9] << 7))         & 0x1FFF);
        tt[6] = (int16_t)(((buf[pos+ 9] >> 6) | ((uint16_t)buf[pos+10] << 2)
                                                | ((uint16_t)buf[pos+11] << 10))        & 0x1FFF);
        tt[7] = (int16_t)(((buf[pos+11] >> 3) | ((uint16_t)buf[pos+12] << 5))         & 0x1FFF);
        pos += 13;
        for (k = 0; k < 8 && ctr < len; k++) {
            if (tt[k] < DKE_Q)
                res[ctr++] = (int16_t)tt[k];
        }
    }

    while (ctr < len && pos + 2 <= buflen) {
        val = ((buf[pos] >> 0) | ((uint16_t)buf[pos+1] << 8)) & 0x1FFF;
        pos += 2;
        if (val < DKE_Q)
            res[ctr++] = (int16_t)val;
    }

    return ctr;
}

#elif defined(DKE_USE_AARCH64)
/* rej_uniform supplied by aarch64/rejsample_neon.c (NEON 13-bit) */
#else
/* Pure scalar fallback for DKE-512 without AVX2/NEON */
unsigned int rej_uniform(int16_t *res,
                         unsigned int len,
                         const unsigned char *buf,
                         unsigned int buflen) {
    unsigned int ctr, pos, k;
    uint16_t val;
    uint16_t t[8];
    ctr = pos = 0;

    while (ctr < len && pos + 13 <= buflen) {
        t[0] = ((buf[pos+ 0] >> 0) | ((uint16_t)buf[pos+ 1] << 8))         & 0x1FFF;
        t[1] = ((buf[pos+ 1] >> 5) | ((uint16_t)buf[pos+ 2] << 3)
                                    | ((uint16_t)buf[pos+ 3] << 11))        & 0x1FFF;
        t[2] = ((buf[pos+ 3] >> 2) | ((uint16_t)buf[pos+ 4] << 6))         & 0x1FFF;
        t[3] = ((buf[pos+ 4] >> 7) | ((uint16_t)buf[pos+ 5] << 1)
                                    | ((uint16_t)buf[pos+ 6] << 9))         & 0x1FFF;
        t[4] = ((buf[pos+ 6] >> 4) | ((uint16_t)buf[pos+ 7] << 4)
                                    | ((uint16_t)buf[pos+ 8] << 12))        & 0x1FFF;
        t[5] = ((buf[pos+ 8] >> 1) | ((uint16_t)buf[pos+ 9] << 7))         & 0x1FFF;
        t[6] = ((buf[pos+ 9] >> 6) | ((uint16_t)buf[pos+10] << 2)
                                    | ((uint16_t)buf[pos+11] << 10))        & 0x1FFF;
        t[7] = ((buf[pos+11] >> 3) | ((uint16_t)buf[pos+12] << 5))         & 0x1FFF;
        pos += 13;
        for (k = 0; k < 8 && ctr < len; k++) {
            if (t[k] < DKE_Q)
                res[ctr++] = t[k];
        }
    }

    while (ctr < len && pos + 2 <= buflen) {
        val = ((buf[pos] >> 0) | ((uint16_t)buf[pos+1] << 8)) & 0x1FFF;
        pos += 2;
        if (val < DKE_Q)
            res[ctr++] = val;
    }

    return ctr;
}
#endif /* DKE_USE_AVX2 */

#else
// 12-bit rejection sampling for q=3329.
// Extracts 2 values of 12 bits each from every 3 bytes of input.
// Rejection rate: 1 - 3329/4096 ~~ 18.7%.
#if defined(DKE_USE_AVX2)
  /* rej_uniform supplied by avx2/rejsample_avx2.c (vectorized pshufb compaction) */
#elif defined(DKE_USE_AARCH64)
  /* rej_uniform supplied by aarch64/rejsample_neon.c */
#else
unsigned int rej_uniform(int16_t *res,
                         unsigned int len,
                         const unsigned char *buf,
                         unsigned int buflen) {
    unsigned int ctr, pos;
    uint16_t val0, val1;
    ctr = pos = 0;
    while (ctr < len && pos + 3 <= buflen) {
        val0 = ((buf[pos+0] >> 0) | ((uint16_t)buf[pos+1] << 8)) & 0xFFF;
        val1 = ((buf[pos+1] >> 4) | ((uint16_t)buf[pos+2] << 4)) & 0xFFF;
        pos += 3;
        if (val0 < DKE_Q) res[ctr++] = val0;
        if (ctr < len && val1 < DKE_Q) res[ctr++] = val1;
    }
    return ctr;
}
#endif /* !DKE_USE_AVX2 && !DKE_USE_AARCH64 */

#endif

// XOF state for gen_matrix ----------------------------------------------------

#if defined(DKE_USE_OPT_C) && !defined(DKE_HASH_SHAKE)

/*
 * Optimized incremental XOF: maintains a counter and generates SM3 blocks
 * on demand. Each squeeze only computes the NEW blocks needed, never
 * re-computes previously generated output.
 *
 * pseudoXOF(output_len, extseed, ...) = SM3(extseed||1) || SM3(extseed||2) || ...
 * where || denotes big-endian 4-byte counter concatenation.
 */

typedef struct {
    uint8_t msg_buf[DKE_SEEDBYTES + 2 + 4]; /* extseed || counter */
    size_t  msg_bytes;                        /* = SEEDBYTES + 2 */
    unsigned int counter;                     /* next counter value (starts at 1) */
    /* Partial block buffer: pseudoXOF output is 32-byte aligned (SM3 blocks),
     * but DKE_XOF_BLOCKBYTES=168 is not a multiple of 32.
     * We buffer leftover bytes from the last SM3 block. */
    uint8_t residue[32];
    size_t  residue_len;                      /* bytes available in residue */
} dke_xof_state;

static void dke_xof_absorb(dke_xof_state *state,
                            const uint8_t seed[DKE_SEEDBYTES],
                            uint8_t x, uint8_t y) {
    state->msg_bytes = DKE_SEEDBYTES + 2;
    memcpy(state->msg_buf, seed, DKE_SEEDBYTES);
    state->msg_buf[DKE_SEEDBYTES + 0] = x;
    state->msg_buf[DKE_SEEDBYTES + 1] = y;
    state->counter = 1;
    state->residue_len = 0;
}

/* Generate one 32-byte SM3 block with current counter, advance counter */
static inline void dke_xof_gen_one(dke_xof_state *state, uint8_t out[32]) {
    unsigned int ct = state->counter;
    size_t mb = state->msg_bytes;
    state->msg_buf[mb + 0] = (uint8_t)(ct >> 24);
    state->msg_buf[mb + 1] = (uint8_t)(ct >> 16);
    state->msg_buf[mb + 2] = (uint8_t)(ct >> 8);
    state->msg_buf[mb + 3] = (uint8_t)(ct);
    dke_hash256(state->msg_buf, (unsigned long long)(mb + 4) * 8ULL, out);
    state->counter++;
}

static void dke_xof_squeezeblocks(uint8_t *out,
                                   size_t outblocks,
                                   dke_xof_state *state) {
    size_t outlen = outblocks * (size_t)DKE_XOF_BLOCKBYTES;
    size_t written = 0;

    if (outlen == 0) return;

    /* First, drain any residue from previous squeeze */
    if (state->residue_len > 0) {
        size_t take = state->residue_len;
        if (take > outlen) take = outlen;
        memcpy(out, state->residue + (32 - state->residue_len), take);
        state->residue_len -= take;
        written += take;
    }

    /* Generate full 32-byte SM3 blocks directly into output */
    while (written + 32 <= outlen) {
        dke_xof_gen_one(state, out + written);
        written += 32;
    }

    /* Handle remaining bytes (< 32): generate one more block, buffer the rest */
    if (written < outlen) {
        uint8_t tmp[32];
        dke_xof_gen_one(state, tmp);
        size_t need = outlen - written;
        memcpy(out + written, tmp, need);
        /* Save leftover for next squeeze */
        state->residue_len = 32 - need;
        memcpy(state->residue + (32 - state->residue_len), tmp + need, state->residue_len);
    }
}

static void dke_xof_release(dke_xof_state *state) {
    state->counter = 1;
    state->residue_len = 0;
}

#elif defined(DKE_HASH_SHAKE)

/*
 * Incremental SHAKE128 XOF state for gen_matrix (HASH=2, ML-KEM suite).
 * Uses keccak_state directly — no malloc/free overhead.
 * shake128_absorb_once absorbs seed||x||y and readies state for squeezing.
 * Subsequent shake128_squeezeblocks calls continue the stream incrementally.
 */
#include "fips202.h"

typedef struct {
    keccak_state kst;
} dke_xof_state;

static void dke_xof_absorb(dke_xof_state *state,
                            const uint8_t seed[DKE_SEEDBYTES],
                            uint8_t x, uint8_t y) {
    uint8_t extseed[DKE_SEEDBYTES + 2];
    memcpy(extseed, seed, DKE_SEEDBYTES);
    extseed[DKE_SEEDBYTES + 0] = x;
    extseed[DKE_SEEDBYTES + 1] = y;
    shake128_absorb_once(&state->kst, extseed, DKE_SEEDBYTES + 2);
}

static void dke_xof_squeezeblocks(uint8_t *out,
                                   size_t outblocks,
                                   dke_xof_state *state) {
    shake128_squeezeblocks(out, outblocks, &state->kst);
}

static void dke_xof_release(dke_xof_state *state) { (void)state; }

#else /* Original SM3 implementation (no DKE_USE_OPT_C, no DKE_HASH_SHAKE) */

typedef struct {
    uint8_t extseed[DKE_SEEDBYTES + 2];
    size_t generated_bytes;
} dke_xof_state;

static void dke_xof_absorb(dke_xof_state *state,
                            const uint8_t seed[DKE_SEEDBYTES],
                            uint8_t x, uint8_t y) {
    memcpy(state->extseed, seed, DKE_SEEDBYTES);
    state->extseed[DKE_SEEDBYTES + 0] = x;
    state->extseed[DKE_SEEDBYTES + 1] = y;
    state->generated_bytes = 0;
}

static void dke_xof_squeezeblocks(uint8_t *out,
                                   size_t outblocks,
                                   dke_xof_state *state) {
    size_t outlen = outblocks * (size_t)DKE_XOF_BLOCKBYTES;
    size_t needed = state->generated_bytes + outlen;
    unsigned char *prefix;
    if (outlen == 0) return;
    if (needed > (size_t)(ULLONG_MAX / 8ULL)) {
        fprintf(stderr, "FATAL: XOF output length overflow at %s, line %d.\n", __FILE__, __LINE__);
        abort();
    }
    prefix = (unsigned char *)malloc(needed);
    if (prefix == NULL) {
        fprintf(stderr, "FATAL: Memory allocation failed at %s, line %d.\n", __FILE__, __LINE__);
        abort();
    }
    if (dke_xof((unsigned long long)needed * 8ULL,
                  state->extseed,
                  (unsigned long long)sizeof(state->extseed) * 8ULL,
                  prefix) != 0) {
        fprintf(stderr, "FATAL: pseudoXOF failed at %s, line %d.\n", __FILE__, __LINE__);
        free(prefix);
        abort();
    }
    memcpy(out, prefix + state->generated_bytes, outlen);
    state->generated_bytes += outlen;
    free(prefix);
}

static void dke_xof_release(dke_xof_state *state) {
    state->generated_bytes = 0;
}

#endif /* DKE_USE_OPT_C / DKE_HASH_SHAKE / else */

/* ── DKE-512 Cortex-M4 matacc XOF bridge ───────────────────────────────────
 * plantard512_matacc_asm.S calls `bl dke3_xof_squeezeblocks` (ARM AAPCS:
 *   r0=out, r1=nblocks, r2=state).  We expose non-static wrappers here so
 * the linker can resolve the symbol from the same translation unit that has
 * the dke_xof_state definition.
 *
 * DKE3_XOF_STATE_BYTES (128) in random_sampling.h is a safe upper bound for
 * sizeof(dke_xof_state) across all backend variants.                        */
#if DKE_N == 512 && defined(DKE_USE_CORTEX_M4_PLANTARD)
void dke3_xof_absorb(uint8_t state_buf[/* DKE3_XOF_STATE_BYTES */],
                     const uint8_t seed[DKE_SEEDBYTES],
                     uint8_t x, uint8_t y) {
    dke_xof_absorb((dke_xof_state *)state_buf, seed, x, y);
}
void dke3_xof_squeezeblocks(uint8_t *out, size_t nblocks,
                             uint8_t state_buf[/* DKE3_XOF_STATE_BYTES */]) {
    dke_xof_squeezeblocks(out, nblocks, (dke_xof_state *)state_buf);
}
#endif /* DKE_N == 512 && DKE_USE_CORTEX_M4_PLANTARD */

// Matrix generation -----------------------------------------------------------

#if defined(DKE_HASH_SHAKE) && (defined(DKE_USE_AVX2) || defined(DKE_USE_AARCH64_NATIVE))

/* 4x parallel SHAKE128 matrix generation (ML-KEM-style).
 * Generates 4 matrix elements simultaneously using shake128x4. */
#include "fips202.h"
#if defined(DKE_USE_AARCH64_NATIVE)
#include "aarch64-native/fips202x4_aarch64.h"
#else
#include "fips202x4.h"
#endif

void DKE_gen_matrix(polyvec *res,
                    const uint8_t seed[DKE_SEEDBYTES],
                    const int transposed) {
    unsigned int k;
    unsigned int buflen;
    /* Process K*K elements in groups of 4 — aligned for AVX2 */
#if defined(_MSC_VER)
    __declspec(align(32)) uint8_t bufs[4][DKE_GEN_MATRIX_NBLOCKS * SHAKE128_RATE];
    __declspec(align(32)) uint8_t extseed[4][DKE_SEEDBYTES + 2];
#elif defined(__GNUC__)
    uint8_t bufs[4][DKE_GEN_MATRIX_NBLOCKS * SHAKE128_RATE] __attribute__((aligned(32)));
    uint8_t extseed[4][DKE_SEEDBYTES + 2] __attribute__((aligned(32)));
#else
    uint8_t bufs[4][DKE_GEN_MATRIX_NBLOCKS * SHAKE128_RATE];
    uint8_t extseed[4][DKE_SEEDBYTES + 2];
#endif
    /* Indices for the 4 elements being processed */
    unsigned int idx_i[4], idx_j[4];

    /* Prepare all K*K (i,j) pairs */
    unsigned int total = DKE_K * DKE_K;
    unsigned int done = 0;

    while (done + 4 <= total) {
        /* Prepare 4 seeds */
#if defined(DKE_USE_AVX2) && DKE_SEEDBYTES == 32
        {
            __m256i f = _mm256_loadu_si256((const __m256i *)seed);
            _mm256_storeu_si256((__m256i *)extseed[0], f);
            _mm256_storeu_si256((__m256i *)extseed[1], f);
            _mm256_storeu_si256((__m256i *)extseed[2], f);
            _mm256_storeu_si256((__m256i *)extseed[3], f);
        }
#elif defined(DKE_USE_AVX2) && DKE_SEEDBYTES == 64
        {
            __m256i f0 = _mm256_loadu_si256((const __m256i *)seed);
            __m256i f1 = _mm256_loadu_si256((const __m256i *)(seed + 32));
            _mm256_storeu_si256((__m256i *)extseed[0], f0);
            _mm256_storeu_si256((__m256i *)(extseed[0]+32), f1);
            _mm256_storeu_si256((__m256i *)extseed[1], f0);
            _mm256_storeu_si256((__m256i *)(extseed[1]+32), f1);
            _mm256_storeu_si256((__m256i *)extseed[2], f0);
            _mm256_storeu_si256((__m256i *)(extseed[2]+32), f1);
            _mm256_storeu_si256((__m256i *)extseed[3], f0);
            _mm256_storeu_si256((__m256i *)(extseed[3]+32), f1);
        }
#else
        for (k = 0; k < 4; k++) memcpy(extseed[k], seed, DKE_SEEDBYTES);
#endif
        for (k = 0; k < 4; k++) {
            unsigned int flat = done + k;
            unsigned int ii = flat / DKE_K;
            unsigned int jj = flat % DKE_K;
            idx_i[k] = ii;
            idx_j[k] = jj;
            if (transposed) {
                extseed[k][DKE_SEEDBYTES + 0] = (uint8_t)ii;
                extseed[k][DKE_SEEDBYTES + 1] = (uint8_t)jj;
            } else {
                extseed[k][DKE_SEEDBYTES + 0] = (uint8_t)jj;
                extseed[k][DKE_SEEDBYTES + 1] = (uint8_t)ii;
            }
        }

        /* 4x SHAKE128 absorb + squeeze */
#if defined(_MSC_VER)
        __declspec(align(32)) keccakx4_state state;
#else
        keccakx4_state state __attribute__((aligned(32)));
#endif
        shake128x4_absorb_once(&state,
            extseed[0], extseed[1], extseed[2], extseed[3],
            DKE_SEEDBYTES + 2);
        shake128x4_squeezeblocks(bufs[0], bufs[1], bufs[2], bufs[3],
            DKE_GEN_MATRIX_NBLOCKS, &state);
        buflen = DKE_GEN_MATRIX_NBLOCKS * SHAKE128_RATE;

        /* Rejection sampling on each of the 4 streams.
         * If initial squeeze is insufficient, continue with 4x squeeze
         * to maintain parallelism (instead of falling back to scalar). */
        unsigned int ctr0, ctr1, ctr2, ctr3;
        ctr0 = rej_uniform(res[idx_i[0]].vec[idx_j[0]].coeffs, DKE_N, bufs[0], buflen);
        ctr1 = rej_uniform(res[idx_i[1]].vec[idx_j[1]].coeffs, DKE_N, bufs[1], buflen);
        ctr2 = rej_uniform(res[idx_i[2]].vec[idx_j[2]].coeffs, DKE_N, bufs[2], buflen);
        ctr3 = rej_uniform(res[idx_i[3]].vec[idx_j[3]].coeffs, DKE_N, bufs[3], buflen);

        while (ctr0 < DKE_N || ctr1 < DKE_N || ctr2 < DKE_N || ctr3 < DKE_N) {
            shake128x4_squeezeblocks(bufs[0], bufs[1], bufs[2], bufs[3], 1, &state);
            ctr0 += rej_uniform(res[idx_i[0]].vec[idx_j[0]].coeffs + ctr0,
                                DKE_N - ctr0, bufs[0], SHAKE128_RATE);
            ctr1 += rej_uniform(res[idx_i[1]].vec[idx_j[1]].coeffs + ctr1,
                                DKE_N - ctr1, bufs[1], SHAKE128_RATE);
            ctr2 += rej_uniform(res[idx_i[2]].vec[idx_j[2]].coeffs + ctr2,
                                DKE_N - ctr2, bufs[2], SHAKE128_RATE);
            ctr3 += rej_uniform(res[idx_i[3]].vec[idx_j[3]].coeffs + ctr3,
                                DKE_N - ctr3, bufs[3], SHAKE128_RATE);
        }

#if (defined(_MSC_VER) || defined(DKM_LINUX)) && (DKE_N == 256) && !defined(DKE_AVX2_NTT_INTRINSIC)
        /* Convert sequential format to packed NTT format.
         * res[] is _mm_malloc'd (32-byte aligned), safe for vmovdqa. */
        {
#if defined(_MSC_VER)
            extern void dke_nttunpack_avx2(int16_t *r);
#define _nttunpack(r) dke_nttunpack_avx2(r)
#else
            extern void dke_nttunpack_avx2_linux(int16_t *r);
#define _nttunpack(r) dke_nttunpack_avx2_linux(r)
#endif
            _nttunpack(res[idx_i[0]].vec[idx_j[0]].coeffs);
            _nttunpack(res[idx_i[1]].vec[idx_j[1]].coeffs);
            _nttunpack(res[idx_i[2]].vec[idx_j[2]].coeffs);
            _nttunpack(res[idx_i[3]].vec[idx_j[3]].coeffs);
#undef _nttunpack
        }
#endif
        done += 4;
    }

    /* Handle remaining elements (< 4) with scalar SHAKE128 */
    for (; done < total; done++) {
        unsigned int ctr;
        unsigned int ii = done / DKE_K;
        unsigned int jj = done % DKE_K;
        uint8_t ext[DKE_SEEDBYTES + 2];
        memcpy(ext, seed, DKE_SEEDBYTES);
        if (transposed) {
            ext[DKE_SEEDBYTES + 0] = (uint8_t)ii;
            ext[DKE_SEEDBYTES + 1] = (uint8_t)jj;
        } else {
            ext[DKE_SEEDBYTES + 0] = (uint8_t)jj;
            ext[DKE_SEEDBYTES + 1] = (uint8_t)ii;
        }
        keccak_state kst;
        shake128_absorb_once(&kst, ext, DKE_SEEDBYTES + 2);
        shake128_squeezeblocks(bufs[0], DKE_GEN_MATRIX_NBLOCKS, &kst);
        ctr = rej_uniform(res[ii].vec[jj].coeffs, DKE_N, bufs[0],
                          DKE_GEN_MATRIX_NBLOCKS * SHAKE128_RATE);
        while (ctr < DKE_N) {
            shake128_squeezeblocks(bufs[0], 1, &kst);
            ctr += rej_uniform(res[ii].vec[jj].coeffs + ctr,
                               DKE_N - ctr, bufs[0], SHAKE128_RATE);
        }
#if (defined(_MSC_VER) || defined(DKM_LINUX)) && (DKE_N == 256) && !defined(DKE_AVX2_NTT_INTRINSIC)
        {
#if defined(_MSC_VER)
            extern void dke_nttunpack_avx2(int16_t *r);
#define _nttunpack(r) dke_nttunpack_avx2(r)
#else
            extern void dke_nttunpack_avx2_linux(int16_t *r);
#define _nttunpack(r) dke_nttunpack_avx2_linux(r)
#endif
            _nttunpack(res[ii].vec[jj].coeffs);
#undef _nttunpack
        }
#endif
    }
}

#else /* SM3 or non-AVX2: original gen_matrix */


void DKE_gen_matrix(polyvec *res,
                    const uint8_t seed[DKE_SEEDBYTES],
                    const int transposed) {
    unsigned int ctr, buflen;
    dke_xof_state state;
    uint8_t buf[DKE_GEN_MATRIX_NBLOCKS * DKE_XOF_BLOCKBYTES];

    for (unsigned int i = 0; i < DKE_K; ++i) {
        for (unsigned int j = 0; j < DKE_K; ++j) {
            if (transposed)
                dke_xof_absorb(&state, seed, (uint8_t)i, (uint8_t)j);
            else
                dke_xof_absorb(&state, seed, (uint8_t)j, (uint8_t)i);

            dke_xof_squeezeblocks(buf, DKE_GEN_MATRIX_NBLOCKS, &state);
            buflen = DKE_GEN_MATRIX_NBLOCKS * DKE_XOF_BLOCKBYTES;
            ctr = rej_uniform(res[i].vec[j].coeffs, DKE_N, buf, buflen);

            while (ctr < DKE_N) {
                dke_xof_squeezeblocks(buf, 1, &state);
                buflen = DKE_XOF_BLOCKBYTES;
                ctr += rej_uniform(res[i].vec[j].coeffs + ctr,
                                   DKE_N - ctr, buf, buflen);
            }
            dke_xof_release(&state);
        }
    }
}


#endif /* DKE_HASH_SHAKE && (DKE_USE_AVX2 || DKE_USE_AARCH64_NATIVE) */
