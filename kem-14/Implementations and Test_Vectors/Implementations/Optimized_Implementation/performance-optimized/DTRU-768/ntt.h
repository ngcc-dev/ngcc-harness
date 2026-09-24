#ifndef NTT_H
#define NTT_H

#include <stdint.h>

#define ROOT_DIMENSION 2

extern int16_t zetas_ref[384];
extern int16_t zetas_inv_ref[386];
extern int16_t zetas_base_ref[128];

void ntt(int16_t *a);
void invntt(int16_t *a);
void basemul(int16_t *c, const int16_t *a, const int16_t *b, const int16_t zeta);

// AVX2 implementations
#include "poly.h"

extern int16_t zetas[384];
extern int16_t zetas_inv[384];
extern int16_t zetas_base[384];

void ntt_avx(poly *b, const poly *a);
void invntt_avx(int16_t b[], const int16_t a[]);
void basemul_avx(poly *c, const poly *a, const poly *b);
void ntt_avx7681(int16_t b[], const int16_t a[]);
void invntt_avx7681(int16_t b[], const int16_t a[]);
void basemul_avx7681(poly *c, const poly *a, const poly *b);
int baseinv_avx(int16_t b[], const int16_t a[]);

void frommont_avx(int16_t b[]);
void polydouble_avx(int16_t b[], const int16_t a[]);
void polyadd_avx(int16_t c[], const int16_t a[], const int16_t b[]);
void freeze_avx(int16_t b[]);
void barret_avx(int16_t b[]);
int baseinv(int16_t b[2], const int16_t a[2], int16_t zeta);

#endif