#include <stdint.h>
#include <string.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"
#include "reduce_neon.h"
#include "symmetric.h"

// 定义多项式的模约化----------------------------------------------------------------------------------

/*************************************************
 * Name:        poly_freeze
 *
 * Description: For all coefficients of in/out polynomial compute standard
 *              representative r = a mod^+ Q
 *
 * Arguments:   - poly *a: pointer to input/output polynomial
 **************************************************/
void poly_freeze(poly *a) {
    unsigned int i;
    for (i = 0; i < N; i += 4) {
        int32x4_t v = vld1q_s32(&a->coeffs[i]);
        vst1q_s32(&a->coeffs[i], darts_neon_freeze(v));
    }
}

// 定义多项式的压缩与解压缩---------------------------------------------------------------------------------

/*************************************************
* Name:        poly_compress
*
* Description: Compression of a polynomial
*
* Arguments:   - poly *a_high: pointer to output polynomial
*              - poly *a:      pointer to input polynomial
**************************************************/
void poly_compress(poly *a_high, const poly *a) {
    unsigned int i;

    for (i = 0; i < N; i++){
        a_high->coeffs[i] = (((a->coeffs[i] << D) + Q/2) / Q) & ((1 << D) - 1);
    }
}

/*************************************************
* Name:        poly_decompress
*
* Description: Decompression of a polynomial
*
* Arguments:   - poly *a_mid:  pointer to output polynomial
*              - poly *a_high: pointer to input polynomial
**************************************************/
void poly_decompress(poly *a_mid, const poly *a_high) {
    unsigned int i;
    const int32x4_t v_q = vdupq_n_s32(Q);
    const int32x4_t v_round = vdupq_n_s32(1 << (D - 1));

    for (i = 0; i < N; i += 4) {
        int32x4_t v = vld1q_s32(&a_high->coeffs[i]);
        v = vaddq_s32(vmulq_s32(v, v_q), v_round);
        vst1q_s32(&a_mid->coeffs[i], vshrq_n_s32(v, D));
    }
}

/*************************************************
* Name:        poly_lowbits
*
* Description: Compute the low bits of a polynomial
*
* Arguments:   - poly *a_low:   pointer to output polynomial (low bits)
*              - const poly *a: pointer to input polynomial
**************************************************/
void poly_lowbits(poly *a_low, const poly *a) {
    unsigned int i;
    poly a_high;
    poly a_mid;

    poly_compress(&a_high, a);
    poly_decompress(&a_mid, &a_high);

    for (i = 0; i < N; i += 4) {
        int32x4_t va = vld1q_s32(&a->coeffs[i]);
        int32x4_t vmid = vld1q_s32(&a_mid.coeffs[i]);
        vst1q_s32(&a_low->coeffs[i],
                  darts_neon_freeze_centered(vsubq_s32(va, vmid)));
    }
}

/*************************************************
 * Name:        poly_compose
 * 
 * Description: Compose a polynomial from its low bits and high bits
 * 
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const poly *a_low: pointer to low bits polynomial
 *              - const poly *a_high: pointer to high bits polynomial
 **************************************************/
void poly_compose(poly *r, const poly *a_low, const poly *a_high) {
    unsigned int i;
    poly a_mid;

    poly_decompress(&a_mid, a_high);

    for (i = 0; i < N; i += 4) {
        int32x4_t vlow = vld1q_s32(&a_low->coeffs[i]);
        int32x4_t vmid = vld1q_s32(&a_mid.coeffs[i]);
        vst1q_s32(&r->coeffs[i], vaddq_s32(vlow, vmid));
    }
}

// 定义的多项式之间的运算-------------------------------------------------------------------------------

/*************************************************
 * Name:        poly_mul_halfq
 *
 * Description: Multiply polynomial by (Q+1)/2
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const poly *a: pointer to input polynomial
 **************************************************/
void poly_mul_halfq(poly *r, const poly *a) {
    unsigned int i;
    const int32x4_t v_half_q = vdupq_n_s32(HALF_Q);

    for (i = 0; i < N; i += 4) {
        int32x4_t v = vld1q_s32(&a->coeffs[i]);
        vst1q_s32(&r->coeffs[i], vmulq_s32(v, v_half_q));
    }
}

/*************************************************
 * Name:        poly_equal
 *
 * Description: Check whether two polynomials are equal
 *
 * Arguments:   - const poly *a: pointer to first input polynomial
 *              - const poly *b: pointer to second input polynomial
 *
 * Returns:     - 1 if equal, 0 otherwise
 **************************************************/
int poly_equal(const poly *a, const poly *b) {
    uint32x4_t diff = vdupq_n_u32(0);

    for (size_t i = 0; i < N; i += 4) {
        uint32x4_t va = vreinterpretq_u32_s32(vld1q_s32(&a->coeffs[i]));
        uint32x4_t vb = vreinterpretq_u32_s32(vld1q_s32(&b->coeffs[i]));
        diff = vorrq_u32(diff, veorq_u32(va, vb));
    }

    uint32_t d = vgetq_lane_u32(diff, 0) | vgetq_lane_u32(diff, 1) |
                 vgetq_lane_u32(diff, 2) | vgetq_lane_u32(diff, 3);
    return (int)(d == 0);
}

/*************************************************
 * Name:        poly_mul_1_b
 *
 * Description: Multiply polynomial by (1 - b), where b is a bit (0 or 1)
 *
 * Arguments:   - poly *r:       pointer to output polynomial
 *              - const poly *a: pointer to input polynomial
 *              - uint8_t b:     bit (0 or 1)
 **************************************************/
void poly_mul_1_b(poly *r, const poly *a, uint8_t b) {
    unsigned int i;
    const int32x4_t v_factor = vdupq_n_s32(1 - (int32_t)b);

    for (i = 0; i < N; i += 4) {
        int32x4_t v = vld1q_s32(&a->coeffs[i]);
        vst1q_s32(&r->coeffs[i], vmulq_s32(v, v_factor));
    }
}

/*************************************************
* Name:        poly_ntt
*
* Description: Computes negacyclic number-theoretic transform (NTT) of
*              a polynomial in place;
*              inputs assumed to be in normal order, output in bitreversed order
*
* Arguments:   - poly *r: pointer to in/output polynomial
**************************************************/
void poly_ntt(poly *r) {
	ntt(r->coeffs);
	poly_freeze(r);
}

