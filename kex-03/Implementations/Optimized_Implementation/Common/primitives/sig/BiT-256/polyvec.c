/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
*/
#ifndef POLYVEC_C
#define POLYVEC_C
#include "polyvec.h"
#include "params.h"
#include "align.h"
#include "sample.h"
#include "endian.h"
#include "symmetric.h"
#include <stddef.h>
#include <string.h>
#include <immintrin.h>

/* AVX2, 8-wide signed Barrett reduction — bit-identical to the scalar
 * reduce_signed_barrett() in reduce.h for any int32 input (|x| ≲ 2^29):
 *   t = (x·MULT) >> SHIFT ;  r = x − t·q ;  r −= q ; r += (r<0)?q:0 → [0,q)
 *   then center to [-q/2, q/2).  Used by compute_w / compute_w_hat_prime. */
static inline __m256i mm256_reduce_signed_barrett_epi32(__m256i x)
{
    const __m256i vM  = _mm256_set1_epi32((int32_t)BIT_BARRETT_MULT);
    const __m256i vq  = _mm256_set1_epi32((int32_t)BIT_Q);
    const __m256i vqh = _mm256_set1_epi32((int32_t)(BIT_Q / 2));
    __m256i xo = _mm256_shuffle_epi32(x, 0xF5);
    __m256i p_lo = _mm256_mul_epi32(x,  vM);
    __m256i p_hi = _mm256_mul_epi32(xo, vM);
    __m256i t_lo = _mm256_srai_epi32(_mm256_srli_epi64(p_lo, 32), BIT_BARRETT_SHIFT - 32);
    __m256i t_hi = _mm256_srai_epi32(_mm256_srli_epi64(p_hi, 32), BIT_BARRETT_SHIFT - 32);
    __m256i t  = _mm256_blend_epi32(t_lo, _mm256_slli_epi64(t_hi, 32), 0xAA);
    __m256i r  = _mm256_sub_epi32(x, _mm256_mullo_epi32(t, vq));   /* ∈ [0,2q) */
    __m256i d  = _mm256_sub_epi32(r, vq);
    r = _mm256_add_epi32(d, _mm256_and_si256(_mm256_srai_epi32(d, 31), vq)); /* [0,q) */
    __m256i over = _mm256_srai_epi32(_mm256_sub_epi32(vqh, r), 31);          /* -1 if r>q/2 */
    return _mm256_sub_epi32(r, _mm256_and_si256(over, vq));                  /* [-q/2,q/2) */
}
#include <immintrin.h>

void polyvecl_sample_S1(polyvecl *v, const uint8_t *seed, uint16_t *nonce) {
    for (size_t i = 0; i < BIT_L; i++)
        poly_sample_S1(&v->vec[i], seed, nonce);
}

void polyveck_sample_S1(polyveck *v, const uint8_t *seed, uint16_t *nonce) {
    for (size_t i = 0; i < BIT_K; i++)
        poly_sample_S1(&v->vec[i], seed, nonce);
}

/* AVX2 x4 batched: s[0..2] + e[0] in one parallel XOF call */
void polyvec_sample_S1_avx(polyvecl *s, polyveck *e,
                           const uint8_t *seed, uint16_t *nonce) {
#if BIT_USE_SHAKE
    poly_sample_S1_x4(&s->vec[0], &s->vec[1], &s->vec[2], &e->vec[0],
                      seed, nonce);
    poly_sample_S1_x2(&e->vec[1], &e->vec[2], seed, nonce);
#else
    /* SM3: the x4/x2 prefetch uses bit_xof256_nonce (one-shot pseudoXOF over
       seed||nonce), which differs from the reference stateful stream whose
       per-squeeze input is seed||nonce||block.  The SHAKE sponge makes the two
       coincide; SM3's explicit block counter does not.  SM3 has no 4-way XOF
       batching anyway, so fall back to the exact scalar/stateful path the
       reference uses (byte-for-byte identical, zero perf cost). */
    polyvecl_sample_S1(s, seed, nonce);
    polyveck_sample_S1(e, seed, nonce);
#endif
}

