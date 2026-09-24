#include <stdio.h>
#include <string.h>
#include "polyvec.h"
#include <immintrin.h>
#include "cbd.h"
#include "hashkdf.h"
#include "genmatrix.h"
#include "api.h"

extern const uint16_t chac[];

static void storeu128_partial(uint8_t *r, __m128i v, size_t n)
{
	uint8_t tmp[16];
	_mm_storeu_si128((__m128i *)tmp, v);
	memcpy(r, tmp, n);
}

void poly_decompress10(poly *r, const uint8_t *a) {
	int i;
	const size_t cpbytes = (10 * PARAM_N) >> 3;
	__m256i d0, d1;
	const __m256i q16x = _mm256_set1_epi16(PARAM_Q);
	const __m256i permu = _mm256_set_epi32(5, 4, 3, 2, 3, 2, 1, 0);
	const __m256i mask16 = _mm256_set1_epi16(1023 << 5);
	const __m256i idx8 = _mm256_set_epi8(11, 10, 9, 0, 9, 8, 7, 0, 6, 5, 4, 0, 4, 3, 2, 0, 9, 8, 7, 0, 7, 6, 5, 0, 4, 3,
	                                     2, 0, 2, 1, 0, 0);
	const __m256i shift = _mm256_set_epi32(7, 3, 7, 3, 7, 3, 7, 3);

	for (i = 0; i < PARAM_N / 16; i++) {
		d0 = _mm256_loadu_si256((__m256i *) &a[20 * i]);
		d1 = _mm256_permutevar8x32_epi32(d0, permu);
		d1 = _mm256_shuffle_epi8(d1, idx8);

		d1 = _mm256_srlv_epi32(d1, shift);
		d0 = _mm256_slli_epi32(d1, 6);
		d1 = _mm256_blend_epi16(d0, d1, 0x55);
		d1 = _mm256_and_si256(d1, mask16);

		d0 = _mm256_mulhrs_epi16(d1, q16x);
		_mm256_storeu_si256((__m256i *) &r->coeffs[16 * i], d0);
	}
}

void polyvec_decompress10(polyvec *r, const uint8_t *a)
{
	int i, j;
	uint16_t cpbytes = ((PARAM_N * 10) >> 3);//the bytes for storing a polynomial in compressed form
	__m256i d0, d1;
	const __m256i q16x = _mm256_set1_epi16(PARAM_Q);
	const __m256i permu = _mm256_set_epi32(5, 4, 3, 2, 3, 2, 1, 0);
	const __m256i mask16 = _mm256_set1_epi16(1023<<5);
	const __m256i idx8 = _mm256_set_epi8(11, 10, 9, 0, 9, 8, 7, 0,
		6, 5, 4, 0, 4, 3, 2, 0,
		9, 8, 7, 0, 7, 6, 5, 0,
		4, 3, 2, 0, 2, 1, 0, 0);
	const __m256i shift = _mm256_set_epi32(7, 3, 7, 3, 7, 3, 7, 3);

	for (j = 0; j < PARAM_K; j++)
	{
		for (i = 0; i < PARAM_N / 16; i++)
		{
			d0 = _mm256_loadu_si256((__m256i *)&a[20 * i]);
			d1 = _mm256_permutevar8x32_epi32(d0, permu);
			d1 = _mm256_shuffle_epi8(d1, idx8);

			d1 = _mm256_srlv_epi32(d1, shift);
			d0 = _mm256_slli_epi32(d1, 6);
			d1 = _mm256_blend_epi16(d0, d1, 0x55);
			d1 = _mm256_and_si256(d1, mask16);

			d0 = _mm256_mulhrs_epi16(d1, q16x);
			_mm256_storeu_si256((__m256i *)&r->vec[j].coeffs[16 * i], d0);	
		}
		a += cpbytes;
	}
}

