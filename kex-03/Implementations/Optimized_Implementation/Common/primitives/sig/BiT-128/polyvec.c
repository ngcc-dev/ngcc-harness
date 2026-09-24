/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
*/
#ifndef POLYVEC_C
#define POLYVEC_C
#include "polyvec.h"
#include "params.h"
#include "ntt_avx.h"
#include "symmetric.h"
#include "endian.h"
#include "align.h"
#include "rej_table.h"
#include <immintrin.h>
#include <stddef.h>
#include <string.h>

// S1 sampler: BIT_L=3 (s) + BIT_K=3 (e) = 6 → x4 + x2
void polyvec_sample_S1_avx(polyvecl *s, polyveck *e,
                           const uint8_t *seed, uint16_t *nonce)
{
#if BIT_USE_SHAKE
    poly_sample_S1_x4(&s->vec[0], &s->vec[1], &s->vec[2], &e->vec[0],
                      seed, nonce);
    poly_sample_S1_x2(&e->vec[1], &e->vec[2], seed, nonce);
#else
    /* SM3: x4/x2 prefetch (one-shot pseudoXOF over seed||nonce) differs from the
       reference stateful stream (seed||nonce||block); SM3 has no 4-way batching
       anyway, so use the exact scalar/stateful path the reference uses. */
    for (int i = 0; i < BIT_L; i++) poly_sample_S1(&s->vec[i], seed, nonce);
    for (int i = 0; i < BIT_K; i++) poly_sample_S1(&e->vec[i], seed, nonce);
#endif
}

#define MATRIX_XOF_BLOCKS 4
#if BIT_USE_SHAKE
#define MATRIX_XOF_BUFFER_SIZE (MATRIX_XOF_BLOCKS * SHAKE128_RATE)
#else
#define MATRIX_XOF_BUFFER_SIZE 672
#endif

// =========================================================================
// Scalar tail: rejection-sample coefficients one-by-one from buf
// =========================================================================
static size_t rej_uniform(int16_t *a, size_t ctr, size_t len,
                          const uint8_t *buf, size_t buflen)
{
    size_t pos = 0;
    while (ctr < len && pos + 1 < buflen) {
        uint16_t val = load16_le(buf + pos) & 0x7FFFU;
        pos += 2;
        if (val < BIT_Q) {
            a[ctr++] = (int16_t)val;
        }
    }
    return ctr;
}

#if BIT_USE_SHAKE
// =========================================================================
// AVX2 rejection sampling using a shuffle dictionary.
// Processes 8 int16 values at once — the 8-bit accept/reject mask indexes
// a precomputed vpshufb pattern that compresses valid coefficients into a
// contiguous block for a single unaligned store.
// =========================================================================
static size_t rej_uniform_avx(int16_t *a, size_t ctr, size_t len,
                              const uint8_t *buf, size_t buflen)
{
    size_t pos = 0;
    const __m128i q_vec   = _mm_set1_epi16((short)BIT_Q);
    const __m128i mask15  = _mm_set1_epi16(0x7FFF);

    // Process full 8-element batches while output has ≥8 slots
    while (ctr <= len - 8 && pos <= buflen - 16) {
        __m128i in  = _mm_loadu_si128((const __m128i *)(buf + pos));
        pos += 16;

        __m128i val = _mm_and_si128(in, mask15);           // clear msb
        __m128i cmp = _mm_cmpgt_epi16(q_vec, val);         // 0xFFFF if val < Q
        __m128i pack = _mm_packs_epi16(cmp, cmp);          // compress to bytes
        int mask8 = _mm_movemask_epi8(pack) & 0xFF;        // 8-bit bitmap

        // Shuffle-dictionary lookup: compress in-flight
        __m128i shuf = _mm_load_si128(
            (const __m128i *)rej_uniform_table[mask8]);
        __m128i compressed = _mm_shuffle_epi8(val, shuf);
        _mm_storeu_si128((__m128i *)(a + ctr), compressed);

        ctr += __builtin_popcount(mask8);
    }

    // Scalar tail
    return rej_uniform(a, ctr, len, buf + pos, buflen - pos);
}

