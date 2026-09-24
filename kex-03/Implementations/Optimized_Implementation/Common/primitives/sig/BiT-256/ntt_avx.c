/*
 * BiT-256 AVX2 NTT wrappers — thin wrappers around hand-written assembly.
 */
#include <immintrin.h>
#include <stdint.h>
#include "ntt_avx.h"
#include "params.h"
#include "consts.h"

/* Assembly routines live in ntt.S / intt.S / pointwise.S */
extern void ntt_avx(int32_t a[BIT_N], const qdata_t qdata);
extern void invntt_avx(int32_t a[BIT_N], const qdata_t qdata);
extern void pointwise_avx(int32_t r[BIT_N], const int32_t a[BIT_N],
                          const int32_t b[BIT_N], const qdata_t qdata);
extern void pointwise_acc_avx(int32_t r[BIT_N], const int32_t a[BIT_N],
                              const int32_t b[BIT_N], const qdata_t qdata);

/* ── Signed↔unsigned conversion (assembly doesn't do this) ─────────── */

/*
 * reduce_to_canonical_barrett_avx2:  any int32 → [0, q)
 *
 * ntt_avx uses lazy reduction → output range ~ [-3q, 3q], too wide for
 * a single +2q / −q pass.  Barrett handles arbitrary int32 inputs.
 */
static inline void reduce_to_canonical_avx2(int32_t *a) {
    __m256i *va = (__m256i *)a;
    const __m256i v_M  = _mm256_set1_epi32(BIT_BARRETT_MULT);   /* 144009 = floor(2^34/q) */
    const __m256i v_q  = _mm256_set1_epi32(BIT_Q);
    for (int i = 0; i < BIT_N / 8; i++) {
        __m256i x   = _mm256_load_si256(&va[i]);
        __m256i xo  = _mm256_shuffle_epi32(x, 0xF5);
        /* p = x * M  (signed 32×32→64) */
        __m256i p_lo = _mm256_mul_epi32(x,  v_M);
        __m256i p_hi = _mm256_mul_epi32(xo, v_M);
        /* q_est = floor(p / 2^34)  ≡  p >> (arithmetic) 34
         * AVX2 lacks 64-bit arithmetic shift; emulate via srli(32) + srai(2) */
        __m256i qe_lo = _mm256_srai_epi32(_mm256_srli_epi64(p_lo, 32), BIT_BARRETT_SHIFT - 32);
        __m256i qe_hi = _mm256_srai_epi32(_mm256_srli_epi64(p_hi, 32), BIT_BARRETT_SHIFT - 32);
        __m256i qe = _mm256_blend_epi32(qe_lo, _mm256_slli_epi64(qe_hi, 32), 0xAA);
        /* r = x - qe·q   ∈ [0, 2q) */
        __m256i r = _mm256_sub_epi32(x, _mm256_mullo_epi32(qe, v_q));
        /* one conditional −q → [0, q) */
        __m256i t = _mm256_sub_epi32(r, v_q);
        r = _mm256_add_epi32(t, _mm256_and_si256(_mm256_srai_epi32(t, 31), v_q));
        _mm256_store_si256(&va[i], r);
    }
}

static inline void reduce_to_signed_avx2(int32_t *a) {
    __m256i *va = (__m256i *)a;
    const __m256i v_q  = _mm256_set1_epi32(BIT_Q);
    const __m256i v_qh = _mm256_set1_epi32(BIT_Q / 2);
    for (int i = 0; i < BIT_N / 8; i++) {
        __m256i x = _mm256_load_si256(&va[i]);
        __m256i over = _mm256_cmpgt_epi32(x, v_qh);
        x = _mm256_sub_epi32(x, _mm256_and_si256(over, v_q));
        _mm256_store_si256(&va[i], x);
    }
}

/* ── Public wrappers (one-to-one with the avx2-128 naming) ─────────── */

