/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
*/
#include "poly.h"
#include "ntt_avx.h"
#include "symmetric.h"
#include "align.h"
#include "endian.h"
#include <immintrin.h>
#include <stddef.h>
#include <string.h>

static const int32_t map_S1[4] = {-1, 0, 1, 0};

static size_t rej_S1(int32_t *a, size_t len, const uint8_t *buf, size_t buflen) {
    size_t ctr = 0, pos = 0;
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

uint32_t reduce_barrett(int64_t a) {
    int64_t t;
    t = (((int64_t)a * BIT_BARRETT_MULT) >> BIT_BARRETT_SHIFT);
    a -= t * BIT_Q;
    /* Barrett estimate may be off by one for deeply negative inputs. */
    a += (a >> 63) & BIT_Q;
    a -= BIT_Q;
    a += (a >> 63) & BIT_Q;
    return (uint32_t)a;
}

inline void decompose_hint(int32_t *highbits, int32_t r) {
    uint32_t val = reduce_to_unsigned((int32_t)r);
    uint64_t numerator = (uint64_t)val + (BIT_GAMMA2 >> 1);
    int32_t hb = (int32_t)((numerator * BIT_GAMMA2_BARRETT_MULT) >> BIT_GAMMA2_BARRETT_SHIFT);
    int32_t edgecase = (ALPHA_HINT - (hb + 1)) >> 31;
    hb -= ALPHA_HINT & edgecase;
    *highbits = hb;
}

/* ================================================================
   AVX2 constant vectors (constructed once, used in all poly ops)
   ================================================================ */
#define VQ   _mm256_set1_epi32((int32_t)BIT_Q)
#define VQH  _mm256_set1_epi32((int32_t)BIT_Q_HALF)
#define VMQH _mm256_set1_epi32((int32_t)(-BIT_Q_HALF))

/* signed reduction: clamp x ∈ [-q, q] → [-q/2, q/2) by adding/subtracting q */
static inline __m256i mm256_reduce_signed_epi32(__m256i x) {
    __m256i m1 = _mm256_cmpgt_epi32(VMQH, x);   /* x < -q/2 */
    __m256i m2 = _mm256_cmpgt_epi32(x, VQH);     /* x >  q/2 */
    x = _mm256_add_epi32(x, _mm256_and_si256(m1, VQ));
    x = _mm256_sub_epi32(x, _mm256_and_si256(m2, VQ));
    return x;
}

/* ================================================================
   Sampling
   ================================================================ */
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

/* x4 batched S1 sampling — 4 polynomials from one parallel XOF call */
void poly_sample_S1_x4(poly *p0, poly *p1, poly *p2, poly *p3,
                       const uint8_t *seed, uint16_t *nonce) {
    uint8_t ALIGNED_32 buf0[BIT_SAMPLE_S1_BUFLEN], buf1[BIT_SAMPLE_S1_BUFLEN];
    uint8_t ALIGNED_32 buf2[BIT_SAMPLE_S1_BUFLEN], buf3[BIT_SAMPLE_S1_BUFLEN];
    uint16_t n0 = *nonce, n1 = (uint16_t)(*nonce + 1U);
    uint16_t n2 = (uint16_t)(*nonce + 2U), n3 = (uint16_t)(*nonce + 3U);
    *nonce = (uint16_t)(*nonce + 4U);
    bit_xof256x4(buf0, buf1, buf2, buf3, BIT_SAMPLE_S1_BUFLEN,
                 seed, BIT_SEEDBYTES, n0, n1, n2, n3);
    if (rej_S1(p0->coeffs, BIT_N, buf0, BIT_SAMPLE_S1_BUFLEN) < BIT_N) poly_sample_S1(p0, seed, &n0);
    if (rej_S1(p1->coeffs, BIT_N, buf1, BIT_SAMPLE_S1_BUFLEN) < BIT_N) poly_sample_S1(p1, seed, &n1);
    if (rej_S1(p2->coeffs, BIT_N, buf2, BIT_SAMPLE_S1_BUFLEN) < BIT_N) poly_sample_S1(p2, seed, &n2);
    if (rej_S1(p3->coeffs, BIT_N, buf3, BIT_SAMPLE_S1_BUFLEN) < BIT_N) poly_sample_S1(p3, seed, &n3);
}

/* x2 batched S1 sampling */
void poly_sample_S1_x2(poly *p0, poly *p1, const uint8_t *seed, uint16_t *nonce) {
    uint8_t ALIGNED_32 buf0[BIT_SAMPLE_S1_BUFLEN], buf1[BIT_SAMPLE_S1_BUFLEN];
    uint16_t n0 = *nonce, n1 = (uint16_t)(*nonce + 1U);
    *nonce = (uint16_t)(*nonce + 2U);
    bit_xof256x2(buf0, buf1, BIT_SAMPLE_S1_BUFLEN, seed, BIT_SEEDBYTES, n0, n1);
    if (rej_S1(p0->coeffs, BIT_N, buf0, BIT_SAMPLE_S1_BUFLEN) < BIT_N) poly_sample_S1(p0, seed, &n0);
    if (rej_S1(p1->coeffs, BIT_N, buf1, BIT_SAMPLE_S1_BUFLEN) < BIT_N) poly_sample_S1(p1, seed, &n1);
}

/* ================================================================
   NTT wrappers (signed↔unsigned fused into NTT as in avx2-128)
   ================================================================ */
void poly_to_ntt(poly_ntt *result, const poly *a) {
    *result = *(const poly_ntt *)a;
    ntt_forward(result->coeffs);
}
void poly_from_ntt(poly *result, const poly_ntt *a) {
    *result = *(const poly *)a;
    ntt_inverse(result->coeffs);
}
void poly_ntt_mul_raw(poly_ntt *result, const poly_ntt *a, const poly_ntt *b) {
    ntt_pointwise_mul_raw(result->coeffs, a->coeffs, b->coeffs);
}
void poly_ntt_acc_mul_raw(poly_ntt *result, const poly_ntt *a, const poly_ntt *b) {
    ntt_pointwise_acc_raw(result->coeffs, a->coeffs, b->coeffs);
}
void poly_ntt_montgomery_lift(poly_ntt *result) {
    ntt_montgomery_lift(result->coeffs);
}
void poly_ntt_zero(poly_ntt *result) {
    const __m256i z = _mm256_setzero_si256();
    PRAGMA_UNROLL_4
    for (int i = 0; i < BIT_N; i += 8)
        _mm256_store_si256((__m256i *)(&result->coeffs[i]), z);
}

/* ================================================================
   poly_add / poly_sub — AVX2, 8×int32 per vector
   ================================================================ */
void poly_add(poly *result, const poly *a, const poly *b) {
    PRAGMA_UNROLL_4
    for (int i = 0; i < BIT_N; i += 8) {
        __m256i va = _mm256_load_si256((const __m256i *)&a->coeffs[i]);
        __m256i vb = _mm256_load_si256((const __m256i *)&b->coeffs[i]);
        __m256i v  = _mm256_add_epi32(va, vb);
        _mm256_store_si256((__m256i *)&result->coeffs[i],
                           mm256_reduce_signed_epi32(v));
    }
}

void poly_add_eta1(poly *result, const poly *a, const poly *eta1) {
    /* eta1 ∈ {-1,0,1}; a ∈ [-Q/2,Q/2]; sum ∈ [-(Q/2+1), Q/2+1].
     * One round of signed reduction suffices (same as poly_add). */
    PRAGMA_UNROLL_4
    for (int i = 0; i < BIT_N; i += 8) {
        __m256i va = _mm256_load_si256((const __m256i *)&a->coeffs[i]);
        __m256i ve = _mm256_load_si256((const __m256i *)&eta1->coeffs[i]);
        __m256i v  = _mm256_add_epi32(va, ve);
        _mm256_store_si256((__m256i *)&result->coeffs[i],
                           mm256_reduce_signed_epi32(v));
    }
}

void poly_sub(poly *result, const poly *a, const poly *b) {
    PRAGMA_UNROLL_4
    for (int i = 0; i < BIT_N; i += 8) {
        __m256i va = _mm256_load_si256((const __m256i *)&a->coeffs[i]);
        __m256i vb = _mm256_load_si256((const __m256i *)&b->coeffs[i]);
        __m256i v  = _mm256_sub_epi32(va, vb);
        _mm256_store_si256((__m256i *)&result->coeffs[i],
                           mm256_reduce_signed_epi32(v));
    }
}

static inline __m256i mm256_div_gamma2(__m256i v) {
    const __m256i v_g2h  = _mm256_set1_epi32((int32_t)(BIT_GAMMA2 >> 1));
    const __m256i v_mult = _mm256_set1_epi32((int32_t)BIT_GAMMA2_BARRETT_MULT);
    v = _mm256_add_epi32(v, v_g2h);
    __m256i even = _mm256_mul_epu32(v, v_mult);
    __m256i odd  = _mm256_mul_epu32(_mm256_srli_epi64(v, 32), v_mult);
    even = _mm256_srli_epi64(even, BIT_GAMMA2_BARRETT_SHIFT);
    odd  = _mm256_srli_epi64(odd,  BIT_GAMMA2_BARRETT_SHIFT);
    return _mm256_blend_epi32(even, _mm256_slli_epi64(odd, 32), 0xAA);
}

void poly_highbits(poly *w1, const poly *w) {
    const __m256i v_q   = VQ;
    const __m256i v_alpha = _mm256_set1_epi32((int32_t)ALPHA_HINT);
    const __m256i v_one   = _mm256_set1_epi32(1);

    PRAGMA_UNROLL_4
    for (int i = 0; i < BIT_N; i += 8) {
        __m256i r    = _mm256_load_si256((const __m256i *)&w->coeffs[i]);
        __m256i sign = _mm256_srai_epi32(r, 31);
        __m256i val  = _mm256_add_epi32(r, _mm256_and_si256(sign, v_q));
        __m256i hb   = mm256_div_gamma2(val);
        __m256i over = _mm256_srai_epi32(
                           _mm256_sub_epi32(v_alpha, _mm256_add_epi32(hb, v_one)), 31);
        hb = _mm256_sub_epi32(hb, _mm256_and_si256(over, v_alpha));
        _mm256_store_si256((__m256i *)&w1->coeffs[i], hb);
    }
}

/* ================================================================
   poly_decompose_b — AVX2, 8×int32
   ================================================================ */
void poly_decompose_b(poly *b1, poly *b0, const poly *b) {
    const __m256i v_q   = VQ;
    const __m256i v_qh  = _mm256_set1_epi32((int32_t)((BIT_Q - 1) / 2));
    const __m256i v_gh  = _mm256_set1_epi32((int32_t)(BIT_GAMMA_B >> 1));
    const __m256i v_gb  = _mm256_set1_epi32((int32_t)BIT_GAMMA_B);

    PRAGMA_UNROLL_4
    for (int i = 0; i < BIT_N; i += 8) {
        __m256i r    = _mm256_load_si256((const __m256i *)&b->coeffs[i]);
        __m256i sign = _mm256_srai_epi32(r, 31);
        __m256i a    = _mm256_add_epi32(r, _mm256_and_si256(sign, v_q));
        __m256i hb   = _mm256_srli_epi32(_mm256_add_epi32(a, v_gh), 6);
        __m256i low  = _mm256_sub_epi32(a, _mm256_mullo_epi32(hb, v_gb));
        __m256i over = _mm256_cmpgt_epi32(low, v_qh);
        low = _mm256_sub_epi32(low, _mm256_and_si256(over, v_q));
        _mm256_store_si256((__m256i *)&b1->coeffs[i], hb);
        _mm256_store_si256((__m256i *)&b0->coeffs[i], low);
    }
}

/* ================================================================
   poly_pack_w1 — scalar bit packing (parameter-specific layout)
   ================================================================ */
void poly_pack_w1(unsigned char *r, const poly *a) {
    for (int i = 0; i < BIT_N / 8; i++) {
        uint32_t t0 = (uint32_t)a->coeffs[8*i+0];
        uint32_t t1 = (uint32_t)a->coeffs[8*i+1];
        uint32_t t2 = (uint32_t)a->coeffs[8*i+2];
        uint32_t t3 = (uint32_t)a->coeffs[8*i+3];
        uint32_t t4 = (uint32_t)a->coeffs[8*i+4];
        uint32_t t5 = (uint32_t)a->coeffs[8*i+5];
        uint32_t t6 = (uint32_t)a->coeffs[8*i+6];
        uint32_t t7 = (uint32_t)a->coeffs[8*i+7];
        r[5*i+0] = (uint8_t)(t0 | (t1 << 5));
        r[5*i+1] = (uint8_t)((t1 >> 3) | (t2 << 2) | (t3 << 7));
        r[5*i+2] = (uint8_t)((t3 >> 1) | (t4 << 4));
        r[5*i+3] = (uint8_t)((t4 >> 4) | (t5 << 1) | (t6 << 6));
        r[5*i+4] = (uint8_t)((t6 >> 2) | (t7 << 3));
    }
}

/* ================================================================
   poly_challenge / poly_challenge_to_sparse / poly_cneg
   ================================================================ */
void poly_challenge(poly *c, const unsigned char seed[BIT_CHALLENGEBYTES]) {
    unsigned int i, b, pos;
    uint64_t signs;
    uint8_t buf[BIT_XOF256_RATE];
    bit_xof256_ctx ctx;
    bit_xof256_ctx_init(&ctx, seed, BIT_CHALLENGEBYTES);
    bit_xof256_ctx_squeezeblocks(&ctx, buf, 1);
    signs = 0;
    for (i = 0; i < 8; i++) signs |= (uint64_t)buf[i] << (8 * i);
    pos = 8;
    for (i = 0; i < BIT_N; i++) c->coeffs[i] = 0;
    for (i = BIT_N - BIT_TAU; i < BIT_N; i++) {
        do {
            if (pos + 1 >= (unsigned)BIT_XOF256_RATE) {
                bit_xof256_ctx_squeezeblocks(&ctx, buf, 1);
                pos = 0;
            }
            b = load16_le(buf + pos) & 0x03FF;
            pos += 2;
        } while (b > i);
        c->coeffs[i] = c->coeffs[b];
        c->coeffs[b] = 1;
        signs >>= 1;
    }
    bit_xof256_ctx_zeroize(&ctx);
}

void poly_challenge_to_sparse(sparse_challenge *s, const poly *c) {
    const __m256i vzero = _mm256_setzero_si256();
    int ctr = 0;
    for (int i = 0; i < BIT_N; i += 8) {
        __m256i v  = _mm256_load_si256((const __m256i *)&c->coeffs[i]);
        /* cmpeq_epi32 → 4 bytes per lane in movemask.  Mask 0x11111111 keeps
           only bit 0,4,8,...,28 (one bit per 32-bit lane) so ctz>>2 gives the
           correct lane index and nz&=nz-1 clears exactly that lane. */
        uint32_t nz = ~_mm256_movemask_epi8(_mm256_cmpeq_epi32(v, vzero))
                      & 0x11111111u;
        while (nz && ctr < BIT_TAU) {
            int k = __builtin_ctz(nz) >> 2;   /* bit index → 32-bit lane index */
            nz &= nz - 1;
            int idx = i + k;
            s->pos[ctr]  = (uint16_t)idx;
            s->sign[ctr] = (int8_t)c->coeffs[idx];
            ctr++;
        }
        if (ctr == BIT_TAU) break;
    }
}

/*
 * Constant-time challenge multiply (challenge ∈ {0,1}; poly_cneg gives the
 * uniform bimodal sign s = (−1)^b to every term).  We accumulate the unsigned
 * negacyclic shift-sum from a SINGLE extended array and apply s branch-free at
 * the store — no per-position `?:` select on the sign, so the secret bit b no
 * longer steers control flow or pointer selection.  Numerically identical to
 * the scalar reference (KAT unchanged), and faster (one array, no select).
 */
void poly_mul_challenge_no_reduce(poly *result, const poly *a, const sparse_challenge *c) {
    int32_t A_ext[BIT_N * 2];     /* [ −A ‖ A ] : reading at (N−pos) yields x^pos·a */
    const __m256i zero = _mm256_setzero_si256();

    PRAGMA_UNROLL_4
    for (int i = 0; i < BIT_N; i += 8) {
        __m256i val = _mm256_load_si256((const __m256i *)(a->coeffs + i));
        __m256i neg = _mm256_sub_epi32(zero, val);
        _mm256_storeu_si256((__m256i *)(A_ext + i), neg);
        _mm256_storeu_si256((__m256i *)(A_ext + BIT_N + i), val);
    }

    const int32_t *RESTRICT ptrs[BIT_TAU];
    for (int t = 0; t < BIT_TAU; t++)
        ptrs[t] = A_ext + BIT_N - c->pos[t];

    /* uniform sign s = (−1)^b → branch-free mask: 0 (s=+1) or −1 (s=−1) */
    const __m256i smask = _mm256_set1_epi32((int32_t)c->sign[0] >> 31);

    PRAGMA_UNROLL_2
    for (int pass = 0; pass < 16; pass++) {
        __m256i a0 = _mm256_setzero_si256(), a1 = _mm256_setzero_si256();
        __m256i a2 = _mm256_setzero_si256(), a3 = _mm256_setzero_si256();
        __m256i a4 = _mm256_setzero_si256(), a5 = _mm256_setzero_si256();
        __m256i a6 = _mm256_setzero_si256(), a7 = _mm256_setzero_si256();

        int off = pass * 64;

        for (int t = 0; t < BIT_TAU; t++) {
            const int32_t *RESTRICT p = ptrs[t] + off;
            a0 = _mm256_add_epi32(a0, _mm256_loadu_si256((const __m256i *)(p + 0)));
            a1 = _mm256_add_epi32(a1, _mm256_loadu_si256((const __m256i *)(p + 8)));
            a2 = _mm256_add_epi32(a2, _mm256_loadu_si256((const __m256i *)(p + 16)));
            a3 = _mm256_add_epi32(a3, _mm256_loadu_si256((const __m256i *)(p + 24)));
            a4 = _mm256_add_epi32(a4, _mm256_loadu_si256((const __m256i *)(p + 32)));
            a5 = _mm256_add_epi32(a5, _mm256_loadu_si256((const __m256i *)(p + 40)));
            a6 = _mm256_add_epi32(a6, _mm256_loadu_si256((const __m256i *)(p + 48)));
            a7 = _mm256_add_epi32(a7, _mm256_loadu_si256((const __m256i *)(p + 56)));
        }

        /* result = (acc ^ smask) − smask  →  +acc (s=+1) or −acc (s=−1) */
        #define ST(reg, k) _mm256_store_si256((__m256i *)(result->coeffs + off + (k)), \
            _mm256_sub_epi32(_mm256_xor_si256((reg), smask), smask))
        ST(a0, 0); ST(a1, 8); ST(a2, 16); ST(a3, 24);
        ST(a4, 32); ST(a5, 40); ST(a6, 48); ST(a7, 56);
        #undef ST
    }
}

void poly_cneg(poly *a, uint8_t b_val) {
    int32_t mask = -((int32_t)b_val);
    __m256i vmask = _mm256_set1_epi32(mask);
    PRAGMA_UNROLL_4
    for (int i = 0; i < BIT_N; i += 8) {
        __m256i v = _mm256_load_si256((__m256i *)&a->coeffs[i]);
        v = _mm256_sub_epi32(_mm256_xor_si256(v, vmask), vmask);
        _mm256_store_si256((__m256i *)&a->coeffs[i], v);
    }
}

int poly_check_reject_highbits_w1_sparse(const poly *w1, const poly *w0,
                                          const sparse_challenge *c) {
    uint32_t reject_flag = 0;
    for (int t = 0; t < BIT_TAU; t++) {
        int i = c->pos[t];
        int32_t hb;
        int32_t w_prime_val = (int32_t)w0->coeffs[i] + c->sign[t];
        decompose_hint(&hb, w_prime_val);
        reject_flag |= (uint32_t)((uint16_t)w1->coeffs[i] ^ (uint16_t)hb);
    }
    return (reject_flag != 0) ? 1 : 0;
}
