/*
 * avx2/simd_red.h -- AVX2 (ymm, 8x int32) vector mirrors of the scalar
 * scheme-domain reductions in reduce.c, plus the small mod-q / mod-H_h
 * folds used by the rounding/hint/norm hot paths.
 *
 * Every routine here is a LANE-WISE BIT-EXACT mirror of its scalar
 * counterpart (reduce.c reduce32 / caddq / freeze / reduce_mod_2q; the
 * rounding.c bmodpm_q / highbits_reduced / addmod_Hh / centermod_Hh;
 * poly.c poly_sqnorm centering).  The arithmetic is the SAME Barrett
 * reciprocal (BARRETT_SH=46, REC=floor(2^46/d)) and the SAME masked
 * corrections, so the AVX2 result equals the scalar result for every
 * int32 input -- this is what keeps the avx2 fork byte-exact to ref.
 *
 * CT discipline: branchless throughout (compare -> all-ones mask -> AND
 * with the modulus -> add/sub).  No idiv, no `% const`, no float.  Pure
 * AVX2/BMI2 (ymm only); -mno-avx512f keeps it zmm-free.
 *
 * Only the avx2 poly.c / rounding.c forks include this (guarded by
 * SHUTTLE_ROUNDING_SIMD / USE_AVX2_NTT); the ref build never sees it.
 */
#ifndef SHUTTLE_SIMD_RED_H
#define SHUTTLE_SIMD_RED_H

#include <immintrin.h>
#include <stdint.h>

#include "params.h"          /* Q, DQ, ALPHA_H, HH */
#include "rounding_consts.h" /* LOG2_ALPHA_H (the alpha_h shift) */

/* ---- width-abstracting aliases so the rounding.c hint/lift loops are
 * written ONCE and compile to ymm here / zmm in the avx512 fork.  SIMD_W =
 * int32 lanes per vector. ---- */
typedef __m256i simdv;
#define SIMD_W 8
#define simdv_load(p) _mm256_loadu_si256((const __m256i *)(p))
#define simdv_store(p, v) _mm256_storeu_si256((__m256i *)(p), (v))
#define simdv_set1(x) _mm256_set1_epi32(x)
#define simdv_setzero() _mm256_setzero_si256()
#define simdv_add(a, b) _mm256_add_epi32((a), (b))
#define simdv_sub(a, b) _mm256_sub_epi32((a), (b))
#define simdv_and(a, b) _mm256_and_si256((a), (b))
#define simdv_slli(a, n) _mm256_slli_epi32((a), (n))
#define simdv_srai(a, n) _mm256_srai_epi32((a), (n))
#define simdv_srli(a, n) _mm256_srli_epi32((a), (n))
#define simdv_mullo(a, b) _mm256_mullo_epi32((a), (b))

/* The Barrett reciprocals must agree byte-for-byte with reduce.c. */
#define SIMD_BARRETT_SH 46
#define SIMD_BARRETT_Q (((uint64_t)1 << SIMD_BARRETT_SH) / (uint64_t)Q)
#define SIMD_BARRETT_2Q (((uint64_t)1 << SIMD_BARRETT_SH) / (uint64_t)DQ)

/* ---- 8x32 unsigned Barrett: au mod d in [0,d), au in [0,2^31], REC =
 * floor(2^46/d).  Mirrors barrett_mod_u.  au*REC is a 32x32->64 product;
 * _mm256_mul_epu32 takes the even 32-bit lanes (0,2,4,6) of each operand,
 * so we run two passes (even lanes, then the >>32-shifted odd lanes) and
 * recombine the two >>46 quotients into one ymm of 8 32-bit quotients. */
