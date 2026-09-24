/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
*/
#ifndef POLYVEC_C
#define POLYVEC_C
#include "polyvec.h"
#include "align.h"
#include "params.h"
#include "sample.h"
#include "endian.h"
#include "symmetric.h"
#include <immintrin.h>
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

/* Batched S1: s_0[0..2] + e[0] via x4 XOF, e[1] scalar */
void polyvec_sample_S1_avx(polyvecl *s, polyveck *e,
                           const uint8_t *seed, uint16_t *nonce) {
#if BIT_USE_SHAKE
    poly_sample_S1_x4(&s->vec[0], &s->vec[1], &s->vec[2], &e->vec[0],
                      seed, nonce);
    poly_sample_S1_x2(&e->vec[1], &e->vec[2], seed, nonce);
#else
    /* SM3: x4/x2 prefetch (one-shot pseudoXOF over seed||nonce) differs from the
       reference stateful stream (seed||nonce||block); SM3 has no 4-way batching
       anyway, so use the exact scalar/stateful path the reference uses. */
    polyvecl_sample_S1(s, seed, nonce);
    polyveck_sample_S1(e, seed, nonce);
#endif
}

/* Accept rate 520193/524288 ≈ 99.2%.  Each 24-bit load needs 3 bytes.
   BIT_N=1024 requires ~3096 bytes → ~19 SHAKE128 blocks per polynomial.
   With 16 blocks (2688 B) initial pre-fetch we get ~889 accepted coefficients;
   2-3 additional squeeze rounds finish the remaining 135. */
#define MATRIX_XOF_BLOCKS 16
#if BIT_USE_SHAKE
#define MATRIX_XOF_BUFFER_SIZE (MATRIX_XOF_BLOCKS * SHAKE128_RATE)
#else
/* SM3: bit_xof128_squeeze is a block-indexed pseudoXOF (input msg||0000||block),
   so the per-squeeze chunk size is part of the KAT and must equal the
   reference's 4*168 = 672 bytes. */
#define MATRIX_XOF_BUFFER_SIZE 672
#endif

/* Scalar uniform rejection: reads 24-bit values, keeps those < BIT_Q */
static size_t rej_uniform(int32_t *a, size_t ctr, size_t len,
                          const uint8_t *buf, size_t buflen) {
    size_t pos = 0;
    while (ctr < len && pos + 2 < buflen) {
        uint32_t val = load24_le(buf + pos) & 0x7FFFF;
        pos += 3;
        if (val < BIT_Q) a[ctr++] = (int32_t)val;
    }
    return ctr;
}

/* 4-way unrolled rejection: 12 bytes → 4 candidates.
   Acceptance rate 99.2% → the branch is almost always taken. */
static size_t rej_uniform_avx(int32_t *a, size_t ctr, size_t len,
                              const uint8_t *buf, size_t buflen) {
    size_t pos = 0;
    /* Batch 4 values at a time while output has ≥4 slots */
    while (ctr <= len - 4 && pos + 11 < buflen) {
        uint32_t v0 = load24_le(buf + pos)      & 0x7FFFF;
        uint32_t v1 = load24_le(buf + pos + 3)  & 0x7FFFF;
        uint32_t v2 = load24_le(buf + pos + 6)  & 0x7FFFF;
        uint32_t v3 = load24_le(buf + pos + 9)  & 0x7FFFF;
        pos += 12;
        if (v0 < BIT_Q) a[ctr++] = (int32_t)v0;
        if (v1 < BIT_Q) a[ctr++] = (int32_t)v1;
        if (v2 < BIT_Q) a[ctr++] = (int32_t)v2;
        if (v3 < BIT_Q) a[ctr++] = (int32_t)v3;
    }
    /* Scalar tail */
    return rej_uniform(a, ctr, len, buf + pos, buflen - pos);
}

#if BIT_USE_SHAKE
/* 4-way parallel uniform NTT-domain sampling (SHAKE128 x4).
   Pre-fetches 16 blocks per polynomial, then refills as needed. */
