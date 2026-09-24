/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
#include "poly.h"
#include "ntt.h"
#include "symmetric.h"
#include <stddef.h>

static const int16_t map_S1[4] = {- 1, 0, 1, 0};

static size_t rej_S1(int16_t *a, size_t len, const uint8_t *buf, size_t buflen) {
    size_t ctr = 0;
    size_t pos = 0;
    uint8_t t0, t1, t2, t3;

    while (ctr < len && pos < buflen) {
        t0 = buf[pos] & 0x03;
        t1 = (buf[pos] >> 2) & 0x03;
        t2 = (buf[pos] >> 4) & 0x03;
        t3 = (buf[pos] >> 6) & 0x03;
        pos++;

        if (t0 < 3 && ctr < len) a[ctr++] = map_S1[t0];
        if (t1 < 3 && ctr < len) a[ctr++] = map_S1[t1];
        if (t2 < 3 && ctr < len) a[ctr++] = map_S1[t2];
        if (t3 < 3 && ctr < len) a[ctr++] = map_S1[t3];
    }
    return ctr;
}

inline void decompose_hint(int16_t *highbits, int16_t r) {
    uint32_t val = (uint32_t)reduce_to_unsigned(r);
    uint32_t numerator = val + (BIT_GAMMA2 >> 1);
    int32_t hb = (int32_t)(((uint64_t)numerator * BIT_GAMMA2_BARRETT_MULT) >> BIT_GAMMA2_BARRETT_SHIFT);

    int32_t edgecase = (ALPHA_HINT - (hb + 1)) >> 31;
    hb -= ALPHA_HINT & edgecase;
    *highbits = (int16_t)hb;
}

static int16_t decompose_b_coeff(int16_t *lowbits, int16_t r) {
    int32_t a = reduce_to_unsigned(r);
    int32_t highbits = (a + (BIT_GAMMA_B >> 1)) >> BIT_GAMMA_B_BITS;
    int32_t low;

    low = a - highbits * BIT_GAMMA_B;
    low -= BIT_Q & (((BIT_Q - 1) / 2 - low) >> 31);

    *lowbits = (int16_t)low;
    return (int16_t)highbits;
}

uint16_t reduce_barrett(int32_t a) {
    int32_t t;
    t = (int32_t)(((int64_t)a * BIT_BARRETT_MULT) >> BIT_BARRETT_SHIFT);
    a -= t * BIT_Q;
    /* Barrett estimate may be off by one; fold back into [0, q) */
    a += (a >> 31) & BIT_Q;
    a -= BIT_Q;
    a += (a >> 31) & BIT_Q;
    return (uint16_t)a;
}

void poly_sample_S1(poly *a, const uint8_t *seed, uint16_t *nonce) {
    uint8_t buf[BIT_SAMPLE_S1_BUFLEN];
    bit_xof256_state st;
    size_t ctr;

    bit_xof256_init(&st, seed, BIT_SEEDBYTES, *nonce);
    bit_xof256_squeeze(&st, buf, sizeof(buf));
    ctr = rej_S1(a->coeffs, BIT_N, buf, sizeof(buf));

    while (ctr < BIT_N) {
        bit_xof256_squeeze(&st, buf, sizeof(buf));
        ctr += rej_S1(a->coeffs + ctr, BIT_N - ctr, buf, sizeof(buf));
    }
    *nonce = (uint16_t)(*nonce + 1U);
    bit_xof256_zeroize(&st);
}

void poly_to_ntt(poly_ntt *result, const poly *a) {
    for (int i = 0; i < BIT_N; i++) {
        result->coeffs[i] = (int16_t)reduce_to_unsigned(a->coeffs[i]);
    }
    ntt_forward(result->coeffs);
}

void poly_from_ntt(poly *result, const poly_ntt *a) {
    poly_ntt tmp = *a;

    ntt_inverse(tmp.coeffs);
    for (int i = 0; i < BIT_N; i++) {
        result->coeffs[i] = reduce_to_signed((uint16_t)tmp.coeffs[i]);
    }
}

void poly_ntt_basemul_raw(poly_ntt *result, const poly_ntt *a, const poly_ntt *b) {
    ntt_basemul_raw(result->coeffs, a->coeffs, b->coeffs);
}

void poly_ntt_basemul_acc_raw(poly_ntt *result, const poly_ntt *a, const poly_ntt *b) {
    ntt_basemul_acc_raw(result->coeffs, a->coeffs, b->coeffs);
}

void poly_ntt_montgomery_lift(poly_ntt *result) {
    ntt_montgomery_lift(result->coeffs);
}

void poly_ntt_zero(poly_ntt *result) {
    for (int i = 0; i < BIT_N; i++) {
        result->coeffs[i] = 0;
    }
}

void poly_add(poly *result, const poly *a, const poly *b) {
    for(int i = 0; i < BIT_N; i++) {
        result->coeffs[i] = reduce_signed_modq((int32_t)a->coeffs[i] + b->coeffs[i]);
    }
}

void poly_add_eta1(poly *result, const poly *a, const poly *eta1) {
    for(int i = 0; i < BIT_N; i++) {
        int32_t val = (int32_t)a->coeffs[i] + eta1->coeffs[i];

        val += BIT_Q & ((val + BIT_Q_HALF) >> 31);
        val -= BIT_Q & ((BIT_Q_HALF - val) >> 31);
        result->coeffs[i] = (int16_t)val;
    }
}

