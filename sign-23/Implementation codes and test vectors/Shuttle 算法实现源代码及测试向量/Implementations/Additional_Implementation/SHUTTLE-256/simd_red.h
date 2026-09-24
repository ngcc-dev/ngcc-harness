/*
 * avx512/simd_red.h -- AVX-512 (zmm, 16x int32) vector mirrors of the
 * scalar scheme-domain reductions in reduce.c, plus the small mod-q /
 * mod-H_h folds used by the rounding/hint/norm hot paths.
 *
 * Bit-exact lane-wise mirror of the scalar reduce.c / rounding.c / poly.c
 * helpers (same Barrett reciprocal BARRETT_SH=46, REC=floor(2^46/d), same
 * masked corrections), so the avx512 fork stays byte-exact to ref.  Uses
 * native AVX-512 mask compares (vpcmpud / vpcmpd) for the branchless
 * corrections.  CT: no idiv, no `% const`, no float.  AVX-512F/BW/DQ/VL.
 *
 * Only the avx512 poly.c / rounding.c forks include this (guarded by
 * SHUTTLE_ROUNDING_SIMD / USE_AVX512_NTT); the ref build never sees it.
 */
#ifndef SHUTTLE_SIMD_RED_H
#define SHUTTLE_SIMD_RED_H

#include <immintrin.h>
#include <stdint.h>

#include "params.h"          /* Q, DQ, ALPHA_H, HH */
#include "rounding_consts.h" /* LOG2_ALPHA_H (the alpha_h shift) */

/* ---- width-abstracting aliases so the rounding.c hint/lift loops are
 * written ONCE and compile to zmm here / ymm in the avx2 fork.  SIMD_W =
 * int32 lanes per vector. ---- */
typedef __m512i simdv;
#define SIMD_W 16
#define simdv_load(p) _mm512_loadu_si512((const void *)(p))
#define simdv_store(p, v) _mm512_storeu_si512((void *)(p), (v))
#define simdv_set1(x) _mm512_set1_epi32(x)
#define simdv_setzero() _mm512_setzero_si512()
#define simdv_add(a, b) _mm512_add_epi32((a), (b))
#define simdv_sub(a, b) _mm512_sub_epi32((a), (b))
#define simdv_and(a, b) _mm512_and_si512((a), (b))
#define simdv_slli(a, n) _mm512_slli_epi32((a), (n))
#define simdv_srai(a, n) _mm512_srai_epi32((a), (n))
#define simdv_srli(a, n) _mm512_srli_epi32((a), (n))
#define simdv_mullo(a, b) _mm512_mullo_epi32((a), (b))

#define SIMD_BARRETT_SH 46
#define SIMD_BARRETT_Q (((uint64_t)1 << SIMD_BARRETT_SH) / (uint64_t)Q)
#define SIMD_BARRETT_2Q (((uint64_t)1 << SIMD_BARRETT_SH) / (uint64_t)DQ)

/* 16x32 unsigned Barrett: au mod d in [0,d), au in [0,2^31], REC =
 * floor(2^46/d).  Mirrors barrett_mod_u.  au*REC is 32x32->64; even and
 * odd 32-bit lanes are multiplied with _mm512_mul_epu32 then the >>46
 * quotients recombined into one zmm of 16 32-bit quotients. */
static inline __m512i simd_barrett_mod_u(__m512i au, uint32_t d,
                                         uint64_t REC)
{
    const __m512i vrec = _mm512_set1_epi64((long long)REC);
    __m512i pe = _mm512_mul_epu32(au, vrec); /* even lanes */
    __m512i ao = _mm512_srli_epi64(au, 32);
    __m512i po = _mm512_mul_epu32(ao, vrec); /* odd lanes  */
    pe = _mm512_srli_epi64(pe, SIMD_BARRETT_SH);
    po = _mm512_srli_epi64(po, SIMD_BARRETT_SH);
    /* even quotients in lanes {0,2,...}; merge odd quotients (shifted up
     * 32) via a 0xAAAA dword mask blend. */
    __m512i qh =
        _mm512_mask_blend_epi32(0xAAAA, pe, _mm512_slli_epi64(po, 32));
    const __m512i vd = _mm512_set1_epi32((int)d);
    __m512i prod = _mm512_mullo_epi32(qh, vd); /* qh*d low 32 (exact)  */
    __m512i r = _mm512_sub_epi32(au, prod);    /* in [0, 2d)           */
    __mmask16 ge = _mm512_cmpge_epu32_mask(r, vd);
    return _mm512_mask_sub_epi32(r, ge, r, vd); /* r>=d ? r-d : r      */
}

/* reduce32: result in (-Q, Q) (truncate toward zero, == a%Q). */
static inline __m512i simd_reduce32(__m512i a)
{
    __m512i sa = _mm512_srai_epi32(a, 31);
    __m512i au = _mm512_sub_epi32(_mm512_xor_si512(a, sa), sa); /* |a| */
    __m512i r = simd_barrett_mod_u(au, (uint32_t)Q, SIMD_BARRETT_Q);
    return _mm512_sub_epi32(_mm512_xor_si512(r, sa), sa); /* sign */
}

/* caddq: a += (a<0)? Q : 0.  Output [0,Q) when input in (-Q,Q). */
static inline __m512i simd_caddq(__m512i a)
{
    __mmask16 m = _mm512_cmplt_epi32_mask(a, _mm512_setzero_si512());
    return _mm512_mask_add_epi32(a, m, a, _mm512_set1_epi32(Q));
}

