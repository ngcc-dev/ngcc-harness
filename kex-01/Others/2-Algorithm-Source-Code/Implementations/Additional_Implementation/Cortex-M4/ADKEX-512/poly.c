#ifdef _MSC_VER
#pragma warning(disable: 4819)
#endif
#include "parameters.h"
#include "poly.h"
#include "reduce.h"
#include "ntt.h"
#include <stdint.h>
#if defined(DKE_USE_AVX2)
#include <immintrin.h>
#endif
#if defined(DKE_USE_AARCH64)
#include <arm_neon.h>
#endif

// Basic arithmetic ----------------------------------------------
/* When AVX2 or AArch64 is enabled, reduce/add/sub/tomont are in platform-specific files */
#if defined(DKE_USE_AVX2)
  /* poly_reduce, poly_add, poly_sub, poly_tomont supplied by avx2/poly_avx2.c */
#elif defined(DKE_USE_AARCH64)
  /* poly_reduce, poly_add, poly_sub, poly_tomont supplied by aarch64/poly_neon.c */
#elif defined(DKE_USE_CORTEX_M4) || defined(DKE_USE_CORTEX_M4_PLANTARD) \
   || defined(DKE_USE_CORTEX_M4_BARRETT)
  /* poly_reduce, poly_add, poly_sub, poly_tomont supplied by cortex-m4/dke_cortex_m4.c */
#else

void DKE_poly_reduce(poly *pol) {
    unsigned int i;
    for (i = 0; i < DKE_N; i++) {
        pol->coeffs[i] = DKE_barrett_reduce(pol->coeffs[i]);
    }
}

void DKE_poly_add(poly *r, const poly *a, const poly *b) {
    unsigned int i;
    for (i = 0; i < DKE_N; i++) {
        r->coeffs[i] = a->coeffs[i] + b->coeffs[i];
    }
}

void DKE_poly_sub(poly *r, const poly *a, const poly *b) {
    unsigned int i;
    for (i = 0; i < DKE_N; i++) {
        r->coeffs[i] = a->coeffs[i] - b->coeffs[i];
    }
}

#endif /* !DKE_USE_AVX2 && !DKE_USE_AARCH64 */

/* Centered Barrett reduction: maps each coefficient to {-(q-1)/2, ..., (q-1)/2}.
 * DKE_barrett_reduce already yields a centered representative, so this is the
 * portable equivalent of the reference reduce_center used in apply_signal. It is
 * defined once for all backends (the scalar Barrett path is correct everywhere). */
void DKE_poly_reduce_center(poly *pol) {
#if defined(DKE_USE_AVX2)
    /* AVX2, BIT-IDENTICAL to the scalar DKE_barrett_reduce: t=(v*a+2^25)>>26; a-t*q.
     * Uses a 32-bit intermediate so the rounding exactly matches the scalar path —
     * the 16-bit mulhi barrett used elsewhere rounds differently and would flip ss
     * bit0. This is apply_signal's hot loop; -O2 left the scalar version un-vectorized
     * (cross-TU call to DKE_barrett_reduce), which is why derive_ss lagged ref's -O3. */
    unsigned int i;
    const __m256i vv  = _mm256_set1_epi32(((1 << 26) + DKE_Q / 2) / DKE_Q);
    const __m256i rnd = _mm256_set1_epi32(1 << 25);
    const __m256i qq  = _mm256_set1_epi32(DKE_Q);
    for (i = 0; i < DKE_N; i += 16) {
        __m256i a   = _mm256_load_si256((const __m256i *)&pol->coeffs[i]);
        __m256i alo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(a));
        __m256i ahi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(a, 1));
        __m256i tlo = _mm256_srai_epi32(_mm256_add_epi32(_mm256_mullo_epi32(vv, alo), rnd), 26);
        __m256i thi = _mm256_srai_epi32(_mm256_add_epi32(_mm256_mullo_epi32(vv, ahi), rnd), 26);
        tlo = _mm256_sub_epi32(alo, _mm256_mullo_epi32(tlo, qq));
        thi = _mm256_sub_epi32(ahi, _mm256_mullo_epi32(thi, qq));
        __m256i r = _mm256_packs_epi32(tlo, thi);
        r = _mm256_permute4x64_epi64(r, 0xD8);   /* fix 128-lane interleave from packs */
        _mm256_store_si256((__m256i *)&pol->coeffs[i], r);
    }
#elif defined(DKE_USE_AARCH64)
    /* NEON Barrett reduce: bit-identical to scalar DKE_barrett_reduce.
     * Uses 32-bit intermediate for exact rounding match with scalar path. */
    unsigned int i;
    const int32x4_t vv  = vdupq_n_s32(((1 << 26) + DKE_Q / 2) / DKE_Q);
    const int32x4_t rnd = vdupq_n_s32(1 << 25);
    const int32x4_t qq  = vdupq_n_s32(DKE_Q);
    for (i = 0; i < DKE_N; i += 8) {
        int16x8_t a = vld1q_s16(&pol->coeffs[i]);
        int32x4_t alo = vmovl_s16(vget_low_s16(a));
        int32x4_t ahi = vmovl_s16(vget_high_s16(a));
        int32x4_t tlo = vshrq_n_s32(vaddq_s32(vmulq_s32(vv, alo), rnd), 26);
        int32x4_t thi = vshrq_n_s32(vaddq_s32(vmulq_s32(vv, ahi), rnd), 26);
        tlo = vsubq_s32(alo, vmulq_s32(tlo, qq));
        thi = vsubq_s32(ahi, vmulq_s32(thi, qq));
        int16x8_t r = vcombine_s16(vmovn_s32(tlo), vmovn_s32(thi));
        vst1q_s16(&pol->coeffs[i], r);
    }