static void poly_uniform_ntt_x4(poly_ntt *a0, poly_ntt *a1,
                                poly_ntt *a2, poly_ntt *a3,
                                const unsigned char *seed,
                                uint8_t r0, uint8_t c0,
                                uint8_t r1, uint8_t c1,
                                uint8_t r2, uint8_t c2,
                                uint8_t r3, uint8_t c3)
{
    uint8_t ALIGNED_32 buf0[MATRIX_XOF_BUFFER_SIZE];
    uint8_t ALIGNED_32 buf1[MATRIX_XOF_BUFFER_SIZE];
    uint8_t ALIGNED_32 buf2[MATRIX_XOF_BUFFER_SIZE];
    uint8_t ALIGNED_32 buf3[MATRIX_XOF_BUFFER_SIZE];

    /* extseed = seed || col || row  (BIT_SEEDBYTES + 2 bytes) */
    uint8_t ext0[BIT_SEEDBYTES + 2], ext1[BIT_SEEDBYTES + 2];
    uint8_t ext2[BIT_SEEDBYTES + 2], ext3[BIT_SEEDBYTES + 2];

    memcpy(ext0, seed, BIT_SEEDBYTES); ext0[BIT_SEEDBYTES] = c0; ext0[BIT_SEEDBYTES + 1] = r0;
    memcpy(ext1, seed, BIT_SEEDBYTES); ext1[BIT_SEEDBYTES] = c1; ext1[BIT_SEEDBYTES + 1] = r1;
    memcpy(ext2, seed, BIT_SEEDBYTES); ext2[BIT_SEEDBYTES] = c2; ext2[BIT_SEEDBYTES + 1] = r2;
    memcpy(ext3, seed, BIT_SEEDBYTES); ext3[BIT_SEEDBYTES] = c3; ext3[BIT_SEEDBYTES + 1] = r3;

    bit_xof128x4_state x4s;
    bit_xof128x4_init(&x4s, ext0, ext1, ext2, ext3, BIT_SEEDBYTES + 2);
    bit_xof128x4_squeezeblocks(&x4s, buf0, buf1, buf2, buf3, MATRIX_XOF_BLOCKS);

    size_t ctr0 = rej_uniform_avx(a0->coeffs, 0, BIT_N, buf0, MATRIX_XOF_BUFFER_SIZE);
    size_t ctr1 = rej_uniform_avx(a1->coeffs, 0, BIT_N, buf1, MATRIX_XOF_BUFFER_SIZE);
    size_t ctr2 = rej_uniform_avx(a2->coeffs, 0, BIT_N, buf2, MATRIX_XOF_BUFFER_SIZE);
    size_t ctr3 = rej_uniform_avx(a3->coeffs, 0, BIT_N, buf3, MATRIX_XOF_BUFFER_SIZE);

    while (ctr0 < BIT_N || ctr1 < BIT_N || ctr2 < BIT_N || ctr3 < BIT_N) {
        bit_xof128x4_squeezeblocks(&x4s, buf0, buf1, buf2, buf3, 1);
        if (ctr0 < BIT_N) ctr0 = rej_uniform_avx(a0->coeffs, ctr0, BIT_N, buf0, SHAKE128_RATE);
        if (ctr1 < BIT_N) ctr1 = rej_uniform_avx(a1->coeffs, ctr1, BIT_N, buf1, SHAKE128_RATE);
        if (ctr2 < BIT_N) ctr2 = rej_uniform_avx(a2->coeffs, ctr2, BIT_N, buf2, SHAKE128_RATE);
        if (ctr3 < BIT_N) ctr3 = rej_uniform_avx(a3->coeffs, ctr3, BIT_N, buf3, SHAKE128_RATE);
    }
}
#endif /* BIT_USE_SHAKE */

