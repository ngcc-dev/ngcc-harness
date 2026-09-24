#ifndef TOOM_H
#define TOOM_H

#include <stdint.h>
#include "params.h"

/*
 * toom4_mul: Toom-Cook-4 polynomial multiplication over Z.
 *
 * Computes r = a * b where a, b are degree-(n-1) polynomials
 * with int64_t coefficients. The result r has 2*n-1 int64_t
 * coefficients (the full product, not yet reduced modulo x^n+1).
 *
 * All arithmetic is exact over Z (no modular reduction).
 */
void toom4_mul(int64_t *r, const int64_t *a, const int64_t *b, int n);

#endif
