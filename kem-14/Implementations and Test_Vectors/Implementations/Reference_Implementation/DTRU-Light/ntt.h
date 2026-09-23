#ifndef NTT_H
#define NTT_H

#include <stdint.h>

#define ROOT_DIMENSION 4

extern int16_t zetas[128];
extern int16_t zetas_inv[128];

void ntt(int16_t *a);
void invntt(int16_t *a);
void basemul(int16_t *c, const int16_t *a, const int16_t *b, const int16_t zeta);

#endif