#include "inverse_avx2.h"

#include <immintrin.h>

#include "ntt.h"
#include "params.h"
#include "reduce.h"

#define R2 867

extern const int16_t zetas_base_exp[384];

static inline __m256i fqmul_avx2(__m256i a, __m256i b)
{
  const __m256i q = _mm256_set1_epi16(DTRU_Q);
  const __m256i qinv = _mm256_set1_epi16(QINV);
  __m256i hi = _mm256_mulhi_epi16(a, b);
  __m256i lo = _mm256_mullo_epi16(a, b);
  __m256i t = _mm256_mullo_epi16(qinv, lo);

  t = _mm256_mulhi_epi16(q, t);
  return _mm256_sub_epi16(hi, t);
}

static inline __m256i barrett_reduce_avx2(__m256i a)
{
  const __m256i q = _mm256_set1_epi16(DTRU_Q);
  const __m256i v = _mm256_set1_epi16(BARRETT_V);
  __m256i t = _mm256_mulhi_epi16(v, a);

  t = _mm256_srai_epi16(t, 10);
  t = _mm256_mullo_epi16(q, t);
  return _mm256_sub_epi16(a, t);
}

static inline __m256i fqinv_avx2(__m256i a)
{
  __m256i t = _mm256_set1_epi16(1);

  a = fqmul_avx2(a, _mm256_set1_epi16(R2));
  for (int16_t exp = DTRU_Q - 2; exp > 0; exp >>= 1)
  {
    __m256i prod = fqmul_avx2(t, a);
    __m256i mask = _mm256_set1_epi16(-((int16_t)exp & 1));

    t = _mm256_blendv_epi8(t, prod, mask);
    a = fqmul_avx2(a, a);
  }

  return t;
}

static inline int zero_lane_mask(__m256i a)
{
  const __m256i zero = _mm256_setzero_si256();
  __m256i is_zero = _mm256_cmpeq_epi16(a, zero);

  return _mm256_movemask_epi8(is_zero);
}

int baseinv_avx2(int16_t *b, const int16_t *a)
{
  const __m256i zero = _mm256_setzero_si256();
  int r = 0;

  for (int i = 0; i < DTRU_N / (ROOT_DIMENSION * 16); ++i)
  {
    __m256i a0 = _mm256_loadu_si256((const __m256i *)(a + 32 * i));
    __m256i a1 = _mm256_loadu_si256((const __m256i *)(a + 32 * i + 16));
    __m256i zeta = _mm256_loadu_si256((const __m256i *)(zetas_base_exp + 16 * i));
    __m256i det = fqmul_avx2(a1, a1);
    __m256i b0, b1;

    det = fqmul_avx2(det, _mm256_sub_epi16(zero, zeta));
    det = _mm256_add_epi16(det, fqmul_avx2(a0, a0));
    det = barrett_reduce_avx2(det);
    det = fqinv_avx2(det);

    r |= zero_lane_mask(det) != 0;

    b0 = fqmul_avx2(a0, det);
    b1 = fqmul_avx2(_mm256_sub_epi16(zero, a1), det);

    _mm256_storeu_si256((__m256i *)(b + 32 * i), b0);
    _mm256_storeu_si256((__m256i *)(b + 32 * i + 16), b1);
  }

  return -r;
}
