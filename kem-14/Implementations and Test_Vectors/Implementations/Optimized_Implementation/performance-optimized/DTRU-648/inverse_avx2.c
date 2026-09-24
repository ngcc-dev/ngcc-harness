#include <immintrin.h>
#include <stdint.h>

#include "inverse_avx2.h"
#include "ntt.h"
#include "params.h"
#include "reduce.h"

#define R2 867
#define qinv_vec _mm256_set1_epi16(QINV)
#define q_vec _mm256_set1_epi16(DTRU_Q)
#define v_vec _mm256_set1_epi16(BARRETT_V)

int16_t zetas9inv[80]={
1622, 2674, -4296, 1443, 725, -2168, 2505, 353, -2858, 3104, 2858, -5962, 839, 1835, -2674, 2168,
2014, -4182, 36, 1628, -1664, 1119, 3358, -4477, 2805, 1244, -4049, 2213, 592, -2805, 1664, 3421,
-5085, 1020, 2338, -3358, 3361, 268, -3629, 473, 264, -737, 2891, 1292, -4183, 2165, 726, -2891,
172, 96, -268, 737, 2984, -3721, 3246, 445, -3691, 2372, 3173, -5545, 2285, 391, -2676, 3066,
2676, -5742, 234, 211, -445, 2088, 1085, -3173, 1622, 1622, 1622, 1622, 1622, 1622, 1622, 1622
};

void shuffle8_avx2(__m256i *a, __m256i *b)
{
  __m256i c = _mm256_permute2x128_si256(*a, *b, 0x20);

  *b = _mm256_permute2x128_si256(*a, *b, 0x31);
  *a = c;
}

void shuffle4_avx2(__m256i *a, __m256i *b)
{
  __m256i c = _mm256_unpacklo_epi64(*a, *b);

  *b = _mm256_unpackhi_epi64(*a, *b);
  *a = c;
}

void shuffle2_avx2(__m256i *a, __m256i *b)
{
  __m256i b_shift = _mm256_slli_epi64(*b, 32);
  __m256i a_shift = _mm256_srli_epi64(*a, 32);

  *a = _mm256_blend_epi32(*a, b_shift, 0xAA);
  *b = _mm256_blend_epi32(a_shift, *b, 0xAA);
}

void shuffle1_avx2(__m256i *a, __m256i *b)
{
  __m256i b_shift = _mm256_slli_epi32(*b, 16);
  __m256i a_shift = _mm256_srli_epi32(*a, 16);

  *a = _mm256_blend_epi16(*a, b_shift, 0xAA);
  *b = _mm256_blend_epi16(a_shift, *b, 0xAA);
}

static void transpose16_avx2(__m256i a[16])
{
  for (int i = 0; i < 8; ++i)
    shuffle8_avx2(&a[i], &a[i + 8]);

  for (int i = 0; i < 16; i += 8)
    for (int j = 0; j < 4; ++j)
      shuffle4_avx2(&a[i + j], &a[i + j + 4]);

  for (int i = 0; i < 16; i += 4)
    for (int j = 0; j < 2; ++j)
      shuffle2_avx2(&a[i + j], &a[i + j + 2]);

  for (int i = 0; i < 16; i += 2)
    shuffle1_avx2(&a[i], &a[i + 1]);
}

static inline __m256i fqmul_avx2(__m256i a, __m256i b)
{
  __m256i hi = _mm256_mulhi_epi16(a, b);
  __m256i lo = _mm256_mullo_epi16(a, b);
  __m256i u = _mm256_mullo_epi16(qinv_vec, lo);
  __m256i t = _mm256_mulhi_epi16(q_vec, u);

  return _mm256_sub_epi16(hi, t);
}

static inline __m256i barrett_reduce_avx2(__m256i a)
{
  __m256i t = _mm256_mulhi_epi16(v_vec, a);

  t = _mm256_srai_epi16(t, 10);
  t = _mm256_mullo_epi16(q_vec, t);
  return _mm256_sub_epi16(a, t);
}

