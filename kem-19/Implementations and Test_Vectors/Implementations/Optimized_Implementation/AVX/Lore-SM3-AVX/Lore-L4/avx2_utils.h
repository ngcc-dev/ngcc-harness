#ifndef LORE_AVX2_UTILS_H
#define LORE_AVX2_UTILS_H

#include <immintrin.h>
#include <stdint.h>

/*
 * Barrett reduction for 16 int16_t values using AVX2.
 * Two folds are sufficient for q=257 with coefficient ranges used in Lore.
 * Input:  16 packed int16_t in v
 * Output: 16 packed int16_t, reduced modulo q
 */
static inline __m256i barrett_reduce_16way(__m256i v)
{
    const __m256i mask = _mm256_set1_epi32(0xFF);

    /* Sign-extend low 8 values to 32-bit */
    __m128i v_lo = _mm256_castsi256_si128(v);
    __m256i lo = _mm256_cvtepi16_epi32(v_lo);

    /* Sign-extend high 8 values to 32-bit */
    __m128i v_hi = _mm256_extracti128_si256(v, 1);
    __m256i hi = _mm256_cvtepi16_epi32(v_hi);

    /* Fold 1: t = (t & 0xFF) - (t >> 8) */
    lo = _mm256_sub_epi32(_mm256_and_si256(lo, mask), _mm256_srai_epi32(lo, 8));
    hi = _mm256_sub_epi32(_mm256_and_si256(hi, mask), _mm256_srai_epi32(hi, 8));

    /* Fold 2 */
    lo = _mm256_sub_epi32(_mm256_and_si256(lo, mask), _mm256_srai_epi32(lo, 8));
    hi = _mm256_sub_epi32(_mm256_and_si256(hi, mask), _mm256_srai_epi32(hi, 8));

    /* Pack back to 16-bit using 128-bit packs to avoid lane interleave */
    {
        __m128i lo_lo = _mm256_castsi256_si128(lo);
        __m128i lo_hi = _mm256_extracti128_si256(lo, 1);
        __m128i hi_lo = _mm256_castsi256_si128(hi);
        __m128i hi_hi = _mm256_extracti128_si256(hi, 1);
        __m128i packed_lo = _mm_packs_epi32(lo_lo, lo_hi);
        __m128i packed_hi = _mm_packs_epi32(hi_lo, hi_hi);
        return _mm256_set_m128i(packed_hi, packed_lo);
    }
}

#endif /* LORE_AVX2_UTILS_H */
