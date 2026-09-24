/*
 * AArch64 NEON poly_tobytes / poly_frombytes / getsignal / fromsignal.
 * Only compiled when DKE_USE_AARCH64 is defined.
 *
 * tobytes/frombytes: 12-bit (DKE-128/256) or 13-bit (DKE-512) packing.
 * getsignal/fromsignal: 4-bit (L=4) or 5-bit (L=5) compress/decompress.
 *
 * NEON is used for the coefficient normalization (conditional add Q),
 * with scalar bit-packing (irregular bit widths don't vectorize well).
 */
#include "../parameters.h"

#if defined(DKE_USE_AARCH64)

#include <arm_neon.h>
#include <stdint.h>
#include "../poly.h"

/* ================================================================
 * poly_tobytes / poly_frombytes
 * ================================================================ */

#if DKE_MODE == 512
/* 13-bit: 8 coeffs → 13 bytes */

void DKE_poly_tobytes(uint8_t bytes[DKE_POLYBYTES], const poly *pol) {
    unsigned int i;
    int16x8_t q_vec = vdupq_n_s16((int16_t)DKE_Q);

    for (i = 0; i < DKE_N / 8; i++) {
        int16x8_t v = vld1q_s16(&pol->coeffs[8*i]);
        /* Conditional add Q if negative */
        uint16x8_t neg = vcltq_s16(v, vdupq_n_s16(0));
        v = vaddq_s16(v, vandq_s16(vreinterpretq_s16_u16(neg), q_vec));

        int16_t t[8];
        vst1q_s16(t, v);

        bytes[13*i+ 0] = (uint8_t)(t[0]);
        bytes[13*i+ 1] = (uint8_t)((t[0] >> 8) | (t[1] << 5));
        bytes[13*i+ 2] = (uint8_t)(t[1] >> 3);
        bytes[13*i+ 3] = (uint8_t)((t[1] >> 11) | (t[2] << 2));
        bytes[13*i+ 4] = (uint8_t)((t[2] >> 6) | (t[3] << 7));
        bytes[13*i+ 5] = (uint8_t)(t[3] >> 1);
        bytes[13*i+ 6] = (uint8_t)((t[3] >> 9) | (t[4] << 4));
        bytes[13*i+ 7] = (uint8_t)(t[4] >> 4);
        bytes[13*i+ 8] = (uint8_t)((t[4] >> 12) | (t[5] << 1));
        bytes[13*i+ 9] = (uint8_t)((t[5] >> 7) | (t[6] << 6));
        bytes[13*i+10] = (uint8_t)(t[6] >> 2);
        bytes[13*i+11] = (uint8_t)((t[6] >> 10) | (t[7] << 3));
        bytes[13*i+12] = (uint8_t)(t[7] >> 5);
    }
}

void DKE_poly_frombytes(poly *pol, const uint8_t bytes[DKE_POLYBYTES]) {
    unsigned int i;
    for (i = 0; i < DKE_N / 8; i++) {
        pol->coeffs[8*i]   = ((bytes[13*i+0] >> 0) | ((uint16_t)bytes[13*i+1] << 8)) & 0x1FFF;
        pol->coeffs[8*i+1] = ((bytes[13*i+1] >> 5) | ((uint16_t)bytes[13*i+2] << 3) |
                              ((uint16_t)bytes[13*i+3] << 11)) & 0x1FFF;
        pol->coeffs[8*i+2] = ((bytes[13*i+3] >> 2) | ((uint16_t)bytes[13*i+4] << 6)) & 0x1FFF;
        pol->coeffs[8*i+3] = ((bytes[13*i+4] >> 7) | ((uint16_t)bytes[13*i+5] << 1) |
                              ((uint16_t)bytes[13*i+6] << 9)) & 0x1FFF;
        pol->coeffs[8*i+4] = ((bytes[13*i+6] >> 4) | ((uint16_t)bytes[13*i+7] << 4) |
                              ((uint16_t)bytes[13*i+8] << 12)) & 0x1FFF;
        pol->coeffs[8*i+5] = ((bytes[13*i+8] >> 1) | ((uint16_t)bytes[13*i+9] << 7)) & 0x1FFF;
        pol->coeffs[8*i+6] = ((bytes[13*i+9] >> 6) | ((uint16_t)bytes[13*i+10] << 2) |
                              ((uint16_t)bytes[13*i+11] << 10)) & 0x1FFF;
        pol->coeffs[8*i+7] = ((bytes[13*i+11] >> 3) | ((uint16_t)bytes[13*i+12] << 5)) & 0x1FFF;
    }
}