#else
    unsigned int i;
    for (i = 0; i < DKE_N; i++) {
        pol->coeffs[i] = DKE_barrett_reduce(pol->coeffs[i]);
    }
#endif
}

void DKE_poly_nttunpack(poly *pol) {
#if defined(DKE_AVX2_NTT256_ASM) && DKE_N == 256
    extern void dke_k_nttunpack_avx(int16_t *, const int16_t *);
    extern const int16_t dke_k_qdata[];
    dke_k_nttunpack_avx(pol->coeffs, dke_k_qdata);
#elif defined(DKE_NTT512_PACKED) && DKE_N == 512
    extern void dke_p512_nttunpack_avx(int16_t *, const int16_t *);
    extern const int16_t dke_p512_qdata[];
    dke_p512_nttunpack_avx(pol->coeffs, dke_p512_qdata);
#else
    (void)pol;   /* intrinsic/scalar NTT keeps natural order — no permutation */
#endif
}

/* Inverse of DKE_poly_nttunpack: AVX2-internal NTT layout -> standard order.
 * Used by 512 packed serialization (tobytes); no-op for natural-order builds. */
void DKE_poly_nttpack(poly *pol) {
#if defined(DKE_NTT512_PACKED) && DKE_N == 512
    extern void dke_p512_nttpack_avx(int16_t *, const int16_t *);
    extern const int16_t dke_p512_qdata[];
    dke_p512_nttpack_avx(pol->coeffs, dke_p512_qdata);
#else
    (void)pol;
#endif
}

void DKE_poly_scale2(poly *pol) {
#if defined(DKE_USE_AVX2)
    unsigned int i;
    for (i = 0; i < DKE_N; i += 16) {
        __m256i v = _mm256_load_si256((__m256i *)&pol->coeffs[i]);
        v = _mm256_slli_epi16(v, 1);
        _mm256_store_si256((__m256i *)&pol->coeffs[i], v);
    }
#elif defined(DKE_USE_AARCH64)
    unsigned int i;
    for (i = 0; i < DKE_N; i += 8) {
        int16x8_t v = vld1q_s16(&pol->coeffs[i]);
        v = vshlq_n_s16(v, 1);
        vst1q_s16(&pol->coeffs[i], v);
    }
#else
    DKE_poly_add(pol, pol, pol);
#endif
}

// Advanced arithmetic -----------------------------------

void DKE_poly_ntt(poly* pol) {
    DKE_ntt(pol->coeffs);
    DKE_poly_reduce(pol);
}

void DKE_poly_invntt_tomont(poly *pol) {
    DKE_invntt(pol->coeffs);
}

#if defined(DKE_USE_AVX2)
  /* poly_basemul_montgomery supplied by avx2/poly_avx2.c */
#elif defined(DKE_USE_AARCH64)
  /* poly_basemul_montgomery supplied by aarch64/poly_neon.c */
#elif defined(DKE_USE_CORTEX_M4) || defined(DKE_USE_CORTEX_M4_PLANTARD) \
   || defined(DKE_USE_CORTEX_M4_BARRETT)
  /* poly_basemul_montgomery supplied by cortex-m4/dke_cortex_m4.c */
#else
void DKE_poly_basemul_montgomery(poly *res, const poly *a, const poly *b) {
    unsigned int i;
    for (i = 0; i < DKE_N / 4; i++) {
        DKE_basemul(&res->coeffs[4 * i], &a->coeffs[4 * i],
                    &b->coeffs[4 * i], DKE_zetas[DKE_NTT_ZETAS_LEN / 2 + i]);
        DKE_basemul(&res->coeffs[4 * i + 2], &a->coeffs[4 * i + 2],
                    &b->coeffs[4 * i + 2], -DKE_zetas[DKE_NTT_ZETAS_LEN / 2 + i]);
    }
}
#endif /* DKE_USE_AVX2 / DKE_USE_AARCH64 */

#if defined(DKE_USE_AVX2)
  /* poly_tomont supplied by avx2/poly_avx2.c */
#elif defined(DKE_USE_AARCH64)
  /* poly_tomont supplied by aarch64/poly_neon.c */
#elif defined(DKE_USE_CORTEX_M4) || defined(DKE_USE_CORTEX_M4_PLANTARD) \
   || defined(DKE_USE_CORTEX_M4_BARRETT)
  /* poly_tomont supplied by cortex-m4/dke_cortex_m4.c */
#else
void DKE_poly_tomont(poly *pol) {
    unsigned int i;
    const int16_t f = (1ULL << 32) % DKE_Q;
    for (i = 0; i < DKE_N; i++) {
        pol->coeffs[i] = DKE_montgomery_reduce((int32_t)pol->coeffs[i] * f);
    }
}
#endif /* !DKE_USE_AVX2 && !DKE_USE_AARCH64 */

// Serialization: poly <---> bytes ----------------------------

#if defined(DKE_USE_AARCH64)
  /* tobytes, frombytes, getsignal, fromsignal supplied by aarch64/poly_pack_neon.c */
#else

#if DKE_MODE == 512
// 13-bit coefficients: 8 coeffs -> 13 bytes

#if defined(DKE_USE_AVX2)
#include <immintrin.h>
#include <string.h>

