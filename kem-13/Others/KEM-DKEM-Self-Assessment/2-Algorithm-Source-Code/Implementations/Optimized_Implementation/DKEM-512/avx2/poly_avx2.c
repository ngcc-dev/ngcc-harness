/*
 * AVX2 vectorized poly arithmetic for all DKE modes.
 * Only compiled when DKE_USE_AVX2 is defined.
 */
#include "../parameters.h"

#if defined(DKE_USE_AVX2)

#include <immintrin.h>
#include <stdint.h>
#include "../poly.h"
#include "../ntt.h"
#include "../reduce.h"

/* Mode-independent AVX2 helpers (same as in ntt_avx2.c, but static here) */

static inline __m256i poly_avx2_montgomery_reduce(__m256i lo, __m256i hi) {
    __m256i qinv = _mm256_set1_epi16((int16_t)DKE_QINV);
    __m256i q    = _mm256_set1_epi16((int16_t)DKE_Q);
    __m256i t    = _mm256_mullo_epi16(lo, qinv);
    t = _mm256_mulhi_epi16(t, q);
    return _mm256_sub_epi16(hi, t);
}

static inline __m256i poly_avx2_fqmul(__m256i a, __m256i b) {
    __m256i lo = _mm256_mullo_epi16(a, b);
    __m256i hi = _mm256_mulhi_epi16(a, b);
    return poly_avx2_montgomery_reduce(lo, hi);
}

static inline __m256i poly_avx2_barrett_reduce(__m256i a) {
    const int16_t v = (int16_t)(((1 << 26) + DKE_Q / 2) / DKE_Q);
    __m256i vv = _mm256_set1_epi16(v);
    __m256i q  = _mm256_set1_epi16((int16_t)DKE_Q);
    __m256i t;
    t = _mm256_mulhi_epi16(a, vv);
    t = _mm256_add_epi16(t, _mm256_set1_epi16(1 << 9));
    t = _mm256_srai_epi16(t, 10);
    t = _mm256_mullo_epi16(t, q);
    return _mm256_sub_epi16(a, t);
}

#if DKE_MODE != 512 && defined(_MSC_VER) && !defined(DKE_AVX2_NTT_INTRINSIC)
/* MSVC: Assembly reduce/tomont for N=256 */
extern void dke_reduce_avx2(int16_t *r, const int16_t *qdata);
extern void dke_tomont_avx2(int16_t *r, const int16_t *qdata);
extern const int16_t dke_qdata[];

void DKE_poly_reduce(poly *pol) {
    dke_reduce_avx2(pol->coeffs, dke_qdata);
}

void DKE_poly_tomont(poly *pol) {
    dke_tomont_avx2(pol->coeffs, dke_qdata);
}
#elif DKE_MODE != 512 && defined(DKM_LINUX) && !defined(DKE_AVX2_NTT_INTRINSIC)
/* Linux: GAS assembly reduce/tomont for N=256 */
extern void dke_reduce_avx2_linux(int16_t *r, const int16_t *qdata);
extern void dke_tomont_avx2_linux(int16_t *r, const int16_t *qdata);
extern const int16_t dke_qdata[];

void DKE_poly_reduce(poly *pol) {
    dke_reduce_avx2_linux(pol->coeffs, dke_qdata);
}

void DKE_poly_tomont(poly *pol) {
    dke_tomont_avx2_linux(pol->coeffs, dke_qdata);
}
#else
/* DKE-512 or non-MSVC: intrinsics */
void DKE_poly_reduce(poly *pol) {
    unsigned int i;
#if defined(DKE_AVX2_NTT256_ASM) && DKE_N == 256
    /* ML-KEM's ntttobytes assumes [0,q); use ML-KEM's reduce (same range) so
     * serialized NTT-domain polys are canonical. (The portable barrett yields a
     * CENTERED rep, which ntttobytes would mis-pack.) */
    {
        extern void dke_k_reduce_avx(int16_t *, const int16_t *);
        extern const int16_t dke_k_qdata[];
        dke_k_reduce_avx(pol->coeffs, dke_k_qdata);
        return;
    }
#endif
    for (i = 0; i < DKE_N; i += 16) {
        __m256i v = _mm256_load_si256((__m256i *)&pol->coeffs[i]);
        v = poly_avx2_barrett_reduce(v);
        _mm256_store_si256((__m256i *)&pol->coeffs[i], v);
    }
}

