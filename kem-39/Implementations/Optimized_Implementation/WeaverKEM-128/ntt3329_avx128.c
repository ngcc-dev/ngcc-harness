/*
 * q=3329, n=128 AVX2 NTT matching ref/src/ntt.c bit-for-bit (WEAVER-640 / mode 1).
 * Montgomery mul uses mullo/mulhi_epi16 (Kyber-style), not int32 widen.
 */
#include <stdint.h>
#include <immintrin.h>

#include "params.h"
#include "ntt.h"
#include "reduce.h"
#include "reduce_avx.h"
#include "ntt3329_avx128.h"

#if defined(WEAVER_USE_AVX_NTT128) && (WEAVER_Q == 3329) && (WEAVER_N == 128)

static int16_t fqmul_scalar(int16_t a, int16_t b)
{
  return montgomery_reduce((int32_t)a * b);
}

static __m256i vq(void)
{
  return _mm256_set1_epi16(WEAVER_Q);
}

static __m256i vqinv(void)
{
  return _mm256_set1_epi16(QINV);
}

static __m128i vq128(void)
{
  return _mm_set1_epi16(WEAVER_Q);
}

static __m128i vqinv128(void)
{
  return _mm_set1_epi16(QINV);
}

static __m256i montmul_x16(__m256i a, __m256i b)
{
  __m256i l = _mm256_mullo_epi16(a, b);
  __m256i h = _mm256_mulhi_epi16(a, b);
  __m256i t = _mm256_mullo_epi16(l, vqinv());

  t = _mm256_mulhi_epi16(t, vq());
  return _mm256_sub_epi16(h, t);
}

static __m128i montmul_x8(__m128i a, __m128i b)
{
  __m128i l = _mm_mullo_epi16(a, b);
  __m128i h = _mm_mulhi_epi16(a, b);
  __m128i t = _mm_mullo_epi16(l, vqinv128());

  t = _mm_mulhi_epi16(t, vq128());
  return _mm_sub_epi16(h, t);
}

static __m256i fqmul_zeta_x16(__m256i a, int16_t zeta)
{
  return montmul_x16(a, _mm256_set1_epi16(zeta));
}

static __m128i fqmul_zeta_x8(__m128i a, int16_t zeta)
{
  return montmul_x8(a, _mm_set1_epi16(zeta));
}

static __attribute__((always_inline)) inline void butterfly_lazy(int16_t *r,
                                                                 unsigned start,
                                                                 unsigned len,
                                                                 int16_t zeta)
{
  unsigned j;

  for(j = start; j + 16 <= start + len; j += 16) {
    __m256i u = _mm256_loadu_si256((__m256i *)&r[j]);
    __m256i v = _mm256_loadu_si256((__m256i *)&r[j + len]);
    __m256i t = fqmul_zeta_x16(v, zeta);

    _mm256_storeu_si256((__m256i *)&r[j + len], _mm256_sub_epi16(u, t));
    _mm256_storeu_si256((__m256i *)&r[j], _mm256_add_epi16(u, t));
  }

  for(; j + 8 <= start + len; j += 8) {
    __m128i u = _mm_loadu_si128((__m128i *)&r[j]);
    __m128i v = _mm_loadu_si128((__m128i *)&r[j + len]);
    __m128i t = fqmul_zeta_x8(v, zeta);

    _mm_storeu_si128((__m128i *)&r[j + len], _mm_sub_epi16(u, t));
    _mm_storeu_si128((__m128i *)&r[j], _mm_add_epi16(u, t));
  }

  for(; j < start + len; j++) {
    int16_t t = fqmul_scalar(zeta, r[j + len]);

    r[j + len] = r[j] - t;
    r[j] = r[j] + t;
  }
}

/* Eight adjacent (2i,2i+1) pairs at r[start..start+15], z[0..7] per pair. */
static __attribute__((always_inline)) inline void
butterfly_lazy_len1_x8(int16_t *r, unsigned start, const int16_t z[8])
{
  unsigned i;

  for(i = 0; i < 8; i++) {
    int16_t u = r[start + 2 * i];
    int16_t t = fqmul_scalar(z[i], r[start + 2 * i + 1]);

    r[start + 2 * i + 1] = u - t;
    r[start + 2 * i] = u + t;
  }
}

/* Two pairs (start,start+2) and (start+1,start+3) sharing one zeta. */
static __attribute__((always_inline)) inline void
butterfly_lazy_len2(int16_t *r, unsigned start, int16_t zeta)
{
  butterfly_lazy(r, start, 2, zeta);
}

