#ifndef RADIX16_R2_H
#define RADIX16_R2_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define R2_RADIX16_COEFFS_PER_WORD 8u
#define R2_RADIX16_WORDS(n) (((n) + R2_RADIX16_COEFFS_PER_WORD - 1u) / R2_RADIX16_COEFFS_PER_WORD)
#define R2_RADIX16_LANE_MASK 0x11111111u

void r2_radix16_pack(uint32_t *out, const int16_t *in, size_t n);
void r2_radix16_unpack(int16_t *out, const uint32_t *in, size_t n);
void r2_radix16_frombytes(uint32_t *out, const uint8_t *in, size_t n);
void r2_radix16_tobytes(uint8_t *out, const uint32_t *in, size_t n);

void r2_radix16_mul_256x128(uint32_t *res,
                            const uint32_t *a,
                            const uint32_t *b);

void r2_radix16_mul(uint32_t *res,
                    const uint32_t *a,
                    const uint32_t *b,
                    size_t n);

#ifdef __cplusplus
}
#endif

#endif