void DKE_poly_tobytes(uint8_t bytes[DKE_POLYBYTES], const poly *pol) {
    unsigned int i;
    const __m256i q = _mm256_set1_epi16((int16_t)DKE_Q);
    const __m256i mask13 = _mm256_set1_epi16(0x1FFF);
#if defined(DKE_NTT512_PACKED) && DKE_N == 512
    /* NTT-domain polys are in AVX2-internal layout; un-permute to standard order
     * (into a local copy, pol is const) before 13-bit serialization. */
    poly tmp;
    memcpy(&tmp, pol, sizeof(poly));
    DKE_poly_nttpack(&tmp);
    pol = &tmp;
#endif

    for (i = 0; i < DKE_N / 16; i++) {
        /* Load 16 coefficients, conditional add Q if negative */
        __m256i v = _mm256_load_si256((__m256i *)&pol->coeffs[16 * i]);
        v = _mm256_add_epi16(v, _mm256_and_si256(_mm256_srai_epi16(v, 15), q));
        v = _mm256_and_si256(v, mask13);

        /* Extract to scalar for 13-bit packing (2 groups of 8 → 13 bytes each) */
        int16_t t[16];
        _mm256_storeu_si256((__m256i *)t, v);

        /* Group 0: t[0..7] → 13 bytes */
        bytes[ 0] = (uint8_t)(t[0]);
        bytes[ 1] = (uint8_t)((t[0] >> 8) | (t[1] << 5));
        bytes[ 2] = (uint8_t)(t[1] >> 3);
        bytes[ 3] = (uint8_t)((t[1] >> 11) | (t[2] << 2));
        bytes[ 4] = (uint8_t)((t[2] >> 6) | (t[3] << 7));
        bytes[ 5] = (uint8_t)(t[3] >> 1);
        bytes[ 6] = (uint8_t)((t[3] >> 9) | (t[4] << 4));
        bytes[ 7] = (uint8_t)(t[4] >> 4);
        bytes[ 8] = (uint8_t)((t[4] >> 12) | (t[5] << 1));
        bytes[ 9] = (uint8_t)((t[5] >> 7) | (t[6] << 6));
        bytes[10] = (uint8_t)(t[6] >> 2);
        bytes[11] = (uint8_t)((t[6] >> 10) | (t[7] << 3));
        bytes[12] = (uint8_t)(t[7] >> 5);

        /* Group 1: t[8..15] → 13 bytes */
        bytes[13] = (uint8_t)(t[8]);
        bytes[14] = (uint8_t)((t[8] >> 8) | (t[9] << 5));
        bytes[15] = (uint8_t)(t[9] >> 3);
        bytes[16] = (uint8_t)((t[9] >> 11) | (t[10] << 2));
        bytes[17] = (uint8_t)((t[10] >> 6) | (t[11] << 7));
        bytes[18] = (uint8_t)(t[11] >> 1);
        bytes[19] = (uint8_t)((t[11] >> 9) | (t[12] << 4));
        bytes[20] = (uint8_t)(t[12] >> 4);
        bytes[21] = (uint8_t)((t[12] >> 12) | (t[13] << 1));
        bytes[22] = (uint8_t)((t[13] >> 7) | (t[14] << 6));
        bytes[23] = (uint8_t)(t[14] >> 2);
        bytes[24] = (uint8_t)((t[14] >> 10) | (t[15] << 3));
        bytes[25] = (uint8_t)(t[15] >> 5);
        bytes += 26;
    }
}

void DKE_poly_frombytes(poly *pol, const uint8_t bytes[DKE_POLYBYTES]) {
    unsigned int i;
    for (i = 0; i < DKE_N / 8; i++) {
        pol->coeffs[8*i]   = ((bytes[13*i+0] >> 0) | ((uint16_t)bytes[13*i+1] << 8)) & 0x1FFF;
        pol->coeffs[8*i+1] = ((bytes[13*i+1] >> 5) | ((uint16_t)bytes[13*i+2] << 3) |
                              ((uint16_t)bytes[13*i+3] << 11)) & 0x1FFF;
        pol->coeffs[8*i+2] = ((bytes[13*i+3] >> 2) | ((uint16_t)bytes[13*i+4] << 6)) & 0x1FFF;
        pol->coeffs[8*i+3] = ((bytes[13*i+4] >> 7) | ((uint16_t)bytes[13*i+5] << 1) |
                              ((uint16_t)bytes[13*i+6] << 9)) & 0x1FFF;
        pol->coeffs[8*i+4] = ((bytes[13*i+6] >> 4) | ((uint16_t)bytes[13*i+7] << 4) |
                              ((uint16_t)bytes[13*i+8] << 12)) & 0x1FFF;
        pol->coeffs[8*i+5] = ((bytes[13*i+8] >> 1) | ((uint16_t)bytes[13*i+9] << 7)) & 0x1FFF;
        pol->coeffs[8*i+6] = ((bytes[13*i+9] >> 6) | ((uint16_t)bytes[13*i+10] << 2) |
                              ((uint16_t)bytes[13*i+11] << 10)) & 0x1FFF;
        pol->coeffs[8*i+7] = ((bytes[13*i+11] >> 3) | ((uint16_t)bytes[13*i+12] << 5)) & 0x1FFF;
    }
#if defined(DKE_NTT512_PACKED) && DKE_N == 512
    /* standard order -> AVX2-internal layout for basemul/invntt */
    DKE_poly_nttunpack(pol);
#endif
}

#else /* scalar fallback for DKE-512 */