static void inv_butterfly_n128(int16_t *r, unsigned start, unsigned len, int16_t zeta)
{
  unsigned j;

  for(j = start; j + 16 <= start + len; j += 16) {
    __m256i u = _mm256_loadu_si256((__m256i *)&r[j]);
    __m256i v = _mm256_loadu_si256((__m256i *)&r[j + len]);
    __m256i sum = _mm256_add_epi16(u, v);
    __m256i diff = _mm256_sub_epi16(v, u);

    _mm256_storeu_si256((__m256i *)&r[j], barrett_reduce_x16(sum));
    _mm256_storeu_si256((__m256i *)&r[j + len], fqmul_zeta_x16(diff, zeta));
  }

  for(; j + 8 <= start + len; j += 8) {
    __m128i u = _mm_loadu_si128((__m128i *)&r[j]);
    __m128i v = _mm_loadu_si128((__m128i *)&r[j + len]);
    __m128i sum = _mm_add_epi16(u, v);
    __m128i diff = _mm_sub_epi16(v, u);

    _mm_storeu_si128((__m128i *)&r[j], barrett_reduce_x8(sum));
    _mm_storeu_si128((__m128i *)&r[j + len], fqmul_zeta_x8(diff, zeta));
  }

  for(; j < start + len; j++) {
    int16_t t = r[j];

    r[j] = barrett_reduce(t + r[j + len]);
    r[j + len] = r[j + len] - t;
    r[j + len] = fqmul_scalar(zeta, r[j + len]);
  }
}

void ntt3329_avx128(int16_t r[WEAVER_N])
{
  butterfly_lazy(r, 0, 64, zetas[1]);
  butterfly_lazy(r, 0, 32, zetas[2]);
  butterfly_lazy(r, 64, 32, zetas[3]);
  butterfly_lazy(r, 0, 16, zetas[4]);
  butterfly_lazy(r, 32, 16, zetas[5]);
  butterfly_lazy(r, 64, 16, zetas[6]);
  butterfly_lazy(r, 96, 16, zetas[7]);
  butterfly_lazy(r, 0, 8, zetas[8]);
  butterfly_lazy(r, 16, 8, zetas[9]);
  butterfly_lazy(r, 32, 8, zetas[10]);
  butterfly_lazy(r, 48, 8, zetas[11]);
  butterfly_lazy(r, 64, 8, zetas[12]);
  butterfly_lazy(r, 80, 8, zetas[13]);
  butterfly_lazy(r, 96, 8, zetas[14]);
  butterfly_lazy(r, 112, 8, zetas[15]);
  butterfly_lazy(r, 0, 4, zetas[16]);
  butterfly_lazy(r, 8, 4, zetas[17]);
  butterfly_lazy(r, 16, 4, zetas[18]);
  butterfly_lazy(r, 24, 4, zetas[19]);
  butterfly_lazy(r, 32, 4, zetas[20]);
  butterfly_lazy(r, 40, 4, zetas[21]);
  butterfly_lazy(r, 48, 4, zetas[22]);
  butterfly_lazy(r, 56, 4, zetas[23]);
  butterfly_lazy(r, 64, 4, zetas[24]);
  butterfly_lazy(r, 72, 4, zetas[25]);
  butterfly_lazy(r, 80, 4, zetas[26]);
  butterfly_lazy(r, 88, 4, zetas[27]);
  butterfly_lazy(r, 96, 4, zetas[28]);
  butterfly_lazy(r, 104, 4, zetas[29]);
  butterfly_lazy(r, 112, 4, zetas[30]);
  butterfly_lazy(r, 120, 4, zetas[31]);
  butterfly_lazy_len2(r, 0, zetas[32]);
  butterfly_lazy_len2(r, 4, zetas[33]);
  butterfly_lazy_len2(r, 8, zetas[34]);
  butterfly_lazy_len2(r, 12, zetas[35]);
  butterfly_lazy_len2(r, 16, zetas[36]);
  butterfly_lazy_len2(r, 20, zetas[37]);
  butterfly_lazy_len2(r, 24, zetas[38]);
  butterfly_lazy_len2(r, 28, zetas[39]);
  butterfly_lazy_len2(r, 32, zetas[40]);
  butterfly_lazy_len2(r, 36, zetas[41]);
  butterfly_lazy_len2(r, 40, zetas[42]);
  butterfly_lazy_len2(r, 44, zetas[43]);
  butterfly_lazy_len2(r, 48, zetas[44]);
  butterfly_lazy_len2(r, 52, zetas[45]);
  butterfly_lazy_len2(r, 56, zetas[46]);
  butterfly_lazy_len2(r, 60, zetas[47]);
  butterfly_lazy_len2(r, 64, zetas[48]);
  butterfly_lazy_len2(r, 68, zetas[49]);
  butterfly_lazy_len2(r, 72, zetas[50]);
  butterfly_lazy_len2(r, 76, zetas[51]);
  butterfly_lazy_len2(r, 80, zetas[52]);
  butterfly_lazy_len2(r, 84, zetas[53]);
  butterfly_lazy_len2(r, 88, zetas[54]);
  butterfly_lazy_len2(r, 92, zetas[55]);
  butterfly_lazy_len2(r, 96, zetas[56]);
  butterfly_lazy_len2(r, 100, zetas[57]);
  butterfly_lazy_len2(r, 104, zetas[58]);
  butterfly_lazy_len2(r, 108, zetas[59]);
  butterfly_lazy_len2(r, 112, zetas[60]);
  butterfly_lazy_len2(r, 116, zetas[61]);
  butterfly_lazy_len2(r, 120, zetas[62]);
  butterfly_lazy_len2(r, 124, zetas[63]);
  butterfly_lazy_len1_x8(r, 0, &zetas[64]);  butterfly_lazy_len1_x8(r, 16, &zetas[72]);  butterfly_lazy_len1_x8(r, 32, &zetas[80]);  butterfly_lazy_len1_x8(r, 48, &zetas[88]);  butterfly_lazy_len1_x8(r, 64, &zetas[96]);  butterfly_lazy_len1_x8(r, 80, &zetas[104]);  butterfly_lazy_len1_x8(r, 96, &zetas[112]);  butterfly_lazy_len1_x8(r, 112, &zetas[120]);}

