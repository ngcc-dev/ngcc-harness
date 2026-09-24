#include <stdint.h>
#include <immintrin.h>
#include "params.h"
#include "consts.h"
#include "reduce.h"
#include "ntt.h"
#include "basemul_avx.h"

/* ── Barrett reduction: a mod± Q, shift=24, V=21817 ── */
static inline __m256i barrett_reduce(__m256i a, __m256i q, __m256i v) {
    __m256i t = _mm256_mulhi_epi16(v, a);
    t = _mm256_srai_epi16(t, 8);
    t = _mm256_mullo_epi16(q, t);
    return _mm256_sub_epi16(a, t);
}

/* ── Montgomery multiply: a*b * R^{-1} mod Q (element-wise, 16-wide) ── */
static inline __m256i fqmul_avx(__m256i a, __m256i b, __m256i q, __m256i qinv) {
    __m256i lo = _mm256_mullo_epi16(a, b);
    __m256i hi = _mm256_mulhi_epi16(a, b);
    __m256i u  = _mm256_mullo_epi16(lo, qinv);
    __m256i uq = _mm256_mulhi_epi16(q, u);
    return _mm256_sub_epi16(hi, uq);
}

/* ── Montgomery multiply with precomputed zeta pair ── */
static inline __m256i fqmul_precomp(__m256i a, __m256i zeta_lo, __m256i zeta_hi, __m256i q) {
    __m256i lo = _mm256_mullo_epi16(zeta_lo, a);
    __m256i t  = _mm256_mulhi_epi16(zeta_hi, a);
    lo = _mm256_mulhi_epi16(q, lo);
    return _mm256_sub_epi16(t, lo);
}

/*
 * 4×4 int16 transpose inside a ymm
 * Input:  [a0,a1,a2,a3, b0,b1,b2,b3, c0,c1,c2,c3, d0,d1,d2,d3]
 * Output: [a0,b0,c0,d0, a1,b1,c1,d1, a2,b2,c2,d2, a3,b3,c3,d3]
 */
static inline __m256i transpose_4x4_epi16(__m256i in) {
    const __m256i mask = _mm256_setr_epi8(
        0x00,0x01,0x08,0x09, 0x02,0x03,0x0A,0x0B,
        0x04,0x05,0x0C,0x0D, 0x06,0x07,0x0E,0x0F,
        0x00,0x01,0x08,0x09, 0x02,0x03,0x0A,0x0B,
        0x04,0x05,0x0C,0x0D, 0x06,0x07,0x0E,0x0F);
    __m256i t0 = _mm256_permute4x64_epi64(in, _MM_SHUFFLE(3, 1, 2, 0));
    __m256i t1 = _mm256_shuffle_epi8(t0, mask);
    __m256i t2 = _mm256_permute4x64_epi64(t1, _MM_SHUFFLE(3, 1, 2, 0));
    return   _mm256_shuffle_epi8(t2, mask);
}

/*
 * Pack 4 column vectors (each in low 128 bits of 4 ymm regs) into
 * one ymm in column-major layout, then transpose back to row-major.
 */
static inline __m256i pack_cols_and_transpose(__m256i c0, __m256i c1,
                                               __m256i c2, __m256i c3) {
    __m128i lo = _mm_unpacklo_epi64(_mm256_castsi256_si128(c0),
                                     _mm256_castsi256_si128(c1));
    __m128i hi = _mm_unpacklo_epi64(_mm256_castsi256_si128(c2),
                                     _mm256_castsi256_si128(c3));
    __m256i cols = _mm256_set_m128i(hi, lo);
    return transpose_4x4_epi16(cols);
}

/*
 * Broadcast a 4-element int16 pattern (packed in a 64-bit word)
 * to all four 64-bit lanes of a ymm.
 *   packed = elt0 | (elt1 << 16) | (elt2 << 32) | (elt3 << 48)
 */
static inline __m256i bcst4(int64_t packed) {
    return _mm256_set1_epi64x(packed);
}

/*
 * Core K(2) Karatsuba: 4 parallel basemuls (16 elements).
 * Takes pre-transposed inputs, returns packed result in a ymm register.
 * Callers that already have inputs in registers can avoid the store-load
 * roundtrip by using this directly instead of k2_4x().
 */
static inline __m256i k2_4x_packed(__m256i at, __m256i bt,
                                    __m256i zeta_lo, __m256i zeta_hi,
                                    __m256i q, __m256i qinv, __m256i v) {
#define COL(atv, c)  _mm256_permute4x64_epi64(atv, _MM_SHUFFLE(c,c,c,c))

    /* ── Karatsuba column sums ── */
    __m256i sa0 = _mm256_add_epi16(COL(at,0), COL(at,2));
    __m256i sa1 = _mm256_add_epi16(COL(at,1), COL(at,3));
    __m256i sb0 = _mm256_add_epi16(COL(bt,0), COL(bt,2));
    __m256i sb1 = _mm256_add_epi16(COL(bt,1), COL(bt,3));

    /* ── 9 Karatsuba products ── */
    __m256i p_lo0 = fqmul_avx(COL(at,0), COL(bt,0), q, qinv);
    __m256i p_lo2 = fqmul_avx(COL(at,1), COL(bt,1), q, qinv);
    __m256i p_lo1 = _mm256_sub_epi16(
        fqmul_avx(_mm256_add_epi16(COL(at,0), COL(at,1)),
                   _mm256_add_epi16(COL(bt,0), COL(bt,1)), q, qinv),
        _mm256_add_epi16(p_lo0, p_lo2));

    __m256i p_hi0 = fqmul_avx(COL(at,2), COL(bt,2), q, qinv);
    __m256i p_hi2 = fqmul_avx(COL(at,3), COL(bt,3), q, qinv);
    __m256i p_hi1 = _mm256_sub_epi16(
        fqmul_avx(_mm256_add_epi16(COL(at,2), COL(at,3)),
                   _mm256_add_epi16(COL(bt,2), COL(bt,3)), q, qinv),
        _mm256_add_epi16(p_hi0, p_hi2));

    __m256i p_mid0 = fqmul_avx(sa0, sb0, q, qinv);
    __m256i p_mid2 = fqmul_avx(sa1, sb1, q, qinv);
    __m256i p_mid1 = _mm256_sub_epi16(
        fqmul_avx(_mm256_add_epi16(sa0, sa1),
                   _mm256_add_epi16(sb0, sb1), q, qinv),
        _mm256_add_epi16(p_mid0, p_mid2));

#undef COL

#define FQMULZ(v) fqmul_precomp((v), zeta_lo, zeta_hi, q)

    __m256i tmp = _mm256_sub_epi16(_mm256_add_epi16(p_hi0, p_mid2),
                                    _mm256_add_epi16(p_lo2, p_hi2));
    __m256i z_t0 = FQMULZ(tmp);

    __m256i z_hi1 = FQMULZ(p_hi1);
    __m256i z_hi2 = FQMULZ(p_hi2);

    /* ── output columns ── */
    __m256i c0 = barrett_reduce(_mm256_add_epi16(p_lo0, z_t0), q, v);
    __m256i c1 = barrett_reduce(_mm256_add_epi16(p_lo1, z_hi1), q, v);

    __m256i c2 = _mm256_add_epi16(p_lo2, z_hi2);
    c2 = _mm256_add_epi16(c2, p_mid0);
    c2 = _mm256_sub_epi16(c2, p_lo0);
    c2 = _mm256_sub_epi16(c2, p_hi0);
    c2 = barrett_reduce(c2, q, v);

    __m256i c3 = _mm256_sub_epi16(p_mid1, _mm256_add_epi16(p_lo1, p_hi1));
    c3 = barrett_reduce(c3, q, v);

#undef FQMULZ

    return pack_cols_and_transpose(c0, c1, c2, c3);
}