void DKE_poly_tobytes(uint8_t bytes[DKE_POLYBYTES], const poly *pol) {
    unsigned int i;
    uint16_t t0, t1, t2, t3, t4, t5, t6, t7;
    for (i = 0; i < DKE_N / 8; i++) {
        t0 = pol->coeffs[8*i]; t0 += ((int16_t)t0 >> 15) & DKE_Q;
        t1 = pol->coeffs[8*i+1]; t1 += ((int16_t)t1 >> 15) & DKE_Q;
        t2 = pol->coeffs[8*i+2]; t2 += ((int16_t)t2 >> 15) & DKE_Q;
        t3 = pol->coeffs[8*i+3]; t3 += ((int16_t)t3 >> 15) & DKE_Q;
        t4 = pol->coeffs[8*i+4]; t4 += ((int16_t)t4 >> 15) & DKE_Q;
        t5 = pol->coeffs[8*i+5]; t5 += ((int16_t)t5 >> 15) & DKE_Q;
        t6 = pol->coeffs[8*i+6]; t6 += ((int16_t)t6 >> 15) & DKE_Q;
        t7 = pol->coeffs[8*i+7]; t7 += ((int16_t)t7 >> 15) & DKE_Q;
        bytes[13*i+ 0] = (uint8_t)(t0 >> 0);
        bytes[13*i+ 1] = (uint8_t)((t0 >> 8)|(t1 << 5));
        bytes[13*i+ 2] = (uint8_t)(t1 >> 3);
        bytes[13*i+ 3] = (uint8_t)((t1 >> 11)|(t2 << 2));
        bytes[13*i+ 4] = (uint8_t)((t2 >> 6)|(t3 << 7));
        bytes[13*i+ 5] = (uint8_t)(t3 >> 1);
        bytes[13*i+ 6] = (uint8_t)((t3 >> 9)|(t4 << 4));
        bytes[13*i+ 7] = (uint8_t)(t4 >> 4);
        bytes[13*i+ 8] = (uint8_t)((t4 >> 12)|(t5 << 1));
        bytes[13*i+ 9] = (uint8_t)((t5 >> 7)|(t6 << 6));
        bytes[13*i+10] = (uint8_t)(t6 >> 2);
        bytes[13*i+11] = (uint8_t)((t6 >> 10)|(t7 << 3));
        bytes[13*i+12] = (uint8_t)(t7 >> 5);
    }
}

void DKE_poly_frombytes(poly *pol, const uint8_t bytes[DKE_POLYBYTES]) {
    unsigned int i;
    for (i = 0; i < DKE_N / 8; i++) {
        pol->coeffs[8*i]   = ((bytes[13*i+0] >> 0) | ((uint16_t)bytes[13*i+1] << 8)) & 0x1FFF;
        pol->coeffs[8*i+1] = ((bytes[13*i+1] >> 5) | ((uint16_t)bytes[13*i+2] << 3) |
                              ((uint16_t)bytes[13*i+3] << 11)) & 0x1FFF;
        pol->coeffs[8*i+2] = ((bytes[13*i+3] >> 2) | ((uint16_t)bytes[13*i+4] << 6)) & 0x1FFF;
        pol->coeffs[8*i+3] = ((bytes[13*i+4] >> 7) | ((uint16_t)bytes[13*i+5] << 1) |
                              ((uint16_t)bytes[13*i+6] << 9)) & 0x1FFF;
        pol->coeffs[8*i+4] = ((bytes[13*i+6] >> 4) | ((uint16_t)bytes[13*i+7] << 4) |
                              ((uint16_t)bytes[13*i+8] << 12)) & 0x1FFF;
        pol->coeffs[8*i+5] = ((bytes[13*i+8] >> 1) | ((uint16_t)bytes[13*i+9] << 7)) & 0x1FFF;
        pol->coeffs[8*i+6] = ((bytes[13*i+9] >> 6) | ((uint16_t)bytes[13*i+10] << 2) |
                              ((uint16_t)bytes[13*i+11] << 10)) & 0x1FFF;
        pol->coeffs[8*i+7] = ((bytes[13*i+11] >> 3) | ((uint16_t)bytes[13*i+12] << 5)) & 0x1FFF;
    }
}

#endif /* DKE_USE_AVX2 for DKE-512 */

#else /* DKE_MODE == 128 or 256: 12-bit coefficients, 2 coeffs -> 3 bytes */

#if defined(DKE_USE_AVX2) && defined(_MSC_VER) && !defined(DKE_AVX2_NTT_INTRINSIC)
/* MSVC AVX2: use packed NTT format (ML-KEM-style) for tobytes/frombytes. */
extern void dke_ntttobytes_avx2(uint8_t *r, const int16_t *a, const int16_t *qdata);
extern void dke_nttfrombytes_avx2(int16_t *r, const uint8_t *a, const int16_t *qdata);
extern const int16_t dke_qdata[];

void DKE_poly_tobytes(uint8_t bytes[DKE_POLYBYTES], const poly *pol) {
    dke_ntttobytes_avx2(bytes, pol->coeffs, dke_qdata);
}

void DKE_poly_frombytes(poly *pol, const uint8_t bytes[DKE_POLYBYTES]) {
    dke_nttfrombytes_avx2(pol->coeffs, bytes, dke_qdata);
}

#elif defined(DKE_USE_AVX2) && defined(DKM_LINUX) && !defined(DKE_AVX2_NTT_INTRINSIC)
/* Linux AVX2: use packed NTT format (ML-KEM-style) for tobytes/frombytes. */
extern void dke_ntttobytes_avx2_linux(uint8_t *r, const int16_t *a, const int16_t *qdata);
extern void dke_nttfrombytes_avx2_linux(int16_t *r, const uint8_t *a, const int16_t *qdata);
extern const int16_t dke_qdata[];

void DKE_poly_tobytes(uint8_t bytes[DKE_POLYBYTES], const poly *pol) {
    dke_ntttobytes_avx2_linux(bytes, pol->coeffs, dke_qdata);
}

void DKE_poly_frombytes(poly *pol, const uint8_t bytes[DKE_POLYBYTES]) {
    dke_nttfrombytes_avx2_linux(pol->coeffs, bytes, dke_qdata);
}

#elif defined(DKE_USE_AVX2)
#include <immintrin.h>

