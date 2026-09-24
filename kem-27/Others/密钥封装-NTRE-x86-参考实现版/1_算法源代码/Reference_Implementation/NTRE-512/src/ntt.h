#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "params.h"

extern const int16_t zetas[576];
extern const int16_t base_zetas[288];

void ntt(int16_t r[NTRE_N]);
void invntt(int16_t r[NTRE_N]);

int  baseinv(int16_t r[4], const int16_t a[4], int16_t zeta);
void basemul(int16_t r[4], const int16_t a[4], const int16_t b[4], int16_t zeta);
void basemul_add(int16_t r[4], const int16_t a[4], const int16_t b[4], const int16_t c[4], int16_t zeta);

#endif
