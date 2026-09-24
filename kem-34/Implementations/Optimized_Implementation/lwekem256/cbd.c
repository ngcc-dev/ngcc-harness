#include "cbd.h"
#include "params.h"
#include "reduce.h"
#include <stdint.h>
#include <stdio.h>
#include <immintrin.h>

/*************************************************
 * Name:        load32_littleendian
 *
 * Description: load bytes into a 32-bit integer
 *              in little-endian order
 *
 * Arguments:   - const unsigned char *x: pointer to input byte array
 *
 * Returns 32-bit unsigned integer loaded from x
 **************************************************/
static uint32_t
load32_littleendian (const unsigned char *x)
{
	uint32_t r;
	r = (uint32_t)x[0];
	r |= (uint32_t)x[1] << 8;
	r |= (uint32_t)x[2] << 16;
	r |= (uint32_t)x[3] << 24;
	return r;
}

/*************************************************
 * Name:        cbd
 *
 * Description: Given an array of uniformly random bytes, compute
 *              polynomial with coefficients distributed according to
 *              a centered binomial distribution with parameter KEM_ETA
 *
 * Arguments:   - poly *r:                  pointer to output polynomial
 *              - const unsigned char *buf: pointer to input byte array
 **************************************************/
static void
cbd (poly *r, const unsigned char *buf)
{
	// AVX2 implementation of the centered binomial distribution (CBD) function with ETA=1
	
	__m256i f0, f1, f2, f3, f4, f5, f6, f7;
	const __m256i mask55 = _mm256_set1_epi32(0x55555555);
	const __m256i mask0F = _mm256_set1_epi32(0x0f0f0f0f);
	const __m256i maskaa = _mm256_set1_epi32(0xaaaaaaaa);
	const __m256i mask03 = _mm256_set1_epi32(0x03030303);
	const __m256i mask02 = _mm256_set1_epi32(0x02020202);
	const __m256i maskQ = _mm256_set1_epi32(0x0D010D01);

	f0 = _mm256_loadu_si256((__m256i *)buf);

    f1 = _mm256_srli_epi16(f0, 1);
    f0 = _mm256_and_si256(mask55, f0);
    f1 = _mm256_and_si256(mask55, f1);
    
    f0 = _mm256_add_epi8(f0, maskaa);
    f0 = _mm256_sub_epi8(f0, f1);

    f2 = _mm256_srli_epi16(f0, 4);

	f0 = _mm256_and_si256(mask0F, f0);
    f2 = _mm256_and_si256(mask0F, f2);

	
	f1 = _mm256_srli_epi16(f0, 2);
	f3 = _mm256_srli_epi16(f2, 2);

    f0 = _mm256_and_si256(mask03, f0);
    f1 = _mm256_and_si256(mask03, f1);
	f2 = _mm256_and_si256(mask03, f2);
    f3 = _mm256_and_si256(mask03, f3);
	
    f0 = _mm256_sub_epi8(f0, mask02);
    f1 = _mm256_sub_epi8(f1, mask02);
	f2 = _mm256_sub_epi8(f2, mask02);
    f3 = _mm256_sub_epi8(f3, mask02);

	f4 = _mm256_unpacklo_epi8(f0, f1);
    f5 = _mm256_unpackhi_epi8(f0, f1);
    f6 = _mm256_unpacklo_epi8(f2, f3);
    f7 = _mm256_unpackhi_epi8(f2, f3);

    f0 = _mm256_unpacklo_epi16(f4, f6);
    f1 = _mm256_unpackhi_epi16(f4, f6);
    f2 = _mm256_unpacklo_epi16(f5, f7);
    f3 = _mm256_unpackhi_epi16(f5, f7);

	f4 = _mm256_permute2x128_si256(f0, f1, 0x20); // Low halves of v0 and v1
	f5 = _mm256_permute2x128_si256(f0, f1, 0x31); // High halves of v0 and v1
	f6 = _mm256_permute2x128_si256(f2, f3, 0x20); // Low halves of v2 and v3
	f7 = _mm256_permute2x128_si256(f2, f3, 0x31); // High halves of v2 and v3

    f0 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(f4));
	f1 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(f4, 1));
	f2 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(f5));
	f3 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(f5, 1));
	f4 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(f6));
	f5 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(f6, 1));
	f6 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(f7));
	f7 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(f7, 1));

    f0 = _mm256_add_epi16(f0, maskQ);
    f1 = _mm256_add_epi16(f1, maskQ);
    f2 = _mm256_add_epi16(f2, maskQ);
    f3 = _mm256_add_epi16(f3, maskQ);
    f4 = _mm256_add_epi16(f4, maskQ);
    f5 = _mm256_add_epi16(f5, maskQ);
    f6 = _mm256_add_epi16(f6, maskQ);
    f7 = _mm256_add_epi16(f7, maskQ);

    _mm256_storeu_si256((__m256i *)r->coeffs, f0);
    _mm256_storeu_si256((__m256i *)(r->coeffs + 16), f1);
    _mm256_storeu_si256((__m256i *)(r->coeffs + 32), f4);
    _mm256_storeu_si256((__m256i *)(r->coeffs + 48), f5);
    _mm256_storeu_si256((__m256i *)(r->coeffs + 64), f2);
    _mm256_storeu_si256((__m256i *)(r->coeffs + 80), f3);
    _mm256_storeu_si256((__m256i *)(r->coeffs + 96), f6);
    _mm256_storeu_si256((__m256i *)(r->coeffs + 112), f7);

}

void
poly_cbd_eta (poly *r, const uint8_t buf[KEM_ETA * KEM_N / 4])
{
	cbd (r, buf);
}