void invntt3329_avx128(int16_t r[WEAVER_N])
{
  unsigned start, len, j, k;
  const int16_t f = 1441;

  k = 127;
  for(len = 1; len <= 64; len <<= 1) {
    for(start = 0; start < WEAVER_N; start += 2 * len)
      inv_butterfly_n128(r, start, len, zetas[k--]);
  }

  for(j = 0; j + 16 <= WEAVER_N; j += 16)
    _mm256_storeu_si256((__m256i *)&r[j],
                        fqmul_zeta_x16(_mm256_loadu_si256((__m256i *)&r[j]), f));
  for(; j + 8 <= WEAVER_N; j += 8)
    _mm_storeu_si128((__m128i *)&r[j],
                     fqmul_zeta_x8(_mm_loadu_si128((__m128i *)&r[j]), f));
  for(; j < WEAVER_N; j++)
    r[j] = fqmul_scalar(r[j], f);
}

void basemul3329_avx128(int16_t r[WEAVER_N],
                        const int16_t a[WEAVER_N],
                        const int16_t b[WEAVER_N])
{
  unsigned j;

  for(j = 0; j + 16 <= WEAVER_N; j += 16)
    _mm256_storeu_si256((__m256i *)&r[j],
                        montmul_x16(_mm256_loadu_si256((__m256i *)&a[j]),
                                    _mm256_loadu_si256((__m256i *)&b[j])));
  for(; j + 8 <= WEAVER_N; j += 8)
    _mm_storeu_si128((__m128i *)&r[j],
                     montmul_x8(_mm_loadu_si128((__m128i *)&a[j]),
                                _mm_loadu_si128((__m128i *)&b[j])));
  for(; j < WEAVER_N; j++)
    r[j] = montgomery_reduce((int32_t)a[j] * b[j]);
}

void poly3329_reduce_avx128(int16_t r[WEAVER_N])
{
  unsigned j;

  for(j = 0; j + 16 <= WEAVER_N; j += 16)
    _mm256_storeu_si256((__m256i *)&r[j],
                        barrett_reduce_x16(_mm256_loadu_si256((__m256i *)&r[j])));
  for(; j + 8 <= WEAVER_N; j += 8)
    _mm_storeu_si128((__m128i *)&r[j],
                     barrett_reduce_x8(_mm_loadu_si128((__m128i *)&r[j])));
  for(; j < WEAVER_N; j++)
    r[j] = barrett_reduce(r[j]);
}

#endif
