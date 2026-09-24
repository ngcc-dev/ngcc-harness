/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense, Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
*/
#include "polyvec.h"
#include "params.h"
#include "sample.h"
#include "endian.h"
#include "symmetric.h"
#include "align.h"
#include <stddef.h>
#include <string.h>

void polyvecl_sample_S1(polyvecl *v, const uint8_t *seed, uint16_t *nonce) {
    for (size_t i = 0; i < BIT_L; i++)
        poly_sample_S1(&v->vec[i], seed, nonce);
}

void polyveck_sample_S1(polyveck *v, const uint8_t *seed, uint16_t *nonce) {
    for (size_t i = 0; i < BIT_K; i++)
        poly_sample_S1(&v->vec[i], seed, nonce);
}

#define MATRIX_XOF_BUFFER_SIZE 672

BIT_ALIGN_64
static void poly_uniform_ntt(poly_ntt *a, const unsigned char *seed, size_t row, size_t col) {
    unsigned char buf[MATRIX_XOF_BUFFER_SIZE];
    unsigned char msg[BIT_SEEDBYTES + 2];
    bit_xof128_state st;
    size_t ctr = 0;

    memcpy(msg, seed, BIT_SEEDBYTES);
    msg[BIT_SEEDBYTES]     = (unsigned char)col;
    msg[BIT_SEEDBYTES + 1] = (unsigned char)row;

    bit_xof128_init(&st, msg, sizeof(msg));
    while (ctr < BIT_N) {
        size_t pos = 0;

        bit_xof128_squeeze(&st, buf, sizeof(buf));
        while (ctr < BIT_N && pos + 2 < sizeof(buf)) {
            uint32_t val = load24_le(buf + pos) & 0x7FFFF;

            pos += 3;
            if (val < BIT_Q) {
                a->coeffs[ctr++] = (int32_t)val;
            }
        }
    }
    bit_xof128_zeroize(&st);
}

void poly_matrix_expand_ntt(poly_matrix_ntt *A_ntt, const unsigned char *seed) {
    for (size_t i = 0; i < BIT_K; ++i) {
        for (size_t j = 0; j < BIT_L; ++j) {
            poly_uniform_ntt(&A_ntt->vec[i].vec[j], seed, i, j);
        }
    }
}

void polyvecl_to_ntt(polyvecl_ntt *result, const polyvecl *v) {
    for (int i = 0; i < BIT_L; i++) {
        poly_to_ntt(&result->vec[i], &v->vec[i]);
    }
}

void polyvecy_y1_to_ntt(polyvecl_ntt *result, const polyvecy *y) {
    for (int i = 0; i < BIT_L; i++) {
        poly_to_ntt(&result->vec[i], &y->vec[1 + i]);
    }
}

void polyveck_from_ntt(polyveck *result, const polyveck_ntt *v_ntt) {
    for (int i = 0; i < BIT_K; i++) {
        poly_from_ntt(&result->vec[i], &v_ntt->vec[i]);
    }
}

void poly_matrix_mul_vector_ntt(polyveck *result, const poly_matrix_ntt *A_ntt, const polyvecl_ntt *v_ntt) {
    polyveck_ntt acc;

    for (int i = 0; i < BIT_K; i++) {
        poly_ntt_zero(&acc.vec[i]);
        for (int j = 0; j < BIT_L; j++) {
            poly_ntt_acc_mul_raw(&acc.vec[i], &A_ntt->vec[i].vec[j], &v_ntt->vec[j]);
        }
        poly_ntt_montgomery_lift(&acc.vec[i]);
    }

    polyveck_from_ntt(result, &acc);
}

void polyveck_add_eta1(polyveck *result, const polyveck *a, const polyveck *eta1) {
    for (int i = 0; i < BIT_K; i++) {
        poly_add_eta1(&result->vec[i], &a->vec[i], &eta1->vec[i]);
    }
}

void polyveck_decompose_b(polyveck *b1, polyveck *b0, const polyveck *b) {
    for (int i = 0; i < BIT_K; i++) {
        poly_decompose_b(&b1->vec[i], &b0->vec[i], &b->vec[i]);
    }
}



