#include "cbd.h"
#include "params.h"
#include "reduce.h"
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>

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
	const __m256i mask55 = _mm256_set1_epi32(0x55555555);
	const __m256i mask33 = _mm256_set1_epi32(0x33333333);
	const __m256i mask03 = _mm256_set1_epi32(0x03030303);
	const __m256i mask0F = _mm256_set1_epi32(0x0F0F0F0F);
	const __m256i maskQ = _mm256_set1_epi32(((uint32_t)KEM_Q << 16) | (uint32_t)KEM_Q);

	for (unsigned int block = 0; block < KEM_N / 64; ++block)
		{
			__m256i f0, f1, f2, f3;

			f0 = _mm256_loadu_si256((const __m256i *)(buf + 32 * block));

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

			f0 = _mm256_add_epi16(f0, maskQ);
			f1 = _mm256_add_epi16(f1, maskQ);
			f2 = _mm256_add_epi16(f2, maskQ);
			f3 = _mm256_add_epi16(f3, maskQ);

			_mm256_storeu_si256((__m256i *)(r->coeffs + 64 * block), f0);
			_mm256_storeu_si256((__m256i *)(r->coeffs + 64 * block + 16), f2);
			_mm256_storeu_si256((__m256i *)(r->coeffs + 64 * block + 32), f1);
			_mm256_storeu_si256((__m256i *)(r->coeffs + 64 * block + 48), f3);
		}
}

void
poly_cbd_eta (poly *r, const uint8_t buf[KEM_ETA * KEM_N / 4])
{
	cbd (r, buf);
}
