/* AVX2 centered binomial sampling (eta=2,3,7,9). */
#include <stdint.h>
#include <string.h>
#include <immintrin.h>
#include "params.h"
#include "poly.h"
#include "cbd.h"

#if defined(WEAVER_USE_AVX_CBD)

static inline uint64_t load7_le(const uint8_t *p)
{
  return (uint64_t)p[0]
       | ((uint64_t)p[1] << 8)
       | ((uint64_t)p[2] << 16)
       | ((uint64_t)p[3] << 24)
       | ((uint64_t)p[4] << 32)
       | ((uint64_t)p[5] << 40)
       | ((uint64_t)p[6] << 48);
}

static inline __m128i popcount32_small(__m128i x, unsigned bits)
{
  const __m128i mask55 = _mm_set1_epi32(0x55555555);
  const __m128i mask33 = _mm_set1_epi32(0x33333333);
  const __m128i mask0f = _mm_set1_epi32(0x0F0F0F0F);
  uint32_t m;

  if(bits == 7)
    m = 0x7F;
  else
    m = 0x1FF;

  x = _mm_and_si128(x, _mm_set1_epi32((int)m));
  x = _mm_sub_epi32(x, _mm_and_si128(_mm_srli_epi32(x, 1), mask55));
  x = _mm_add_epi32(_mm_and_si128(x, mask33),
                    _mm_and_si128(_mm_srli_epi32(x, 2), mask33));
  x = _mm_and_si128(_mm_add_epi32(x, _mm_srli_epi32(x, 4)), mask0f);
  return x;
}

static inline __m128i cbd_block_popcount(__m128i v, unsigned half_bits)
{
  __m128i lo, hi, pc_lo, pc_hi, diff;
  uint32_t mask = (half_bits == 7) ? 0x7Fu : 0x1FFu;

  lo = _mm_and_si128(v, _mm_set1_epi32((int)mask));
  hi = _mm_and_si128(_mm_srli_epi32(v, half_bits), _mm_set1_epi32((int)mask));
  pc_lo = popcount32_small(lo, half_bits);
  pc_hi = popcount32_small(hi, half_bits);
  diff = _mm_sub_epi32(pc_lo, pc_hi);
  return _mm_packs_epi32(diff, _mm_setzero_si128());
}

static void cbd2_avx(poly *r, const uint8_t buf[2 * WEAVER_N / 4])
{
  unsigned int i;
  __m256i f0, f1, f2, f3;
  const __m256i mask55 = _mm256_set1_epi32(0x55555555);
  const __m256i mask33 = _mm256_set1_epi32(0x33333333);
  const __m256i mask03 = _mm256_set1_epi32(0x03030303);
  const __m256i mask0F = _mm256_set1_epi32(0x0F0F0F0F);

  for(i = 0; i < WEAVER_N / 64; i++) {
    f0 = _mm256_loadu_si256((const __m256i *)&buf[32 * i]);

    f1 = _mm256_srli_epi16(f0, 1);
    f0 = _mm256_and_si256(mask55, f0);
    f1 = _mm256_and_si256(mask55, f1);
    f0 = _mm256_add_epi8(f0, f1);

    f1 = _mm256_srli_epi16(f0, 2);
    f0 = _mm256_and_si256(mask33, f0);
    f1 = _mm256_and_si256(mask33, f1);
    f0 = _mm256_add_epi8(f0, mask33);
    f0 = _mm256_sub_epi8(f0, f1);

    f1 = _mm256_srli_epi16(f0, 4);
    f0 = _mm256_and_si256(mask0F, f0);
    f1 = _mm256_and_si256(mask0F, f1);
    f0 = _mm256_sub_epi8(f0, mask03);
    f1 = _mm256_sub_epi8(f1, mask03);

    f2 = _mm256_unpacklo_epi8(f0, f1);
    f3 = _mm256_unpackhi_epi8(f0, f1);

    f0 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(f2));
    f1 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(f2, 1));
    f2 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(f3));
    f3 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(f3, 1));

    _mm256_storeu_si256((__m256i *)&r->coeffs[64 * i + 0], f0);
    _mm256_storeu_si256((__m256i *)&r->coeffs[64 * i + 16], f2);
    _mm256_storeu_si256((__m256i *)&r->coeffs[64 * i + 32], f1);
    _mm256_storeu_si256((__m256i *)&r->coeffs[64 * i + 48], f3);
  }
}