/*
 * Core K(2) Karatsuba: 4 parallel basemuls (16 elements) with caller-supplied
 * zeta vectors, storing result to memory.
 */
#if KEM_MODE == 128
static void k2_4x(int16_t *r, const int16_t *a, const int16_t *b,
                  __m256i zeta_lo, __m256i zeta_hi,
                  __m256i q, __m256i qinv, __m256i v) {
    __m256i av = _mm256_load_si256((const __m256i *)a);
    __m256i bv = _mm256_load_si256((const __m256i *)b);
    __m256i at = transpose_4x4_epi16(av);
    __m256i bt = transpose_4x4_epi16(bv);
    __m256i result = k2_4x_packed(at, bt, zeta_lo, zeta_hi, q, qinv, v);
    _mm256_store_si256((__m256i *)r, result);
}
#endif

/*
 * Process 4 basemuls (16 elements) in parallel.
 *
 * a[16], b[16] → r[16]
 *
 * zeta values for the 4 blocks:
 *   block 0: +zetaA,  block 1: -zetaA
 *   block 2: +zetaB,  block 3: -zetaB
 */
#if KEM_MODE == 128
static void basemul_avx_4x(int16_t *r, const int16_t *a, const int16_t *b,
                           int16_t zetaA, int16_t zetaB,
                           __m256i q, __m256i qinv, __m256i v) {
    int64_t zl = (int64_t)(uint16_t)( zetaA * QINV)
               | ((int64_t)(uint16_t)((-zetaA) * QINV) << 16)
               | ((int64_t)(uint16_t)( zetaB * QINV) << 32)
               | ((int64_t)(uint16_t)((-zetaB) * QINV) << 48);
    int64_t zh = (int64_t)(uint16_t) zetaA
               | ((int64_t)(uint16_t)(-zetaA) << 16)
               | ((int64_t)(uint16_t) zetaB  << 32)
               | ((int64_t)(uint16_t)(-zetaB) << 48);
    k2_4x(r, a, b, bcst4(zl), bcst4(zh), q, qinv, v);
}
#endif

/*
 * Process 2 K(4) basemuls (16 elements = 2 × 8-element blocks) in parallel.
 *
 * Each block is an 8-element Karatsuba multiplication (half=4).
 * basemul_K(4) decomposes into:
 *   1. Split into evens/odds (4 elements each)
 *   2. 3× K(2) sub-operations: lo, hi, mid
 *   3. K(4) combine: cross=mid-lo-hi, yhi rotation, lo+yhi, cross output
 *
 * Block 0 uses +zeta, block 1 uses -zeta (as in poly_basemul_montgomery).
 *
 * lo+hi and mid are each computed via one k2_4x AVX2 call, eliminating the
 * scalar mid K(2) bottleneck.
 */
