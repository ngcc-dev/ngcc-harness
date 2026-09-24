#include <stdint.h>
#include <immintrin.h>
#include "params.h"
#include "reduce.h"
#include "rounding.h"

#define _mm256_blendv_epi32(a,b,mask) \
  _mm256_castps_si256(_mm256_blendv_ps(_mm256_castsi256_ps(a), \
                                       _mm256_castsi256_ps(b), \
                                       _mm256_castsi256_ps(mask)))

int32_t power2round(int32_t *a0, int32_t a)  {
  int32_t a1;

  a1 = (a + (1 << (D-1)) - 1) >> D;
  *a0 = a - (a1 << D);
  return a1;
}

void power2round_avx(__m256i *a1, __m256i *a0, const __m256i *a) {
  unsigned int i;
  __m256i f, f0, f1;
  const __m256i mask = _mm256_set1_epi32(-(1 << D));
  const __m256i half = _mm256_set1_epi32((1 << (D - 1)) - 1);

  for(i = 0; i < N/8; ++i) {
    f = _mm256_load_si256(&a[i]);
    f1 = _mm256_add_epi32(f, half);
    f0 = _mm256_and_si256(f1, mask);
    f1 = _mm256_srli_epi32(f1, D);
    f0 = _mm256_sub_epi32(f, f0);
    _mm256_store_si256(&a1[i], f1);
    _mm256_store_si256(&a0[i], f0);
  }
}

int32_t decompose(int32_t *a0, int32_t a) {
  int32_t a1;
  
#if GAMMA2 == (Q-1)/32
  a1 = ((((int64_t)a + 2097023) >> 8) * 524321) >> 33;
  //a1 = (a + 2097023)/4194048;
  a1 &= 15;
#elif GAMMA2 == (Q-1)/48
  a1 = ((((int64_t)a + 1398015) >> 9) * 393240) >> 31;
  //a1 = (a + 1398015)/2796032;
  a1 ^= ((23 - a1) >> 31) & a1;
#elif GAMMA2 == (Q-1)/96
  a1 = ((((int64_t)a + 699007) >> 8) * 786480) >> 32;
  //a1 = (a + 699007)/1398016;
  a1 ^= ((47 - a1) >> 31) & a1;
#endif

  *a0  = a - a1*2*GAMMA2;
  *a0 -= (((Q-1)/2 - *a0) >> 31) & Q;
  return a1;
}

void decompose_avx(__m256i *a1, __m256i *a0, const __m256i *a) {
  unsigned int i;
  __m256i f, f0, f1, t;
  const __m256i q = _mm256_set1_epi32(Q);
  const __m256i hq = _mm256_srli_epi32(q, 1);
  const __m256i alpha = _mm256_set1_epi32(2 * GAMMA2);

  for(i = 0; i < N/8; ++i) {
    f = _mm256_load_si256(&a[i]);
#if GAMMA2 == (Q-1)/32
    {
      const __m256i off = _mm256_set1_epi64x(524321);
      const __m256i bias = _mm256_set1_epi32(2097023);
      const __m256i mask = _mm256_set1_epi32(15);
      __m256i even, odd;
      uint64_t even64[4];
      uint64_t odd64[4];
      int32_t out[8];

      f1 = _mm256_add_epi32(f, bias);
      f1 = _mm256_srli_epi32(f1, 8);
      even = _mm256_mul_epu32(f1, off);
      odd = _mm256_mul_epu32(_mm256_srli_epi64(f1, 32), off);
      even = _mm256_srli_epi64(even, 33);
      odd = _mm256_srli_epi64(odd, 33);
      _mm256_storeu_si256((__m256i *)even64, even);
      _mm256_storeu_si256((__m256i *)odd64, odd);
      out[0] = (int32_t)even64[0];
      out[1] = (int32_t)odd64[0];
      out[2] = (int32_t)even64[1];
      out[3] = (int32_t)odd64[1];
      out[4] = (int32_t)even64[2];
      out[5] = (int32_t)odd64[2];
      out[6] = (int32_t)even64[3];
      out[7] = (int32_t)odd64[3];
      f1 = _mm256_loadu_si256((const __m256i *)out);
      f1 = _mm256_and_si256(f1, mask);
    }
#elif GAMMA2 == (Q-1)/48
    f1 = _mm256_add_epi32(f, _mm256_set1_epi32(1398015));
    f1 = _mm256_srli_epi32(f1, 9);
    {
      const __m256i mul = _mm256_set1_epi64x(393240);
      __m256i even = _mm256_mul_epu32(f1, mul);
      __m256i odd = _mm256_mul_epu32(_mm256_srli_epi64(f1, 32), mul);
      uint64_t even64[4];
      uint64_t odd64[4];
      int32_t out[8];

      even = _mm256_srli_epi64(even, 31);
      odd = _mm256_srli_epi64(odd, 31);
      _mm256_storeu_si256((__m256i *)even64, even);
      _mm256_storeu_si256((__m256i *)odd64, odd);
      out[0] = (int32_t)even64[0];
      out[1] = (int32_t)odd64[0];
      out[2] = (int32_t)even64[1];
      out[3] = (int32_t)odd64[1];
      out[4] = (int32_t)even64[2];
      out[5] = (int32_t)odd64[2];
      out[6] = (int32_t)even64[3];
      out[7] = (int32_t)odd64[3];
      f1 = _mm256_loadu_si256((const __m256i *)out);
    }
    t = _mm256_srai_epi32(_mm256_sub_epi32(_mm256_set1_epi32(23), f1), 31);
    f1 = _mm256_xor_si256(f1, _mm256_and_si256(t, f1));
#elif GAMMA2 == (Q-1)/96
    f1 = _mm256_add_epi32(f, _mm256_set1_epi32(699007));
    f1 = _mm256_srli_epi32(f1, 8);
    {
      const __m256i mul = _mm256_set1_epi64x(786480);
      __m256i even = _mm256_mul_epu32(f1, mul);
      __m256i odd = _mm256_mul_epu32(_mm256_srli_epi64(f1, 32), mul);
      uint64_t even64[4];
      uint64_t odd64[4];
      int32_t out[8];

      even = _mm256_srli_epi64(even, 32);
      odd = _mm256_srli_epi64(odd, 32);
      _mm256_storeu_si256((__m256i *)even64, even);
      _mm256_storeu_si256((__m256i *)odd64, odd);
      out[0] = (int32_t)even64[0];
      out[1] = (int32_t)odd64[0];
      out[2] = (int32_t)even64[1];
      out[3] = (int32_t)odd64[1];
      out[4] = (int32_t)even64[2];
      out[5] = (int32_t)odd64[2];
      out[6] = (int32_t)even64[3];
      out[7] = (int32_t)odd64[3];
      f1 = _mm256_loadu_si256((const __m256i *)out);
    }
    t = _mm256_srai_epi32(_mm256_sub_epi32(_mm256_set1_epi32(47), f1), 31);
    f1 = _mm256_xor_si256(f1, _mm256_and_si256(t, f1));
#endif
    f0 = _mm256_sub_epi32(f, _mm256_mullo_epi32(f1, alpha));
    t = _mm256_cmpgt_epi32(f0, hq);
    f0 = _mm256_sub_epi32(f0, _mm256_and_si256(t, q));
    _mm256_store_si256(&a1[i], f1);
    _mm256_store_si256(&a0[i], f0);
  }
}

