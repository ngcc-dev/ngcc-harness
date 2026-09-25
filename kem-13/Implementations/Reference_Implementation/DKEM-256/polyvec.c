#include "parameters.h"
#include "poly.h"
#include "polyvec.h"
#include <stdint.h>

#if defined(DKE_USE_AARCH64_NATIVE) && DKE_N == 256 && DKE_Q == 3329
#include "aarch64-native/dke_native.h"
#endif

void DKE_polyvec_reduce(polyvec *v) {
    unsigned int i;
    for (i = 0; i < DKE_K; i++) DKE_poly_reduce(&v->vec[i]);
}

void DKE_polyvec_add(polyvec *res, const polyvec *a, const polyvec *b) {
    unsigned int i;
    for (i = 0; i < DKE_K; i++) DKE_poly_add(&res->vec[i], &a->vec[i], &b->vec[i]);
}

void DKE_polyvec_sub(polyvec *res, const polyvec *a, const polyvec *b) {
    unsigned int i;
    for (i = 0; i < DKE_K; i++) DKE_poly_sub(&res->vec[i], &a->vec[i], &b->vec[i]);
}

void DKE_polyvec_scale2(polyvec *v) {
    DKE_polyvec_add(v, v, v);
}

void DKE_polyvec_ntt(polyvec *v) {
    unsigned int i;
    for (i = 0; i < DKE_K; i++) DKE_poly_ntt(&v->vec[i]);
}

void DKE_polyvec_invntt_tomont(polyvec *v) {
    unsigned int i;
    for (i = 0; i < DKE_K; i++) DKE_poly_invntt_tomont(&v->vec[i]);
}

#if defined(DKE_USE_AARCH64_NATIVE) && DKE_N == 256 && DKE_Q == 3329
/* Native AArch64 basemul with mulcache — computes cache internally,
 * transparent to callers. Uses mlkem-native assembly for K=2/4. */
void DKE_polyvec_basemul_acc_montgomery(poly *res, const polyvec *a, const polyvec *b) {
    unsigned int i;
    /* Compute mulcache for b: cache[i] = b[2i+1] * zeta (Montgomery domain) */
    int16_t b_cache[DKE_K * (DKE_N / 2)];
    for (i = 0; i < DKE_K; i++)
        dke_poly_mulcache_compute_native(
            b_cache + i * (DKE_N / 2),
            b->vec[i].coeffs,
            dke_aarch64_zetas_mulcache_native,
            dke_aarch64_zetas_mulcache_twisted_native);

    /* Native basemul+acc: r = sum(a[i] * b[i]) with cached b */
#if DKE_K == 2
    dke_polyvec_basemul_acc_cached_k2(
        res->coeffs, a->vec[0].coeffs, b->vec[0].coeffs, b_cache);
#elif DKE_K == 4
    dke_polyvec_basemul_acc_cached_k4(
        res->coeffs, a->vec[0].coeffs, b->vec[0].coeffs, b_cache);
#else
    /* Fallback for unsupported K */
    {
        poly temp;
        DKE_poly_basemul_montgomery(res, &a->vec[0], &b->vec[0]);
        for (i = 1; i < DKE_K; i++) {
            DKE_poly_basemul_montgomery(&temp, &a->vec[i], &b->vec[i]);
            DKE_poly_add(res, res, &temp);
        }
    }
#endif
    DKE_poly_reduce(res);
}
#elif defined(DKE_USE_AARCH64) && DKE_N == 512 && DKE_K == 4
/* Fused NEON basemul+acc for DKE-512: accumulates K=4 basemul results
 * in registers without intermediate store/load round-trips. */
extern void DKE_polyvec_basemul_acc_montgomery_neon(poly *res,
                                                     const polyvec *a, const polyvec *b);
void DKE_polyvec_basemul_acc_montgomery(poly *res, const polyvec *a, const polyvec *b) {
    DKE_polyvec_basemul_acc_montgomery_neon(res, a, b);
    DKE_poly_reduce(res);
}
#elif defined(DKE_USE_CORTEX_M4_PLANTARD)
/* Fused M4 basemul+acc (Montgomery, Plantard, or Barrett). */
extern void DKE_polyvec_basemul_acc_montgomery_m4(poly *res,
                                                    const polyvec *a, const polyvec *b);
