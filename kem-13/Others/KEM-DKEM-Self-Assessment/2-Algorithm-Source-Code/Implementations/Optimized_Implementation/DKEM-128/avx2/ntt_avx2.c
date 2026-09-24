/*
 * AVX2 NTT / INVNTT / basemul for all DKE modes.
 * Only compiled when DKE_USE_AVX2 is defined.
 */
#include "../parameters.h"

#if defined(DKE_USE_AVX2)

#include <immintrin.h>
#include <stdint.h>
#include "../ntt.h"
#include "../reduce.h"

#if defined(DKM_LINUX)
#include "../avx2-linux/ntt_avx2_asm.h"
#endif

static inline __m256i avx2_montgomery_reduce(__m256i lo, __m256i hi) {
    __m256i qinv = _mm256_set1_epi16((int16_t)DKE_QINV);
    __m256i q    = _mm256_set1_epi16((int16_t)DKE_Q);
    __m256i t    = _mm256_mullo_epi16(lo, qinv);
    t = _mm256_mulhi_epi16(t, q);
    return _mm256_sub_epi16(hi, t);
}

static inline __m256i avx2_fqmul(__m256i a, __m256i b) {
    __m256i lo = _mm256_mullo_epi16(a, b);
    __m256i hi = _mm256_mulhi_epi16(a, b);
    return avx2_montgomery_reduce(lo, hi);
}