static void cbd3_avx(poly *r, const uint8_t buf[3 * WEAVER_N / 4])
{
  unsigned int i;
  __m256i f0, f1, f2, f3;
  const __m256i mask249 = _mm256_set1_epi32(0x249249);
  const __m256i mask6DB = _mm256_set1_epi32(0x6DB6DB);
  const __m256i mask07 = _mm256_set1_epi32(7);
  const __m256i mask70 = _mm256_set1_epi32(7 << 16);
  const __m256i mask3 = _mm256_set1_epi16(3);
  const __m256i shufbidx = _mm256_set_epi8(-1, 15, 14, 13, -1, 12, 11, 10, -1, 9, 8, 7, -1, 6, 5, 4,
                                            -1, 11, 10, 9, -1, 8, 7, 6, -1, 5, 4, 3, -1, 2, 1, 0);

  for(i = 0; i < WEAVER_N / 32; i++) {
    f0 = _mm256_loadu_si256((__m256i *)&buf[24 * i]);
    f0 = _mm256_permute4x64_epi64(f0, 0x94);
    f0 = _mm256_shuffle_epi8(f0, shufbidx);

    f1 = _mm256_srli_epi32(f0, 1);
    f2 = _mm256_srli_epi32(f0, 2);
    f0 = _mm256_and_si256(mask249, f0);
    f1 = _mm256_and_si256(mask249, f1);
    f2 = _mm256_and_si256(mask249, f2);
    f0 = _mm256_add_epi32(f0, f1);
    f0 = _mm256_add_epi32(f0, f2);

    f1 = _mm256_srli_epi32(f0, 3);
    f0 = _mm256_add_epi32(f0, mask6DB);
    f0 = _mm256_sub_epi32(f0, f1);

    f1 = _mm256_slli_epi32(f0, 10);
    f2 = _mm256_srli_epi32(f0, 12);
    f3 = _mm256_srli_epi32(f0, 2);
    f0 = _mm256_and_si256(f0, mask07);
    f1 = _mm256_and_si256(f1, mask70);
    f2 = _mm256_and_si256(f2, mask07);
    f3 = _mm256_and_si256(f3, mask70);
    f0 = _mm256_add_epi16(f0, f1);
    f1 = _mm256_add_epi16(f2, f3);
    f0 = _mm256_sub_epi16(f0, mask3);
    f1 = _mm256_sub_epi16(f1, mask3);

    f2 = _mm256_unpacklo_epi32(f0, f1);
    f3 = _mm256_unpackhi_epi32(f0, f1);

    f0 = _mm256_permute2x128_si256(f2, f3, 0x20);
    f1 = _mm256_permute2x128_si256(f2, f3, 0x31);

    _mm256_storeu_si256((__m256i *)&r->coeffs[32 * i + 0], f0);
    _mm256_storeu_si256((__m256i *)&r->coeffs[32 * i + 16], f1);
  }
}