void DKE_poly_tomont(poly *pol) {
    unsigned int i;
    __m256i f = _mm256_set1_epi16((int16_t)((1ULL << 32) % DKE_Q));
    for (i = 0; i < DKE_N; i += 16) {
        __m256i v = _mm256_load_si256((__m256i *)&pol->coeffs[i]);
        v = poly_avx2_fqmul(v, f);
        _mm256_store_si256((__m256i *)&pol->coeffs[i], v);
    }
}
#endif

void DKE_poly_add(poly *r, const poly *a, const poly *b) {
    unsigned int i;
    for (i = 0; i < DKE_N; i += 16) {
        __m256i va = _mm256_load_si256((__m256i *)&a->coeffs[i]);
        __m256i vb = _mm256_load_si256((__m256i *)&b->coeffs[i]);
        _mm256_store_si256((__m256i *)&r->coeffs[i], _mm256_add_epi16(va, vb));
    }
}

void DKE_poly_sub(poly *r, const poly *a, const poly *b) {
    unsigned int i;
    for (i = 0; i < DKE_N; i += 16) {
        __m256i va = _mm256_load_si256((__m256i *)&a->coeffs[i]);
        __m256i vb = _mm256_load_si256((__m256i *)&b->coeffs[i]);
        _mm256_store_si256((__m256i *)&r->coeffs[i], _mm256_sub_epi16(va, vb));
    }
}

/*
 * AVX2 poly_basemul_montgomery.
 * DKE-128/256 (N=256): uses MASM assembly (basemul_avx2.asm) when available.
 * DKE-512 (N=512) or fallback: uses intrinsics.
 */
#if DKE_MODE != 512 && defined(_MSC_VER) && !defined(DKE_AVX2_NTT_INTRINSIC)
/* MSVC: ML-KEM-style basemul for packed NTT format (N=256) */
extern void dke_basemul_dkek_avx2(int16_t *r, const int16_t *a, const int16_t *b,
                                    const int16_t *qdata);
extern const int16_t dke_qdata[];

void DKE_poly_basemul_montgomery(poly *res, const poly *a, const poly *b) {
    dke_basemul_dkek_avx2(res->coeffs, a->coeffs, b->coeffs, dke_qdata);
}
#elif DKE_MODE != 512 && defined(DKM_LINUX) && !defined(DKE_AVX2_NTT_INTRINSIC)
/* Linux: ML-KEM-style basemul for packed NTT format (N=256) */
extern void dke_basemul_dkek_avx2_linux(int16_t *r, const int16_t *a, const int16_t *b,
                                          const int16_t *qdata);
extern const int16_t dke_qdata[];

