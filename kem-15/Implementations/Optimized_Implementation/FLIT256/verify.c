#include <stddef.h>
#include <stdint.h>
#include <immintrin.h>
#include "verify.h"

/*************************************************
* Name:        verify
*
* Description: Compare two arrays for equality in constant time.
*              AVX2-accelerated: 32 bytes per iteration.
*
* Arguments:   const uint8_t *a: pointer to first byte array
*              const uint8_t *b: pointer to second byte array
*              size_t len:       length of the byte arrays
*
* Returns 0 if the byte arrays are equal, 1 otherwise
**************************************************/
int verify(const uint8_t *a, const uint8_t *b, size_t len)
{
    __m256i acc = _mm256_setzero_si256();
    size_t i;

    for (i = 0; i + 32 <= len; i += 32) {
        __m256i va = _mm256_loadu_si256((const __m256i *)(a + i));
        __m256i vb = _mm256_loadu_si256((const __m256i *)(b + i));
        acc = _mm256_or_si256(acc, _mm256_xor_si256(va, vb));
    }

    /* Reduce 256-bit accumulator to 64-bit scalar */
    __m128i lo = _mm256_castsi256_si128(acc);
    __m128i hi = _mm256_extracti128_si256(acc, 1);
    uint64_t r = _mm_cvtsi128_si64(_mm_or_si128(lo, hi))
               | (uint64_t)_mm_extract_epi64(_mm_or_si128(lo, hi), 1);

    for (; i < len; i++)
        r |= a[i] ^ b[i];

    return (-(int64_t)r) >> 63;
}

/*************************************************
* Name:        cmov
*
* Description: Copy len bytes from x to r if b is 1;
*              don't modify x if b is 0. Requires b to be in {0,1};
*              assumes two's complement representation of negative integers.
*              Runs in constant time.
*
* Arguments:   uint8_t *r:       pointer to output byte array
*              const uint8_t *x: pointer to input byte array
*              size_t len:       Amount of bytes to be copied
*              uint8_t b:        Condition bit; has to be in {0,1}
**************************************************/
void cmov(uint8_t *r, const uint8_t *x, size_t len, uint8_t b)
{
    size_t i;
    __m256i mb = _mm256_set1_epi8((char)-b);

    for (i = 0; i + 32 <= len; i += 32) {
        __m256i vr = _mm256_loadu_si256((const __m256i *)(r + i));
        __m256i vx = _mm256_loadu_si256((const __m256i *)(x + i));
        vr = _mm256_xor_si256(vr, _mm256_and_si256(mb, _mm256_xor_si256(vr, vx)));
        _mm256_storeu_si256((__m256i *)(r + i), vr);
    }
    b = -b;
    for (; i < len; i++)
        r[i] ^= b & (r[i] ^ x[i]);
}