void DKE_poly_tobytes(uint8_t bytes[DKE_POLYBYTES], const poly *pol) {
    unsigned int i;
    const __m256i q = _mm256_set1_epi16((int16_t)DKE_Q);
#if defined(DKE_AVX2_NTT256_ASM)
    /* NTT-domain polys are in ntt(packed) order here; ntttobytes un-permutes to
     * canonical 12-bit serialization. */
    {
        extern void dke_k_ntttobytes_avx(uint8_t *, const int16_t *, const int16_t *);
        extern const int16_t dke_k_qdata[];
        dke_k_ntttobytes_avx(bytes, pol->coeffs, dke_k_qdata);
        (void)q;
        return;
    }
#endif
    for (i = 0; i < DKE_N / 16; i++) {
        __m256i v = _mm256_load_si256((__m256i *)&pol->coeffs[16 * i]);
        __m256i neg = _mm256_srai_epi16(v, 15);
        neg = _mm256_and_si256(neg, q);
        v = _mm256_add_epi16(v, neg);

        int16_t t[16];
        _mm256_storeu_si256((__m256i *)t, v);
        unsigned int j;
        for (j = 0; j < 8; j++) {
            uint16_t t0 = (uint16_t)t[2*j];
            uint16_t t1 = (uint16_t)t[2*j+1];
            bytes[3*(8*i+j)+0] = (uint8_t)(t0 >> 0);
            bytes[3*(8*i+j)+1] = (uint8_t)((t0 >> 8) | (t1 << 4));
            bytes[3*(8*i+j)+2] = (uint8_t)(t1 >> 4);
        }
    }
}

void DKE_poly_frombytes(poly *pol, const uint8_t bytes[DKE_POLYBYTES]) {
    unsigned int i;
#if defined(DKE_AVX2_NTT256_ASM)
    {
        extern void dke_k_nttfrombytes_avx(int16_t *, const uint8_t *, const int16_t *);
        extern const int16_t dke_k_qdata[];
        dke_k_nttfrombytes_avx(pol->coeffs, bytes, dke_k_qdata); /* canonical -> ntt order */
        return;
    }
#endif
    for (i = 0; i < DKE_N / 2; i++) {
        pol->coeffs[2*i]   = ((bytes[3*i+0] >> 0) | ((uint16_t)bytes[3*i+1] << 8)) & 0xFFF;
        pol->coeffs[2*i+1] = ((bytes[3*i+1] >> 4) | ((uint16_t)bytes[3*i+2] << 4)) & 0xFFF;
    }
}

#else

void DKE_poly_tobytes(uint8_t bytes[DKE_POLYBYTES], const poly *pol) {
    unsigned int i;
    uint16_t t0, t1;
    for (i = 0; i < DKE_N / 2; i++) {
        t0 = pol->coeffs[2*i];   t0 += ((int16_t)t0 >> 15) & DKE_Q;
        t1 = pol->coeffs[2*i+1]; t1 += ((int16_t)t1 >> 15) & DKE_Q;
        bytes[3*i+0] = (uint8_t)(t0 >> 0);
        bytes[3*i+1] = (uint8_t)((t0 >> 8) | (t1 << 4));
        bytes[3*i+2] = (uint8_t)(t1 >> 4);
    }
}

void DKE_poly_frombytes(poly *pol, const uint8_t bytes[DKE_POLYBYTES]) {
    unsigned int i;
    for (i = 0; i < DKE_N / 2; i++) {
        pol->coeffs[2*i]   = ((bytes[3*i+0] >> 0) | ((uint16_t)bytes[3*i+1] << 8)) & 0xFFF;
        pol->coeffs[2*i+1] = ((bytes[3*i+1] >> 4) | ((uint16_t)bytes[3*i+2] << 4)) & 0xFFF;
    }
}

#endif /* DKE_USE_AVX2 */
#endif /* DKE_MODE */

// Signal functions ----------------------------

#if DKE_MODE == 128 /* L=4, q=3329 */

#if defined(DKE_USE_AVX2)
#include <immintrin.h>

/* AVX2 getsignal for L=4: same as ML-KEM poly_compress(128) */
void DKE_getsignal(uint8_t bytes[DKE_SIGNALBYTES], const poly *a) {
    unsigned int i;
    const __m256i q = _mm256_set1_epi16((int16_t)DKE_Q);
    const __m256i v = _mm256_set1_epi16(20159); /* round(2^26/Q) */
    const __m256i shift1 = _mm256_set1_epi16(1 << 9);
    const __m256i mask = _mm256_set1_epi16(15);
    const __m256i shift2 = _mm256_set1_epi16((16 << 8) + 1);
    const __m256i permdidx = _mm256_set_epi32(7,3,6,2,5,1,4,0);

    for (i = 0; i < DKE_N / 64; i++) {
        __m256i f0 = _mm256_load_si256((__m256i *)&a->coeffs[64*i]);
        __m256i f1 = _mm256_load_si256((__m256i *)&a->coeffs[64*i+16]);
        __m256i f2 = _mm256_load_si256((__m256i *)&a->coeffs[64*i+32]);
        __m256i f3 = _mm256_load_si256((__m256i *)&a->coeffs[64*i+48]);

        /* Map to positive representatives */
        f0 = _mm256_add_epi16(f0, _mm256_and_si256(_mm256_srai_epi16(f0, 15), q));
        f1 = _mm256_add_epi16(f1, _mm256_and_si256(_mm256_srai_epi16(f1, 15), q));
        f2 = _mm256_add_epi16(f2, _mm256_and_si256(_mm256_srai_epi16(f2, 15), q));
        f3 = _mm256_add_epi16(f3, _mm256_and_si256(_mm256_srai_epi16(f3, 15), q));

        /* Compress: round(x * 16 / Q) */
        f0 = _mm256_mulhi_epi16(f0, v);
        f1 = _mm256_mulhi_epi16(f1, v);
        f2 = _mm256_mulhi_epi16(f2, v);
        f3 = _mm256_mulhi_epi16(f3, v);
        f0 = _mm256_mulhrs_epi16(f0, shift1);
        f1 = _mm256_mulhrs_epi16(f1, shift1);
        f2 = _mm256_mulhrs_epi16(f2, shift1);
        f3 = _mm256_mulhrs_epi16(f3, shift1);
        f0 = _mm256_and_si256(f0, mask);
        f1 = _mm256_and_si256(f1, mask);
        f2 = _mm256_and_si256(f2, mask);
        f3 = _mm256_and_si256(f3, mask);

        /* Pack 4-bit values into bytes */
        f0 = _mm256_packus_epi16(f0, f1);
        f2 = _mm256_packus_epi16(f2, f3);
        f0 = _mm256_maddubs_epi16(f0, shift2);
        f2 = _mm256_maddubs_epi16(f2, shift2);
        f0 = _mm256_packus_epi16(f0, f2);
        f0 = _mm256_permutevar8x32_epi32(f0, permdidx);
        _mm256_storeu_si256((__m256i *)&bytes[32*i], f0);
    }
}

