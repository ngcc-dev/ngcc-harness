#include <immintrin.h>
#include <stdint.h>
#include "params.h"
#include "cbd.h"

static inline __m256i cbd2_nibble_diff_avx2(__m256i bytes)
{
  const __m256i nibble_mask = _mm256_set1_epi8(0x0f);
  const __m256i lut = _mm256_setr_epi8(
      0, 1, 1, 2, -1, 0, 0, 1, -1, 0, 0, 1, -2, -1, -1, 0,
      0, 1, 1, 2, -1, 0, 0, 1, -1, 0, 0, 1, -2, -1, -1, 0);

  return _mm256_shuffle_epi8(lut, _mm256_and_si256(bytes, nibble_mask));
}

static void cbd2(poly *r, const uint8_t buf[2 * KYBER_N / 4])
{
  unsigned int i;

  for(i = 0; i < KYBER_N / 2; i += 32) {
    __m256i bytes = _mm256_loadu_si256((const __m256i *)(buf + i));
    __m256i lo = cbd2_nibble_diff_avx2(bytes);
    __m256i hi = cbd2_nibble_diff_avx2(_mm256_srli_epi16(bytes, 4));
    __m128i lo0 = _mm256_castsi256_si128(lo);
    __m128i hi0 = _mm256_castsi256_si128(hi);
    __m128i lo1 = _mm256_extracti128_si256(lo, 1);
    __m128i hi1 = _mm256_extracti128_si256(hi, 1);
    __m128i inter0 = _mm_unpacklo_epi8(lo0, hi0);
    __m128i inter1 = _mm_unpackhi_epi8(lo0, hi0);
    __m128i inter2 = _mm_unpacklo_epi8(lo1, hi1);
    __m128i inter3 = _mm_unpackhi_epi8(lo1, hi1);

    _mm256_storeu_si256((__m256i *)(r->coeffs + 2 * i + 0),
                        _mm256_cvtepi8_epi16(inter0));
    _mm256_storeu_si256((__m256i *)(r->coeffs + 2 * i + 16),
                        _mm256_cvtepi8_epi16(inter1));
    _mm256_storeu_si256((__m256i *)(r->coeffs + 2 * i + 32),
                        _mm256_cvtepi8_epi16(inter2));
    _mm256_storeu_si256((__m256i *)(r->coeffs + 2 * i + 48),
                        _mm256_cvtepi8_epi16(inter3));
  }
}

void poly_cbd_eta1(poly *r, const uint8_t buf[KYBER_ETA1*KYBER_N/4])
{
#if KYBER_ETA1 == 2
  cbd2(r, buf);
#else
#error "This C512 AVX2 implementation requires eta1 == 2"
#endif
}

void poly_cbd_eta2(poly *r, const uint8_t buf[KYBER_ETA2*KYBER_N/4])
{
#if KYBER_ETA2 == 2
  cbd2(r, buf);
#else
#error "This implementation requires eta2 = 2"
#endif
}