void DKE_poly_basemul_montgomery(poly *res, const poly *a, const poly *b) {
    dke_basemul_dkek_avx2_linux(res->coeffs, a->coeffs, b->coeffs, dke_qdata);
}
#elif DKE_MODE != 512
/* Non-MSVC, N=256: intrinsics basemul (or vendored ML-KEM asm when opted in) */
void DKE_poly_basemul_montgomery(poly *res, const poly *a, const poly *b) {
    unsigned int i, j;
    const int16_t *zp = DKE_zetas + DKE_NTT_ZETAS_LEN / 2;
#if defined(DKE_AVX2_NTT256_ASM)
    {
        extern void dke_k_basemul_avx(int16_t *, const int16_t *, const int16_t *,
                                      const int16_t *);
        extern const int16_t dke_k_qdata[];
        dke_k_basemul_avx(res->coeffs, a->coeffs, b->coeffs, dke_k_qdata);
        (void)zp;
        return;
    }
#endif
    /* Loop-invariant helpers to build the per-group zeta vector
     * [z,z,-z,-z, ...] cheaply (2 pshufb + 1 psignw) instead of a 16-way
     * _mm256_set_epi16 each iteration. */
    const __m128i zsh_lo = _mm_setr_epi8(0,1,0,1,0,1,0,1, 2,3,2,3,2,3,2,3);
    const __m128i zsh_hi = _mm_setr_epi8(4,5,4,5,4,5,4,5, 6,7,6,7,6,7,6,7);
    const __m256i zsign  = _mm256_set_epi16(-1,-1,1,1, -1,-1,1,1,
                                            -1,-1,1,1, -1,-1,1,1);

    for (i = 0; i < DKE_N / 4; i += 4) {
        /* Load 4 groups of 4 coefficients = 16 int16_t */
        __m256i av = _mm256_load_si256((__m256i *)&a->coeffs[4 * i]);
        __m256i bv = _mm256_load_si256((__m256i *)&b->coeffs[4 * i]);

        /* Separate even (a0,a2) and odd (a1,a3) within each 32-bit word.
         * a = [a0,a1, a2,a3, a4,a5, a6,a7, a8,a9, a10,a11, a12,a13, a14,a15]
         * We need:
         *   a_lo = [a0,a0, a2,a2, a4,a4, a6,a6, a8,a8, a10,a10, a12,a12, a14,a14]
         *   a_hi = [a1,a1, a3,a3, a5,a5, a7,a7, a9,a9, a11,a11, a13,a13, a15,a15]
         */
        /* Broadcast low 16 bits of each 32-bit word */
        __m256i a0 = _mm256_shufflelo_epi16(av, 0xA0); /* 10 10 00 00 = a0,a0,a2,a2 in low64 */
        a0 = _mm256_shufflehi_epi16(a0, 0xA0);
        __m256i a1 = _mm256_shufflelo_epi16(av, 0xF5); /* 11 11 01 01 = a1,a1,a3,a3 in low64 */
        a1 = _mm256_shufflehi_epi16(a1, 0xF5);

        __m256i b0 = _mm256_shufflelo_epi16(bv, 0xA0);
        b0 = _mm256_shufflehi_epi16(b0, 0xA0);
        __m256i b1 = _mm256_shufflelo_epi16(bv, 0xF5);
        b1 = _mm256_shufflehi_epi16(b1, 0xF5);

        /* Build zeta vector [z0,z0,-z0,-z0, z1,z1,-z1,-z1, z2,...,z3,z3,-z3,-z3]:
         * load 4 zetas, duplicate each x4 (pshufb), then negate odd pairs (psignw). */
        __m128i z4 = _mm_loadl_epi64((const __m128i *)&zp[i]);
        __m256i zexp = _mm256_inserti128_si256(
            _mm256_castsi128_si256(_mm_shuffle_epi8(z4, zsh_lo)),
            _mm_shuffle_epi8(z4, zsh_hi), 1);
        __m256i zv = _mm256_sign_epi16(zexp, zsign);

        /* basemul: r0 = a0*b0 + a1*b1*zeta, r1 = a0*b1 + a1*b0 */
        __m256i ab11 = poly_avx2_fqmul(a1, b1);
        __m256i ab11z = poly_avx2_fqmul(ab11, zv);
        __m256i ab00 = poly_avx2_fqmul(a0, b0);
        __m256i r0 = _mm256_add_epi16(ab11z, ab00);

        __m256i ab01 = poly_avx2_fqmul(a0, b1);
        __m256i ab10 = poly_avx2_fqmul(a1, b0);
        __m256i r1 = _mm256_add_epi16(ab01, ab10);

        /* Merge: even positions from r0, odd positions from r1 */
        __m256i result = _mm256_blend_epi16(r0, r1, 0xAA);

        _mm256_store_si256((__m256i *)&res->coeffs[4 * i], result);
    }
}
#else
/* DKE-512: assembly basemul */
#if defined(DKE_NTT512_PACKED)
/* Packed-domain ML-KEM-style schoolbook basemul (X^2-zeta) for n=512, q=7681. */
extern void dke_p512_basemul_avx(int16_t *r, const int16_t *a, const int16_t *b,
                                 const int16_t *qdata);
extern const int16_t dke_p512_qdata[];

void DKE_poly_basemul_montgomery(poly *res, const poly *a, const poly *b) {
    dke_p512_basemul_avx(res->coeffs, a->coeffs, b->coeffs, dke_p512_qdata);
}
#elif defined(DKE_NTT512_ASM)
/* Hand-written GAS/MASM pointwise basemul for n=512, q=7681 (verified KAT==ref). */
extern void dke_basemul512_gas(int16_t *r, const int16_t *a, const int16_t *b,
                               const int16_t *qdata512, const int16_t *zp);