void polyveck_b1_scaled_to_ntt(polyveck_ntt *b1_ntt, const polyveck *b1) {
    poly tmp;

    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            tmp.coeffs[j] = reduce_signed_modq((int64_t)b1->vec[i].coeffs[j] * BIT_GAMMA_B);
        }
        poly_to_ntt(&b1_ntt->vec[i], &tmp);
    }
}

void polyveck_compute_w_ntt(polyveck *w, const poly_matrix_ntt *A_0_ntt, const polyveck_ntt *b_ntt, const polyvecl_ntt *y1_ntt, const poly_ntt *y0_ntt, const polyvecy *y) {
    polyveck_ntt w_ntt;
    poly_ntt tmp_ntt;

    for (int i = 0; i < BIT_K; i++) {
        poly_ntt_zero(&w_ntt.vec[i]);
        for (int j = 0; j < BIT_L; j++) {
            poly_ntt_acc_mul_raw(&w_ntt.vec[i], &A_0_ntt->vec[i].vec[j], &y1_ntt->vec[j]);
        }
        poly_ntt_mul_raw(&tmp_ntt, &b_ntt->vec[i], y0_ntt);
        for (int j = 0; j < BIT_N; j++) {
            w_ntt.vec[i].coeffs[j] = reduce_modq((int32_t)(w_ntt.vec[i].coeffs[j] - tmp_ntt.coeffs[j] + BIT_Q));
        }
        poly_ntt_montgomery_lift(&w_ntt.vec[i]);
    }
    polyveck_from_ntt(w, &w_ntt);

    for (int j = 0; j < BIT_N; j++) {
        int32_t val = w->vec[0].coeffs[j];
        int32_t q_hat_y0 = (int32_t)y->vec[0].coeffs[j] * BIT_Q_HAT_MINUS;

        w->vec[0].coeffs[j] = reduce_signed_barrett(val + q_hat_y0);
    }

    for (int i = 0; i < BIT_K; i++) {
        poly_add(&w->vec[i], &w->vec[i], &y->vec[1 + BIT_L + i]);
    }

}

void polyvecy_sample_triangular(polyvecy *y, const uint8_t *seed_y, uint16_t *nonce) {

    poly_sample_triangular_eta0(y->vec[0].coeffs, seed_y, nonce);
    for (int i = 1; i <= BIT_L; i++) {
        poly_sample_triangular(y->vec[i].coeffs, seed_y, nonce);         /* y1: 13-bit */
    }
    for (int i = BIT_L + 1; i < BIT_Y; i++) {
        poly_sample_triangular_15bit(y->vec[i].coeffs, seed_y, nonce);   /* y2: 15-bit */
    }
}

void polyvecy_sample_y1(polyvecy *y, const uint8_t *seed_y, uint16_t *nonce) {
    for (int i = 1; i <= BIT_L; i++) {
        poly_sample_triangular(y->vec[i].coeffs, seed_y, nonce);
    }
}

void polyvecy_sample_y0(polyvecy *y, const uint8_t *seed_y, uint16_t *nonce) {
    poly_sample_triangular_eta0(y->vec[0].coeffs, seed_y, nonce);
}

void polyveck_highbits(polyveck *w1, const polyveck *w) {
    for (int i = 0; i < BIT_K; i++) {
        poly_highbits(&w1->vec[i], &w->vec[i]);
    }
}

void polyveck_pack_w1(unsigned char *r, const polyveck *w1) {
    for (int i = 0; i < BIT_K; i++) {
        poly_pack_w1(r + i * BIT_POLY_W1_BYTES, &w1->vec[i]);
    }
}

void polyvecl_mul_challenge_no_reduce(polyvecl *result, const polyvecl *a, const sparse_challenge *c) {
    for (int i = 0; i < BIT_L; i++) {
        poly_mul_challenge_no_reduce(&result->vec[i], &a->vec[i], c);
    }
}