void DKE_polyvec_basemul_acc_montgomery(poly *res, const polyvec *a, const polyvec *b) {
    DKE_polyvec_basemul_acc_montgomery_m4(res, a, b);
    DKE_poly_reduce(res);
}
#else
void DKE_polyvec_basemul_acc_montgomery(poly *res, const polyvec *a, const polyvec *b) {
    unsigned int i;
    poly temp;
    DKE_poly_basemul_montgomery(res, &a->vec[0], &b->vec[0]);
    for (i = 1; i < DKE_K; i++) {
        DKE_poly_basemul_montgomery(&temp, &a->vec[i], &b->vec[i]);
        DKE_poly_add(res, res, &temp);
    }
    DKE_poly_reduce(res);
}
#endif

void DKE_polyvec_tobytes(uint8_t bytes[DKE_POLYVECBYTES], const polyvec *v) {
    unsigned int i;
    for (i = 0; i < DKE_K; i++) DKE_poly_tobytes(bytes + i * DKE_POLYBYTES, &v->vec[i]);
}

void DKE_polyvec_frombytes(polyvec *v, const uint8_t bytes[DKE_POLYVECBYTES]) {
    unsigned int i;
    for (i = 0; i < DKE_K; i++) DKE_poly_frombytes(&v->vec[i], bytes + i * DKE_POLYBYTES);
}

// Polyvec compress/decompress for ciphertext (dB bits per coefficient)

#if defined(DKE_USE_AARCH64)
  /* compressB, decompressB supplied by aarch64/polyvec_neon.c */
#else

/* compress/decompress: fully vectorized when AVX2 — EXACT-Barrett compress arithmetic
 * (`dke_compress_arith16`, reproducing scalar `(t<<D+HALF)*MUL>>S & mask` bit-for-bit,
 * brute-force verified over all int16 for q=3329/7681) followed by ML-KEM's vectorized
 * bit-pack; decompress is ML-KEM's vectorized unpack+`round(c*q/2^D)` (== scalar
 * `(c*q+2^(D-1))>>D`) with q=DKE_Q. Scalar fallback otherwise. KAT unchanged.
 * Packers may over-write/-read a few bytes past the 20/22-byte group, so callers
 * (un)pack a poly through a padded stack buffer. */
#if defined(DKE_USE_AVX2)
#include <immintrin.h>
#include <string.h>

/* 16 signed coeffs -> __m256i of 16 D-bit compressed values (exact Barrett, masked,
 * natural lane order). */
static inline __m256i dke_compress_arith16(const int16_t *coeffs,
                                           int D, uint32_t HALF, uint32_t MUL, int S) {
    const __m256i q    = _mm256_set1_epi16((int16_t)DKE_Q);
    const __m256i MULv = _mm256_set1_epi32((int)MUL);
    const __m256i HALFv= _mm256_set1_epi32((int)HALF);
    __m256i t = _mm256_loadu_si256((const __m256i *)coeffs);
    t = _mm256_add_epi16(t, _mm256_and_si256(_mm256_srai_epi16(t, 15), q)); /* [0,q) */
    __m256i out[2];
    for (int h = 0; h < 2; h++) {
        __m128i half = h ? _mm256_extracti128_si256(t, 1) : _mm256_castsi256_si128(t);
        __m256i x = _mm256_cvtepu16_epi32(half);
        x = _mm256_add_epi32(_mm256_slli_epi32(x, D), HALFv);   /* (c<<D)+HALF */
        __m256i pe = _mm256_mul_epu32(x, MULv);                 /* even 32-bit lanes */
        __m256i po = _mm256_mul_epu32(_mm256_srli_epi64(x, 32), MULv); /* odd lanes */
        pe = _mm256_srli_epi64(pe, S);
        po = _mm256_srli_epi64(po, S);
        out[h] = _mm256_or_si256(pe, _mm256_slli_epi64(po, 32)); /* 8×u32 results */
    }
    __m256i f = _mm256_packus_epi32(out[0], out[1]);
    f = _mm256_permute4x64_epi64(f, 0xD8);                       /* 16×u16 natural */
    f = _mm256_and_si256(f, _mm256_set1_epi16((1 << D) - 1));    /* mask to D bits */
    return f;
}

/* ML-KEM-style packers: 16 masked D-bit values -> bit-packed bytes.
 * pack10 writes 20 valid bytes (touches r[0..19]); pack11 writes 22 (touches r[0..23]). */