/* Matrix expansion: K×L = 3×3 = 9 polys → x4 + x2 */
void poly_matrix_expand_ntt(poly_matrix_ntt *A_ntt, const unsigned char *seed)
{
#if BIT_USE_SHAKE
    poly_uniform_ntt_x4(&A_ntt->vec[0].vec[0],
                        &A_ntt->vec[0].vec[1],
                        &A_ntt->vec[0].vec[2],
                        &A_ntt->vec[1].vec[0],
                        seed, 0,0, 0,1, 0,2, 1,0);

    poly_uniform_ntt_x4(&A_ntt->vec[1].vec[1],
                        &A_ntt->vec[1].vec[2],
                        &A_ntt->vec[2].vec[0],
                        &A_ntt->vec[2].vec[1],
                        seed, 1,1, 1,2, 2,0, 2,1);

    {   /* (2,2) scalar */
        unsigned char buf[MATRIX_XOF_BUFFER_SIZE];
        unsigned char msg[BIT_SEEDBYTES + 2];
        bit_xof128_state st;
        size_t ctr = 0;
        memcpy(msg, seed, BIT_SEEDBYTES);
        msg[BIT_SEEDBYTES]     = 2;
        msg[BIT_SEEDBYTES + 1] = 2;
        bit_xof128_init(&st, msg, sizeof(msg));
        while (ctr < BIT_N) {
            bit_xof128_squeeze(&st, buf, sizeof(buf));
            ctr = rej_uniform(A_ntt->vec[2].vec[2].coeffs, ctr, BIT_N, buf, sizeof(buf));
        }
        bit_xof128_zeroize(&st);
    }
#else
    /* SM3 scalar path */
    for (size_t i = 0; i < BIT_K; ++i) {
        for (size_t j = 0; j < BIT_L; ++j) {
            unsigned char buf[MATRIX_XOF_BUFFER_SIZE];
            unsigned char msg[BIT_SEEDBYTES + 2];
            bit_xof128_state st;
            size_t ctr = 0;

            memcpy(msg, seed, BIT_SEEDBYTES);
            msg[BIT_SEEDBYTES]     = (unsigned char)j;
            msg[BIT_SEEDBYTES + 1] = (unsigned char)i;

            bit_xof128_init(&st, msg, sizeof(msg));
            while (ctr < BIT_N) {
                bit_xof128_squeeze(&st, buf, sizeof(buf));
                ctr = rej_uniform(A_ntt->vec[i].vec[j].coeffs, ctr, BIT_N, buf, sizeof(buf));
            }
            bit_xof128_zeroize(&st);
        }
    }
#endif
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

void polyveck_to_ntt(polyveck_ntt *result, const polyveck *v) {
    for (int i = 0; i < BIT_K; i++) {
        poly_to_ntt(&result->vec[i], &v->vec[i]);
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

void polyveck_add(polyveck *result, const polyveck *a, const polyveck *b) {
    for (int i = 0; i < BIT_K; i++) {
        poly_add(&result->vec[i], &a->vec[i], &b->vec[i]);
    }
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
    for (int i = 1; i < BIT_Y; i++) {
        poly_sample_triangular(y->vec[i].coeffs, seed_y, nonce);
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



/* AVX2 hint decomposition: like ref's decompose_hint but for 8×int32 lanes */
static inline __m256i mm256_div_g2(__m256i v) {
    const __m256i v_g2h  = _mm256_set1_epi32((int32_t)(BIT_GAMMA2 >> 1));
    const __m256i v_mult = _mm256_set1_epi32((int32_t)BIT_GAMMA2_BARRETT_MULT);
    v = _mm256_add_epi32(v, v_g2h);
    __m256i even = _mm256_mul_epu32(v, v_mult);
    __m256i odd  = _mm256_mul_epu32(_mm256_srli_epi64(v, 32), v_mult);
    even = _mm256_srli_epi64(even, BIT_GAMMA2_BARRETT_SHIFT);
    odd  = _mm256_srli_epi64(odd,  BIT_GAMMA2_BARRETT_SHIFT);
    return _mm256_blend_epi32(even, _mm256_slli_epi64(odd, 32), 0xAA);
}

static inline __m256i mm256_decompose_hint_avx(__m256i v_r) {
    __m256i v_q = _mm256_set1_epi32(BIT_Q);
    __m256i sign = _mm256_srai_epi32(v_r, 31);
    __m256i v_val = _mm256_add_epi32(v_r, _mm256_and_si256(sign, v_q));
    __m256i v_hb = mm256_div_g2(v_val);
    __m256i v_hb1 = _mm256_add_epi32(v_hb, _mm256_set1_epi32(1));
    __m256i v_diff = _mm256_sub_epi32(_mm256_set1_epi32(ALPHA_HINT), v_hb1);
    v_hb = _mm256_sub_epi32(v_hb,
        _mm256_and_si256(_mm256_set1_epi32(ALPHA_HINT), _mm256_srai_epi32(v_diff, 31)));
    return v_hb;
}

void polyveck_make_hint_compressed(polyveck *h, const polyveck *w1, const polyveck *w,
                                    const polyveck *z2, const polyveck *b0c,
                                    const poly *c, uint8_t b) {
    __m256i v_alpha      = _mm256_set1_epi32(ALPHA_HINT);
    __m256i v_alpha_half = _mm256_set1_epi32(ALPHA_HINT / 2);

    for (int i = 0; i < BIT_K; i++) {
        __m256i v_c_mask = (i == 0) ? _mm256_set1_epi32(-(int32_t)(1 - b))
                                    : _mm256_setzero_si256();

        for (int j = 0; j < BIT_N; j += 8) {
            __m256i w_v   = _mm256_load_si256((__m256i*)&w->vec[i].coeffs[j]);
            __m256i z2_v  = _mm256_load_si256((__m256i*)&z2->vec[i].coeffs[j]);
            __m256i b0c_v = _mm256_load_si256((__m256i*)&b0c->vec[i].coeffs[j]);
            __m256i c_v   = _mm256_loadu_si256((__m256i*)&c->coeffs[j]);

            __m256i what = _mm256_add_epi32(
                _mm256_add_epi32(_mm256_sub_epi32(w_v, z2_v), b0c_v),
                _mm256_and_si256(c_v, v_c_mask));

            __m256i hb = mm256_decompose_hint_avx(what);

            __m256i w1_v = _mm256_load_si256((__m256i*)&w1->vec[i].coeffs[j]);
            __m256i h_val = _mm256_sub_epi32(w1_v, hb);

            h_val = _mm256_add_epi32(h_val,
                _mm256_and_si256(_mm256_srai_epi32(h_val, 31), v_alpha));
            __m256i over = _mm256_sub_epi32(v_alpha_half, h_val);
            h_val = _mm256_sub_epi32(h_val,
                _mm256_and_si256(_mm256_srai_epi32(over, 31), v_alpha));

            _mm256_store_si256((__m256i*)&h->vec[i].coeffs[j], h_val);
        }
    }
}

void polyveck_compute_w_hat_prime_ntt(polyveck *w_hat_prime, const poly_matrix_ntt *A_0_ntt,
                                       const polyveck_ntt *b1_ntt, const polyvecl_ntt *z1_tail_ntt,
                                       const poly_ntt *z0_ntt, const poly *z0, const poly *c_poly) {
    polyveck_ntt acc;
    poly_ntt tmp_ntt;

    for (int i = 0; i < BIT_K; i++) {
        poly_ntt_zero(&acc.vec[i]);
        for (int j = 0; j < BIT_L; j++)
            poly_ntt_acc_mul_raw(&acc.vec[i], &A_0_ntt->vec[i].vec[j], &z1_tail_ntt->vec[j]);
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
    __m256i v_alpha = _mm256_set1_epi32(ALPHA_HINT);

    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j += 8) {
            __m256i wh = _mm256_load_si256((__m256i*)&w_hat_prime->vec[i].coeffs[j]);
            __m256i hv = _mm256_load_si256((__m256i*)&h->vec[i].coeffs[j]);

            __m256i hb  = mm256_decompose_hint_avx(wh);
            __m256i val = _mm256_add_epi32(hb, hv);

            val = _mm256_add_epi32(val, _mm256_and_si256(_mm256_srai_epi32(val, 31), v_alpha));
            val = _mm256_sub_epi32(val, v_alpha);
            val = _mm256_add_epi32(val, _mm256_and_si256(_mm256_srai_epi32(val, 31), v_alpha));

            _mm256_store_si256((__m256i*)&w_prime->vec[i].coeffs[j], val);
        }
    }
}

#endif