/*************************************************
* Name:        poly_invntt_tomont
*
* Description: Computes inverse of negacyclic number-theoretic transform (NTT)
*              of a polynomial in place;
*              inputs assumed to be in bitreversed order, output in normal order
*
* Arguments:   - poly *r: pointer to in/output polynomial
**************************************************/
void poly_invntt_tomont(poly *r) {
	invntt_tomont(r->coeffs);
}   

/*************************************************
* Name:        poly_mul_mont
*
* Description: Multiply each coefficient with MONT
*
* Arguments:   - poly *r: pointer to in/output polynomial
**************************************************/
void poly_mul_mont(poly *r) {
    const int32x4_t v_montsq = vdupq_n_s32(MONTSQ);
    unsigned int i;

    for (i = 0; i < N; i += 4) {
        int32x4_t v = vld1q_s32(&r->coeffs[i]);
        vst1q_s32(&r->coeffs[i], darts_neon_fqmul(v, v_montsq));
    }
}

/*************************************************
* Name:        poly_add
*
* Description: Add two polynomials
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_add(poly *r, const poly *a, const poly *b) {
    unsigned int i;

    for (i = 0; i < N; i += 4) {
        int32x4_t va = vld1q_s32(&a->coeffs[i]);
        int32x4_t vb = vld1q_s32(&b->coeffs[i]);
        vst1q_s32(&r->coeffs[i], vaddq_s32(va, vb));
    }
}

/*************************************************
* Name:        poly_sub
*
* Description: Subtract two polynomials
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_sub(poly *r, const poly *a, const poly *b) {
    unsigned int i;

    for (i = 0; i < N; i += 4) {
        int32x4_t va = vld1q_s32(&a->coeffs[i]);
        int32x4_t vb = vld1q_s32(&b->coeffs[i]);
        vst1q_s32(&r->coeffs[i], vsubq_s32(va, vb));
    }
}

/*************************************************
* Name:        poly_caddq
*
* Description: Conditional addition of q to each coefficient
*
* Arguments: - poly *r:       pointer to output polynomial
**************************************************/
void poly_caddq(poly *r) {
    unsigned int i;

    for (i = 0; i < N; i += 4) {
        int32x4_t v = vld1q_s32(&r->coeffs[i]);
        vst1q_s32(&r->coeffs[i], darts_neon_caddq(v));
    }
}

static inline __attribute__((always_inline))
void poly_basemul4(int32_t r[4], const int32_t a[4],
                   const int32_t b[4], int32_t zeta) {
    int32_t sa0 = a[0] + a[2], sa1 = a[1] + a[3];
    int32_t sb0 = b[0] + b[2], sb1 = b[1] + b[3];
    int32_t lo0, lo1, lo2;
    int32_t hi0, hi1, hi2;
    int32_t mid0, mid1, mid2;

    lo0  = darts_scalar_fqmul(a[0], b[0]);
    lo2  = darts_scalar_fqmul(a[1], b[1]);
    lo1  = darts_scalar_fqmul(a[0] + a[1], b[0] + b[1]) - lo0 - lo2;

    hi0  = darts_scalar_fqmul(a[2], b[2]);
    hi2  = darts_scalar_fqmul(a[3], b[3]);
    hi1  = darts_scalar_fqmul(a[2] + a[3], b[2] + b[3]) - hi0 - hi2;

    mid0 = darts_scalar_fqmul(sa0, sb0);
    mid2 = darts_scalar_fqmul(sa1, sb1);
    mid1 = darts_scalar_fqmul(sa0 + sa1, sb0 + sb1) - mid0 - mid2;

    r[0] = darts_scalar_freeze(lo0 + darts_scalar_fqmul(zeta, hi0 + mid2 - lo2 - hi2));
    r[1] = darts_scalar_freeze(lo1 + darts_scalar_fqmul(zeta, hi1));
    r[2] = darts_scalar_freeze(lo2 + darts_scalar_fqmul(zeta, hi2) + mid0 - lo0 - hi0);
    r[3] = darts_scalar_freeze(mid1 - lo1 - hi1);
}

static inline __attribute__((always_inline))
int poly_baseinv4(int32_t b[4], const int32_t a[4], int32_t zeta) {
    int r1;
    int32_t t0, t1, t2;
    uint32_t x;

    t0 = darts_scalar_fqmul(a[2], a[2]) - darts_scalar_fqmul(2 * a[1], a[3]);
    t0 = darts_scalar_fqmul(a[0], a[0]) + darts_scalar_fqmul(t0, zeta);
    t1 = darts_scalar_fqmul(a[3], a[3]);
    t1 = darts_scalar_fqmul(2 * a[0], a[2]) - darts_scalar_fqmul(a[1], a[1]) -
         darts_scalar_fqmul(t1, zeta);

    t2 = darts_scalar_fqmul(t1, t1);
    t2 = darts_scalar_fqmul(t0, t0) - darts_scalar_fqmul(t2, zeta);
    t2 = darts_scalar_fqinv(t2);

    x = (uint32_t)t2;
    r1 = (-(uint64_t)x) >> 63;

    t0 = montgomery_reduce(darts_scalar_fqmul(t0, t2));
    t1 = montgomery_reduce(darts_scalar_fqmul(t1, t2));
    t0 = montgomery_reduce(t0);
    t1 = montgomery_reduce(t1);

    t2 = darts_scalar_fqmul(t1, zeta);

    b[0] =  darts_scalar_fqmul(a[0], t0) - darts_scalar_fqmul(a[2], t2);
    b[1] = -darts_scalar_fqmul(a[1], t0) + darts_scalar_fqmul(a[3], t2);
    b[2] =  darts_scalar_fqmul(a[2], t0) - darts_scalar_fqmul(a[0], t1);
    b[3] = -darts_scalar_fqmul(a[3], t0) + darts_scalar_fqmul(a[1], t1);

    return r1 - 1;
}