static inline void dke_pack10(uint8_t *r, __m256i f0) {
    const __m256i shift2 = _mm256_set1_epi64x((1024LL<<48)+(1LL<<32)+(1024<<16)+1);
    const __m256i sllvdidx = _mm256_set1_epi64x(12);
    const __m256i shufbidx = _mm256_set_epi8( 8,4,3,2,1,0,-1,-1,-1,-1,-1,-1,12,11,10,9,
                                             -1,-1,-1,-1,-1,-1,12,11,10,9,8,4,3,2,1,0);
    f0 = _mm256_madd_epi16(f0, shift2);
    f0 = _mm256_sllv_epi32(f0, sllvdidx);
    f0 = _mm256_srli_epi64(f0, 12);
    f0 = _mm256_shuffle_epi8(f0, shufbidx);
    __m128i t0 = _mm256_castsi256_si128(f0);
    __m128i t1 = _mm256_extracti128_si256(f0, 1);
    t0 = _mm_blend_epi16(t0, t1, 0xE0);
    _mm_storeu_si128((__m128i *)r, t0);
    memcpy(r + 16, &t1, 4);
}
static inline void dke_pack11(uint8_t *r, __m256i f0) {
    const __m256i shift2 = _mm256_set1_epi64x((2048LL<<48)+(1LL<<32)+(2048<<16)+1);
    const __m256i sllvdidx = _mm256_set1_epi64x(10);
    const __m256i srlvqidx = _mm256_set_epi64x(30,10,30,10);
    const __m256i shufbidx = _mm256_set_epi8( 4,3,2,1,0,0,-1,-1,-1,-1,10,9,8,7,6,5,
                                             -1,-1,-1,-1,-1,10,9,8,7,6,5,4,3,2,1,0);
    f0 = _mm256_madd_epi16(f0, shift2);
    f0 = _mm256_sllv_epi32(f0, sllvdidx);
    __m256i f1 = _mm256_bsrli_epi128(f0, 8);
    f0 = _mm256_srlv_epi64(f0, srlvqidx);
    f1 = _mm256_slli_epi64(f1, 34);
    f0 = _mm256_add_epi64(f0, f1);
    f0 = _mm256_shuffle_epi8(f0, shufbidx);
    __m128i t0 = _mm256_castsi256_si128(f0);
    __m128i t1 = _mm256_extracti128_si256(f0, 1);
    t0 = _mm_blendv_epi8(t0, t1, _mm256_castsi256_si128(shufbidx));
    _mm_storeu_si128((__m128i *)r, t0);
    _mm_storel_epi64((__m128i *)(r + 16), t1);
}

/* ML-KEM-style unpack+decompress: read bit-packed bytes -> 16 coeffs (round(c*q/2^D)).
 * Each reads 32 bytes from a (the trailing pad is unused). */
static inline __m256i dke_unpack10(const uint8_t *a) {
    const __m256i q = _mm256_set1_epi32((DKE_Q << 16) + 4 * DKE_Q);
    const __m256i shufbidx = _mm256_set_epi8(11,10,10,9,9,8,8,7, 6,5,5,4,4,3,3,2,
                                              9,8,8,7,7,6,6,5, 4,3,3,2,2,1,1,0);
    const __m256i sllvdidx = _mm256_set1_epi64x(4);
    const __m256i mask = _mm256_set1_epi32((32736 << 16) + 8184);
    __m256i f = _mm256_loadu_si256((const __m256i *)a);
    f = _mm256_permute4x64_epi64(f, 0x94);
    f = _mm256_shuffle_epi8(f, shufbidx);
    f = _mm256_sllv_epi32(f, sllvdidx);
    f = _mm256_srli_epi16(f, 1);
    f = _mm256_and_si256(f, mask);
    return _mm256_mulhrs_epi16(f, q);
}
static inline __m256i dke_unpack11(const uint8_t *a) {
    const __m256i q = _mm256_set1_epi16((int16_t)DKE_Q);
    const __m256i shufbidx = _mm256_set_epi8(13,12,12,11,10,9,9,8, 8,7,6,5,5,4,4,3,
                                             10,9,9,8,7,6,6,5, 5,4,3,2,2,1,1,0);
    const __m256i srlvdidx = _mm256_set_epi32(0,0,1,0,0,0,1,0);
    const __m256i srlvqidx = _mm256_set_epi64x(2,0,2,0);
    const __m256i shift = _mm256_set_epi16(4,32,1,8,32,1,4,32,4,32,1,8,32,1,4,32);
    const __m256i mask = _mm256_set1_epi16(32752);
    __m256i f = _mm256_loadu_si256((const __m256i *)a);
    f = _mm256_permute4x64_epi64(f, 0x94);
    f = _mm256_shuffle_epi8(f, shufbidx);
    f = _mm256_srlv_epi32(f, srlvdidx);
    f = _mm256_srlv_epi64(f, srlvqidx);
    f = _mm256_mullo_epi16(f, shift);
    f = _mm256_srli_epi16(f, 1);
    f = _mm256_and_si256(f, mask);
    return _mm256_mulhrs_epi16(f, q);
}
#endif /* DKE_USE_AVX2 helpers */

