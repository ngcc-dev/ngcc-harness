/*
 * q=7681 AVX2 NTT matching ref/src/ntt.c bit-for-bit (Route A).
 * n=256: static unroll; n=512: loop + len=1/2 batch helpers.
 */
#include <stdint.h>
#include <immintrin.h>

#include "params.h"
#include "ntt.h"
#include "reduce.h"
#include "reduce_avx.h"
#include "ntt7681_avx.h"

#if defined(WEAVER_USE_AVX_NTT7681) && (WEAVER_Q == 7681) && \
    (WEAVER_N == 256 || WEAVER_N == 512)

static __attribute__((always_inline)) inline int16_t fqmul_scalar(int16_t a, int16_t b)
{
  return montgomery_reduce((int32_t)a * b);
}

static __attribute__((always_inline)) inline __m256i vq256(void)
{
  return _mm256_set1_epi16(WEAVER_Q);
}

static __attribute__((always_inline)) inline __m256i vqinv256(void)
{
  return _mm256_set1_epi16(QINV);
}

static __attribute__((always_inline)) inline __m128i vq128(void)
{
  return _mm_set1_epi16(WEAVER_Q);
}

static __attribute__((always_inline)) inline __m128i vqinv128(void)
{
  return _mm_set1_epi16(QINV);
}

static __attribute__((always_inline)) inline __m256i montmul_x16(__m256i a, __m256i b)
{
  __m256i l = _mm256_mullo_epi16(a, b);
  __m256i h = _mm256_mulhi_epi16(a, b);
  __m256i t = _mm256_mullo_epi16(l, vqinv256());

  t = _mm256_mulhi_epi16(t, vq256());
  return _mm256_sub_epi16(h, t);
}

static __attribute__((always_inline)) inline __m128i montmul_x8(__m128i a, __m128i b)
{
  __m128i l = _mm_mullo_epi16(a, b);
  __m128i h = _mm_mulhi_epi16(a, b);
  __m128i t = _mm_mullo_epi16(l, vqinv128());

  t = _mm_mulhi_epi16(t, vq128());
  return _mm_sub_epi16(h, t);
}

static __attribute__((always_inline)) inline __m256i fqmul_zeta_x16(__m256i a, int16_t zeta)
{
  return montmul_x16(a, _mm256_set1_epi16(zeta));
}

static __attribute__((always_inline)) inline __m128i fqmul_zeta_x8(__m128i a, int16_t zeta)
{
  return montmul_x8(a, _mm_set1_epi16(zeta));
}

