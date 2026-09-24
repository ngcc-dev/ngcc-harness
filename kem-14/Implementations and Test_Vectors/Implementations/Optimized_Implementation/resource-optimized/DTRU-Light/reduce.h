#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>
#include <immintrin.h>
#include "params.h"

extern int16_t fq_inverse_table[DTRU_Q];


#define MONT 171   // 2^16 mod 769
#define QINV -767   // 769^(-1) mod 2^16
#define BARRETT_V 21817 // (2^24 / 769 = 21816.92..) rounded to nearest integer

int16_t montgomery_reduce(int32_t a);
int16_t barrett_reduce(int16_t a);
int16_t fqcsubq(int16_t a);
int16_t fqmul(int16_t a, int16_t b);
int16_t fqinv(int16_t a);

#define Q1 3329
#define Q2 7681
#define QINV1 -3327 // q1^-1 mod 2^16
#define QINV2 -7679 // q2^-1 mod  2^16


#define MONT2 19 // Mont^2 mod q

#define BARRETT_V 21817

int16_t fquniform();

int16_t montgomery_reduce1(int32_t a);
int16_t montgomery_reduce2(int32_t a);
int16_t barrett_reduce1(int16_t a);
int16_t barrett_reduce2(int16_t a);

// AVX2 implementations
__m256i fqcsubq_avx2(__m256i a, __m256i q_vec);
__m256i _mm256_barrett_epi16(__m256i h, __m256i barrett_v_vec, __m256i qvec);
__m256i _mm256_fqmul_epi16(__m256i h1, __m256i h2, __m256i qvec, __m256i qinvvec);
__m256i _mm256_fqinv_epi16(__m256i a, __m256i qvec, __m256i qinvvec);

#endif