static void cbd4_avx(poly *r, const uint8_t buf[4 * WEAVER_N / 4])
{
  unsigned int i, k;
  const __m256i mask = _mm256_set1_epi32(0x11111111);
  const __m256i m0f = _mm256_set1_epi32(0x0F);
  int32_t t0[8], t1[8], t2[8], t3[8];

  for(i = 0; i < WEAVER_N / 32; i++) {
    __m256i f0 = _mm256_loadu_si256((const __m256i *)&buf[32 * i]);
    __m256i f1, f2, f3, d;
    __m256i n0, n1, n2, n3, d0, d1, d2, d3;

    f1 = _mm256_srli_epi32(f0, 1);
    f2 = _mm256_srli_epi32(f0, 2);
    f3 = _mm256_srli_epi32(f0, 3);
    f0 = _mm256_and_si256(f0, mask);
    f1 = _mm256_and_si256(f1, mask);
    f2 = _mm256_and_si256(f2, mask);
    f3 = _mm256_and_si256(f3, mask);
    d  = _mm256_add_epi32(_mm256_add_epi32(f0, f1),
                          _mm256_add_epi32(f2, f3));

    n0 = _mm256_and_si256(d, m0f);
    n1 = _mm256_and_si256(_mm256_srli_epi32(d, 4), m0f);
    n2 = _mm256_and_si256(_mm256_srli_epi32(d, 8), m0f);
    n3 = _mm256_and_si256(_mm256_srli_epi32(d, 12), m0f);
    d0 = _mm256_sub_epi32(n0, n1);
    d1 = _mm256_sub_epi32(n2, n3);
    n0 = _mm256_and_si256(_mm256_srli_epi32(d, 16), m0f);
    n1 = _mm256_and_si256(_mm256_srli_epi32(d, 20), m0f);
    n2 = _mm256_and_si256(_mm256_srli_epi32(d, 24), m0f);
    n3 = _mm256_and_si256(_mm256_srli_epi32(d, 28), m0f);
    d2 = _mm256_sub_epi32(n0, n1);
    d3 = _mm256_sub_epi32(n2, n3);

    _mm256_storeu_si256((__m256i *)t0, d0);
    _mm256_storeu_si256((__m256i *)t1, d1);
    _mm256_storeu_si256((__m256i *)t2, d2);
    _mm256_storeu_si256((__m256i *)t3, d3);

    for(k = 0; k < 8; k++) {
      r->coeffs[32 * i + 4 * k + 0] = (int16_t)t0[k];
      r->coeffs[32 * i + 4 * k + 1] = (int16_t)t1[k];
      r->coeffs[32 * i + 4 * k + 2] = (int16_t)t2[k];
      r->coeffs[32 * i + 4 * k + 3] = (int16_t)t3[k];
    }
  }
}

static void cbd8_avx(poly *r, const uint8_t buf[8 * WEAVER_N / 4])
{
  unsigned int i, k;
  const __m256i mask = _mm256_set1_epi32(0x01010101);
  int32_t t0[8], t1[8];

  for(i = 0; i < WEAVER_N / 16; i++) {
    __m256i f0 = _mm256_loadu_si256((const __m256i *)&buf[32 * i]);
    __m256i f1, f2, f3, f4, f5, f6, f7, d;
    __m256i lo, hi, d0, d1;

    f1 = _mm256_srli_epi32(f0, 1);
    f2 = _mm256_srli_epi32(f0, 2);
    f3 = _mm256_srli_epi32(f0, 3);
    f4 = _mm256_srli_epi32(f0, 4);
    f5 = _mm256_srli_epi32(f0, 5);
    f6 = _mm256_srli_epi32(f0, 6);
    f7 = _mm256_srli_epi32(f0, 7);
    f0 = _mm256_and_si256(f0, mask);
    f1 = _mm256_and_si256(f1, mask);
    f2 = _mm256_and_si256(f2, mask);
    f3 = _mm256_and_si256(f3, mask);
    f4 = _mm256_and_si256(f4, mask);
    f5 = _mm256_and_si256(f5, mask);
    f6 = _mm256_and_si256(f6, mask);
    f7 = _mm256_and_si256(f7, mask);
    d  = _mm256_add_epi32(f0, f1);
    d  = _mm256_add_epi32(d, f2);
    d  = _mm256_add_epi32(d, f3);
    d  = _mm256_add_epi32(d, f4);
    d  = _mm256_add_epi32(d, f5);
    d  = _mm256_add_epi32(d, f6);
    d  = _mm256_add_epi32(d, f7);

    lo = _mm256_and_si256(d, _mm256_set1_epi32(0xFF));
    hi = _mm256_and_si256(_mm256_srli_epi32(d, 8), _mm256_set1_epi32(0xFF));
    d0 = _mm256_sub_epi32(lo, hi);
    lo = _mm256_and_si256(_mm256_srli_epi32(d, 16), _mm256_set1_epi32(0xFF));
    hi = _mm256_and_si256(_mm256_srli_epi32(d, 24), _mm256_set1_epi32(0xFF));
    d1 = _mm256_sub_epi32(lo, hi);

    _mm256_storeu_si256((__m256i *)t0, d0);
    _mm256_storeu_si256((__m256i *)t1, d1);

    for(k = 0; k < 8; k++) {
      r->coeffs[16 * i + 2 * k + 0] = (int16_t)t0[k];
      r->coeffs[16 * i + 2 * k + 1] = (int16_t)t1[k];
    }
  }
}