/* AVX2 fromsignal for L=4. Ref uses TRUNCATION: coeff = (v*q) >> 4 = floor(v*q/16),
 * NOT rounding. We align each nibble to <<12 then mulhi_epu16(.,q) = floor(v*q/16)
 * exactly (mulhrs would round, giving +1 for e.g. v=8: 1665 vs ref 1664). */
void DKE_poly_fromsignal(poly *r, const uint8_t a[DKE_SIGNALBYTES]) {
    unsigned int i;
    const __m256i q = _mm256_set1_epi16((int16_t)DKE_Q);
    const __m256i shufbidx = _mm256_set_epi8(
        7,7,7,7,6,6,6,6,5,5,5,5,4,4,4,4,
        3,3,3,3,2,2,2,2,1,1,1,1,0,0,0,0);
    const __m256i fmask = _mm256_set1_epi32(0x00F0000F);
    /* low16 lane (low nibble n) *= 4096 -> n<<12; high16 lane (n<<4) *= 256 -> n<<12 */
    const __m256i shift = _mm256_set1_epi32((256 << 16) + 4096);

    for (i = 0; i < DKE_N / 16; i++) {
        __m128i t = _mm_loadl_epi64((__m128i *)&a[8*i]);
        __m256i f = _mm256_broadcastsi128_si256(t);
        f = _mm256_shuffle_epi8(f, shufbidx);
        f = _mm256_and_si256(f, fmask);
        f = _mm256_mullo_epi16(f, shift);
        f = _mm256_mulhi_epu16(f, q);   /* floor((n<<12)*q / 2^16) = floor(n*q/16) */
        _mm256_store_si256((__m256i *)&r->coeffs[16*i], f);
    }
}

#else /* scalar fallback */

void DKE_getsignal(uint8_t bytes[DKE_SIGNALBYTES], const poly *a) {
    unsigned int i, j;
    int16_t u;
    uint32_t d0;
    uint8_t t[8];
    for (i = 0; i < DKE_N / 8; i++) {
        for (j = 0; j < 8; j++) {
            u = a->coeffs[8*i+j];
            u += (u >> 15) & DKE_Q;
            d0 = u << 4;
            d0 += 1665;
            d0 *= 80635;
            d0 >>= 28;
            t[j] = d0 & 0xf;
        }
        bytes[0] = t[0] | (t[1] << 4);
        bytes[1] = t[2] | (t[3] << 4);
        bytes[2] = t[4] | (t[5] << 4);
        bytes[3] = t[6] | (t[7] << 4);
        bytes += 4;
    }
}

void DKE_poly_fromsignal(poly *r, const uint8_t a[DKE_SIGNALBYTES]) {
    unsigned int i;
    for (i = 0; i < DKE_N / 2; i++) {
        r->coeffs[2*i+0] = ((uint16_t)(a[0] & 15) * DKE_Q) >> 4;
        r->coeffs[2*i+1] = ((uint16_t)(a[0] >> 4) * DKE_Q) >> 4;
        a += 1;
    }
}
#endif /* DKE_USE_AVX2 for DKE-128 */

#elif DKE_MODE == 256 /* L=5, q=3329 */

#if defined(DKE_USE_AVX2)
#include <immintrin.h>

