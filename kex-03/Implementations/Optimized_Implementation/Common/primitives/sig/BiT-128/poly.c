/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
*/
#include "poly.h"
#include "ntt_avx.h"
#include "symmetric.h"
#include "align.h"
#include <immintrin.h>
#include <stddef.h>

static const int16_t map_S1[4] = {-1, 0, 1, 0};

static size_t rej_S1(int16_t *a, size_t len, const uint8_t *buf, size_t buflen) {
    size_t ctr = 0, pos = 0;
    while (ctr < len && pos < buflen) {
        uint8_t val = buf[pos++];
        uint8_t t0 = val & 0x03, t1 = (val >> 2) & 0x03;
        uint8_t t2 = (val >> 4) & 0x03, t3 = (val >> 6) & 0x03;
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

uint16_t reduce_barrett(int32_t a) {
    int32_t t = (int32_t)(((int64_t)a * BIT_BARRETT_MULT) >> BIT_BARRETT_SHIFT);
    a -= t * BIT_Q;
    /* Barrett estimate may be off by one for deeply negative inputs. */
    a += (a >> 31) & BIT_Q;
    a -= BIT_Q;
    a += (a >> 31) & BIT_Q;
    return (uint16_t)a;
}

void poly_sample_S1(poly *a, const uint8_t *seed, uint16_t *nonce) {
    uint8_t buf[BIT_SAMPLE_S1_BUFLEN];
    bit_xof256_state stream;
    size_t ctr;
    bit_xof256_init(&stream, seed, BIT_SEEDBYTES, *nonce);
    bit_xof256_squeeze(&stream, buf, sizeof(buf));
    ctr = rej_S1(a->coeffs, BIT_N, buf, sizeof(buf));
    while (ctr < BIT_N) {
        bit_xof256_squeeze(&stream, buf, sizeof(buf));
        ctr += rej_S1(a->coeffs + ctr, BIT_N - ctr, buf, sizeof(buf));
    }
    *nonce = (uint16_t)(*nonce + 1U);
    bit_xof256_zeroize(&stream);
}

#define S1_BUFLEN BIT_SAMPLE_S1_BUFLEN

void poly_sample_S1_x4(poly *p0, poly *p1, poly *p2, poly *p3,
                       const uint8_t *seed, uint16_t *nonce)
{
    uint8_t ALIGNED_32 buf0[S1_BUFLEN], ALIGNED_32 buf1[S1_BUFLEN];
    uint8_t ALIGNED_32 buf2[S1_BUFLEN], ALIGNED_32 buf3[S1_BUFLEN];
    uint16_t n0 = *nonce, n1 = (uint16_t)(*nonce + 1U);
    uint16_t n2 = (uint16_t)(*nonce + 2U), n3 = (uint16_t)(*nonce + 3U);
    *nonce = (uint16_t)(*nonce + 4U);
    bit_xof256x4(buf0, buf1, buf2, buf3, S1_BUFLEN, seed, BIT_SEEDBYTES, n0, n1, n2, n3);
    if (rej_S1(p0->coeffs, BIT_N, buf0, S1_BUFLEN) < BIT_N) poly_sample_S1(p0, seed, &n0);
    if (rej_S1(p1->coeffs, BIT_N, buf1, S1_BUFLEN) < BIT_N) poly_sample_S1(p1, seed, &n1);
    if (rej_S1(p2->coeffs, BIT_N, buf2, S1_BUFLEN) < BIT_N) poly_sample_S1(p2, seed, &n2);
    if (rej_S1(p3->coeffs, BIT_N, buf3, S1_BUFLEN) < BIT_N) poly_sample_S1(p3, seed, &n3);
}

void poly_sample_S1_x2(poly *p0, poly *p1, const uint8_t *seed, uint16_t *nonce)
{
    uint8_t ALIGNED_32 buf0[S1_BUFLEN], ALIGNED_32 buf1[S1_BUFLEN];
    uint16_t n0 = *nonce, n1 = (uint16_t)(*nonce + 1U);
    *nonce = (uint16_t)(*nonce + 2U);
    bit_xof256x2(buf0, buf1, S1_BUFLEN, seed, BIT_SEEDBYTES, n0, n1);
    if (rej_S1(p0->coeffs, BIT_N, buf0, S1_BUFLEN) < BIT_N) poly_sample_S1(p0, seed, &n0);
    if (rej_S1(p1->coeffs, BIT_N, buf1, S1_BUFLEN) < BIT_N) poly_sample_S1(p1, seed, &n1);
}

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
    for (int i = 0; i < BIT_N; i += 16)
        _mm256_store_si256((__m256i *)(&result->coeffs[i]), z);
}

void poly_add(poly *result, const poly *a, const poly *b) {
    const __m256i q = _mm256_set1_epi16((short)BIT_Q);
    const __m256i hq = _mm256_set1_epi16((short)BIT_Q_HALF);
    const __m256i mhq = _mm256_set1_epi16((short)-BIT_Q_HALF);
    for (int i = 0; i < BIT_N / 16; i++) {
        __m256i va = _mm256_load_si256((const __m256i *)&a->coeffs[16*i]);
        __m256i vb = _mm256_load_si256((const __m256i *)&b->coeffs[16*i]);
        __m256i v = _mm256_add_epi16(va, vb);
        __m256i m1 = _mm256_cmpgt_epi16(mhq, v), m2 = _mm256_cmpgt_epi16(v, hq);
        v = _mm256_add_epi16(v, _mm256_and_si256(m1, q));
        v = _mm256_sub_epi16(v, _mm256_and_si256(m2, q));
        _mm256_store_si256((__m256i *)&result->coeffs[16*i], v);
    }
}

void poly_sub(poly *result, const poly *a, const poly *b) {
    const __m256i q = _mm256_set1_epi16((short)BIT_Q);
    const __m256i hq = _mm256_set1_epi16((short)BIT_Q_HALF);
    const __m256i mhq = _mm256_set1_epi16((short)-BIT_Q_HALF);
    for (int i = 0; i < BIT_N / 16; i++) {
        __m256i va = _mm256_load_si256((const __m256i *)&a->coeffs[16*i]);
        __m256i vb = _mm256_load_si256((const __m256i *)&b->coeffs[16*i]);
        __m256i v = _mm256_sub_epi16(va, vb);
        __m256i m1 = _mm256_cmpgt_epi16(mhq, v), m2 = _mm256_cmpgt_epi16(v, hq);
        v = _mm256_add_epi16(v, _mm256_and_si256(m1, q));
        v = _mm256_sub_epi16(v, _mm256_and_si256(m2, q));
        _mm256_store_si256((__m256i *)&result->coeffs[16*i], v);
    }
}

void poly_add_eta1(poly *result, const poly *a, const poly *eta1) {
    /* eta1 ∈ {-1,0,1}; a ∈ [-Q/2,Q/2]; sum ∈ [-(Q/2+1), Q/2+1].
     * One signed-reduction round. AVX2: 16×int16 per vector. */
    const __m256i q   = _mm256_set1_epi16((short)BIT_Q);
    const __m256i hq  = _mm256_set1_epi16((short)BIT_Q_HALF);
    const __m256i mhq = _mm256_set1_epi16((short)-BIT_Q_HALF);
    PRAGMA_UNROLL_4
    for (int i = 0; i < BIT_N; i += 16) {
        __m256i va = _mm256_load_si256((const __m256i *)&a->coeffs[i]);
        __m256i ve = _mm256_load_si256((const __m256i *)&eta1->coeffs[i]);
        __m256i v  = _mm256_add_epi16(va, ve);
        __m256i m1 = _mm256_cmpgt_epi16(mhq, v);
        __m256i m2 = _mm256_cmpgt_epi16(v, hq);
        v = _mm256_add_epi16(v, _mm256_and_si256(m1, q));
        v = _mm256_sub_epi16(v, _mm256_and_si256(m2, q));
        _mm256_store_si256((__m256i *)&result->coeffs[i], v);
    }
}

/* Barrett division by BIT_GAMMA2=955 for 8 int32 lanes: hb = floor((v+477)/955) */
static inline __m256i mm256_div_gamma2_x8(__m256i v) {
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
    const __m256i v_q   = _mm256_set1_epi16((short)BIT_Q);
    const __m256i v_ah  = _mm256_set1_epi32((int32_t)ALPHA_HINT);
    const __m256i v_one = _mm256_set1_epi32(1);

    PRAGMA_UNROLL_4
    for (int i = 0; i < BIT_N; i += 16) {
        __m256i r = _mm256_load_si256((const __m256i *)&w->coeffs[i]);
        __m256i sign = _mm256_srai_epi16(r, 15);
        __m256i val  = _mm256_add_epi16(r, _mm256_and_si256(sign, v_q));

        /* Process lower 8 lanes */
        __m256i v_lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(val));
        __m256i hb_lo = mm256_div_gamma2_x8(v_lo);

        /* Process upper 8 lanes */
        __m256i v_hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(val, 1));
        __m256i hb_hi = mm256_div_gamma2_x8(v_hi);

        /* edgecase clamp on both halves */
        __m256i wrap_lo = _mm256_srai_epi32(_mm256_sub_epi32(v_ah,
                          _mm256_add_epi32(hb_lo, v_one)), 31);
        hb_lo = _mm256_sub_epi32(hb_lo, _mm256_and_si256(v_ah, wrap_lo));

        __m256i wrap_hi = _mm256_srai_epi32(_mm256_sub_epi32(v_ah,
                          _mm256_add_epi32(hb_hi, v_one)), 31);
        hb_hi = _mm256_sub_epi32(hb_hi, _mm256_and_si256(v_ah, wrap_hi));

        /* Pack lo and hi: packs_epi32 → [lo0..3,hi0..3 | lo4..7,hi4..7].
         * Permute to [lo0..7 | hi0..7], then store both halves. */
        __m256i packed = _mm256_packs_epi32(hb_lo, hb_hi);
        packed = _mm256_permute4x64_epi64(packed, _MM_SHUFFLE(3,1,2,0));
        _mm_storeu_si128((__m128i *)&w1->coeffs[i],
                         _mm256_castsi256_si128(packed));
        _mm_storeu_si128((__m128i *)&w1->coeffs[i + 8],
                         _mm256_extracti128_si256(packed, 1));
    }
}