static inline __m256i avx2_barrett_reduce(__m256i a) {
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

static inline void butterfly_ct(__m256i *r0, __m256i *r1, __m256i zeta) {
    __m256i t = avx2_fqmul(zeta, *r1);
    *r1 = _mm256_sub_epi16(*r0, t);
    *r0 = _mm256_add_epi16(*r0, t);
}

static inline void butterfly_gs(__m256i *r0, __m256i *r1, __m256i zeta) {
    __m256i t = *r0;
    *r0 = avx2_barrett_reduce(_mm256_add_epi16(t, *r1));
    *r1 = avx2_fqmul(zeta, _mm256_sub_epi16(*r1, t));
}

static inline int16_t fqmul_s(int16_t a, int16_t b) {
    return DKE_montgomery_reduce((int32_t)a * b);
}

#if defined(DKE_USE_AVX2) && (DKE_N == 512)
/* ---- Register-blocked two-half forward NTT for n=512, q=7681 --------------
 * Design: the first layer (len=256) couples the two 256-coeff halves; every
 * subsequent layer (len<=128) stays within a half, so the transform decomposes
 * into ONE cross-half butterfly layer + TWO independent q=7681 n=256 NTTs, each
 * of which would keep its 16 vectors in YMM registers across all remaining
 * layers. NOTE (empirical): in INTRINSICS the compiler spills the 16-vector
 * array (16 data + zeta + temps > 16 YMM), so this REGRESSES vs the generic loop
 * (~1300 -> ~1560 cyc). It is kept as the verified reference for the hand-
 * scheduled ASM realization (where the 16 regs are managed explicitly); that is
 * precisely why assembly is required for the n=512 ring. Opt-in via DKE_NTT512_RB.
 * Bit-identical to the generic intrinsic NTT (same DKE_zetas, same butterflies).
 * Per-half zeta bases: half0 {2,4,8,16,32,64,128}, half1 {3,6,12,24,48,96,192}. */
static void dke_ntt512_rb(int16_t r[512]) {
    int h, i, stride, layer; unsigned g, ng, m, s, k2;
    /* layer len=256: v[m] (r[16m]) <-> v[m+16] (r[256+16m]) */
    {
        __m256i zeta = _mm256_set1_epi16(DKE_zetas[1]);
        for (i = 0; i < 16; i++) {
            __m256i a = _mm256_load_si256((__m256i *)&r[16*i]);
            __m256i b = _mm256_load_si256((__m256i *)&r[256 + 16*i]);
            butterfly_ct(&a, &b, zeta);
            _mm256_store_si256((__m256i *)&r[16*i], a);
            _mm256_store_si256((__m256i *)&r[256 + 16*i], b);
        }
    }
    static const unsigned B0[7] = {2,4,8,16,32,64,128};
    static const unsigned B1[7] = {3,6,12,24,48,96,192};
    for (h = 0; h < 2; h++) {
        int16_t *rh = r + 256*h;
        const unsigned *B = h ? B1 : B0;
        __m256i v[16];
        for (i = 0; i < 16; i++) v[i] = _mm256_load_si256((__m256i *)&rh[16*i]);
        /* cross-vector layers len=128,64,32,16  (stride 8,4,2,1 in vectors) */
        layer = 0;
        for (stride = 8; stride >= 1; stride >>= 1, layer++) {
            ng = 16u / (2u*stride);
            for (g = 0; g < ng; g++) {
                __m256i zeta = _mm256_set1_epi16(DKE_zetas[B[layer] + g]);
                s = g * 2u * stride;
                for (m = s; m < s + (unsigned)stride; m++)
                    butterfly_ct(&v[m], &v[m + stride], zeta);
            }
        }
        /* within-vector layers len=8,4,2 (bases B[4],B[5],B[6]) */
        for (i = 0; i < 16; i++) {
            __m256i x = v[i], a, b, t;
            __m256i z8 = _mm256_set1_epi16(DKE_zetas[B[4] + i]);
            a = _mm256_castsi128_si256(_mm256_castsi256_si128(x));
            b = _mm256_castsi128_si256(_mm256_extracti128_si256(x, 1));
            t = avx2_fqmul(z8, b);
            x = _mm256_inserti128_si256(_mm256_add_epi16(a, t),
                    _mm256_castsi256_si128(_mm256_sub_epi16(a, t)), 1);
            { int16_t za = DKE_zetas[B[5]+2*i], zb = DKE_zetas[B[5]+2*i+1];
              __m256i z4 = _mm256_setr_epi16(za,za,za,za,za,za,za,za, zb,zb,zb,zb,zb,zb,zb,zb);
              a = _mm256_unpacklo_epi64(x, x);
              b = _mm256_unpackhi_epi64(x, x);
              t = avx2_fqmul(z4, b);
              x = _mm256_unpacklo_epi64(_mm256_add_epi16(a, t), _mm256_sub_epi16(a, t)); }
            k2 = B[6] + 4u*i;
            { __m256i z2 = _mm256_setr_epi16(
                DKE_zetas[k2],   DKE_zetas[k2],   DKE_zetas[k2],   DKE_zetas[k2],
                DKE_zetas[k2+1], DKE_zetas[k2+1], DKE_zetas[k2+1], DKE_zetas[k2+1],
                DKE_zetas[k2+2], DKE_zetas[k2+2], DKE_zetas[k2+2], DKE_zetas[k2+2],
                DKE_zetas[k2+3], DKE_zetas[k2+3], DKE_zetas[k2+3], DKE_zetas[k2+3]);
              a = _mm256_shuffle_epi32(x, 0xA0);
              b = _mm256_shuffle_epi32(x, 0xF5);
              t = avx2_fqmul(z2, b);
              x = _mm256_blend_epi32(_mm256_add_epi16(a, t), _mm256_sub_epi16(a, t), 0xAA); }
            v[i] = x;
        }
        for (i = 0; i < 16; i++) _mm256_store_si256((__m256i *)&rh[16*i], v[i]);
    }
}
#endif

void DKE_ntt(int16_t r[DKE_N]) {
    unsigned int start, k;
    unsigned int len, j;
    (void)len; (void)j; (void)start; (void)k;

#if defined(DKE_AVX2_NTT256_ASM) && DKE_N == 256
    /* Vendored ntt256 AVX2 forward NTT: leaves coeffs in ntt(packed) order, UNREDUCED
     * (like ntt256's own poly_ntt). The single reduce comes from the DKE_poly_reduce
     * that every DKE_poly_ntt runs right after — so we avoid a redundant reduce pass
     * (and its xmm6-15 save/restore on Windows). basemul_avx tolerates unreduced
     * inputs; serialization is always preceded by a reduce. */
    {
        extern void dke_k_ntt_avx(int16_t *, const int16_t *);
        extern const int16_t dke_k_qdata[];
        dke_k_ntt_avx(r, dke_k_qdata);
        return;
    }
#endif
#if defined(DKE_NTT512_PACKED) && DKE_N == 512
    /* Packed-domain (bitreversed) packed-NTT forward NTT for n=512, q=7681.
     * Output is in the AVX2-internal layout consumed by dke_p512_basemul_avx /
     * dke_p512_invntt_avx; serialization re-packs via nttpack. */
    {
        extern void dke_p512_ntt_avx(int16_t *, const int16_t *);
        extern const int16_t dke_p512_qdata[];
        dke_p512_ntt_avx(r, dke_p512_qdata);
        return;
    }
#endif
#if defined(DKE_NTT512_RB) && DKE_N == 512
    dke_ntt512_rb(r);
    return;
#endif
#if defined(DKE_NTT512_ASM) && DKE_N == 512
    /* Hand-scheduled GAS/MASM register-blocked forward NTT for n=512, q=7681.
     * Bit-identical to the generic intrinsic (verified 2000-poly fuzz, KAT==ref). */
    {
        extern void dke_ntt512_gas(int16_t *, const int16_t *, const int16_t *);
        extern const int16_t dke_qdata512[];
        dke_ntt512_gas(r, dke_qdata512, DKE_zetas);
        return;
    }
#endif
#if defined(DKE_AVX2_NTT_INTRINSIC)
    /* Natural-order intrinsic forward NTT (canonical, matches scalar). */
    k = 1;
    for (len = DKE_N / 2; len >= 16; len >>= 1) {
        for (start = 0; start < DKE_N; start += 2 * len) {
            __m256i zeta = _mm256_set1_epi16(DKE_zetas[k++]);
            for (j = start; j < start + len; j += 16) {
                __m256i a = _mm256_load_si256((__m256i *)&r[j]);
                __m256i b = _mm256_load_si256((__m256i *)&r[j + len]);
                butterfly_ct(&a, &b, zeta);
                _mm256_store_si256((__m256i *)&r[j], a);
                _mm256_store_si256((__m256i *)&r[j + len], b);
            }
        }
    }
    /* len=8 */
    for (start = 0; start < DKE_N; start += 16) {
        __m256i zeta = _mm256_set1_epi16(DKE_zetas[k++]);
        __m256i v = _mm256_load_si256((__m256i *)&r[start]);
        __m256i a256 = _mm256_castsi128_si256(_mm256_castsi256_si128(v));
        __m256i b256 = _mm256_castsi128_si256(_mm256_extracti128_si256(v, 1));
        __m256i t = avx2_fqmul(zeta, b256);
        __m256i add = _mm256_add_epi16(a256, t);
        __m256i sub = _mm256_sub_epi16(a256, t);
        v = _mm256_inserti128_si256(add, _mm256_castsi256_si128(sub), 1);
        _mm256_store_si256((__m256i *)&r[start], v);
    }
    /* len=4 */
    for (start = 0; start < DKE_N; start += 16) {
        int16_t za = DKE_zetas[k++];
        int16_t zb = DKE_zetas[k++];
        __m256i zeta = _mm256_setr_epi16(za,za,za,za,za,za,za,za, zb,zb,zb,zb,zb,zb,zb,zb);
        __m256i v = _mm256_load_si256((__m256i *)&r[start]);
        __m256i a = _mm256_unpacklo_epi64(v, v);
        __m256i b = _mm256_unpackhi_epi64(v, v);
        __m256i t = avx2_fqmul(zeta, b);
        __m256i add = _mm256_add_epi16(a, t);
        __m256i sub = _mm256_sub_epi16(a, t);
        v = _mm256_unpacklo_epi64(add, sub);
        _mm256_store_si256((__m256i *)&r[start], v);
    }
    /* len=2 */
    for (start = 0; start < DKE_N; start += 16) {
        __m256i zeta = _mm256_setr_epi16(
            DKE_zetas[k],   DKE_zetas[k],   DKE_zetas[k],   DKE_zetas[k],
            DKE_zetas[k+1], DKE_zetas[k+1], DKE_zetas[k+1], DKE_zetas[k+1],
            DKE_zetas[k+2], DKE_zetas[k+2], DKE_zetas[k+2], DKE_zetas[k+2],
            DKE_zetas[k+3], DKE_zetas[k+3], DKE_zetas[k+3], DKE_zetas[k+3]);
        k += 4;
        __m256i v = _mm256_load_si256((__m256i *)&r[start]);
        __m256i a = _mm256_shuffle_epi32(v, 0xA0);
        __m256i b = _mm256_shuffle_epi32(v, 0xF5);
        __m256i t = avx2_fqmul(zeta, b);
        __m256i add = _mm256_add_epi16(a, t);
        __m256i sub = _mm256_sub_epi16(a, t);
        v = _mm256_blend_epi32(add, sub, 0xAA);
        _mm256_store_si256((__m256i *)&r[start], v);
    }
    return;
#endif

#if defined(DKM_LINUX)
#if DKE_N == 256
    {
        extern void dke_ntt_dkek_avx2_linux(int16_t *r, const int16_t *qdata);
        extern const int16_t dke_qdata[];
        /* packed-NTT NTT, outputs packed format */
        dke_ntt_dkek_avx2_linux(r, dke_qdata);
        return;
    }
#elif DKE_N == 512
    k = dke_ntt512_avx2_asm(r, DKE_zetas);
#else
    k = dke_ntt_avx2_asm(r, DKE_zetas, DKE_N);
#endif
    for (len = 8; len >= 2; len >>= 1) {
        for (start = 0; start < DKE_N; start += 2 * len) {
            int16_t zeta = DKE_zetas[k++];
            for (j = start; j < start + len; j++) {
                int16_t t = fqmul_s(zeta, r[j + len]);
                r[j + len] = r[j] - t;
                r[j] = r[j] + t;
            }
        }
    }
#else
    k = 1;
#if DKE_N == 256 && defined(_MSC_VER)
    {
        extern void dke_ntt_dkek_avx2(int16_t *r, const int16_t *qdata);
        extern const int16_t dke_qdata[];
        /* packed-NTT NTT, outputs packed format */
        dke_ntt_dkek_avx2(r, dke_qdata);
        (void)k; (void)start;
    }
#elif DKE_N == 256 && defined(DKM_LINUX)
    {
        extern void dke_ntt_unrolled_avx2_linux(int16_t *r, const int16_t *zetas, const int16_t *qdata);
        extern const int16_t dke_qdata[];
        dke_ntt_unrolled_avx2_linux(r, DKE_zetas + 1, dke_qdata);
        k = 16;
    }
#elif DKE_N == 512 && defined(_MSC_VER)
    {
        extern void dke_ntt_large512_avx2(int16_t *r, const int16_t *zetas, const int16_t *qdata512);
        extern const int16_t dke_qdata512[32];
        dke_ntt_large512_avx2(r, DKE_zetas + 1, dke_qdata512);
        k = 32; /* zetas[1..31] consumed */
    }
#elif DKE_N == 512 && defined(DKM_LINUX)
    {
        extern void dke_ntt_large512_avx2_linux(int16_t *r, const int16_t *zetas, const int16_t *qdata512);
        extern const int16_t dke_qdata512[32];
        dke_ntt_large512_avx2_linux(r, DKE_zetas + 1, dke_qdata512);
        k = 32;
    }
#else
    for (len = DKE_N / 2; len >= 16; len >>= 1) {
        for (start = 0; start < DKE_N; start += 2 * len) {
            __m256i zeta = _mm256_set1_epi16(DKE_zetas[k++]);
            for (j = start; j < start + len; j += 16) {
                __m256i a = _mm256_load_si256((__m256i *)&r[j]);
                __m256i b = _mm256_load_si256((__m256i *)&r[j + len]);
                butterfly_ct(&a, &b, zeta);
                _mm256_store_si256((__m256i *)&r[j], a);
                _mm256_store_si256((__m256i *)&r[j + len], b);
            }
        }
    }
#endif
    /* len=8,4,2: small layers — only needed when assembly didn't handle them */
#if !(DKE_N == 256 && defined(_MSC_VER))
    /* len=8: CT butterfly between r[s..s+7] and r[s+8..s+15]. */
    for (start = 0; start < DKE_N; start += 16) {
        __m256i zeta = _mm256_set1_epi16(DKE_zetas[k++]);
        __m256i v = _mm256_load_si256((__m256i *)&r[start]);
        /* a = low 128 bits (r[s..s+7]), b = high 128 bits (r[s+8..s+15]) */
        __m128i a128 = _mm256_castsi256_si128(v);
        __m128i b128 = _mm256_extracti128_si256(v, 1);
        /* Extend to 256-bit for fqmul (only low 128 bits used) */
        __m256i a256 = _mm256_castsi128_si256(a128);
        __m256i b256 = _mm256_castsi128_si256(b128);
        __m256i t = avx2_fqmul(zeta, b256);
        __m256i add = _mm256_add_epi16(a256, t);
        __m256i sub = _mm256_sub_epi16(a256, t);
        /* Recombine: low 128 from add, high 128 from sub */
        v = _mm256_inserti128_si256(add, _mm256_castsi256_si128(sub), 1);
        _mm256_store_si256((__m256i *)&r[start], v);
    }
    /* len=4: CT butterfly between r[s+j] and r[s+j+4], j=0..3.
     * In 16 consecutive coeffs: 2 groups of 8, each with its own zeta.
     * Group 0: [c0..c3] vs [c4..c7] with zeta_a
     * Group 1: [c8..c11] vs [c12..c15] with zeta_b */
    for (start = 0; start < DKE_N; start += 16) {
        int16_t za = DKE_zetas[k++];
        int16_t zb = DKE_zetas[k++];
        __m256i zeta = _mm256_setr_epi16(
            za, za, za, za, za, za, za, za,
            zb, zb, zb, zb, zb, zb, zb, zb);
        __m256i v = _mm256_load_si256((__m256i *)&r[start]);
        /* a = [c0..c3, c0..c3, c8..c11, c8..c11] (low 64 bits of each 128-bit lane duplicated) */
        __m256i a = _mm256_unpacklo_epi64(v, v);
        /* b = [c4..c7, c4..c7, c12..c15, c12..c15] (high 64 bits duplicated) */
        __m256i b = _mm256_unpackhi_epi64(v, v);
        __m256i t = avx2_fqmul(zeta, b);
        __m256i add = _mm256_add_epi16(a, t);
        __m256i sub = _mm256_sub_epi16(a, t);
        /* Recombine: low 64 from add, high 64 from sub in each 128-bit lane */
        v = _mm256_unpacklo_epi64(add, sub);
        _mm256_store_si256((__m256i *)&r[start], v);
    }
    /* len=2: CT butterfly between r[s+j] and r[s+j+2], j=0..1.
     * In 16 consecutive coeffs: 4 groups of 4, each with its own zeta.
     * Use shuffle_epi32 to separate even/odd 32-bit words (pairs of 2 int16). */
    for (start = 0; start < DKE_N; start += 16) {
        __m256i zeta = _mm256_setr_epi16(
            DKE_zetas[k], DKE_zetas[k], DKE_zetas[k], DKE_zetas[k],
            DKE_zetas[k+1], DKE_zetas[k+1], DKE_zetas[k+1], DKE_zetas[k+1],
            DKE_zetas[k+2], DKE_zetas[k+2], DKE_zetas[k+2], DKE_zetas[k+2],
            DKE_zetas[k+3], DKE_zetas[k+3], DKE_zetas[k+3], DKE_zetas[k+3]);
        k += 4;
        __m256i v = _mm256_load_si256((__m256i *)&r[start]);
        /* a = even 32-bit words: [c0,c1, c0,c1, c4,c5, c4,c5, c8,c9, c8,c9, c12,c13, c12,c13] */
        __m256i a = _mm256_shuffle_epi32(v, 0xA0); /* 10 10 00 00 per 128-bit lane */
        /* b = odd 32-bit words: [c2,c3, c2,c3, c6,c7, c6,c7, c10,c11, c10,c11, c14,c15, c14,c15] */
        __m256i b = _mm256_shuffle_epi32(v, 0xF5); /* 11 11 01 01 per 128-bit lane */
        __m256i t = avx2_fqmul(zeta, b);
        __m256i add = _mm256_add_epi16(a, t);
        __m256i sub = _mm256_sub_epi16(a, t);
        /* Recombine: even 32-bit from add, odd 32-bit from sub */
        v = _mm256_blend_epi32(add, sub, 0xAA); /* 10101010 */
        _mm256_store_si256((__m256i *)&r[start], v);
    }
#endif /* !(DKE_N == 256 && _MSC_VER) */
#endif /* !DKM_LINUX */
}

void DKE_invntt(int16_t r[DKE_N]) {
    unsigned int start, len, j, k;
    const int16_t f = DKE_INVNTT_F;
    (void)f; (void)start; (void)len; (void)j;

#if defined(DKE_AVX2_NTT256_ASM) && DKE_N == 256
    {
        extern void dke_k_invntt_avx(int16_t *, const int16_t *);
        extern const int16_t dke_k_qdata[];
        dke_k_invntt_avx(r, dke_k_qdata);   /* ntt-order -> natural, includes tomont */
        return;
    }
#endif
#if defined(DKE_NTT512_PACKED) && DKE_N == 512
    /* Packed-domain packed-NTT inverse NTT for n=512, q=7681. Input in the
     * AVX2-internal layout, output in standard order (×R^2 / tomont folded in). */
    {
        extern void dke_p512_invntt_avx(int16_t *, const int16_t *);
        extern const int16_t dke_p512_qdata[];
        dke_p512_invntt_avx(r, dke_p512_qdata);
        return;
    }
#endif
#if defined(DKE_NTT512_ASM) && DKE_N == 512
    /* Hand-scheduled GAS/MASM register-blocked inverse NTT for n=512, q=7681.
     * Bit-identical to the intrinsic inverse (verified 2000-poly fuzz, KAT==ref). */
    {
        extern void dke_invntt512_gas(int16_t *, const int16_t *, const int16_t *);
        extern const int16_t dke_qdata512[];
        dke_invntt512_gas(r, dke_qdata512, DKE_zetas);
        return;
    }
#endif

#if defined(DKE_AVX2_NTT_INTRINSIC)
    /* Natural-order inverse-NTT consistent with the intrinsic forward NTT.
     * Small layers (len=2,4,8) are scalar (their shuffled vector form is fiddly
     * and low payoff); large layers (len>=16) and the final f-multiply use AVX2.
     * Correct for N=256 and N=512. */
    {
        k = DKE_NTT_ZETAS_LEN - 1;
        /* small layers (len=2,4,8): AVX2 16-wide GS butterflies via in-register
         * shuffles, with per-group zeta vectors. Consumes the same zetas in the
         * same order as the scalar GS loop (len=2: 4 groups/vector, len=4: 2,
         * len=8: 1). Lifted from the validated DKM_LINUX small-layer fallback. */
        /* len=2: GS between adjacent 32-bit halves */
        for (start = 0; start < DKE_N; start += 16) {
            __m256i zeta = _mm256_setr_epi16(
                DKE_zetas[k],   DKE_zetas[k],   DKE_zetas[k],   DKE_zetas[k],
                DKE_zetas[k-1], DKE_zetas[k-1], DKE_zetas[k-1], DKE_zetas[k-1],
                DKE_zetas[k-2], DKE_zetas[k-2], DKE_zetas[k-2], DKE_zetas[k-2],
                DKE_zetas[k-3], DKE_zetas[k-3], DKE_zetas[k-3], DKE_zetas[k-3]);
            k -= 4;
            __m256i v = _mm256_load_si256((__m256i *)&r[start]);
            __m256i a = _mm256_shuffle_epi32(v, 0xA0);
            __m256i b = _mm256_shuffle_epi32(v, 0xF5);
            __m256i add = avx2_barrett_reduce(_mm256_add_epi16(a, b));
            __m256i sub = avx2_fqmul(zeta, _mm256_sub_epi16(b, a));
            v = _mm256_blend_epi32(add, sub, 0xAA);
            _mm256_store_si256((__m256i *)&r[start], v);
        }
        /* len=4: GS between adjacent 64-bit halves */
        for (start = 0; start < DKE_N; start += 16) {
            __m256i zeta = _mm256_setr_epi16(
                DKE_zetas[k],   DKE_zetas[k],   DKE_zetas[k],   DKE_zetas[k],
                DKE_zetas[k],   DKE_zetas[k],   DKE_zetas[k],   DKE_zetas[k],
                DKE_zetas[k-1], DKE_zetas[k-1], DKE_zetas[k-1], DKE_zetas[k-1],
                DKE_zetas[k-1], DKE_zetas[k-1], DKE_zetas[k-1], DKE_zetas[k-1]);
            k -= 2;
            __m256i v = _mm256_load_si256((__m256i *)&r[start]);
            __m256i a = _mm256_unpacklo_epi64(v, v);
            __m256i b = _mm256_unpackhi_epi64(v, v);
            __m256i add = avx2_barrett_reduce(_mm256_add_epi16(a, b));
            __m256i sub = avx2_fqmul(zeta, _mm256_sub_epi16(b, a));
            v = _mm256_unpacklo_epi64(add, sub);
            _mm256_store_si256((__m256i *)&r[start], v);
        }
        /* len=8: GS between low/high 128-bit halves */
        for (start = 0; start < DKE_N; start += 16) {
            __m256i zeta = _mm256_set1_epi16(DKE_zetas[k--]);
            __m256i v = _mm256_load_si256((__m256i *)&r[start]);
            __m256i a = _mm256_castsi128_si256(_mm256_castsi256_si128(v));
            __m256i b = _mm256_castsi128_si256(_mm256_extracti128_si256(v, 1));
            __m256i add = avx2_barrett_reduce(_mm256_add_epi16(a, b));
            __m256i sub = avx2_fqmul(zeta, _mm256_sub_epi16(b, a));
            v = _mm256_inserti128_si256(add, _mm256_castsi256_si128(sub), 1);
            _mm256_store_si256((__m256i *)&r[start], v);
        }
        /* large layers: AVX2 16-wide GS butterflies */
        for (len = 16; len <= DKE_N / 2; len <<= 1) {
            for (start = 0; start < DKE_N; start += 2 * len) {
                __m256i zv = _mm256_set1_epi16(DKE_zetas[k--]);
                for (j = start; j < start + len; j += 16) {
                    __m256i a = _mm256_load_si256((__m256i *)&r[j]);
                    __m256i b = _mm256_load_si256((__m256i *)&r[j + len]);
                    butterfly_gs(&a, &b, zv);
                    _mm256_store_si256((__m256i *)&r[j], a);
                    _mm256_store_si256((__m256i *)&r[j + len], b);
                }
            }
        }
        /* multiply by f (Montgomery) */
        {
            __m256i fv = _mm256_set1_epi16(f);
            for (j = 0; j < DKE_N; j += 16) {
                __m256i rv = _mm256_load_si256((__m256i *)&r[j]);
                rv = avx2_fqmul(rv, fv);
                _mm256_store_si256((__m256i *)&r[j], rv);
            }
        }
        return;
    }
#endif

#if DKE_N == 256 && defined(_MSC_VER)
    {
        /* packed-NTT fused INVNTT: operates directly on packed NTT format,
         * no separate unpack step needed. */
        extern void dke_invntt_dkek_avx2(int16_t *r, const int16_t *qdata);
        extern const int16_t dke_qdata[];
        dke_invntt_dkek_avx2(r, dke_qdata);
        return;
    }
#elif DKE_N == 256 && defined(DKM_LINUX)
    {
        /* packed-NTT fused INVNTT: operates directly on packed NTT format,
         * no separate unpack step needed. ~130 cycles vs ~520 cycles. */
        extern void dke_invntt_dkek_avx2_linux(int16_t *r, const int16_t *qdata);
        extern const int16_t dke_qdata[];
        dke_invntt_dkek_avx2_linux(r, dke_qdata);
        return;
    }

    /* len=2: GS butterfly between adjacent 32-bit halves */
    for (start = 0; start < DKE_N; start += 16) {
        __m256i zeta = _mm256_setr_epi16(
            DKE_zetas[k], DKE_zetas[k], DKE_zetas[k], DKE_zetas[k],
            DKE_zetas[k-1], DKE_zetas[k-1], DKE_zetas[k-1], DKE_zetas[k-1],
            DKE_zetas[k-2], DKE_zetas[k-2], DKE_zetas[k-2], DKE_zetas[k-2],
            DKE_zetas[k-3], DKE_zetas[k-3], DKE_zetas[k-3], DKE_zetas[k-3]);
        k -= 4;
        __m256i v = _mm256_load_si256((__m256i *)&r[start]);
        __m256i a = _mm256_shuffle_epi32(v, 0xA0);
        __m256i b = _mm256_shuffle_epi32(v, 0xF5);
        __m256i add = avx2_barrett_reduce(_mm256_add_epi16(a, b));
        __m256i sub = avx2_fqmul(zeta, _mm256_sub_epi16(b, a));
        v = _mm256_blend_epi32(add, sub, 0xAA);
        _mm256_store_si256((__m256i *)&r[start], v);
    }

    /* len=4: GS butterfly between adjacent 64-bit halves */
    for (start = 0; start < DKE_N; start += 16) {
        __m256i zeta = _mm256_setr_epi16(
            DKE_zetas[k], DKE_zetas[k], DKE_zetas[k], DKE_zetas[k],
            DKE_zetas[k], DKE_zetas[k], DKE_zetas[k], DKE_zetas[k],
            DKE_zetas[k-1], DKE_zetas[k-1], DKE_zetas[k-1], DKE_zetas[k-1],
            DKE_zetas[k-1], DKE_zetas[k-1], DKE_zetas[k-1], DKE_zetas[k-1]);
        k -= 2;
        __m256i v = _mm256_load_si256((__m256i *)&r[start]);
        __m256i a = _mm256_unpacklo_epi64(v, v);
        __m256i b = _mm256_unpackhi_epi64(v, v);
        __m256i add = avx2_barrett_reduce(_mm256_add_epi16(a, b));
        __m256i sub = avx2_fqmul(zeta, _mm256_sub_epi16(b, a));
        v = _mm256_unpacklo_epi64(add, sub);
        _mm256_store_si256((__m256i *)&r[start], v);
    }

    /* len=8: GS butterfly between low/high 128-bit halves */
    for (start = 0; start < DKE_N; start += 16) {
        __m256i zeta = _mm256_set1_epi16(DKE_zetas[k--]);
        __m256i v = _mm256_load_si256((__m256i *)&r[start]);
        __m128i a128 = _mm256_castsi256_si128(v);
        __m128i b128 = _mm256_extracti128_si256(v, 1);
        __m256i a = _mm256_castsi128_si256(a128);
        __m256i b = _mm256_castsi128_si256(b128);
        __m256i add = avx2_barrett_reduce(_mm256_add_epi16(a, b));
        __m256i sub = avx2_fqmul(zeta, _mm256_sub_epi16(b, a));
        v = _mm256_inserti128_si256(add, _mm256_castsi256_si128(sub), 1);
        _mm256_store_si256((__m256i *)&r[start], v);
    }

#endif /* DKE_N == 256 && DKM_LINUX */

    k = DKE_NTT_ZETAS_LEN - 1;
    len = 16;
    for (; len <= DKE_N / 2; len <<= 1) {
        for (start = 0; start < DKE_N; start += 2 * len) {
            __m256i zeta = _mm256_set1_epi16(DKE_zetas[k--]);
            for (j = start; j < start + len; j += 16) {
                __m256i a = _mm256_load_si256((__m256i *)&r[j]);
                __m256i b = _mm256_load_si256((__m256i *)&r[j + len]);
                butterfly_gs(&a, &b, zeta);
                _mm256_store_si256((__m256i *)&r[j], a);
                _mm256_store_si256((__m256i *)&r[j + len], b);
            }
        }
    }
    {
        __m256i fv = _mm256_set1_epi16(f);
        for (j = 0; j < DKE_N; j += 16) {
            __m256i rv = _mm256_load_si256((__m256i *)&r[j]);
            rv = avx2_fqmul(rv, fv);
            _mm256_store_si256((__m256i *)&r[j], rv);
        }
    }
}

void DKE_basemul(int16_t r[2], const int16_t a[2], const int16_t b[2], int16_t zeta) {
    r[0]  = fqmul_s(a[1], b[1]);
    r[0]  = fqmul_s(r[0], zeta);
    r[0] += fqmul_s(a[0], b[0]);
    r[1]  = fqmul_s(a[0], b[1]);
    r[1] += fqmul_s(a[1], b[0]);
}

#endif /* DKE_USE_AVX2 */
