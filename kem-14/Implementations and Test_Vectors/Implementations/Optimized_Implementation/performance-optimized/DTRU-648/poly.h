#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include <immintrin.h>
#include "params.h"

typedef struct
{
  int16_t coeffs[DTRU_N];
} poly;

/* SIMD-domain polynomial: 81 AVX2 vectors, each holding 8 int32 lanes.
 * Produced by poly_ntt_simd(); consumed by poly_basemul_simd() and
 * poly_invntt_simd(). Not interchangeable with the scalar NTT domain. */
typedef struct
{
  __m256i vec[81];
} poly_simd;

void poly_reduce(poly *a);
void poly_freeze(poly *a);
void poly_add(poly *c, const poly *a, const poly *b);
void poly_multi_p(poly *b, const poly *a);

void poly_sample_keygen_f(poly *a, const unsigned char *buf);
void poly_sample_keygen_g(poly *a, const unsigned char *buf);
void poly_sample_enc_r(poly *a, const unsigned char *buf);
void poly_sample_enc_e(poly *a, const unsigned char *buf);

/* Scalar NTT path (used where poly_baseinv is needed, e.g. keygen). */
void poly_ntt(poly *b);
void poly_invntt(poly *b);
void poly_basemul(poly *c, const poly *a, const poly *b);
int poly_baseinv(poly *b, const poly *a);

/* SIMD NTT path: forward → basemul/baseinv → inverse. */
void poly_ntt_simd_v2(poly_simd *out, const poly *in);
void poly_basemul_simd(poly_simd *c, const poly_simd *a, const poly_simd *b);
int  poly_baseinv_simd(poly_simd *b, const poly_simd *a);
void poly_invntt_simd(poly *out, const poly_simd *in);

void poly_encode_compress(poly *c,
                          const poly *sigma,
                          const unsigned char *msg);
void poly_decode(unsigned char *msg,
                 const poly *cf);

void poly_freeze_avx2(poly *a);
void poly_add_avx2(poly *c, const poly *a, const poly *b);
void poly_multi_p_avx2(poly *b, const poly *a);
#endif