void poly_decompose_b(poly *b1, poly *b0, const poly *b) {
    const __m256i q = _mm256_set1_epi16((short)BIT_Q);
    const __m256i qh = _mm256_set1_epi16((short)((BIT_Q - 1) / 2));
    const __m256i gh = _mm256_set1_epi16((short)(BIT_GAMMA_B >> 1));
    const __m256i gb = _mm256_set1_epi16((short)BIT_GAMMA_B);
    for (int i = 0; i < BIT_N / 16; i++) {
        __m256i r = _mm256_load_si256((const __m256i *)&b->coeffs[16*i]);
        __m256i sign = _mm256_srai_epi16(r, 15);
        __m256i a = _mm256_add_epi16(r, _mm256_and_si256(sign, q));
        __m256i hb = _mm256_srli_epi16(_mm256_add_epi16(a, gh), 4);
        __m256i low = _mm256_sub_epi16(a, _mm256_mullo_epi16(hb, gb));
        __m256i over = _mm256_cmpgt_epi16(low, qh);
        low = _mm256_sub_epi16(low, _mm256_and_si256(over, q));
        _mm256_store_si256((__m256i *)&b1->coeffs[16*i], hb);
        _mm256_store_si256((__m256i *)&b0->coeffs[16*i], low);
    }
}

