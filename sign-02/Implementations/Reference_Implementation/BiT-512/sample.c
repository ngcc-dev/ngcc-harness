/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense, Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
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

/* 16-bit triangular: 4 bytes per 2 values per half (16+16=32 bits) */
#define BYTELEN_16BIT (2 * 4 * BIT_N / 4)
/* 15-bit triangular reads 4 bytes/2-coeffs from BOTH a lo and a hi half →
 * needs 4*BIT_N bytes (hi = buf + BYTELEN_15BIT/2).  The old (2*BIT_N) value
 * made the hi-half read run off the end of the buffer past coeff N/2. */
#define BYTELEN_15BIT (4 * BIT_N)

void poly_sample_triangular_15bit(int32_t *out, const uint8_t *seed, uint16_t *nonce)
{
    uint8_t buf[BYTELEN_15BIT];
    bit_xof256_nonce(buf, sizeof(buf), seed, BIT_SEEDBYTES, *nonce);
    *nonce = (uint16_t)(*nonce + 1U);

    for (int i = 0; i < BIT_N / 2; i++) {
        const uint8_t *lo = buf + 4 * i;
        const uint8_t *hi = buf + BYTELEN_15BIT / 2 + 4 * i;

        uint32_t lo0 = (uint32_t)lo[0] | ((uint32_t)lo[1] << 8);
        uint32_t lo1 = (uint32_t)lo[2] | ((uint32_t)lo[3] << 8);

        uint32_t hi0 = (uint32_t)hi[0] | ((uint32_t)hi[1] << 8);
        uint32_t hi1 = (uint32_t)hi[2] | ((uint32_t)hi[3] << 8);

#define M15 0x7FFFU
        out[2*i+0] = (int32_t)(lo0 & M15) - (int32_t)(hi0 & M15);
        out[2*i+1] = (int32_t)(lo1 & M15) - (int32_t)(hi1 & M15);
#undef M15
    }
}

/* 14-bit triangular sampling: 4 bytes per 2 values */
void poly_sample_triangular(int32_t *out, const uint8_t *seed, uint16_t *nonce)
{
    unsigned int i;
    uint8_t buf[BYTELEN];

    bit_xof256_nonce(buf, sizeof(buf), seed, BIT_SEEDBYTES, *nonce);
    *nonce = (uint16_t)(*nonce + 1U);

    for (i = 0; i < BIT_N / 2; i++) {
        const uint8_t *lo = buf + 4 * i;
        const uint8_t *hi = buf + BYTELEN / 2 + 4 * i;

        uint32_t lo0 = (uint32_t)lo[0]        | ((uint32_t)(lo[1] & 0x1F) << 8);
        uint32_t lo1 = ((uint32_t)lo[1] >> 5) | ((uint32_t)lo[2] << 3) | ((uint32_t)(lo[3] & 0x03) << 11);

        uint32_t hi0 = (uint32_t)hi[0]        | ((uint32_t)(hi[1] & 0x1F) << 8);
        uint32_t hi1 = ((uint32_t)hi[1] >> 5) | ((uint32_t)hi[2] << 3) | ((uint32_t)(hi[3] & 0x03) << 11);

        out[2 * i + 0] = (int32_t)lo0 - (int32_t)hi0;
        out[2 * i + 1] = (int32_t)lo1 - (int32_t)hi1;
    }
}

/* 4-bit eta0 sampling: 2 bytes per 4 values */
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

        out[4 * i + 0] = (int32_t)(l0 & 0x0F)        - (int32_t)(h0 & 0x0F);
        out[4 * i + 1] = (int32_t)((l0 >> 4) & 0x0F) - (int32_t)((h0 >> 4) & 0x0F);
        out[4 * i + 2] = (int32_t)(l1 & 0x0F)        - (int32_t)(h1 & 0x0F);
        out[4 * i + 3] = (int32_t)((l1 >> 4) & 0x0F) - (int32_t)((h1 >> 4) & 0x0F);
    }
}

#define TWO_POW_24 16777216ULL