// =========================================================================
// 4-way parallel uniform NTT-domain sampling (SHAKE128 x4)
// =========================================================================
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

    // extseed = seed || row || col || nonce(0)
    // Must match bit_xof128_init which absorbs seed[18] || 0x00 (19 bytes)
    uint8_t extseed0[BIT_SEEDBYTES + 2];
    uint8_t extseed1[BIT_SEEDBYTES + 2];
    uint8_t extseed2[BIT_SEEDBYTES + 2];
    uint8_t extseed3[BIT_SEEDBYTES + 2];

    memcpy(extseed0, seed, BIT_SEEDBYTES); extseed0[BIT_SEEDBYTES]=c0; extseed0[BIT_SEEDBYTES+1]=r0;
    memcpy(extseed1, seed, BIT_SEEDBYTES); extseed1[BIT_SEEDBYTES]=c1; extseed1[BIT_SEEDBYTES+1]=r1;
    memcpy(extseed2, seed, BIT_SEEDBYTES); extseed2[BIT_SEEDBYTES]=c2; extseed2[BIT_SEEDBYTES+1]=r2;
    memcpy(extseed3, seed, BIT_SEEDBYTES); extseed3[BIT_SEEDBYTES]=c3; extseed3[BIT_SEEDBYTES+1]=r3;

    bit_xof128x4_state x4s;
    bit_xof128x4_init(&x4s, extseed0, extseed1, extseed2, extseed3,
                             BIT_SEEDBYTES + 2);
    bit_xof128x4_squeezeblocks(&x4s, buf0, buf1, buf2, buf3,
                                      MATRIX_XOF_BLOCKS);

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

/* Scalar uniform NTT-domain sampling for a single (row, col) polynomial.  */
__attribute__((aligned(64)))
static void poly_uniform_ntt(poly_ntt *a, const unsigned char *seed, size_t row, size_t col)
{
    unsigned char buf[MATRIX_XOF_BUFFER_SIZE];
    unsigned char msg[BIT_SEEDBYTES + 2];
    bit_xof128_state st;
    size_t ctr = 0;

    memcpy(msg, seed, BIT_SEEDBYTES);
    msg[BIT_SEEDBYTES]     = (unsigned char)col;
    msg[BIT_SEEDBYTES + 1] = (unsigned char)row;
    bit_xof128_init(&st, msg, sizeof(msg));
    while (ctr < BIT_N) {
        bit_xof128_squeeze(&st, buf, sizeof(buf));
        ctr = rej_uniform(a->coeffs, ctr, BIT_N, buf, sizeof(buf));
    }
    bit_xof128_zeroize(&st);
}