unsigned int make_hint(int32_t a0, int32_t a1) { 
  if(a0 > GAMMA2 || a0 < -GAMMA2 || (a0 == -GAMMA2 && a1 != 0))
    return 1;

  return 0;
}

unsigned int make_hint_avx(__m256i *h, const __m256i *a0, const __m256i *a1) {
  unsigned int i, j, s = 0;
  const __m256i gamma2 = _mm256_set1_epi32(GAMMA2);
  const __m256i neg_gamma2 = _mm256_set1_epi32(-GAMMA2);
  const __m256i zero = _mm256_setzero_si256();
  const __m256i one = _mm256_set1_epi32(1);

  for(i = 0; i < N/8; ++i) {
    __m256i f0 = _mm256_load_si256(&a0[i]);
    __m256i f1 = _mm256_load_si256(&a1[i]);
    __m256i gt = _mm256_cmpgt_epi32(f0, gamma2);
    __m256i lt = _mm256_cmpgt_epi32(neg_gamma2, f0);
    __m256i eq = _mm256_cmpeq_epi32(f0, neg_gamma2);
    __m256i nz = _mm256_cmpgt_epi32(_mm256_abs_epi32(f1), zero);
    __m256i mask = _mm256_or_si256(_mm256_or_si256(gt, lt), _mm256_and_si256(eq, nz));
    __m256i hint = _mm256_and_si256(mask, one);
    int32_t tmp[8];

    _mm256_store_si256(&h[i], hint);
    _mm256_storeu_si256((__m256i *)tmp, hint);
    for(j = 0; j < 8; ++j)
      s += (unsigned int)tmp[j];
  }

  return s;
}

int32_t use_hint(int32_t a, unsigned int hint) {
  int32_t a0, a1;

  a1 = decompose(&a0, a);
  if(hint == 0)
    return a1;

  if(a0 > 0)
    return (a1 + 1) & 15;
  else
    return (a1 - 1) & 15;
}

void use_hint_avx(__m256i *b, const __m256i *a, const __m256i *hint) {
  unsigned int i;
  __m256i a0[N/8];
  __m256i f, g, h, t;
  const __m256i zero = _mm256_setzero_si256();
#if GAMMA2 == (Q-1)/32
  const __m256i mask = _mm256_set1_epi32(15);
#endif

  decompose_avx(b, a0, a);
  for(i = 0; i < N/8; ++i) {
    f = _mm256_load_si256(&a0[i]);
    g = _mm256_load_si256(&b[i]);
    h = _mm256_load_si256(&hint[i]);
    t = _mm256_blendv_epi32(zero, h, f);
    t = _mm256_slli_epi32(t, 1);
    h = _mm256_sub_epi32(h, t);
    g = _mm256_add_epi32(g, h);
#if GAMMA2 == (Q-1)/32
    g = _mm256_and_si256(g, mask);
#elif GAMMA2 == (Q-1)/48
    g = _mm256_xor_si256(g, _mm256_and_si256(_mm256_srai_epi32(_mm256_sub_epi32(_mm256_set1_epi32(23), g), 31), g));
#elif GAMMA2 == (Q-1)/96
    g = _mm256_xor_si256(g, _mm256_and_si256(_mm256_srai_epi32(_mm256_sub_epi32(_mm256_set1_epi32(47), g), 31), g));
#endif
    _mm256_store_si256(&b[i], g);
  }
}
