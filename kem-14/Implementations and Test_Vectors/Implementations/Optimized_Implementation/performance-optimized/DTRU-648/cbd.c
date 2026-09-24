#include <stdint.h>
#include <immintrin.h>
#include "params.h"
#include "cbd.h"

void cbd2(poly *r, const uint8_t buf[DTRU_CBD2_BYTES])
{
  __m256i lut = _mm256_setr_epi8(0, 1, 1, 2, -1, 0, 0, 1, -1, 0, 0, 1, -2, -1, -1, 0,
                                 0, 1, 1, 2, -1, 0, 0, 1, -1, 0, 0, 1, -2, -1, -1, 0);
  __m256i mask_0F = _mm256_set1_epi8(0x0F);
  
  int i;
  for (i = 0; i < 10; i++) {
    __m256i v = _mm256_loadu_si256((const __m256i *)(buf + 32 * i));
    
    __m256i v_low = _mm256_and_si256(v, mask_0F);
    __m256i v_high = _mm256_and_si256(_mm256_srli_epi16(v, 4), mask_0F);
    
    __m256i res_low = _mm256_shuffle_epi8(lut, v_low);
    __m256i res_high = _mm256_shuffle_epi8(lut, v_high);
    
    __m256i res0 = _mm256_unpacklo_epi8(res_low, res_high);
    __m256i res1 = _mm256_unpackhi_epi8(res_low, res_high);
    
    __m256i c0 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(res0));
    __m256i c1 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(res0, 1));
    __m256i c2 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(res1));
    __m256i c3 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(res1, 1));
    
    _mm256_storeu_si256((__m256i *)&r->coeffs[64 * i + 0], c0);
    _mm256_storeu_si256((__m256i *)&r->coeffs[64 * i + 16], c2);
    _mm256_storeu_si256((__m256i *)&r->coeffs[64 * i + 32], c1);
    _mm256_storeu_si256((__m256i *)&r->coeffs[64 * i + 48], c3);
  }
  
  // Remaining 4 bytes -> 8 coefficients (648 - 640 = 8)
  for (i = 320; i < 324; i++) {
    uint8_t t = buf[i];
    r->coeffs[2 * i + 0] = ((t >> 0) & 1) + ((t >> 1) & 1) - ((t >> 2) & 1) - ((t >> 3) & 1);
    r->coeffs[2 * i + 1] = ((t >> 4) & 1) + ((t >> 5) & 1) - ((t >> 6) & 1) - ((t >> 7) & 1);
  }
}


void cbd9(poly *r, const uint8_t buf[DTRU_CBD9_BYTES])
{
  int i, j;
  uint8_t t[9];
  for (i = 0; i < DTRU_N / 4; i++)
  {
    t[0] = buf[9*i+0];
    t[1] = buf[9*i+1];
    t[2] = buf[9*i+2];
    t[3] = buf[9*i+3];
    t[4] = buf[9*i+4];
    t[5] = buf[9*i+5];
    t[6] = buf[9*i+6];
    t[7] = buf[9*i+7];
    t[8] = buf[9*i+8];
    
    for (j = 0; j < 4; j++)
    {
      r->coeffs[4*i+j] = 0;
      r->coeffs[4*i+j] += (t[0] >> (2*j+0)) & 1;
      r->coeffs[4*i+j] += (t[1] >> (2*j+0)) & 1;
      r->coeffs[4*i+j] += (t[2] >> (2*j+0)) & 1;
      r->coeffs[4*i+j] += (t[3] >> (2*j+0)) & 1;
      r->coeffs[4*i+j] += (t[4] >> (2*j+0)) & 1;
      r->coeffs[4*i+j] += (t[5] >> (2*j+0)) & 1;
      r->coeffs[4*i+j] += (t[6] >> (2*j+0)) & 1;
      r->coeffs[4*i+j] += (t[7] >> (2*j+0)) & 1;
      r->coeffs[4*i+j] += (t[8] >> (2*j+0)) & 1;
      
      r->coeffs[4*i+j] -= (t[0] >> (2*j+1)) & 1;
      r->coeffs[4*i+j] -= (t[1] >> (2*j+1)) & 1;
      r->coeffs[4*i+j] -= (t[2] >> (2*j+1)) & 1;
      r->coeffs[4*i+j] -= (t[3] >> (2*j+1)) & 1;
      r->coeffs[4*i+j] -= (t[4] >> (2*j+1)) & 1;
      r->coeffs[4*i+j] -= (t[5] >> (2*j+1)) & 1;
      r->coeffs[4*i+j] -= (t[6] >> (2*j+1)) & 1;
      r->coeffs[4*i+j] -= (t[7] >> (2*j+1)) & 1;
      r->coeffs[4*i+j] -= (t[8] >> (2*j+1)) & 1;
    }
  }
}
