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
#elif WEAVER_ETA1 == 7
  cbd7_avx(r, buf);
#elif WEAVER_ETA1 == 9
  cbd9_avx(r, buf);
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
#elif WEAVER_ETA2 == 7
  cbd7_avx(r, buf);
#elif WEAVER_ETA2 == 9
  cbd9_avx(r, buf);
#else
#error "AVX CBD eta2 not implemented for this mode"
#endif
}

#endif /* WEAVER_USE_AVX_CBD */
