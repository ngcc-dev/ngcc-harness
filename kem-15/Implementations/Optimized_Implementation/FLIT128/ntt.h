#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "params.h"

#define zetas KEM_NAMESPACE(zetas)
extern const int16_t zetas[128];
#define zetas_inv KEM_NAMESPACE(zetas_inv)
extern const int16_t zetas_inv[128];

#define fqinv KEM_NAMESPACE(fqinv)
int16_t fqinv(int16_t a);
extern const int16_t fqinv_table[769];

#define ntt KEM_NAMESPACE(ntt)
void ntt(int16_t poly[N]);
#define invntt_tomont KEM_NAMESPACE(invntt_tomont)
void invntt_tomont(int16_t poly[N]);

/* Recursive Karatsuba inversion (accessible for mode 512 AVX2) */
int baseinv_K(int half, int16_t *b, const int16_t *a, int16_t zeta);

/* AVX2 K(4) multiply: 8-element arrays, returns YMM */
#if KEM_MODE == 512
#include <immintrin.h>
__m256i k4_ymm(const int16_t a[8], const int16_t b[8],
               int16_t zeta, __m256i q, __m256i qinv, __m256i v);
#endif

#endif
