#ifndef NTT_H
#define NTT_H

#include <stdint.h>

#define ROOT_DIMENSION 2

extern int16_t zetas[384];
extern int16_t zetas_inv[386];
extern int16_t zetas_base[128];
extern int16_t zetas_invseq[384];

void ntt(int16_t *a);
void ntt_asm3457(int16_t a[768]);
void invntt(int16_t *a);
void invntt_asm(int16_t a[768]);
void basemul(int16_t *c, const int16_t *a, const int16_t *b, const int16_t zeta);

#endif