/* AVX2 6-bit packer: 16 coeffs (low 6 bits each) → 12 bytes, per iteration.
 * Bit-identical to the scalar version (both keep the low 6 bits of each int16).
 *   madd  : pairs (c_even | c_odd<<6)        → 8 x 12-bit in 32-bit lanes
 *   or/sr : pairs (d_even | d_odd<<12)       → 4 x 24-bit in low 3B of 64-bit lanes
 *   shuf  : gather the 3 low bytes of each 64-bit lane within both 128-bit halves
 * Store is exactly 12 bytes/iter so the final group lands on the 192-byte boundary. */
void poly_pack_w1(unsigned char *r, const poly *a) {
    const __m256i mult   = _mm256_set1_epi32((int32_t)0x00400001); /* 16-bit lanes [1,64,...] */
    const __m256i mask3f = _mm256_set1_epi16(0x003F);
    const __m256i shuf   = _mm256_setr_epi8(
        0, 1, 2, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 1, 2, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);

    for (int i = 0; i < BIT_N / 16; i++) {
        __m256i c = _mm256_load_si256((const __m256i *)&a->coeffs[16*i]);
        c = _mm256_and_si256(c, mask3f);
        __m256i m = _mm256_madd_epi16(c, mult);                       /* 8 x 12-bit */
        __m256i e = _mm256_or_si256(m, _mm256_srli_epi64(m, 20));     /* 4 x 24-bit */
        __m256i s = _mm256_shuffle_epi8(e, shuf);
        __m128i lo = _mm256_castsi256_si128(s);                       /* 6 bytes @ [0..5] */
        __m128i hi = _mm256_extracti128_si256(s, 1);                  /* 6 bytes @ [0..5] */
        __m128i combined = _mm_or_si128(lo, _mm_bslli_si128(hi, 6));  /* 12 bytes @ [0..11] */
        _mm_storel_epi64((__m128i *)(r + 12*i), combined);            /* bytes 0..7 */
        *(uint32_t *)(r + 12*i + 8) = (uint32_t)_mm_extract_epi32(combined, 2); /* bytes 8..11 */
    }
}

