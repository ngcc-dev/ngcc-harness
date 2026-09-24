#include <stdint.h>
#include <immintrin.h>
#include "params.h"
#include "ntt.h"
#include "reduce.h"
#include <stdio.h>
#define R2 867 // R^2 mod q,R=2^16
#define qinv_vec _mm256_set1_epi16(QINV)
#define q_vec _mm256_set1_epi16(DTRU_Q)
#define v_vec _mm256_set1_epi16(BARRETT_V)
extern int16_t zetas4inv[DTRU_N/16];
int rq_inverse_recursive8_avx2(__m256i b_vec[8], const __m256i a_vec[8], __m256i zeta_vec);    
int baseinv_avx2(int16_t *b, const int16_t *a);       