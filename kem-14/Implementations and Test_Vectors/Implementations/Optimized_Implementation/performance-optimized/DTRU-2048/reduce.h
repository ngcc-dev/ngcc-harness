#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>
#include <immintrin.h>
#include "params.h"

extern int16_t fq_inverse_table[DTRU_Q];


#define MONT 3310   // 2^16 mod 3457
#define QINV 12929   // 3457^(-1) mod 2^16
#define BARRETT_V 19412 // (2^26 / 3457 = 19412.45..) rounded to nearest integer

int16_t montgomery_reduce(int32_t a);
int16_t barrett_reduce(int16_t a);
int16_t fqcsubq(int16_t a);
int16_t fqmul(int16_t a, int16_t b);
int16_t fqinv(int16_t a);

// AVX2 intrinsic functions
__m256i fqcsubq_avx2(__m256i a, __m256i q_vec);
__m256i _mm256_barrett_epi16(__m256i h, __m256i barrett_v_vec, __m256i qvec);
__m256i _mm256_fqmul_epi16(__m256i h1, __m256i h2, __m256i qvec, __m256i qinvvec);
__m256i _mm256_fqinv_epi16(__m256i a, __m256i qvec, __m256i qinvvec);

#endif