/*************************************************
* Name:        poly_basemul_montgomery
*
* Description: Multiplication of two polynomials in NTT domain
*
* Arguments:   - poly *r:       pointer to output polynomial
*              - const poly *a: pointer to first input polynomial
*              - const poly *b: pointer to second input polynomial
*************************************************/
void poly_basemul_montgomery(poly *r, const poly *a, const poly *b) {
    unsigned int i;
#if DARTS_MODE == 128 || DARTS_MODE == 256
    for (i = 0; i < N/8; i++) {
        poly_basemul4(&r->coeffs[8*i], &a->coeffs[8*i], &b->coeffs[8*i], zetas[64+i]);
        poly_basemul4(&r->coeffs[8*i+4], &a->coeffs[8*i+4], &b->coeffs[8*i+4], -zetas[64+i]);
    }

#elif DARTS_MODE == 512
    for (i = 0; i < N/8; i++) {
        poly_basemul4(&r->coeffs[8*i], &a->coeffs[8*i], &b->coeffs[8*i], zetas[128+i]);
        poly_basemul4(&r->coeffs[8*i+4], &a->coeffs[8*i+4], &b->coeffs[8*i+4], -zetas[128+i]);
    }

#endif
}

/*************************************************
* Name:        poly_baseinv
*
* Description: Inversion of polynomial in Zq[X]/(X^8-zeta)
*              used for inversion of element in Rq in NTT domain
*  
* Arguments:   - poly *b: pointer to the output polynomial
*              - const poly *a: pointer to the input polynomial
***************************************************/
int poly_baseinv(poly *b, const poly *a) {
    int result = 0;
    unsigned int i;

#if DARTS_MODE == 128 || DARTS_MODE == 256
    for(i = 0; i < N/8; i++){
        result += poly_baseinv4(&b->coeffs[8*i], &a->coeffs[8*i], zetas[64+i]);
        result += poly_baseinv4(&b->coeffs[8*i + 4], &a->coeffs[8*i + 4], -zetas[64+i]);
    }

#elif DARTS_MODE == 512
    for(i = 0; i < N/8; i++){
        result += poly_baseinv4(&b->coeffs[8*i], &a->coeffs[8*i], zetas[128+i]);
        result += poly_baseinv4(&b->coeffs[8*i + 4], &a->coeffs[8*i + 4], -zetas[128+i]);
    }

#endif

    return result;
}

/*************************************************
 * Name:        poly_neg
 *
 * Description: Negate a polynomial
 *
 * Arguments:   - poly *r: pointer to output polynomial
 **************************************************/
void poly_neg(poly *r) {
    unsigned int i;

    for (i = 0; i < N; i += 4) {
        int32x4_t v = vld1q_s32(&r->coeffs[i]);
        vst1q_s32(&r->coeffs[i], vnegq_s32(v));
    }
}


// 定义多项式的封装------------------------------------------------------------------------------------

/*************************************************
 * Name:        poly_q_pack
 *
 * Description: Pack polynomial with coefficients in [0, Q - 1].
 *
 * Arguments:   - uint8_t *r: pointer to output byte array with at least
 *                            POLY_Q_PACKEDBYTES bytes
 *              - const poly *a: pointer to input polynomial
 **************************************************/
void poly_q_pack(uint8_t *r, const poly *a) {
    unsigned int out = 0;

#if DARTS_MODE == 128 || DARTS_MODE == 256
    // q = 130817, 17 bits
    for (unsigned int i = 0; i < N; i += 8) {
        int32x4_t f0 = darts_neon_freeze(vld1q_s32(&a->coeffs[i]));
        int32x4_t f1 = darts_neon_freeze(vld1q_s32(&a->coeffs[i + 4]));
        uint32_t t0 = (uint32_t)vgetq_lane_s32(f0, 0) & 0x1FFFFu;
        uint32_t t1 = (uint32_t)vgetq_lane_s32(f0, 1) & 0x1FFFFu;
        uint32_t t2 = (uint32_t)vgetq_lane_s32(f0, 2) & 0x1FFFFu;
        uint32_t t3 = (uint32_t)vgetq_lane_s32(f0, 3) & 0x1FFFFu;
        uint32_t t4 = (uint32_t)vgetq_lane_s32(f1, 0) & 0x1FFFFu;
        uint32_t t5 = (uint32_t)vgetq_lane_s32(f1, 1) & 0x1FFFFu;
        uint32_t t6 = (uint32_t)vgetq_lane_s32(f1, 2) & 0x1FFFFu;
        uint32_t t7 = (uint32_t)vgetq_lane_s32(f1, 3) & 0x1FFFFu;

        r[out +  0] = (uint8_t)(t0);
        r[out +  1] = (uint8_t)(t0 >> 8);
        r[out +  2] = (uint8_t)((t0 >> 16) | (t1 << 1));
        r[out +  3] = (uint8_t)(t1 >> 7);
        r[out +  4] = (uint8_t)((t1 >> 15) | (t2 << 2));
        r[out +  5] = (uint8_t)(t2 >> 6);
        r[out +  6] = (uint8_t)((t2 >> 14) | (t3 << 3));
        r[out +  7] = (uint8_t)(t3 >> 5);
        r[out +  8] = (uint8_t)((t3 >> 13) | (t4 << 4));
        r[out +  9] = (uint8_t)(t4 >> 4);
        r[out + 10] = (uint8_t)((t4 >> 12) | (t5 << 5));
        r[out + 11] = (uint8_t)(t5 >> 3);
        r[out + 12] = (uint8_t)((t5 >> 11) | (t6 << 6));
        r[out + 13] = (uint8_t)(t6 >> 2);
        r[out + 14] = (uint8_t)((t6 >> 10) | (t7 << 7));
        r[out + 15] = (uint8_t)(t7 >> 1);
        r[out + 16] = (uint8_t)(t7 >> 9);

        out += 17;
    }

#elif DARTS_MODE == 512
    // q = 260609, 18 bits
    for (unsigned int i = 0; i < N; i += 4) {
        int32x4_t f = darts_neon_freeze(vld1q_s32(&a->coeffs[i]));
        uint32_t t0 = (uint32_t)vgetq_lane_s32(f, 0) & 0x3FFFFu;
        uint32_t t1 = (uint32_t)vgetq_lane_s32(f, 1) & 0x3FFFFu;
        uint32_t t2 = (uint32_t)vgetq_lane_s32(f, 2) & 0x3FFFFu;
        uint32_t t3 = (uint32_t)vgetq_lane_s32(f, 3) & 0x3FFFFu;

        r[out + 0] = (uint8_t)(t0);
        r[out + 1] = (uint8_t)(t0 >> 8);
        r[out + 2] = (uint8_t)((t0 >> 16) | (t1 << 2));
        r[out + 3] = (uint8_t)(t1 >> 6);
        r[out + 4] = (uint8_t)((t1 >> 14) | (t2 << 4));
        r[out + 5] = (uint8_t)(t2 >> 4);
        r[out + 6] = (uint8_t)((t2 >> 12) | (t3 << 6));
        r[out + 7] = (uint8_t)(t3 >> 2);
        r[out + 8] = (uint8_t)(t3 >> 10);

        out += 9;
    }
#endif
}