#else /* DKE_MODE == 128 or 256: 12-bit, 2 coeffs → 3 bytes */

#if defined(DKE_USE_AARCH64_NATIVE) && DKE_N == 256 && DKE_Q == 3329
#include "../aarch64-native/dke_native.h"
#endif

void DKE_poly_tobytes(uint8_t bytes[DKE_POLYBYTES], const poly *pol) {
#if 0 /* native tobytes disabled — testing NTT+basemul only */
    dke_poly_tobytes_native(bytes, pol->coeffs);
    return;
#endif
    unsigned int i;
    int16x8_t q_vec = vdupq_n_s16((int16_t)DKE_Q);

    for (i = 0; i < DKE_N / 8; i++) {
        int16x8_t v = vld1q_s16(&pol->coeffs[8*i]);
        uint16x8_t neg = vcltq_s16(v, vdupq_n_s16(0));
        v = vaddq_s16(v, vandq_s16(vreinterpretq_s16_u16(neg), q_vec));

        int16_t t[8];
        vst1q_s16(t, v);

        unsigned int j;
        for (j = 0; j < 4; j++) {
            uint16_t t0 = (uint16_t)t[2*j];
            uint16_t t1 = (uint16_t)t[2*j+1];
            bytes[3*(4*i+j)+0] = (uint8_t)(t0 >> 0);
            bytes[3*(4*i+j)+1] = (uint8_t)((t0 >> 8) | (t1 << 4));
            bytes[3*(4*i+j)+2] = (uint8_t)(t1 >> 4);
        }
    }
}

void DKE_poly_frombytes(poly *pol, const uint8_t bytes[DKE_POLYBYTES]) {
    unsigned int i;
    for (i = 0; i < DKE_N / 2; i++) {
        pol->coeffs[2*i]   = ((bytes[3*i+0] >> 0) | ((uint16_t)bytes[3*i+1] << 8)) & 0xFFF;
        pol->coeffs[2*i+1] = ((bytes[3*i+1] >> 4) | ((uint16_t)bytes[3*i+2] << 4)) & 0xFFF;
    }
}

#endif /* DKE_MODE */

/* ================================================================
 * getsignal / fromsignal
 * ================================================================ */

#if DKE_MODE == 128 /* L=4, Q=3329 */

