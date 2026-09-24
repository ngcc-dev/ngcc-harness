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

#define BYTELEN (10 * 2 * BIT_N / 8)
void poly_sample_triangular(int16_t *out, const uint8_t *seed, uint16_t *nonce)
{
    unsigned int i;
    uint8_t BIT_ALIGN buf[BYTELEN];
    bit_xof256_nonce(buf, sizeof(buf), seed, BIT_SEEDBYTES, *nonce);
    *nonce = (uint16_t)(*nonce + 1U);

    for(i = 0; i < BIT_N / 4; i++)
    {
        const uint8_t *lo = buf + 5 * i;
        const uint8_t *hi = buf + BYTELEN / 2 + 5 * i;

        uint16_t lo0 = load16_le(lo) & 0x03FF;
        uint16_t lo1 = (((uint16_t)lo[1] >> 2) | ((uint16_t)lo[2] << 6)) & 0x03FF;
        uint16_t lo2 = (((uint16_t)lo[2] >> 4) | ((uint16_t)lo[3] << 4)) & 0x03FF;
        uint16_t lo3 = (((uint16_t)lo[3] >> 6) | ((uint16_t)lo[4] << 2)) & 0x03FF;

        uint16_t hi0 = load16_le(hi) & 0x03FF;
        uint16_t hi1 = (((uint16_t)hi[1] >> 2) | ((uint16_t)hi[2] << 6)) & 0x03FF;
        uint16_t hi2 = (((uint16_t)hi[2] >> 4) | ((uint16_t)hi[3] << 4)) & 0x03FF;
        uint16_t hi3 = (((uint16_t)hi[3] >> 6) | ((uint16_t)hi[4] << 2)) & 0x03FF;

        out[4 * i + 0] = (int16_t)lo0 - (int16_t)hi0;
        out[4 * i + 1] = (int16_t)lo1 - (int16_t)hi1;
        out[4 * i + 2] = (int16_t)lo2 - (int16_t)hi2;
        out[4 * i + 3] = (int16_t)lo3 - (int16_t)hi3;
    }
}

#define BYTELEN_11 (11 * 2 * BIT_N / 8)

void poly_sample_triangular_11bit(int16_t *out, const uint8_t *seed, uint16_t *nonce)
{
    uint8_t BIT_ALIGN buf[BYTELEN_11];
    bit_xof256_nonce(buf, sizeof(buf), seed, BIT_SEEDBYTES, *nonce);
    *nonce = (uint16_t)(*nonce + 1U);

    for (int i = 0; i < BIT_N / 8; i++) {
        const uint8_t *lo = buf + 11 * i;
        const uint8_t *hi = buf + BYTELEN_11 / 2 + 11 * i;

        uint16_t a0 = load16_le(lo) & 0x07FF;
        uint16_t a1 = (((uint16_t)lo[1] >> 3) | ((uint16_t)lo[2] << 5)) & 0x07FF;
        uint16_t a2 = (((uint16_t)lo[2] >> 6) | ((uint16_t)lo[3] << 2) | ((uint16_t)lo[4] << 10)) & 0x07FF;
        uint16_t a3 = (((uint16_t)lo[4] >> 1) | ((uint16_t)lo[5] << 7)) & 0x07FF;
        uint16_t a4 = (((uint16_t)lo[5] >> 4) | ((uint16_t)lo[6] << 4)) & 0x07FF;
        uint16_t a5 = (((uint16_t)lo[6] >> 7) | ((uint16_t)lo[7] << 1) | ((uint16_t)lo[8] << 9)) & 0x07FF;
        uint16_t a6 = (((uint16_t)lo[8] >> 2) | ((uint16_t)lo[9] << 6)) & 0x07FF;
        uint16_t a7 = (((uint16_t)lo[9] >> 5) | ((uint16_t)lo[10] << 3)) & 0x07FF;

        uint16_t b0 = load16_le(hi) & 0x07FF;
        uint16_t b1 = (((uint16_t)hi[1] >> 3) | ((uint16_t)hi[2] << 5)) & 0x07FF;
        uint16_t b2 = (((uint16_t)hi[2] >> 6) | ((uint16_t)hi[3] << 2) | ((uint16_t)hi[4] << 10)) & 0x07FF;
        uint16_t b3 = (((uint16_t)hi[4] >> 1) | ((uint16_t)hi[5] << 7)) & 0x07FF;
        uint16_t b4 = (((uint16_t)hi[5] >> 4) | ((uint16_t)hi[6] << 4)) & 0x07FF;
        uint16_t b5 = (((uint16_t)hi[6] >> 7) | ((uint16_t)hi[7] << 1) | ((uint16_t)hi[8] << 9)) & 0x07FF;
        uint16_t b6 = (((uint16_t)hi[8] >> 2) | ((uint16_t)hi[9] << 6)) & 0x07FF;
        uint16_t b7 = (((uint16_t)hi[9] >> 5) | ((uint16_t)hi[10] << 3)) & 0x07FF;

        out[8 * i + 0] = (int16_t)a0 - (int16_t)b0;
        out[8 * i + 1] = (int16_t)a1 - (int16_t)b1;
        out[8 * i + 2] = (int16_t)a2 - (int16_t)b2;
        out[8 * i + 3] = (int16_t)a3 - (int16_t)b3;
        out[8 * i + 4] = (int16_t)a4 - (int16_t)b4;
        out[8 * i + 5] = (int16_t)a5 - (int16_t)b5;
        out[8 * i + 6] = (int16_t)a6 - (int16_t)b6;
        out[8 * i + 7] = (int16_t)a7 - (int16_t)b7;
    }
}

