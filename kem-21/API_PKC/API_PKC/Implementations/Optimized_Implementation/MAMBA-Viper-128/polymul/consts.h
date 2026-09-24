#ifndef VIPER_AVX_POLYMUL_CONSTS_H
#define VIPER_AVX_POLYMUL_CONSTS_H

#include <immintrin.h>
#include "../viper_params.h"

#define POLYMUL_N VIPER_N
#define POLYMUL_K VIPER_K

#define AVX_N (POLYMUL_N >> 4)
#define small_len_avx (AVX_N >> 2)

#define SCHB_N 16

#define N_SB (POLYMUL_N >> 2)
#define N_SB_RES (2*N_SB-1)

#define N_SB_16 (N_SB >> 2)
#define N_SB_16_RES (2*N_SB_16-1)

#define AVX_N1 16 /*N/16*/

#define SCM_SIZE 16

#define NUM_POLY POLYMUL_K

extern __m256i mask, inv3_avx, inv9_avx, inv15_avx, int45_avx, int30_avx, int0_avx;
void viper_avx_load_values(void);

void TC_interpol(__m256i *c_bucket, __m256i* res_avx_output);
void KARA_interpol(__m256i *c_bucket, __m256i* result_final0, __m256i* result_final1, __m256i* result_final2, __m256i* result_final3, __m256i* result_final4, __m256i* result_final5, __m256i* result_final6);
void KARA_eval(__m256i* b, __m256i *b_bucket);
void TC_eval(__m256i* b_avx, __m256i* b_bucket);
void toom_cook_4way_avx_n1(__m256i* a_avx,__m256i* b_bucket, __m256i *c_bucket, int f);

#endif
