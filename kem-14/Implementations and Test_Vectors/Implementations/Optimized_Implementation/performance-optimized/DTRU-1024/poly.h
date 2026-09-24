#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "params.h"

typedef struct
{
  __attribute__((aligned(32))) int16_t coeffs[DTRU_N];
} poly;

void poly_reduce(poly *a);
void poly_freeze(poly *a);
void poly_add(poly *c, const poly *a, const poly *b);
void poly_multi_p(poly *b, const poly *a);

void poly_sample_keygen_f(poly *a, const unsigned char *buf);
void poly_sample_keygen_g(poly *a, const unsigned char *buf);
void poly_sample_enc_r(poly *a, const unsigned char *buf);
void poly_sample_enc_e(poly *a, const unsigned char *buf);

void poly_ntt(poly *b);
void poly_invntt(poly *b);    
void poly_basemul(poly *c, const poly *a, const poly *b); 
int poly_baseinv(poly *b, const poly *a);

void poly_encode_compress(poly *c,
                          const poly *sigma,
                          const unsigned char *msg);
void poly_decode(unsigned char *msg,
                 const poly *cf);  

// AVX2 implementations
void poly_add_avx(poly *c, const poly *a, const poly *b);
void poly_double_avx(poly *b, const poly *a);
void poly_tomont_avx(poly *a);
void poly_freeze_avx(poly *a);
void poly_barret_avx(poly *a);
void poly_sample_avx(poly *a, const unsigned char *buf);
void poly_ntt_avx(poly *a);
void poly_invntt_avx(poly *a);
void poly_basemul_avx(poly *c, const poly *a, const poly *b);
void poly_decode_avx(unsigned char *m, const poly *c, const poly *fhat);
int  poly_baseinv_bernstein_avx(poly *b, const poly *a);
int  poly_baseinv_avx(poly *b, const poly *a);
void poly_basemul_avx(poly *c, const poly *a, const poly *b);

void poly_split(poly *b, const poly *a);
void poly_combine(poly *b, const poly *a);
void poly_encode_compress(poly *c, const poly *sigma, const unsigned char *m);

#endif