#if defined(DKE_NEON_OPT)
/* Vectorized getsignal: compress coefficients to L-bit and pack. */
void DKE_getsignal(uint8_t bytes[DKE_SIGNALBYTES], const poly *a) {
    unsigned int i;
    int16x8_t q_vec = vdupq_n_s16((int16_t)DKE_Q);

    for (i = 0; i < DKE_N / 8; i++) {
        int16x8_t v = vld1q_s16(&a->coeffs[8*i]);
        uint16x8_t neg = vcltq_s16(v, vdupq_n_s16(0));
        v = vaddq_s16(v, vandq_s16(vreinterpretq_s16_u16(neg), q_vec));

        /* d = round(u * 16 / Q) = ((u << 4) + 1665) * 80635 >> 28 */
        uint32x4_t lo = vshll_n_u16(vreinterpret_u16_s16(vget_low_s16(v)), 4);
        uint32x4_t hi = vshll_n_u16(vreinterpret_u16_s16(vget_high_s16(v)), 4);
        lo = vaddq_u32(lo, vdupq_n_u32(1665));
        hi = vaddq_u32(hi, vdupq_n_u32(1665));
        lo = vmulq_n_u32(lo, 80635);
        hi = vmulq_n_u32(hi, 80635);
        lo = vshrq_n_u32(lo, 28);
        hi = vshrq_n_u32(hi, 28);
        uint16x4_t lo16 = vmovn_u32(lo);
        uint16x4_t hi16 = vmovn_u32(hi);
        uint16x8_t all = vandq_u16(vcombine_u16(lo16, hi16), vdupq_n_u16(0xf));
        uint16_t t[8];
        vst1q_u16(t, all);
        bytes[0] = (uint8_t)(t[0] | (t[1] << 4));
        bytes[1] = (uint8_t)(t[2] | (t[3] << 4));
        bytes[2] = (uint8_t)(t[4] | (t[5] << 4));
        bytes[3] = (uint8_t)(t[6] | (t[7] << 4));
        bytes += 4;
    }
}

/* Vectorized fromsignal: decompress nibbles to coefficients. */
void DKE_poly_fromsignal(poly *r, const uint8_t a[DKE_SIGNALBYTES]) {
    unsigned int i;
    for (i = 0; i < DKE_N / 8; i++) {
        uint16_t t[8];
        t[0] = a[0] & 15; t[1] = a[0] >> 4;
        t[2] = a[1] & 15; t[3] = a[1] >> 4;
        t[4] = a[2] & 15; t[5] = a[2] >> 4;
        t[6] = a[3] & 15; t[7] = a[3] >> 4;
        a += 4;
        uint16x8_t v = vmulq_u16(vld1q_u16(t), vdupq_n_u16((uint16_t)DKE_Q));
        vst1q_s16(&r->coeffs[8*i], vreinterpretq_s16_u16(vshrq_n_u16(v, 4)));
    }
}
#else
void DKE_getsignal(uint8_t bytes[DKE_SIGNALBYTES], const poly *a) {
    unsigned int i, j;
    int16_t u;
    uint32_t d0;
    uint8_t t[8];
    int16x8_t q_vec = vdupq_n_s16((int16_t)DKE_Q);

    for (i = 0; i < DKE_N / 8; i++) {
        int16x8_t v = vld1q_s16(&a->coeffs[8*i]);
        uint16x8_t neg = vcltq_s16(v, vdupq_n_s16(0));
        v = vaddq_s16(v, vandq_s16(vreinterpretq_s16_u16(neg), q_vec));

        int16_t buf[8];
        vst1q_s16(buf, v);

        for (j = 0; j < 8; j++) {
            u = buf[j];
            d0 = (uint32_t)u << 4;
            d0 += 1665;
            d0 *= 80635;
            d0 >>= 28;
            t[j] = d0 & 0xf;
        }
        bytes[0] = t[0] | (t[1] << 4);
        bytes[1] = t[2] | (t[3] << 4);
        bytes[2] = t[4] | (t[5] << 4);
        bytes[3] = t[6] | (t[7] << 4);
        bytes += 4;
    }
}

void DKE_poly_fromsignal(poly *r, const uint8_t a[DKE_SIGNALBYTES]) {
    unsigned int i;
    for (i = 0; i < DKE_N / 2; i++) {
        r->coeffs[2*i+0] = ((uint16_t)(a[0] & 15) * DKE_Q) >> 4;
        r->coeffs[2*i+1] = ((uint16_t)(a[0] >> 4) * DKE_Q) >> 4;
        a += 1;
    }
}
#endif

#elif DKE_MODE == 256 /* L=5, Q=3329 */

