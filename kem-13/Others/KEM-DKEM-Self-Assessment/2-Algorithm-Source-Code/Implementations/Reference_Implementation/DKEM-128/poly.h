#ifndef DKE_POLY_H
#define DKE_POLY_H

#include "parameters.h"
#include <stdint.h>

#if defined(DKE_USE_AVX2)
#include <immintrin.h>
/*
 * 32-byte aligned poly for AVX2: allows vmovdqa (aligned load/store).
 * +8 int16 padding: rej_uniform's _mm_storeu_si128 may write up to 7
 * extra values past DKE_N (ML-KEM AVX2 known issue, pq-crystals/mlkem#21).
 */
typedef union {
    ALIGN(256) int16_t coeffs[DKE_N + 8];
    ALIGN(256) __m256i vec[((DKE_N + 8) * sizeof(int16_t) + 31) / 32];
} poly;
#elif defined(DKE_USE_AARCH64)
/*
 * 16-byte aligned poly for NEON: allows vld1q/vst1q aligned access.
 */
typedef struct {
    ALIGN(16) int16_t coeffs[DKE_N];
} poly;
#else
typedef struct {
    int16_t coeffs[DKE_N];
} poly;
#endif

void DKE_poly_reduce(poly *pol);
/* Centered Barrett reduction: maps coefficients to {-(q-1)/2, ..., (q-1)/2}.
 * Required by the signal/shared-secret derivation (apply_signal). Portable C,
 * valid for every backend (operates on ->coeffs). */
void DKE_poly_reduce_center(poly *pol);
void DKE_poly_add(poly *res, const poly *a, const poly *b);
void DKE_poly_sub(poly *res, const poly *a, const poly *b);
void DKE_poly_scale2(poly *pol);

void DKE_poly_ntt(poly *pol);
void DKE_poly_invntt_tomont(poly *pol);
void DKE_poly_basemul_montgomery(poly *res, const poly *a, const poly *b);
void DKE_poly_tomont(poly *pol);
/* Permute a freshly-sampled (canonical-order) NTT-domain poly into the ntt(packed)
 * order that the vendored ML-KEM asm basemul expects. No-op unless DKE_AVX2_NTT256_ASM. */
void DKE_poly_nttunpack(poly *pol);
/* Inverse permutation: AVX2-internal NTT layout -> standard order (512 packed). No-op otherwise. */
void DKE_poly_nttpack(poly *pol);

void DKE_poly_tobytes(uint8_t bytes[DKE_POLYBYTES], const poly *pol);
void DKE_poly_frombytes(poly *pol, const uint8_t bytes[DKE_POLYBYTES]);

void DKE_getsignal(uint8_t sig[DKE_SIGNALBYTES], const poly *pol);
void DKE_poly_fromsignal(poly *pol, const uint8_t sig[DKE_SIGNALBYTES]);

#endif
