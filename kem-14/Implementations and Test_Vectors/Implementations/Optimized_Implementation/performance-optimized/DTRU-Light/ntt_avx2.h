#ifndef NTT_avx2_H
#define NTT_avx2_H

#include <stdint.h>
#include <immintrin.h>
#include "params.h"
#include "reduce.h"
#include "shuffle.h"

void ntt_avx2(int16_t b[DTRU_N], const int16_t a[DTRU_N]);
void invntt_avx2(int16_t b[DTRU_N], const int16_t a[DTRU_N]);
void basemul_avx2(int16_t *c, const int16_t *a, const int16_t *b, const int16_t *zeta);

#endif