void DKE_getsignal(uint8_t r[DKE_SIGNALBYTES], const poly *a) {
    unsigned int i, j;
    int16_t u;
    uint32_t d0;
    uint8_t t[8];
    int16x8_t q_vec = vdupq_n_s16((int16_t)DKE_Q);

    for (i = 0; i < DKE_N / 8; i++) {
        int16x8_t v = vld1q_s16(&a->coeffs[8*i]);
        uint16x8_t neg = vcltq_s16(v, vdupq_n_s16(0));
        v = vaddq_s16(v, vandq_s16(vreinterpretq_s16_u16(neg), q_vec));

        int16_t buf[8];
        vst1q_s16(buf, v);

        for (j = 0; j < 8; j++) {
            u = buf[j];
            d0 = (uint32_t)u << 5;
            d0 += 1664;
            d0 *= 40318;
            d0 >>= 27;
            t[j] = d0 & 0x1f;
        }
        r[0] = (t[0] >> 0) | (t[1] << 5);
        r[1] = (t[1] >> 3) | (t[2] << 2) | (t[3] << 7);
        r[2] = (t[3] >> 1) | (t[4] << 4);
        r[3] = (t[4] >> 4) | (t[5] << 1) | (t[6] << 6);
        r[4] = (t[6] >> 2) | (t[7] << 3);
        r += 5;
    }
}

void DKE_poly_fromsignal(poly *r, const uint8_t a[DKE_SIGNALBYTES]) {
    unsigned int i, j;
    uint8_t t[8];
    for (i = 0; i < DKE_N / 8; i++) {
        t[0] = (a[0] >> 0);
        t[1] = (a[0] >> 5) | (a[1] << 3);
        t[2] = (a[1] >> 2);
        t[3] = (a[1] >> 7) | (a[2] << 1);
        t[4] = (a[2] >> 4) | (a[3] << 4);
        t[5] = (a[3] >> 1);
        t[6] = (a[3] >> 6) | (a[4] << 2);
        t[7] = (a[4] >> 3);
        a += 5;
        for (j = 0; j < 8; j++)
            r->coeffs[8*i+j] = ((uint32_t)(t[j] & 31) * DKE_Q) >> 5;
    }
}

#elif DKE_MODE == 512 /* L=4, Q=7681 */

void DKE_getsignal(uint8_t bytes[DKE_SIGNALBYTES], const poly *pol) {
    unsigned int i, j;
    int16_t u;
    uint32_t d0;
    uint8_t t[8];
    int16x8_t q_vec = vdupq_n_s16((int16_t)DKE_Q);

    for (i = 0; i < DKE_N / 8; i++) {
        int16x8_t v = vld1q_s16(&pol->coeffs[8*i]);
        uint16x8_t neg = vcltq_s16(v, vdupq_n_s16(0));
        v = vaddq_s16(v, vandq_s16(vreinterpretq_s16_u16(neg), q_vec));

        int16_t buf[8];
        vst1q_s16(buf, v);

        for (j = 0; j < 8; j++) {
            u = buf[j];
            d0 = (uint32_t)u << 4;
            d0 += 3840;
            d0 *= 34948;
            d0 >>= 28;
            t[j] = d0 & 0xf;
        }
        bytes[0] = t[0] | (t[1] << 4);
        bytes[1] = t[2] | (t[3] << 4);
        bytes[2] = t[4] | (t[5] << 4);
        bytes[3] = t[6] | (t[7] << 4);
        bytes += 4;
    }
}

void DKE_poly_fromsignal(poly *pol, const uint8_t sig[DKE_SIGNALBYTES]) {
    unsigned int i;
    for (i = 0; i < DKE_N / 2; i++) {
        pol->coeffs[2*i+0] = ((uint16_t)(sig[0] & 15) * DKE_Q) >> 4;
        pol->coeffs[2*i+1] = ((uint16_t)(sig[0] >> 4) * DKE_Q) >> 4;
        sig += 1;
    }
}

#endif /* DKE_MODE */

#endif /* DKE_USE_AARCH64 */