/* AVX2 getsignal for L=5: same as ML-KEM poly_compress(160) */
void DKE_getsignal(uint8_t r[DKE_SIGNALBYTES], const poly *a) {
    unsigned int i;
    const __m256i q = _mm256_set1_epi16((int16_t)DKE_Q);
    const __m256i v = _mm256_set1_epi16(20159);
    const __m256i shift1 = _mm256_set1_epi16(1 << 10);
    const __m256i mask5 = _mm256_set1_epi16(31);
    const __m256i shift2 = _mm256_set1_epi16((32 << 8) + 1);
    const __m256i shift3 = _mm256_set1_epi32((1024 << 16) + 1);
    const __m256i sllvdidx = _mm256_set1_epi64x(12);
    const __m256i shufbidx = _mm256_set_epi8(
         8,-1,-1,-1,-1,-1, 4, 3, 2, 1, 0,-1,12,11,10, 9,
        -1,12,11,10, 9, 8,-1,-1,-1,-1,-1, 4, 3, 2, 1, 0);

    for (i = 0; i < DKE_N / 32; i++) {
        __m256i f0 = _mm256_load_si256((__m256i *)&a->coeffs[32*i]);
        __m256i f1 = _mm256_load_si256((__m256i *)&a->coeffs[32*i+16]);
        f0 = _mm256_add_epi16(f0, _mm256_and_si256(_mm256_srai_epi16(f0, 15), q));
        f1 = _mm256_add_epi16(f1, _mm256_and_si256(_mm256_srai_epi16(f1, 15), q));
        f0 = _mm256_mulhi_epi16(f0, v);
        f1 = _mm256_mulhi_epi16(f1, v);
        f0 = _mm256_mulhrs_epi16(f0, shift1);
        f1 = _mm256_mulhrs_epi16(f1, shift1);
        f0 = _mm256_and_si256(f0, mask5);
        f1 = _mm256_and_si256(f1, mask5);
        f0 = _mm256_packus_epi16(f0, f1);
        f0 = _mm256_maddubs_epi16(f0, shift2);
        f0 = _mm256_madd_epi16(f0, shift3);
        f0 = _mm256_sllv_epi32(f0, sllvdidx);
        f0 = _mm256_srlv_epi64(f0, sllvdidx);
        f0 = _mm256_shuffle_epi8(f0, shufbidx);
        {
            __m128i t0 = _mm256_castsi256_si128(f0);
            __m128i t1 = _mm256_extracti128_si256(f0, 1);
            t0 = _mm_blendv_epi8(t0, t1, _mm256_castsi256_si128(shufbidx));
            _mm_storeu_si128((__m128i *)&r[20*i], t0);
            memcpy(&r[20*i+16], &t1, 4);
        }
    }
}

/* AVX2 fromsignal for L=5 */
void DKE_poly_fromsignal(poly *r, const uint8_t a[DKE_SIGNALBYTES]) {
    unsigned int i;
    int16_t ti;
    const __m256i q = _mm256_set1_epi16((int16_t)DKE_Q);
    const __m256i shufbidx = _mm256_set_epi8(
        9,9,9,8,8,8,8,7,7,6,6,6,6,5,5,5,
        4,4,4,3,3,3,3,2,2,1,1,1,1,0,0,0);
    const __m256i fmask = _mm256_set_epi16(
        248,1984,62,496,3968,124,992,31,
        248,1984,62,496,3968,124,992,31);
    /* Ref truncates: coeff = (v*q) >> 5 = floor(v*q/32). Shift constants are 2x the
     * round-path values so each 5-bit field becomes v<<11, then mulhi_epu16(.,q) =
     * floor((v<<11)*q / 2^16) = floor(v*q/32) exactly (mulhrs would round up). */
    const __m256i shift = _mm256_set_epi16(
        256,32,1024,128,16,512,64,2048,
        256,32,1024,128,16,512,64,2048);

    for (i = 0; i < DKE_N / 16; i++) {
        __m128i t = _mm_loadl_epi64((__m128i *)&a[10*i]);
        memcpy(&ti, &a[10*i+8], 2);
        t = _mm_insert_epi16(t, ti, 4);
        __m256i f = _mm256_broadcastsi128_si256(t);
        f = _mm256_shuffle_epi8(f, shufbidx);
        f = _mm256_and_si256(f, fmask);
        f = _mm256_mullo_epi16(f, shift);
        f = _mm256_mulhi_epu16(f, q);
        _mm256_store_si256((__m256i *)&r->coeffs[16*i], f);
    }
}

#else /* scalar fallback */

void DKE_getsignal(uint8_t r[DKE_SIGNALBYTES], const poly *a) {
    unsigned int i, j;
    int16_t u;
    uint32_t d0;
    uint8_t t[8];
    for (i = 0; i < DKE_N / 8; i++) {
        for (j = 0; j < 8; j++) {
            u = a->coeffs[8*i+j];
            u += (u >> 15) & DKE_Q;
            d0 = u << 5;
            d0 += 1664;
            d0 *= 40318;
            d0 >>= 27;
            t[j] = d0 & 0x1f;
        }
        r[0] = (t[0] >> 0) | (t[1] << 5);
        r[1] = (t[1] >> 3) | (t[2] << 2) | (t[3] << 7);
        r[2] = (t[3] >> 1) | (t[4] << 4);
        r[3] = (t[4] >> 4) | (t[5] << 1) | (t[6] << 6);
        r[4] = (t[6] >> 2) | (t[7] << 3);
        r += 5;
    }
}

void DKE_poly_fromsignal(poly *r, const uint8_t a[DKE_SIGNALBYTES]) {
    unsigned int i, j;
    uint8_t t[8];
    for (i = 0; i < DKE_N / 8; i++) {
        t[0] = (a[0] >> 0);
        t[1] = (a[0] >> 5) | (a[1] << 3);
        t[2] = (a[1] >> 2);
        t[3] = (a[1] >> 7) | (a[2] << 1);
        t[4] = (a[2] >> 4) | (a[3] << 4);
        t[5] = (a[3] >> 1);
        t[6] = (a[3] >> 6) | (a[4] << 2);
        t[7] = (a[4] >> 3);
        a += 5;
        for (j = 0; j < 8; j++) {
            r->coeffs[8*i+j] = ((uint32_t)(t[j] & 31) * DKE_Q) >> 5;
        }
    }
}
#endif /* DKE_USE_AVX2 for DKE-256 */

#elif DKE_MODE == 512 /* L=4, q=7681 */

#if defined(DKE_USE_AVX2)
#include <immintrin.h>

/* AVX2 getsignal for L=4, Q=7681.
 * Compress: round(x * 16 / 7681).
 * v = round(2^26 / 7681) = 8737. */
