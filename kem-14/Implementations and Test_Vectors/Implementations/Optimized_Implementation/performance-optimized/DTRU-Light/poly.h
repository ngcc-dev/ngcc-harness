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
int poly_baseinv_opt(poly *b, const poly *a);

// void poly_sample_keygen(poly *a, const unsigned char *buf);
// void poly_sample_enc(poly *a, const unsigned char *buf);

void poly_naivemul_q2_opt(poly *c, const poly *a, const poly *b, const int Q);
void poly_naivemul_q(poly *c, const poly *a, const poly *b, int16_t q);

// void poly_decode(unsigned char *msg,
//                  const poly *c,
//                  const poly *f);  


void poly_geth(int16_t h1[DTRU_N], int16_t h2[DTRU_N], const int16_t a[DTRU_N], const int16_t b[DTRU_N]);
void poly_crt(int16_t h[DTRU_N], int16_t h1[DTRU_N], int16_t h2[DTRU_N]);
void poly_decrypt(unsigned char *msg,
                 const unsigned char ct[DTRU_PKE_CIPHERTEXTBYTES],
                  const unsigned char sk[DTRU_PKE_SECRETKEYBYTES]);
                  void poly_add(poly *c, const poly *a, const poly *b);


void poly_double_avx(poly *b, const poly *a);
void poly_reduce_avx(poly *a);                                                  // done
void fqcsubq_avx(poly *a);                                                      // done
void poly_freeze_avx(poly *a);                                                  // done
int poly_baseinv_avx(poly *b, const poly *a);                                   // done
void poly_basemul_avx(poly *c, const poly *a, const poly *b);                   // done
void poly_ntt_avx(poly *b);                                                     // done
void poly_invntt_avx(poly *b);                                                  // done
void poly_ntt3329_avx(int16_t *a);
void poly_basemul3329_avx(int16_t* r, const int16_t* a, const int16_t* b);
void poly_invntt3329_avx(int16_t *a);
void poly_ntt7681_avx(int16_t *a);
void poly_basemul7681_avx(int16_t* r, const int16_t* a, const int16_t* b);
void poly_invntt7681_avx(int16_t *a);
void barrett_reduce_avx(poly *b);                                               // done

void poly_add_avx(poly *c, const poly *a, const poly *b);
void poly_double_avx(poly *b, const poly *a);


void poly_freeze_avx2(poly *a);

void poly_decode_avx(unsigned char *msg, const poly *cf);


// new AVX2 implementations
void poly_ntt_avx2(poly *b);
void poly_invntt_avx2(poly *b);    
void poly_basemul_avx2(poly *c, const poly *a, const poly *b); 
int poly_baseinv_avx2(poly *b, const poly *a);

#endif
