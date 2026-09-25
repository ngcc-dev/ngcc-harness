#ifndef POLY_H
#define POLY_H

#include <stdint.h>
#include "params.h"

/*
 * Elements of R_q = Z_q[X]/(X^n - X^n/2 + 1). Represents polynomial
 * coeffs[0] + X*coeffs[1] + X^2*xoeffs[2] + ... + X^{n-1}*coeffs[n-1]
 */
typedef struct{
	int16_t coeffs[NTRUOAEP_N];
} poly;

void poly_tobytes(uint8_t r[NTRUOAEP_POLYBYTES], const poly *a);
void poly_frombytes(poly *r, const uint8_t a[NTRUOAEP_POLYBYTES]);

void poly_cbd1(poly *r, const uint8_t buf[NTRUOAEP_N/4]);
int poly_cbd1_inv(uint8_t *w, const poly *a, const uint8_t buf[NTRUOAEP_N/8]);
void poly_sotp(poly *r, const unsigned char *msg, const unsigned char *buf);
int  poly_sotp_inv(unsigned char *msg, const poly *e, const unsigned char *buf);

void poly_ntt(poly *r, const poly *a);
void poly_invntt(poly *r, const poly *a);
void poly_invntt_normalized(poly *r, const poly *a);
int  poly_is_invertible(const poly *a);
int  poly_baseinv(poly *r, const poly *a);
void poly_basemul(poly *r, const poly *a, const poly *b);
void poly_basemul_add(poly *r, const poly *a, const poly *b, const poly *c);
void poly_sub(poly *r, const poly *a, const poly *b);
void poly_triple(poly *r, const poly *a);
void poly_crepmod3(poly *r, const poly *a);
void short_poly_to_bytes(uint8_t *buf, const poly *a);

void poly_tomontgomery(poly *r, const poly *a);
void poly_frommontgomery(poly *r, const poly *a);

void poly_oaep(poly *s, poly *t, const uint8_t *r, const uint8_t *m);
int poly_oaep_inv(const poly *s, const poly *t, uint8_t *r, uint8_t *m);

#endif
