#ifndef POLYVEC_H
#define POLYVEC_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

/* Vectors of polynomials of length L */
typedef struct {
  poly vec[L];
} polyvecl;


void polyvecl_freeze(polyvecl *v, int mod);

void polyvecl_add(polyvecl *w, const polyvecl *u, const polyvecl *v);

int polyvecl_chknorm(const polyvecl *v, uint32_t B, int mod);



/* Vectors of polynomials of length K */
typedef struct {
  poly vec[K];
} polyveck;

void polyvec_l_mul(polyvecl mat[K], polyvecl *s1, polyveck *t);

void polyveck_freeze(polyveck *v, int mod);

void polyveck_add(polyveck *w, const polyveck *u, const polyveck *v);
void polyveck_sub(polyveck *w, const polyveck *u, const polyveck *v);
void polyveck_neg(polyveck *v);
void polyveck_shiftl(polyveck *v, unsigned int k);

int polyveck_chknorm(const polyveck *v, uint32_t B, int mod);

void polyveck_power2round_avx2(polyveck *v1, polyveck *v0, const polyveck *v);
void polyveck_decompose_avx2(polyveck *v1, polyveck *v0, const polyveck *v);
unsigned int polyveck_make_hint_avx2(polyveck *h,
    const polyveck *u,
    const polyveck *v);
void polyveck_use_hint_avx2(polyveck *w, const polyveck *u, const polyveck *h);

#endif
