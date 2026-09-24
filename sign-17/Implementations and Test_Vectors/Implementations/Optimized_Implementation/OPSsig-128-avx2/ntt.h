#ifndef OPSSIG_NTT_H
#define OPSSIG_NTT_H

#include <stdint.h>
#include <immintrin.h>
#include "params.h"

void ntt(int32_t a[N]);
void invntt_tomont(int32_t a[N]);
void pointwise_avx(__m256i *c, const __m256i *a, const __m256i *b, const __m256i *qdata);

#endif