static __attribute__((always_inline)) inline void
bf_lazy(int16_t *r, unsigned start, unsigned len, int16_t zeta)
{
  unsigned j;

  if(len == 4) {
    for(j = start; j < start + len; j++) {
      int16_t t = fqmul_scalar(zeta, r[j + len]);

      r[j + len] = r[j] - t;
      r[j] = r[j] + t;
    }
    return;
  }

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

static __attribute__((always_inline)) inline void
bf_full(int16_t *r, unsigned start, unsigned len, int16_t zeta)
{
  unsigned j;

  if(len == 4) {
    for(j = start; j < start + len; j++) {
      int16_t t = fqmul_scalar(zeta, r[j + len]);

      r[j + len] = barrett_reduce(r[j] - t);
      r[j] = barrett_reduce(r[j] + t);
    }
    return;
  }

  for(j = start; j + 16 <= start + len; j += 16) {
    __m256i u = _mm256_loadu_si256((__m256i *)&r[j]);
    __m256i v = _mm256_loadu_si256((__m256i *)&r[j + len]);
    __m256i t = fqmul_zeta_x16(v, zeta);

    _mm256_storeu_si256((__m256i *)&r[j + len], barrett_reduce_x16(_mm256_sub_epi16(u, t)));
    _mm256_storeu_si256((__m256i *)&r[j], barrett_reduce_x16(_mm256_add_epi16(u, t)));
  }

  for(; j + 8 <= start + len; j += 8) {
    __m128i u = _mm_loadu_si128((__m128i *)&r[j]);
    __m128i v = _mm_loadu_si128((__m128i *)&r[j + len]);
    __m128i t = fqmul_zeta_x8(v, zeta);

    _mm_storeu_si128((__m128i *)&r[j + len], barrett_reduce_x8(_mm_sub_epi16(u, t)));
    _mm_storeu_si128((__m128i *)&r[j], barrett_reduce_x8(_mm_add_epi16(u, t)));
  }

  for(; j < start + len; j++) {
    int16_t t = fqmul_scalar(zeta, r[j + len]);

    r[j + len] = barrett_reduce(r[j] - t);
    r[j] = barrett_reduce(r[j] + t);
  }
}

static __attribute__((always_inline)) inline void
bf_lazy_len2_all(int16_t *r, const int16_t *z, unsigned count)
{
  unsigned i;

  for(i = 0; i < count; i++) {
    unsigned j = 4 * i;
    int16_t t = fqmul_scalar(z[i], r[j + 2]);

    r[j + 2] = r[j] - t;
    r[j] = r[j] + t;
    t = fqmul_scalar(z[i], r[j + 3]);
    r[j + 3] = r[j + 1] - t;
    r[j + 1] = r[j + 1] + t;
  }
}

static __attribute__((always_inline)) inline void
bf_full_len1_all(int16_t *r, const int16_t *z, unsigned count)
{
  unsigned i;
  const __m256i mask16 = _mm256_set1_epi32(0xFFFF);

  for(i = 0; i + 8 <= count; i += 8) {
    unsigned j = 2 * i;
    __m256i x = _mm256_loadu_si256((__m256i *)&r[j]);
    __m256i odds_w = _mm256_srli_epi32(x, 16);
    __m256i evens_w = _mm256_and_si256(x, mask16);
    __m128i ev = _mm_packus_epi32(_mm256_castsi256_si128(evens_w),
                                  _mm256_extracti128_si256(evens_w, 1));
    __m128i od = _mm_packus_epi32(_mm256_castsi256_si128(odds_w),
                                  _mm256_extracti128_si256(odds_w, 1));
    __m128i z8 = _mm_loadu_si128((__m128i *)&z[i]);
    __m128i t = montmul_x8(od, z8);
    __m128i new_od = barrett_reduce_x8(_mm_sub_epi16(ev, t));
    __m128i new_ev = barrett_reduce_x8(_mm_add_epi16(ev, t));

    _mm_storeu_si128((__m128i *)&r[j], _mm_unpacklo_epi16(new_ev, new_od));
    _mm_storeu_si128((__m128i *)&r[j + 8], _mm_unpackhi_epi16(new_ev, new_od));
  }

  for(; i < count; i++) {
    unsigned j = 2 * i;
    int16_t t = fqmul_scalar(z[i], r[j + 1]);

    r[j + 1] = barrett_reduce(r[j] - t);
    r[j] = barrett_reduce(r[j] + t);
  }
}

static __attribute__((always_inline)) inline void
bf_inv_lazy(int16_t *r, unsigned start, unsigned len, int16_t zeta)
{
  unsigned j;

  if(len == 4) {
    for(j = start; j < start + len; j++) {
      int16_t t = r[j];

      r[j] = t + r[j + len];
      r[j + len] = r[j + len] - t;
      r[j + len] = fqmul_scalar(zeta, r[j + len]);
    }
    return;
  }

  for(j = start; j + 16 <= start + len; j += 16) {
    __m256i u = _mm256_loadu_si256((__m256i *)&r[j]);
    __m256i v = _mm256_loadu_si256((__m256i *)&r[j + len]);
    __m256i sum = _mm256_add_epi16(u, v);
    __m256i diff = _mm256_sub_epi16(v, u);
    __m256i t = fqmul_zeta_x16(diff, zeta);

    _mm256_storeu_si256((__m256i *)&r[j], sum);
    _mm256_storeu_si256((__m256i *)&r[j + len], t);
  }

  for(; j + 8 <= start + len; j += 8) {
    __m128i u = _mm_loadu_si128((__m128i *)&r[j]);
    __m128i v = _mm_loadu_si128((__m128i *)&r[j + len]);
    __m128i sum = _mm_add_epi16(u, v);
    __m128i diff = _mm_sub_epi16(v, u);
    __m128i t = fqmul_zeta_x8(diff, zeta);

    _mm_storeu_si128((__m128i *)&r[j], sum);
    _mm_storeu_si128((__m128i *)&r[j + len], t);
  }

  for(; j < start + len; j++) {
    int16_t t = r[j];

    r[j] = t + r[j + len];
    r[j + len] = r[j + len] - t;
    r[j + len] = fqmul_scalar(zeta, r[j + len]);
  }
}

static __attribute__((always_inline)) inline void
bf_inv_full(int16_t *r, unsigned start, unsigned len, int16_t zeta)
{
  unsigned j;

  if(len == 4) {
    for(j = start; j < start + len; j++) {
      int16_t t = r[j];

      r[j] = barrett_reduce(t + r[j + len]);
      r[j + len] = barrett_reduce(r[j + len] - t);
      r[j + len] = fqmul_scalar(zeta, r[j + len]);
    }
    return;
  }

  for(j = start; j + 16 <= start + len; j += 16) {
    __m256i u = _mm256_loadu_si256((__m256i *)&r[j]);
    __m256i v = _mm256_loadu_si256((__m256i *)&r[j + len]);
    __m256i sum = _mm256_add_epi16(u, v);
    __m256i diff = _mm256_sub_epi16(v, u);

    _mm256_storeu_si256((__m256i *)&r[j], barrett_reduce_x16(sum));
    _mm256_storeu_si256((__m256i *)&r[j + len], fqmul_zeta_x16(barrett_reduce_x16(diff), zeta));
  }

  for(; j + 8 <= start + len; j += 8) {
    __m128i u = _mm_loadu_si128((__m128i *)&r[j]);
    __m128i v = _mm_loadu_si128((__m128i *)&r[j + len]);
    __m128i sum = _mm_add_epi16(u, v);
    __m128i diff = _mm_sub_epi16(v, u);

    _mm_storeu_si128((__m128i *)&r[j], barrett_reduce_x8(sum));
    _mm_storeu_si128((__m128i *)&r[j + len], fqmul_zeta_x8(barrett_reduce_x8(diff), zeta));
  }

  for(; j < start + len; j++) {
    int16_t t = r[j];

    r[j] = barrett_reduce(t + r[j + len]);
    r[j + len] = barrett_reduce(r[j + len] - t);
    r[j + len] = fqmul_scalar(zeta, r[j + len]);
  }
}

static __attribute__((always_inline)) inline void
bf_inv_lazy_len1_all(int16_t *r, const int16_t *z, unsigned count)
{
  unsigned i;

  for(i = 0; i < count; i++) {
    unsigned j = 2 * i;
    int16_t t = r[j];
    int16_t zeta = z[count - 1 - i];

    r[j] = t + r[j + 1];
    r[j + 1] = r[j + 1] - t;
    r[j + 1] = fqmul_scalar(zeta, r[j + 1]);
  }
}

static __attribute__((always_inline)) inline void
bf_inv_full_len2_all(int16_t *r, const int16_t *z, unsigned count)
{
  unsigned i;

  for(i = 0; i < count; i++) {
    unsigned j = 4 * i;
    int16_t zeta = z[count - 1 - i];
    int16_t t0 = r[j];
    int16_t t1 = r[j + 1];

    r[j] = barrett_reduce(t0 + r[j + 2]);
    r[j + 2] = barrett_reduce(r[j + 2] - t0);
    r[j + 2] = fqmul_scalar(zeta, r[j + 2]);
    r[j + 1] = barrett_reduce(t1 + r[j + 3]);
    r[j + 3] = barrett_reduce(r[j + 3] - t1);
    r[j + 3] = fqmul_scalar(zeta, r[j + 3]);
  }
}

static __attribute__((always_inline)) inline void
invntt7681_scale(int16_t *r)
{
  unsigned j;
  const int16_t f = 1912;

  for(j = 0; j + 16 <= WEAVER_N; j += 16)
    _mm256_storeu_si256((__m256i *)&r[j],
                        fqmul_zeta_x16(_mm256_loadu_si256((__m256i *)&r[j]), f));
  for(; j + 8 <= WEAVER_N; j += 8)
    _mm_storeu_si128((__m128i *)&r[j],
                     fqmul_zeta_x8(_mm_loadu_si128((__m128i *)&r[j]), f));
  for(; j < WEAVER_N; j++)
    r[j] = fqmul_scalar(r[j], f);
}

#if WEAVER_N == 256 && !defined(WEAVER_USE_AVX_NTT7681_ASM)
#include "ntt7681_avx256.inc"
#endif

#if defined(WEAVER_USE_AVX_NTT7681_ASM) && (WEAVER_N == 256)
#define ntt7681_asm256 WEAVER_NAMESPACE(_ntt7681_asm256)
#define invntt7681_asm256 WEAVER_NAMESPACE(_invntt7681_asm256)
void ntt7681_asm256(int16_t *r, const int16_t *zetas);
void invntt7681_asm256(int16_t *r, const int16_t *zetas);

#define ntt7681_tail_lazy_len2 WEAVER_NAMESPACE(_ntt7681_tail_lazy_len2)
#define ntt7681_tail_full_len1 WEAVER_NAMESPACE(_ntt7681_tail_full_len1)
#define ntt7681_tail_inv_lazy_len1 WEAVER_NAMESPACE(_ntt7681_tail_inv_lazy_len1)
#define ntt7681_tail_inv_full_len2 WEAVER_NAMESPACE(_ntt7681_tail_inv_full_len2)
#define ntt7681_tail_scale WEAVER_NAMESPACE(_ntt7681_tail_scale)
void ntt7681_tail_lazy_len2(int16_t *r, const int16_t *z, unsigned count);
void ntt7681_tail_full_len1(int16_t *r, const int16_t *z, unsigned count);
void ntt7681_tail_inv_lazy_len1(int16_t *r, const int16_t *z, unsigned count);
void ntt7681_tail_inv_full_len2(int16_t *r, const int16_t *z, unsigned count);
void ntt7681_tail_scale(int16_t *r);
#endif

void ntt7681_avx(int16_t r[WEAVER_N])
{
#if defined(WEAVER_USE_AVX_NTT7681_ASM) && (WEAVER_N == 256)
  ntt7681_asm256(r, zetas);
#elif WEAVER_N == 256
  ntt7681_avx256(r);
#else
  unsigned int len, start, i, k;

  k = 1;
  for(i = 9; i > 1; i -= 2) {
    len = 1U << (i - 1);
    if(len == 2) {
      bf_lazy_len2_all(r, &zetas[k], WEAVER_N / 4);
      k += WEAVER_N / 4;
    } else {
      for(start = 0; start < WEAVER_N; start += 2 * len)
        bf_lazy(r, start, len, zetas[k++]);
    }
    len >>= 1;
    if(len == 1) {
      bf_full_len1_all(r, &zetas[k], WEAVER_N / 2);
      k += WEAVER_N / 2;
    } else {
      for(start = 0; start < WEAVER_N; start += 2 * len)
        bf_full(r, start, len, zetas[k++]);
    }
  }
#endif
}

void invntt7681_avx(int16_t r[WEAVER_N])
{
#if defined(WEAVER_USE_AVX_NTT7681_ASM) && (WEAVER_N == 256)
  invntt7681_asm256(r, zetas);
#elif WEAVER_N == 256
  invntt7681_avx256(r);
#else
  unsigned int start, len, i, k;

  k = 255;
  for(i = 1; i < 9; i += 2) {
    len = 1U << i;
    for(start = 0; start < WEAVER_N; start += 2 * len)
      bf_inv_lazy(r, start, len, zetas[k--]);
    len <<= 1;
    if(len == 2) {
      bf_inv_full_len2_all(r, &zetas[k - (WEAVER_N / 4 - 1)], WEAVER_N / 4);
      k -= WEAVER_N / 4;
    } else {
      for(start = 0; start < WEAVER_N; start += 2 * len)
        bf_inv_full(r, start, len, zetas[k--]);
    }
  }
  invntt7681_scale(r);
#endif
}

#if WEAVER_N == 256

void basemul7681_avx(int16_t r[WEAVER_N],
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

#elif WEAVER_N == 512

static void basemul2(int16_t r[2],
                     const int16_t a[2],
                     const int16_t b[2],
                     int16_t zeta)
{
  r[0] = fqmul_scalar(a[1], b[1]);
  r[0] = fqmul_scalar(r[0], zeta);
  r[0] += fqmul_scalar(a[0], b[0]);
  r[1] = fqmul_scalar(a[0], b[1]);
  r[1] += fqmul_scalar(a[1], b[0]);
}

void basemul7681_avx(int16_t r[WEAVER_N],
                     const int16_t a[WEAVER_N],
                     const int16_t b[WEAVER_N])
{
  unsigned i;

  for(i = 0; i < WEAVER_N / 4; i++) {
    basemul2(&r[4 * i], &a[4 * i], &b[4 * i], zetas[128 + i]);
    basemul2(&r[4 * i + 2], &a[4 * i + 2], &b[4 * i + 2], (int16_t)-zetas[128 + i]);
  }
}

#endif

void poly7681_reduce_avx(int16_t r[WEAVER_N])
{
  unsigned j;

  for(j = 0; j + 16 <= WEAVER_N; j += 16)
    _mm256_storeu_si256((__m256i *)&r[j],
                        barrett_reduce_x16(_mm256_loadu_si256((__m256i *)&r[j])));
  for(; j + 8 <= WEAVER_N; j += 8)
    _mm_storeu_si128((__m128i *)&r[j], barrett_reduce_x8(_mm_loadu_si128((__m128i *)&r[j])));
  for(; j < WEAVER_N; j++)
    r[j] = barrett_reduce(r[j]);
}

#if defined(WEAVER_USE_AVX_NTT7681_ASM) && (WEAVER_N == 256)

void ntt7681_tail_lazy_len2(int16_t *r, const int16_t *z, unsigned count)
{
  bf_lazy_len2_all(r, z, count);
}

void ntt7681_tail_full_len1(int16_t *r, const int16_t *z, unsigned count)
{
  bf_full_len1_all(r, z, count);
}

void ntt7681_tail_inv_lazy_len1(int16_t *r, const int16_t *z, unsigned count)
{
  bf_inv_lazy_len1_all(r, z, count);
}

void ntt7681_tail_inv_full_len2(int16_t *r, const int16_t *z, unsigned count)
{
  bf_inv_full_len2_all(r, z, count);
}

void ntt7681_tail_scale(int16_t *r)
{
  invntt7681_scale(r);
}

#endif

#endif