// =========================================================================
// Matrix expansion: K×L = 3×3 = 9 polynomials → x4 + x4 + x1
// =========================================================================
void poly_matrix_expand_ntt(poly_matrix_ntt *A_ntt, const unsigned char *seed)
{
#if BIT_USE_SHAKE
    // Batch 1 (x4): (0,0) (0,1) (0,2) (1,0)
    poly_uniform_ntt_x4(&A_ntt->vec[0].vec[0],
                        &A_ntt->vec[0].vec[1],
                        &A_ntt->vec[0].vec[2],
                        &A_ntt->vec[1].vec[0],
                        seed, 0,0, 0,1, 0,2, 1,0);

    // Batch 2 (x4): (1,1) (1,2) (2,0) (2,1)
    poly_uniform_ntt_x4(&A_ntt->vec[1].vec[1],
                        &A_ntt->vec[1].vec[2],
                        &A_ntt->vec[2].vec[0],
                        &A_ntt->vec[2].vec[1],
                        seed, 1,1, 1,2, 2,0, 2,1);

    // Batch 3 (x1): (2,2)
    poly_uniform_ntt(&A_ntt->vec[2].vec[2], seed, 2, 2);
#else
    // SM3 fallback — scalar path
    for (size_t i = 0; i < BIT_K; ++i) {
        for (size_t j = 0; j < BIT_L; ++j) {
            unsigned char buf[MATRIX_XOF_BUFFER_SIZE];
            unsigned char msg[BIT_SEEDBYTES + 2];
            bit_xof128_state stream;
            size_t ctr = 0;

            memcpy(msg, seed, BIT_SEEDBYTES);
            /* extseed = seed || col || row (matches ref poly_uniform_ntt and the
               SHAKE x4 path).  i is the row, j the column. */
               
            msg[BIT_SEEDBYTES]   = (unsigned char)j;
            msg[BIT_SEEDBYTES+1] = (unsigned char)i;

            bit_xof128_init(&stream, msg, sizeof(msg));
            while (ctr < BIT_N) {
                bit_xof128_squeeze(&stream, buf, sizeof(buf));
                ctr = rej_uniform(A_ntt->vec[i].vec[j].coeffs, ctr, BIT_N,
                                  buf, sizeof(buf));
            }
            bit_xof128_zeroize(&stream);
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
    poly_to_ntt(&result->vec[0], &y->vec[1]);
    poly_to_ntt(&result->vec[1], &y->vec[2]);
    poly_to_ntt(&result->vec[2], &y->vec[3]);
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
    const __m256i scale = _mm256_set1_epi16((int16_t)BIT_GAMMA_B);

    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j += 16) {
            __m256i vals = _mm256_load_si256((const __m256i *)(&b1->vec[i].coeffs[j]));
            vals = _mm256_mullo_epi16(vals, scale);
            _mm256_store_si256((__m256i *)(&tmp.coeffs[j]), vals);
        }
        poly_to_ntt(&b1_ntt->vec[i], &tmp);
    }
}

static inline __m256i mm256_reduce_signed_barrett_avx(__m256i a) {
    __m256i v_mult = _mm256_set1_epi32(BIT_BARRETT_MULT), v_q = _mm256_set1_epi32(BIT_Q), v_qh = _mm256_set1_epi32(BIT_Q/2);
    __m256i te = _mm256_mul_epi32(a, v_mult), to = _mm256_mul_epi32(_mm256_srli_epi64(a,32), v_mult);
    te = _mm256_srli_epi64(te, BIT_BARRETT_SHIFT); to = _mm256_srli_epi64(to, BIT_BARRETT_SHIFT);
    __m256i t = _mm256_blend_epi32(te, _mm256_slli_epi64(to, 32), 0xAA);
    a = _mm256_sub_epi32(a, _mm256_mullo_epi32(t, v_q)); a = _mm256_sub_epi32(a, v_q);
    a = _mm256_add_epi32(a, _mm256_and_si256(_mm256_srai_epi32(a, 31), v_q));
    __m256i d = _mm256_sub_epi32(v_qh, a); return _mm256_sub_epi32(a, _mm256_and_si256(v_q, _mm256_srai_epi32(d,31)));
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
        {
            const __m256i vq = _mm256_set1_epi16((short)BIT_Q);
            for (int k = 0; k < BIT_N; k += 16) {
                __m256i wv = _mm256_load_si256((__m256i *)&w_ntt.vec[i].coeffs[k]);
                __m256i tv = _mm256_load_si256((__m256i *)&tmp_ntt.coeffs[k]);
                __m256i sub = _mm256_sub_epi16(wv, tv);
                sub = _mm256_add_epi16(sub, _mm256_and_si256(_mm256_srai_epi16(sub, 15), vq));
                _mm256_store_si256((__m256i *)&w_ntt.vec[i].coeffs[k], sub);
            }
        }
        poly_ntt_montgomery_lift(&w_ntt.vec[i]);
    }
    polyveck_from_ntt(w, &w_ntt);

    __m256i v_qhat_minus = _mm256_set1_epi32(BIT_Q_HAT_MINUS);
    for (int j = 0; j < BIT_N; j += 16) {
        __m256i w16 = _mm256_load_si256((__m256i*)&w->vec[0].coeffs[j]);
        __m256i y16 = _mm256_load_si256((__m256i*)&y->vec[0].coeffs[j]);
        __m256i w_lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(w16));
        __m256i w_hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(w16, 1));
        __m256i y_lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(y16));
        __m256i y_hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(y16, 1));
        w_lo = _mm256_add_epi32(w_lo, _mm256_mullo_epi32(y_lo, v_qhat_minus));
        w_hi = _mm256_add_epi32(w_hi, _mm256_mullo_epi32(y_hi, v_qhat_minus));
        w_lo = mm256_reduce_signed_barrett_avx(w_lo); w_hi = mm256_reduce_signed_barrett_avx(w_hi);
        __m256i pk = _mm256_packs_epi32(w_lo, w_hi);
        pk = _mm256_permute4x64_epi64(pk, _MM_SHUFFLE(3, 1, 2, 0));
        _mm256_store_si256((__m256i*)&w->vec[0].coeffs[j], pk);
    }

    for (int i = 0; i < BIT_K; i++) {
        poly_add(&w->vec[i], &w->vec[i], &y->vec[1 + BIT_L + i]);
    }

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

