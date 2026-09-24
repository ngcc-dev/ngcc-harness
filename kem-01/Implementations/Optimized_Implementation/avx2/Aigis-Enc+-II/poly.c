#include <stdio.h>
#include <immintrin.h>
#include <string.h>
#include "poly.h"
#include "ntt.h"
#include "cbd.h"
#include "hashkdf.h"
#include "api.h"


const int16_t NTT_Y[PARAM_N] = { -1103,1103,430,-430,555,-555,843,-843,-1251,1251,871,-871,1550,-1550,105,-105,422,-422,587,-587,177,-177,-235,235,-291,291,-460,460,1574,-1574,1653,-1653,-246,246,778,-778,1159,-1159,-147,147,-777,777,1483,-1483,-602,602,1119,-1119,-1590,1590,644,-644,-872,872,349,-349,418,-418,329,-329,-156,156,-75,75,817,-817,1097,-1097,603,-603,610,-610,1322,-1322,-1285,1285,-1465,1465,384,-384,-1215,1215,-136,136,1218,-1218,-1335,1335,-874,874,220,-220,-1187,1187,-1659,1659,-1185,1185,-1530,1530,-1278,1278,794,-794,-1510,1510,-854,854,-870,870,478,-478,-108,108,-308,308,996,-996,991,-991,958,-958,-1460,1460,1522,-1522,1628,-1628 };

int16_t montgomery_reduce(int32_t a)
{
	int16_t t;
	t = (int16_t)a * QINV;
	t = (a - (int32_t)t * PARAM_Q) >> 16;
	return t;
}
int16_t barrett_reduce(int16_t a)
{
	int16_t u;
#if PARAM_Q == 3329
	u = ((int32_t)a * 9 + (1 << 14)) >> 15;
#elif PARAM_Q == 641
	u = ((int32_t)a * 51 + (1 << 14)) >> 15;
#endif
	u *= PARAM_Q;
	a -= u;
	return a;
}
void poly_ss_getnoise(poly *r, const uint8_t *seed, uint8_t nonce)
{
	ALIGN(32) uint8_t buf[ETAS_BYTES];
	ALIGN(32) uint8_t extseed[SEED_BYTES + 1];
	int i;

	for (i = 0; i < SEED_BYTES; i++)
		extseed[i] = seed[i];
	extseed[SEED_BYTES] = nonce;

	KDF(buf, sizeof(buf), extseed, SEED_BYTES + 1);

	cbd_etas(r, buf);
}
void poly_ee_getnoise(poly *r, const uint8_t *seed, uint8_t nonce)
{
	ALIGN(32) uint8_t buf[ETAE_BYTES];
	ALIGN(32) uint8_t extseed[SEED_BYTES + 1];
	int i;

	for (i = 0; i < SEED_BYTES; i++)
		extseed[i] = seed[i];
	extseed[SEED_BYTES] = nonce;

	KDF(buf, sizeof(buf), extseed, SEED_BYTES + 1);

	cbd_etae(r, buf);
}

void poly_compress(uint8_t r[POLY_COMPRESSED_BYTES + 10], const poly *a)
{
	int i;
	uint8_t tmp[16];
	__m256i d0, d1;

	const __m256i fdiv = _mm256_set1_epi16(20159);//fast division  right shift >> 26
	const __m256i hfq = _mm256_set1_epi16(13 << (7 - BITS_C2));
	int shift = 10 - BITS_C2;

	const __m256i zero = _mm256_setzero_si256();
	const __m256i permu = _mm256_set_epi32(7, 7, 7, 7, 5, 4, 1, 0);
#if BITS_C2 == 3
	const __m256i lomask32 = _mm256_set1_epi32(0x7);
	const __m256i himask32 = _mm256_set1_epi32(0x7 << 16);
	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 9, 8, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 9, 8, 15);

	const __m256i idx82 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 10, 9, 8, 2, 1, 0);

#if PARAM_N == 512
	for (i = 0; i < 30; i++)
#elif PARAM_N == 1024
	for (i = 0; i < 62; i++)
#elif PARAM_N == 2048
	for (i = 0; i < 126; i++)
#else
#error "unsupported PARAM_N"
#endif
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i]);
		d0 = _mm256_add_epi16(d0, hfq);
		d0 = _mm256_mulhi_epi16(d0, fdiv);
		d0 = _mm256_srli_epi16(d0, shift);

		d1 = _mm256_and_si256(d0, himask32);
		d0 = _mm256_and_si256(d0, lomask32);
		d1 = _mm256_srli_epi32(d1, 13);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_blend_epi32(d1, zero, 0xAA);
		d1 = _mm256_blend_epi32(zero, d1, 0xAA);
		d1 = _mm256_srli_epi64(d1, 26);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_blend_epi32(d1, zero, 0xCC);
		d1 = _mm256_slli_epi64(d1, 4);
		d1 = _mm256_shuffle_epi8(d1, idx8);
		d0 = _mm256_or_si256(d0, d1);

		d1 = _mm256_permutevar8x32_epi32(d0, permu);
		d0 = _mm256_shuffle_epi8(d1, idx82);

		_mm_storeu_si128((__m128i *)&r[6 * i], _mm256_castsi256_si128(d0));
	}
#if PARAM_N == 512
	i = 30;
#elif PARAM_N == 1024
	i = 62;
#elif PARAM_N == 2048
	i = 126;
#endif
	d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i]);
	d0 = _mm256_add_epi16(d0, hfq);
	d0 = _mm256_mulhi_epi16(d0, fdiv);
	d0 = _mm256_srli_epi16(d0, shift);

	d1 = _mm256_and_si256(d0, himask32);
	d0 = _mm256_and_si256(d0, lomask32);
	d1 = _mm256_srli_epi32(d1, 13);
	d1 = _mm256_or_si256(d0, d1);

	d0 = _mm256_blend_epi32(d1, zero, 0xAA);
	d1 = _mm256_blend_epi32(zero, d1, 0xAA);
	d1 = _mm256_srli_epi64(d1, 26);
	d1 = _mm256_or_si256(d0, d1);

	d0 = _mm256_blend_epi32(d1, zero, 0xCC);
	d1 = _mm256_slli_epi64(d1, 4);
	d1 = _mm256_shuffle_epi8(d1, idx8);
	d0 = _mm256_or_si256(d0, d1);

	d1 = _mm256_permutevar8x32_epi32(d0, permu);
	d0 = _mm256_shuffle_epi8(d1, idx82);

	_mm_storeu_si128((__m128i *)tmp, _mm256_castsi256_si128(d0));
	memcpy(&r[6 * i], tmp, 12);
	i++;

	d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i]);
	d0 = _mm256_add_epi16(d0, hfq);
	d0 = _mm256_mulhi_epi16(d0, fdiv);
	d0 = _mm256_srli_epi16(d0, shift);

	d1 = _mm256_and_si256(d0, himask32);
	d0 = _mm256_and_si256(d0, lomask32);
	d1 = _mm256_srli_epi32(d1, 13);
	d1 = _mm256_or_si256(d0, d1);

	d0 = _mm256_blend_epi32(d1, zero, 0xAA);
	d1 = _mm256_blend_epi32(zero, d1, 0xAA);
	d1 = _mm256_srli_epi64(d1, 26);
	d1 = _mm256_or_si256(d0, d1);

	d0 = _mm256_blend_epi32(d1, zero, 0xCC);
	d1 = _mm256_slli_epi64(d1, 4);
	d1 = _mm256_shuffle_epi8(d1, idx8);
	d0 = _mm256_or_si256(d0, d1);

	d1 = _mm256_permutevar8x32_epi32(d0, permu);
	d0 = _mm256_shuffle_epi8(d1, idx82);

	_mm_storeu_si128((__m128i *)tmp, _mm256_castsi256_si128(d0));
	memcpy(&r[6 * i], tmp, 6);
#elif BITS_C2 == 4
	const __m256i permu2 = _mm256_set_epi32(7, 7, 7, 7, 7, 7, 4, 0);
	const __m256i lomask32 = _mm256_set1_epi32(0xf);
	const __m256i himask32 = _mm256_set1_epi32(0xf << 16);
	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 12, 8, 4, 0,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 12, 8, 4, 0);

#if PARAM_N == 512
	for (i = 0; i < 31; i++)
#elif PARAM_N == 1024
	for (i = 0; i < 63; i++)
#elif PARAM_N == 2048
	for (i = 0; i < 127; i++)
#else
#error "unsupported PARAM_N"
#endif
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i]);
		d0 = _mm256_add_epi16(d0, hfq);
		d0 = _mm256_mulhi_epi16(d0, fdiv);
		d0 = _mm256_srli_epi16(d0, shift);

		d1 = _mm256_and_si256(d0, himask32);
		d0 = _mm256_and_si256(d0, lomask32);
		d1 = _mm256_srli_epi32(d1, 12);
		d1 = _mm256_or_si256(d0, d1);

		d1 = _mm256_shuffle_epi8(d1, idx8);
		d0 = _mm256_permutevar8x32_epi32(d1, permu2);
		_mm_storeu_si128((__m128i *)&r[8 * i], _mm256_castsi256_si128(d0));
	}
#if PARAM_N == 512
	i = 31;
#elif PARAM_N == 1024
	i = 63;
#elif PARAM_N == 2048
	i = 127;
#endif
	d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i]);
	d0 = _mm256_add_epi16(d0, hfq);
	d0 = _mm256_mulhi_epi16(d0, fdiv);
	d0 = _mm256_srli_epi16(d0, shift);

	d1 = _mm256_and_si256(d0, himask32);
	d0 = _mm256_and_si256(d0, lomask32);
	d1 = _mm256_srli_epi32(d1, 12);
	d1 = _mm256_or_si256(d0, d1);

	d1 = _mm256_shuffle_epi8(d1, idx8);
	d0 = _mm256_permutevar8x32_epi32(d1, permu2);
	_mm_storeu_si128((__m128i *)tmp, _mm256_castsi256_si128(d0));
	memcpy(&r[8 * i], tmp, 8);