extern const int16_t dke_qdata512[];

void DKE_poly_basemul_montgomery(poly *res, const poly *a, const poly *b) {
    dke_basemul512_gas(res->coeffs, a->coeffs, b->coeffs,
                       dke_qdata512, DKE_zetas + DKE_NTT_ZETAS_LEN / 2);
}
#elif defined(_MSC_VER) && !defined(DKE_AVX2_NTT_INTRINSIC)
extern void dke_basemul512_avx2(int16_t *r, const int16_t *a, const int16_t *b,
                                 const int16_t *qdata512, const int16_t *zetas);
extern const int16_t dke_qdata512[32];

void DKE_poly_basemul_montgomery(poly *res, const poly *a, const poly *b) {
    dke_basemul512_avx2(res->coeffs, a->coeffs, b->coeffs,
                        dke_qdata512, DKE_zetas + DKE_NTT_ZETAS_LEN / 2);
}
#elif defined(DKM_LINUX) && !defined(DKE_AVX2_NTT_INTRINSIC)
extern void dke_basemul512_avx2_linux(int16_t *r, const int16_t *a, const int16_t *b,
                                       const int16_t *qdata512, const int16_t *zetas);
extern const int16_t dke_qdata512[32];

void DKE_poly_basemul_montgomery(poly *res, const poly *a, const poly *b) {
    dke_basemul512_avx2_linux(res->coeffs, a->coeffs, b->coeffs,
                              dke_qdata512, DKE_zetas + DKE_NTT_ZETAS_LEN / 2);
}
#else
/* Fallback intrinsics */
void DKE_poly_basemul_montgomery(poly *res, const poly *a, const poly *b) {
    unsigned int i;
    const int16_t *zp = DKE_zetas + DKE_NTT_ZETAS_LEN / 2;
    const __m128i zsh_lo = _mm_setr_epi8(0,1,0,1,0,1,0,1, 2,3,2,3,2,3,2,3);
    const __m128i zsh_hi = _mm_setr_epi8(4,5,4,5,4,5,4,5, 6,7,6,7,6,7,6,7);
    const __m256i zsign  = _mm256_set_epi16(-1,-1,1,1, -1,-1,1,1,
                                            -1,-1,1,1, -1,-1,1,1);
    for (i = 0; i < DKE_N / 4; i += 4) {
        __m256i av = _mm256_load_si256((__m256i *)&a->coeffs[4 * i]);
        __m256i bv = _mm256_load_si256((__m256i *)&b->coeffs[4 * i]);
        __m256i a0 = _mm256_shufflelo_epi16(av, 0xA0);
        a0 = _mm256_shufflehi_epi16(a0, 0xA0);
        __m256i a1 = _mm256_shufflelo_epi16(av, 0xF5);
        a1 = _mm256_shufflehi_epi16(a1, 0xF5);
        __m256i b0 = _mm256_shufflelo_epi16(bv, 0xA0);
        b0 = _mm256_shufflehi_epi16(b0, 0xA0);
        __m256i b1 = _mm256_shufflelo_epi16(bv, 0xF5);
        b1 = _mm256_shufflehi_epi16(b1, 0xF5);
        __m128i z4 = _mm_loadl_epi64((const __m128i *)&zp[i]);
        __m256i zexp = _mm256_inserti128_si256(
            _mm256_castsi128_si256(_mm_shuffle_epi8(z4, zsh_lo)),
            _mm_shuffle_epi8(z4, zsh_hi), 1);
        __m256i zv = _mm256_sign_epi16(zexp, zsign);
        __m256i ab11 = poly_avx2_fqmul(a1, b1);
        __m256i ab11z = poly_avx2_fqmul(ab11, zv);
        __m256i ab00 = poly_avx2_fqmul(a0, b0);
        __m256i r0 = _mm256_add_epi16(ab11z, ab00);
        __m256i ab01 = poly_avx2_fqmul(a0, b1);
        __m256i ab10 = poly_avx2_fqmul(a1, b0);
        __m256i r1 = _mm256_add_epi16(ab01, ab10);
        __m256i result = _mm256_blend_epi16(r0, r1, 0xAA);
        _mm256_store_si256((__m256i *)&res->coeffs[4 * i], result);
    }
}
#endif
#endif

#endif /* DKE_USE_AVX2 */