// =========================================================================
// AVX2 decompose_hint with Barrett division by GAMMA2=955.
static inline __m256i mm256_decompose_hint_avx(__m256i v_r) {
    __m256i v_q = _mm256_set1_epi32(BIT_Q);
    __m256i v_mult = _mm256_set1_epi32((int32_t)BIT_GAMMA2_BARRETT_MULT);
    __m256i v_g2h  = _mm256_set1_epi32((int32_t)(BIT_GAMMA2 >> 1));
    __m256i v_alpha = _mm256_set1_epi32(ALPHA_HINT);
    __m256i v_one = _mm256_set1_epi32(1);

    __m256i sign = _mm256_srai_epi32(v_r, 31);
    __m256i val  = _mm256_add_epi32(v_r, _mm256_and_si256(sign, v_q));
    val = _mm256_add_epi32(val, v_g2h);

    __m256i even = _mm256_mul_epu32(val, v_mult);
    __m256i odd  = _mm256_mul_epu32(_mm256_srli_epi64(val, 32), v_mult);
    even = _mm256_srli_epi64(even, BIT_GAMMA2_BARRETT_SHIFT);
    odd  = _mm256_srli_epi64(odd,  BIT_GAMMA2_BARRETT_SHIFT);
    __m256i v_hb = _mm256_blend_epi32(even, _mm256_slli_epi64(odd, 32), 0xAA);

    __m256i v_hb1 = _mm256_add_epi32(v_hb, v_one);
    __m256i v_diff = _mm256_sub_epi32(v_alpha, v_hb1);
    v_hb = _mm256_sub_epi32(v_hb, _mm256_and_si256(v_alpha, _mm256_srai_epi32(v_diff, 31)));
    return v_hb;
}