#elif BITS_C2 == 5
	const __m256i lomask32 = _mm256_set1_epi32(0x1f);
	const __m256i himask32 = _mm256_set1_epi32(0x1f << 16);
	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 10, 9, 8, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 10, 9, 8, 15, 15);

	const __m256i idx82 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 12, 11,
		10, 9, 8, 4, 3, 2, 1, 0);

#if PARAM_N == 512
	for (i = 0; i < 31; i++)
#elif PARAM_N == 1024
	for (i = 0; i < 63; i++)
#elif PARAM_N == 2048
	for (i = 0; i < 127; i++)
#else
#error "unsupported PARAM_N"
#endif
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i]);
		d0 = _mm256_add_epi16(d0, hfq);
		d0 = _mm256_mulhi_epi16(d0, fdiv);
		d0 = _mm256_srli_epi16(d0, shift);

		d1 = _mm256_and_si256(d0, himask32);
		d0 = _mm256_and_si256(d0, lomask32);
		d1 = _mm256_srli_epi32(d1, 11);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_blend_epi32(d1, zero, 0xAA);
		d1 = _mm256_blend_epi32(zero, d1, 0xAA);
		d1 = _mm256_srli_epi64(d1, 22);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_blend_epi32(d1, zero, 0xCC);
		d1 = _mm256_slli_epi64(d1, 4);
		d1 = _mm256_shuffle_epi8(d1, idx8);
		d0 = _mm256_or_si256(d0, d1);

		d1 = _mm256_permutevar8x32_epi32(d0, permu);
		d0 = _mm256_shuffle_epi8(d1, idx82);

		_mm_storeu_si128((__m128i *)&r[10 * i], _mm256_castsi256_si128(d0));
	}
#if PARAM_N == 512
	i = 31;
#elif PARAM_N == 1024
	i = 63;
#elif PARAM_N == 2048
	i = 127;
#endif
	d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i]);
	d0 = _mm256_add_epi16(d0, hfq);
	d0 = _mm256_mulhi_epi16(d0, fdiv);
	d0 = _mm256_srli_epi16(d0, shift);

	d1 = _mm256_and_si256(d0, himask32);
	d0 = _mm256_and_si256(d0, lomask32);
	d1 = _mm256_srli_epi32(d1, 11);
	d1 = _mm256_or_si256(d0, d1);

	d0 = _mm256_blend_epi32(d1, zero, 0xAA);
	d1 = _mm256_blend_epi32(zero, d1, 0xAA);
	d1 = _mm256_srli_epi64(d1, 22);
	d1 = _mm256_or_si256(d0, d1);

	d0 = _mm256_blend_epi32(d1, zero, 0xCC);
	d1 = _mm256_slli_epi64(d1, 4);
	d1 = _mm256_shuffle_epi8(d1, idx8);
	d0 = _mm256_or_si256(d0, d1);

	d1 = _mm256_permutevar8x32_epi32(d0, permu);
	d0 = _mm256_shuffle_epi8(d1, idx82);

	_mm_storeu_si128((__m128i *)tmp, _mm256_castsi256_si128(d0));
	memcpy(&r[10 * i], tmp, 10);
#else
#error "poly_compress only supports BITS_C2 in {3,4,5}"
#endif
}
void poly_decompress(poly *r, const uint8_t *a)
{
	unsigned int i;
	__m128i t;
	__m256i d0, d1;
	const __m256i q16x = _mm256_set1_epi16(PARAM_Q);
#if BITS_C2 == 3
	const __m256i idx8 = _mm256_set_epi8(6, 5, 4, 3, 6, 5, 4, 3,
		5, 4, 3, 0, 4, 3, 0, 0,
		3, 2, 1, 0, 3, 2, 1, 0,
		2, 1, 0, 0, 1, 0, 0, 0);
	const __m256i shift = _mm256_set_epi32(6, 0, 2, 4, 6, 0, 2, 4);

	const __m256i mask16 = _mm256_set1_epi16(7 << 12);
	for (i = 0; i < PARAM_N / 16; i++)
	{
		d0 = _mm256_broadcastq_epi64(*(__m128i *)&a[i * 6]);
		d0 = _mm256_shuffle_epi8(d0, idx8);
		d0 = _mm256_srlv_epi32(d0, shift);
		d1 = _mm256_slli_epi32(d0, 13);
		d0 = _mm256_blend_epi16(d1, d0, 0x55);
		d1 = _mm256_and_si256(d0, mask16);

		d0 = _mm256_mulhrs_epi16(d1, q16x);
		_mm256_store_si256((__m256i *)&r->coeffs[16 * i], d0);
	}
#elif BITS_C2 == 4
	const __m256i mask16 = _mm256_set1_epi16(15 << 11);
	for (i = 0; i < PARAM_N / 16; i++)
	{
		t = _mm_loadu_si128(&a[i * 8]);
		// d0 = _mm256_cvtepu8_epi32(*(__m128i *)&a[i * 8]);
		d0 = _mm256_cvtepu8_epi32(t);
		d0 = _mm256_slli_epi32(d0, 11);
		d1 = _mm256_slli_epi32(d0, 12);
		d0 = _mm256_or_si256(d0, d1);
		d1 = _mm256_and_si256(d0, mask16);

		d0 = _mm256_mulhrs_epi16(d1, q16x);
		_mm256_storeu_si256((__m256i *)&r->coeffs[16 * i], d0);
	}

#elif BITS_C2 == 5
	const __m256i permu = _mm256_set_epi32(4, 3, 2, 1, 3, 2, 1, 0);
	const __m256i mask16 = _mm256_set1_epi16(31 << 10);
	const __m256i idx8 = _mm256_set_epi8(6, 5, 4, 0, 5, 4, 3, 0,
		4, 3, 2, 0, 2, 1, 0, 0,
		5, 4, 3, 0, 4, 3, 2, 0,
		3, 2, 1, 0, 1, 0, 0, 0);
	const __m256i shift = _mm256_set_epi32(4, 2, 0, 6, 4, 2, 0, 6);
	for (i = 0; i < PARAM_N / 16; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a[10 * i]);
		d1 = _mm256_permutevar8x32_epi32(d0, permu);
		d1 = _mm256_shuffle_epi8(d1, idx8);
		d1 = _mm256_srlv_epi32(d1, shift);
		d0 = _mm256_slli_epi32(d1, 11);
		d1 = _mm256_blend_epi16(d0, d1, 0x55);
		d1 = _mm256_and_si256(d1, mask16);

		d0 = _mm256_mulhrs_epi16(d1, q16x);
		_mm256_storeu_si256((__m256i *)&r->coeffs[16 * i], d0);
	}
#else
#error "poly_decompress only supports BITS_C2 in {3,4,5}"
#endif
}


void poly_tobytes(uint8_t r[POLY_BYTES + 6], const poly *a)
{
	int i, j;
	__m256i d0, d1;
	const __m256i permu = _mm256_set_epi32(7, 7, 6, 5, 4, 2, 1, 0);
	const __m256i zero = _mm256_setzero_si256();
	const __m256i lomask32 = _mm256_set1_epi32(0xffffU);
	const __m256i himask32 = _mm256_set1_epi32(0xffffU << 16);
	
	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 14, 13, 12, 10,
		9, 8, 6, 5, 4, 2, 1, 0,
		15, 15, 15, 15, 14, 13, 12, 10,
		9, 8, 6, 5, 4, 2, 1, 0);

	for (i = 0; i < PARAM_N / 16; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i]);
		d1 = _mm256_and_si256(d0, himask32);
		d0 = _mm256_and_si256(d0, lomask32);
		d1 = _mm256_srli_epi32(d1, 4);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_shuffle_epi8(d1, idx8);
		d0 = _mm256_permutevar8x32_epi32(d0, permu);
		_mm256_storeu_si256((__m256i *)&r[24 * i], d0);
	}
}
void poly_frombytes(poly *r, const uint8_t *a)
{
	int i;
	const __m256i permu = _mm256_set_epi32(6, 5, 4, 3, 3, 2, 1, 0);
	const __m256i mask16 = _mm256_set1_epi16(0xfff);
	const __m256i idx8 = _mm256_set_epi8(12, 11, 10, 9, 9, 8, 7, 6,
		6, 5, 4, 3, 3, 2, 1, 0,
		12, 11, 10, 9, 9, 8, 7, 6,
		6, 5, 4, 3, 3, 2, 1, 0);
	__m256i d0, d1;

	for (i = 0; i < PARAM_N / 16; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a[24 * i]);
		d1 = _mm256_permutevar8x32_epi32(d0, permu);
		d1 = _mm256_shuffle_epi8(d1, idx8);
		d0 = _mm256_slli_epi32(d1, 4);
		d0 = _mm256_blend_epi16(d0, d1, 0x55);
		d0 = _mm256_and_si256(d0, mask16);
		_mm256_storeu_si256((__m256i *)&r->coeffs[16 * i], d0);
	}
}