#if KEM_MODE == 256
static void basemul_avx_8x(int16_t *r, const int16_t *a, const int16_t *b,
                           int16_t zeta, __m256i q, __m256i qinv, __m256i v) {
    /* ── pshufb mask: within each 128-bit lane, reorder [a0..a7] → [e0,e1,e2,e3, o0,o1,o2,o3] ── */
    __m256i eo_mask = _mm256_setr_epi8(
        0x00,0x01, 0x04,0x05, 0x08,0x09, 0x0C,0x0D,  /* evens 0-3 */
        0x02,0x03, 0x06,0x07, 0x0A,0x0B, 0x0E,0x0F,  /* odds 0-3 */
        0x00,0x01, 0x04,0x05, 0x08,0x09, 0x0C,0x0D,  /* same for high lane */
        0x02,0x03, 0x06,0x07, 0x0A,0x0B, 0x0E,0x0F);

    __m256i av = _mm256_load_si256((const __m256i *)a);
    __m256i bv = _mm256_load_si256((const __m256i *)b);

    /* Separate evens and odds within each 128-bit lane.
     * Result: lane0 = [e0_0..e0_3, o0_0..o0_3], lane1 = [e1_0..e1_3, o1_0..o1_3] */
    __m256i a_eo = _mm256_shuffle_epi8(av, eo_mask);
    __m256i b_eo = _mm256_shuffle_epi8(bv, eo_mask);

    /* ── k2_4x for lo+hi ──
     * Zeta: [+ζ, +ζ, -ζ, -ζ] for columns [lo0, hi0, lo1, hi1].
     * a_eo=[e0,o0,e1,o1] already in the right column order — no permute needed.
     * Output: [lo0(4), hi0(4), lo1(4), hi1(4)] (row-major, blocks grouped). */
    int64_t zl_lh = (int64_t)(uint16_t)( zeta * QINV)
                  | ((int64_t)(uint16_t)( zeta * QINV) << 16)
                  | ((int64_t)(uint16_t)((-zeta) * QINV) << 32)
                  | ((int64_t)(uint16_t)((-zeta) * QINV) << 48);
    int64_t zh_lh = (int64_t)(uint16_t) zeta
                  | ((int64_t)(uint16_t) zeta << 16)
                  | ((int64_t)(uint16_t)(-zeta) << 32)
                  | ((int64_t)(uint16_t)(-zeta) << 48);

    __m256i at_lo = transpose_4x4_epi16(a_eo);
    __m256i bt_lo = transpose_4x4_epi16(b_eo);
    __m256i lo_hi_result = k2_4x_packed(at_lo, bt_lo, bcst4(zl_lh), bcst4(zh_lh), q, qinv, v);

    /* ── mid via k2_4x: K(2)(e+o, e+o, block_zeta) for both blocks ── */
    const __m256i hq = _mm256_set1_epi16(HALF_Q);
    const __m256i zr = _mm256_setzero_si256();

    /* sa = freeze_centered(e + o): shift odds down, add, Barrett-freeze, center */
    __m256i a_sh = _mm256_bsrli_epi128(a_eo, 8);           /* [o0(4),0, o1(4),0] */
    __m256i a_sum = _mm256_add_epi16(a_eo, a_sh);          /* [e0+o0, o0, e1+o1, o1] */
    a_sum = barrett_reduce(a_sum, q, v);
    __m256i tmp = _mm256_sub_epi16(a_sum, hq);
    a_sum = _mm256_sub_epi16(a_sum, _mm256_and_si256(_mm256_cmpgt_epi16(tmp, zr), q));

    __m256i b_sh = _mm256_bsrli_epi128(b_eo, 8);
    __m256i b_sum = _mm256_add_epi16(b_eo, b_sh);
    b_sum = barrett_reduce(b_sum, q, v);
    tmp = _mm256_sub_epi16(b_sum, hq);
    b_sum = _mm256_sub_epi16(b_sum, _mm256_and_si256(_mm256_cmpgt_epi16(tmp, zr), q));

    /* Extract sa0,sa1 (low 64 bits of each 128-bit lane) into columns 0,1 */
    __m256i a_sw = _mm256_permute4x64_epi64(a_sum, _MM_SHUFFLE(1,0,3,2));
    __m256i a_mid = _mm256_unpacklo_epi64(a_sum, a_sw);    /* [sa0(4), sa1(4), o0(4), o1(4)] */
    __m256i b_sw = _mm256_permute4x64_epi64(b_sum, _MM_SHUFFLE(1,0,3,2));
    __m256i b_mid = _mm256_unpacklo_epi64(b_sum, b_sw);    /* [sb0(4), sb1(4), o0(4), o1(4)] */

    /* Dummy columns 2,3 → [1,0,0,0]; output rows 2,3 are ignored */
    a_mid = _mm256_insertf128_si256(a_mid, _mm_set1_epi64x(0x0000000000000001ULL), 1);
    b_mid = _mm256_insertf128_si256(b_mid, _mm_set1_epi64x(0x0000000000000001ULL), 1);

    /* Mid zeta: [+ζ, -ζ, +ζ, -ζ] for [mid0, mid1, dummy, dummy] */
    int64_t zl_md = (int64_t)(uint16_t)( zeta * QINV)
                  | ((int64_t)(uint16_t)((-zeta) * QINV) << 16)
                  | ((int64_t)(uint16_t)( zeta * QINV) << 32)
                  | ((int64_t)(uint16_t)((-zeta) * QINV) << 48);
    int64_t zh_md = (int64_t)(uint16_t) zeta
                  | ((int64_t)(uint16_t)(-zeta) << 16)
                  | ((int64_t)(uint16_t) zeta  << 32)
                  | ((int64_t)(uint16_t)(-zeta) << 48);

    __m256i a_mid_t = transpose_4x4_epi16(a_mid);
    __m256i b_mid_t = transpose_4x4_epi16(b_mid);
    __m256i mid_result = k2_4x_packed(a_mid_t, b_mid_t, bcst4(zl_md), bcst4(zh_md), q, qinv, v);
    /* Output: [mid0(4), mid1(4), garbage(8)] */

    /* ── K(4) combine, AVX2 (data in low 128 bits, duplicated to YMM) ── */
    {
        /* Extract lo+hi from k2_4x_packed result */
        __m128i lh0 = _mm256_castsi256_si128(lo_hi_result);
        __m128i lh1 = _mm256_extracti128_si256(lo_hi_result, 1);
        __m128i lo = _mm_unpacklo_epi64(lh0, lh1);  /* lo0(4), lo1(4) */
        __m128i hi = _mm_unpackhi_epi64(lh0, lh1);  /* hi0(4), hi1(4) */
        __m128i mid = _mm256_castsi256_si128(mid_result);

        /* yhi: rotate-right-1 within each 4-element block, scalar yhi[0]=fqmul(±zeta, hi_[3]) */
        const __m128i rot_mask = _mm_setr_epi8(
            6,7, 0,1, 2,3, 4,5, 14,15, 8,9, 10,11, 12,13);
        __m128i yhi = _mm_shuffle_epi8(hi, rot_mask);
        {
            int32_t mra0 = (int32_t)zeta * _mm_extract_epi16(lh0, 7);
            int16_t yh0 = (mra0 - (int32_t)(uint16_t)(mra0 * QINV) * Q) >> 16;
            int32_t mra1 = (int32_t)(-zeta) * _mm_extract_epi16(lh1, 7);
            int16_t yh1 = (mra1 - (int32_t)(uint16_t)(mra1 * QINV) * Q) >> 16;
            yhi = _mm_insert_epi16(yhi, yh0, 0);
            yhi = _mm_insert_epi16(yhi, yh1, 4);
        }

        /* Promote to YMM: duplicate XMM data to both 128-bit lanes */
        __m256i lo_v  = _mm256_set_m128i(lo, lo);
        __m256i hi_v  = _mm256_set_m128i(hi, hi);
        __m256i mid_v = _mm256_set_m128i(mid, mid);
        __m256i yhi_v = _mm256_set_m128i(yhi, yhi);

        /* cross = freeze(mid - lo - hi), ev = freeze(lo + yhi) — full AVX2 Barrett */
        __m256i cross_v = _mm256_sub_epi16(_mm256_sub_epi16(mid_v, lo_v), hi_v);
        cross_v = barrett_reduce(cross_v, q, v);
        __m256i ev_v = _mm256_add_epi16(lo_v, yhi_v);
        ev_v = barrett_reduce(ev_v, q, v);

        /* Interleave and store: low 128 bits of ev+cross go to r[0..7], high 128 to r[8..15] */
        _mm_store_si128((__m128i *)r,
                        _mm_unpacklo_epi16(_mm256_castsi256_si128(ev_v),
                                           _mm256_castsi256_si128(cross_v)));
        _mm_store_si128((__m128i *)(r + 8),
                        _mm_unpackhi_epi16(_mm256_castsi256_si128(ev_v),
                                           _mm256_castsi256_si128(cross_v)));
    }
}
#endif

#if KEM_MODE == 128
/*
 * Process 4 baseinv operations (16 elements) in parallel.
 * a[16] → b[16], returns 0 when all 4 inversions succeed.
 */