void DKE_getsignal(uint8_t bytes[DKE_SIGNALBYTES], const poly *pol) {
    unsigned int i;
    const __m256i q = _mm256_set1_epi16((int16_t)DKE_Q);
    const __m256i v = _mm256_set1_epi16(8737);
    const __m256i shift1 = _mm256_set1_epi16(1 << 9);
    const __m256i mask4 = _mm256_set1_epi16(15);
    const __m256i shift2 = _mm256_set1_epi16((16 << 8) + 1);
    const __m256i permdidx = _mm256_set_epi32(7,3,6,2,5,1,4,0);

    for (i = 0; i < DKE_N / 64; i++) {
        __m256i f0 = _mm256_load_si256((__m256i *)&pol->coeffs[64*i]);
        __m256i f1 = _mm256_load_si256((__m256i *)&pol->coeffs[64*i+16]);
        __m256i f2 = _mm256_load_si256((__m256i *)&pol->coeffs[64*i+32]);
        __m256i f3 = _mm256_load_si256((__m256i *)&pol->coeffs[64*i+48]);

        f0 = _mm256_add_epi16(f0, _mm256_and_si256(_mm256_srai_epi16(f0, 15), q));
        f1 = _mm256_add_epi16(f1, _mm256_and_si256(_mm256_srai_epi16(f1, 15), q));
        f2 = _mm256_add_epi16(f2, _mm256_and_si256(_mm256_srai_epi16(f2, 15), q));
        f3 = _mm256_add_epi16(f3, _mm256_and_si256(_mm256_srai_epi16(f3, 15), q));

        f0 = _mm256_mulhi_epi16(f0, v);
        f1 = _mm256_mulhi_epi16(f1, v);
        f2 = _mm256_mulhi_epi16(f2, v);
        f3 = _mm256_mulhi_epi16(f3, v);
        f0 = _mm256_mulhrs_epi16(f0, shift1);
        f1 = _mm256_mulhrs_epi16(f1, shift1);
        f2 = _mm256_mulhrs_epi16(f2, shift1);
        f3 = _mm256_mulhrs_epi16(f3, shift1);
        f0 = _mm256_and_si256(f0, mask4);
        f1 = _mm256_and_si256(f1, mask4);
        f2 = _mm256_and_si256(f2, mask4);
        f3 = _mm256_and_si256(f3, mask4);

        f0 = _mm256_packus_epi16(f0, f1);
        f2 = _mm256_packus_epi16(f2, f3);
        f0 = _mm256_maddubs_epi16(f0, shift2);
        f2 = _mm256_maddubs_epi16(f2, shift2);
        f0 = _mm256_packus_epi16(f0, f2);
        f0 = _mm256_permutevar8x32_epi32(f0, permdidx);
        _mm256_storeu_si256((__m256i *)&bytes[32*i], f0);
    }
}

/* AVX2 fromsignal for L=4, Q=7681 */
void DKE_poly_fromsignal(poly *pol, const uint8_t sig[DKE_SIGNALBYTES]) {
    unsigned int i;
    const __m256i q = _mm256_set1_epi16((int16_t)DKE_Q);
    const __m256i shufbidx = _mm256_set_epi8(
        7,7,7,7,6,6,6,6,5,5,5,5,4,4,4,4,
        3,3,3,3,2,2,2,2,1,1,1,1,0,0,0,0);
    const __m256i fmask = _mm256_set1_epi32(0x00F0000F);
    /* Ref truncates: coeff = (v*q) >> 4. Align nibble to <<12, mulhi_epu16(.,q) =
     * floor((n<<12)*q/2^16) = floor(n*q/16) exactly (mulhrs would round). */
    const __m256i shift = _mm256_set1_epi32((256 << 16) + 4096);

    for (i = 0; i < DKE_N / 16; i++) {
        __m128i t = _mm_loadl_epi64((__m128i *)&sig[8*i]);
        __m256i f = _mm256_broadcastsi128_si256(t);
        f = _mm256_shuffle_epi8(f, shufbidx);
        f = _mm256_and_si256(f, fmask);
        f = _mm256_mullo_epi16(f, shift);
        f = _mm256_mulhi_epu16(f, q);
        _mm256_store_si256((__m256i *)&pol->coeffs[16*i], f);
    }
}

#else /* scalar fallback */

void DKE_getsignal(uint8_t bytes[DKE_SIGNALBYTES], const poly *pol) {
    unsigned int i, j;
    int16_t u;
    uint32_t d0;
    uint8_t t[8];
    for (i = 0; i < DKE_N / 8; i++) {
        for (j = 0; j < 8; j++) {
            u = pol->coeffs[8*i+j];
            u += (u >> 15) & DKE_Q;
            d0 = u << 4;
            d0 += 3840;
            d0 *= 34948;
            d0 >>= 28;
            t[j] = d0 & 0xf;
        }
        bytes[0] = t[0] | (t[1] << 4);
        bytes[1] = t[2] | (t[3] << 4);
        bytes[2] = t[4] | (t[5] << 4);
        bytes[3] = t[6] | (t[7] << 4);
        bytes += 4;
    }
}

void DKE_poly_fromsignal(poly *pol, const uint8_t sig[DKE_SIGNALBYTES]) {
    unsigned int i;
    for (i = 0; i < DKE_N / 2; i++) {
        pol->coeffs[2*i+0] = ((uint16_t)(sig[0] & 15) * DKE_Q) >> 4;
        pol->coeffs[2*i+1] = ((uint16_t)(sig[0] >> 4) * DKE_Q) >> 4;
        sig += 1;
    }
}
#endif /* DKE_USE_AVX2 for DKE-512 */

#endif /* DKE_MODE */

#endif /* !DKE_USE_AARCH64 */
