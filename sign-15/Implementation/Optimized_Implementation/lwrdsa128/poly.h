#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include <immintrin.h>
#include "params.h"

typedef union {
  uint32_t coeffs[N];
  __m256i vec[(N+7)/8];
} poly __attribute__((aligned(32)));

void pol_mul(poly *c, const poly *a, const poly *b);

void poly_copy(poly *b, const poly *a);
void poly_freeze_avx2(poly *a, int mod);

void poly_add(poly *c, const poly *a, const poly *b);
void poly_sub(poly *c, const poly *a, const poly *b);
void poly_neg(poly *a);
void poly_shiftl(poly *a, unsigned int k);

void schoolbook_128_wrap(poly * restrict       c,
                       const poly * restrict a,
                       const poly * restrict b);
void toom4_32(const uint32_t *a, const uint32_t *b, uint32_t *res);
void toom_cook_4way (const uint32_t *a1, const uint32_t *b1, uint32_t *result);

int  poly_chknorm(const poly *a, uint32_t B, int mod);
void poly_uniform(poly *a, unsigned char *buf);
void poly_uniform_eta(poly *a,
                      const unsigned char seed[SEEDBYTES],
                      unsigned char nonce);
void poly_uniform_gamma1m1(poly *a,
                           const unsigned char seed[SEEDBYTES + CRHBYTES],
                           uint16_t nonce);

void polyeta_pack(unsigned char *r, const poly *a);
void polyeta_unpack(poly *r, const unsigned char *a);

void polyt1_pack(unsigned char *r, const poly *a);
void polyt1_unpack(poly *r, const unsigned char *a);

void polyt0_pack(unsigned char *r, const poly *a);
void polyt0_unpack(poly *r, const unsigned char *a);

void polyz_pack(unsigned char *r, const poly *a);
void polyz_unpack(poly *r, const unsigned char *a);

void polyw1_pack(unsigned char *r, const poly *a);
#endif