/*************************************************
 * Name:        poly_q_unpack
 *
 * Description: Unpack polynomial with coefficients in [0, Q - 1].
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const uint8_t *a: pointer to input byte array with 
 *                                  at least POLY_Q_PACKEDBYTES bytes
 **************************************************/
void poly_q_unpack(poly *r, const uint8_t *a) {
    unsigned int in = 0;

#if DARTS_MODE == 128 || DARTS_MODE == 256
    // q = 130817, 17 bits
    for (unsigned int i = 0; i < N; i += 8) {
        uint32_t b0  = a[in +  0];
        uint32_t b1  = a[in +  1];
        uint32_t b2  = a[in +  2];
        uint32_t b3  = a[in +  3];
        uint32_t b4  = a[in +  4];
        uint32_t b5  = a[in +  5];
        uint32_t b6  = a[in +  6];
        uint32_t b7  = a[in +  7];
        uint32_t b8  = a[in +  8];
        uint32_t b9  = a[in +  9];
        uint32_t b10 = a[in + 10];
        uint32_t b11 = a[in + 11];
        uint32_t b12 = a[in + 12];
        uint32_t b13 = a[in + 13];
        uint32_t b14 = a[in + 14];
        uint32_t b15 = a[in + 15];
        uint32_t b16 = a[in + 16];

        uint32_t t0 = b0 | (b1 << 8) | ((b2 & 0x01u) << 16);
        uint32_t t1 = (b2 >> 1) | (b3 << 7) | ((b4 & 0x03u) << 15);
        uint32_t t2 = (b4 >> 2) | (b5 << 6) | ((b6 & 0x07u) << 14);
        uint32_t t3 = (b6 >> 3) | (b7 << 5) | ((b8 & 0x0Fu) << 13);
        uint32_t t4 = (b8 >> 4) | (b9 << 4) | ((b10 & 0x1Fu) << 12);
        uint32_t t5 = (b10 >> 5) | (b11 << 3) | ((b12 & 0x3Fu) << 11);
        uint32_t t6 = (b12 >> 6) | (b13 << 2) | ((b14 & 0x7Fu) << 10);
        uint32_t t7 = (b14 >> 7) | (b15 << 1) | (b16 << 9);

        int32x4_t v0 = vdupq_n_s32((int32_t)(t0 & 0x1FFFFu));
        v0 = vsetq_lane_s32((int32_t)(t1 & 0x1FFFFu), v0, 1);
        v0 = vsetq_lane_s32((int32_t)(t2 & 0x1FFFFu), v0, 2);
        v0 = vsetq_lane_s32((int32_t)(t3 & 0x1FFFFu), v0, 3);
        int32x4_t v1 = vdupq_n_s32((int32_t)(t4 & 0x1FFFFu));
        v1 = vsetq_lane_s32((int32_t)(t5 & 0x1FFFFu), v1, 1);
        v1 = vsetq_lane_s32((int32_t)(t6 & 0x1FFFFu), v1, 2);
        v1 = vsetq_lane_s32((int32_t)(t7 & 0x1FFFFu), v1, 3);
        vst1q_s32(&r->coeffs[i], v0);
        vst1q_s32(&r->coeffs[i + 4], v1);

        in += 17;
    }

#elif DARTS_MODE == 512
    // q = 260609, 18 bits
    for (unsigned int i = 0; i < N; i += 4) {
        uint32_t b0 = a[in + 0];
        uint32_t b1 = a[in + 1];
        uint32_t b2 = a[in + 2];
        uint32_t b3 = a[in + 3];
        uint32_t b4 = a[in + 4];
        uint32_t b5 = a[in + 5];
        uint32_t b6 = a[in + 6];
        uint32_t b7 = a[in + 7];
        uint32_t b8 = a[in + 8];

        uint32_t t0 = b0 | (b1 << 8) | ((b2 & 0x03u) << 16);
        uint32_t t1 = (b2 >> 2) | (b3 << 6) | ((b4 & 0x0Fu) << 14);
        uint32_t t2 = (b4 >> 4) | (b5 << 4) | ((b6 & 0x3Fu) << 12);
        uint32_t t3 = (b6 >> 6) | (b7 << 2) | (b8 << 10);

        int32x4_t v = vdupq_n_s32((int32_t)(t0 & 0x3FFFFu));
        v = vsetq_lane_s32((int32_t)(t1 & 0x3FFFFu), v, 1);
        v = vsetq_lane_s32((int32_t)(t2 & 0x3FFFFu), v, 2);
        v = vsetq_lane_s32((int32_t)(t3 & 0x3FFFFu), v, 3);
        vst1q_s32(&r->coeffs[i], v);

        in += 9;
    }
#endif
}

/*************************************************
 * Name:        poly_p_pack
 *
 * Description: Pack polynomial with coefficients in [-1, 1].
 *              Pr[X = 1] = Pr[X = -1] = P, Pr[X = 0] = 1 - 2P
 * 
 * Arguments:   - uint8_t *r: pointer to output byte array with 
 *                            at least POLY_S_PACKEDBYTES bytes
 *              - const poly *a: pointer to input polynomial
 **************************************************/
void poly_p_pack(uint8_t *r, const poly *a) {
    unsigned int i;
    uint8_t t[8];

    for (i = 0; i < N / 4; ++i) {
        t[0] = 1 - a->coeffs[4 * i + 0]; // 将 {-1,0,1} 映为 {2,1,0}
        t[1] = 1 - a->coeffs[4 * i + 1];
        t[2] = 1 - a->coeffs[4 * i + 2];
        t[3] = 1 - a->coeffs[4 * i + 3];
        r[i] = t[0] | (t[1] << 2) | (t[2] << 4) | (t[3] << 6);
    }
}

/*************************************************
 * Name:        poly_p_unpack
 *
 * Description: Unpack polynomial with coefficients in [-1, 1].
 *              Pr[X = 1] = Pr[X = -1] = P, Pr[X = 0] = 1 - 2P
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const uint8_t *a: byte array with bit-packed polynomial
 **************************************************/