static int baseinv_avx_4x(int16_t *b_out, const int16_t *a,
                          int16_t zetaA, int16_t zetaB,
                          __m256i q, __m256i qinv) {
    __m256i av = _mm256_load_si256((const __m256i *)a);
    __m256i at = transpose_4x4_epi16(av);

    /* zeta vectors */
    int64_t zl = (int64_t)(uint16_t)( zetaA * QINV)
               | ((int64_t)(uint16_t)((-zetaA) * QINV) << 16)
               | ((int64_t)(uint16_t)( zetaB * QINV) << 32)
               | ((int64_t)(uint16_t)((-zetaB) * QINV) << 48);
    int64_t zh = (int64_t)(uint16_t) zetaA
               | ((int64_t)(uint16_t)(-zetaA) << 16)
               | ((int64_t)(uint16_t) zetaB  << 32)
               | ((int64_t)(uint16_t)(-zetaB) << 48);
    __m256i zeta_lo = bcst4(zl);
    __m256i zeta_hi = bcst4(zh);

#define COL(atv, c) _mm256_permute4x64_epi64(atv, _MM_SHUFFLE(c,c,c,c))
#define FQMUL(aa,bb)  fqmul_avx((aa),(bb),q,qinv)
#define FQMULZ(vv)    fqmul_precomp((vv), zeta_lo, zeta_hi, q)

    __m256i a0 = COL(at,0);
    __m256i a1 = COL(at,1);
    __m256i a2 = COL(at,2);
    __m256i a3 = COL(at,3);

    /* t0 = a0² + zeta*(a2² - 2*a1*a3) */
    __m256i two_a1 = _mm256_add_epi16(a1, a1);
    __m256i inner0 = _mm256_sub_epi16(FQMUL(a2, a2), FQMUL(two_a1, a3));
    __m256i t0     = _mm256_add_epi16(FQMUL(a0, a0), FQMULZ(inner0));

    /* t1 = 2*a0*a2 - a1² - zeta*a3² */
    __m256i two_a0 = _mm256_add_epi16(a0, a0);
    __m256i t1     = _mm256_sub_epi16(FQMUL(two_a0, a2), FQMUL(a1, a1));
    t1 = _mm256_sub_epi16(t1, FQMULZ(FQMUL(a3, a3)));

    /* t2 = t0² - zeta*t1² */
    __m256i t2 = _mm256_sub_epi16(FQMUL(t0, t0), FQMULZ(FQMUL(t1, t1)));

    /* ── freeze t2 via Barrett, then extract 4 idx for fqinv table lookup ── */
    int16_t inv[4];
    int ok = 0;
    {
        /* Inline Barrett: freeze t2 in YMM using local V=21817 for Q=769 */
        __m256i v_local = _mm256_set1_epi16(21817);
        __m256i t2_frozen = barrett_reduce(t2, q, v_local);
        int16_t idx0 = (int16_t)_mm256_extract_epi16(t2_frozen, 0);
        int16_t idx1 = (int16_t)_mm256_extract_epi16(t2_frozen, 1);
        int16_t idx2 = (int16_t)_mm256_extract_epi16(t2_frozen, 2);
        int16_t idx3 = (int16_t)_mm256_extract_epi16(t2_frozen, 3);
        inv[0] = fqinv_table[idx0];
        inv[1] = fqinv_table[idx1];
        inv[2] = fqinv_table[idx2];
        inv[3] = fqinv_table[idx3];
        for (int k = 0; k < 4; k++) {
            uint32_t x = (uint32_t)inv[k];
            int r1 = (-(uint64_t)x) >> 63;
            ok += (r1 - 1);
        }
    }

    /* ── scale t0, t1 by inv*MONTSQINV ──
       Reference:  t0 = fqmul(t0, t2) * MONTSQINV
       This is:    t0 * inv * R^{-1} mod Q, then × 81.
       We store the int32 product and truncate back to int16
       before using it in subsequent fqmul calls. ── */
    int64_t inv_p = (int64_t)(uint16_t)inv[0] | ((int64_t)(uint16_t)inv[1] << 16)
                  | ((int64_t)(uint16_t)inv[2] << 32) | ((int64_t)(uint16_t)inv[3] << 48);
    __m256i inv_v = bcst4(inv_p);
    __m256i t0_mont = FQMUL(t0, inv_v);
    __m256i t1_mont = FQMUL(t1, inv_v);

    /* Multiply by MONTSQINV=81: broadcast layout means mullo applies to all lanes */
    __m256i t0v = _mm256_mullo_epi16(t0_mont, _mm256_set1_epi16(MONTSQINV));
    __m256i t1v = _mm256_mullo_epi16(t1_mont, _mm256_set1_epi16(MONTSQINV));

    /* t2 = fqmul(t1, zeta) — passes the truncated t1 */
    __m256i t2v2 = FQMULZ(t1v);

    /* ── final output ── */
    __m256i c0 = _mm256_sub_epi16(FQMUL(a0, t0v), FQMUL(a2, t2v2));
    __m256i c1 = _mm256_sub_epi16(FQMUL(a3, t2v2), FQMUL(a1, t0v));      /* -fqmul(a[1],t0)+fqmul(a[3],t2) */
    __m256i c2 = _mm256_sub_epi16(FQMUL(a2, t0v), FQMUL(a0, t1v));
    __m256i c3 = _mm256_sub_epi16(FQMUL(a1, t1v), FQMUL(a3, t0v));      /* -fqmul(a[3],t0)+fqmul(a[1],t1) */

#undef COL
#undef FQMUL
#undef FQMULZ

    __m256i result = pack_cols_and_transpose(c0, c1, c2, c3);
    _mm256_store_si256((__m256i *)b_out, result);
    return ok;
}
#endif

/*
 * Process 2 K(4) baseinv operations (16 elements = 2 × 8-element blocks) in parallel.
 *
 * baseinv_K(4) decomposes into:
 *   1. Split into evens/odds (4 elements each)
 *   2. e² = K(2) square of evens, o² = K(2) square of odds
 *   3. yo2 rotate: yo2[0]=fqmul(ζ,o²[3]), yo2[1..3]=o²[0..2]
 *   4. norm = freeze(e² - yo²), then baseinv_K(2) on norm
 *   5. p = K(2)(e, ninv), q = K(2)(o, ninv)
 *   6. b[2*i]=p[i], b[2*i+1]=-q[i]
 *
 * Block 0 uses +zeta, block 1 uses -zeta.
 *
 * yo2 rotation + norm, and final p/q interleave are vectorized with SSE2/AVX2.
 */
#if KEM_MODE == 256

