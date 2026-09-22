#ifndef GF2X_H
#define GF2X_H

#include "trike_types.h"

// Function prototypes for GF2X operations
void gf2x_add(uint8_t *c, const uint8_t *a, const uint8_t *b);
void gf2x_mul(uint8_t *c, const uint8_t *a, const uint8_t *b);
void gf2x_inv(uint8_t *inv_a, const uint8_t *a);
void gf2x_shift(uint8_t *out, const uint8_t *in, uint32_t shift);

// Sets the destination array to zero.
static inline void trike_setz(uint8_t *dst, size_t dst_size)
{
    memset(dst, 0, dst_size);
}

// Copies the source array to the destination array and zeroes out the remaining bytes.
static inline void fast_cpy(uint64_t *dst, const uint64_t *src, size_t size64)
{
    memcpy(dst, src, size64 * 8);
}

// Assigns the source array to the destination array and zeroes out the remaining bytes.
static inline void trike_assign(const uint8_t *src, uint8_t *dst, size_t src_size, size_t dst_size)
{
    memcpy(dst, src, src_size);
    memset(dst + src_size, 0, dst_size - src_size);
}
#endif