_Static_assert(BIT_GAMMA1 == 8192 && BIT_BETA == 115,
               "fast path in reject_sample uses constants derived for gamma1=8192,beta2=115");

static int reject_sample_coeffs(const poly *z,
                                const int32_t *v,
                                int32_t gamma1,
                                int32_t beta2,
                                bit_xof256_state *stream) {
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
            bit_xof256_squeeze(stream, r_buf, sizeof(r_buf));
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
                                       bit_xof256_state *stream) {
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
            bit_xof256_squeeze(stream, r_buf, sizeof(r_buf));
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
    bit_xof256_state stream;
    int ret;
    int32_t v[BIT_N];
    for (int i = 0; i < BIT_N; i++) v[i] = 1;

    bit_xof256_init(&stream, seed, BIT_SEEDBYTES, *nonce);
    ret = reject_sample_coeffs(z0, v, BIT_GAMMA1_0, 1, &stream);
    *nonce = (uint16_t)(*nonce + 1U);
    bit_xof256_zeroize(&stream);
    return ret;
}

/* Check rejection for z0 and z1 using independent per-polynomial XOF streams. */
int check_reject_sample_z0z1(const polyvecy *z, const polyvecl *c_s,
                              const poly *c_poly, const sparse_challenge *c_sparse,
                              const unsigned char *seed, uint16_t *nonce) {
    int global_reject = 0;

    for (int i = 1; i <= BIT_L; i++) {
        bit_xof256_state stream;
        bit_xof256_init(&stream, seed, BIT_SEEDBYTES, (uint16_t)(*nonce + i));
        int rej = reject_sample_coeffs(&z->vec[i], c_s->vec[i - 1].coeffs, BIT_GAMMA1, BIT_BETA, &stream);
        bit_xof256_zeroize(&stream);
        if (rej) { global_reject = 1; break; }
    }

    if (!global_reject) {
        bit_xof256_state stream0;
        bit_xof256_init(&stream0, seed, BIT_SEEDBYTES, (uint16_t)(*nonce + 0));
        int rej = reject_sample_coeffs_sparse(&z->vec[0], c_sparse, c_poly->coeffs, BIT_GAMMA1_0, 1, &stream0);
        bit_xof256_zeroize(&stream0);
        if (rej) global_reject = 1;
    }

    *nonce = (uint16_t)(*nonce + 4U);
    return global_reject;
}

int check_reject_sample_z2(const polyvecy *z, const polyveck *c_e, const unsigned char *seed, uint16_t *nonce) {
    int global_reject = 0;

    for (int i = BIT_L + 1; i < BIT_Y; i++) {
        bit_xof256_state stream;
        bit_xof256_init(&stream, seed, BIT_SEEDBYTES, (uint16_t)(*nonce + (i - BIT_L - 1)));
        int rej = reject_sample_coeffs(&z->vec[i], c_e->vec[i - BIT_L - 1].coeffs, BIT_GAMMA1_2, BIT_BETA, &stream);
        bit_xof256_zeroize(&stream);
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
            int32_t h_val = (int32_t)h->vec[i].coeffs[j];
            int32_t abs_h = ct_abs(h_val);

            int32_t gamma2_h = abs_h * BIT_GAMMA2;
            uint32_t over_bound = (uint32_t)(BIT_B_INF - gamma2_h) >> 31;
            reject_flag |= over_bound;
        }
    }

    return reject_flag != 0 ? 1 : 0;
}

int check_reject_hint_range(const polyveck *h) {
    /* base3+overflow: reject if |h| > 2 or overflow count exceeds limit */
    uint32_t reject_flag = 0;
    uint32_t overflow_count = 0;
    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int32_t abs_h = ct_abs(h->vec[i].coeffs[j]);
            reject_flag |= (uint32_t)(2 - abs_h) >> 31;       /* reject |h| > 2 */
            overflow_count += (uint32_t)(abs_h >> 1);          /* count |h| >= 2 */
        }
    }
    reject_flag |= (uint32_t)(BIT_H_OVERFLOW_MAX - overflow_count) >> 31;
    return (int)reject_flag;
}