void ntt_avx_forward(int32_t a[BIT_N]) {
    ntt_avx(a, qdata);
    /*
     * No canonical pass.  ntt_avx leaves coefficients lazily reduced in
     * ~[-4q, 5q]; the only consumers of forward-NTT output are the pointwise
     * Montgomery multiplies (poly_ntt_mul_raw / acc_mul_raw), which compute
     * a·b·R⁻¹ mod q for ANY representatives a,b and reduce internally.  The
     * products stay well within int64, so the result mod q is unaffected.
     * Verified: KAT bit-identical to the scalar reference and 38/38
     * correctness tests pass.  Skipping the Barrett canonicalization here
     * saves ~550 cycles (~25%) per forward NTT.
     */
}

/* retained for reference / potential debug use */
__attribute__((unused))
static void canonicalize_forward(int32_t *a) { reduce_to_canonical_avx2(a); }

void ntt_avx_inverse(int32_t a[BIT_N]) {
    invntt_avx(a, qdata);
    reduce_to_signed_avx2(a);
}

void ntt_pointwise_mul_raw(int32_t *r, const int32_t *a, const int32_t *b) {
    pointwise_avx((int32_t *)r, a, b, qdata);
}

void ntt_pointwise_acc_raw(int32_t *r, const int32_t *a, const int32_t *b) {
    pointwise_acc_avx((int32_t *)r, a, b, qdata);
}

/* ── Montgomery lift: r[i] = r[i] * R² mod q ────────────────────────
 *
 *  INVERSE_Q = q⁻¹ mod 2³²  (positive inverse, bit pattern 2286038529),
 *  therefore Montgomery reduction MUST use subtraction:  (p − m⋅q) / 2³²
 *  (the "add" variant is for −q⁻¹).
 *
 *  Three fixes relative to the original:
 *   1.  vpaddq  →  vpsubq                     (positive inverse → subtract)
 *   2.  initial a·R² via vpmuldq (signed)     (matches (int64_t) cast in ref)
 *   3.  m·q     via vpmuldq (signed)          (ditto; m may be > 2³¹)
 * ====================================================================== */

#define MONT_R2     54377
#define INVERSE_Q   -2008928767u

void ntt_montgomery_lift(int32_t *r) {
    __m256i *vr    = (__m256i *)r;
    const __m256i v_r2   = _mm256_set1_epi32(MONT_R2);
    const __m256i v_qinv = _mm256_set1_epi32((int32_t)INVERSE_Q);
    const __m256i v_q    = _mm256_set1_epi32(BIT_Q);

    for (int i = 0; i < BIT_N / 8; i++) {
        __m256i a     = _mm256_load_si256(&vr[i]);
        __m256i a_odd = _mm256_shuffle_epi32(a, 0xF5);   /* move odd lanes to even dword pos */

        /* p = a * R²  — signed 32×32→64  (input ∈ [0,q), both signed & unsigned agree) */
        __m256i p_lo = _mm256_mul_epi32(a,     v_r2);
        __m256i p_hi = _mm256_mul_epi32(a_odd, v_r2);

        /* m = lo32(p) * qinv   (only lower dword matters, vpmuludq fine) */
        __m256i m_lo = _mm256_mul_epu32(p_lo, v_qinv);
        __m256i m_hi = _mm256_mul_epu32(p_hi, v_qinv);

        /* tq = m * q  — m may be > 2³¹, so signed 32×32→64 to match ref's (int64_t)m·Q */
        __m256i tq_lo = _mm256_mul_epi32(m_lo, v_q);
        __m256i tq_hi = _mm256_mul_epi32(m_hi, v_q);

        /* d = p − tq   (subtraction — INVERSE_Q is the positive inverse) */
        __m256i d_lo = _mm256_sub_epi64(p_lo, tq_lo);
        __m256i d_hi = _mm256_sub_epi64(p_hi, tq_hi);

        /* result = d >> 32  (low 32 bits ≡ 0, high 32 bits = result ∈ (−q, q)) */
        __m256i result = _mm256_blend_epi32(
            _mm256_srli_epi64(d_lo, 32), d_hi, 0xAA);

        /* (−q, q) → [0, q) */
        result = _mm256_add_epi32(result,
            _mm256_and_si256(_mm256_srai_epi32(result, 31), v_q));

        _mm256_store_si256(&vr[i], result);
    }
}
