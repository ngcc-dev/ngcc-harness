#include "reduce.h"
#include "params.h"
#include <stdint.h>

/*
 * montgomery_reduce and barrett_reduce implementations
 * are now static inline in reduce.h for cross-TU inlining.
 */

/* AVX2 fast path: 16-wide Barrett reduction for N=768 (L4).
 * For N=512 the per-call setup overhead exceeds the savings. */
#if LORE_N == 768 && (defined(LORE_USE_AVX2_SHAKE) || defined(LORE_USE_AVX2_POLYMUL_NTT))
#include <immintrin.h>

static inline __m256i barrett_reduce_16x(__m256i a)
{
    const __m256i mask = _mm256_set1_epi16(0x00FF);
    __m256i t = _mm256_sub_epi16(_mm256_and_si256(a, mask), _mm256_srai_epi16(a, 8));
    t = _mm256_sub_epi16(_mm256_and_si256(t, mask), _mm256_srai_epi16(t, 8));
    return t;
}

void poly_reduce(poly *r) {
    for (int i = 0; i < LORE_N; i += 16) {
        __m256i coeffs = _mm256_loadu_si256(
            (const __m256i *)(const void *)(r->coeffs + i));
        coeffs = barrett_reduce_16x(coeffs);
        _mm256_storeu_si256((__m256i *)(void *)(r->coeffs + i), coeffs);
    }
    _mm256_zeroupper();
}

#else /* scalar fallback for N=512 or when AVX2 is not enabled */

void poly_reduce(poly *r) {
    for (int i = 0; i < LORE_N; i += 4) {
        r->coeffs[i]   = barrett_reduce(r->coeffs[i]);
        r->coeffs[i+1] = barrett_reduce(r->coeffs[i+1]);
        r->coeffs[i+2] = barrett_reduce(r->coeffs[i+2]);
        r->coeffs[i+3] = barrett_reduce(r->coeffs[i+3]);
    }
}

#endif
