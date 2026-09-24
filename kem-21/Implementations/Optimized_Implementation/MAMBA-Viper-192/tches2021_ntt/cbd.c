#include <stdint.h>
#include <immintrin.h>
#include "params.h"
#include "cbd.h"

void cbd(int16_t r[KEM_N], const uint8_t buf[KEM_N])
{
  unsigned int i;
  __m256i f0, f1;
  const __m256i mask55 = _mm256_set1_epi32(0x55555555);
  const __m256i mask33 = _mm256_set1_epi32(0x33333333);
  const __m256i mask0F = _mm256_set1_epi32(0x0F0F0F0F);

  for(i = 0; i < (KEM_N+31)/32; i++) {
    f0 = _mm256_load_si256((__m256i *)&buf[32*i]);

    f1 = _mm256_srli_epi32(f0, 1);
    f0 = _mm256_and_si256(mask55, f0);
    f1 = _mm256_and_si256(mask55, f1);
    f0 = _mm256_add_epi32(f0, f1);

    f1 = _mm256_srli_epi32(f0, 2);
    f0 = _mm256_and_si256(mask33, f0);
    f1 = _mm256_and_si256(mask33, f1);
    f0 = _mm256_add_epi32(f0, f1);

    f1 = _mm256_srli_epi32(f0, 4);
    f0 = _mm256_and_si256(mask0F, f0);
    f1 = _mm256_and_si256(mask0F, f1);
    f1 = _mm256_sub_epi8(f0, f1);

    f0 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(f1));
    f1 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(f1,1));

    _mm256_store_si256((__m256i *)&r[32*i+0], f0);
    _mm256_store_si256((__m256i *)&r[32*i+16], f1);
  }
}
