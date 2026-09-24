#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "params.h"

extern const int16_t zetas[216];

void ntt(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N]);
void invntt(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N]);
void invntt_normalized(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N]);

int  baseinv(int16_t r[3], const int16_t a[3], int16_t zeta);
void basemul(int16_t r[3], const int16_t a[3], const int16_t b[3], int16_t zeta);
void basemul_add(int16_t r[3], const int16_t a[3], const int16_t b[3], const int16_t c[3], int16_t zeta);

#endif
