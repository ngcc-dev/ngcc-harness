/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */

#include "sample.h"
#include "symmetric.h"
#include "endian.h"
#include "align.h"
#include <stddef.h>
#include <stdint.h>

#if BIT_USE_SHAKE
#define REJECT_SAMPLE_SQUEEZE_BYTES BIT_XOF256_RATE
#else
#define REJECT_SAMPLE_SQUEEZE_BYTES 136
#endif

/* 12-bit triangular sampling: 3 bytes per 2 values */
void poly_sample_triangular(int32_t *out, const uint8_t *seed, uint16_t *nonce)
{
    unsigned int i;
    uint8_t buf[BYTELEN];

    bit_xof256_nonce(buf, sizeof(buf), seed, BIT_SEEDBYTES, *nonce);
    *nonce = (uint16_t)(*nonce + 1U);

    for (i = 0; i < BIT_N / 2; i++) {
        const uint8_t *lo = buf + 3 * i;
        const uint8_t *hi = buf + BYTELEN / 2 + 3 * i;

        uint32_t lo_raw = load24_le(lo);
        uint32_t hi_raw = load24_le(hi);

        out[2 * i + 0] = (int32_t)(lo_raw & 0xFFF) - (int32_t)(hi_raw & 0xFFF);
        out[2 * i + 1] = (int32_t)(lo_raw >> 12)   - (int32_t)(hi_raw >> 12);
    }
}

/* 13-bit triangular: LCM(13,8)=104 bits = 13B per 8 values per half */
#define BYTELEN_13BIT (2 * 13 * BIT_N / 8)

void poly_sample_triangular_13bit(int32_t *out, const uint8_t *seed, uint16_t *nonce)
{
    uint8_t buf[BYTELEN_13BIT];
    bit_xof256_nonce(buf, sizeof(buf), seed, BIT_SEEDBYTES, *nonce);
    *nonce = (uint16_t)(*nonce + 1U);

    for (int i = 0; i < BIT_N / 8; i++) {
        const uint8_t *lo = buf + 13 * i;
        const uint8_t *hi = buf + BYTELEN_13BIT / 2 + 13 * i;

        uint32_t a0 = load16_le(lo) | ((uint32_t)lo[2] << 16);
        uint32_t a1 = ((uint32_t)lo[2] >> 5) | ((uint32_t)lo[3] << 3) | ((uint32_t)lo[4] << 11);
        uint32_t a2 = ((uint32_t)lo[4] >> 2) | ((uint32_t)lo[5] << 6);
        uint32_t a3 = ((uint32_t)lo[5] >> 7) | ((uint32_t)lo[6] << 1) | ((uint32_t)lo[7] << 9);
        uint32_t a4 = ((uint32_t)lo[7] >> 4) | ((uint32_t)lo[8] << 4);
        uint32_t a5 = ((uint32_t)lo[8] >> 1) | ((uint32_t)lo[9] << 7);
        uint32_t a6 = ((uint32_t)lo[9] >> 6) | ((uint32_t)lo[10] << 2) | ((uint32_t)lo[11] << 10);
        uint32_t a7 = ((uint32_t)lo[11] >> 3) | ((uint32_t)lo[12] << 5);

        uint32_t b0 = load16_le(hi) | ((uint32_t)hi[2] << 16);
        uint32_t b1 = ((uint32_t)hi[2] >> 5) | ((uint32_t)hi[3] << 3) | ((uint32_t)hi[4] << 11);
        uint32_t b2 = ((uint32_t)hi[4] >> 2) | ((uint32_t)hi[5] << 6);
        uint32_t b3 = ((uint32_t)hi[5] >> 7) | ((uint32_t)hi[6] << 1) | ((uint32_t)hi[7] << 9);
        uint32_t b4 = ((uint32_t)hi[7] >> 4) | ((uint32_t)hi[8] << 4);
        uint32_t b5 = ((uint32_t)hi[8] >> 1) | ((uint32_t)hi[9] << 7);
        uint32_t b6 = ((uint32_t)hi[9] >> 6) | ((uint32_t)hi[10] << 2) | ((uint32_t)hi[11] << 10);
        uint32_t b7 = ((uint32_t)hi[11] >> 3) | ((uint32_t)hi[12] << 5);

#define M13 0x1FFFU
        out[8*i+0] = (int32_t)(a0 & M13) - (int32_t)(b0 & M13);
        out[8*i+1] = (int32_t)(a1 & M13) - (int32_t)(b1 & M13);
        out[8*i+2] = (int32_t)(a2 & M13) - (int32_t)(b2 & M13);
        out[8*i+3] = (int32_t)(a3 & M13) - (int32_t)(b3 & M13);
        out[8*i+4] = (int32_t)(a4 & M13) - (int32_t)(b4 & M13);
        out[8*i+5] = (int32_t)(a5 & M13) - (int32_t)(b5 & M13);
        out[8*i+6] = (int32_t)(a6 & M13) - (int32_t)(b6 & M13);
        out[8*i+7] = (int32_t)(a7 & M13) - (int32_t)(b7 & M13);
#undef M13
    }
}

