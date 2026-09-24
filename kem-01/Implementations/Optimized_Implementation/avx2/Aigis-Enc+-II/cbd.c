#include "cbd.h"
#include <immintrin.h>
#include<stdio.h>

void cbd1(int16_t *r, const uint8_t *buf) {
    __m256i f0, f1, f2, f3;
    __m256i t0, t1, t2, t3;
    const __m256i mask55 = _mm256_set1_epi8(0x55);
    const __m256i mask03 = _mm256_set1_epi8(0x03);
    const __m256i mask01 = _mm256_set1_epi8(0x01);
    for (int i = 0; i < PARAM_N / 128 ; i++)
    {
        f0 = _mm256_load_si256(buf + 32 * i);

        f1 = _mm256_srli_epi16(f0,1);
        f0 = _mm256_and_si256(f0, mask55);
        f1 = _mm256_and_si256(f1, mask55);
        f0 = _mm256_add_epi16(f0, mask55);
        f0 = _mm256_sub_epi16(f0, f1);

        t0 = _mm256_and_si256(f0, mask03);
        t1 = _mm256_srli_epi16(f0, 2);
        t1 = _mm256_and_si256(t1, mask03);
        t2 = _mm256_srli_epi16(f0, 4);
        t2 = _mm256_and_si256(t2, mask03);
        t3 = _mm256_srli_epi16(f0, 6);
        t3 = _mm256_and_si256(t3, mask03);

        t0 = _mm256_sub_epi8(t0, mask01);
        t1 = _mm256_sub_epi8(t1, mask01);
        t2 = _mm256_sub_epi8(t2, mask01);
        t3 = _mm256_sub_epi8(t3, mask01);

        f0 = _mm256_unpacklo_epi8(t0,t1);
        f1 = _mm256_unpackhi_epi8(t0,t1);
        f2 = _mm256_unpacklo_epi8(t2,t3);
        f3 = _mm256_unpackhi_epi8(t2,t3);

        t0 = _mm256_unpacklo_epi16(f0,f2);
        t1 = _mm256_unpackhi_epi16(f0,f2);
        t2 = _mm256_unpacklo_epi16(f1,f3);
        t3 = _mm256_unpackhi_epi16(f1,f3);

        f0 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t0));
        f1 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t0, 1));
        f2 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t1));
        f3 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t1, 1));

        _mm256_store_si256(r + 128 * i, f0);
        _mm256_store_si256(r + 128 * i + 16, f2);
        _mm256_store_si256(r + 128 * i + 64, f1);
        _mm256_store_si256(r + 128 * i + 80, f3);

        f0 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t2));
        f1 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t2, 1));
        f2 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t3));
        f3 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t3, 1));

        _mm256_store_si256(r + 128 * i + 32, f0);
        _mm256_store_si256(r + 128 * i + 48, f2);
        _mm256_store_si256(r + 128 * i + 96, f1);
        _mm256_store_si256(r + 128 * i + 112, f3);
    }
    
}

void cbd2(int16_t *r, const uint8_t *buf) {
    __m256i f0, f1, f2, f3;
	const __m256i mask55 = _mm256_set1_epi8(0x55);
	const __m256i mask33 = _mm256_set1_epi8(0x33);
	const __m256i mask03 = _mm256_set1_epi8(0x03);
	const __m256i mask0F = _mm256_set1_epi8(0x0F);
    for (int i = 0; i < PARAM_N / 64; i++) {
        f0 = _mm256_load_si256(buf + 32 * i);

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

        _mm256_store_si256(r + 64 * i, f0);
        _mm256_store_si256(r + 64 * i + 16, f2);
        _mm256_store_si256(r + 64 * i + 32, f1);
        _mm256_store_si256(r + 64 * i + 48, f3);
    }
}


void cbd3(int16_t  r[PARAM_N], const uint8_t* buf)
{
	ALIGN(32) int16_t t[PARAM_N];
	__m256i d0, d1;
	cbd1(r, buf);
	cbd2(t, buf + PARAM_N/4);
	for (int i = 0; i < PARAM_N/16; i++)
	{
		d0 = _mm256_loadu_si256(&r[16 * i]);
		d1 = _mm256_loadu_si256(&t[16 * i]);
		d0 = _mm256_add_epi16(d0, d1);
		_mm256_storeu_si256((__m256i*) & r[16 * i], d0);
	}
}

void cbd4(int16_t *r, const uint8_t *buf)
{
	__m256i mask55 = _mm256_set1_epi8(0x55);
	__m256i mask33 = _mm256_set1_epi8(0x33);
	__m256i mask0f = _mm256_set1_epi8(0x0f);
	__m256i t, d, e;

	for (int i = 0; i < PARAM_N / 32; i++)
	{
		e = _mm256_loadu_si256(buf + i * 32);
		d = _mm256_and_si256(e, mask55);
		t = _mm256_srli_epi16(e, 1);
		t = _mm256_and_si256(t, mask55);
		t = _mm256_add_epi8(d, t); 

		d = _mm256_and_si256(t, mask33);
		t = _mm256_srli_epi16(t, 2);
		t = _mm256_and_si256(t, mask33);
		t = _mm256_add_epi8(d, t);

		d = _mm256_and_si256(t, mask0f);
		t = _mm256_srli_epi16(t, 4);
		t = _mm256_and_si256(t, mask0f);
		t = _mm256_sub_epi8(d, t);

		d = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t));
		e = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t, 1));
		_mm256_storeu_si256(r + 32 * i, d);
		_mm256_storeu_si256(r + 32 * i + 16, e);
	}
}

void cbd5(int16_t  r[PARAM_N], const uint8_t* buf)
{
	int16_t  t[PARAM_N];
	int i;
	cbd4(r, buf);
	cbd1(t, buf + PARAM_N);
	for (i = 0; i < PARAM_N; i++)
		r[i] = r[i] + t[i];
}

void cbd6(int16_t  r[PARAM_N], const uint8_t* buf)
{
	ALIGN(32) int16_t t[PARAM_N];
	__m256i d0, d1;
	int i;
	cbd4(r, buf);
	cbd2(t, buf + PARAM_N);
	for (i = 0; i < PARAM_N/16; i++)
	{
		d0 = _mm256_loadu_si256(&r[16 * i]);
		d1 = _mm256_loadu_si256(&t[16 * i]);
		d0 = _mm256_add_epi16(d0, d1);
		_mm256_storeu_si256((__m256i*) & r[16 * i], d0);
	}
}


void cbd_etas(poly  *r, const uint8_t *buf)
{
#if ETA_S == 1
	cbd1(r->coeffs, buf);
#elif ETA_S == 2
	cbd2(r->coeffs, buf);
#elif ETA_S == 3
	cbd3(r->coeffs, buf);
#elif ETA_S == 5
	cbd5(r->coeffs, buf);
#elif ETA_S == 6
	cbd6(r->coeffs, buf);
#else
#error "polyvec_etas_getnoise() only supports ETA_S in {1,2,3,6}!\n"
#endif
}

void cbd_etae(poly  *r, const uint8_t *buf)
{
#if ETA_E == 1
	cbd1(r->coeffs, buf);
#elif ETA_E == 2
	cbd2(r->coeffs, buf);
#elif ETA_E == 3
	cbd3(r->coeffs, buf);
#elif ETA_E == 5
	cbd5(r->coeffs, buf);
#elif ETA_E == 6
	cbd6(r->coeffs, buf);
#else
#error "polyvec_etae_getnoise() only supports ETA_E in {1,2,3,6}!\n"
#endif
}
