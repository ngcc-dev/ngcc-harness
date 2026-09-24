#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "poly.h"
#include "poly_mul.h"

#if defined(SCABBARD_USE_ASM_MUL) && SCABBARD_USE_ASM_MUL

static void unpack_signed_nibbles(uint16_t *b, const uint8_t *b_packed)
{
    size_t i;

    for (i = 0; i < SCABBARD_N / 2; i++) {
        b[2 * i] = (uint16_t)(((b_packed[i] & 0x0F) ^ 0x08) - 0x08);
        b[2 * i + 1] = (uint16_t)(((b_packed[i] >> 4) ^ 0x08) - 0x08);
    }
}

extern void polymul_asm(uint16_t *r, const uint16_t *a, const uint16_t *b);

void poly_mul_64_sch_acc(
    const uint16_t *a,
    const uint8_t *b_packed,
    uint16_t *acc,
    uint16_t mod_mask)
{
    size_t i;
    uint32_t mask = mod_mask;
    uint16_t b[SCABBARD_N] __attribute__((aligned(4)));
    uint16_t c[2 * SCABBARD_N] __attribute__((aligned(4)));

    unpack_signed_nibbles(b, b_packed);
    memset(c, 0, 2 * SCABBARD_N * sizeof(c[0]));
    polymul_asm(c, a, b);

    for (i = 0; i < SCABBARD_N; i++) {
        acc[i] = (uint16_t)((acc[i] + (uint32_t)((c[i] - c[i + SCABBARD_N]) & mod_mask)) & mask);
    }
}

#endif