#if NTT_DIM == 64
#if PARAM_N/MSG_BYTES == 16
void poly_frommsg(poly* r, const uint8_t msg[SEED_BYTES])
{
	int i, j;
	__m128i tmp;
	__m256i a[4], d0, d1, d2, d3;
	const __m256i shift = _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0);
	const __m256i zeros = _mm256_setzero_si256();
	const __m256i ones = _mm256_set1_epi32(1);
	const __m256i hqs = _mm256_set1_epi32((PARAM_Q + 1) / 2);

	for (j = 0; j < MSG_BYTES / 16; j++)
	{
		tmp = _mm_loadu_si128((__m128i*) & msg[j * 16]);
		for (i = 0; i < 4; i++)
		{
			a[i] = _mm256_broadcastd_epi32(tmp);
			tmp = _mm_srli_si128(tmp, 4);
		}

		for (i = 0; i < 4; i++)
		{
			d0 = _mm256_srlv_epi32(a[i], shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, ones);
			d1 = _mm256_and_si256(d1, ones);
			d2 = _mm256_and_si256(d2, ones);
			d3 = _mm256_and_si256(d3, ones);

			d0 = _mm256_sub_epi32(zeros, d0);
			d1 = _mm256_sub_epi32(zeros, d1);
			d2 = _mm256_sub_epi32(zeros, d2);
			d3 = _mm256_sub_epi32(zeros, d3);

			d0 = _mm256_and_si256(hqs, d0);
			d1 = _mm256_and_si256(hqs, d1);
			d2 = _mm256_and_si256(hqs, d2);
			d3 = _mm256_and_si256(hqs, d3);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			d0 = _mm256_permute4x64_epi64(d0, 0xD8);
			d2 = _mm256_permute4x64_epi64(d2, 0xD8);

			_mm256_storeu_si256((__m256i*) & r->coeffs[256 * j + 64 * i + 0], d0);
			_mm256_storeu_si256((__m256i*) & r->coeffs[256 * j + 64 * i + 16], d2);
			_mm256_storeu_si256((__m256i*) & r->coeffs[256 * j + 64 * i + 32], d0);
			_mm256_storeu_si256((__m256i*) & r->coeffs[256 * j + 64 * i + 48], d2);
		}
	}
}
void poly_tomsg(unsigned char* msg, const poly* r)
{
	int i, j, small;
	__m256i u, v, mask;
	const __m256i q16x = _mm256_set1_epi16(PARAM_Q);
	const __m256i hfq = _mm256_set1_epi16(PARAM_Q / 2);

	for (i = 0; i < PARAM_N / 64; i++)
	{
		for (j = 0; j < 2; j++)
		{
			v = _mm256_loadu_si256((__m256i*) & r->coeffs[64 * i + 16 * j]);
			mask = _mm256_srai_epi16(v, 15);
			mask = _mm256_and_si256(mask, q16x);
			v = _mm256_add_epi16(v, mask);

			v = _mm256_sub_epi16(v, hfq);
			mask = _mm256_srai_epi16(v, 15);
			v = _mm256_add_epi16(v, mask);
			u = _mm256_xor_si256(v, mask);

			v = _mm256_loadu_si256((__m256i*) & r->coeffs[64 * i + 16 * j + 32]);
			mask = _mm256_srai_epi16(v, 15);
			mask = _mm256_and_si256(mask, q16x);
			v = _mm256_add_epi16(v, mask);

			v = _mm256_sub_epi16(v, hfq);
			mask = _mm256_srai_epi16(v, 15);
			v = _mm256_add_epi16(v, mask);
			v = _mm256_xor_si256(v, mask);

			u = _mm256_add_epi16(u, v);
			v = _mm256_sub_epi16(u, hfq);

			small = _mm256_movemask_epi8(v);
			small = _pext_u32(small, 0xAAAAAAAA);
			msg[4 * i + 2 * j + 0] = small;
			msg[4 * i + 2 * j + 1] = small >> 8;
		}
	}
}
#elif PARAM_N/SEED_BYTES == 32
void poly_frommsg(poly* r, const uint8_t msg[SEED_BYTES])
{
	int i, j;
	__m128i tmp;
	__m256i a[4], d0, d1, d2, d3;
	const __m256i shift = _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0);
	const __m256i zeros = _mm256_setzero_si256();
	const __m256i ones = _mm256_set1_epi32(1);
	const __m256i hqs = _mm256_set1_epi32((PARAM_Q + 1) / 2);

	for (j = 0; j < SEED_BYTES / 16; j++)
	{
		tmp = _mm_loadu_si128((__m128i*) & msg[j * 16]);
		for (i = 0; i < 4; i++)
		{
			a[i] = _mm256_broadcastd_epi32(tmp);
			tmp = _mm_srli_si128(tmp, 4);
		}

		for (i = 0; i < 4; i++)
		{
			d0 = _mm256_srlv_epi32(a[i], shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, ones);
			d1 = _mm256_and_si256(d1, ones);
			d2 = _mm256_and_si256(d2, ones);
			d3 = _mm256_and_si256(d3, ones);

			d0 = _mm256_sub_epi32(zeros, d0);
			d1 = _mm256_sub_epi32(zeros, d1);
			d2 = _mm256_sub_epi32(zeros, d2);
			d3 = _mm256_sub_epi32(zeros, d3);

			d0 = _mm256_and_si256(hqs, d0);
			d1 = _mm256_and_si256(hqs, d1);
			d2 = _mm256_and_si256(hqs, d2);
			d3 = _mm256_and_si256(hqs, d3);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			d0 = _mm256_permute4x64_epi64(d0, 0xD8);
			d2 = _mm256_permute4x64_epi64(d2, 0xD8);

			_mm256_storeu_si256((__m256i*) & r->coeffs[512 * j + 128 * i + 0], d0);
			_mm256_storeu_si256((__m256i*) & r->coeffs[512 * j + 128 * i + 16], d0);
			_mm256_storeu_si256((__m256i*) & r->coeffs[512 * j + 128 * i + 32], d0);
			_mm256_storeu_si256((__m256i*) & r->coeffs[512 * j + 128 * i + 48], d0);

			_mm256_storeu_si256((__m256i*) & r->coeffs[512 * j + 128 * i + 64], d2);
			_mm256_storeu_si256((__m256i*) & r->coeffs[512 * j + 128 * i + 80], d2);
			_mm256_storeu_si256((__m256i*) & r->coeffs[512 * j + 128 * i + 96], d2);
			_mm256_storeu_si256((__m256i*) & r->coeffs[512 * j + 128 * i + 112], d2);
		}
	}
}
void poly_tomsg(uint8_t msg[32], const poly* r)
{
	int i, j, small;
	__m256i u, v, mask;
	const __m256i q16x = _mm256_set1_epi16(PARAM_Q);
	const __m256i hfq = _mm256_set1_epi16(PARAM_Q / 2);

	for (i = 0; i < SEED_BYTES / 2; i++)
	{
		v = _mm256_loadu_si256((__m256i*) & r->coeffs[64 * i]);
		mask = _mm256_srai_epi16(v, 15);
		mask = _mm256_and_si256(mask, q16x);
		v = _mm256_add_epi16(v, mask);

		v = _mm256_sub_epi16(v, hfq);
		mask = _mm256_srai_epi16(v, 15);
		v = _mm256_add_epi16(v, mask);
		u = _mm256_xor_si256(v, mask);

		v = _mm256_loadu_si256((__m256i*) & r->coeffs[64 * i + 16]);
		mask = _mm256_srai_epi16(v, 15);
		mask = _mm256_and_si256(mask, q16x);
		v = _mm256_add_epi16(v, mask);

		v = _mm256_sub_epi16(v, hfq);
		mask = _mm256_srai_epi16(v, 15);
		v = _mm256_add_epi16(v, mask);
		v = _mm256_xor_si256(v, mask);
		u = _mm256_add_epi16(u, v);

		v = _mm256_load_si256((__m256i*) & r->coeffs[64 * i + 32]);
		mask = _mm256_srai_epi16(v, 15);
		mask = _mm256_and_si256(mask, q16x);
		v = _mm256_add_epi16(v, mask);

		v = _mm256_sub_epi16(v, hfq);
		mask = _mm256_srai_epi16(v, 15);
		v = _mm256_add_epi16(v, mask);
		v = _mm256_xor_si256(v, mask);
		u = _mm256_add_epi16(u, v);

		v = _mm256_load_si256((__m256i*) & r->coeffs[64 * i + 48]);
		mask = _mm256_srai_epi16(v, 15);
		mask = _mm256_and_si256(mask, q16x);
		v = _mm256_add_epi16(v, mask);

		v = _mm256_sub_epi16(v, hfq);
		mask = _mm256_srai_epi16(v, 15);
		v = _mm256_add_epi16(v, mask);
		v = _mm256_xor_si256(v, mask);
		u = _mm256_add_epi16(u, v);

		v = _mm256_sub_epi16(u, q16x);

		small = _mm256_movemask_epi8(v);
		small = _pext_u32(small, 0xAAAAAAAA);
		msg[2 * i + 0] = small;
		msg[2 * i + 1] = small >> 8;
	}
}
#endif
#elif NTT_DIM ==128
#if PARAM_N/MSG_BYTES == 16
void poly_frommsg(poly* r, const uint8_t msg[SEED_BYTES])
{
	int i, j;
	__m128i tmp;
	__m256i a[4], d0, d1, d2, d3;
	const __m256i shift = _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0);
	const __m256i zeros = _mm256_setzero_si256();
	const __m256i ones = _mm256_set1_epi32(1);
	const __m256i hqs = _mm256_set1_epi32((PARAM_Q + 1) / 2);

	for (j = 0; j < MSG_BYTES / 16; j++)
	{
		tmp = _mm_loadu_si128((__m128i*) & msg[j * 16]);
		for (i = 0; i < 4; i++)
		{
			a[i] = _mm256_broadcastd_epi32(tmp);
			tmp = _mm_srli_si128(tmp, 4);
		}

		for (i = 0; i < 2; i++)
		{
			d0 = _mm256_srlv_epi32(a[i], shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, ones);
			d1 = _mm256_and_si256(d1, ones);
			d2 = _mm256_and_si256(d2, ones);
			d3 = _mm256_and_si256(d3, ones);

			d0 = _mm256_sub_epi32(zeros, d0);
			d1 = _mm256_sub_epi32(zeros, d1);
			d2 = _mm256_sub_epi32(zeros, d2);
			d3 = _mm256_sub_epi32(zeros, d3);

			d0 = _mm256_and_si256(hqs, d0);
			d1 = _mm256_and_si256(hqs, d1);
			d2 = _mm256_and_si256(hqs, d2);
			d3 = _mm256_and_si256(hqs, d3);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			d0 = _mm256_permute4x64_epi64(d0, 0xD8);
			d2 = _mm256_permute4x64_epi64(d2, 0xD8);

			

			_mm256_storeu_si256((__m256i*) & r->coeffs[256 * j + 32 * i + 0], d0);
			_mm256_storeu_si256((__m256i*) & r->coeffs[256 * j + 32 * i + 64], d0);

			_mm256_storeu_si256((__m256i*) & r->coeffs[256 * j + 32 * i + 16], d2);
			_mm256_storeu_si256((__m256i*) & r->coeffs[256 * j + 32 * i + 80], d2);

			/*d1 = _mm256_load_si256((__m256i*) & r->coeffs[256 * j + 32 * i + 0]);
			d3 = _mm256_load_si256((__m256i*) & r->coeffs[256 * j + 32 * i + 64]);
			d1 = _mm256_add_epi16(d1, d0);
			d3 = _mm256_add_epi16(d3, d0);
			_mm256_store_si256((__m256i*) & r->coeffs[256 * j + 32 * i + 0], d1);
			_mm256_store_si256((__m256i*) & r->coeffs[256 * j + 32 * i + 64], d3);

			d1 = _mm256_load_si256((__m256i*) & r->coeffs[256 * j + 32 * i + 16]);
			d3 = _mm256_load_si256((__m256i*) & r->coeffs[256 * j + 32 * i + 80]);
			d1 = _mm256_add_epi16(d1, d2);
			d3 = _mm256_add_epi16(d3, d2);
			_mm256_store_si256((__m256i*) & r->coeffs[256 * j + 32 * i + 16], d1);
			_mm256_store_si256((__m256i*) & r->coeffs[256 * j + 32 * i + 80], d3);*/


		}

		for (i = 0; i < 2; i++)
		{
			d0 = _mm256_srlv_epi32(a[2 + i], shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, ones);
			d1 = _mm256_and_si256(d1, ones);
			d2 = _mm256_and_si256(d2, ones);
			d3 = _mm256_and_si256(d3, ones);

			d0 = _mm256_sub_epi32(zeros, d0);
			d1 = _mm256_sub_epi32(zeros, d1);
			d2 = _mm256_sub_epi32(zeros, d2);
			d3 = _mm256_sub_epi32(zeros, d3);

			d0 = _mm256_and_si256(hqs, d0);
			d1 = _mm256_and_si256(hqs, d1);
			d2 = _mm256_and_si256(hqs, d2);
			d3 = _mm256_and_si256(hqs, d3);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			d0 = _mm256_permute4x64_epi64(d0, 0xD8);
			d2 = _mm256_permute4x64_epi64(d2, 0xD8);

			_mm256_storeu_si256((__m256i*) & r->coeffs[256 * j + 32 * i + 128], d0);
			_mm256_storeu_si256((__m256i*) & r->coeffs[256 * j + 32 * i + 192], d0);

			_mm256_storeu_si256((__m256i*) & r->coeffs[256 * j + 32 * i + 144], d2);
			_mm256_storeu_si256((__m256i*) & r->coeffs[256 * j + 32 * i + 208], d2);

		}
	}
}
void poly_tomsg(unsigned char* msg, const poly* r)
{
	int i, j, small;
	__m256i u, v, mask;
	const __m256i q16x = _mm256_set1_epi16(PARAM_Q);
	const __m256i hfq = _mm256_set1_epi16(PARAM_Q / 2);

	for (i = 0; i < SEED_BYTES / 8; i++)
	{
		for (j = 0; j < 4; j++)
		{
			v = _mm256_loadu_si256((__m256i*) & r->coeffs[128 * i + 16 * j]);
			mask = _mm256_srai_epi16(v, 15);
			mask = _mm256_and_si256(mask, q16x);
			v = _mm256_add_epi16(v, mask);

			v = _mm256_sub_epi16(v, hfq);
			mask = _mm256_srai_epi16(v, 15);
			v = _mm256_add_epi16(v, mask);
			u = _mm256_xor_si256(v, mask);

			v = _mm256_loadu_si256((__m256i*) & r->coeffs[128 * i + 16 * j + 64]);
			mask = _mm256_srai_epi16(v, 15);
			mask = _mm256_and_si256(mask, q16x);
			v = _mm256_add_epi16(v, mask);

			v = _mm256_sub_epi16(v, hfq);
			mask = _mm256_srai_epi16(v, 15);
			v = _mm256_add_epi16(v, mask);
			v = _mm256_xor_si256(v, mask);

			u = _mm256_add_epi16(u, v);
			v = _mm256_sub_epi16(u, hfq);

			small = _mm256_movemask_epi8(v);
			small = _pext_u32(small, 0xAAAAAAAA);
			msg[8 * i + 2 * j + 0] = small;
			msg[8 * i + 2 * j + 1] = small >> 8;
		}
	}
}
#elif PARAM_N/MSG_BYTES == 32
void poly_frommsg(poly* r, const uint8_t msg[SEED_BYTES])
{
	int i, j, k;
	__m128i tmp;
	__m256i a[4], d0, d1, d2, d3;
	const __m256i shift = _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0);
	const __m256i zeros = _mm256_setzero_si256();
	const __m256i ones = _mm256_set1_epi32(1);
	const __m256i hqs = _mm256_set1_epi32((PARAM_Q + 1) / 2);

	for (j = 0; j < SEED_BYTES / 16; j++)
	{
		tmp = _mm_loadu_si128((__m128i*) & msg[j * 16]);
		for (i = 0; i < 4; i++)
		{
			a[i] = _mm256_broadcastd_epi32(tmp);
			tmp = _mm_srli_si128(tmp, 4);
		}

		for (i = 0; i < 4; i++)
		{
			d0 = _mm256_srlv_epi32(a[i], shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, ones);
			d1 = _mm256_and_si256(d1, ones);
			d2 = _mm256_and_si256(d2, ones);
			d3 = _mm256_and_si256(d3, ones);

			d0 = _mm256_sub_epi32(zeros, d0);
			d1 = _mm256_sub_epi32(zeros, d1);
			d2 = _mm256_sub_epi32(zeros, d2);
			d3 = _mm256_sub_epi32(zeros, d3);

			d0 = _mm256_and_si256(hqs, d0);
			d1 = _mm256_and_si256(hqs, d1);
			d2 = _mm256_and_si256(hqs, d2);
			d3 = _mm256_and_si256(hqs, d3);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			d0 = _mm256_permute4x64_epi64(d0, 0xD8);
			d2 = _mm256_permute4x64_epi64(d2, 0xD8);

			_mm256_storeu_si256((__m256i*) & r->coeffs[512 * j + 128 * i + 0], d0);
			_mm256_storeu_si256((__m256i*) & r->coeffs[512 * j + 128 * i + 32], d0);
			_mm256_storeu_si256((__m256i*) & r->coeffs[512 * j + 128 * i + 64], d0);
			_mm256_storeu_si256((__m256i*) & r->coeffs[512 * j + 128 * i + 96], d0);

			_mm256_storeu_si256((__m256i*) & r->coeffs[512 * j + 128 * i + 16], d2);
			_mm256_storeu_si256((__m256i*) & r->coeffs[512 * j + 128 * i + 48], d2);
			_mm256_storeu_si256((__m256i*) & r->coeffs[512 * j + 128 * i + 80], d2);
			_mm256_storeu_si256((__m256i*) & r->coeffs[512 * j + 128 * i + 112], d2);

		}
	}
}
void poly_tomsg(uint8_t msg[64], const poly* r)
{
	int i, j, small;
	__m256i u, v, mask;
	const __m256i q16x = _mm256_set1_epi16(PARAM_Q);
	const __m256i hfq = _mm256_set1_epi16(PARAM_Q / 2);

	for (i = 0; i < PARAM_N / NTT_DIM; i++)
		for (j = 0; j < NTT_DIM / 64; j++)
		{
			v = _mm256_loadu_si256((__m256i*) & r->coeffs[NTT_DIM * i + 16 * j]);
			mask = _mm256_srai_epi16(v, 15);
			mask = _mm256_and_si256(mask, q16x);
			v = _mm256_add_epi16(v, mask);

			v = _mm256_sub_epi16(v, hfq);
			mask = _mm256_srai_epi16(v, 15);
			v = _mm256_add_epi16(v, mask);
			u = _mm256_xor_si256(v, mask);

			v = _mm256_loadu_si256((__m256i*) & r->coeffs[NTT_DIM * i + NTT_DIM / 4 + 16 * j]);
			mask = _mm256_srai_epi16(v, 15);
			mask = _mm256_and_si256(mask, q16x);
			v = _mm256_add_epi16(v, mask);

			v = _mm256_sub_epi16(v, hfq);
			mask = _mm256_srai_epi16(v, 15);
			v = _mm256_add_epi16(v, mask);
			v = _mm256_xor_si256(v, mask);
			u = _mm256_add_epi16(u, v);

			v = _mm256_loadu_si256((__m256i*) & r->coeffs[NTT_DIM * i + NTT_DIM / 2 + 16 * j]);
			mask = _mm256_srai_epi16(v, 15);
			mask = _mm256_and_si256(mask, q16x);
			v = _mm256_add_epi16(v, mask);

			v = _mm256_sub_epi16(v, hfq);
			mask = _mm256_srai_epi16(v, 15);
			v = _mm256_add_epi16(v, mask);
			v = _mm256_xor_si256(v, mask);
			u = _mm256_add_epi16(u, v);

			v = _mm256_loadu_si256((__m256i*) & r->coeffs[NTT_DIM * i + 3 * NTT_DIM / 4 + 16 * j]);
			mask = _mm256_srai_epi16(v, 15);
			mask = _mm256_and_si256(mask, q16x);
			v = _mm256_add_epi16(v, mask);

			v = _mm256_sub_epi16(v, hfq);
			mask = _mm256_srai_epi16(v, 15);
			v = _mm256_add_epi16(v, mask);
			v = _mm256_xor_si256(v, mask);
			u = _mm256_add_epi16(u, v);

			v = _mm256_sub_epi16(u, q16x);

			small = _mm256_movemask_epi8(v);
			small = _pext_u32(small, 0xAAAAAAAA);
			msg[2 * i * NTT_DIM / 64 + 2 * j + 0] = small;
			msg[2 * i * NTT_DIM / 64 + 2 * j + 1] = small >> 8;
		}
}
#endif
#endif
void poly_caddq(poly *r)
{
	int i;
	__m256i * pr = (__m256i *) r->coeffs;
	__m256i q16x = _mm256_set1_epi16(PARAM_Q);
	__m256i t, d;

	for (i = 0; i < PARAM_N / 16; i++)
	{
		t = _mm256_loadu_si256((__m256i*)&r->coeffs[16 * i]);
		d = _mm256_srai_epi16(t, 15);
		d = _mm256_and_si256(q16x, d);
		t = _mm256_add_epi16(t, d);
		_mm256_storeu_si256((__m256i*)&r->coeffs[16 * i], t);
	}
}

