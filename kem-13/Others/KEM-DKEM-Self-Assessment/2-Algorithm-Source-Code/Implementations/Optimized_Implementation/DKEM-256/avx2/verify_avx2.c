/*
 * AVX2 vectorized verify / cmov for DKE-128/256.
 * Only compiled when DKE_USE_AVX2 is defined and DKE_MODE != 512.
 */
#include "../parameters.h"

#if defined(DKE_USE_AVX2)

#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#include "../verify.h"

int DKE_verify(const uint8_t *a, const uint8_t *b, size_t len) {
    size_t i;
    __m256i acc = _mm256_setzero_si256();

    /* Process 32 bytes at a time */
    for (i = 0; i + 32 <= len; i += 32) {
        __m256i va = _mm256_loadu_si256((__m256i *)&a[i]);
        __m256i vb = _mm256_loadu_si256((__m256i *)&b[i]);
        acc = _mm256_or_si256(acc, _mm256_xor_si256(va, vb));
    }

    /* Horizontal OR: reduce 256 bits to a single flag */
    uint8_t r = 0;
    if (!_mm256_testz_si256(acc, acc))
        r = 1;

    /* Handle tail bytes */
    for (; i < len; i++)
        r |= a[i] ^ b[i];

    return (r != 0);
}

void DKE_cmov(uint8_t *r, const uint8_t *x, size_t len, uint8_t b) {
    size_t i;
    __m256i mask = _mm256_set1_epi8(-(int8_t)b);

    for (i = 0; i + 32 <= len; i += 32) {
        __m256i vr = _mm256_loadu_si256((__m256i *)&r[i]);
        __m256i vx = _mm256_loadu_si256((__m256i *)&x[i]);
        /* r[i] ^= mask & (r[i] ^ x[i]) */
        __m256i diff = _mm256_xor_si256(vr, vx);
        diff = _mm256_and_si256(mask, diff);
        vr = _mm256_xor_si256(vr, diff);
        _mm256_storeu_si256((__m256i *)&r[i], vr);
    }

    /* Tail */
    uint8_t bs = -b;
    for (; i < len; i++)
        r[i] ^= bs & (r[i] ^ x[i]);
}

void DKE_cmov_int16(int16_t *r, int16_t v, uint16_t b) {
    b = -b;
    *r ^= b & ((*r) ^ v);
}

#endif /* DKE_USE_AVX2 && DKE_MODE != 512 */