void poly_p_unpack(poly *r, const uint8_t *a) {
    unsigned int i;

    for (i = 0; i < N / 4; ++i) {
        r->coeffs[4 * i + 0] = 1 - ((a[i] >> 0) & 0x03); // 将 {0,1,2} 映为 {1,0,-1}
        r->coeffs[4 * i + 1] = 1 - ((a[i] >> 2) & 0x03);
        r->coeffs[4 * i + 2] = 1 - ((a[i] >> 4) & 0x03);
        r->coeffs[4 * i + 3] = 1 - ((a[i] >> 6) & 0x03);
    }
}

/*************************************************
 * Name:        poly_pack_highbits
 *
 * Description: Pack the high bits of a polynomial (does compression internally)
 *              Input should be in [0, Q-1] range.
 *
 * Arguments:   - uint8_t *r: pointer to output byte array with at least
 *                            N * D / 8 bytes
 *              - const poly *a: pointer to input polynomial in [0, Q-1]
 **************************************************/
void poly_pack_highbits(uint8_t *r, const poly *a) {
    unsigned int i;    

#if D == 10
    for (i = 0; i < N / 4; ++i) {
        uint32_t t0 = (uint32_t)(((a->coeffs[4 * i + 0] << D) + Q/2) / Q) & 0x3FFu;
        uint32_t t1 = (uint32_t)(((a->coeffs[4 * i + 1] << D) + Q/2) / Q) & 0x3FFu;
        uint32_t t2 = (uint32_t)(((a->coeffs[4 * i + 2] << D) + Q/2) / Q) & 0x3FFu;
        uint32_t t3 = (uint32_t)(((a->coeffs[4 * i + 3] << D) + Q/2) / Q) & 0x3FFu;

        r[5 * i + 0] = (uint8_t)(t0);
        r[5 * i + 1] = (uint8_t)(t0 >> 8 | (t1 << 2));
        r[5 * i + 2] = (uint8_t)(t1 >> 6 | (t2 << 4));
        r[5 * i + 3] = (uint8_t)(t2 >> 4 | (t3 << 6));
        r[5 * i + 4] = (uint8_t)(t3 >> 2);
    }

#elif D == 9
    for (i = 0; i < N / 8; ++i) {
        uint32_t t0 = (uint32_t)(((a->coeffs[8 * i + 0] << D) + Q/2) / Q) & 0x1FFu;
        uint32_t t1 = (uint32_t)(((a->coeffs[8 * i + 1] << D) + Q/2) / Q) & 0x1FFu;
        uint32_t t2 = (uint32_t)(((a->coeffs[8 * i + 2] << D) + Q/2) / Q) & 0x1FFu;
        uint32_t t3 = (uint32_t)(((a->coeffs[8 * i + 3] << D) + Q/2) / Q) & 0x1FFu;
        uint32_t t4 = (uint32_t)(((a->coeffs[8 * i + 4] << D) + Q/2) / Q) & 0x1FFu;
        uint32_t t5 = (uint32_t)(((a->coeffs[8 * i + 5] << D) + Q/2) / Q) & 0x1FFu;
        uint32_t t6 = (uint32_t)(((a->coeffs[8 * i + 6] << D) + Q/2) / Q) & 0x1FFu;
        uint32_t t7 = (uint32_t)(((a->coeffs[8 * i + 7] << D) + Q/2) / Q) & 0x1FFu;

        r[9 * i + 0] = (uint8_t)(t0);
        r[9 * i + 1] = (uint8_t)(t0 >> 8 | (t1 << 1));
        r[9 * i + 2] = (uint8_t)(t1 >> 7 | (t2 << 2));
        r[9 * i + 3] = (uint8_t)(t2 >> 6 | (t3 << 3));
        r[9 * i + 4] = (uint8_t)(t3 >> 5 | (t4 << 4));
        r[9 * i + 5] = (uint8_t)(t4 >> 4 | (t5 << 5));
        r[9 * i + 6] = (uint8_t)(t5 >> 3 | (t6 << 6));
        r[9 * i + 7] = (uint8_t)(t6 >> 2 | (t7 << 7));
        r[9 * i + 8] = (uint8_t)(t7 >> 1);
    }
#endif
}

/*************************************************
 * Name:        poly_pack_compressed
 *
 * Description: Pack already-compressed D-bit values (no compression performed).
 *              Input should already be in [0, 2^D - 1] range.
 *
 * Arguments:   - uint8_t *r: pointer to output byte array with at least
 *                            N * D / 8 bytes
 *              - const poly *a: pointer to input polynomial in [0, 2^D - 1]
 **************************************************/
void poly_pack_compressed(uint8_t *r, const poly *a) {
    unsigned int i;

#if D == 10
    for (i = 0; i < N / 4; ++i) {
        uint32_t t0 = (uint32_t)a->coeffs[4 * i + 0] & 0x3FFu;
        uint32_t t1 = (uint32_t)a->coeffs[4 * i + 1] & 0x3FFu;
        uint32_t t2 = (uint32_t)a->coeffs[4 * i + 2] & 0x3FFu;
        uint32_t t3 = (uint32_t)a->coeffs[4 * i + 3] & 0x3FFu;

        r[5 * i + 0] = (uint8_t)(t0);
        r[5 * i + 1] = (uint8_t)(t0 >> 8 | (t1 << 2));
        r[5 * i + 2] = (uint8_t)(t1 >> 6 | (t2 << 4));
        r[5 * i + 3] = (uint8_t)(t2 >> 4 | (t3 << 6));
        r[5 * i + 4] = (uint8_t)(t3 >> 2);
    }

#elif D == 9
    for (i = 0; i < N / 8; ++i) {
        uint32_t t0 = (uint32_t)a->coeffs[8 * i + 0] & 0x1FFu;
        uint32_t t1 = (uint32_t)a->coeffs[8 * i + 1] & 0x1FFu;
        uint32_t t2 = (uint32_t)a->coeffs[8 * i + 2] & 0x1FFu;
        uint32_t t3 = (uint32_t)a->coeffs[8 * i + 3] & 0x1FFu;
        uint32_t t4 = (uint32_t)a->coeffs[8 * i + 4] & 0x1FFu;
        uint32_t t5 = (uint32_t)a->coeffs[8 * i + 5] & 0x1FFu;
        uint32_t t6 = (uint32_t)a->coeffs[8 * i + 6] & 0x1FFu;
        uint32_t t7 = (uint32_t)a->coeffs[8 * i + 7] & 0x1FFu;

        r[9 * i + 0] = (uint8_t)(t0);
        r[9 * i + 1] = (uint8_t)(t0 >> 8 | (t1 << 1));
        r[9 * i + 2] = (uint8_t)(t1 >> 7 | (t2 << 2));
        r[9 * i + 3] = (uint8_t)(t2 >> 6 | (t3 << 3));
        r[9 * i + 4] = (uint8_t)(t3 >> 5 | (t4 << 4));
        r[9 * i + 5] = (uint8_t)(t4 >> 4 | (t5 << 5));
        r[9 * i + 6] = (uint8_t)(t5 >> 3 | (t6 << 6));
        r[9 * i + 7] = (uint8_t)(t6 >> 2 | (t7 << 7));
        r[9 * i + 8] = (uint8_t)(t7 >> 1);
    }
#endif
}