static __m256i fqinv_avx2(__m256i a)
{
  __m256i t = _mm256_set1_epi16(1);

  a = fqmul_avx2(a, _mm256_set1_epi16(R2));
  for (int16_t exp = DTRU_Q - 2; exp > 0; exp >>= 1)
  {
    if (exp & 1)
      t = fqmul_avx2(t, a);
    a = fqmul_avx2(a, a);
  }

  return t;
}

static inline __m256i calc_d_avx2(const __m256i a[3], const __m256i b[3],
                                  int x, int y, const __m256i d[3])
{
  __m256i s = fqmul_avx2(_mm256_add_epi16(a[x], a[y]),
                         _mm256_add_epi16(b[x], b[y]));

  s = _mm256_sub_epi16(s, d[x]);
  return _mm256_sub_epi16(s, d[y]);
}

static void basemul3_avx2(__m256i c[3], const __m256i a[3],
                          const __m256i b[3], __m256i zeta)
{
  __m256i d[3];
  __m256i t;

  for (int i = 0; i < 3; ++i)
    d[i] = fqmul_avx2(a[i], b[i]);

  t = fqmul_avx2(calc_d_avx2(a, b, 1, 2, d), zeta);
  c[0] = barrett_reduce_avx2(_mm256_add_epi16(d[0], t));

  t = fqmul_avx2(d[2], zeta);
  c[1] = barrett_reduce_avx2(_mm256_add_epi16(calc_d_avx2(a, b, 0, 1, d), t));

  t = _mm256_add_epi16(calc_d_avx2(a, b, 0, 2, d), d[1]);
  c[2] = barrett_reduce_avx2(t);
}

static int base_inv3_avx2(__m256i finv[3], const __m256i f[3],
                          __m256i zeta)
{
  __m256i t[3], d, invd, zero, is_zero;

  t[0] = _mm256_sub_epi16(fqmul_avx2(f[0], f[0]),
                          fqmul_avx2(fqmul_avx2(f[1], f[2]), zeta));
  t[1] = _mm256_sub_epi16(fqmul_avx2(fqmul_avx2(f[2], f[2]), zeta),
                          fqmul_avx2(f[0], f[1]));
  t[2] = _mm256_sub_epi16(fqmul_avx2(f[1], f[1]),
                          fqmul_avx2(f[0], f[2]));

  d = fqmul_avx2(t[0], f[0]);
  d = _mm256_add_epi16(d, fqmul_avx2(t[1], fqmul_avx2(f[2], zeta)));
  d = _mm256_add_epi16(d, fqmul_avx2(t[2], fqmul_avx2(f[1], zeta)));

  invd = fqinv_avx2(d);
  zero = _mm256_setzero_si256();
  is_zero = _mm256_cmpeq_epi16(invd, zero);
  if (_mm256_movemask_epi8(is_zero) != 0)
    return 1;

  for (int i = 0; i < 3; ++i)
    finv[i] = fqmul_avx2(invd, t[i]);

  return 0;
}

