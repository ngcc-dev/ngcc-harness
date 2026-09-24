#include "poly_mul_ntt_avx2.h"
#include "ntt.h"
#include "reduce.h"
#include "params.h"

#ifdef LORE_USE_AVX2_POLYMUL_NTT
#include <immintrin.h>

/* 8-lane AVX2 fqmul (verified oracle PASS) */
static inline __m256i fqmul_8way(__m256i a32, __m256i b32) {
    __m256i prod = _mm256_mullo_epi32(a32, b32);
    __m256i qinv = _mm256_set1_epi32(-255);
    __m256i v32  = _mm256_mullo_epi32(prod, qinv);
    v32 = _mm256_srai_epi32(_mm256_slli_epi32(v32, 16), 16);
    __m256i q = _mm256_set1_epi32((int)LORE_Q);
    __m256i t = _mm256_mullo_epi32(v32, q);
    __m256i r = _mm256_sub_epi32(prod, t);
    r = _mm256_srai_epi32(r, 16);
    return r;
}

/*
 * Compute basemul4 for 8 elements (2 groups of 4) using AVX2.
 * a[0..3],b[0..3] → r[0..3] with zeta
 * a[4..7],b[4..7] → r[4..7] with -zeta
 *
 * Uses fqmul_8way to batch 8 fqmul calls per convolution step,
 * keeps accumulation in int32, folds and stores 8 int16 results.
 */
static void basemul8_avx2(
    int16_t r[8],
    const int16_t a[8], const int16_t b[8],
    int16_t zeta)
{
    /* Extend a[0..7], b[0..7] to int32 */
    __m128i a16 = _mm_loadu_si128((const __m128i*)a);
    __m128i b16 = _mm_loadu_si128((const __m128i*)b);
    __m256i a32 = _mm256_cvtepi16_epi32(a16);
    __m256i b32 = _mm256_cvtepi16_epi32(b16);

    /*
     * 4×4 convolution for low group (first 4):
     *   c_low[i+j] += fqmul(a_low[i], b_low[j])
     *
     * To use fqmul_8way efficiently, broadcast each a[i] across 4 lanes
     * and multiply by b[0..3]. This gives c[i+0..i+3] in one SIMD op.
     *
     * Product rows:
     *   i=0: a[0] * b[0..3] → c[0..3]
     *   i=1: a[1] * b[0..3] → c[1..4]
     *   i=2: a[2] * b[0..3] → c[2..5]
     *   i=3: a[3] * b[0..3] → c[3..6]
     */

    /* Extract low/high 4 lanes */
    int32_t al[4], bl[4], ah[4], bh[4], aa[8], bb[8];
    _mm256_storeu_si256((__m256i*)aa, a32);
    _mm256_storeu_si256((__m256i*)bb, b32);
    for (int i = 0; i < 4; i++) { al[i] = aa[i]; bl[i] = bb[i]; ah[i] = aa[i+4]; bh[i] = bb[i+4]; }

    /* Low group: basemul4 with zeta */
    int32_t cl[7] = {0};
    for (int i = 0; i < 4; i++) {
        __m256i ai = _mm256_set1_epi32(al[i]);
        __m256i bj32 = _mm256_setr_epi32(bl[0], bl[1], bl[2], bl[3], 0, 0, 0, 0);
        __m256i prod = fqmul_8way(ai, bj32);
        int32_t p[8]; _mm256_storeu_si256((__m256i*)p, prod);
        for (int j = 0; j < 4; j++) cl[i+j] += p[j];
    }
    /* Fold with zeta */
    int32_t z32 = (int32_t)zeta;
    int16_t rl[4];
    rl[0] = (int16_t)(cl[0] + (int32_t)fqmul((int16_t)cl[4], zeta));
    rl[1] = (int16_t)(cl[1] + (int32_t)fqmul((int16_t)cl[5], zeta));
    rl[2] = (int16_t)(cl[2] + (int32_t)fqmul((int16_t)cl[6], zeta));
    rl[3] = (int16_t)cl[3];

    /* High group: basemul4 with -zeta */
    int32_t ch[7] = {0};
    int32_t nz32 = -(int32_t)zeta;
    for (int i = 0; i < 4; i++) {
        __m256i ai = _mm256_set1_epi32(ah[i]);
        __m256i bj32 = _mm256_setr_epi32(bh[0], bh[1], bh[2], bh[3], 0, 0, 0, 0);
        __m256i prod = fqmul_8way(ai, bj32);
        int32_t p[8]; _mm256_storeu_si256((__m256i*)p, prod);
        for (int j = 0; j < 4; j++) ch[i+j] += p[j];
    }
    int16_t rh[4];
    rh[0] = (int16_t)(ch[0] + (int32_t)fqmul((int16_t)ch[4], (int16_t)nz32));
    rh[1] = (int16_t)(ch[1] + (int32_t)fqmul((int16_t)ch[5], (int16_t)nz32));
    rh[2] = (int16_t)(ch[2] + (int32_t)fqmul((int16_t)ch[6], (int16_t)nz32));
    rh[3] = (int16_t)ch[3];

    /* Store results */
    r[0]=rl[0]; r[1]=rl[1]; r[2]=rl[2]; r[3]=rl[3];
    r[4]=rh[0]; r[5]=rh[1]; r[6]=rh[2]; r[7]=rh[3];
}

void poly_mul_ntt_avx2(int16_t r[512], const int16_t a[512], const int16_t b[512])
{
    extern int16_t zetas[128];

    for (int i = 0; i < 64; i++) {
        int16_t zeta = zetas[64 + i];
        basemul8_avx2(&r[8*i], &a[8*i], &b[8*i], zeta);
    }
}
#endif /* LORE_USE_AVX2_POLYMUL_NTT */