/* 4-bit eta0 sampling (gamma1_0=16): 4 bits per coeff, lo+hi each 4 bits.
   buf = [lo_half(N/2 B) | hi_half(N/2 B)], total N = 512 B. */
void poly_sample_triangular_eta0(int32_t *out, const uint8_t *seed, uint16_t *nonce)
{
    uint8_t buf[BIT_N];

    bit_xof256_nonce(buf, sizeof(buf), seed, BIT_SEEDBYTES, *nonce);
    *nonce = (uint16_t)(*nonce + 1U);

    for (int i = 0; i < BIT_N / 4; i++) {
        const uint8_t *lo = buf + 2 * i;
        const uint8_t *hi = buf + BIT_N / 2 + 2 * i;

        uint8_t l0 = lo[0], l1 = lo[1];
        uint8_t h0 = hi[0], h1 = hi[1];

        out[4*i + 0] = (int32_t)(l0 & 0x0F)        - (int32_t)(h0 & 0x0F);
        out[4*i + 1] = (int32_t)((l0 >> 4) & 0x0F) - (int32_t)((h0 >> 4) & 0x0F);
        out[4*i + 2] = (int32_t)(l1 & 0x0F)        - (int32_t)(h1 & 0x0F);
        out[4*i + 3] = (int32_t)((l1 >> 4) & 0x0F) - (int32_t)((h1 >> 4) & 0x0F);
    }
}

#define TWO_POW_24 16777216ULL

_Static_assert(BIT_GAMMA1 == 4096 && BIT_BETA == 58,
               "fast path in reject_sample uses constants derived for gamma1=4096,beta2=58");

static int reject_sample_coeffs(const poly *z,
                                const int32_t *v,
                                int32_t gamma1,
                                int32_t beta2,
                                bit_xof256_state *st) {
    uint8_t r_buf[REJECT_SAMPLE_SQUEEZE_BYTES];
    const int32_t fast_accept_lo = beta2;
    const int32_t fast_accept_hi = gamma1 - beta2;
    const int32_t G_fast = 2 * (gamma1 - beta2);
    size_t buf_len = 0;
    size_t pos = sizeof(r_buf);
    uint32_t reject_flag = 0;

    for (int j = 0; j < BIT_N; j++) {
        int32_t val = ct_abs(z->coeffs[j]);
        int32_t beta1_i = ct_abs(v[j]);

        if (val >= fast_accept_lo && val < fast_accept_hi) {
            continue;
        }

        if ((uint32_t)(val >= gamma1)) {
            reject_flag = 1;
            continue;
        }

        if (pos + 3 > buf_len) {
            bit_xof256_squeeze(st, r_buf, sizeof(r_buf));
            buf_len = sizeof(r_buf);
            pos = 0;
        }

        uint32_t R = load24_le(r_buf + pos);
        pos += 3;

        if (val < beta2) {
            int32_t F_int = 2 * gamma1 - 2 * ct_max(val, beta1_i);
            if ((uint64_t)R * F_int > (uint64_t)G_fast * TWO_POW_24) {
                reject_flag = 1;
            }
        } else {
            int32_t f_minus = ct_max(gamma1 - ct_abs(val - beta1_i), 0);
            int32_t f_plus  = ct_max(gamma1 - (val + beta1_i), 0);
            int32_t F_int = f_minus + f_plus;
            int32_t G_int = 2 * ct_max(gamma1 - ct_max(val, beta2), 0);
            if ((uint64_t)R * F_int > (uint64_t)G_int * TWO_POW_24) {
                reject_flag = 1;
            }
        }
    }

    return (int)reject_flag;
}