void poly_challenge(poly *c, const unsigned char seed[BIT_CHALLENGEBYTES]) {
    unsigned int i, b, pos;
    uint8_t buf[BIT_XOF256_RATE];
    bit_xof256_ctx ctx;
    bit_xof256_ctx_init(&ctx, seed, BIT_CHALLENGEBYTES);
    bit_xof256_ctx_squeezeblocks(&ctx, buf, 1);
    pos = 0;
    for (i = 0; i < BIT_N; ++i) c->coeffs[i] = 0;
    for (i = BIT_N - BIT_TAU; i < BIT_N; ++i) {
        do {
            if (pos >= BIT_XOF256_RATE) { bit_xof256_ctx_squeezeblocks(&ctx, buf, 1); pos = 0; }
            b = buf[pos++];
        } while (b > i);
        c->coeffs[i] = c->coeffs[b];
        c->coeffs[b] = 1;  /* {0,1} challenge: no negative signs */
    }
    bit_xof256_ctx_zeroize(&ctx);
}

void poly_challenge_to_sparse(sparse_challenge *s, const poly *c) {
    int ctr = 0; __m256i vzero = _mm256_setzero_si256();
    for (int i = 0; i < BIT_N; i += 16) {
        __m256i v = _mm256_load_si256((__m256i*)&c->coeffs[i]);
        uint32_t nz = ~_mm256_movemask_epi8(_mm256_cmpeq_epi16(v, vzero)) & 0x55555555u;
        while (nz && ctr < BIT_TAU) {
            int k = __builtin_ctz(nz) >> 1; nz &= nz - 1;
            int idx = i + k; s->pos[ctr] = (uint8_t)idx; s->sign[ctr] = (int8_t)c->coeffs[idx]; ctr++;
        }
        if (ctr == BIT_TAU) break;
    }
}

/*
 * Constant-time challenge multiply (challenge ∈ {0,1}; poly_cneg gives the
 * uniform bimodal sign s = (−1)^b to every term).  Single extended array, no
 * per-position sign select, uniform sign applied branch-free at the store —
 * the secret bit b no longer steers control flow.  Numerically identical to
 * the scalar reference (KAT unchanged), and faster (one array, no select).
 */