#if DKE_MODE == 128 /* dB=10: 16 coeffs -> 20 bytes */
#define DKE_PB_PERPOLY (DKE_N / 16 * 20)   /* = DKE_N*10/8 */

void DKE_polyvec_compressB(uint8_t bytes[DKE_PBCOMPRESSEDBYTES], const polyvec *v) {
    unsigned int i, j;
#if defined(DKE_USE_AVX2)
    /* d=10: pack10 writes exactly 20 bytes/group (no overrun), so we can pack
     * straight into the output — no padded temp / memcpy needed. */
    for (i = 0; i < DKE_K; i++) {
        for (j = 0; j < DKE_N / 16; j++)
            dke_pack10(&bytes[20*j],
                       dke_compress_arith16(&v->vec[i].coeffs[16*j], 10, 1665, 1290167u, 32));
        bytes += DKE_PB_PERPOLY;
    }
#else
    uint16_t t[DKE_N];
    for (i = 0; i < DKE_K; i++) {
        for (j = 0; j < DKE_N; j++) {
            uint64_t d0; uint16_t x = v->vec[i].coeffs[j];
            x += ((int16_t)x >> 15) & DKE_Q;
            d0 = x; d0 <<= 10; d0 += 1665; d0 *= 1290167; d0 >>= 32;
            t[j] = d0 & 0x3ff;
        }
        for (j = 0; j < DKE_N / 4; j++) {
            bytes[0] = (uint8_t)(t[4*j+0] >> 0);
            bytes[1] = (uint8_t)((t[4*j+0] >> 8) | (t[4*j+1] << 2));
            bytes[2] = (uint8_t)((t[4*j+1] >> 6) | (t[4*j+2] << 4));
            bytes[3] = (uint8_t)((t[4*j+2] >> 4) | (t[4*j+3] << 6));
            bytes[4] = (uint8_t)(t[4*j+3] >> 2);
            bytes += 5;
        }
    }
#endif
}

void DKE_polyvec_decompressB(polyvec *v, const uint8_t bytes[DKE_PBCOMPRESSEDBYTES]) {
    unsigned int i, j;
#if defined(DKE_USE_AVX2)
    uint8_t tmp[DKE_PB_PERPOLY + 16];
    for (i = 0; i < DKE_K; i++) {
        memcpy(tmp, bytes, DKE_PB_PERPOLY);
        for (j = 0; j < DKE_N / 16; j++)
            _mm256_storeu_si256((__m256i *)&v->vec[i].coeffs[16*j], dke_unpack10(&tmp[20*j]));
        bytes += DKE_PB_PERPOLY;
    }
#else
    uint16_t t[DKE_N];
    for (i = 0; i < DKE_K; i++) {
        for (j = 0; j < DKE_N / 4; j++) {
            t[4*j+0] = (bytes[0] >> 0) | ((uint16_t)bytes[1] << 8);
            t[4*j+1] = (bytes[1] >> 2) | ((uint16_t)bytes[2] << 6);
            t[4*j+2] = (bytes[2] >> 4) | ((uint16_t)bytes[3] << 4);
            t[4*j+3] = (bytes[3] >> 6) | ((uint16_t)bytes[4] << 2);
            bytes += 5;
        }
        for (j = 0; j < DKE_N; j++)
            v->vec[i].coeffs[j] = ((uint32_t)(t[j] & 0x3FF) * DKE_Q + 512) >> 10;
    }
#endif
}

#else /* DKE_MODE == 256 or 512: dB=11, 16 coeffs -> 22 bytes */
#define DKE_PB_PERPOLY (DKE_N / 16 * 22)   /* = DKE_N*11/8 */

