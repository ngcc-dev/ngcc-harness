#ifndef AVX2_NTT_H
#define AVX2_NTT_H

#include <immintrin.h>
#include "consts.h"
#include <stdio.h>
#include "shuffle.h"
#include "radix_ntt_n1087.h"

void butterfly(__m256i *l, __m256i *r, __m256i qinvzeta_0, __m256i qinvzeta_1, __m256i zetas_0, __m256i zetas_1);
void invbutterfly(__m256i *l, __m256i *r, __m256i qinvzeta_0, __m256i qinvzeta_1, __m256i zetas_0, __m256i zetas_1);
void butterfly3(__m256i *a0, __m256i *a1, __m256i *a2, __m256i qinvzeta_0, __m256i qinvzeta_1, __m256i zetas_0, __m256i zetas_1);

void barrett_reduce_avx2(__m256i *input);
__m256i montgomery_reduce_avx2(const __m256i a, const __m256i b);
__m256i CALC_D_avx2_intrinsic(__m256i *a, __m256i *b, int x, int y, __m256i *d);

void ntt_avx2_intrinsic(__m256i* f);
void invntt_avx2_intrinsic(__m256i* f);
void basemul3x3_avx2_intrinsic(__m256i* c, const __m256i* a, const __m256i* b);
void poly_radix_ntt_n1087_q1_intrinsic(poly *c, const poly *a, const poly *b);
#endif
