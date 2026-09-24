/*
 * Hand-written x86-64 AVX2 NTT / polynomial assembly for the DKE KEM.
 * Author: Liu Rui (刘锐), proposal-team member.
 * Original contribution of this submission; WITHIN the scope of the B.3
 * reference-and-optimized-implementation ownership statement.
 */
/*
 * Linux AVX2 assembly NTT/INVNTT declarations.
 * Only used when DKE_USE_AVX2 and DKM_LINUX are both defined.
 */
#ifndef DKE_NTT_AVX2_ASM_H
#define DKE_NTT_AVX2_ASM_H

#if defined(DKE_USE_AVX2) && defined(DKM_LINUX)

#include <stdint.h>

/* N=256 (DKE-128/256): fully unrolled, all large layers + scaling */
unsigned int dke_ntt256_avx2_asm(int16_t *r, const int16_t *zetas);
void dke_invntt256_avx2_asm(int16_t *r, const int16_t *zetas,
                             int16_t f, unsigned int k_start);

/* N=512 (DKE-512): fully unrolled, all large layers + scaling */
unsigned int dke_ntt512_avx2_asm(int16_t *r, const int16_t *zetas);
void dke_invntt512_avx2_asm(int16_t *r, const int16_t *zetas,
                             int16_t f, unsigned int k_start);

/* Generic NTT (old looped version, kept for reference) */
unsigned int dke_ntt_avx2_asm(int16_t *r, const int16_t *zetas, unsigned int n);
void dke_invntt_avx2_asm(int16_t *r, const int16_t *zetas,
                          unsigned int n, unsigned int zetas_len,
                          int16_t f, unsigned int k_start);

#endif
#endif