void DKE_polyvec_compressB(uint8_t bytes[DKE_PBCOMPRESSEDBYTES], const polyvec *v) {
    unsigned int i, j;
#if DKE_MODE == 256
    const uint32_t HALF = 1664, MUL = 645084u;
#else
    const uint32_t HALF = 3840, MUL = 279584u;
#endif
#if defined(DKE_USE_AVX2)
    uint8_t tmp[DKE_PB_PERPOLY + 16];
    for (i = 0; i < DKE_K; i++) {
        for (j = 0; j < DKE_N / 16; j++)
            dke_pack11(&tmp[22*j],
                       dke_compress_arith16(&v->vec[i].coeffs[16*j], 11, HALF, MUL, 31));
        memcpy(bytes, tmp, DKE_PB_PERPOLY);
        bytes += DKE_PB_PERPOLY;
    }
#else
    uint16_t t[DKE_N];
    for (i = 0; i < DKE_K; i++) {
        for (j = 0; j < DKE_N; j++) {
            uint64_t d0; uint16_t x = v->vec[i].coeffs[j];
            x += ((int16_t)x >> 15) & DKE_Q;
            d0 = x; d0 <<= 11; d0 += HALF; d0 *= MUL; d0 >>= 31;
            t[j] = d0 & 0x7ff;
        }
        for (j = 0; j < DKE_N / 8; j++) {
            uint16_t *tp = &t[8*j];
            bytes[ 0] = (uint8_t)(tp[0] >>  0);
            bytes[ 1] = (uint8_t)((tp[0] >>  8) | (tp[1] << 3));
            bytes[ 2] = (uint8_t)((tp[1] >>  5) | (tp[2] << 6));
            bytes[ 3] = (uint8_t)(tp[2] >>  2);
            bytes[ 4] = (uint8_t)((tp[2] >> 10) | (tp[3] << 1));
            bytes[ 5] = (uint8_t)((tp[3] >>  7) | (tp[4] << 4));
            bytes[ 6] = (uint8_t)((tp[4] >>  4) | (tp[5] << 7));
            bytes[ 7] = (uint8_t)(tp[5] >>  1);
            bytes[ 8] = (uint8_t)((tp[5] >>  9) | (tp[6] << 2));
            bytes[ 9] = (uint8_t)((tp[6] >>  6) | (tp[7] << 5));
            bytes[10] = (uint8_t)(tp[7] >>  3);
            bytes += 11;
        }
    }
#endif
}

void DKE_polyvec_decompressB(polyvec *v, const uint8_t bytes[DKE_PBCOMPRESSEDBYTES]) {
    unsigned int i, j;
#if defined(DKE_USE_AVX2)
    uint8_t tmp[DKE_PB_PERPOLY + 16];
    for (i = 0; i < DKE_K; i++) {
        memcpy(tmp, bytes, DKE_PB_PERPOLY);
        for (j = 0; j < DKE_N / 16; j++)
            _mm256_storeu_si256((__m256i *)&v->vec[i].coeffs[16*j], dke_unpack11(&tmp[22*j]));
        bytes += DKE_PB_PERPOLY;
    }
#else
    uint16_t t[DKE_N];
    for (i = 0; i < DKE_K; i++) {
        for (j = 0; j < DKE_N / 8; j++) {
            uint16_t *tp = &t[8*j];
            tp[0] = (bytes[0] >> 0) | ((uint16_t)bytes[ 1] << 8);
            tp[1] = (bytes[1] >> 3) | ((uint16_t)bytes[ 2] << 5);
            tp[2] = (bytes[2] >> 6) | ((uint16_t)bytes[ 3] << 2) | ((uint16_t)bytes[4] << 10);
            tp[3] = (bytes[4] >> 1) | ((uint16_t)bytes[ 5] << 7);
            tp[4] = (bytes[5] >> 4) | ((uint16_t)bytes[ 6] << 4);
            tp[5] = (bytes[6] >> 7) | ((uint16_t)bytes[ 7] << 1) | ((uint16_t)bytes[8] << 9);
            tp[6] = (bytes[8] >> 2) | ((uint16_t)bytes[ 9] << 6);
            tp[7] = (bytes[9] >> 5) | ((uint16_t)bytes[10] << 3);
            bytes += 11;
        }
        for (j = 0; j < DKE_N; j++)
            v->vec[i].coeffs[j] = ((uint32_t)(t[j] & 0x7FF) * DKE_Q + 1024) >> 11;
    }
#endif
}

#endif /* DKE_MODE */
#undef DKE_PB_PERPOLY

#endif /* !DKE_USE_AARCH64 */