/* Accept rate 119297/131072 ≈ 91.0%.  Each 24-bit load needs 3 bytes.
   BIT_N=512 requires ~1684 bytes → ~10 SHAKE128 blocks per polynomial. */
#define MATRIX_XOF_BLOCKS 10
#if BIT_USE_SHAKE
#define MATRIX_XOF_BUFFER_SIZE (MATRIX_XOF_BLOCKS * SHAKE128_RATE)
#else
/* SM3: bit_xof128_squeeze is a block-indexed pseudoXOF (input = msg||0000||block),
   NOT a continuous stream, so the per-squeeze chunk size is part of the KAT and
   must equal the reference's 4*168 = 672 bytes. */
#define MATRIX_XOF_BUFFER_SIZE 672
#endif

/* Scalar uniform rejection: reads 24-bit values, keeps those < BIT_Q */
static size_t rej_uniform(int32_t *a, size_t ctr, size_t len,
                          const uint8_t *buf, size_t buflen) {
    size_t pos = 0;
    while (ctr < len && pos + 2 < buflen) {
        uint32_t val = load24_le(buf + pos) & 0x1FFFF;
        pos += 3;
        if (val < BIT_Q) a[ctr++] = (int32_t)val;
    }
    return ctr;
}

#if BIT_USE_SHAKE
/* AVX2 rejection: 24 bytes → 8 candidate int32 (24-bit loads via vpshufb), one
   vpcmpgt against q, then vpermd through a precomputed compaction table packs the
   accepted lanes contiguously for a single store.  Byte-for-byte identical output
   to the scalar path (same 3-byte values, same order); the scalar tail finishes
   the <24-byte remainder.  ~5x faster than the scalar 4-way unroll. */
extern const uint32_t rej_uniform_idx[256][8];
/* Asymmetric shuffle: low lane takes bytes 0..11 of a load at pos (→ v0..v3),
   high lane takes bytes 4..15 of a load at pos+8 (= buf[pos+12..23] → v4..v7).
   The pos+8 high load reads only up to pos+23, so 24 bytes available is enough
   and there is no out-of-bounds over-read past the buffer. */
static const uint8_t rej_shuf24[32] __attribute__((aligned(32))) = {
    0,1,2,255, 3,4,5,255, 6,7,8,255, 9,10,11,255,
    4,5,6,255, 7,8,9,255, 10,11,12,255, 13,14,15,255
};
static size_t rej_uniform_avx(int32_t *a, size_t ctr, size_t len,
                              const uint8_t *buf, size_t buflen) {
    size_t pos = 0;
    const __m256i vq  = _mm256_set1_epi32(BIT_Q);
    const __m256i vm  = _mm256_set1_epi32(0x1FFFF);
    const __m256i vsh = _mm256_load_si256((const __m256i *)rej_shuf24);

    /* 8 values (24 bytes) per step while ≥8 output slots and ≥24 bytes remain */
    while (ctr + 8 <= len && pos + 24 <= buflen) {
        __m128i lo = _mm_loadu_si128((const __m128i *)(buf + pos));        /* bytes 0..11 */
        __m128i hi = _mm_loadu_si128((const __m128i *)(buf + pos + 8));    /* bytes 12..23 */
        __m256i raw = _mm256_set_m128i(hi, lo);
        __m256i v = _mm256_and_si256(_mm256_shuffle_epi8(raw, vsh), vm);   /* 8×17-bit */
        pos += 24;

        int mask = _mm256_movemask_ps(_mm256_castsi256_ps(
                       _mm256_cmpgt_epi32(vq, v)));                        /* accept = q>v */
        __m256i perm = _mm256_permutevar8x32_epi32(
                           v, _mm256_load_si256((const __m256i *)rej_uniform_idx[mask]));
        _mm256_storeu_si256((__m256i *)(a + ctr), perm);
        ctr += (size_t)__builtin_popcount(mask);
    }
    /* Scalar tail for the remaining <24 bytes (and last <8 output slots) */
    return rej_uniform(a, ctr, len, buf + pos, buflen - pos);
}

/* 4-way parallel uniform NTT-domain sampling (SHAKE128 x4). */
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