static int baseinv_avx_8x(int16_t *b, const int16_t *a,
                          int16_t zeta, __m256i q, __m256i qinv, __m256i v) {
    /* ── pshufb mask: within each 128-bit lane, reorder [a0..a7] → [e0..e3, o0..o3] ── */
    __m256i eo_mask = _mm256_setr_epi8(
        0x00,0x01, 0x04,0x05, 0x08,0x09, 0x0C,0x0D,
        0x02,0x03, 0x06,0x07, 0x0A,0x0B, 0x0E,0x0F,
        0x00,0x01, 0x04,0x05, 0x08,0x09, 0x0C,0x0D,
        0x02,0x03, 0x06,0x07, 0x0A,0x0B, 0x0E,0x0F);

    __m256i av = _mm256_load_si256((const __m256i *)a);

    /* a_eo = [e0(4), o0(4), e1(4), o1(4)] */
    __m256i a_eo = _mm256_shuffle_epi8(av, eo_mask);

    /* a_perm = [e0(4), e1(4), o0(4), o1(4)] */
    __m256i a_perm = _mm256_permute4x64_epi64(a_eo, _MM_SHUFFLE(3,1,2,0));

    /* Zeta: [+ζ, -ζ, +ζ, -ζ] for [e0², e1², o0², o1²] */
    int64_t zl = (int64_t)(uint16_t)( zeta * QINV)
               | ((int64_t)(uint16_t)((-zeta) * QINV) << 16)
               | ((int64_t)(uint16_t)( zeta * QINV) << 32)
               | ((int64_t)(uint16_t)((-zeta) * QINV) << 48);
    int64_t zh = (int64_t)(uint16_t) zeta
               | ((int64_t)(uint16_t)(-zeta) << 16)
               | ((int64_t)(uint16_t) zeta  << 32)
               | ((int64_t)(uint16_t)(-zeta) << 48);
    __m256i zeta_lo = bcst4(zl);
    __m256i zeta_hi = bcst4(zh);

    /* Step 1: transpose, then square via inlined k2_4x — result stays in ymm */
    __m256i at = transpose_4x4_epi16(a_perm);
    __m256i sq = k2_4x_packed(at, at, zeta_lo, zeta_hi, q, qinv, v);
    /* sq = [e0²(4), e1²(4), o0²(4), o1²(4)] row-major */

    /* Step 2: yo2 rotation + norm + freeze — result kept in ymm register */
    __m256i norm_ymm;
    {
        const __m128i v128 = _mm_set1_epi16(21817);
        const __m128i q128 = _mm_set1_epi16(Q);

        __m128i e2_v = _mm256_castsi256_si128(sq);
        __m128i o2_v = _mm256_extracti128_si256(sq, 1);

        int16_t o0_3 = _mm_extract_epi16(o2_v, 3);
        int16_t o1_3 = _mm_extract_epi16(o2_v, 7);
        int32_t ma0 = (int32_t) zeta * o0_3;
        int16_t yo2_0_0 = (ma0 - (int32_t)(uint16_t)(ma0 * QINV) * Q) >> 16;
        int32_t ma1 = (int32_t)(-zeta) * o1_3;
        int16_t yo2_1_0 = (ma1 - (int32_t)(uint16_t)(ma1 * QINV) * Q) >> 16;

        __m128i rot_mask = _mm_setr_epi8(
            6,7, 0,1, 2,3, 4,5,
            14,15, 8,9, 10,11, 12,13);
        __m128i yo2_v = _mm_shuffle_epi8(o2_v, rot_mask);
        yo2_v = _mm_insert_epi16(yo2_v, yo2_0_0, 0);
        yo2_v = _mm_insert_epi16(yo2_v, yo2_1_0, 4);

        __m128i norm_v = _mm_sub_epi16(e2_v, yo2_v);
        __m128i t = _mm_mulhi_epi16(v128, norm_v);
        t = _mm_srai_epi16(t, 8);
        norm_v = _mm_sub_epi16(norm_v, _mm_mullo_epi16(q128, t));

        /* Build ymm: rows 0,1 = norm_v (real); rows 2,3 = [1,0,0,0] (dummy) */
        __m128i dummy_v = _mm_set_epi64x(0x0000000000000001ULL,
                                          0x0000000000000001ULL);
        norm_ymm = _mm256_set_m128i(dummy_v, norm_v);
    }

    /* Step 3: invert norm — inlined baseinv_avx_4x */
#define COL(atv, c) _mm256_permute4x64_epi64(atv, _MM_SHUFFLE(c,c,c,c))
#define FQMUL(aa,bb)  fqmul_avx((aa),(bb),q,qinv)
#define FQMULZ(vv)    fqmul_precomp((vv), zeta_lo, zeta_hi, q)

    __m256i nt = transpose_4x4_epi16(norm_ymm);

    __m256i a0 = COL(nt,0);
    __m256i a1 = COL(nt,1);
    __m256i a2 = COL(nt,2);
    __m256i a3 = COL(nt,3);

    __m256i two_a1 = _mm256_add_epi16(a1, a1);
    __m256i inner0 = _mm256_sub_epi16(FQMUL(a2, a2), FQMUL(two_a1, a3));
    __m256i t0     = _mm256_add_epi16(FQMUL(a0, a0), FQMULZ(inner0));

    __m256i two_a0 = _mm256_add_epi16(a0, a0);
    __m256i t1     = _mm256_sub_epi16(FQMUL(two_a0, a2), FQMUL(a1, a1));
    t1 = _mm256_sub_epi16(t1, FQMULZ(FQMUL(a3, a3)));

    __m256i t2 = _mm256_sub_epi16(FQMUL(t0, t0), FQMULZ(FQMUL(t1, t1)));

    /* ── freeze t2 via Barrett, then extract 4 idx for fqinv table lookup ── */
    int16_t inv[4];
    int ok = 0;
    {
        __m256i v_local = _mm256_set1_epi16(21817);
        __m256i t2_frozen = barrett_reduce(t2, q, v_local);
        int16_t idx0 = (int16_t)_mm256_extract_epi16(t2_frozen, 0);
        int16_t idx1 = (int16_t)_mm256_extract_epi16(t2_frozen, 1);
        int16_t idx2 = (int16_t)_mm256_extract_epi16(t2_frozen, 2);
        int16_t idx3 = (int16_t)_mm256_extract_epi16(t2_frozen, 3);
        inv[0] = fqinv_table[idx0];
        inv[1] = fqinv_table[idx1];
        inv[2] = fqinv_table[idx2];
        inv[3] = fqinv_table[idx3];
        for (int k = 0; k < 4; k++) {
            uint32_t x = (uint32_t)inv[k];
            int r1 = (-(uint64_t)x) >> 63;
            ok += (r1 - 1);
        }
    }

    int64_t inv_p = (int64_t)(uint16_t)inv[0] | ((int64_t)(uint16_t)inv[1] << 16)
                  | ((int64_t)(uint16_t)inv[2] << 32) | ((int64_t)(uint16_t)inv[3] << 48);
    __m256i inv_v = bcst4(inv_p);
    __m256i t0_mont = FQMUL(t0, inv_v);
    __m256i t1_mont = FQMUL(t1, inv_v);

    __m256i t0v = _mm256_mullo_epi16(t0_mont, _mm256_set1_epi16(MONTSQINV));
    __m256i t1v = _mm256_mullo_epi16(t1_mont, _mm256_set1_epi16(MONTSQINV));

    __m256i t2v2 = FQMULZ(t1v);

    __m256i c0 = _mm256_sub_epi16(FQMUL(a0, t0v), FQMUL(a2, t2v2));
    __m256i c1 = _mm256_sub_epi16(FQMUL(a3, t2v2), FQMUL(a1, t0v));
    __m256i c2 = _mm256_sub_epi16(FQMUL(a2, t0v), FQMUL(a0, t1v));
    __m256i c3 = _mm256_sub_epi16(FQMUL(a1, t1v), FQMUL(a3, t0v));

#undef COL
#undef FQMUL
#undef FQMULZ

    __m256i ninv_ymm = pack_cols_and_transpose(c0, c1, c2, c3);
    /* ninv_ymm(lo 128) = [ninv0(4), ninv1(4)] */

    /* Step 4: p = K(2)(e, ninv), q = K(2)(o, ninv) via inlined k2_4x */
    __m128i ninv_v = _mm256_castsi256_si128(ninv_ymm);
    __m256i b_pq_v = _mm256_set_m128i(ninv_v, ninv_v);
    __m256i bt = transpose_4x4_epi16(b_pq_v);
    __m256i pq = k2_4x_packed(at, bt, zeta_lo, zeta_hi, q, qinv, v);
    /* pq = [p0(4), p1(4), q0(4), q1(4)] row-major */

    /* Step 5: interleave — b[2*i]=p[i], b[2*i+1]=-q[i] */
    {
        __m128i z = _mm_setzero_si128();
        __m128i pq_lo = _mm256_castsi256_si128(pq);
        __m128i pq_hi = _mm256_extracti128_si256(pq, 1);

        __m128i p0_v = _mm_unpacklo_epi64(pq_lo, z);
        __m128i p1_v = _mm_unpackhi_epi64(pq_lo, z);
        __m128i q0_v = _mm_unpacklo_epi64(pq_hi, z);
        __m128i q1_v = _mm_unpackhi_epi64(pq_hi, z);

        __m128i r0 = _mm_unpacklo_epi16(p0_v, _mm_sub_epi16(z, q0_v));
        __m128i r1 = _mm_unpacklo_epi16(p1_v, _mm_sub_epi16(z, q1_v));

        _mm256_store_si256((__m256i *)b, _mm256_set_m128i(r1, r0));
    }

    return ok;
}
#endif