void poly_addq(poly *r)
{
	int i;
	__m256i * pr = (__m256i *) r->coeffs;
	__m256i q16x = _mm256_set1_epi16(PARAM_Q);

	for (i = 0; i < PARAM_N / 16; i++)
		pr[i] = _mm256_add_epi16(pr[i], q16x);
}
void poly_caddq2(poly *r)
{
	int i;
	__m256i * pr = (__m256i *) r->coeffs;
	__m256i q16x = _mm256_set1_epi16(PARAM_Q);
	__m256i t, d;

	for (i = 0; i < PARAM_N / 16; i++)
	{
		t = _mm256_loadu_si256((__m256i*)&r->coeffs[16 * i]);
		d = _mm256_srai_epi16(t, 15);
		d = _mm256_and_si256(q16x, d);
		t = _mm256_add_epi16(t, d);

		d = _mm256_srai_epi16(t, 15);
		d = _mm256_and_si256(q16x, d);
		t = _mm256_add_epi16(t, d);
		_mm256_storeu_si256((__m256i*)&r->coeffs[16 * i], t);
	}
}

void poly_add(poly *r, const poly *a, const poly *b)
{
	int i;
	__m256i * pa = (__m256i *) a->coeffs;
	__m256i * pb = (__m256i *) b->coeffs;
	__m256i * pr = (__m256i *) r->coeffs;

	for (i = 0; i < PARAM_N / 16; i++)
		pr[i] = _mm256_add_epi16(pa[i], pb[i]);
}