void polyveck_make_hint_compressed(polyveck *h, const polyveck *w1, const polyveck *w, const polyveck *z2, const polyveck *b0c, const poly *c, uint8_t b) {
    __m256i v_alpha = _mm256_set1_epi32(ALPHA_HINT);
    __m256i v_alpha_half = _mm256_set1_epi32(ALPHA_HINT / 2);

    for (int i = 0; i < BIT_K; i++) {
        __m256i v_c_mask = (i == 0) ? _mm256_set1_epi32(-(int32_t)(1 - b)) : _mm256_setzero_si256();

        for (int j = 0; j < BIT_N; j += 16) {
            __m256i w_16   = _mm256_load_si256((__m256i*)&w->vec[i].coeffs[j]);
            __m256i z2_16  = _mm256_load_si256((__m256i*)&z2->vec[i].coeffs[j]);
            __m256i b0c_16 = _mm256_load_si256((__m256i*)&b0c->vec[i].coeffs[j]);
            __m256i c_16   = _mm256_loadu_si256((__m256i*)&c->coeffs[j]);

            __m256i w_lo   = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(w_16));
            __m256i w_hi   = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(w_16, 1));
            __m256i z2_lo  = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(z2_16));
            __m256i z2_hi  = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(z2_16, 1));
            __m256i b0c_lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(b0c_16));
            __m256i b0c_hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(b0c_16, 1));
            __m256i c_lo   = _mm256_and_si256(_mm256_cvtepi16_epi32(_mm256_castsi256_si128(c_16)), v_c_mask);
            __m256i c_hi   = _mm256_and_si256(_mm256_cvtepi16_epi32(_mm256_extracti128_si256(c_16, 1)), v_c_mask);

            __m256i what_lo = _mm256_add_epi32(_mm256_add_epi32(_mm256_sub_epi32(w_lo, z2_lo), b0c_lo), c_lo);
            __m256i what_hi = _mm256_add_epi32(_mm256_add_epi32(_mm256_sub_epi32(w_hi, z2_hi), b0c_hi), c_hi);

            __m256i hb_lo = mm256_decompose_hint_avx(what_lo);
            __m256i hb_hi = mm256_decompose_hint_avx(what_hi);

            __m256i w1_16 = _mm256_load_si256((__m256i*)&w1->vec[i].coeffs[j]);
            __m256i h_lo  = _mm256_sub_epi32(_mm256_cvtepi16_epi32(_mm256_castsi256_si128(w1_16)), hb_lo);
            __m256i h_hi  = _mm256_sub_epi32(_mm256_cvtepi16_epi32(_mm256_extracti128_si256(w1_16, 1)), hb_hi);

            #define HINT_MOD(h_val) do { \
                h_val = _mm256_add_epi32(h_val, _mm256_and_si256(_mm256_srai_epi32(h_val, 31), v_alpha)); \
                __m256i diff = _mm256_sub_epi32(v_alpha_half, h_val); \
                h_val = _mm256_sub_epi32(h_val, _mm256_and_si256(_mm256_srai_epi32(diff, 31), v_alpha)); \
            } while(0)

            HINT_MOD(h_lo);
            HINT_MOD(h_hi);
            #undef HINT_MOD

            __m256i packed = _mm256_packs_epi32(h_lo, h_hi);
            packed = _mm256_permute4x64_epi64(packed, _MM_SHUFFLE(3, 1, 2, 0));
            _mm256_store_si256((__m256i*)&h->vec[i].coeffs[j], packed);
        }
    }
}