/*************************************************
 * Name:        poly_pack_lowbits
 *
 * Description: Pack the low bits of a polynomial using BAT encoding.
 *              For D=10 at 128-bit security: coefficients in [-64,64] are
 *              encoded as x' = x + 64 in [0,128], then decomposed as
 *              x' = 8*x1 + x2 where x1 in [0,16], x2 in [0,7].
 *              Each of 8 consecutive coefficients: 3*8 bits low (x2) + 
 *              base-17 encoding of 8 x1 values (33 bits) = 57 bits total.
 *
 *              For D=9 at 256-bit security: coefficients in [-128,128] are
 *              encoded similarly, reducing 72 bits to 65 bits per 8 coeff.
 *
 * Arguments:   - uint8_t *r: pointer to output byte array with at least
 *                            POLY_LOWBITS_PACKEDBYTES bytes
 *              - const poly *a: pointer to input polynomial
 **************************************************/
void poly_pack_lowbits(uint8_t *r, const poly *a) {
#if DARTS_MODE == 128
    /* 128-bits security: x in [-64,64] -> [0,128] -> 8*x1 + x2 */
    const int32_t offset = 64;
    const int32_t base = 8;      // x' = 8*x1 + x2 
    const int32_t base_val = 17; // base for encoding x1 values
    
    memset(r, 0, POLY_LOWBITS_PACKEDBYTES);
    int bit_offset = 0; 
    
    for (unsigned int i = 0; i < N; i += 8) {
        int32_t x1[8], x2[8];
        uint64_t base17_val = 0;
        
        /* Decompose each of 8 coefficients */
        for (unsigned int j = 0; j < 8; j++) {
            int32_t x_shifted = a->coeffs[i + j] + offset;
            x1[j] = x_shifted / base;
            x2[j] = x_shifted % base;
        }
        
        /* Encode x1 values in base-17 */
        for (unsigned int j = 0; j < 8; j++) {
            base17_val = base17_val * base_val + (uint64_t)x1[j];
        }
        
        /* Pack: 3 bits per x2 (24 bits total) + base17_val (33 bits) = 57 bits */
        uint64_t low_bits = (uint64_t)((x2[0] & 0x7) |
                                        ((x2[1] & 0x7) << 3) |
                                        ((x2[2] & 0x7) << 6) |
                                        ((x2[3] & 0x7) << 9) |
                                        ((x2[4] & 0x7) << 12) |
                                        ((x2[5] & 0x7) << 15) |
                                        ((x2[6] & 0x7) << 18) |
                                        ((x2[7] & 0x7) << 21));
        
        uint64_t packed = low_bits | (base17_val << 24);
        
        /* Write 57 bits to buffer with proper byte/bit alignment */
        for (int bit = 0; bit < 57; bit++) {
            int byte_idx = bit_offset / 8;
            int bit_in_byte = bit_offset % 8;
            uint64_t bit_val = (packed >> bit) & 1;
            r[byte_idx] |= (uint8_t)(bit_val << bit_in_byte);
            bit_offset++;
        }
    }
    
#elif DARTS_MODE == 256
    /* 256-bit security: x in [-128,128] -> [0,256] -> 16*x1 + x2 */
    const int32_t offset = 128;
    const int32_t base = 16;     // x' = 16*x1 + x2 
    const int32_t base_val = 17; // base for encoding x1 values

    memset(r, 0, POLY_LOWBITS_PACKEDBYTES);
    int bit_offset = 0;

    for (unsigned int i = 0; i < N; i += 8) {
        int32_t x1[8], x2[8];
        uint64_t base17_val = 0;
        
        /* Decompose each of 8 coefficients */
        for (unsigned int j = 0; j < 8; j++) {
            int32_t x_shifted = a->coeffs[i + j] + offset;
            x1[j] = x_shifted / base;
            x2[j] = x_shifted % base;
        }
        
        /* Encode x1 values in base-17 */
        for (unsigned int j = 0; j < 8; j++) {
            base17_val = base17_val * base_val + (uint64_t)x1[j];
        }
        
        /* Pack: 4 bits per x2 (32 bits total) + base17_val (33 bits) = 65 bits */
        uint64_t low_bits = (uint64_t)((x2[0] & 0xF) |
                                        ((x2[1] & 0xF) << 4) |
                                        ((x2[2] & 0xF) << 8) |
                                        ((x2[3] & 0xF) << 12) |
                                        ((x2[4] & 0xF) << 16) |
                                        ((x2[5] & 0xF) << 20) |
                                        ((x2[6] & 0xF) << 24) |
                                        ((uint64_t)(x2[7] & 0xF) << 28));
        
        uint64_t packed_low = low_bits | (base17_val << 32);
        uint8_t packed_high = (uint8_t)(base17_val >> 32);
        
        /* Write 65 bits to buffer with proper byte/bit alignment */
        for (int bit = 0; bit < 64; bit++) {
            int byte_idx = bit_offset / 8;
            int bit_in_byte = bit_offset % 8;
            uint64_t bit_val = (packed_low >> bit) & 1;
            r[byte_idx] |= (uint8_t)(bit_val << bit_in_byte);
            bit_offset++;
        }
        /* Write the 65th bit */
        int byte_idx = bit_offset / 8;
        int bit_in_byte = bit_offset % 8;
        r[byte_idx] |= (uint8_t)((packed_high & 1) << bit_in_byte);
        bit_offset++;
    }
#elif DARTS_MODE == 512
    unsigned int i;
    const int32x4_t v_offset = vdupq_n_s32(128);
    
    for (i = 0; i < N; i += 8) {
        int32x4_t v0 = vaddq_s32(vld1q_s32(&a->coeffs[i]), v_offset);
        int32x4_t v1 = vaddq_s32(vld1q_s32(&a->coeffs[i + 4]), v_offset);
        uint16x8_t v16 = vcombine_u16(vreinterpret_u16_s16(vmovn_s32(v0)),
                                      vreinterpret_u16_s16(vmovn_s32(v1)));
        vst1_u8(&r[i], vmovn_u16(v16));
    }

#endif
}