void poly_mul_challenge_no_reduce(poly *result, const poly *a, const sparse_challenge *c) {
    ALIGNED_32 int16_t A_ext[BIT_N * 2];   /* [ −A ‖ A ] */
    const __m256i zero = _mm256_setzero_si256();
    for (int i = 0; i < BIT_N; i += 16) {
        __m256i val = _mm256_load_si256((const __m256i *)(a->coeffs + i));
        __m256i neg = _mm256_sub_epi16(zero, val);
        _mm256_store_si256((__m256i *)(A_ext + i), neg);
        _mm256_store_si256((__m256i *)(A_ext + BIT_N + i), val);
    }
    const int16_t *RESTRICT ptrs[BIT_TAU];
    for (int t = 0; t < BIT_TAU; t++)
        ptrs[t] = A_ext + BIT_N - c->pos[t];

    /* uniform sign s = (−1)^b → branch-free mask: 0 (s=+1) or −1 (s=−1) */
    const __m256i smask = _mm256_set1_epi16((int16_t)((int32_t)c->sign[0] >> 31));

    for (int pass = 0; pass < 2; pass++) {
        __m256i a0 = _mm256_setzero_si256(), a1 = _mm256_setzero_si256();
        __m256i a2 = _mm256_setzero_si256(), a3 = _mm256_setzero_si256();
        __m256i a4 = _mm256_setzero_si256(), a5 = _mm256_setzero_si256();
        __m256i a6 = _mm256_setzero_si256(), a7 = _mm256_setzero_si256();
        int off = pass * 128;
        for (int t = 0; t < BIT_TAU; t++) {
            const int16_t *RESTRICT p = ptrs[t] + off;
            a0 = _mm256_add_epi16(a0, _mm256_loadu_si256((const __m256i *)(p + 0)));
            a1 = _mm256_add_epi16(a1, _mm256_loadu_si256((const __m256i *)(p + 16)));
            a2 = _mm256_add_epi16(a2, _mm256_loadu_si256((const __m256i *)(p + 32)));
            a3 = _mm256_add_epi16(a3, _mm256_loadu_si256((const __m256i *)(p + 48)));
            a4 = _mm256_add_epi16(a4, _mm256_loadu_si256((const __m256i *)(p + 64)));
            a5 = _mm256_add_epi16(a5, _mm256_loadu_si256((const __m256i *)(p + 80)));
            a6 = _mm256_add_epi16(a6, _mm256_loadu_si256((const __m256i *)(p + 96)));
            a7 = _mm256_add_epi16(a7, _mm256_loadu_si256((const __m256i *)(p + 112)));
        }
        /* result = (acc ^ smask) − smask  →  +acc (s=+1) or −acc (s=−1) */
        #define ST16(reg, k) _mm256_store_si256((__m256i *)(result->coeffs + off + (k)), \
            _mm256_sub_epi16(_mm256_xor_si256((reg), smask), smask))
        ST16(a0, 0); ST16(a1, 16); ST16(a2, 32); ST16(a3, 48);
        ST16(a4, 64); ST16(a5, 80); ST16(a6, 96); ST16(a7, 112);
        #undef ST16
    }
}

void poly_cneg(poly *a, uint8_t b_val) {
    int32_t mask = -((int32_t)b_val);
    __m256i vmask = _mm256_set1_epi16((int16_t)mask);
    for (int i = 0; i < BIT_N; i += 16) {
        __m256i v = _mm256_load_si256((__m256i*)&a->coeffs[i]);
        v = _mm256_sub_epi16(_mm256_xor_si256(v, vmask), vmask);
        _mm256_store_si256((__m256i*)&a->coeffs[i], v);
    }
}

int poly_check_reject_highbits_w1_sparse(const poly *w1, const poly *w0, const sparse_challenge *c) {
    uint32_t reject_flag = 0;
    for (int t = 0; t < BIT_TAU; t++) {
        int i = c->pos[t]; int16_t hb;
        int32_t w_prime_val = (int32_t)w0->coeffs[i] + c->sign[t];
        decompose_hint(&hb, w_prime_val);
        reject_flag |= (uint32_t)((uint16_t)w1->coeffs[i] ^ (uint16_t)hb);
    }
    return (reject_flag != 0) ? 1 : 0;
}
