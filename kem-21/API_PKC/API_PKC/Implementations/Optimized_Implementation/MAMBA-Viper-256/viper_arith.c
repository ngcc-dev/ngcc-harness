/* MAMBA-Viper implementation and implementation support layer where applicable.
 * Optimized AVX2 polynomial-arithmetic backend for Viper.
 */

#include "viper_arith.h"
#include "polymul/consts.h"
#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

__m256i mask, inv3_avx, inv9_avx, inv15_avx, int45_avx, int30_avx, int0_avx;

void viper_avx_load_values(void)
{
  inv3_avx = _mm256_set1_epi16((short)43691);
  inv9_avx = _mm256_set1_epi16((short)36409);
  inv15_avx = _mm256_set1_epi16((short)61167);
  int45_avx = _mm256_set1_epi16(45);
  int30_avx = _mm256_set1_epi16(30);
  int0_avx = _mm256_setzero_si256();
  mask = _mm256_cmpeq_epi32(_mm256_setzero_si256(), _mm256_setzero_si256());
}

static void poly_to_avx(__m256i out[AVX_N1], const vpoly in)
{
  for (size_t i = 0; i < AVX_N1; i++) {
    out[i] = _mm256_loadu_si256((const __m256i *)(const void *)&in[16 * i]);
  }
}

static void avx_to_poly(vpoly out, const __m256i in[AVX_N1])
{
  for (size_t i = 0; i < AVX_N1; i++) {
    _mm256_storeu_si256((__m256i *)(void *)&out[16 * i], in[i]);
  }
  for (size_t i = 0; i < VIPER_N; i++) out[i] &= VIPER_Q_MASK;
}

void poly_mul_basebackend_avx(vpoly c, const vpoly a, const vpoly b)
{
  __m256i a_avx[AVX_N1];
  __m256i b_avx[AVX_N1];
  __m256i b_bucket[SCHB_N * 4];
  __m256i c_bucket[2 * SCM_SIZE * 4];
  __m256i res_avx[AVX_N1];
  viper_avx_load_values();
  poly_to_avx(a_avx, a);
  poly_to_avx(b_avx, b);
  TC_eval(b_avx, b_bucket);
  toom_cook_4way_avx_n1(a_avx, b_bucket, c_bucket, 0);
  TC_interpol(c_bucket, res_avx);
  avx_to_poly(c, res_avx);
}

void matvec_basebackend_avx(vpolyvec out, vpoly A[VIPER_K][VIPER_K], const vpolyvec s)
{
  __m256i a_avx[VIPER_K][VIPER_K][AVX_N1];
  __m256i s_avx[VIPER_K][AVX_N1];
  __m256i b_bucket[VIPER_K][SCHB_N * 4];
  __m256i res_avx[VIPER_K][AVX_N1];
  __m256i c_bucket[2 * SCM_SIZE * 4];
  viper_avx_load_values();
  for (size_t i = 0; i < VIPER_K; i++) {
    poly_to_avx(s_avx[i], s[i]);
    TC_eval(s_avx[i], b_bucket[i]);
    for (size_t j = 0; j < VIPER_K; j++) poly_to_avx(a_avx[i][j], A[i][j]);
  }
  for (size_t i = 0; i < VIPER_K; i++) {
    for (size_t j = 0; j < VIPER_K; j++) {
      toom_cook_4way_avx_n1(a_avx[i][j], b_bucket[j], c_bucket, (int)j);
    }
    TC_interpol(c_bucket, res_avx[i]);
    avx_to_poly(out[i], res_avx[i]);
  }
}

void matTvec_basebackend_avx(vpolyvec out, vpoly A[VIPER_K][VIPER_K], const vpolyvec s)
{
  __m256i a_avx[VIPER_K][VIPER_K][AVX_N1];
  __m256i s_avx[VIPER_K][AVX_N1];
  __m256i b_bucket[VIPER_K][SCHB_N * 4];
  __m256i res_avx[VIPER_K][AVX_N1];
  __m256i c_bucket[2 * SCM_SIZE * 4];
  viper_avx_load_values();
  for (size_t i = 0; i < VIPER_K; i++) {
    poly_to_avx(s_avx[i], s[i]);
    TC_eval(s_avx[i], b_bucket[i]);
    for (size_t j = 0; j < VIPER_K; j++) poly_to_avx(a_avx[i][j], A[i][j]);
  }
  for (size_t i = 0; i < VIPER_K; i++) {
    for (size_t j = 0; j < VIPER_K; j++) {
      toom_cook_4way_avx_n1(a_avx[j][i], b_bucket[j], c_bucket, (int)j);
    }
    TC_interpol(c_bucket, res_avx[i]);
    avx_to_poly(out[i], res_avx[i]);
  }
}

void dot_basebackend_avx(vpoly out, const vpolyvec a, const vpolyvec b)
{
  __m256i a_avx[VIPER_K][AVX_N1];
  __m256i b_avx[VIPER_K][AVX_N1];
  __m256i b_bucket[VIPER_K][SCHB_N * 4];
  __m256i res_avx[AVX_N1];
  __m256i c_bucket[2 * SCM_SIZE * 4];
  viper_avx_load_values();
  for (size_t i = 0; i < VIPER_K; i++) {
    poly_to_avx(a_avx[i], a[i]);
    poly_to_avx(b_avx[i], b[i]);
    TC_eval(b_avx[i], b_bucket[i]);
  }
  for (size_t i = 0; i < VIPER_K; i++) {
    toom_cook_4way_avx_n1(a_avx[i], b_bucket[i], c_bucket, (int)i);
  }
  TC_interpol(c_bucket, res_avx);
  avx_to_poly(out, res_avx);
}