static int rq_inverse9_avx2(__m256i finv[ROOT_DIMENSION],
                            const __m256i f[ROOT_DIMENSION],
                            __m256i zeta)
{
  __m256i f0[3], f1[3], f2[3];
  __m256i f0_sq[3], f1_sq[3], f2_sq[3];
  __m256i f1f2[3], f0f1[3], f0f2[3];
  __m256i N0[3], N1[3], N2[3];
  __m256i N0f0[3], N1f2[3], N2f1[3];
  __m256i D[3], invD[3];
  __m256i g0[3], g1[3], g2[3];

  f0[0] = f[0]; f0[1] = f[3]; f0[2] = f[6];
  f1[0] = f[1]; f1[1] = f[4]; f1[2] = f[7];
  f2[0] = f[2]; f2[1] = f[5]; f2[2] = f[8];

  basemul3_avx2(f0_sq, f0, f0, zeta);
  basemul3_avx2(f1_sq, f1, f1, zeta);
  basemul3_avx2(f2_sq, f2, f2, zeta);
  basemul3_avx2(f0f1, f0, f1, zeta);
  basemul3_avx2(f0f2, f0, f2, zeta);
  basemul3_avx2(f1f2, f1, f2, zeta);

  N0[0] = _mm256_sub_epi16(f0_sq[0], fqmul_avx2(zeta, f1f2[2]));
  N0[1] = _mm256_sub_epi16(f0_sq[1], f1f2[0]);
  N0[2] = _mm256_sub_epi16(f0_sq[2], f1f2[1]);

  N1[0] = _mm256_sub_epi16(fqmul_avx2(zeta, f2_sq[2]), f0f1[0]);
  N1[1] = _mm256_sub_epi16(f2_sq[0], f0f1[1]);
  N1[2] = _mm256_sub_epi16(f2_sq[1], f0f1[2]);

  N2[0] = _mm256_sub_epi16(f1_sq[0], f0f2[0]);
  N2[1] = _mm256_sub_epi16(f1_sq[1], f0f2[1]);
  N2[2] = _mm256_sub_epi16(f1_sq[2], f0f2[2]);

  basemul3_avx2(N0f0, N0, f0, zeta);
  basemul3_avx2(N1f2, N1, f2, zeta);
  basemul3_avx2(N2f1, N2, f1, zeta);

  D[0] = _mm256_add_epi16(N1f2[2], N2f1[2]);
  D[0] = fqmul_avx2(D[0], zeta);
  D[0] = barrett_reduce_avx2(_mm256_add_epi16(N0f0[0], D[0]));

  D[1] = _mm256_add_epi16(N0f0[1], N1f2[0]);
  D[1] = barrett_reduce_avx2(_mm256_add_epi16(D[1], N2f1[0]));

  D[2] = _mm256_add_epi16(N0f0[2], N1f2[1]);
  D[2] = barrett_reduce_avx2(_mm256_add_epi16(D[2], N2f1[1]));

  if (base_inv3_avx2(invD, D, zeta) != 0)
    return 1;

  basemul3_avx2(g0, N0, invD, zeta);
  basemul3_avx2(g1, N1, invD, zeta);
  basemul3_avx2(g2, N2, invD, zeta);

  finv[0] = g0[0]; finv[1] = g1[0]; finv[2] = g2[0];
  finv[3] = g0[1]; finv[4] = g1[1]; finv[5] = g2[1];
  finv[6] = g0[2]; finv[7] = g1[2]; finv[8] = g2[2];

  return 0;
}

int baseinv_avx2(int16_t *b, const int16_t *a)
{
  int total_blocks = DTRU_N / ROOT_DIMENSION;
  int r = 0;

  for (int i = 0; i < (total_blocks + 15) / 16; ++i)
  {
    int block = 16 * i;
    int valid = total_blocks - block;
    __m256i b_vec[16], a_vec[16], zeta_vec;

    if (valid > 16)
      valid = 16;

    for (int j = 0; j < 16; ++j)
    {
      if (j < valid)
      {
        int offset = ROOT_DIMENSION * (block + j);
        __m128i lo = _mm_loadu_si128((const __m128i *)(a + offset));
        __m128i hi = _mm_cvtsi32_si128((uint16_t)a[offset + 8]);

        a_vec[j] = _mm256_inserti128_si256(_mm256_castsi128_si256(lo), hi, 1);
      }
      else
      {
        __m128i lo = _mm_cvtsi32_si128(1);

        a_vec[j] = _mm256_inserti128_si256(_mm256_castsi128_si256(lo),
                                           _mm_setzero_si128(), 1);
      }
    }

    transpose16_avx2(a_vec);

    zeta_vec = _mm256_loadu_si256((const __m256i *)(zetas9inv + 16 * i));

    r += rq_inverse9_avx2(b_vec, a_vec, zeta_vec);
    if (r != 0)
      return r;

    for (int j = ROOT_DIMENSION; j < 16; ++j)
      b_vec[j] = _mm256_setzero_si256();

    transpose16_avx2(b_vec);

    for (int j = 0; j < valid; ++j)
    {
      int offset = ROOT_DIMENSION * (block + j);
      __m128i hi = _mm256_extracti128_si256(b_vec[j], 1);

      _mm_storeu_si128((__m128i *)(b + offset), _mm256_castsi256_si128(b_vec[j]));
      b[offset + 8] = (int16_t)_mm_extract_epi16(hi, 0);
    }
  }

  return r;
}