/* Triangular sampling with 3-bit coefficients */
#define BYTELEN_3BIT (2 * 3 * BIT_N / 8)

void poly_sample_triangular_3bit(int16_t *out, const uint8_t *seed, uint16_t *nonce)
{
    unsigned int i;
    uint8_t BIT_ALIGN buf[BYTELEN_3BIT];
    bit_xof256_nonce(buf, sizeof(buf), seed, BIT_SEEDBYTES, *nonce);
    *nonce = (uint16_t)(*nonce + 1U);

    for (i = 0; i < BIT_N / 8; i++) {
        const uint8_t *lo = buf + 3 * i;
        const uint8_t *hi = buf + BYTELEN_3BIT / 2 + 3 * i;

        uint8_t l0 = lo[0], l1 = lo[1], l2 = lo[2];
        uint8_t h0 = hi[0], h1 = hi[1], h2 = hi[2];

        out[8 * i + 0] = (int16_t)(l0 & 0x07)         - (int16_t)(h0 & 0x07);
        out[8 * i + 1] = (int16_t)((l0 >> 3) & 0x07)  - (int16_t)((h0 >> 3) & 0x07);
        out[8 * i + 2] = (int16_t)(((l0 >> 6) | (l1 << 2)) & 0x07)
                       - (int16_t)(((h0 >> 6) | (h1 << 2)) & 0x07);
        out[8 * i + 3] = (int16_t)((l1 >> 1) & 0x07)  - (int16_t)((h1 >> 1) & 0x07);
        out[8 * i + 4] = (int16_t)((l1 >> 4) & 0x07)  - (int16_t)((h1 >> 4) & 0x07);
        out[8 * i + 5] = (int16_t)(((l1 >> 7) | (l2 << 1)) & 0x07)
                       - (int16_t)(((h1 >> 7) | (h2 << 1)) & 0x07);
        out[8 * i + 6] = (int16_t)((l2 >> 2) & 0x07)  - (int16_t)((h2 >> 2) & 0x07);
        out[8 * i + 7] = (int16_t)((l2 >> 5) & 0x07)  - (int16_t)((h2 >> 5) & 0x07);
    }
}

#define TWO_POW_24 16777216ULL



static int reject_sample_coeffs(const poly *z,
                                const int16_t *v,
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
                                       const int16_t *v,
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

/* Check rejection for z0 and z1 using independent per-polynomial XOF streams. */
int check_reject_sample_z0z1(const polyvecy *z, const polyvecl *c_s,
                              const poly *c_poly, const sparse_challenge *c_sparse,
                              const unsigned char *seed, uint16_t *nonce) {
    int global_reject = 0;

    /* z1 streams */
    for (int i = 1; i <= BIT_L; i++) {
        bit_xof256_state st_i;
        bit_xof256_init(&st_i, seed, BIT_SEEDBYTES, (uint16_t)(*nonce + i));
        int rej = reject_sample_coeffs(&z->vec[i], c_s->vec[i - 1].coeffs, BIT_GAMMA1, BIT_BETA, &st_i);
        bit_xof256_zeroize(&st_i);
        if (rej) { global_reject = 1; break; }
    }

    /* z0 stream */
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

/* Check rejection for z2 using independent per-polynomial XOF streams. */
int check_reject_sample_z2(const polyvecy *z, const polyveck *c_e, const unsigned char *seed, uint16_t *nonce) {
    int global_reject = 0;

    for (int i = BIT_L + 1; i < BIT_Y; i++) {
        bit_xof256_state st_i;
        bit_xof256_init(&st_i, seed, BIT_SEEDBYTES, (uint16_t)(*nonce + (i - BIT_L - 1)));
        int rej = reject_sample_coeffs(&z->vec[i], c_e->vec[i - BIT_L - 1].coeffs, BIT_GAMMA1_2, BIT_BETA, &st_i);
        bit_xof256_zeroize(&st_i);
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