#if !BIT_USE_SHAKE
/* Scalar uniform NTT-domain sampling for single-polynomial SM3 fallback. */
__attribute__((aligned(64)))
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
            uint32_t val = load24_le(buf + pos) & 0x1FFFF;
            pos += 3;
            if (val < BIT_Q) {
                a->coeffs[ctr++] = (int32_t)val;
            }
        }
    }
    bit_xof128_zeroize(&st);
}
#endif /* !BIT_USE_SHAKE */

/* Matrix expansion: K×L = 3×3 = 9 polys → x4 + x4 + x1 */
void poly_matrix_expand_ntt(poly_matrix_ntt *A_ntt, const unsigned char *seed)
{
#if BIT_USE_SHAKE
    /* Batch 1 (x4): (0,0) (0,1) (0,2) (1,0) */
    poly_uniform_ntt_x4(&A_ntt->vec[0].vec[0],
                        &A_ntt->vec[0].vec[1],
                        &A_ntt->vec[0].vec[2],
                        &A_ntt->vec[1].vec[0],
                        seed, 0,0,  0,1,  0,2,  1,0);
    /* Batch 2 (x4): (1,1) (1,2) (2,0) (2,1) */
    poly_uniform_ntt_x4(&A_ntt->vec[1].vec[1],
                        &A_ntt->vec[1].vec[2],
                        &A_ntt->vec[2].vec[0],
                        &A_ntt->vec[2].vec[1],
                        seed, 1,1,  1,2,  2,0,  2,1);
    /* Batch 3: (2,2) via the x4 engine (lanes 1..3 dummy) — the AVX2
       Keccak + rej_uniform_avx path is far cheaper than the scalar
       poly_uniform_ntt, even with 3 wasted lanes. */
    {
        poly_ntt dummy1, dummy2, dummy3;
        poly_uniform_ntt_x4(&A_ntt->vec[2].vec[2], &dummy1, &dummy2, &dummy3,
                            seed, 2,2,  2,2,  2,2,  2,2);
    }
#else
    for (size_t i = 0; i < BIT_K; ++i)
        for (size_t j = 0; j < BIT_L; ++j)
            poly_uniform_ntt(&A_ntt->vec[i].vec[j], seed, i, j);
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
        /* plain subtract: w_ntt ∈ ~±4q, tmp ∈ ~±q → ±5q (fits int32); the
         * following montgomery_lift reduces mod q, so no Barrett needed here */
        for (int j = 0; j < BIT_N; j++) {
            w_ntt.vec[i].coeffs[j] = w_ntt.vec[i].coeffs[j] - tmp_ntt.coeffs[j];
        }
        poly_ntt_montgomery_lift(&w_ntt.vec[i]);
    }
    polyveck_from_ntt(w, &w_ntt);

    {
        const __m256i vqhm = _mm256_set1_epi32((int32_t)BIT_Q_HAT_MINUS);
        for (int j = 0; j < BIT_N; j += 8) {
            __m256i wv = _mm256_loadu_si256((const __m256i *)&w->vec[0].coeffs[j]);
            __m256i yv = _mm256_loadu_si256((const __m256i *)&y->vec[0].coeffs[j]);
            __m256i v  = _mm256_add_epi32(wv, _mm256_mullo_epi32(yv, vqhm));
            _mm256_storeu_si256((__m256i *)&w->vec[0].coeffs[j],
                                mm256_reduce_signed_barrett_epi32(v));
        }
    }

    for (int i = 0; i < BIT_K; i++) {
        poly_add(&w->vec[i], &w->vec[i], &y->vec[1 + BIT_L + i]);
    }

}