/*************************************************
 * Name:        poly_unpack_lowbits
 *
 * Description: Unpack the low bits of a polynomial using BAT decoding.
 *              Reverses the encoding from poly_pack_lowbits.
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const uint8_t *a: pointer to input byte array with at least
 *                                  POLY_LOWBITS_PACKEDBYTES bytes
 **************************************************/
void poly_unpack_lowbits(poly *r, const uint8_t *a) {
#if DARTS_MODE == 128
    /* 128-bit security: decode x' = 8*x1 + x2, then x = x' - 64 */
    const int32_t offset = 64;
    const int32_t base = 8;
    const int32_t base_val = 17;
    
    int bit_offset = 0;  /* Global bit offset in the input buffer */
    
    for (unsigned int i = 0; i < N; i += 8) {
        /* Read 57 bits from buffer */
        uint64_t packed = 0;
        for (int bit = 0; bit < 57; bit++) {
            int byte_idx = bit_offset / 8;
            int bit_in_byte = bit_offset % 8;
            uint64_t bit_val = (a[byte_idx] >> bit_in_byte) & 1;
            packed |= (bit_val << bit);
            bit_offset++;
        }
        
        /* Extract low bits (24 bits) and base17 value (33 bits) */
        uint64_t low_bits = packed & 0xFFFFFF;
        uint64_t base17_val = (packed >> 24) & 0x1FFFFFFFF;
        
        /* Decode x2 values from low_bits */
        int32_t x2[8];
        x2[0] = (int32_t)((low_bits >> 0) & 0x7);
        x2[1] = (int32_t)((low_bits >> 3) & 0x7);
        x2[2] = (int32_t)((low_bits >> 6) & 0x7);
        x2[3] = (int32_t)((low_bits >> 9) & 0x7);
        x2[4] = (int32_t)((low_bits >> 12) & 0x7);
        x2[5] = (int32_t)((low_bits >> 15) & 0x7);
        x2[6] = (int32_t)((low_bits >> 18) & 0x7);
        x2[7] = (int32_t)((low_bits >> 21) & 0x7);
        
        /* Decode x1 values from base17_val */
        int32_t x1[8];
        for (int j = 7; j >= 0; j--) {
            x1[j] = (int32_t)(base17_val % base_val);
            base17_val /= base_val;
        }
        
        /* Reconstruct coefficients */
        for (unsigned int j = 0; j < 8; j++) {
            int32_t x_shifted = base * x1[j] + x2[j];
            r->coeffs[i + j] = x_shifted - offset;
        }
    }
    
#elif DARTS_MODE == 256
    /* 256-bit security: decode x' = 16*x1 + x2, then x = x' - 128 */
    const int32_t offset = 128;
    const int32_t base = 16;
    const int32_t base_val = 17;
    
    int bit_offset = 0;  /* Global bit offset in the input buffer */
    
    for (unsigned int i = 0; i < N; i += 8) {
        /* Read 65 bits from buffer */
        uint64_t packed_low = 0;
        for (int bit = 0; bit < 64; bit++) {
            int byte_idx = bit_offset / 8;
            int bit_in_byte = bit_offset % 8;
            uint64_t bit_val = (a[byte_idx] >> bit_in_byte) & 1;
            packed_low |= (bit_val << bit);
            bit_offset++;
        }
        /* Read the 65th bit */
        int byte_idx = bit_offset / 8;
        int bit_in_byte = bit_offset % 8;
        uint64_t packed_high = (a[byte_idx] >> bit_in_byte) & 1;
        bit_offset++;
        
        /* Extract low bits (32 bits) and base17 value (33 bits) */
        uint64_t low_bits = packed_low & 0xFFFFFFFF;
        uint64_t base17_val = (packed_low >> 32) | (packed_high << 32);
        
        /* Decode x2 values from low_bits */
        int32_t x2[8];
        x2[0] = (int32_t)((low_bits >> 0) & 0xF);
        x2[1] = (int32_t)((low_bits >> 4) & 0xF);
        x2[2] = (int32_t)((low_bits >> 8) & 0xF);
        x2[3] = (int32_t)((low_bits >> 12) & 0xF);
        x2[4] = (int32_t)((low_bits >> 16) & 0xF);
        x2[5] = (int32_t)((low_bits >> 20) & 0xF);
        x2[6] = (int32_t)((low_bits >> 24) & 0xF);
        x2[7] = (int32_t)((low_bits >> 28) & 0xF);
        
        /* Decode x1 values from base17_val */
        int32_t x1[8];
        for (int j = 7; j >= 0; j--) {
            x1[j] = (int32_t)(base17_val % base_val);
            base17_val /= base_val;
        }
        
        /* Reconstruct coefficients */
        for (unsigned int j = 0; j < 8; j++) {
            int32_t x_shifted = base * x1[j] + x2[j];
            r->coeffs[i + j] = x_shifted - offset;
        }
    }
#elif DARTS_MODE == 512
    unsigned int i;
    const int32x4_t v_offset = vdupq_n_s32(128);

    for (i = 0; i < N; i += 8) {
        uint16x8_t v16 = vmovl_u8(vld1_u8(&a[i]));
        uint32x4_t lo = vmovl_u16(vget_low_u16(v16));
        uint32x4_t hi = vmovl_u16(vget_high_u16(v16));
        vst1q_s32(&r->coeffs[i],
                  vsubq_s32(vreinterpretq_s32_u32(lo), v_offset));
        vst1q_s32(&r->coeffs[i + 4],
                  vsubq_s32(vreinterpretq_s32_u32(hi), v_offset));
    }
#endif
}

// 定义多项式的采样------------------------------------------------------------------------------------

