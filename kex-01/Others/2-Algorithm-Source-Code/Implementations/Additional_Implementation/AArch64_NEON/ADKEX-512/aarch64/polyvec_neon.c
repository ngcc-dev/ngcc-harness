/*
 * AArch64 NEON polyvec_compressB / polyvec_decompressB.
 * Only compiled when DKE_USE_AARCH64 is defined.
 *
 * compressB uses NEON for the division-by-Q approximation,
 * with scalar bit-packing for the irregular output format.
 */
#include "../parameters.h"

#if defined(DKE_USE_AARCH64)

#include <arm_neon.h>
#include <stdint.h>
#include "../poly.h"
#include "../polyvec.h"
#include "neon_params.h"

/* ================================================================
 * polyvec_compressB: NEON-accelerated coefficient compression
 * ================================================================ */

#if DKE_MODE == 128 /* dB=10: 4 coeffs → 5 bytes */

void DKE_polyvec_compressB(uint8_t bytes[DKE_PBCOMPRESSEDBYTES], const polyvec *v) {
    unsigned int i, j, k;
    int16x8_t q_vec = vdupq_n_s16((int16_t)DKE_Q);
    /* Compress: round(x * 2^10 / Q) = ((x << 10) + 1665) * 1290167 >> 32.
     * (val * mul) >> 32 = (2 * val * mul) >> 33 = vqdmulh(val, mul) >> 1.
     * Use vqdmulhq_s32 which gives (2*a*b) >> 32, then shift right by 1. */
    const int32x4_t half_v = vdupq_n_s32(1665);
    const int32x4_t mul_v  = vdupq_n_s32(1290167);

    for (i = 0; i < DKE_K; i++) {
        for (j = 0; j < DKE_N / 8; j++) {
            int16x8_t f = vld1q_s16(&v->vec[i].coeffs[8*j]);
            uint16x8_t neg = vcltq_s16(f, vdupq_n_s16(0));
            f = vaddq_s16(f, vandq_s16(vreinterpretq_s16_u16(neg), q_vec));

            uint16x8_t u = vreinterpretq_u16_s16(f);
            int32x4_t lo = vaddq_s32(vreinterpretq_s32_u32(vshll_n_u16(vget_low_u16(u), 10)), half_v);
            int32x4_t hi = vaddq_s32(vreinterpretq_s32_u32(vshll_n_u16(vget_high_u16(u), 10)), half_v);

            /* vqdmulhq_s32 gives (2*val*mul) >> 32; we need >> 32, so shift right by 1 more */
            lo = vshrq_n_s32(vqdmulhq_s32(lo, mul_v), 1);
            hi = vshrq_n_s32(vqdmulhq_s32(hi, mul_v), 1);

            /* Narrow and mask to 10 bits */
            int16x8_t compressed = vcombine_s16(vmovn_s32(lo), vmovn_s32(hi));
            uint16_t c[8];
            vst1q_u16(c, vandq_u16(vreinterpretq_u16_s16(compressed), vdupq_n_u16(0x3FF)));

            for (k = 0; k < 2; k++) {
                uint16_t *cp = &c[4*k];
                bytes[0] = (uint8_t)(cp[0] >> 0);
                bytes[1] = (uint8_t)((cp[0] >> 8) | (cp[1] << 2));
                bytes[2] = (uint8_t)((cp[1] >> 6) | (cp[2] << 4));
                bytes[3] = (uint8_t)((cp[2] >> 4) | (cp[3] << 6));
                bytes[4] = (uint8_t)(cp[3] >> 2);
                bytes += 5;
            }
        }
    }
}

