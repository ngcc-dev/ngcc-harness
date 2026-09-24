#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "params.h"
#include "align.h"

/* 
 * Elements of R_q = Z_q[X]/(X^n + 1). Represents polynomial
 * coeffs[0] + X*coeffs[1] + X^2*xoeffs[2] + ... + X^{n-1}*coeffs[n-1] 
 */
typedef struct{
	ALIGN(32) int16_t coeffs[PARAM_N];
} poly;


void poly_caddq(poly *r);
void poly_reduce(poly *r);
void poly_getmontgomery(poly *r);
void poly_add_vinv(poly *f);
void poly_rotv(poly* r, poly* a);
void poly_add(poly *r, const poly *a, const poly *b);
void poly_sub(poly *r, const poly *a, const poly *b);
void poly_ntt(poly *r);
void poly_invntt(poly *r);
void poly_mont_mul(poly * r, const poly*a, const poly*b);
int poly_mont2_inverse(poly* r, const poly*a);
int poly_mont2_inverse_judge(const poly*a);
void poly_extract(uint8_t *key, poly *m);
#endif