void poly_sub(poly *result, const poly *a, const poly *b) {
    for (int i = 0; i < BIT_N; i++) {
        result->coeffs[i] = reduce_signed_modq((int32_t)a->coeffs[i] - b->coeffs[i]);
    }
}

void poly_highbits(poly *w1, const poly *w) {
    for (int i = 0; i < BIT_N; i++) {
        decompose_hint(&w1->coeffs[i], w->coeffs[i]);
    }
}


void poly_decompose_b(poly *b1, poly *b0, const poly *b) {
    for (int i = 0; i < BIT_N; i++) {
        b1->coeffs[i] = decompose_b_coeff(&b0->coeffs[i], b->coeffs[i]);
    }
}

void poly_pack_w1(unsigned char *r, const poly *a) {
    for (int i = 0; i < BIT_N / 8; i++) {

        uint16_t t0 = a->coeffs[8 * i + 0] & 0x3F;
        uint16_t t1 = a->coeffs[8 * i + 1] & 0x3F;
        uint16_t t2 = a->coeffs[8 * i + 2] & 0x3F;
        uint16_t t3 = a->coeffs[8 * i + 3] & 0x3F;
        uint16_t t4 = a->coeffs[8 * i + 4] & 0x3F;
        uint16_t t5 = a->coeffs[8 * i + 5] & 0x3F;
        uint16_t t6 = a->coeffs[8 * i + 6] & 0x3F;
        uint16_t t7 = a->coeffs[8 * i + 7] & 0x3F;

        r[6 * i + 0] = (unsigned char)( t0        | (t1 << 6));
        r[6 * i + 1] = (unsigned char)((t1 >> 2)  | (t2 << 4));
        r[6 * i + 2] = (unsigned char)((t2 >> 4)  | (t3 << 2));
        r[6 * i + 3] = (unsigned char)( t4        | (t5 << 6));
        r[6 * i + 4] = (unsigned char)((t5 >> 2)  | (t6 << 4));
        r[6 * i + 5] = (unsigned char)((t6 >> 4)  | (t7 << 2));

    }
}

void poly_challenge(poly *c, const unsigned char seed[BIT_CHALLENGEBYTES]) {
    unsigned int i, b, pos;
    uint8_t buf[BIT_XOF256_RATE];
    bit_xof256_ctx ctx;

    bit_xof256_ctx_init(&ctx, seed, BIT_CHALLENGEBYTES);
    bit_xof256_ctx_squeezeblocks(&ctx, buf, 1);

    pos = 0;

    for(i = 0; i < BIT_N; ++i) {
        c->coeffs[i] = 0;
    }

    for(i = BIT_N - BIT_TAU; i < BIT_N; ++i) {
        do {
            if (pos >= BIT_XOF256_RATE) {
                bit_xof256_ctx_squeezeblocks(&ctx, buf, 1);
                pos = 0;
            }

            b = buf[pos++];
        } while(b > i);

        c->coeffs[i] = c->coeffs[b];
        c->coeffs[b] = 1;  /* {0,1} challenge: no negative signs */
    }
    bit_xof256_ctx_zeroize(&ctx);
}

void poly_challenge_to_sparse(sparse_challenge *s, const poly *c) {
    int ctr = 0;

    for (int i = 0; i < BIT_N; i++) {
        int16_t coeff = c->coeffs[i];
        if (coeff != 0) {
            s->pos[ctr] = (uint8_t)i;
            s->sign[ctr] = (int8_t)coeff;
            ctr++;
            if (ctr == BIT_TAU) {
                break;
            }
        }
    }
}

/* Constant-time sparse challenge multiply: accumulate negacyclic shifts,
 * then apply the sign (−1)^b branch-free via arithmetic mask. */
void poly_mul_challenge_no_reduce(poly *result, const poly *a, const sparse_challenge *c) {
    int16_t acc[BIT_N];

    for(int i = 0; i < BIT_N; i++) {
        acc[i] = 0;
    }

    for(int t = 0; t < BIT_TAU; t++) {
        int i = c->pos[t];
        for(int j = 0; j < BIT_N - i; j++) {
            acc[i + j] += a->coeffs[j];
        }
        for(int j = BIT_N - i; j < BIT_N; j++) {
            acc[i + j - BIT_N] -= a->coeffs[j];
        }
    }

    const int16_t smask = (int16_t)((int32_t)c->sign[0] >> 31);  /* 0 if s=+1, −1 if s=−1 */
    for(int i = 0; i < BIT_N; i++) {
        result->coeffs[i] = (int16_t)((acc[i] ^ smask) - smask);
    }
}

void poly_cneg(poly *a, uint8_t b_val) {

    int32_t mask = -((int32_t)b_val); 

    for(int i = 0; i < BIT_N; i++) {
        a->coeffs[i] = (a->coeffs[i] ^ mask) - mask;
    }
}


int poly_check_reject_highbits_w1_sparse(const poly *w1, const poly *w0, const sparse_challenge *c) {
    uint32_t reject_flag = 0;

    for (int t = 0; t < BIT_TAU; t++) {
        int i = c->pos[t];
        int16_t hb;
        int32_t w_prime_val = (int32_t)w0->coeffs[i] + c->sign[t];

        decompose_hint(&hb, w_prime_val);
        reject_flag |= (uint32_t)((uint16_t)w1->coeffs[i] ^ (uint16_t)hb);
    }

    return (reject_flag != 0) ? 1 : 0;
}
