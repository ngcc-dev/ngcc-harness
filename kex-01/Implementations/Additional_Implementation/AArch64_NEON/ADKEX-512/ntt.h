#ifndef DKE_NTT_H
#define DKE_NTT_H

#include "parameters.h"
#include <stdint.h>

extern const int16_t DKE_zetas[DKE_NTT_ZETAS_LEN];

void DKE_ntt(int16_t r[DKE_N]);
void DKE_invntt(int16_t r[DKE_N]);
void DKE_basemul(int16_t r[2], const int16_t a[2], const int16_t b[2], int16_t zeta);

#endif
