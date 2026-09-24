#ifndef NTT_H
#define NTT_H

#include <stdint.h>

#define ROOT_DIMENSION 4

extern int16_t zetas[384];
extern int16_t zetas_inv[386];
extern int16_t zetas_base[128];

void ntt(int16_t *a);
void stage1(int16_t *a);
void invntt(int16_t *a);
void basemul(int16_t *c, const int16_t *a, const int16_t *b, const int16_t zeta);
void ntt_fpu(int16_t *a);
void invntt_fpu(int16_t *a);

extern int16_t zetas_fpu[400];
extern int16_t zetas_inv_fpu[394];
extern int16_t zetas_base_fpu[384];
#endif