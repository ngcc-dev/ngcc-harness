#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "params.h"

typedef struct
{
  int16_t coeffs[DTRU_N];
} poly;

void poly_reduce(poly *a);
void poly_reduce_avx2(poly *a);

void poly_freeze(poly *a);
void poly_freeze_avx2(poly *a);

void poly_add(poly *c, const poly *a, const poly *b);
void poly_add_avx2(poly *c, const poly *a, const poly *b);

void poly_multi_p(poly *b, const poly *a);
void poly_multi_p_avx2(poly *b, const poly *a);

void poly_sample_keygen_f(poly *a, const unsigned char *buf);
void poly_sample_keygen_g(poly *a, const unsigned char *buf);
void poly_sample_enc_r(poly *a, const unsigned char *buf);
void poly_sample_enc_e(poly *a, const unsigned char *buf);

void poly_sample_keygen_f_avx2(poly *a, const unsigned char *buf);
void poly_sample_keygen_g_avx2(poly *a, const unsigned char *buf);
void poly_sample_enc_r_avx2(poly *a, const unsigned char *buf);
void poly_sample_enc_e_avx2(poly *a, const unsigned char *buf);

void poly_ntt(poly *b);
void poly_invntt(poly *b);    
void poly_basemul(poly *c, const poly *a, const poly *b); 
int poly_baseinv(poly *b, const poly *a);

void poly_ntt_avx2(poly *b);
void poly_invntt_avx2(poly *b);    
void poly_basemul_avx2(poly *c, const poly *a, const poly *b); 
int poly_baseinv_avx2(poly *b, const poly *a);

void poly_encode_compress(poly *c,
                          const poly *sigma,
                          const unsigned char *msg);
void poly_decode(unsigned char *msg,
                 const poly *cf);  
#endif