/* ════════════════════════════════════════════════════════════════
 * 512: K(2)→K(4)→K(8) AVX2, all-YMM data flow (no scalar extract)
   ════════════════════════════════════════════════════════════════ */
#if KEM_MODE == 512

/* K(2): returns __m256i with r[0..3] duplicated in both 128-bit lanes.
 * Optimized: avoids memory stores for intermediates; uses only 7 extracts. */
/* K(2) basemul from XMM registers (avoids store-load roundtrip in k4_ymm) */
static inline __m256i k2_ymm_from_xmm(__m128i a128, __m128i b128,
                                       int16_t zeta, __m256i q, __m256i qinv, __m256i v)
{
    __m256i av=_mm256_insertf128_si256(_mm256_castsi128_si256(a128),a128,1);
    __m256i bv=_mm256_insertf128_si256(_mm256_castsi128_si256(b128),b128,1);

    __m256i Ev=fqmul_avx(av,bv,q,qinv);
    __m256i Hv=fqmul_avx(_mm256_hadd_epi16(av,av),_mm256_hadd_epi16(bv,bv),q,qinv);
    __m256i sv=barrett_reduce(_mm256_add_epi16(av,_mm256_shuffle_epi32(av,_MM_SHUFFLE(2,3,0,1))),q,v);
    __m256i tv=barrett_reduce(_mm256_add_epi16(bv,_mm256_shuffle_epi32(bv,_MM_SHUFFLE(2,3,0,1))),q,v);
    __m256i Mv=fqmul_avx(sv,tv,q,qinv);
    __m256i MHv=fqmul_avx(_mm256_hadd_epi16(sv,sv),_mm256_hadd_epi16(tv,tv),q,qinv);

    int16_t E01=(int16_t)_mm256_extract_epi16(Ev,0),E23=(int16_t)_mm256_extract_epi16(Ev,1);
    int16_t E45=(int16_t)_mm256_extract_epi16(Ev,2),hi2=(int16_t)_mm256_extract_epi16(Ev,3);
    int16_t H0=(int16_t)_mm256_extract_epi16(Hv,0),H1=(int16_t)_mm256_extract_epi16(Hv,1);
    int16_t M0=(int16_t)_mm256_extract_epi16(Mv,0),M1=(int16_t)_mm256_extract_epi16(Mv,1);
    int16_t MH0=(int16_t)_mm256_extract_epi16(MHv,0);
    int16_t lo0=E01,lo2=E23,hi0=E45;
    int16_t lo1=H0-lo0-lo2,hi1=H1-hi0-hi2,mid0=M0,mid2=M1,mid1=MH0-mid0-mid2;
    int16_t t1=hi0+mid2-lo2-hi2,extra=mid0-lo0-hi0;

    int64_t zl=(int64_t)(uint16_t)((int32_t)zeta*QINV)|((int64_t)(uint16_t)((int32_t)zeta*QINV)<<16)
              |((int64_t)(uint16_t)((int32_t)zeta*QINV)<<32);
    int64_t zh=(int64_t)(uint16_t)zeta|((int64_t)(uint16_t)zeta<<16)|((int64_t)(uint16_t)zeta<<32);
    int64_t zp=(int64_t)(uint16_t)t1|((int64_t)(uint16_t)hi1<<16)|((int64_t)(uint16_t)hi2<<32)|((int64_t)(uint16_t)extra<<48);
    __m128i z128=_mm_set1_epi64x(zp);
    __m256i zav=_mm256_insertf128_si256(_mm256_castsi128_si256(z128),z128,1);
    __m256i Zv=fqmul_precomp(zav,bcst4(zl),bcst4(zh),q);

    int64_t lo_p=(int64_t)(uint16_t)lo0|((int64_t)(uint16_t)lo1<<16)
                |((int64_t)(uint16_t)lo2<<32)|((int64_t)(uint16_t)(mid1-lo1-hi1)<<48);
    __m128i lo128=_mm_set1_epi64x(lo_p);
    __m256i lo_v=_mm256_insertf128_si256(_mm256_castsi128_si256(lo128),lo128,1);
    int64_t ep=(int64_t)(uint16_t)extra<<32;
    __m128i e128=_mm_set1_epi64x(ep);
    __m256i ev=_mm256_insertf128_si256(_mm256_castsi128_si256(e128),e128,1);
    __m256i rv=_mm256_add_epi16(_mm256_add_epi16(lo_v,Zv),ev);
    rv=barrett_reduce(rv,q,v);(void)v; return rv;
}