/*************************************************
 * Name:        poly_challenge
 *
 * Description: Generate challenge polynomial wiht Hamming weight of TAU.
 *
 * Arguments:   - poly *c: pointer to output challenge polynomial
 *              - const uint8_t highbits[]: pointer to compressed w
 *              - const uint8_t mu[CRHBYTES]: pointer to message hash (64 bytes)
 **************************************************/
void poly_challenge(poly *c, const uint8_t highbits[POLYVECK_HIGHBITS_PACKEDBYTES], const uint8_t mu[CRHBYTES]) {
    unsigned int i, b, pos = 0;
    uint8_t buf[SM3_XOF_BLOCKBYTES];
    sm3_xof_state state;

    sm3_xof_absorb_twice(&state, highbits,
                         POLYVECK_HIGHBITS_PACKEDBYTES, mu,
                         CRHBYTES);
    sm3_xof_squeezeblocks(buf, 1, &state);

    for (i = 0; i < N; ++i)
        c->coeffs[i] = 0;
    for (i = N - TAU; i < N; ++i) {
        do {
#if N == 512
            // N=512: i ≤ 511, need 9 bits, read 2 bytes, mask to 9 bits
            if (pos + 1 >= SM3_XOF_BLOCKBYTES) {
                sm3_xof_squeezeblocks(buf, 1, &state);
                pos = 0;
            }
            b = buf[pos] | ((unsigned int)buf[pos + 1] << 8);
            pos += 2;
            b &= 0x1FFu;
#elif N == 1024
            // N=1024: i ≤ 1023, need 10 bits, read 2 bytes, mask to 10 bits
            if (pos + 1 >= SM3_XOF_BLOCKBYTES) {
                sm3_xof_squeezeblocks(buf, 1, &state);
                pos = 0;
            }
            b = buf[pos] | ((unsigned int)buf[pos + 1] << 8);
            pos += 2;
            b &= 0x3FFu;
#endif
        } while (b > i);

        c->coeffs[i] = c->coeffs[b];
        c->coeffs[b] = 1;
    }
}

/*************************************************
 * Name:        poly_uniform
 *
 * Description: Generate a polynomial with uniform random coefficients in [0, Q-1]
 *              by performing rejection sampling on output of a SHAKE128(seed|nonce).
 *
 * Arguments:   - poly *a: pointer to output polynomial
 *              - const uint8_t seed[SEEDBYTES]: byte array with seed of length SEEDBYTES
 *              - uint16_t nonce: 2-byte nonce
 **************************************************/
void poly_uniform(poly *a, const uint8_t seed[SEEDBYTES], uint16_t nonce) {
#if DARTS_MODE == 128 || DARTS_MODE == 256
    unsigned int blocks_numbers = 9;
    // (( (N * Q_BITS) + 7) / 8 + SM3_XOF_BLOCKBYTES - 1) / SM3_XOF_BLOCKBYTES
#elif DARTS_MODE == 512
    unsigned int blocks_numbers = 9;
    // (( (N * Q_BITS) + 7) / 8 + SM3_XOF_BLOCKBYTES - 1) / SM3_XOF_BLOCKBYTES
#endif

    unsigned int i, ctr, off = 0;
    unsigned int buflen = blocks_numbers * SM3_XOF_BLOCKBYTES;
    uint8_t buf[blocks_numbers * SM3_XOF_BLOCKBYTES + 2];
    sm3_xof_state state;

    sm3_xof_stream_init(&state, seed, SEEDBYTES, nonce);
    sm3_xof_squeezeblocks(buf, blocks_numbers, &state);

    ctr = rej_uniform(a->coeffs, N, buf, buflen);

    while (ctr < N) {
        off = ((8 * buflen) % 16 + 7) / 8;
        for (i = 0; i < off; ++i)
            buf[i] = buf[buflen - off + i];

        sm3_xof_squeezeblocks(buf + off, 1, &state);
        buflen = SM3_XOF_BLOCKBYTES + off;
        ctr += rej_uniform(a->coeffs + ctr, N - ctr, buf, buflen);
    }
}

/*************************************************
 * Name:        poly_ternary_p
 *
 * Description: Generate a polynomial with coefficients in {-1,0,1}
 *              such that Pr[X = 1] = Pr[X = -1] = P, Pr[X = 0] = 1 - 2P
 *              using output of a SHAKE128(seed|nonce).
 *
 * Arguments:   - poly *a: pointer to output polynomial
 *              - const uint8_t seed[SEEDBYTES]: byte array with seed of length SEEDBYTES
 *              - uint16_t nonce: 2-byte nonce
 **************************************************/
void poly_ternary_p(poly *a, const uint8_t seed[CRHBYTES], uint16_t nonce) {
#if DARTS_MODE == 128
    unsigned int blocks_numbers = 3;
    // Pr[X = 1] = Pr[X = -1] = 0.15, Pr[X = 0] = 0.7
    // ((N * 13 / 3 * 8) + SM3_XOF_BLOCKBYTES - 1) / SM3_XOF_BLOCKBYTES
#elif DARTS_MODE == 256
    unsigned int blocks_numbers = 2;
    // Pr[X = 1] = Pr[X = -1] = 0.3125, Pr[X = 0] = 0.375
    // ((N * 4 / 8) + SM3_XOF_BLOCKBYTES - 1) / SM3_XOF_BLOCKBYTES
#elif DARTS_MODE == 512
    unsigned int blocks_numbers = 5;
    // Pr[X = 1] = Pr[X = -1] = 0.2, Pr[X = 0] = 0.6
    // ((N * 13 / 3 * 8) + SM3_XOF_BLOCKBYTES - 1) / SM3_XOF_BLOCKBYTES
#endif

    unsigned int ctr = 0;
    unsigned int buflen = blocks_numbers * SM3_XOF_BLOCKBYTES;
    uint8_t buf[blocks_numbers * SM3_XOF_BLOCKBYTES];
    sm3_xof_state state;

    sm3_xof_stream_init(&state, seed, CRHBYTES, nonce);
    sm3_xof_squeezeblocks(buf, blocks_numbers, &state);

    ctr = rej_p(a->coeffs, N, buf, buflen);

    while (ctr < N) {
        sm3_xof_squeezeblocks(buf, 1, &state);
        ctr += rej_p(a->coeffs + ctr, N - ctr, buf, SM3_XOF_BLOCKBYTES);
    }
}
