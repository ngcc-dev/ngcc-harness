#ifndef GF2X_H
#define GF2X_H

#ifdef AVX512_AVAILABLE
#include <immintrin.h>
#endif

#include "trike_types.h"

// Function prototypes for GF2X operations
void gf2x_add(uint8_t *c, const uint8_t *a, const uint8_t *b);
void gf2x_mul(uint8_t *c, const uint8_t *a, const uint8_t *b);
void gf2x_inv(uint8_t *inv_a, const uint8_t *a);
void gf2x_shift(uint8_t *out, const uint8_t *in, uint32_t shift);

// Sets the destination array to zero.
static inline void trike_setz(uint8_t *dst, size_t dst_size)
{
#ifdef AVX512_AVAILABLE
    __m512i zero = _mm512_setzero_si512();
    for (size_t i = 0; i < dst_size; i += 64)
    {
        _mm512_store_si512((__m512i *)(dst + i), zero);
    }
#else
    memset(dst, 0, dst_size);
#endif
}

// Copies the source array to the destination array and zeroes out the remaining bytes.
static inline void fast_cpy(uint64_t *dst, const uint64_t *src, size_t size64)
{
#ifdef AVX512_AVAILABLE
    for (size_t i = 0; i < size64; i += 8)
    {
        __m512i Data = _mm512_load_si512((__m512i*)(src + i));
        _mm512_store_si512((__m512i*)(dst + i), Data);
    }
#else
    memcpy(dst, src, size64 * 8);
#endif
}

// Assigns the source array to the destination array and zeroes out the remaining bytes.
static inline void trike_assign(const uint8_t *src, uint8_t *dst, size_t src_size, size_t dst_size)
{
#ifdef AVX512_AVAILABLE
    for (size_t i = 0; i < src_size; i += 64)
    {
        __m512i v = _mm512_load_si512((__m512i *)(src + i));
        _mm512_store_si512((__m512i *)(dst + i), v);
    }
    __m512i zero = _mm512_setzero_si512();
    for (size_t i = src_size; i < dst_size; i += 64)
    {
        _mm512_store_si512((__m512i *)(dst + i), zero);
    }
#else
    memcpy(dst, src, src_size);
    memset(dst + src_size, 0, dst_size - src_size);
#endif
}
#endif