/* K(4): takes 8-element arrays, returns YMM with 8 interleaved outputs */
__m256i k4_ymm(const int16_t a[8], const int16_t b[8],
                      int16_t zeta, __m256i q, __m256i qinv, __m256i v)
{
    /* SSE2-vectorized evens/odds deinterleave + freeze_centered.
     * Keeps values in XMM registers, passes directly to k2_ymm_from_xmm. */
    __m128i av = _mm_loadu_si128((const __m128i*)a);
    __m128i bv = _mm_loadu_si128((const __m128i*)b);
    const __m128i eo_mask = _mm_setr_epi8(
        0,1, 4,5, 8,9, 12,13, 2,3, 6,7, 10,11, 14,15);
    __m128i a_eo = _mm_shuffle_epi8(av, eo_mask);
    __m128i b_eo = _mm_shuffle_epi8(bv, eo_mask);
    __m128i ea_x = a_eo;                          /* low 64 = evens */
    __m128i oa_x = _mm_srli_si128(a_eo, 8);       /* high 64 = odds */
    __m128i eb_x = b_eo;
    __m128i ob_x = _mm_srli_si128(b_eo, 8);

    /* freeze_centered(ea+oa), (eb+ob) in SSE2 */
    __m128i q128 = _mm256_castsi256_si128(q);
    __m128i v128 = _mm256_castsi256_si128(v);
    __m128i hq = _mm_set1_epi16(Q >> 1), z = _mm_setzero_si128();
    __m128i sa_x = _mm_add_epi16(ea_x, oa_x);
    __m128i sb_x = _mm_add_epi16(eb_x, ob_x);
    { __m128i t=_mm_mulhi_epi16(v128,sa_x); t=_mm_srai_epi16(t,8);
      sa_x=_mm_sub_epi16(sa_x,_mm_mullo_epi16(q128,t)); }
    { __m128i t=_mm_mulhi_epi16(v128,sb_x); t=_mm_srai_epi16(t,8);
      sb_x=_mm_sub_epi16(sb_x,_mm_mullo_epi16(q128,t)); }
    { __m128i b_tmp=_mm_sub_epi16(sa_x,hq);
      sa_x=_mm_sub_epi16(sa_x,_mm_and_si128(_mm_cmpgt_epi16(b_tmp,z),q128)); }
    { __m128i b_tmp=_mm_sub_epi16(sb_x,hq);
      sb_x=_mm_sub_epi16(sb_x,_mm_and_si128(_mm_cmpgt_epi16(b_tmp,z),q128)); }

    __m256i lo_v =k2_ymm_from_xmm(ea_x, eb_x, zeta, q, qinv, v);
    __m256i hi_v =k2_ymm_from_xmm(oa_x, ob_x, zeta, q, qinv, v);
    __m256i mid_v=k2_ymm_from_xmm(sa_x, sb_x, zeta, q, qinv, v);

    __m256i cross_v = _mm256_sub_epi16(_mm256_sub_epi16(mid_v, lo_v), hi_v);

    __m256i yhi = _mm256_alignr_epi8(hi_v, hi_v, 14);
    /* Scalar fqmul on lane 0 only (avoids 7/8 lane waste of fqmul_precomp) */
    { int16_t y0 = (int16_t)_mm256_extract_epi16(yhi, 0);
      yhi = _mm256_insert_epi16(yhi, montgomery_reduce((int32_t)zeta * y0), 0); }

    __m256i rlo = _mm256_add_epi16(lo_v, yhi);
    __m256i r01 = _mm256_unpacklo_epi16(rlo, cross_v);
    __m256i r23 = _mm256_unpackhi_epi16(rlo, cross_v);
    __m256i result = _mm256_insertf128_si256(r01, _mm256_castsi256_si128(r23), 1);
    return barrett_reduce(result, q, v);
}

/* K(8): full YMM data flow, outputs to r[16] */
static void basemul_avx_16x(int16_t *r, const int16_t *a, const int16_t *b,
                            int16_t zeta, __m256i q, __m256i qinv, __m256i v)
{
    int16_t ea[8],oa[8],eb[8],ob[8],sa[8],sb[8];
    /* AVX2 deinterleave via pshufb (same pattern as k4_ymm) */
    {
        const __m256i eo_mask = _mm256_setr_epi8(
            0,1, 4,5, 8,9, 12,13, 2,3, 6,7, 10,11, 14,15,
            0,1, 4,5, 8,9, 12,13, 2,3, 6,7, 10,11, 14,15);
        __m256i av = _mm256_loadu_si256((const __m256i*)a);
        __m256i bv = _mm256_loadu_si256((const __m256i*)b);
        __m256i a_eo = _mm256_shuffle_epi8(av, eo_mask);
        __m256i b_eo = _mm256_shuffle_epi8(bv, eo_mask);
        /* Permute: [e0,o0,e1,o1] → [e0,e1,o0,o1] */
        a_eo = _mm256_permute4x64_epi64(a_eo, _MM_SHUFFLE(3,1,2,0));
        b_eo = _mm256_permute4x64_epi64(b_eo, _MM_SHUFFLE(3,1,2,0));
        /* Store: low 128 = 8 evens, high 128 = 8 odds */
        _mm_storeu_si128((__m128i*)ea, _mm256_castsi256_si128(a_eo));
        _mm_storeu_si128((__m128i*)oa, _mm256_extracti128_si256(a_eo, 1));
        _mm_storeu_si128((__m128i*)eb, _mm256_castsi256_si128(b_eo));
        _mm_storeu_si128((__m128i*)ob, _mm256_extracti128_si256(b_eo, 1));
    }
    /* SSE2 freeze_centered for sa/sb */
    {
        __m128i q128=_mm256_castsi256_si128(q), v128=_mm256_castsi256_si128(v);
        __m128i hq=_mm_set1_epi16(Q>>1), z=_mm_setzero_si128();
        for(int h=0;h<8;h+=4){
            __m128i ea_v=_mm_loadu_si128((const __m128i*)(ea+h));
            __m128i oa_v=_mm_loadu_si128((const __m128i*)(oa+h));
            __m128i sum=_mm_add_epi16(ea_v,oa_v);
            __m128i t=_mm_mulhi_epi16(v128,sum); t=_mm_srai_epi16(t,8);
            sum=_mm_sub_epi16(sum,_mm_mullo_epi16(q128,t));
            __m128i b_tmp=_mm_sub_epi16(sum,hq);
            sum=_mm_sub_epi16(sum,_mm_and_si128(_mm_cmpgt_epi16(b_tmp,z),q128));
            _mm_storeu_si128((__m128i*)(sa+h),sum);
        }
        for(int h=0;h<8;h+=4){
            __m128i eb_v=_mm_loadu_si128((const __m128i*)(eb+h));
            __m128i ob_v=_mm_loadu_si128((const __m128i*)(ob+h));
            __m128i sum=_mm_add_epi16(eb_v,ob_v);
            __m128i t=_mm_mulhi_epi16(v128,sum); t=_mm_srai_epi16(t,8);
            sum=_mm_sub_epi16(sum,_mm_mullo_epi16(q128,t));
            __m128i b_tmp=_mm_sub_epi16(sum,hq);
            sum=_mm_sub_epi16(sum,_mm_and_si128(_mm_cmpgt_epi16(b_tmp,z),q128));
            _mm_storeu_si128((__m128i*)(sb+h),sum);
        }
    }

    __m256i lo_v=k4_ymm(ea,eb,zeta,q,qinv,v);
    __m256i hi_v=k4_ymm(oa,ob,zeta,q,qinv,v);
    __m256i mid_v=k4_ymm(sa,sb,zeta,q,qinv,v);
    __m256i cross_v=_mm256_sub_epi16(_mm256_sub_epi16(mid_v,lo_v),hi_v);

    /* yhi for K(8): right-rotate hi by 1 within each 128-bit lane.
     * yhi[0]=fqmul(zeta, hi[7]), yhi[1..7]=hi[0..6].
     * Since hi_v data is duplicated across 128-bit lanes (from k4_ymm layout),
     * _mm256_alignr_epi8 within-lane rotation gives the correct result. */
    __m256i yhi = _mm256_alignr_epi8(hi_v, hi_v, 14);
    { int16_t y0 = (int16_t)_mm256_extract_epi16(yhi, 0);
      yhi = _mm256_insert_epi16(yhi, montgomery_reduce((int32_t)zeta * y0), 0); }
    __m256i rlo = _mm256_add_epi16(lo_v, yhi);
    __m256i r01=_mm256_unpacklo_epi16(rlo,cross_v),r23=_mm256_unpackhi_epi16(rlo,cross_v);
    r01=barrett_reduce(r01,q,v); r23=barrett_reduce(r23,q,v);
    _mm_store_si128((__m128i*)r,_mm256_castsi256_si128(r01));
    _mm_store_si128((__m128i*)(r+8),_mm256_castsi256_si128(r23));
}