void polyveck_compute_w_hat_prime_ntt(polyveck *w_hat_prime, const poly_matrix_ntt *A_0_ntt, const polyveck_ntt *b1_ntt, const polyvecl_ntt *z1_tail_ntt, const poly_ntt *z0_ntt, const poly *z0, const poly *c_poly) {
    polyveck_ntt acc;
    poly_ntt tmp_ntt;
    __m256i v_q = _mm256_set1_epi16((int16_t)BIT_Q);
    __m256i v_q_hat = _mm256_set1_epi32(BIT_Q_HAT);

    for (int i = 0; i < BIT_K; i++) {
        poly_ntt_zero(&acc.vec[i]);
        for (int j = 0; j < BIT_L; j++) {
            poly_ntt_acc_mul_raw(&acc.vec[i], &A_0_ntt->vec[i].vec[j], &z1_tail_ntt->vec[j]);
        }
        poly_ntt_mul_raw(&tmp_ntt, &b1_ntt->vec[i], z0_ntt);

        for (int j = 0; j < BIT_N; j += 16) {
            __m256i a = _mm256_load_si256((__m256i*)&acc.vec[i].coeffs[j]);
            __m256i t = _mm256_load_si256((__m256i*)&tmp_ntt.coeffs[j]);
            __m256i sub = _mm256_sub_epi16(a, t);
            sub = _mm256_add_epi16(sub, _mm256_and_si256(_mm256_srai_epi16(sub, 15), v_q));
            _mm256_store_si256((__m256i*)&acc.vec[i].coeffs[j], sub);
        }
        poly_ntt_montgomery_lift(&acc.vec[i]);
    }

    polyveck_from_ntt(w_hat_prime, &acc);

    for (int j = 0; j < BIT_N; j += 16) {
        __m256i w0_16 = _mm256_load_si256((__m256i*)&w_hat_prime->vec[0].coeffs[j]);
        __m256i z0_16 = _mm256_loadu_si256((__m256i*)&z0->coeffs[j]);
        __m256i c_16  = _mm256_loadu_si256((__m256i*)&c_poly->coeffs[j]);

        __m256i w0_lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(w0_16));
        __m256i w0_hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(w0_16, 1));
        __m256i z0_lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(z0_16));
        __m256i z0_hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(z0_16, 1));
        __m256i c_lo  = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(c_16));
        __m256i c_hi  = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(c_16, 1));

        __m256i sum_lo = _mm256_add_epi32(z0_lo, c_lo);
        __m256i sum_hi = _mm256_add_epi32(z0_hi, c_hi);

        w0_lo = _mm256_add_epi32(w0_lo, _mm256_mullo_epi32(sum_lo, v_q_hat));
        w0_hi = _mm256_add_epi32(w0_hi, _mm256_mullo_epi32(sum_hi, v_q_hat));

        w0_lo = mm256_reduce_signed_barrett_avx(w0_lo);
        w0_hi = mm256_reduce_signed_barrett_avx(w0_hi);

        __m256i packed = _mm256_packs_epi32(w0_lo, w0_hi);
        packed = _mm256_permute4x64_epi64(packed, _MM_SHUFFLE(3, 1, 2, 0));
        _mm256_store_si256((__m256i*)&w_hat_prime->vec[0].coeffs[j], packed);
    }
}

void polyveck_compute_w_prime(polyveck *w_prime, const polyveck *w_hat_prime, const polyveck *h) {
    __m256i v_alpha = _mm256_set1_epi32(ALPHA_HINT);

    for (int i = 0; i < BIT_K; i++) {
        for (int j = 0; j < BIT_N; j += 16) {
            __m256i wh_16 = _mm256_load_si256((__m256i*)&w_hat_prime->vec[i].coeffs[j]);
            __m256i h_16  = _mm256_load_si256((__m256i*)&h->vec[i].coeffs[j]);

            __m256i wh_lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(wh_16));
            __m256i wh_hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(wh_16, 1));
            __m256i h_lo  = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(h_16));
            __m256i h_hi  = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(h_16, 1));

            __m256i hb_lo = mm256_decompose_hint_avx(wh_lo);
            __m256i hb_hi = mm256_decompose_hint_avx(wh_hi);

            __m256i val_lo = _mm256_add_epi32(hb_lo, h_lo);
            __m256i val_hi = _mm256_add_epi32(hb_hi, h_hi);

            #define W_MOD(val) do { \
                val = _mm256_add_epi32(val, _mm256_and_si256(_mm256_srai_epi32(val, 31), v_alpha)); \
                val = _mm256_sub_epi32(val, v_alpha); \
                val = _mm256_add_epi32(val, _mm256_and_si256(_mm256_srai_epi32(val, 31), v_alpha)); \
            } while(0)

            W_MOD(val_lo);
            W_MOD(val_hi);
            #undef W_MOD

            __m256i packed = _mm256_packs_epi32(val_lo, val_hi);
            packed = _mm256_permute4x64_epi64(packed, _MM_SHUFFLE(3, 1, 2, 0));
            _mm256_store_si256((__m256i*)&w_prime->vec[i].coeffs[j], packed);
        }
    }
}

#endif