void poly_compress10(uint8_t *r, const poly *a) {

	__m256i f0,f1,f2,f3, g0,g1,g2,g3;
	const __m256i hfq = _mm256_set1_epi32(PARAM_Q/2);
	const __m256i mask = _mm256_set1_epi32(0x3ff);
	const __m256i num966366 = _mm256_set1_epi64x(966366);
	const __m256i himask32 = _mm256_set1_epi32(0x3ff << 16);
	const __m256i zero = _mm256_setzero_si256();
	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 12, 11,
	10, 9, 8, 4, 3, 2, 1, 0,
	15, 15, 15, 15, 15, 15, 12, 11,
	10, 9, 8, 4, 3, 2, 1, 0);
	for (int i = 0; i < PARAM_N /16; i++)
	{
		f2 = _mm256_load_si256(a->coeffs+i*16);
		f0 = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(f2));
		f1 = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(f2,1));

		//compress 
		//b=(a*966366)>> 22, a = (a + b)>>12, here a = a/3329
		f0 = _mm256_slli_epi32(f0, 10);
		f1 = _mm256_slli_epi32(f1, 10);
		f0 = _mm256_add_epi32(f0,hfq);
		f1 = _mm256_add_epi32(f1,hfq);

		g0 = _mm256_mul_epi32(f0,num966366);
		g1 = _mm256_mul_epi32(_mm256_srli_epi64(f0,32),num966366);
		g0 = _mm256_srli_epi64(g0,22);
		g1 = _mm256_srli_epi64(g1,22);
		f2 = _mm256_blend_epi32(g0,_mm256_slli_epi64(g1,32),0xAA);

		g2 = _mm256_mul_epi32(f1,num966366);
		g3 = _mm256_mul_epi32(_mm256_srli_epi64(f1,32),num966366);
		g2 = _mm256_srli_epi64(g2,22);
		g3 = _mm256_srli_epi64(g3,22);
		f3 = _mm256_blend_epi32(g2,_mm256_slli_epi64(g3,32),0xAA);

		f0 = _mm256_add_epi32(f0,f2);
		f1 = _mm256_add_epi32(f1,f3);
		f0 = _mm256_srli_epi32(f0,12);
		f1 = _mm256_srli_epi32(f1,12);

		f0 = _mm256_and_si256(f0,mask);
		f1 = _mm256_and_si256(f1,mask);

		f0 = _mm256_packus_epi32(f0,f1);
		f0 = _mm256_permute4x64_epi64(f0,0xd8);

		//pack
		f1 = _mm256_and_si256(f0,himask32);
		f1 = _mm256_srli_epi32(f1,6);
		f0 = _mm256_and_si256(f0,mask);
		f0 = _mm256_xor_si256(f0,f1);

		f1 = _mm256_blend_epi32(f0, zero, 0xAA);
		f0 = _mm256_blend_epi32(zero, f0, 0xAA);
		f0 = _mm256_srli_epi64(f0, 12);
		f0 = _mm256_xor_si256(f0, f1);

		f0 = _mm256_shuffle_epi8(f0, idx8);

		_mm_storeu_si128((__m128i *)(r + i * 20), _mm256_castsi256_si128(f0));
		storeu128_partial(r + i * 20 + 10, _mm256_extracti128_si256(f0, 1), 10);
	}
	
}

void polyvec_compress10(uint8_t *r, const polyvec *a){
	const int cpbytes = (10 * PARAM_N) >> 3;
	for (int i = 0; i < PARAM_K; i++)
		poly_compress10(r + cpbytes * i, &a->vec[i]);

}


/*************************************************
* Name:        polyvec_tobytes
* 
* Description: Serialize vector of polynomials
*
* Arguments:   - uint8_t *r: pointer to output byte array 
*              - const polyvec *a: pointer to input vector of polynomials
**************************************************/
void polyvec_tobytes(uint8_t *r, const polyvec *a)
{
  int i;
  for(i=0;i<PARAM_K;i++)
    poly_tobytes(r+i*POLY_BYTES, &a->vec[i]);
}

/*************************************************
* Name:        polyvec_frombytes
* 
* Description: De-serialize vector of polynomials;
*              inverse of polyvec_tobytes 
*
* Arguments:   - uint8_t *r: pointer to output byte array 
*              - const polyvec *a: pointer to input vector of polynomials
**************************************************/
void polyvec_frombytes(polyvec *r, const uint8_t *a)
{
  int i;
  for(i=0;i<PARAM_K;i++)
    poly_frombytes(&r->vec[i], a+i*POLY_BYTES);
}

void polyvec_ntt(polyvec *r)
{
  int i;
  for(i=0;i<PARAM_K;i++)
    poly_ntt(&r->vec[i]);
}

void polyvec_invntt(polyvec *r)
{
  int i;
  for(i=0;i<PARAM_K;i++)
    poly_invntt(&r->vec[i]);
}
 
