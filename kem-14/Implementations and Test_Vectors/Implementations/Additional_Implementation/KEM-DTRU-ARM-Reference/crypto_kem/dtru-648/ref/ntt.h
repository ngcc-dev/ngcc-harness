#ifndef NTT_H
#define NTT_H

#include <stdint.h>

#define ROOT_DIMENSION 9

extern int16_t zetas[72];
extern int16_t zetas_inv[74];
extern int16_t zetas_base[96];

void ntt(int16_t *a);
void invntt(int16_t *a);
void basemul(int16_t *c, const int16_t *a, const int16_t *b, const int16_t zeta);
void basemul3(int16_t c[3], const int16_t a[3], const int16_t b[3], const int16_t zeta);

#endif