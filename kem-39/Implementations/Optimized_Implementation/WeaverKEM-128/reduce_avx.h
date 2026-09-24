#ifndef REDUCE_AVX_H
#define REDUCE_AVX_H

#include <immintrin.h>
#include "params.h"

static inline __m256i barrett_reduce_x16(__m256i a)
{
  const __m256i v = _mm256_set1_epi32(((1 << 26) + WEAVER_Q / 2) / WEAVER_Q);
  const __m256i q = _mm256_set1_epi32(WEAVER_Q);
  const __m256i bias = _mm256_set1_epi32(1 << 25);
  __m256i alo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(a));
  __m256i ahi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(a, 1));
  __m256i tlo = _mm256_srai_epi32(_mm256_add_epi32(_mm256_mullo_epi32(alo, v), bias), 26);
  __m256i thi = _mm256_srai_epi32(_mm256_add_epi32(_mm256_mullo_epi32(ahi, v), bias), 26);

  tlo = _mm256_mullo_epi32(tlo, q);
  thi = _mm256_mullo_epi32(thi, q);
  alo = _mm256_sub_epi32(alo, tlo);
  ahi = _mm256_sub_epi32(ahi, thi);
  return _mm256_setr_m128i(_mm_packs_epi32(_mm256_castsi256_si128(alo),
                                          _mm256_extracti128_si256(alo, 1)),
                           _mm_packs_epi32(_mm256_castsi256_si128(ahi),
                                           _mm256_extracti128_si256(ahi, 1)));
}

static inline __m128i barrett_reduce_x8(__m128i a)
{
  const __m256i v = _mm256_set1_epi32(((1 << 26) + WEAVER_Q / 2) / WEAVER_Q);
  const __m256i q = _mm256_set1_epi32(WEAVER_Q);
  const __m256i bias = _mm256_set1_epi32(1 << 25);
  __m256i e = _mm256_cvtepi16_epi32(a);
  __m256i t = _mm256_srai_epi32(_mm256_add_epi32(_mm256_mullo_epi32(e, v), bias), 26);

  t = _mm256_mullo_epi32(t, q);
  e = _mm256_sub_epi32(e, t);
  return _mm_packs_epi32(_mm256_castsi256_si128(e), _mm256_extracti128_si256(e, 1));
}

#endif
