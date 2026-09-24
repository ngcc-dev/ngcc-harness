#ifndef OPSSIG_ROUNDING_H
#define OPSSIG_ROUNDING_H

#include <stdint.h>
#include <immintrin.h>

int32_t power2round(int32_t *a0, int32_t a);
int32_t decompose(int32_t *a0, int32_t a);
unsigned int make_hint(int32_t a0, int32_t a1);
int32_t use_hint(int32_t a, unsigned int hint);

void power2round_avx(__m256i *a1, __m256i *a0, const __m256i *a);
void decompose_avx(__m256i *a1, __m256i *a0, const __m256i *a);
unsigned int make_hint_avx(__m256i *h, const __m256i *a0, const __m256i *a1);
void use_hint_avx(__m256i *b, const __m256i *a, const __m256i *hint);

#endif