/* K(8) baseinv: 2 parallel K(8) inversions (32 elements, +zeta and -zeta). */
int baseinv_avx_32x(int16_t *b, const int16_t *a,
                            int16_t zeta, __m256i q, __m256i qinv, __m256i v) {
    int16_t e[2][8], o[2][8], esq[2][8], osq[2][8], norm[2][8], ninv[2][8];
    int16_t zs[2] = {zeta, -zeta};
    int ok = 0;

    /* AVX2 deinterleave via pshufb (same pattern as basemul_avx_16x) */
    const __m256i eo_mask = _mm256_setr_epi8(
        0,1, 4,5, 8,9, 12,13, 2,3, 6,7, 10,11, 14,15,
        0,1, 4,5, 8,9, 12,13, 2,3, 6,7, 10,11, 14,15);

    for (int k = 0; k < 2; k++) {
        __m256i av = _mm256_loadu_si256((const __m256i*)(a + 16*k));
        __m256i ae = _mm256_shuffle_epi8(av, eo_mask);
        ae = _mm256_permute4x64_epi64(ae, _MM_SHUFFLE(3,1,2,0));
        _mm_storeu_si128((__m128i*)e[k], _mm256_castsi256_si128(ae));
        _mm_storeu_si128((__m128i*)o[k], _mm256_extracti128_si256(ae, 1));

        __m256i ev = k4_ymm(e[k], e[k], zs[k], q, qinv, v);
        __m256i ov = k4_ymm(o[k], o[k], zs[k], q, qinv, v);
        _mm256_storeu_si256((__m256i *)esq[k], ev);
        _mm256_storeu_si256((__m256i *)osq[k], ov);

        /* SSE2 yo2 rotation + norm */
        {
            __m128i esq_v = _mm_loadu_si128((const __m128i*)esq[k]);
            __m128i osq_v = _mm_loadu_si128((const __m128i*)osq[k]);
            const __m128i rot8 = _mm_setr_epi8(
                14,15, 0,1, 2,3, 4,5, 6,7, 8,9, 10,11, 12,13);
            __m128i yo2_v = _mm_shuffle_epi8(osq_v, rot8);
            int32_t ma = (int32_t)zs[k] * _mm_extract_epi16(osq_v, 7);
            int16_t yh0 = (ma - (int32_t)(uint16_t)(ma * QINV) * Q) >> 16;
            yo2_v = _mm_insert_epi16(yo2_v, yh0, 0);
            __m128i nv = _mm_sub_epi16(esq_v, yo2_v);
            __m128i q128 = _mm256_castsi256_si128(q);
            __m128i v128 = _mm256_castsi256_si128(v);
            __m128i t = _mm_mulhi_epi16(v128, nv); t = _mm_srai_epi16(t, 8);
            nv = _mm_sub_epi16(nv, _mm_mullo_epi16(q128, t));
            _mm_storeu_si128((__m128i*)norm[k], nv);
        }

        ok += baseinv_K(4, ninv[k], norm[k], zs[k]);

        __m256i pv = k4_ymm(e[k], ninv[k], zs[k], q, qinv, v);
        __m256i qv = k4_ymm(o[k], ninv[k], zs[k], q, qinv, v);

        __m128i zz = _mm_setzero_si128();
        __m128i p = _mm256_castsi256_si128(pv);
        __m128i q = _mm256_castsi256_si128(qv);
        __m128i nq = _mm_sub_epi16(zz, q);
        _mm_storeu_si128((__m128i *)(b + 16*k),      _mm_unpacklo_epi16(p, nq));
        _mm_storeu_si128((__m128i *)(b + 16*k + 8),  _mm_unpackhi_epi16(p, nq));
    }

    return ok;
}

#endif

/* ════════════════════════════════════════════════════════════════
   Public entry points
   ════════════════════════════════════════════════════════════════ */

void poly_basemul_montgomery_avx(int16_t r[N], const int16_t a[N],
                                  const int16_t b[N], const int16_t *qdata) {
    __m256i q    = _mm256_load_si256((const __m256i *)(qdata + _16XQ));
    __m256i qinv = _mm256_load_si256((const __m256i *)(qdata + _16XQINV));
    __m256i v    = _mm256_load_si256((const __m256i *)(qdata + _16XV));

#if KEM_MODE == 128
    for (unsigned int i = 0; i < N/8; i += 2) {
        basemul_avx_4x(r + 8*i, a + 8*i, b + 8*i,
                       zetas[64 + i], zetas[64 + i + 1], q, qinv, v);
    }
#elif KEM_MODE == 256
    for (unsigned int i = 0; i < N/16; i++) {
        basemul_avx_8x(r + 16*i, a + 16*i, b + 16*i,
                       zetas[64 + i], q, qinv, v);
    }
#elif KEM_MODE == 512
    for (unsigned int i = 0; i < N/32; i++) {
        basemul_avx_16x(r + 32*i, a + 32*i, b + 32*i, zetas[64 + i], q, qinv, v);
        basemul_avx_16x(r + 32*i + 16, a + 32*i + 16, b + 32*i + 16, -zetas[64 + i], q, qinv, v);
    }
#endif
}

int poly_baseinv_avx(int16_t b[N], const int16_t a[N], const int16_t *qdata) {
    __m256i q    = _mm256_load_si256((const __m256i *)(qdata + _16XQ));
    __m256i qinv = _mm256_load_si256((const __m256i *)(qdata + _16XQINV));
    int result = 0;

#if KEM_MODE == 128
    for (unsigned int i = 0; i < N/8; i += 2) {
        result += baseinv_avx_4x(b + 8*i, a + 8*i,
                                 zetas[64 + i], zetas[64 + i + 1],
                                 q, qinv);
    }
#elif KEM_MODE == 256
    {
    __m256i v = _mm256_load_si256((const __m256i *)(qdata + _16XV));
    for (unsigned int i = 0; i < N/16; i++) {
        result += baseinv_avx_8x(b + 16*i, a + 16*i,
                                 zetas[64 + i], q, qinv, v);
    }
    }
#elif KEM_MODE == 512
    {
    __m256i v = _mm256_load_si256((const __m256i *)(qdata + _16XV));
    for (unsigned int i = 0; i < N/32; i++) {
        result += baseinv_avx_32x(b + 32*i, a + 32*i, zetas[64 + i],
                                  q, qinv, v);
    }
    }
#endif

    return result;
}
