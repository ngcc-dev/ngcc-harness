#include "radix16_r2.h"

#include <string.h>

extern void r2_radix16_mul_2_asm(uint32_t *res, const uint32_t *a, const uint32_t *b);
extern void r2_radix16_mul_4_asm(uint32_t *res, const uint32_t *a, const uint32_t *b);
extern void r2_radix16_mul_8_asm(uint32_t *res, const uint32_t *a, const uint32_t *b);
extern void r2_radix16_mul_16_asm(uint32_t *res, const uint32_t *a, const uint32_t *b);
extern void r2_radix16_mul_32_asm(uint32_t *res, const uint32_t *a, const uint32_t *b);
extern void r2_radix16_mul_64_asm(uint32_t *res, const uint32_t *a, const uint32_t *b);
extern void r2_radix16_mul_128_asm(uint32_t *res, const uint32_t *a, const uint32_t *b);
extern void r2_radix16_mul_256_asm(uint32_t *res, const uint32_t *a, const uint32_t *b);
extern void r2_radix16_mul_512_asm(uint32_t *res, const uint32_t *a, const uint32_t *b);
extern void r2_radix16_mul_1024_asm(uint32_t *res, const uint32_t *a, const uint32_t *b);
extern void r2_radix16_mul_1024x512_asm(uint32_t *res, const uint32_t *a, const uint32_t *b);

static uint32_t r2_radix16_get_coeff(const uint32_t *a, size_t idx)
{
    return (a[idx >> 3] >> (4u * (idx & 7u))) & 1u;
}

void r2_radix16_pack(uint32_t *out, const int16_t *in, size_t n)
{
    size_t words = R2_RADIX16_WORDS(n);
    size_t i;

    memset(out, 0, words * sizeof(uint32_t));
    for (i = 0; i < n; i++) {
        out[i >> 3] |= ((uint32_t)in[i] & 1u) << (4u * (i & 7u));
    }
}

void r2_radix16_unpack(int16_t *out, const uint32_t *in, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++) {
        out[i] = (int16_t)r2_radix16_get_coeff(in, i);
    }
}

void r2_radix16_frombytes(uint32_t *out, const uint8_t *in, size_t n)
{
    size_t words = R2_RADIX16_WORDS(n);
    size_t i;

    memset(out, 0, words * sizeof(uint32_t));
    for (i = 0; i < n; i++) {
        uint32_t bit = ((uint32_t)in[i >> 3] >> (i & 7u)) & 1u;

        out[i >> 3] |= bit << (4u * (i & 7u));
    }
}

void r2_radix16_tobytes(uint8_t *out, const uint32_t *in, size_t n)
{
    size_t bytes = (n + 7u) >> 3;
    size_t i, j;

    memset(out, 0, bytes);
    for (i = 0; i < bytes; i++) {
        uint32_t word = in[i] & R2_RADIX16_LANE_MASK;
        uint8_t byte = 0;

        for (j = 0; j < 8u && (8u * i + j) < n; j++) {
            byte |= (uint8_t)(((word >> (4u * j)) & 1u) << j);
        }
        out[i] = byte;
    }
}

static void r2_radix16_unsupported_length(void)
{
#if defined(__GNUC__)
    __builtin_trap();
#else
    for (;;) {
    }
#endif
}

void r2_radix16_mul_1024x512(uint32_t *res,
                            const uint32_t *a,
                            const uint32_t *b)
{
    r2_radix16_mul_1024x512_asm(res, a, b);
}

void r2_radix16_mul(uint32_t *res,
                    const uint32_t *a,
                    const uint32_t *b,
                    size_t n)
{
    switch (n) {
    case 0u:
        return;
    case 2u:
        r2_radix16_mul_2_asm(res, a, b);
        return;
    case 4u:
        r2_radix16_mul_4_asm(res, a, b);
        return;
    case 8u:
        r2_radix16_mul_8_asm(res, a, b);
        return;
    case 16u:
        r2_radix16_mul_16_asm(res, a, b);
        return;
    case 32u:
        r2_radix16_mul_32_asm(res, a, b);
        return;
    case 64u:
        r2_radix16_mul_64_asm(res, a, b);
        return;
    case 128u:
        r2_radix16_mul_128_asm(res, a, b);
        return;
    case 256u:
        r2_radix16_mul_256_asm(res, a, b);
        return;
    case 512u:
        r2_radix16_mul_512_asm(res, a, b);
        return;
    case 1024u:
        r2_radix16_mul_1024_asm(res, a, b);
        return;
    default:
        r2_radix16_unsupported_length();
        return;
    }
}