static inline __m512i simd_freeze(__m512i a)
{
    return simd_caddq(simd_reduce32(a));
}

static inline __m512i simd_reduce_mod_2q(__m512i a)
{
    __m512i sa = _mm512_srai_epi32(a, 31);
    __m512i au = _mm512_sub_epi32(_mm512_xor_si512(a, sa), sa);
    __m512i ru = simd_barrett_mod_u(au, (uint32_t)DQ, SIMD_BARRETT_2Q);
    __m512i r = _mm512_sub_epi32(_mm512_xor_si512(ru, sa), sa); /* a%2q */
    __mmask16 m = _mm512_cmplt_epi32_mask(r, _mm512_setzero_si512());
    return _mm512_mask_add_epi32(r, m, r, _mm512_set1_epi32(DQ));
}

/* bmodpm_q: centered residue mod q in [-(Q-1)/2, (Q-1)/2]. */
static inline __m512i simd_bmodpm_q(__m512i a)
{
    __m512i r = simd_reduce32(a);
    const __m512i hi = _mm512_set1_epi32((int)((Q - 1) / 2));
    const __m512i nhi = _mm512_set1_epi32(-(int)((Q - 1) / 2));
    const __m512i vq = _mm512_set1_epi32(Q);
    __mmask16 mgt = _mm512_cmpgt_epi32_mask(r, hi);
    r = _mm512_mask_sub_epi32(r, mgt, r, vq);
    __mmask16 mlt = _mm512_cmplt_epi32_mask(r, nhi);
    r = _mm512_mask_add_epi32(r, mlt, r, vq);
    return r;
}

/* highbits_reduced: round-to-nearest /alpha_h then fold into [0,H_h). */
static inline __m512i simd_highbits_reduced(__m512i x)
{
    __m512i b = _mm512_add_epi32(x, _mm512_set1_epi32((int)ALPHA_H >> 1));
    b = _mm512_srli_epi32(b, LOG2_ALPHA_H);
    const __m512i vhh = _mm512_set1_epi32((int)HH);
    __mmask16 mge = _mm512_cmpge_epi32_mask(b, vhh);
    return _mm512_mask_sub_epi32(b, mge, b, vhh);
}

/* addmod_Hh: x in [0,2*H_h) -> [0,H_h). */
static inline __m512i simd_addmod_Hh(__m512i x)
{
    const __m512i vhh = _mm512_set1_epi32((int)HH);
    __mmask16 mge = _mm512_cmpge_epi32_mask(x, vhh);
    return _mm512_mask_sub_epi32(x, mge, x, vhh);
}

/* centermod_Hh: x in (-H_h, H_h) -> [0,H_h). */
static inline __m512i simd_centermod_Hh(__m512i x)
{
    const __m512i vhh = _mm512_set1_epi32((int)HH);
    __mmask16 mlt = _mm512_cmplt_epi32_mask(x, _mm512_setzero_si512());
    return _mm512_mask_add_epi32(x, mlt, x, vhh);
}

/* simd_freeze_pack16: freeze 16 int32 coeffs (one zmm) to [0,q) and pack
 * them as 16 uint16 at dst.  freeze is bit-exact to scalar; values in
 * [0,q) < 2^16 so the int32->int16 truncate is lossless. */
static inline void simd_freeze_pack16(uint16_t *dst, __m512i a)
{
    __m256i pk = _mm512_cvtepi32_epi16(simd_freeze(a));
    _mm256_storeu_si256((__m256i *)dst, pk);
}

/* simd_freeze_pack: freeze 16 int32 coeffs at src to [0,q) and store as 16
 * uint16 at dst.  Width-abstracted entry so poly_to_ntt_dom is one source
 * across both forks (avx2: two ymm; avx512: one zmm). */
static inline void simd_freeze_pack(uint16_t *dst, const int32_t *src)
{
    simd_freeze_pack16(dst, _mm512_loadu_si512((const void *)src));
}

/* simd_poly16_negate: in-place a[k] = subm16(0, a[k]) over n uint16 coeffs
 * in canonical [0,q).  subm16(0,z) = (z==0)? 0 : q-z (32x uint16 / zmm).
 * Bit-exact to the scalar subm16(0,.). */
static inline void simd_poly16_negate(uint16_t *a, unsigned n)
{
    const __m512i vq = _mm512_set1_epi16((short)Q);
    const __m512i vz = _mm512_setzero_si512();
    unsigned k;
    for (k = 0; k < n; k += 32) {
        __m512i z = _mm512_loadu_si512((const void *)&a[k]);
        __m512i qmz = _mm512_sub_epi16(vq, z);
        __mmask32 iszero = _mm512_cmpeq_epi16_mask(z, vz);
        __m512i r = _mm512_maskz_mov_epi16(_knot_mask32(iszero), qmz);
        _mm512_storeu_si512((void *)&a[k], r);
    }
}

/* simd_widen_u16_to_i32: t[k] = (int32_t)a[k] over n uint16 -> int32. */
static inline void simd_widen_u16_to_i32(int32_t *t, const uint16_t *a,
                                         unsigned n)
{
    unsigned k;
    for (k = 0; k < n; k += 16) {
        __m256i lo = _mm256_loadu_si256((const __m256i *)&a[k]);
        __m512i w = _mm512_cvtepu16_epi32(lo);
        _mm512_storeu_si512((void *)&t[k], w);
    }
}

#endif /* SHUTTLE_SIMD_RED_H */