void poly_sub(poly *r, const poly *a, const poly *b)
{
	int i;
	__m256i * pa = (__m256i *) a->coeffs;
	__m256i * pb = (__m256i *) b->coeffs;
	__m256i * pr = (__m256i *) r->coeffs;

	for (i = 0; i < PARAM_N / 16; i++)
		pr[i] = _mm256_sub_epi16(pa[i], pb[i]);
}


void poly_ntt(poly* r)
{
	int i = 0;
	for (i = 0; i < PARAM_N / NTT_DIM; i++)
		ntt(&r->coeffs[i * NTT_DIM]);
}
void poly_invntt(poly* r)
{
	int i = 0;
	for (i = 0; i < PARAM_N / NTT_DIM; i++)
		invntt(&r->coeffs[i * NTT_DIM]);
}

void poly_getmontgomery(poly* r) 
{
	int i;
	__m256i c16x = _mm256_set1_epi16(1353);


	__m256i qinv16x = _mm256_set1_epi16(QINV);
	__m256i q16x = _mm256_set1_epi16(PARAM_Q);
	__m256i t, d, dh;

	for (i = 0; i < PARAM_N / 16; i++)
	{
		t = _mm256_loadu_si256((__m256i*) & r->coeffs[16 * i]);
		dh = _mm256_mulhi_epi16(t, c16x);
		d = _mm256_mullo_epi16(t, c16x);
		d = _mm256_mullo_epi16(d, qinv16x);
		d = _mm256_mulhi_epi16(d, q16x);
		t = _mm256_sub_epi16(dh, d);
		_mm256_storeu_si256((__m256i*) & r->coeffs[16 * i], t);
	}
}
#if PARAM_N == 2048
static inline void mont_mul4(int16_t *r, const int16_t *a, const int16_t *b) // multiplication for degree 3 polynomial/4 coefficients
{
	int32_t t[14];
	int i, j;
	for (i = 0; i < NTT_DIM; i++)
	{
		for (j = 0; j < 4; j++)
			t[j] = a[j * NTT_DIM + i]; // a0,a1,a2,a3
		for (j = 0; j < 4; j++)
			t[4 + j] = b[j * NTT_DIM + i]; // b0,b1,b2,b3

		t[8] = montgomery_reduce(t[0] * t[4]);
		t[9] = montgomery_reduce((t[0] + t[1]) * (t[4] + t[5]));
		t[10] = montgomery_reduce(t[1] * t[5]);
		t[9] = t[9] - t[8] - t[10]; // <= 3 *PARAM_Q

		t[11] = montgomery_reduce(t[2] * t[6]);
		t[12] = montgomery_reduce((t[2] + t[3]) * (t[6] + t[7]));
		t[13] = montgomery_reduce(t[3] * t[7]);
		t[12] = t[12] - t[11] - t[13]; // <= 3 *PARAM_Q

		t[0] = t[0] + t[2];
		t[1] = t[1] + t[3];
		t[2] = t[4] + t[6];
		t[3] = t[5] + t[7];

		t[4] = montgomery_reduce(t[0] * t[2]);
		t[5] = montgomery_reduce((t[0] + t[1]) * (t[2] + t[3]));
		t[6] = montgomery_reduce(t[1] * t[3]);
		t[5] = t[5] - t[4] - t[6]; // <= 3 *PARAM_Q

		t[4] = t[4] - t[8] - t[11];	 // <= 3 *PARAM_Q
		t[5] = t[5] - t[9] - t[12];	 // <= 7 *PARAM_Q
		t[6] = t[6] - t[10] - t[13]; // <= 3 *PARAM_Q

		// lay reduction so that the coefficient will not exceed 2^15 before the next reduction
		r[i] = t[8];
		r[NTT_DIM + i] = barrett_reduce(t[9]);
		r[2 * NTT_DIM + i] = barrett_reduce(t[4] + t[10]);
		r[3 * NTT_DIM + i] = barrett_reduce(t[5]);
		r[4 * NTT_DIM + i] = barrett_reduce(t[6] + t[11]);
		r[5 * NTT_DIM + i] = barrett_reduce(t[12]);
		r[6 * NTT_DIM + i] = t[13];
	}
}
#else
static inline void mont_mul4(int16_t* r, const int16_t* a, const int16_t* b)//multiplication for degree 3 polynomial/4 coefficients
{
	__m256i t[14];
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i dh, d;

	__m256i c16x = _mm256_set1_epi16(39);

	__m256i one16x = _mm256_set1_epi16(1);

	int i, j;
	for (i = 0; i < NTT_DIM / 16; i++)
	{
		for (j = 0; j < 4; j++)
			t[j] = _mm256_loadu_si256((__m256i*) & a[j * NTT_DIM + 16 * i]);//a0,a1,a2,a3
		for (j = 0; j < 4; j++)
			t[4 + j] = _mm256_loadu_si256((__m256i*) & b[j * NTT_DIM + 16 * i]);//b0,b1,b2,b3


		dh = _mm256_mulhi_epi16(t[0], t[4]);
		d = _mm256_mullo_epi16(t[0], t[4]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[8] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		t[9] = _mm256_add_epi16(t[0], t[1]);
		t[10] = _mm256_add_epi16(t[4], t[5]);

		dh = _mm256_mulhi_epi16(t[9], t[10]);
		d = _mm256_mullo_epi16(t[9], t[10]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[9] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		dh = _mm256_mulhi_epi16(t[1], t[5]);
		d = _mm256_mullo_epi16(t[1], t[5]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[10] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		t[9] = _mm256_sub_epi16(t[9], t[8]);
		t[9] = _mm256_sub_epi16(t[9], t[10]);


		dh = _mm256_mulhi_epi16(t[2], t[6]);
		d = _mm256_mullo_epi16(t[2], t[6]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[11] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		t[12] = _mm256_add_epi16(t[2], t[3]);
		t[13] = _mm256_add_epi16(t[6], t[7]);

		dh = _mm256_mulhi_epi16(t[12], t[13]);
		d = _mm256_mullo_epi16(t[12], t[13]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[12] = _mm256_sub_epi16(dh, d);//[-Q,Q]



		dh = _mm256_mulhi_epi16(t[3], t[7]);
		d = _mm256_mullo_epi16(t[3], t[7]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[13] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		t[12] = _mm256_sub_epi16(t[12], t[11]);
		t[12] = _mm256_sub_epi16(t[12], t[13]);

		t[0] = _mm256_add_epi16(t[0], t[2]);
		t[1] = _mm256_add_epi16(t[1], t[3]);
		t[2] = _mm256_add_epi16(t[4], t[6]);
		t[3] = _mm256_add_epi16(t[5], t[7]);


		dh = _mm256_mulhi_epi16(t[0], t[2]);
		d = _mm256_mullo_epi16(t[0], t[2]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[4] = _mm256_sub_epi16(dh, d);//[-Q,Q]




		t[5] = _mm256_add_epi16(t[0], t[1]);
		t[6] = _mm256_add_epi16(t[2], t[3]);

		dh = _mm256_mulhi_epi16(t[5], t[6]);
		d = _mm256_mullo_epi16(t[5], t[6]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[5] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		dh = _mm256_mulhi_epi16(t[1], t[3]);
		d = _mm256_mullo_epi16(t[1], t[3]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[6] = _mm256_sub_epi16(dh, d);//[-Q,Q]



		t[5] = _mm256_sub_epi16(t[5], t[4]);
		t[5] = _mm256_sub_epi16(t[5], t[6]);

		t[4] = _mm256_sub_epi16(t[4], t[8]);
		t[4] = _mm256_sub_epi16(t[4], t[11]);

		t[5] = _mm256_sub_epi16(t[5], t[9]);
		t[5] = _mm256_sub_epi16(t[5], t[12]);

		t[6] = _mm256_sub_epi16(t[6], t[10]);
		t[6] = _mm256_sub_epi16(t[6], t[13]);

		t[10] = _mm256_add_epi16(t[4], t[10]);
		t[11] = _mm256_add_epi16(t[6], t[11]);
		
#if PARAM_Q == 3329
// lay reduction so that the coefficient will not exceed 2^15 before the next reduction
		d = _mm256_mulhi_epi16(t[9], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, 1);
		d = _mm256_mullo_epi16(d, vq16x);
		t[9] = _mm256_sub_epi16(t[9], d);

		d = _mm256_mulhi_epi16(t[10], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, 1);
		d = _mm256_mullo_epi16(d, vq16x);
		t[10] = _mm256_sub_epi16(t[10], d);


		d = _mm256_mulhi_epi16(t[5], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, 1);
		d = _mm256_mullo_epi16(d, vq16x);
		t[5] = _mm256_sub_epi16(t[5], d);


		d = _mm256_mulhi_epi16(t[11], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, 1);
		d = _mm256_mullo_epi16(d, vq16x);
		t[11] = _mm256_sub_epi16(t[11], d);


		d = _mm256_mulhi_epi16(t[12], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, 1);
		d = _mm256_mullo_epi16(d, vq16x);
		t[12] = _mm256_sub_epi16(t[12], d);
#endif

		_mm256_storeu_si256((__m256i*) & r[16 * i], t[8]);
		_mm256_storeu_si256((__m256i*) & r[NTT_DIM + 16 * i], t[9]);
		_mm256_storeu_si256((__m256i*) & r[2 * NTT_DIM + 16 * i], t[10]);
		_mm256_storeu_si256((__m256i*) & r[3 * NTT_DIM + 16 * i], t[5]);
		_mm256_storeu_si256((__m256i*) & r[4 * NTT_DIM + 16 * i], t[11]);
		_mm256_storeu_si256((__m256i*) & r[5 * NTT_DIM + 16 * i], t[12]);
		_mm256_storeu_si256((__m256i*) & r[6 * NTT_DIM + 16 * i], t[13]);
	}
}
#endif

static inline void mont_mul8(int16_t* r, const int16_t* a, const int16_t* b)
{
	ALIGN(32) int16_t t1[7 * NTT_DIM];
	ALIGN(32) int16_t t2[7 * NTT_DIM];
	__m256i v[6], d;

	__m256i c16x = _mm256_set1_epi16(39);

	__m256i one16x = _mm256_set1_epi16(1);
	__m256i q16x = _mm256_set1_epi16(PARAM_Q);

	int i;
	for (i = 0; i < 4 * NTT_DIM / 16; i++)
	{
		v[0] = _mm256_loadu_si256((__m256i*) & a[16 * i]);//a0,a1,a2,a3
		v[1] = _mm256_loadu_si256((__m256i*) & a[4 * NTT_DIM + 16 * i]);
		v[0] = _mm256_add_epi16(v[0], v[1]);

		v[1] = _mm256_load_si256((__m256i*) & b[16 * i]);//a0,a1,a2,a3
		v[2] = _mm256_load_si256((__m256i*) & b[4 * NTT_DIM + 16 * i]);
		v[1] = _mm256_add_epi16(v[1], v[2]);

		_mm256_storeu_si256((__m256i*) & t1[16 * i], v[0]);
		_mm256_storeu_si256((__m256i*) & t2[16 * i], v[1]);
	}
	mont_mul4(t1, t1, t2);
	mont_mul4(&r[8 * NTT_DIM], &a[4 * NTT_DIM], &b[4 * NTT_DIM]);
	mont_mul4(r, a, b);

	for (i = 0; i < NTT_DIM / 16; i++)
	{
		v[0] = _mm256_loadu_si256((__m256i*) & t1[3 * NTT_DIM + 16 * i]);
		v[1] = _mm256_loadu_si256((__m256i*) & r[3 * NTT_DIM + 16 * i]);
		v[2] = _mm256_loadu_si256((__m256i*) & r[11 * NTT_DIM + 16 * i]);
		v[0] = _mm256_sub_epi16(v[0], v[1]);
		v[0] = _mm256_sub_epi16(v[0], v[2]);

		//lazy reduction, barrat reduction
		d = _mm256_mulhi_epi16(v[0], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, 1);
		d = _mm256_mullo_epi16(d, q16x);
		v[0] = _mm256_sub_epi16(v[0], d);


		_mm256_storeu_si256((__m256i*) & r[7 * NTT_DIM + 16 * i], v[0]);
	}
	for (i = 0; i < 3 * NTT_DIM / 16; i++)
	{
		v[0] = _mm256_loadu_si256((__m256i*) & r[4 * NTT_DIM + 16 * i]);
		v[1] = _mm256_loadu_si256((__m256i*) & t1[16 * i]);
		v[2] = _mm256_loadu_si256((__m256i*) & r[16 * i]);
		v[3] = _mm256_loadu_si256((__m256i*) & r[8 * NTT_DIM + 16 * i]);
		v[4] = _mm256_loadu_si256((__m256i*) & t1[4 * NTT_DIM + 16 * i]);
		v[5] = _mm256_loadu_si256((__m256i*) & r[12 * NTT_DIM + 16 * i]);

		v[1] = _mm256_add_epi16(v[0], v[1]);
		v[1] = _mm256_sub_epi16(v[1], v[2]);
		v[1] = _mm256_sub_epi16(v[1], v[3]);

		//lazy reduction, barrat reduction
		d = _mm256_mulhi_epi16(v[1], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, 1);
		d = _mm256_mullo_epi16(d, q16x);
		v[1] = _mm256_sub_epi16(v[1], d);


		v[3] = _mm256_add_epi16(v[3], v[4]);
		v[3] = _mm256_sub_epi16(v[3], v[0]);
		v[3] = _mm256_sub_epi16(v[3], v[5]);

		//lazy reduction, barrat reduction
		d = _mm256_mulhi_epi16(v[3], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, 1);
		d = _mm256_mullo_epi16(d, q16x);
		v[3] = _mm256_sub_epi16(v[3], d);

		_mm256_storeu_si256((__m256i*) & r[4 * NTT_DIM + 16 * i], v[1]);
		_mm256_storeu_si256((__m256i*) & r[8 * NTT_DIM + 16 * i], v[3]);
	}
}
static inline void mont_mul16(int16_t* r, const int16_t* a, const int16_t* b)
{
	int16_t t1[15 * NTT_DIM], t2[15 * NTT_DIM];
	__m256i v[6];
	int i;
	for (i = 0; i < 8 * NTT_DIM / 16; i++)
	{
		v[0] = _mm256_loadu_si256((__m256i*) & a[16 * i]);//a0,a1,a2,a3
		v[1] = _mm256_loadu_si256((__m256i*) & a[8 * NTT_DIM + 16 * i]);
		v[0] = _mm256_add_epi16(v[0], v[1]);

		v[1] = _mm256_loadu_si256((__m256i*) & b[16 * i]);//a0,a1,a2,a3
		v[2] = _mm256_loadu_si256((__m256i*) & b[8 * NTT_DIM + 16 * i]);
		v[1] = _mm256_add_epi16(v[1], v[2]);

		_mm256_storeu_si256((__m256i*) & t1[16 * i], v[0]);
		_mm256_storeu_si256((__m256i*) & t2[16 * i], v[1]);
	}
	mont_mul8(t1, t1, t2);
	mont_mul8(&r[16 * NTT_DIM], &a[8 * NTT_DIM], &b[8 * NTT_DIM]);//t2
	mont_mul8(r, a, b);//t0

	for (i = 0; i < NTT_DIM / 16; i++)
	{
		v[0] = _mm256_loadu_si256((__m256i*) & t1[7 * NTT_DIM + 16 * i]);
		v[1] = _mm256_loadu_si256((__m256i*) & r[7 * NTT_DIM + 16 * i]);
		v[2] = _mm256_loadu_si256((__m256i*) & r[23 * NTT_DIM + 16 * i]);
		v[0] = _mm256_sub_epi16(v[0], v[1]);
		v[0] = _mm256_sub_epi16(v[0], v[2]);
		_mm256_storeu_si256((__m256i*) & r[15 * NTT_DIM + 16 * i], v[0]);
	}
	for (i = 0; i < 7 * NTT_DIM / 16; i++)
	{
		v[0] = _mm256_loadu_si256((__m256i*) & r[8 * NTT_DIM + 16 * i]);
		v[1] = _mm256_loadu_si256((__m256i*) & t1[16 * i]);
		v[2] = _mm256_loadu_si256((__m256i*) & r[16 * i]);
		v[3] = _mm256_loadu_si256((__m256i*) & r[16 * NTT_DIM + 16 * i]);
		v[4] = _mm256_loadu_si256((__m256i*) & t1[8 * NTT_DIM + 16 * i]);
		v[5] = _mm256_loadu_si256((__m256i*) & r[24 * NTT_DIM + 16 * i]);

		v[1] = _mm256_add_epi16(v[0], v[1]);
		v[1] = _mm256_sub_epi16(v[1], v[2]);
		v[1] = _mm256_sub_epi16(v[1], v[3]);

		v[3] = _mm256_add_epi16(v[3], v[4]);
		v[3] = _mm256_sub_epi16(v[3], v[0]);
		v[3] = _mm256_sub_epi16(v[3], v[5]);

		_mm256_storeu_si256((__m256i*) & r[8 * NTT_DIM + 16 * i], v[1]);
		_mm256_storeu_si256((__m256i*) & r[16 * NTT_DIM + 16 * i], v[3]);
	}
}

#if PARAM_N/NTT_DIM == 4
void poly_mont_mul(poly* r, const poly* a, const poly* b)
{
	__m256i t[14];
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i dh, d;
	__m256i vntty;


	__m256i c16x = _mm256_set1_epi16(39);

	__m256i one16x = _mm256_set1_epi16(1);

	int i, j;
	for (i = 0; i < NTT_DIM / 16; i++)
	{

		for (j = 0; j < 4; j++)
			t[j] = _mm256_loadu_si256((__m256i*) & a->coeffs[j * NTT_DIM + 16 * i]);//a0,a1,a2,a3
		for (j = 0; j < 4; j++)
			t[4 + j] = _mm256_loadu_si256((__m256i*) & b->coeffs[j * NTT_DIM + 16 * i]);//b0,b1,b2,b3


		dh = _mm256_mulhi_epi16(t[0], t[4]);
		d = _mm256_mullo_epi16(t[0], t[4]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[8] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		t[9] = _mm256_add_epi16(t[0], t[1]);
		t[10] = _mm256_add_epi16(t[4], t[5]);

		dh = _mm256_mulhi_epi16(t[9], t[10]);
		d = _mm256_mullo_epi16(t[9], t[10]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[9] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		dh = _mm256_mulhi_epi16(t[1], t[5]);
		d = _mm256_mullo_epi16(t[1], t[5]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[10] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		t[9] = _mm256_sub_epi16(t[9], t[8]);
		t[9] = _mm256_sub_epi16(t[9], t[10]);


		dh = _mm256_mulhi_epi16(t[2], t[6]);
		d = _mm256_mullo_epi16(t[2], t[6]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[11] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		t[12] = _mm256_add_epi16(t[2], t[3]);
		t[13] = _mm256_add_epi16(t[6], t[7]);

		dh = _mm256_mulhi_epi16(t[12], t[13]);
		d = _mm256_mullo_epi16(t[12], t[13]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[12] = _mm256_sub_epi16(dh, d);//[-Q,Q]



		dh = _mm256_mulhi_epi16(t[3], t[7]);
		d = _mm256_mullo_epi16(t[3], t[7]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[13] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		t[12] = _mm256_sub_epi16(t[12], t[11]);
		t[12] = _mm256_sub_epi16(t[12], t[13]);

		t[0] = _mm256_add_epi16(t[0], t[2]);
		t[1] = _mm256_add_epi16(t[1], t[3]);
		t[2] = _mm256_add_epi16(t[4], t[6]);
		t[3] = _mm256_add_epi16(t[5], t[7]);


		dh = _mm256_mulhi_epi16(t[0], t[2]);
		d = _mm256_mullo_epi16(t[0], t[2]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[4] = _mm256_sub_epi16(dh, d);//[-Q,Q]




		t[5] = _mm256_add_epi16(t[0], t[1]);
		t[6] = _mm256_add_epi16(t[2], t[3]);

		dh = _mm256_mulhi_epi16(t[5], t[6]);
		d = _mm256_mullo_epi16(t[5], t[6]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[5] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		dh = _mm256_mulhi_epi16(t[1], t[3]);
		d = _mm256_mullo_epi16(t[1], t[3]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[6] = _mm256_sub_epi16(dh, d);//[-Q,Q]



		t[5] = _mm256_sub_epi16(t[5], t[4]);
		t[5] = _mm256_sub_epi16(t[5], t[6]);

		t[4] = _mm256_sub_epi16(t[4], t[8]);
		t[4] = _mm256_sub_epi16(t[4], t[11]);

		t[5] = _mm256_sub_epi16(t[5], t[9]);
		t[5] = _mm256_sub_epi16(t[5], t[12]);

		t[6] = _mm256_sub_epi16(t[6], t[10]);
		t[6] = _mm256_sub_epi16(t[6], t[13]);

		t[10] = _mm256_add_epi16(t[4], t[10]);
		t[11] = _mm256_add_epi16(t[6], t[11]);



		vntty = _mm256_loadu_si256((__m256i*) & NTT_Y[16 * i]);

		dh = _mm256_mulhi_epi16(t[11], vntty);
		d = _mm256_mullo_epi16(t[11], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[11] = _mm256_sub_epi16(dh, d);
		t[8] = _mm256_add_epi16(t[8], t[11]);

		dh = _mm256_mulhi_epi16(t[12], vntty);
		d = _mm256_mullo_epi16(t[12], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[12] = _mm256_sub_epi16(dh, d);
		t[9] = _mm256_add_epi16(t[9], t[12]);


		dh = _mm256_mulhi_epi16(t[13], vntty);
		d = _mm256_mullo_epi16(t[13], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[13] = _mm256_sub_epi16(dh, d);
		t[10] = _mm256_add_epi16(t[10], t[13]);


		// the coefficient may as large as <= 7 *PARAM_Q
// lay reduction so that the coefficient will not exceed 2^15 before the next reduction
#if PARAM_K == 2
		d = _mm256_mulhi_epi16(t[10], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, 1);
		d = _mm256_mullo_epi16(d, vq16x);
		t[10] = _mm256_sub_epi16(t[10], d);

		d = _mm256_mulhi_epi16(t[5], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, 1);
		d = _mm256_mullo_epi16(d, vq16x);
		t[5] = _mm256_sub_epi16(t[5], d);
#endif	

		_mm256_storeu_si256((__m256i*) & r->coeffs[16 * i], t[8]);
		_mm256_storeu_si256((__m256i*) & r->coeffs[NTT_DIM + 16 * i], t[9]);
		_mm256_storeu_si256((__m256i*) & r->coeffs[2 * NTT_DIM + 16 * i], t[10]);
		_mm256_storeu_si256((__m256i*) & r->coeffs[3 * NTT_DIM + 16 * i], t[5]);
	}
}
#elif PARAM_N/NTT_DIM == 8
void poly_mont_mul(poly* r, const poly* a, const poly* b)
{
	ALIGN(32) int16_t t0[7 * NTT_DIM], t1[7 * NTT_DIM], t2[7 * NTT_DIM], x;
	int i, j;
	__m256i v[6];
	__m256i vntty, dh, d;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)

#if PARAM_Q == 3329 && PARAM_K == 2
	__m256i c16x = _mm256_set1_epi16(39);
	__m256i one16x = _mm256_set1_epi16(1);
#endif
	


	for (i = 0; i < NTT_DIM / 4; i++)
	{
		v[0] = _mm256_loadu_si256((__m256i*) & a->coeffs[16 * i]);
		v[1] = _mm256_loadu_si256((__m256i*) & a->coeffs[4 * NTT_DIM + 16 * i]);
		v[0] = _mm256_add_epi16(v[0], v[1]);

		v[1] = _mm256_loadu_si256((__m256i*) & b->coeffs[16 * i]);
		v[2] = _mm256_loadu_si256((__m256i*) & b->coeffs[4 * NTT_DIM + 16 * i]);
		v[1] = _mm256_add_epi16(v[1], v[2]);

		_mm256_storeu_si256((__m256i*) & t0[16 * i], v[0]);
		_mm256_storeu_si256((__m256i*) & t1[16 * i], v[1]);
	}

	mont_mul4(t1, t0, t1);
	mont_mul4(t0, a->coeffs, b->coeffs);
	mont_mul4(t2, &a->coeffs[PARAM_N / 2], &b->coeffs[PARAM_N / 2]);

	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_loadu_si256((__m256i*) & NTT_Y[16 * j]);
		for (i = 0; i < 3; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & t0[i * NTT_DIM + 16 * j]);
			v[1] = _mm256_load_si256((__m256i*) & t1[i * NTT_DIM + 16 * j]);
			v[2] = _mm256_load_si256((__m256i*) & t2[i * NTT_DIM + 16 * j]);
			v[3] = _mm256_load_si256((__m256i*) & t0[(4 + i) * NTT_DIM + 16 * j]);
			v[4] = _mm256_load_si256((__m256i*) & t1[(4 + i) * NTT_DIM + 16 * j]);
			v[5] = _mm256_load_si256((__m256i*) & t2[(4 + i) * NTT_DIM + 16 * j]);

			v[1] = _mm256_add_epi16(v[1], v[3]);
			v[1] = _mm256_sub_epi16(v[1], v[0]);
			v[1] = _mm256_sub_epi16(v[1], v[2]);

			v[2] = _mm256_add_epi16(v[2], v[4]);
			v[2] = _mm256_sub_epi16(v[2], v[3]);
			v[2] = _mm256_sub_epi16(v[2], v[5]);

			dh = _mm256_mulhi_epi16(v[2], vntty);
			d = _mm256_mullo_epi16(v[2], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[2] = _mm256_sub_epi16(dh, d);
			v[0] = _mm256_add_epi16(v[0], v[2]);


			dh = _mm256_mulhi_epi16(v[5], vntty);
			d = _mm256_mullo_epi16(v[5], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[5] = _mm256_sub_epi16(dh, d);
			v[1] = _mm256_add_epi16(v[1], v[5]);
			

// lay reduction so that the coefficient will not exceed 2^15 before the next reduction
#if PARAM_K == 2 && PARAM_Q == 3329
			d = _mm256_mulhi_epi16(v[0], c16x);
			d = _mm256_add_epi16(d, one16x);
			d = _mm256_srai_epi16(d, 1);
			d = _mm256_mullo_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(v[0], d);

			d = _mm256_mulhi_epi16(v[1], c16x);
			d = _mm256_add_epi16(d, one16x);
			d = _mm256_srai_epi16(d, 1);
			d = _mm256_mullo_epi16(d, vq16x);
			v[1] = _mm256_sub_epi16(v[1], d);
#endif
			_mm256_storeu_si256((__m256i*) & r->coeffs[i * NTT_DIM + 16 * j], v[0]);
			_mm256_storeu_si256((__m256i*) & r->coeffs[(4 + i) * NTT_DIM + 16 * j], v[1]);
		}

		v[0] = _mm256_loadu_si256((__m256i*) & t0[3 * NTT_DIM + 16 * j]);
		v[1] = _mm256_loadu_si256((__m256i*) & t1[3 * NTT_DIM + 16 * j]);
		v[2] = _mm256_loadu_si256((__m256i*) & t2[3 * NTT_DIM + 16 * j]);

		v[1] = _mm256_sub_epi16(v[1], v[0]);
		v[1] = _mm256_sub_epi16(v[1], v[2]);

		dh = _mm256_mulhi_epi16(v[2], vntty);
		d = _mm256_mullo_epi16(v[2], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[2] = _mm256_sub_epi16(dh, d);//[-Q,Q]
		v[0] = _mm256_add_epi16(v[0], v[2]);

#if PARAM_K == 2 && PARAM_Q == 3329
		d = _mm256_mulhi_epi16(v[0], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, 1);
		d = _mm256_mullo_epi16(d, vq16x);
		v[0] = _mm256_sub_epi16(v[0], d);

		d = _mm256_mulhi_epi16(v[1], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, 1);
		d = _mm256_mullo_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(v[1], d);
#endif

		_mm256_storeu_si256((__m256i*) & r->coeffs[3 * NTT_DIM + 16 * j], v[0]);
		_mm256_storeu_si256((__m256i*) & r->coeffs[7 * NTT_DIM + 16 * j], v[1]);
	}
}
#elif PARAM_N/NTT_DIM == 16
void poly_mont_mul(poly* r, const poly* a, const poly* b)
{
	ALIGN(32) int16_t t0[15 * NTT_DIM];
	ALIGN(32) int16_t t1[15 * NTT_DIM];
	ALIGN(32) int16_t t2[15 * NTT_DIM];
	int16_t x;
	int i, j;
	__m256i v[6];
	__m256i vntty, dh, d;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)

	for (i = 0; i < NTT_DIM / 2; i++)
	{
		v[0] = _mm256_loadu_si256((__m256i*) & a->coeffs[16 * i]);
		v[1] = _mm256_loadu_si256((__m256i*) & a->coeffs[8 * NTT_DIM + 16 * i]);
		v[0] = _mm256_add_epi16(v[0], v[1]);

		v[1] = _mm256_loadu_si256((__m256i*) & b->coeffs[16 * i]);
		v[2] = _mm256_loadu_si256((__m256i*) & b->coeffs[8 * NTT_DIM + 16 * i]);
		v[1] = _mm256_add_epi16(v[1], v[2]);

		_mm256_storeu_si256((__m256i*) & t0[16 * i], v[0]);
		_mm256_storeu_si256((__m256i*) & t1[16 * i], v[1]);
	}

	mont_mul8(t1, t0, t1);
	mont_mul8(t0, a->coeffs, b->coeffs);
	mont_mul8(t2, &a->coeffs[PARAM_N / 2], &b->coeffs[PARAM_N / 2]);

	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_loadu_si256((__m256i*) & NTT_Y[16 * j]);
		for (i = 0; i < 7; i++)
		{
			v[0] = _mm256_loadu_si256((__m256i*) & t0[i * NTT_DIM + 16 * j]);
			v[1] = _mm256_loadu_si256((__m256i*) & t1[i * NTT_DIM + 16 * j]);
			v[2] = _mm256_loadu_si256((__m256i*) & t2[i * NTT_DIM + 16 * j]);
			v[3] = _mm256_loadu_si256((__m256i*) & t0[(8 + i) * NTT_DIM + 16 * j]);
			v[4] = _mm256_loadu_si256((__m256i*) & t1[(8 + i) * NTT_DIM + 16 * j]);
			v[5] = _mm256_loadu_si256((__m256i*) & t2[(8 + i) * NTT_DIM + 16 * j]);

			v[1] = _mm256_add_epi16(v[1], v[3]);
			v[1] = _mm256_sub_epi16(v[1], v[0]);
			v[1] = _mm256_sub_epi16(v[1], v[2]);

			v[2] = _mm256_add_epi16(v[2], v[4]);
			v[2] = _mm256_sub_epi16(v[2], v[3]);
			v[2] = _mm256_sub_epi16(v[2], v[5]);

			dh = _mm256_mulhi_epi16(v[2], vntty);
			d = _mm256_mullo_epi16(v[2], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[2] = _mm256_sub_epi16(dh, d);
			v[0] = _mm256_add_epi16(v[0], v[2]);


			dh = _mm256_mulhi_epi16(v[5], vntty);
			d = _mm256_mullo_epi16(v[5], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[5] = _mm256_sub_epi16(dh, d);
			v[1] = _mm256_add_epi16(v[1], v[5]);


			_mm256_storeu_si256((__m256i*) & r->coeffs[i * NTT_DIM + 16 * j], v[0]);
			_mm256_storeu_si256((__m256i*) & r->coeffs[(8 + i) * NTT_DIM + 16 * j], v[1]);
		}

		v[0] = _mm256_loadu_si256((__m256i*) & t0[7 * NTT_DIM + 16 * j]);
		v[1] = _mm256_loadu_si256((__m256i*) & t1[7 * NTT_DIM + 16 * j]);
		v[2] = _mm256_loadu_si256((__m256i*) & t2[7 * NTT_DIM + 16 * j]);

		v[1] = _mm256_sub_epi16(v[1], v[0]);
		v[1] = _mm256_sub_epi16(v[1], v[2]);

		dh = _mm256_mulhi_epi16(v[2], vntty);
		d = _mm256_mullo_epi16(v[2], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[2] = _mm256_sub_epi16(dh, d);//[-Q,Q]
		v[0] = _mm256_add_epi16(v[0], v[2]);

		_mm256_storeu_si256((__m256i*) & r->coeffs[7 * NTT_DIM + 16 * j], v[0]);
		_mm256_storeu_si256((__m256i*) & r->coeffs[15 * NTT_DIM + 16 * j], v[1]);
	}
}
#elif PARAM_N/NTT_DIM == 32
void poly_mont_mul(poly* r, const poly* a, const poly* b)
{
	int16_t t0[31 * NTT_DIM], t1[31 * NTT_DIM], t2[31 * NTT_DIM], x;
	int i, j;
	__m256i v[6];
	__m256i vntty, dh, d;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)

	for (i = 0; i < NTT_DIM; i++)
	{
		v[0] = _mm256_loadu_si256((__m256i*) & a->coeffs[16 * i]);
		v[1] = _mm256_loadu_si256((__m256i*) & a->coeffs[16 * NTT_DIM + 16 * i]);
		v[0] = _mm256_add_epi16(v[0], v[1]);

		v[1] = _mm256_loadu_si256((__m256i*) & b->coeffs[16 * i]);
		v[2] = _mm256_loadu_si256((__m256i*) & b->coeffs[16 * NTT_DIM + 16 * i]);
		v[1] = _mm256_add_epi16(v[1], v[2]);

		_mm256_storeu_si256((__m256i*) & t0[16 * i], v[0]);
		_mm256_storeu_si256((__m256i*) & t1[16 * i], v[1]);
	}

	mont_mul16(t1, t0, t1);
	mont_mul16(t0, a->coeffs, b->coeffs);
	mont_mul16(t2, &a->coeffs[PARAM_N / 2], &b->coeffs[PARAM_N / 2]);

	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_loadu_si256((__m256i*) & NTT_Y[16 * j]);
		for (i = 0; i < 15; i++)
		{
			v[0] = _mm256_loadu_si256((__m256i*) & t0[i * NTT_DIM + 16 * j]);
			v[1] = _mm256_loadu_si256((__m256i*) & t1[i * NTT_DIM + 16 * j]);
			v[2] = _mm256_loadu_si256((__m256i*) & t2[i * NTT_DIM + 16 * j]);
			v[3] = _mm256_loadu_si256((__m256i*) & t0[(16 + i) * NTT_DIM + 16 * j]);
			v[4] = _mm256_loadu_si256((__m256i*) & t1[(16 + i) * NTT_DIM + 16 * j]);
			v[5] = _mm256_loadu_si256((__m256i*) & t2[(16 + i) * NTT_DIM + 16 * j]);

			v[1] = _mm256_add_epi16(v[1], v[3]);
			v[1] = _mm256_sub_epi16(v[1], v[0]);
			v[1] = _mm256_sub_epi16(v[1], v[2]);

			v[2] = _mm256_add_epi16(v[2], v[4]);
			v[2] = _mm256_sub_epi16(v[2], v[3]);
			v[2] = _mm256_sub_epi16(v[2], v[5]);

			dh = _mm256_mulhi_epi16(v[2], vntty);
			d = _mm256_mullo_epi16(v[2], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[2] = _mm256_sub_epi16(dh, d);
			v[0] = _mm256_add_epi16(v[0], v[2]);


			dh = _mm256_mulhi_epi16(v[5], vntty);
			d = _mm256_mullo_epi16(v[5], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[5] = _mm256_sub_epi16(dh, d);
			v[1] = _mm256_add_epi16(v[1], v[5]);


			_mm256_storeu_si256((__m256i*) & r->coeffs[i * NTT_DIM + 16 * j], v[0]);
			_mm256_storeu_si256((__m256i*) & r->coeffs[(16 + i) * NTT_DIM + 16 * j], v[1]);
		}

		v[0] = _mm256_loadu_si256((__m256i*) & t0[15 * NTT_DIM + 16 * j]);
		v[1] = _mm256_loadu_si256((__m256i*) & t1[15 * NTT_DIM + 16 * j]);
		v[2] = _mm256_loadu_si256((__m256i*) & t2[15 * NTT_DIM + 16 * j]);

		v[1] = _mm256_sub_epi16(v[1], v[0]);
		v[1] = _mm256_sub_epi16(v[1], v[2]);

		dh = _mm256_mulhi_epi16(v[2], vntty);
		d = _mm256_mullo_epi16(v[2], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[2] = _mm256_sub_epi16(dh, d);//[-Q,Q]
		v[0] = _mm256_add_epi16(v[0], v[2]);

		_mm256_storeu_si256((__m256i*) & r->coeffs[15 * NTT_DIM + 16 * j], v[0]);
		_mm256_storeu_si256((__m256i*) & r->coeffs[31 * NTT_DIM + 16 * j], v[1]);
	}
}
#endif
