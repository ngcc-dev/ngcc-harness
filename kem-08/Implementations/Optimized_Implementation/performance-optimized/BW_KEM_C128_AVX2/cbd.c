#include <immintrin.h>
#include <stdint.h>
#include "params.h"
#include "cbd.h"

static inline __m256i bwkem128_popcount_u16_avx2(__m256i x)
{
  const __m256i nibble_mask = _mm256_set1_epi8(0x0f);
  const __m256i ones = _mm256_set1_epi8(1);
  const __m256i lut = _mm256_setr_epi8(
      0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
      0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4);

  __m256i lo = _mm256_and_si256(x, nibble_mask);
  __m256i hi = _mm256_and_si256(_mm256_srli_epi16(x, 4), nibble_mask);
  __m256i byte_counts = _mm256_add_epi8(_mm256_shuffle_epi8(lut, lo),
                                        _mm256_shuffle_epi8(lut, hi));

  return _mm256_maddubs_epi16(byte_counts, ones);
}

static void bwkem128_load_eta6_words(uint16_t out[KYBER_N],
                                     const uint8_t buf[KYBER_ETA1 * KYBER_N / 4])
{
  unsigned int i;

  for(i = 0; i < KYBER_N / 2; i++) {
    uint32_t t = (uint32_t)buf[3 * i + 0];
    t |= (uint32_t)buf[3 * i + 1] << 8;
    t |= (uint32_t)buf[3 * i + 2] << 16;

    out[2 * i + 0] = (uint16_t)(t & 0x0fffU);
    out[2 * i + 1] = (uint16_t)((t >> 12) & 0x0fffU);
  }
}

static void bwkem128_load_eta5_words(uint16_t out[KYBER_N],
                                     const uint8_t buf[KYBER_ETA2 * KYBER_N / 4])
{
  unsigned int i;

  for(i = 0; i < KYBER_N / 4; i++) {
    uint64_t t = (uint64_t)buf[5 * i + 0];
    t |= (uint64_t)buf[5 * i + 1] << 8;
    t |= (uint64_t)buf[5 * i + 2] << 16;
    t |= (uint64_t)buf[5 * i + 3] << 24;
    t |= (uint64_t)buf[5 * i + 4] << 32;

    out[4 * i + 0] = (uint16_t)(t & 0x03ffU);
    out[4 * i + 1] = (uint16_t)((t >> 10) & 0x03ffU);
    out[4 * i + 2] = (uint16_t)((t >> 20) & 0x03ffU);
    out[4 * i + 3] = (uint16_t)((t >> 30) & 0x03ffU);
  }
}

static void bwkem128_cbd_from_words(poly *r, const uint16_t words[KYBER_N], unsigned int eta)
{
  unsigned int i;
  __m256i mask = _mm256_set1_epi16((short)((1u << eta) - 1u));

  for(i = 0; i < KYBER_N; i += 16) {
    __m256i samples = _mm256_loadu_si256((const __m256i *)(words + i));
    __m256i a = bwkem128_popcount_u16_avx2(_mm256_and_si256(samples, mask));
    __m256i b = bwkem128_popcount_u16_avx2(
        _mm256_and_si256(_mm256_srli_epi16(samples, eta), mask));
    __m256i coeffs = _mm256_sub_epi16(a, b);

    _mm256_storeu_si256((__m256i *)(r->coeffs + i), coeffs);
  }
}

static void cbd_eta6(poly *r, const uint8_t buf[KYBER_ETA1 * KYBER_N / 4])
{
  uint16_t words[KYBER_N];

  bwkem128_load_eta6_words(words, buf);
  bwkem128_cbd_from_words(r, words, 6u);
}

static void cbd_eta5(poly *r, const uint8_t buf[KYBER_ETA2 * KYBER_N / 4])
{
  uint16_t words[KYBER_N];

  bwkem128_load_eta5_words(words, buf);
  bwkem128_cbd_from_words(r, words, 5u);
}

void poly_cbd_eta1(poly *r, const uint8_t buf[KYBER_ETA1*KYBER_N/4])
{
  cbd_eta6(r, buf);
}

void poly_cbd_eta2(poly *r, const uint8_t buf[KYBER_ETA2*KYBER_N/4])
{
  cbd_eta5(r, buf);
}