static inline uint64_t load5_le(const uint8_t *p)
{
  return (uint64_t)p[0]
       | ((uint64_t)p[1] << 8)
       | ((uint64_t)p[2] << 16)
       | ((uint64_t)p[3] << 24)
       | ((uint64_t)p[4] << 32);
}

static inline __m128i popcount5_4x32(__m128i x)
{
  const __m128i mask1 = _mm_set1_epi32(0x55555555);
  const __m128i mask3 = _mm_set1_epi32(0x33333333);
  const __m128i mask7 = _mm_set1_epi32(0x07070707);

  x = _mm_and_si128(x, _mm_set1_epi32(0x1F));
  x = _mm_sub_epi32(x, _mm_and_si128(_mm_srli_epi32(x, 1), mask1));
  x = _mm_add_epi32(_mm_and_si128(x, mask3),
                    _mm_and_si128(_mm_srli_epi32(x, 2), mask3));
  x = _mm_and_si128(_mm_add_epi32(x, _mm_srli_epi32(x, 4)), mask7);
  return x;
}

static inline __m128i cbd10_from_v4(__m128i v)
{
  const __m128i mask1f = _mm_set1_epi32(0x1F);
  __m128i a, b;

  v = _mm_and_si128(v, _mm_set1_epi32(0xFFFFF));
  a = _mm_add_epi32(popcount5_4x32(_mm_and_si128(v, mask1f)),
                    popcount5_4x32(_mm_and_si128(_mm_srli_epi32(v, 5), mask1f)));
  b = _mm_add_epi32(popcount5_4x32(_mm_and_si128(_mm_srli_epi32(v, 10), mask1f)),
                    popcount5_4x32(_mm_and_si128(_mm_srli_epi32(v, 15), mask1f)));
  return _mm_packs_epi32(_mm_sub_epi32(a, b), _mm_setzero_si128());
}

static inline void cbd10_store8(int16_t *out, __m128i o0, __m128i o1)
{
  int16_t a[4], b[4];
  unsigned int k;

  _mm_storel_epi64((__m128i *)a, o0);
  _mm_storel_epi64((__m128i *)b, o1);
  for(k = 0; k < 4; k++) {
    out[2 * k + 0] = a[k];
    out[2 * k + 1] = b[k];
  }
}

static void cbd10_avx(poly *r, const uint8_t buf[10 * WEAVER_N / 4])
{
  unsigned int i;

  for(i = 0; i < WEAVER_N / 2; i += 4) {
    uint64_t t0 = load5_le(buf + 5 * (i + 0));
    uint64_t t1 = load5_le(buf + 5 * (i + 1));
    uint64_t t2 = load5_le(buf + 5 * (i + 2));
    uint64_t t3 = load5_le(buf + 5 * (i + 3));
    __m128i v0, v1;

    v0 = _mm_set_epi32((int)(t3 & 0xFFFFFu), (int)(t2 & 0xFFFFFu),
                       (int)(t1 & 0xFFFFFu), (int)(t0 & 0xFFFFFu));
    v1 = _mm_set_epi32((int)((t3 >> 20) & 0xFFFFFu), (int)((t2 >> 20) & 0xFFFFFu),
                       (int)((t1 >> 20) & 0xFFFFFu), (int)((t0 >> 20) & 0xFFFFFu));
    cbd10_store8(&r->coeffs[2 * i], cbd10_from_v4(v0), cbd10_from_v4(v1));
  }
}

static inline __m128i cbd7_block(uint64_t t)
{
  __m128i v = _mm_set_epi32((int)((t >> 42) & 0x3FFF),
                            (int)((t >> 28) & 0x3FFF),
                            (int)((t >> 14) & 0x3FFF),
                            (int)(t & 0x3FFF));
  return cbd_block_popcount(v, 7);
}

