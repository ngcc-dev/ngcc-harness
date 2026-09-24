#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "params.h"
#include "fips202.h"
#include "align.h"

typedef struct {
  ALIGN(32) int32_t coeffs[PARAM_N];
} poly;

#if PARAMS == 1
typedef uint16_t s2Word;
#else
typedef uint8_t s2Word;
#endif

void poly_reduce(poly *a);
void poly_g_reduce(poly *a);
void poly_amodq(poly *a);
void poly_cmodq(poly *a);
void poly_decompose(poly *r1,poly *r0, const poly *a);
void poly_power2round(poly *r1,poly *r0, const poly *a);
int32_t poly_make_hint(poly *h, const poly *a, const poly *b);

void poly_add(poly *c, const poly *a, const poly *b);
void poly_sub(poly *c, const poly *a, const poly *b);
void poly_subw(poly *c, const poly *a, const poly *w);
void poly_shiftl(poly *a, int k);

void poly_ntt(poly *a);
void poly_invntt_montgomery(poly *a);
void poly_pointwise_montgomery(poly *c, const poly *a, const poly *b);

int  poly_chknorm(const poly *a, uint32_t B);
void poly_uniform_seed(poly *a, const uint8_t *seed,int32_t seedbytes);
int poly_uniform(poly *a, const uint8_t *buf, int32_t buflen);
void poly_uniform_eta1_seed(poly *a, const uint8_t *seed, int32_t seedbytes);
int poly_uniform_eta1(poly *a, const uint8_t *buf, int32_t buflen);

void poly_uniform_eta2_seed(poly *a, const uint8_t *seed, int32_t seedbytes);
int poly_uniform_eta2(poly *a, const uint8_t *buf, int32_t buflen);

void poly_uniform_gamma1(poly *a,const uint8_t seed[SEEDBYTES + CRHBYTES],uint16_t nonce);

void polyeta1_pack(uint8_t *r, const poly *a);
void polyeta1_unpack(poly *r, const uint8_t *a);
void polyeta2_pack(uint8_t *r, const poly *a);
void polyeta2_unpack(poly *r, const uint8_t *a);

void polyt1_pack(uint8_t *r, const poly *a);
void polyt1_unpack(poly *r, const uint8_t *a);

void polyt0_pack(uint8_t *r, const poly *a);
void polyt0_unpack(poly *r, const uint8_t *a);

void polyz_pack(uint8_t *r, const poly *a);
void polyz_unpack(poly *r, const uint8_t *a);

void polyw1_pack(uint8_t *r, const poly *a);

void poly_g_reduce_avx(poly *a);
#endif