void polyvecy_sample_triangular(polyvecy *y, const uint8_t *seed_y, uint16_t *nonce) {
    polyvecy_sample_triangular_avx(y, seed_y, nonce);
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

/* Barrett division by BIT_GAMMA2 */
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
    const __m256i v_q     = _mm256_set1_epi32(BIT_Q);
    const __m256i v_alpha = _mm256_set1_epi32(ALPHA_HINT);
    const __m256i v_one   = _mm256_set1_epi32(1);

    __m256i sign = _mm256_srai_epi32(v_r, 31);
    __m256i val  = _mm256_add_epi32(v_r, _mm256_and_si256(sign, v_q));
    __m256i hb   = mm256_div_g2(val);
    __m256i over = _mm256_srai_epi32(
                       _mm256_sub_epi32(v_alpha, _mm256_add_epi32(hb, v_one)), 31);
    hb = _mm256_sub_epi32(hb, _mm256_and_si256(over, v_alpha));
    return hb;
}

void polyveck_make_hint_compressed(polyveck *h, const polyveck *w1, const polyveck *w,
                                   const polyveck *z2, const polyveck *b0c,
                                   const poly *c, uint8_t b) {
    const __m256i v_alpha   = _mm256_set1_epi32(ALPHA_HINT);
    const __m256i v_alpha_h = _mm256_set1_epi32(ALPHA_HINT / 2);
    const __m256i v_c_mask  = _mm256_set1_epi32(-(int32_t)(1 - b));

    for (int j = 0; j < BIT_K; j++) {
        /* c_mask applies only to row 0 */
        __m256i v_cm = (j == 0) ? v_c_mask : _mm256_setzero_si256();

        PRAGMA_UNROLL_4
        for (int i = 0; i < BIT_N; i += 8) {
            __m256i v_w   = _mm256_load_si256((const __m256i *)&w->vec[j].coeffs[i]);
            __m256i v_z2  = _mm256_load_si256((const __m256i *)&z2->vec[j].coeffs[i]);
            __m256i v_c   = _mm256_load_si256((const __m256i *)&c->coeffs[i]);
            __m256i v_b0c = _mm256_load_si256((const __m256i *)&b0c->vec[j].coeffs[i]);
            __m256i v_w1  = _mm256_load_si256((const __m256i *)&w1->vec[j].coeffs[i]);

            __m256i w_hat = _mm256_sub_epi32(v_w, v_z2);
            w_hat = _mm256_add_epi32(w_hat, _mm256_and_si256(v_c, v_cm));
            w_hat = _mm256_add_epi32(w_hat, v_b0c);

            __m256i hb_w_hat = mm256_decompose_hint_avx(w_hat);

            __m256i h_val = _mm256_sub_epi32(v_w1, hb_w_hat);
            h_val = _mm256_add_epi32(h_val,
                     _mm256_and_si256(_mm256_srai_epi32(h_val, 31), v_alpha));
            __m256i over_h = _mm256_srai_epi32(
                                 _mm256_sub_epi32(v_alpha_h, h_val), 31);
            h_val = _mm256_sub_epi32(h_val, _mm256_and_si256(over_h, v_alpha));

            _mm256_store_si256((__m256i *)&h->vec[j].coeffs[i], h_val);
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
        /* plain subtract; montgomery_lift below reduces mod q (no Barrett needed) */
        for (int j = 0; j < BIT_N; j++) {
            acc.vec[i].coeffs[j] = acc.vec[i].coeffs[j] - tmp_ntt.coeffs[j];
        }
        poly_ntt_montgomery_lift(&acc.vec[i]);
    }

    polyveck_from_ntt(w_hat_prime, &acc);

    {
        const __m256i vqh = _mm256_set1_epi32((int32_t)BIT_Q_HAT);
        for (int j = 0; j < BIT_N; j += 8) {
            __m256i wv = _mm256_loadu_si256((const __m256i *)&w_hat_prime->vec[0].coeffs[j]);
            __m256i zv = _mm256_loadu_si256((const __m256i *)&z0->coeffs[j]);
            __m256i cv = _mm256_loadu_si256((const __m256i *)&c_poly->coeffs[j]);
            __m256i s  = _mm256_add_epi32(zv, cv);
            __m256i v  = _mm256_add_epi32(wv, _mm256_mullo_epi32(s, vqh));
            _mm256_storeu_si256((__m256i *)&w_hat_prime->vec[0].coeffs[j],
                                mm256_reduce_signed_barrett_epi32(v));
        }
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

#endif
