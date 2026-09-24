#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include <immintrin.h>

#define ROOT_DIMENSION 9

extern int16_t zetas[72];
extern int16_t zetas_inv[74];
extern int16_t zetas_base[96];

void ntt_forward_81x8_v2(__m256i vec81[81], const int16_t src[648]);
void basemul_81x8(__m256i c[81], const __m256i a[81], const __m256i b[81]);
/* SIMD-domain base inversion (keygen helper): inverts 8 blocks in parallel.
 * Returns an 8-bit failure mask (bit i set => lane i failed). */
int baseinv_81x8(__m256i b[81], const __m256i a[81]);
void invntt_stage_len9_vec81_avx2(__m256i vec81[81]);
void invntt_stage_len27_vec81_avx2(__m256i vec81[81]);
void invntt_full_81x8(int16_t dst[648], const __m256i src[81]);
void ntt(int16_t *a);
void invntt(int16_t *a);
void basemul(int16_t *c, const int16_t *a, const int16_t *b, const int16_t zeta);

#endif