void DKE_polyvec_decompressB(polyvec *v, const uint8_t bytes[DKE_PBCOMPRESSEDBYTES]) {
    unsigned int i, j, k;
    /* dB=10: decompress(x) = (x * Q + 512) >> 10 */
    const uint32x4_t q_vec = vdupq_n_u32(DKE_Q);
    const uint32x4_t half  = vdupq_n_u32(512);

    for (i = 0; i < DKE_K; i++) {
        for (j = 0; j < DKE_N / 8; j++) {
            /* Unpack 2 groups of 4 × 10-bit from 2 × 5 bytes */
            uint16_t t[8];
            t[0] = (bytes[0] >> 0) | ((uint16_t)bytes[1] << 8);
            t[1] = (bytes[1] >> 2) | ((uint16_t)bytes[2] << 6);
            t[2] = (bytes[2] >> 4) | ((uint16_t)bytes[3] << 4);
            t[3] = (bytes[3] >> 6) | ((uint16_t)bytes[4] << 2);
            t[4] = (bytes[5] >> 0) | ((uint16_t)bytes[6] << 8);
            t[5] = (bytes[6] >> 2) | ((uint16_t)bytes[7] << 6);
            t[6] = (bytes[7] >> 4) | ((uint16_t)bytes[8] << 4);
            t[7] = (bytes[8] >> 6) | ((uint16_t)bytes[9] << 2);
            bytes += 10;

            /* Mask to 10 bits and load into NEON */
            uint16x8_t raw = vld1q_u16(t);
            raw = vandq_u16(raw, vdupq_n_u16(0x3FF));

            /* Decompress: (x * Q + 512) >> 10, using u32 for precision */
            uint32x4_t lo = vmull_u16(vget_low_u16(raw), vget_low_u16(vdupq_n_u16(DKE_Q)));
            uint32x4_t hi = vmull_u16(vget_high_u16(raw), vget_high_u16(vdupq_n_u16(DKE_Q)));
            lo = vshrq_n_u32(vaddq_u32(lo, half), 10);
            hi = vshrq_n_u32(vaddq_u32(hi, half), 10);

            /* Narrow back to int16 and store */
            int16x8_t result = vcombine_s16(vmovn_s32(vreinterpretq_s32_u32(lo)),
                                            vmovn_s32(vreinterpretq_s32_u32(hi)));
            vst1q_s16(&v->vec[i].coeffs[8*j], result);
        }
    }
}

#else /* DKE_MODE == 256 or 512: dB=11, 8 coeffs → 11 bytes */

void DKE_polyvec_compressB(uint8_t bytes[DKE_PBCOMPRESSEDBYTES], const polyvec *v) {
    unsigned int i, j;
    int16x8_t q_vec = vdupq_n_s16((int16_t)DKE_Q);
#if DKE_MODE == 256
    const int32x4_t half_v = vdupq_n_s32(1664);
    const int32x4_t mul_v  = vdupq_n_s32(645084);
#else
    const int32x4_t half_v = vdupq_n_s32(3840);
    const int32x4_t mul_v  = vdupq_n_s32(279584);
#endif

    for (i = 0; i < DKE_K; i++) {
        for (j = 0; j < DKE_N / 8; j++) {
            int16x8_t f = vld1q_s16(&v->vec[i].coeffs[8*j]);
            /* Conditional add Q if negative (NEON vectorized) */
            uint16x8_t neg = vcltq_s16(f, vdupq_n_s16(0));
            f = vaddq_s16(f, vandq_s16(vreinterpretq_s16_u16(neg), q_vec));

            /* Compress: round(x * 2^11 / Q) = ((x << 11) + half) * mul >> 31.
             * Use vqdmulhq_s32: returns saturate((2*a*b) >> 32) = (a*b) >> 31. */
            uint16x8_t u = vreinterpretq_u16_s16(f);
            int32x4_t lo = vaddq_s32(vreinterpretq_s32_u32(vshll_n_u16(vget_low_u16(u), 11)), half_v);
            int32x4_t hi = vaddq_s32(vreinterpretq_s32_u32(vshll_n_u16(vget_high_u16(u), 11)), half_v);

            /* vqdmulhq_s32: (2 * val * mul) >> 32 = (val * mul) >> 31 */
            lo = vqdmulhq_s32(lo, mul_v);
            hi = vqdmulhq_s32(hi, mul_v);

            /* Narrow to 16-bit and mask to 11 bits */
            int16x8_t compressed = vcombine_s16(vmovn_s32(lo), vmovn_s32(hi));
            uint16x8_t t_vec = vandq_u16(vreinterpretq_u16_s16(compressed), vdupq_n_u16(0x7FF));

            /* Scalar 11-bit byte packing (irregular layout) */
            uint16_t t[8];
            vst1q_u16(t, t_vec);

            bytes[ 0] = (uint8_t)(t[0] >>  0);
            bytes[ 1] = (uint8_t)((t[0] >>  8) | (t[1] << 3));
            bytes[ 2] = (uint8_t)((t[1] >>  5) | (t[2] << 6));
            bytes[ 3] = (uint8_t)(t[2] >>  2);
            bytes[ 4] = (uint8_t)((t[2] >> 10) | (t[3] << 1));
            bytes[ 5] = (uint8_t)((t[3] >>  7) | (t[4] << 4));
            bytes[ 6] = (uint8_t)((t[4] >>  4) | (t[5] << 7));
            bytes[ 7] = (uint8_t)(t[5] >>  1);
            bytes[ 8] = (uint8_t)((t[5] >>  9) | (t[6] << 2));
            bytes[ 9] = (uint8_t)((t[6] >>  6) | (t[7] << 5));
            bytes[10] = (uint8_t)(t[7] >>  3);
            bytes += 11;
        }
    }
}