/*************************************************
* Name:        polyvec_pointwise_acc
* 
* Description: Pointwise multiply elements of a and b and accumulate into r
*
* Arguments: - poly *r:          pointer to output polynomial
*            - const polyvec *a: pointer to first input vector of polynomials
*            - const polyvec *b: pointer to second input vector of polynomials
**************************************************/ 
void polyvec_pointwise_acc(poly* r, const polyvec* a, const polyvec* b)
{
	poly t;
	poly_mont_mul(r, &a->vec[0], &b->vec[0]);
#if PARAM_K == 2
	poly_mont_mul(&t, &a->vec[1], &b->vec[1]);
	poly_add(r, r, &t);
#endif
	poly_getmontgomery(r);
}
/*************************************************
* Name:        polyvec_add
* 
* Description: Add vectors of polynomials
*
* Arguments: - polyvec *r:       pointer to output vector of polynomials
*            - const polyvec *a: pointer to first input vector of polynomials
*            - const polyvec *b: pointer to second input vector of polynomials
**************************************************/ 
void polyvec_add(polyvec *r, const polyvec *a, const polyvec *b)
{
  int i;
  for(i=0;i<PARAM_K;i++)
    poly_add(&r->vec[i], &a->vec[i], &b->vec[i]);

}
void polyvec_caddq(polyvec *r)
{
	int i;
	for (i = 0; i < PARAM_K; i++)
		poly_caddq(r->vec+i);

}

void polyvec_addq(polyvec *r)
{
	int i;
	for (i = 0; i < PARAM_K; i++)
		poly_addq(r->vec + i);

}

void polyvec_ss_getnoise(polyvec *r, const uint8_t *seed, uint8_t nonce)
{
#ifdef KDF_AVX
	ALIGN(32) uint8_t buf[4][ETAS_BYTES];
	ALIGN(32) uint8_t extseed[4][SEED_BYTES + 1];
	int i;

	for (i = 0; i < SEED_BYTES; i++)
	{
		extseed[0][i] = seed[i];
		extseed[1][i] = seed[i];
		extseed[2][i] = seed[i];
		extseed[3][i] = seed[i];
	}

	extseed[0][SEED_BYTES] = nonce;
	extseed[1][SEED_BYTES] = nonce + 1;
	extseed[2][SEED_BYTES] = nonce + 2;
	extseed[3][SEED_BYTES] = nonce + 3;

	KDFX4(buf[0], buf[1], buf[2], buf[3], ETAS_BYTES, extseed[0], extseed[1], extseed[2], extseed[3], SEED_BYTES + 1);

	for (i = 0; i < PARAM_K; i++)
		cbd_etas(&r->vec[i], buf[i]);
#else
ALIGN(32) uint8_t buf[ETAS_BYTES];
ALIGN(32) uint8_t extseed[SEED_BYTES + 1];
	int i;

	for (i = 0; i < SEED_BYTES; i++)
		extseed[i] = seed[i];
	for (i = 0; i < PARAM_K; i++)
	{
		extseed[SEED_BYTES] = nonce;
		nonce++;
		KDF(buf,sizeof(buf),extseed,SEED_BYTES+1);
		cbd_etas(&r->vec[i], buf);
	}
#endif
}


void polyvec_ee_getnoise(polyvec *r, const uint8_t *seed, uint8_t nonce)
{

#ifdef KDF_AVX
	ALIGN(32) uint8_t buf[4][ETA_E*PARAM_N / 4];
	ALIGN(32) uint8_t extseed[4][SEED_BYTES + 1];
	int i;

	for (i = 0; i < SEED_BYTES; i++)
	{
		extseed[0][i] = seed[i];
		extseed[1][i] = seed[i];
		extseed[2][i] = seed[i];
		extseed[3][i] = seed[i];
	}

	extseed[0][SEED_BYTES] = nonce;
	extseed[1][SEED_BYTES] = nonce + 1;
	extseed[2][SEED_BYTES] = nonce + 2;
	extseed[3][SEED_BYTES] = nonce + 3;
	
	
	KDFX4(buf[0], buf[1], buf[2], buf[3], ETA_E*PARAM_N / 4, extseed[0], extseed[1], extseed[2], extseed[3], SEED_BYTES + 1);

	for (i = 0; i < PARAM_K; i++)
		cbd_etae(&r->vec[i], buf[i]);
#else
ALIGN(32) uint8_t buf[ETAE_BYTES] __attribute__((aligned(32)));
ALIGN(32) uint8_t extseed[SEED_BYTES + 1];
	int i;
	for (i = 0; i < SEED_BYTES; i++)
		extseed[i] = seed[i];
	for (i = 0; i < PARAM_K; i++)
	{
		extseed[SEED_BYTES] = nonce;
		nonce++;
		KDF(buf, sizeof(buf), extseed, SEED_BYTES + 1);
		cbd_etae(&r->vec[i], buf);
	}
#endif
}
