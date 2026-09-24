#include <immintrin.h>
#include <stdint.h>

#include "params.h"
#include "reduce.h"
#include "poly.h"
#include "ntt.h"
#include "poly_avx2.h"

static inline __m256i barrett_reduce_avx2_16(__m256i x)
{
  const __m256i barrett_v = _mm256_set1_epi16(BARRETT_V);
  const __m256i q_vec = _mm256_set1_epi16(DTRU_Q);

  __m256i t = _mm256_mulhi_epi16(barrett_v, x);
  t = _mm256_srai_epi16(t, 10);   // 16 + 10 = 26
  t = _mm256_mullo_epi16(t, q_vec);
  return _mm256_sub_epi16(x, t);
}

static inline __m256i fqcsubq_avx2_16(__m256i x)
{
  const __m256i q_vec = _mm256_set1_epi16(DTRU_Q);
  __m256i mask = _mm256_srai_epi16(x, 15);

  x = _mm256_add_epi16(x, _mm256_and_si256(mask, q_vec));
  x = _mm256_sub_epi16(x, q_vec);

  mask = _mm256_srai_epi16(x, 15);
  x = _mm256_add_epi16(x, _mm256_and_si256(mask, q_vec));
  return x;
}

void poly_multi_p_avx2_impl(poly *b, const poly *a)
{
  unsigned int i;
  for (i = 0; i < DTRU_N / 16; ++i)
  {
    __m256i av = _mm256_loadu_si256((const __m256i *)&a->coeffs[16 * i]);
    __m256i bv = _mm256_slli_epi16(av, 1);
    _mm256_storeu_si256((__m256i *)&b->coeffs[16 * i], bv);
  }

  for (i = (DTRU_N / 16) * 16; i < DTRU_N; ++i)
  {
    b->coeffs[i] = 2 * a->coeffs[i];
  }
}

void poly_add_avx2_impl(poly *c, const poly *a, const poly *b)
{
  unsigned int i;
  for (i = 0; i < DTRU_N / 16; ++i)
  {
    __m256i av = _mm256_loadu_si256((const __m256i *)&a->coeffs[16 * i]);
    __m256i bv = _mm256_loadu_si256((const __m256i *)&b->coeffs[16 * i]);
    __m256i cv = _mm256_add_epi16(av, bv);
    _mm256_storeu_si256((__m256i *)&c->coeffs[16 * i], cv);
  }

  for (i = (DTRU_N / 16) * 16; i < DTRU_N; ++i)
  {
    c->coeffs[i] = a->coeffs[i] + b->coeffs[i];
  }
}

void poly_freeze_avx2_impl(poly *a)
{
  unsigned int i;
  for (i = 0; i < DTRU_N / 16; ++i)
  {
    __m256i v = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i]);
    v = barrett_reduce_avx2_16(v);
    v = fqcsubq_avx2_16(v);
    _mm256_storeu_si256((__m256i *)&a->coeffs[16 * i], v);
  }

  for (i = (DTRU_N / 16) * 16; i < DTRU_N; ++i)
  {
    a->coeffs[i] = barrett_reduce(a->coeffs[i]);
    a->coeffs[i] = fqcsubq(a->coeffs[i]);
  }
}