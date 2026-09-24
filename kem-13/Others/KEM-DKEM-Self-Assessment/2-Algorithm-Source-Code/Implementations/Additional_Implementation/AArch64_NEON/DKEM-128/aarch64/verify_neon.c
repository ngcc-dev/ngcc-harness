/*
 * AArch64 NEON constant-time verify / cmov for DKE.
 * Only compiled when DKE_USE_AARCH64 is defined.
 */
#include "../parameters.h"

#if defined(DKE_USE_AARCH64)

#include <arm_neon.h>
#include <stddef.h>
#include <stdint.h>
#include "../verify.h"

int DKE_verify(const uint8_t *a, const uint8_t *b, size_t len) {
    size_t i;
    uint8x16_t acc = vdupq_n_u8(0);

    for (i = 0; i + 16 <= len; i += 16) {
        uint8x16_t va = vld1q_u8(&a[i]);
        uint8x16_t vb = vld1q_u8(&b[i]);
        acc = vorrq_u8(acc, veorq_u8(va, vb));
    }

    /* Horizontal OR reduce */
    uint8_t r = vmaxvq_u8(acc);

    /* Scalar tail */
    for (; i < len; i++)
        r |= a[i] ^ b[i];

    return (r != 0);
}

void DKE_cmov(uint8_t *r, const uint8_t *x, size_t len, uint8_t b) {
    size_t i;
    uint8x16_t mask = vdupq_n_u8(-(int8_t)b);

    for (i = 0; i + 16 <= len; i += 16) {
        uint8x16_t vr = vld1q_u8(&r[i]);
        uint8x16_t vx = vld1q_u8(&x[i]);
        uint8x16_t diff = veorq_u8(vr, vx);
        diff = vandq_u8(mask, diff);
        vr = veorq_u8(vr, diff);
        vst1q_u8(&r[i], vr);
    }

    uint8_t bs = -b;
    for (; i < len; i++)
        r[i] ^= bs & (r[i] ^ x[i]);
}

void DKE_cmov_int16(int16_t *r, int16_t v, uint16_t b) {
    b = -b;
    *r ^= b & ((*r) ^ v);
}

#endif /* DKE_USE_AARCH64 */