static inline __m256i simd_barrett_mod_u(__m256i au, uint32_t d,
                                         uint64_t REC)
{
    const __m256i vrec = _mm256_set1_epi64x((long long)REC);
    /* even 32-bit lanes -> four 64-bit products */
    __m256i pe = _mm256_mul_epu32(au, vrec);
    /* odd lanes shifted into even slots -> four 64-bit products */
    __m256i ao = _mm256_srli_epi64(au, 32);
    __m256i po = _mm256_mul_epu32(ao, vrec);
    /* quotient = product >> 46 (each fits 32 bits) */
    pe = _mm256_srli_epi64(pe, SIMD_BARRETT_SH);
    po = _mm256_srli_epi64(po, SIMD_BARRETT_SH);
    /* recombine: even quotients sit in lanes {0,2,4,6}; shift odd
     * quotients up 32 and blend so qh ends as 8x32. */
    __m256i qh = _mm256_blend_epi32(pe, _mm256_slli_epi64(po, 32), 0xAA);
    const __m256i vd = _mm256_set1_epi32((int)d);
    __m256i prod = _mm256_mullo_epi32(qh, vd); /* qh*d, low 32 (exact)  */
    __m256i r = _mm256_sub_epi32(au, prod);    /* in [0, 2d)            */
    /* r >= d ? r - d : r  (unsigned compare via add-bias signed cmp) */
    const __m256i bias = _mm256_set1_epi32((int)0x80000000u);
    __m256i rge = _mm256_cmpgt_epi32(_mm256_xor_si256(vd, bias),
                                     _mm256_xor_si256(r, bias)); /* d>r */
    __m256i ge = _mm256_andnot_si256(rge, vd); /* !(d>r) ? d : 0 -> r>=d */
    return _mm256_sub_epi32(r, ge);
}

/* reduce32: 8x32, result in (-Q, Q) (truncate toward zero, == a%Q). */
static inline __m256i simd_reduce32(__m256i a)
{
    __m256i sa = _mm256_srai_epi32(a, 31); /* 0 or -1          */
    __m256i au = _mm256_sub_epi32(_mm256_xor_si256(a, sa), sa); /* |a|   */
    __m256i r = simd_barrett_mod_u(au, (uint32_t)Q, SIMD_BARRETT_Q);
    return _mm256_sub_epi32(_mm256_xor_si256(r, sa),
                            sa); /* reapply sign */
}

/* caddq: a += (a<0)? Q : 0.  Output [0,Q) when input in (-Q,Q). */
static inline __m256i simd_caddq(__m256i a)
{
    __m256i m = _mm256_srai_epi32(a, 31); /* -1 if a<0 */
    return _mm256_add_epi32(a, _mm256_and_si256(m, _mm256_set1_epi32(Q)));
}

/* freeze: standard representative in [0,Q). */
static inline __m256i simd_freeze(__m256i a)
{
    return simd_caddq(simd_reduce32(a));
}

/* reduce_mod_2q: canonical residue in [0,2Q).  Mirrors reduce_mod_2q. */
static inline __m256i simd_reduce_mod_2q(__m256i a)
{
    __m256i sa = _mm256_srai_epi32(a, 31);
    __m256i au = _mm256_sub_epi32(_mm256_xor_si256(a, sa), sa);
    __m256i ru = simd_barrett_mod_u(au, (uint32_t)DQ, SIMD_BARRETT_2Q);
    __m256i r = _mm256_sub_epi32(_mm256_xor_si256(ru, sa), sa); /* a%2q */
    __m256i m = _mm256_srai_epi32(r, 31);
    return _mm256_add_epi32(r, _mm256_and_si256(m, _mm256_set1_epi32(DQ)));
}

/* bmodpm_q: centered residue mod q in [-(Q-1)/2, (Q-1)/2].  Mirrors the
 * rounding.c bmodpm_q (and the poly_sqnorm centering). */
static inline __m256i simd_bmodpm_q(__m256i a)
{
    __m256i r = simd_reduce32(a); /* (-Q, Q)               */
    const __m256i hi = _mm256_set1_epi32((int)((Q - 1) / 2));
    const __m256i nhi = _mm256_set1_epi32(-(int)((Q - 1) / 2));
    const __m256i vq = _mm256_set1_epi32(Q);
    __m256i mgt = _mm256_cmpgt_epi32(r, hi); /* r > hi  -> -1        */
    r = _mm256_sub_epi32(r, _mm256_and_si256(mgt, vq));
    __m256i mlt = _mm256_cmpgt_epi32(nhi, r); /* r < -hi -> -1        */
    r = _mm256_add_epi32(r, _mm256_and_si256(mlt, vq));
    return r;
}

/* highbits_reduced: round-to-nearest /alpha_h then fold into [0,H_h).
 * x in [0,2q), ALPHA_H a power of two.  Mirrors rounding.c. */
static inline __m256i simd_highbits_reduced(__m256i x)
{
    __m256i b = _mm256_add_epi32(x, _mm256_set1_epi32((int)ALPHA_H >> 1));
    b = _mm256_srli_epi32(b, LOG2_ALPHA_H);
    const __m256i vhh = _mm256_set1_epi32((int)HH);
    /* b >= HH ? b - HH : b  (b in [0,HH], so unsigned == signed cmp) */
    __m256i mge =
        _mm256_cmpgt_epi32(b, _mm256_sub_epi32(vhh, _mm256_set1_epi32(1)));
    return _mm256_sub_epi32(b, _mm256_and_si256(mge, vhh));
}