void DKE_polyvec_decompressB(polyvec *v, const uint8_t bytes[DKE_PBCOMPRESSEDBYTES]) {
    unsigned int i, j;
    /* dB=11: decompress(x) = (x * Q + 1024) >> 11 */
    const uint32x4_t half = vdupq_n_u32(1024);
    const uint16x8_t mask = vdupq_n_u16(0x7FF);
    const uint16x4_t q_lo = vdup_n_u16(DKE_Q);

    for (i = 0; i < DKE_K; i++) {
        for (j = 0; j < DKE_N / 8; j++) {
            /* Unpack 8 × 11-bit from 11 bytes (scalar — irregular bit layout) */
            uint16_t t[8];
            t[0] = (bytes[0] >> 0) | ((uint16_t)bytes[ 1] << 8);
            t[1] = (bytes[1] >> 3) | ((uint16_t)bytes[ 2] << 5);
            t[2] = (bytes[2] >> 6) | ((uint16_t)bytes[ 3] << 2) | ((uint16_t)bytes[4] << 10);
            t[3] = (bytes[4] >> 1) | ((uint16_t)bytes[ 5] << 7);
            t[4] = (bytes[5] >> 4) | ((uint16_t)bytes[ 6] << 4);
            t[5] = (bytes[6] >> 7) | ((uint16_t)bytes[ 7] << 1) | ((uint16_t)bytes[8] << 9);
            t[6] = (bytes[8] >> 2) | ((uint16_t)bytes[ 9] << 6);
            t[7] = (bytes[9] >> 5) | ((uint16_t)bytes[10] << 3);
            bytes += 11;

            /* Mask to 11 bits */
            uint16x8_t raw = vandq_u16(vld1q_u16(t), mask);

            /* NEON decompress: (x * Q + 1024) >> 11 in u32 for precision */
            uint32x4_t lo = vmlal_u16(half, vget_low_u16(raw), q_lo);
            uint32x4_t hi = vmlal_u16(half, vget_high_u16(raw), q_lo);
            lo = vshrq_n_u32(lo, 11);
            hi = vshrq_n_u32(hi, 11);

            /* Narrow to int16 and store */
            int16x8_t result = vcombine_s16(vmovn_s32(vreinterpretq_s32_u32(lo)),
                                            vmovn_s32(vreinterpretq_s32_u32(hi)));
            vst1q_s16(&v->vec[i].coeffs[8*j], result);
        }
    }
}

#endif /* DKE_MODE */

#endif /* DKE_USE_AARCH64 */
