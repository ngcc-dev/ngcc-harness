#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "params.h"

typedef struct {
  uint16_t coeffs[PARAM_N];
} poly __attribute__ ((aligned (32)));

void poly_uniform(poly *a, const unsigned char *seed);
void poly_getnoise(poly *r, const unsigned char *seed, unsigned char nonce);
void poly_add(poly *r, const poly *a, const poly *b);

/* Toom-Cook-4 negacyclic convolution r = a*b in Z_q[x]/(x^n+1) */
void poly_convolution(poly *r, const poly *a, const poly *b);

/* Constant-time AVX2 protocol multiplication; Toom-Cook fallback without AVX2. */
void poly_mul_small(poly *r, const poly *a, const poly *small);

/* Generic multiplication retained for the reference/fallback path. */
void poly_pointwise(poly *r, const poly *a, const poly *b);

/* NTT wrappers — no-ops with schoolbook convolution */
void poly_ntt(poly *r);
void poly_invntt(poly *r);

void poly_frombytes(poly *r, const unsigned char *a);
void poly_tobytes(unsigned char *r, const poly *p);

#endif