/* addmod_Hh: x in [0,2*H_h) -> [0,H_h) by one masked subtract. */
static inline __m256i simd_addmod_Hh(__m256i x)
{
    const __m256i vhh = _mm256_set1_epi32((int)HH);
    __m256i mge =
        _mm256_cmpgt_epi32(x, _mm256_sub_epi32(vhh, _mm256_set1_epi32(1)));
    return _mm256_sub_epi32(x, _mm256_and_si256(mge, vhh));
}

/* centermod_Hh: x in (-H_h, H_h) -> [0,H_h) by one masked add (x<0 ->
 * +H_h). */
static inline __m256i simd_centermod_Hh(__m256i x)
{
    __m256i m = _mm256_srai_epi32(x, 31); /* -1 if x<0 */
    return _mm256_add_epi32(
        x, _mm256_and_si256(m, _mm256_set1_epi32((int)HH)));
}

/* simd_freeze_pack16: freeze 16 int32 coeffs (two ymm) to [0,q) and pack
 * them as 16 uint16 at dst.  Each freeze is bit-exact to scalar freeze,
 * and the values are in [0,q) < 2^16 so packus is lossless.  The packus
 * lane interleave is undone by a 0xD8 quadword permute. */
static inline void simd_freeze_pack16(uint16_t *dst, __m256i a, __m256i b)
{
    __m256i fa = simd_freeze(a);
    __m256i fb = simd_freeze(b);
    __m256i pk = _mm256_packus_epi32(fa, fb); /* lane-interleaved u16   */
    pk = _mm256_permute4x64_epi64(pk, 0xD8);  /* fix to linear order    */
    _mm256_storeu_si256((__m256i *)dst, pk);
}

/* simd_poly16_negate: in-place a[k] = subm16(0, a[k]) over N uint16 coeffs
 * in canonical [0,q).  subm16(0,z) = (z==0)? 0 : q-z; vectorized as q-z
 * with the z==0 lanes masked back to 0 (16x uint16 / ymm).  Bit-exact to
 * the scalar subm16(0,.). */
static inline void simd_poly16_negate(uint16_t *a, unsigned n)
{
    const __m256i vq = _mm256_set1_epi16((short)Q);
    const __m256i vz = _mm256_setzero_si256();
    unsigned k;
    for (k = 0; k < n; k += 16) {
        __m256i z = _mm256_loadu_si256((const __m256i *)&a[k]);
        __m256i qmz = _mm256_sub_epi16(vq, z);        /* q - z          */
        __m256i iszero = _mm256_cmpeq_epi16(z, vz);   /* -1 where z==0  */
        __m256i r = _mm256_andnot_si256(iszero, qmz); /* 0 if z==0     */
        _mm256_storeu_si256((__m256i *)&a[k], r);
    }
}

/* simd_widen_u16_to_i32: t[k] = (int32_t)a[k] over n uint16 -> int32. */
static inline void simd_widen_u16_to_i32(int32_t *t, const uint16_t *a,
                                         unsigned n)
{
    unsigned k;
    for (k = 0; k < n; k += 8) {
        __m128i lo = _mm_loadu_si128((const __m128i *)&a[k]);
        __m256i w = _mm256_cvtepu16_epi32(lo);
        _mm256_storeu_si256((__m256i *)&t[k], w);
    }
}

/* simd_freeze_pack: freeze 16 int32 coeffs at src to [0,q) and store as 16
 * uint16 at dst.  Width-abstracted entry so poly_to_ntt_dom is one source
 * across both forks (avx2: two ymm; avx512: one zmm). */
static inline void simd_freeze_pack(uint16_t *dst, const int32_t *src)
{
    simd_freeze_pack16(dst, _mm256_loadu_si256((const __m256i *)src),
                       _mm256_loadu_si256((const __m256i *)(src + 8)));
}

/* horizontal sum of an int64x4 ymm into a scalar int64. */
static inline int64_t simd_hsum_epi64(__m256i v)
{
    __m128i lo = _mm256_castsi256_si128(v);
    __m128i hi = _mm256_extracti128_si256(v, 1);
    __m128i s = _mm_add_epi64(lo, hi);
    int64_t a = (int64_t)_mm_extract_epi64(s, 0);
    int64_t b = (int64_t)_mm_extract_epi64(s, 1);
    return a + b;
}

#endif /* SHUTTLE_SIMD_RED_H */