void polyveck_mul_challenge_no_reduce(polyveck *result, const polyveck *a, const sparse_challenge *c) {
    for (int i = 0; i < BIT_K; i++) {
        poly_mul_challenge_no_reduce(&result->vec[i], &a->vec[i], c);
    }
}



void polyveck_make_hint_compressed(polyveck *h, const polyveck *w1, const polyveck *w, const polyveck *z2, const polyveck *b0c, const poly *c, uint8_t b) {
    int32_t c_mask = -(int32_t)(1 - b);

    for (int j = 0; j < BIT_N; j++) {
        int32_t w_hat_val = (int32_t)w->vec[0].coeffs[j] - z2->vec[0].coeffs[j];
        w_hat_val += (int32_t)c->coeffs[j] & c_mask;
        w_hat_val += b0c->vec[0].coeffs[j];

        int32_t hb_w_hat;
        decompose_hint(&hb_w_hat, w_hat_val);

        int32_t h_val = (int32_t)w1->vec[0].coeffs[j] - hb_w_hat;
        h_val += (h_val >> 31) & ALPHA_HINT;
        int32_t over_half = (ALPHA_HINT / 2 - h_val) >> 31;
        h_val -= ALPHA_HINT & over_half;

        h->vec[0].coeffs[j] = h_val;
    }
    for (int i = 1; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int32_t w_hat_val = (int32_t)w->vec[i].coeffs[j] - z2->vec[i].coeffs[j];
            w_hat_val += b0c->vec[i].coeffs[j];

            int32_t hb_w_hat;
            decompose_hint(&hb_w_hat, w_hat_val);

            int32_t h_val = (int32_t)w1->vec[i].coeffs[j] - hb_w_hat;
            h_val += (h_val >> 31) & ALPHA_HINT;
            int32_t over_half = (ALPHA_HINT / 2 - h_val) >> 31;
            h_val -= ALPHA_HINT & over_half;

            h->vec[i].coeffs[j] = h_val;
        }
    }
}

void polyveck_compute_w_hat_prime_ntt(polyveck *w_hat_prime, const poly_matrix_ntt *A_0_ntt, const polyveck_ntt *b1_ntt, const polyvecl_ntt *z1_tail_ntt, const poly_ntt *z0_ntt, const poly *z0, const poly *c_poly) {
    polyveck_ntt acc;
    poly_ntt tmp_ntt;

    for (int i = 0; i < BIT_K; i++) {
        poly_ntt_zero(&acc.vec[i]);
        for (int j = 0; j < BIT_L; j++) {
            poly_ntt_acc_mul_raw(&acc.vec[i], &A_0_ntt->vec[i].vec[j], &z1_tail_ntt->vec[j]);
        }
        poly_ntt_mul_raw(&tmp_ntt, &b1_ntt->vec[i], z0_ntt);
        for (int j = 0; j < BIT_N; j++) {
            acc.vec[i].coeffs[j] = reduce_modq((int32_t)(acc.vec[i].coeffs[j] - tmp_ntt.coeffs[j] + BIT_Q));
        }
        poly_ntt_montgomery_lift(&acc.vec[i]);
    }

    polyveck_from_ntt(w_hat_prime, &acc);

    for (int j = 0; j < BIT_N; j++) {
        int32_t val = w_hat_prime->vec[0].coeffs[j];
        int32_t z0_c_sum = (int32_t)z0->coeffs[j] + c_poly->coeffs[j];

        val += z0_c_sum * BIT_Q_HAT;
        w_hat_prime->vec[0].coeffs[j] = reduce_signed_barrett(val);
    }
}

void polyveck_compute_w_prime(polyveck *w_prime, const polyveck *w_hat_prime, const polyveck *h) {
    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j++) {
            int32_t hb;
            decompose_hint(&hb, w_hat_prime->vec[i].coeffs[j]);
            int32_t val = (int32_t)hb + h->vec[i].coeffs[j];
            val += (val >> 31) & ALPHA_HINT;
            val -= ALPHA_HINT;
            val += (val >> 31) & ALPHA_HINT;
            
            w_prime->vec[i].coeffs[j] = val;
        }
    }
}