static int reject_sample_coeffs_sparse(const poly *z,
                                       const sparse_challenge *support,
                                       const int32_t *v,
                                       int32_t gamma1,
                                       int32_t beta2,
                                       bit_xof256_state *st) {
    uint8_t r_buf[REJECT_SAMPLE_SQUEEZE_BYTES];
    const int32_t fast_accept_lo = beta2;
    const int32_t fast_accept_hi = gamma1 - beta2;
    const int32_t G_fast = 2 * (gamma1 - beta2);
    size_t buf_len = 0;
    size_t pos = sizeof(r_buf);
    uint32_t reject_flag = 0;

    for (int t = 0; t < BIT_TAU; t++) {
        int j = support->pos[t];
        int32_t val = ct_abs(z->coeffs[j]);
        int32_t beta1_i = ct_abs(v[j]);

        if (val >= fast_accept_lo && val < fast_accept_hi) {
            continue;
        }

        if ((uint32_t)(val >= gamma1)) {
            reject_flag = 1;
            continue;
        }

        if (pos + 3 > buf_len) {
            bit_xof256_squeeze(st, r_buf, sizeof(r_buf));
            buf_len = sizeof(r_buf);
            pos = 0;
        }

        uint32_t R = load24_le(r_buf + pos);
        pos += 3;

        if (val < beta2) {
            int32_t F_int = 2 * gamma1 - 2 * ct_max(val, beta1_i);
            if ((uint64_t)R * F_int > (uint64_t)G_fast * TWO_POW_24) {
                reject_flag = 1;
            }
        } else {
            int32_t f_minus = ct_max(gamma1 - ct_abs(val - beta1_i), 0);
            int32_t f_plus  = ct_max(gamma1 - (val + beta1_i), 0);
            int32_t F_int = f_minus + f_plus;
            int32_t G_int = 2 * ct_max(gamma1 - ct_max(val, beta2), 0);
            if ((uint64_t)R * F_int > (uint64_t)G_int * TWO_POW_24) {
                reject_flag = 1;
            }
        }
    }

    return (int)reject_flag;
}

int check_reject_sample_z0(const poly *z0, const unsigned char *seed, uint16_t *nonce) {
    bit_xof256_state st;
    int ret;
    int32_t v[BIT_N];
    for (int i = 0; i < BIT_N; i++) v[i] = 1;

    bit_xof256_init(&st, seed, BIT_SEEDBYTES, *nonce);
    ret = reject_sample_coeffs(z0, v, BIT_GAMMA1_0, 1, &st);
    *nonce = (uint16_t)(*nonce + 1U);
    bit_xof256_zeroize(&st);
    return ret;
}

/* Check rejection for z0 and z1 using independent per-polynomial XOF streams. */
int check_reject_sample_z0z1(const polyvecy *z, const polyvecl *c_s,
                              const poly *c_poly, const sparse_challenge *c_sparse,
                              const unsigned char *seed, uint16_t *nonce) {
    int global_reject = 0;

    for (int i = 1; i <= BIT_L; i++) {
        bit_xof256_state st;
        bit_xof256_init(&st, seed, BIT_SEEDBYTES, (uint16_t)(*nonce + i));
        int rej = reject_sample_coeffs(&z->vec[i], c_s->vec[i - 1].coeffs, BIT_GAMMA1, BIT_BETA, &st);
        bit_xof256_zeroize(&st);
        if (rej) { global_reject = 1; break; }
    }

    if (!global_reject) {
        bit_xof256_state st0;
        bit_xof256_init(&st0, seed, BIT_SEEDBYTES, (uint16_t)(*nonce + 0));
        int rej = reject_sample_coeffs_sparse(&z->vec[0], c_sparse, c_poly->coeffs, BIT_GAMMA1_0, 1, &st0);
        bit_xof256_zeroize(&st0);
        if (rej) global_reject = 1;
    }

    *nonce = (uint16_t)(*nonce + 4U);
    return global_reject;
}

int check_reject_sample_z2(const polyvecy *z, const polyveck *c_e, const unsigned char *seed, uint16_t *nonce) {
    int global_reject = 0;

    for (int i = BIT_L + 1; i < BIT_Y; i++) {
        bit_xof256_state st;
        bit_xof256_init(&st, seed, BIT_SEEDBYTES, (uint16_t)(*nonce + (i - BIT_L - 1)));
        int rej = reject_sample_coeffs(&z->vec[i], c_e->vec[i - BIT_L - 1].coeffs, BIT_GAMMA1_2, BIT_BETA, &st);
        bit_xof256_zeroize(&st);
        if (rej) { global_reject = 1; break; }
    }

    *nonce = (uint16_t)(*nonce + (uint16_t)BIT_K);
    return global_reject;
}

int check_reject_norm(const polyvecm1 *z1, const polyveck *h) {
    uint32_t reject_flag = 0;

    for (int i = 0; i < BIT_L + 1; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int32_t val = (int32_t)z1->vec[i].coeffs[j];
            int32_t abs_val = ct_abs(val);
            uint32_t over_bound = (uint32_t)(BIT_B_INF - abs_val) >> 31;
            reject_flag |= over_bound;
        }
    }

    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int32_t abs_h = ct_abs(h->vec[i].coeffs[j]);
            reject_flag |= (uint32_t)(BIT_H_INF - abs_h) >> 31;
        }
    }

    return reject_flag != 0 ? 1 : 0;
}

int check_reject_hint_range(const polyveck *h) {
    uint32_t reject_flag = 0;
    for (int i = 0; i < BIT_K; i++)
        for (int j = 0; j < BIT_N; j++) {
            int32_t abs_h = ct_abs(h->vec[i].coeffs[j]);
            reject_flag |= (uint32_t)(BIT_H_INF - abs_h) >> 31;
        }
    return (int)reject_flag;
}