static void cbd7_avx(poly *r, const uint8_t buf[7 * WEAVER_N / 4])
{
  unsigned int i;

  for(i = 0; i < WEAVER_N / 4; i += 4) {
    __m128i o0, o1, o2, o3;

    o0 = cbd7_block(load7_le(buf + 7 * (i + 0)));
    o1 = cbd7_block(load7_le(buf + 7 * (i + 1)));
    o2 = cbd7_block(load7_le(buf + 7 * (i + 2)));
    o3 = cbd7_block(load7_le(buf + 7 * (i + 3)));

    _mm_storel_epi64((__m128i *)&r->coeffs[4 * (i + 0)], o0);
    _mm_storel_epi64((__m128i *)&r->coeffs[4 * (i + 1)], o1);
    _mm_storel_epi64((__m128i *)&r->coeffs[4 * (i + 2)], o2);
    _mm_storel_epi64((__m128i *)&r->coeffs[4 * (i + 3)], o3);
  }
}

static unsigned int popcount_u32_scalar(uint32_t x)
{
  unsigned int r = 0;

  while(x) {
    r += x & 1u;
    x >>= 1;
  }
  return r;
}

static void cbd9_avx(poly *r, const uint8_t buf[9 * WEAVER_N / 4])
{
  unsigned int i, j;

  for(i = 0; i < WEAVER_N / 4; i++) {
    uint64_t t0 = (uint64_t)buf[9 * i + 0]
                | ((uint64_t)buf[9 * i + 1] << 8)
                | ((uint64_t)buf[9 * i + 2] << 16)
                | ((uint64_t)buf[9 * i + 3] << 24)
                | ((uint64_t)buf[9 * i + 4] << 32)
                | ((uint64_t)buf[9 * i + 5] << 40)
                | ((uint64_t)buf[9 * i + 6] << 48)
                | ((uint64_t)buf[9 * i + 7] << 56);
    uint64_t t1 = buf[9 * i + 8];

    for(j = 0; j < 4; j++) {
      uint32_t val;
      unsigned int lo_shift = 18 * j;

      if(lo_shift <= 14)
        val = (uint32_t)((t0 >> lo_shift) & 0x3FFFF);
      else
        val = (uint32_t)(((t0 >> lo_shift) | (t1 << (64 - lo_shift))) & 0x3FFFF);

      r->coeffs[4 * i + j] = (int16_t)((int)popcount_u32_scalar(val & 0x1FF)
                                     - (int)popcount_u32_scalar((val >> 9) & 0x1FF));
    }
  }
}

void cbd_eta1_avx(poly *r, const uint8_t buf[WEAVER_ETA1 * WEAVER_N / 4])
{
#if WEAVER_ETA1 == 2
  cbd2_avx(r, buf);
#elif WEAVER_ETA1 == 3
  cbd3_avx(r, buf);
#elif WEAVER_ETA1 == 4
  cbd4_avx(r, buf);
#elif WEAVER_ETA1 == 7
  cbd7_avx(r, buf);
#elif WEAVER_ETA1 == 8
  cbd8_avx(r, buf);
#elif WEAVER_ETA1 == 9
  cbd9_avx(r, buf);
#elif WEAVER_ETA1 == 10
  cbd10_avx(r, buf);
#else
#error "AVX CBD eta1 not implemented for this mode"
#endif
}

void cbd_eta2_avx(poly *r, const uint8_t buf[WEAVER_ETA2 * WEAVER_N / 4])
{
#if WEAVER_ETA2 == 2
  cbd2_avx(r, buf);
#elif WEAVER_ETA2 == 3
  cbd3_avx(r, buf);
#elif WEAVER_ETA2 == 4
  cbd4_avx(r, buf);
#elif WEAVER_ETA2 == 7
  cbd7_avx(r, buf);
#elif WEAVER_ETA2 == 8
  cbd8_avx(r, buf);
#elif WEAVER_ETA2 == 9
  cbd9_avx(r, buf);
#elif WEAVER_ETA2 == 10
  cbd10_avx(r, buf);
#else
#error "AVX CBD eta2 not implemented for this mode"
#endif
}

#endif /* WEAVER_USE_AVX_CBD */
