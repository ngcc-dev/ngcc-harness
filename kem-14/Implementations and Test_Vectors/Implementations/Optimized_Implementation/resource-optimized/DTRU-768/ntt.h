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
int baseinv(int16_t b[2], const int16_t a[2], int16_t zeta);

#endif
