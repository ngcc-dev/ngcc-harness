#include "poly.h"
#include <immintrin.h>
#include <string.h>

#include "api.h"

void cbd1(int16_t *r, const uint8_t *buf) {
    __m256i f0, f1, f2, f3;
    __m256i t0, t1, t2, t3;
    const __m256i mask55 = _mm256_set1_epi8(0x55);
    const __m256i mask03 = _mm256_set1_epi8(0x03);
    const __m256i mask01 = _mm256_set1_epi8(0x01);
    for (int i = 0; i < PARAM_N / 128 ; i++)
    {
        f0 = _mm256_loadu_si256(buf + 32 * i);

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

        _mm256_storeu_si256(r + 128 * i, f0);
        _mm256_storeu_si256(r + 128 * i + 16, f2);
        _mm256_storeu_si256(r + 128 * i + 64, f1);
        _mm256_storeu_si256(r + 128 * i + 80, f3);

        f0 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t2));
        f1 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t2, 1));
        f2 = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(t3));
        f3 = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(t3, 1));

        _mm256_storeu_si256(r + 128 * i + 32, f0);
        _mm256_storeu_si256(r + 128 * i + 48, f2);
        _mm256_storeu_si256(r + 128 * i + 96, f1);
        _mm256_storeu_si256(r + 128 * i + 112, f3);
    }
    
}

void cbd2(int16_t *r, const uint8_t *buf) {
    __m256i f0, f1, f2, f3;
	const __m256i mask55 = _mm256_set1_epi8(0x55);
	const __m256i mask33 = _mm256_set1_epi8(0x33);
	const __m256i mask03 = _mm256_set1_epi8(0x03);
	const __m256i mask0F = _mm256_set1_epi8(0x0F);
    for (int i = 0; i < PARAM_N / 64; i++) {
        f0 = _mm256_loadu_si256(buf + 32 * i);

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

        _mm256_storeu_si256(r + 64 * i, f0);
        _mm256_storeu_si256(r + 64 * i + 16, f2);
        _mm256_storeu_si256(r + 64 * i + 32, f1);
        _mm256_storeu_si256(r + 64 * i + 48, f3);
    }
}

void cbd3(int16_t  r[PARAM_N], const uint8_t* buf)
{
	int16_t t[PARAM_N];
	int i;
	cbd1(r, buf);
	cbd2(t, buf + PARAM_N/4);
	for (i = 0; i < PARAM_N; i++)
		r[i] = r[i] + t[i];
}

void cbd4(int16_t *r, const uint8_t *buf)
{
	int i;
	__m256i x, t, lo, hi;
	const __m256i mask55 = _mm256_set1_epi8(0x55);
	const __m256i mask33 = _mm256_set1_epi8(0x33);
	const __m256i mask0f = _mm256_set1_epi8(0x0f);

	for (i = 0; i < PARAM_N / 32; i++)
	{
		x = _mm256_loadu_si256((const __m256i *)&buf[32 * i]);

		t = _mm256_srli_epi16(x, 1);
		t = _mm256_and_si256(t, mask55);
		x = _mm256_sub_epi8(x, t);

		t = _mm256_srli_epi16(x, 2);
		t = _mm256_and_si256(t, mask33);
		x = _mm256_and_si256(x, mask33);
		x = _mm256_add_epi8(x, t);

		t = _mm256_srli_epi16(x, 4);
		t = _mm256_and_si256(t, mask0f);
		x = _mm256_and_si256(x, mask0f);
		x = _mm256_sub_epi8(x, t);

		lo = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(x));
		hi = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(x, 1));

		_mm256_storeu_si256((__m256i *)&r[32 * i], lo);
		_mm256_storeu_si256((__m256i *)&r[32 * i + 16], hi);
	}
}
void cbd7(int16_t r[PARAM_N], const uint8_t* buf)
{
	int16_t t[PARAM_N];
	cbd4(r, buf);
	cbd3(t, buf + PARAM_N);
	for (int i = 0; i < PARAM_N; i++)
		r[i] = r[i] + t[i];
}

int ternary3(int16_t *r, const uint8_t *buf, int n, int buf_len) {
	int pos = 0;
	int ctr;
	ALIGN(32) uint16_t t[16];
	ALIGN(32) uint16_t c0[16];
	ALIGN(32) uint16_t c1[16];
	ALIGN(32) uint16_t c2[16];
	ALIGN(32) uint16_t c3[16];
	ALIGN(32) uint16_t c4[16];
	int i = 0;
	__m256i f0,f1,f2;
	__m256i num1 = _mm256_set1_epi16(1);
	__m256i num3 = _mm256_set1_epi16(3);
	__m256i numAAAB = _mm256_set1_epi16(0xAAAB);
	while(pos + 80 < n && i + 16 < buf_len) {
		ctr = 0;
		while (ctr < 16 && i < buf_len) {
			if (buf[i] < 243) {
				t[ctr] = buf[i];
				ctr ++;
			}
			i++;
		}

		if (ctr < 16) {
			int8_t coeffs[5];
			for (int j = 0; j < ctr && pos < n; ++j) {
				uint32_t y = t[j], q;
				q = (y * 0xAAABu) >> 17; coeffs[0] = 1 - (y - 3*q); y = q;
				q = (y * 0xAAABu) >> 17; coeffs[1] = 1 - (y - 3*q); y = q;
				q = (y * 0xAAABu) >> 17; coeffs[2] = 1 - (y - 3*q); y = q;
				q = (y * 0xAAABu) >> 17; coeffs[3] = 1 - (y - 3*q); y = q;
				coeffs[4] = 1 - y;
				for (int k = 0; k < 5 && pos < n; ++k) {
					r[pos++] = coeffs[k];
				}
			}
			break;
		}

		f0 = _mm256_load_si256(t);

		f1 = _mm256_mulhi_epu16(f0,numAAAB);
		f1 = _mm256_srli_epi16(f1,1);
		f2 = _mm256_mullo_epi16(f1,num3);
		f2 = _mm256_sub_epi16(f0,f2);
		f2 = _mm256_sub_epi16(num1,f2);
		_mm256_store_si256(c0,f2);
		f0 = f1;

		f1 = _mm256_mulhi_epu16(f0,numAAAB);
		f1 = _mm256_srli_epi16(f1,1);
		f2 = _mm256_mullo_epi16(f1,num3);
		f2 = _mm256_sub_epi16(f0,f2);
		f2 = _mm256_sub_epi16(num1,f2);
		_mm256_store_si256(c1,f2);
		f0 = f1;

		f1 = _mm256_mulhi_epu16(f0,numAAAB);
		f1 = _mm256_srli_epi16(f1,1);
		f2 = _mm256_mullo_epi16(f1,num3);
		f2 = _mm256_sub_epi16(f0,f2);
		f2 = _mm256_sub_epi16(num1,f2);
		_mm256_store_si256(c2,f2);
		f0 = f1;

		f1 = _mm256_mulhi_epu16(f0,numAAAB);
		f1 = _mm256_srli_epi16(f1,1);
		f2 = _mm256_mullo_epi16(f1,num3);
		f2 = _mm256_sub_epi16(f0,f2);
		f2 = _mm256_sub_epi16(num1,f2);
		_mm256_store_si256(c3,f2);


		f1 = _mm256_sub_epi16(num1,f1);
		_mm256_store_si256(c4,f1);

		for (int j = 0; j < 16; ++j) {
			r[pos++] = c0[j];
			r[pos++] = c1[j];
			r[pos++] = c2[j];
			r[pos++] = c3[j];
			r[pos++] = c4[j];
		}
	}

	int8_t coeffs[5];
	while(pos < n && i < buf_len) {
		uint8_t x = buf[i++];
		if (x < 243) {
			uint32_t y = x, q;
			q = (y * 0xAAABu) >> 17; coeffs[0] = 1 - (y - 3*q); y = q;
			q = (y * 0xAAABu) >> 17; coeffs[1] = 1 - (y - 3*q); y = q;
			q = (y * 0xAAABu) >> 17; coeffs[2] = 1 - (y - 3*q); y = q;
			q = (y * 0xAAABu) >> 17; coeffs[3] = 1 - (y - 3*q); y = q;
			coeffs[4] = 1 - y;
			for (int j = 0; j < 5 && pos < n; ++j) {
				r[pos++] = coeffs[j];
			}
		}
	}
	return pos;
}

void poly_binomial_dist1(poly *r, const uint8_t *seed, uint8_t nonce)
{
	ALIGN(32) uint8_t buf[PARAM_N / 4];
	ALIGN(32) uint8_t extseed[SEED_BYTES + 1];
	int i;

	for (i = 0; i < SEED_BYTES; i++)
		extseed[i] = seed[i];
	extseed[SEED_BYTES] = nonce;

	KDF(buf, PARAM_N / 4, extseed, SEED_BYTES + 1);

	cbd1(r->coeffs, buf);
}

void poly_binomial_dist2(poly *r, const uint8_t *seed, uint8_t nonce)
{
	ALIGN(32) uint8_t buf[PARAM_N / 2];
	ALIGN(32) uint8_t extseed[SEED_BYTES + 1];
	for (int i = 0; i < SEED_BYTES; i++)
		extseed[i] = seed[i];
	extseed[SEED_BYTES] = nonce;

	KDF(buf, PARAM_N / 2, extseed, SEED_BYTES + 1);
	cbd2(r->coeffs, buf);
}
void poly_binomial_dist3(poly *r, const uint8_t *seed, uint8_t nonce)
{
	uint8_t buf[PARAM_N / 2 + PARAM_N / 4];
	uint8_t extseed[SEED_BYTES + 1];
	int i;
	for (i = 0; i < SEED_BYTES; i++)
		extseed[i] = seed[i];
	extseed[SEED_BYTES] = nonce;

	KDF(buf, PARAM_N / 2 + PARAM_N / 4, extseed, SEED_BYTES + 1);
	cbd3(r->coeffs, buf);
}

void poly_binomial_dist4(poly *r, const uint8_t *seed, uint8_t nonce)
{
	uint8_t buf[PARAM_N];
	uint8_t extseed[SEED_BYTES + 1];
	int i;
	for (i = 0; i < SEED_BYTES; i++)
		extseed[i] = seed[i];
	extseed[SEED_BYTES] = nonce;

	KDF(buf, PARAM_N, extseed, SEED_BYTES + 1);
	cbd4(r->coeffs, buf);
}
void poly_binomial_dist7(poly *r, const uint8_t *seed, uint8_t nonce)
{
	uint8_t buf[PARAM_N + PARAM_N / 2 + PARAM_N / 4];
	uint8_t extseed[SEED_BYTES + 1];
	int i;
	for (i = 0; i < SEED_BYTES; i++)
		extseed[i] = seed[i];
	extseed[SEED_BYTES] = nonce;

	KDF(buf, PARAM_N + PARAM_N / 2 + PARAM_N / 4, extseed, SEED_BYTES + 1);
	cbd7(r->coeffs, buf);
}
void poly_bias8_ternary(poly *r, const uint8_t *seed, uint8_t nonce)
{

	ALIGN(32) uint8_t buf[3 * PARAM_N / 8];
	ALIGN(32) uint8_t extseed[SEED_BYTES + 1];
    const __m256i mask01_epi32 = _mm256_set1_epi32(0x1);
    const __m256i masksrlv = _mm256_set_epi32(7,6,5,4,3,2,1,0);
	const __m256i zero = _mm256_setzero_si256();
	__m256i f,f0,f1,f2,f3;
    __m128i t;
    int pos;

    for (int i = 0; i < SEED_BYTES; i++)
		extseed[i] = seed[i];
	extseed[SEED_BYTES] = nonce;

	KDF(buf, 3 * PARAM_N / 8, extseed, SEED_BYTES + 1);

	cbd1(r->coeffs, buf);

    pos = 2 * PARAM_N / 8;
	for (int i = 0; i < PARAM_N/32; i++)
	{
        t = _mm_loadu_si128(buf + pos + 4 * i);
        f = _mm256_broadcastd_epi32(t);
        f = _mm256_srlv_epi32(f,masksrlv);

        f0 = _mm256_and_si256(f,mask01_epi32);
        f1 = _mm256_and_si256(_mm256_srli_epi32(f,8),mask01_epi32);
        f2 = _mm256_and_si256(_mm256_srli_epi32(f,16),mask01_epi32);
        f3 = _mm256_and_si256(_mm256_srli_epi32(f,24),mask01_epi32);

        f0 = _mm256_packs_epi32(f0,f1);
        f2 = _mm256_packs_epi32(f2,f3);
        f0 = _mm256_permute4x64_epi64(f0,0xd8);
        f2 = _mm256_permute4x64_epi64(f2,0xd8);
        f0 = _mm256_sub_epi16(zero, f0);
        f2 = _mm256_sub_epi16(zero, f2);
        f1 = _mm256_loadu_si256(r->coeffs + i * 32);
        f3 = _mm256_loadu_si256(r->coeffs + i * 32 + 16);
        f1 = _mm256_and_si256(f1,f0);
        f3 = _mm256_and_si256(f3,f2);

        _mm256_store_si256(r->coeffs + i * 32, f1);
        _mm256_store_si256(r->coeffs + i * 32 + 16, f3);        
	}
}

void poly_bias3_ternary(poly *r, const uint8_t *seed, uint8_t nonce)
{
	const int min_bytes = (PARAM_N + 4) / 5;
	const int nblocks = (min_bytes + KDF_RATE - 1) / KDF_RATE + 1;
	uint8_t buf[nblocks * KDF_RATE];
	uint8_t block[KDF_RATE];
	uint8_t extseed[SEED_BYTES + 1];
	kdfstate state;
	int ctr;

	memcpy(extseed, seed, SEED_BYTES);
	extseed[SEED_BYTES] = nonce;

	KDF_ABSORB(&state, extseed, SEED_BYTES + 1);
	KDF_SQUEEZEBLOCK(buf, nblocks, &state);
	ctr = ternary3(r->coeffs, buf, PARAM_N, nblocks * KDF_RATE);

	while (ctr < PARAM_N) {
		KDF_SQUEEZEBLOCK(block, 1, &state);
		ctr += ternary3(r->coeffs + ctr, block, PARAM_N - ctr, KDF_RATE);
	}
}

#if NTT_DIM == 64
static void get_noisem_cbd3(poly *r, const uint8_t msg[SEED_BYTES],
						   const uint8_t *seed, uint8_t nonce)
{
	uint8_t buf[24 * SEED_BYTES];
	uint8_t extseed[SEED_BYTES + 1];
	int i, j;

	const __m256i vshift = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
	const __m256i vmask01 = _mm256_set1_epi8(0x01);
	const __m256i vtranspose = _mm256_setr_epi8(
		0, 4, 8, 12, 1, 5, 9, 13, -1, -1, -1, -1, -1, -1, -1, -1,
		0, 4, 8, 12, 1, 5, 9, 13, -1, -1, -1, -1, -1, -1, -1, -1);

	for (i = 0; i < SEED_BYTES; i++)
		extseed[i] = seed[i];
	extseed[SEED_BYTES] = nonce;

	KDF(buf + SEED_BYTES, 23 * SEED_BYTES, extseed, SEED_BYTES + 1);

#if SEED_BYTES == 16
	{
		__m128i v = _mm_loadu_si128((const __m128i *)msg);

		for (i = 1; i < 24; i++)
			v = _mm_xor_si128(v, _mm_loadu_si128((const __m128i *)&buf[i * SEED_BYTES]));

		_mm_storeu_si128((__m128i *)buf, v);
	}
#elif SEED_BYTES == 32
	{
		__m256i v = _mm256_loadu_si256((const __m256i *)msg);

		for (i = 1; i < 24; i++)
			v = _mm256_xor_si256(v, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES]));

		_mm256_storeu_si256((__m256i *)buf, v);
	}
#elif SEED_BYTES == 64
	{
		__m256i v0 = _mm256_loadu_si256((const __m256i *)&msg[0]);
		__m256i v1 = _mm256_loadu_si256((const __m256i *)&msg[32]);

		for (i = 1; i < 24; i++)
		{
			v0 = _mm256_xor_si256(v0, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES]));
			v1 = _mm256_xor_si256(v1, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES + 32]));
		}

		_mm256_storeu_si256((__m256i *)&buf[0], v0);
		_mm256_storeu_si256((__m256i *)&buf[32], v1);
	}
#else
#error "Unsupported SEED_BYTES for NTT_DIM == 64"
#endif

	for (i = 0; i < SEED_BYTES / 2; i++)
	{
		for (j = 0; j < 4; j++)
		{
			const uint8_t *p = &buf[6 * j * SEED_BYTES + 2 * i];
			int16_t *out = &r->coeffs[64 * i + 16 * j];

			__m256i va0, va1, va2;
			__m256i vb0, vb1, vb2;
			__m256i vx;
			__m128i vlo, vhi;

			va0 = _mm256_broadcastd_epi32(_mm_loadu_si16(p + 0 * SEED_BYTES));
			va1 = _mm256_broadcastd_epi32(_mm_loadu_si16(p + 1 * SEED_BYTES));
			va2 = _mm256_broadcastd_epi32(_mm_loadu_si16(p + 2 * SEED_BYTES));
			vb0 = _mm256_broadcastd_epi32(_mm_loadu_si16(p + 3 * SEED_BYTES));
			vb1 = _mm256_broadcastd_epi32(_mm_loadu_si16(p + 4 * SEED_BYTES));
			vb2 = _mm256_broadcastd_epi32(_mm_loadu_si16(p + 5 * SEED_BYTES));

			va0 = _mm256_and_si256(_mm256_srlv_epi32(va0, vshift), vmask01);
			va1 = _mm256_and_si256(_mm256_srlv_epi32(va1, vshift), vmask01);
			va2 = _mm256_and_si256(_mm256_srlv_epi32(va2, vshift), vmask01);
			vb0 = _mm256_and_si256(_mm256_srlv_epi32(vb0, vshift), vmask01);
			vb1 = _mm256_and_si256(_mm256_srlv_epi32(vb1, vshift), vmask01);
			vb2 = _mm256_and_si256(_mm256_srlv_epi32(vb2, vshift), vmask01);

			va0 = _mm256_add_epi8(_mm256_add_epi8(va0, va1), va2);
			vb0 = _mm256_add_epi8(_mm256_add_epi8(vb0, vb1), vb2);
			vx = _mm256_sub_epi8(va0, vb0);

			vx = _mm256_shuffle_epi8(vx, vtranspose);
			vlo = _mm256_castsi256_si128(vx);
			vhi = _mm256_extracti128_si256(vx, 1);
			vlo = _mm_unpacklo_epi32(vlo, vhi);

			_mm256_storeu_si256((__m256i *)out, _mm256_cvtepi8_epi16(vlo));
		}
	}
}

static void get_noisem_cbd2(poly *r, const uint8_t msg[SEED_BYTES],
						   const uint8_t *seed, uint8_t nonce)
{
	uint8_t buf[16 * SEED_BYTES];
	uint8_t extseed[SEED_BYTES + 1];
	int i, j;

	const __m256i vshift = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
	const __m256i vmask01 = _mm256_set1_epi8(0x01);
	const __m256i vtranspose = _mm256_setr_epi8(
		0, 4, 8, 12, 1, 5, 9, 13, -1, -1, -1, -1, -1, -1, -1, -1,
		0, 4, 8, 12, 1, 5, 9, 13, -1, -1, -1, -1, -1, -1, -1, -1);

	for (i = 0; i < SEED_BYTES; i++)
		extseed[i] = seed[i];
	extseed[SEED_BYTES] = nonce;

	KDF(buf + SEED_BYTES, 15 * SEED_BYTES, extseed, SEED_BYTES + 1);

#if SEED_BYTES == 16
	{
		__m128i v = _mm_loadu_si128((const __m128i *)msg);

		for (i = 1; i < 16; i++)
			v = _mm_xor_si128(v, _mm_loadu_si128((const __m128i *)&buf[i * SEED_BYTES]));

		_mm_storeu_si128((__m128i *)buf, v);
	}
#elif SEED_BYTES == 32
	{
		__m256i v = _mm256_loadu_si256((const __m256i *)msg);

		for (i = 1; i < 16; i++)
			v = _mm256_xor_si256(v, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES]));

		_mm256_storeu_si256((__m256i *)buf, v);
	}
#elif SEED_BYTES == 64
	{
		__m256i v0 = _mm256_loadu_si256((const __m256i *)&msg[0]);
		__m256i v1 = _mm256_loadu_si256((const __m256i *)&msg[32]);

		for (i = 1; i < 16; i++)
		{
			v0 = _mm256_xor_si256(v0, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES]));
			v1 = _mm256_xor_si256(v1, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES + 32]));
		}

		_mm256_storeu_si256((__m256i *)&buf[0], v0);
		_mm256_storeu_si256((__m256i *)&buf[32], v1);
	}
#else
#error "Unsupported SEED_BYTES for NTT_DIM == 64"
#endif

	for (i = 0; i < SEED_BYTES / 2; i++)
	{
		for (j = 0; j < 4; j++)
		{
			const uint8_t *p = &buf[4 * j * SEED_BYTES + 2 * i];
			int16_t *out = &r->coeffs[64 * i + 16 * j];

			__m256i va0, va1, vb0, vb1, vx;
			__m128i vlo, vhi;

			va0 = _mm256_broadcastd_epi32(_mm_loadu_si16(p + 0 * SEED_BYTES));
			va1 = _mm256_broadcastd_epi32(_mm_loadu_si16(p + 1 * SEED_BYTES));
			vb0 = _mm256_broadcastd_epi32(_mm_loadu_si16(p + 2 * SEED_BYTES));
			vb1 = _mm256_broadcastd_epi32(_mm_loadu_si16(p + 3 * SEED_BYTES));

			va0 = _mm256_and_si256(_mm256_srlv_epi32(va0, vshift), vmask01);
			va1 = _mm256_and_si256(_mm256_srlv_epi32(va1, vshift), vmask01);
			vb0 = _mm256_and_si256(_mm256_srlv_epi32(vb0, vshift), vmask01);
			vb1 = _mm256_and_si256(_mm256_srlv_epi32(vb1, vshift), vmask01);

			va0 = _mm256_add_epi8(va0, va1);
			vb0 = _mm256_add_epi8(vb0, vb1);
			vx = _mm256_sub_epi8(va0, vb0);

			vx = _mm256_shuffle_epi8(vx, vtranspose);
			vlo = _mm256_castsi256_si128(vx);
			vhi = _mm256_extracti128_si256(vx, 1);
			vlo = _mm_unpacklo_epi32(vlo, vhi);

			_mm256_storeu_si256((__m256i *)out, _mm256_cvtepi8_epi16(vlo));
		}
	}
}
void get_noisem_cbd1(poly* r, const uint8_t msg[32], const uint8_t* seed, uint8_t nonce)
{
	//__declspec(align(32)) uint8_t buf[7 * SEED_BYTES];
	uint8_t buf[7 * SEED_BYTES] __attribute__((aligned(32)));
	__m256i v[8], d0, d1, d2, d3, c0, c1;
	__m256i a, b;
	__m256i one = _mm256_set1_epi32(1);
	const __m256i shift = _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0);
	uint8_t extseed[SEED_BYTES + 1];
	int i, j;
	for (i = 0; i < SEED_BYTES; i++)
		extseed[i] = seed[i];
	extseed[SEED_BYTES] = nonce;

	KDF(buf, 7 * SEED_BYTES, extseed, SEED_BYTES + 1);

	for (j = 0; j < SEED_BYTES / 32; j++)
	{
		v[0] = _mm256_loadu_si256((__m256i*) & msg[32 * j]);
		for (i = 0; i < 7; i++)
		{
			v[i + 1] = _mm256_loadu_si256((__m256i*) & buf[SEED_BYTES * i + 32 * j]);
			v[0] = _mm256_xor_si256(v[0], v[i + 1]);
		}
		for (i = 0; i < 4; i++)
		{
			a = _mm256_permute4x64_epi64(v[2 * i], 0x44);
			b = _mm256_permute4x64_epi64(v[2 * i + 1], 0x44);


			d0 = _mm256_shuffle_epi32(a, 0x00);//32 bit
			d0 = _mm256_srlv_epi32(d0, shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, one);
			d1 = _mm256_and_si256(d1, one);
			d2 = _mm256_and_si256(d2, one);
			d3 = _mm256_and_si256(d3, one);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);

			c0 = _mm256_permute4x64_epi64(d0, 0xD8);
			c1 = _mm256_permute4x64_epi64(d2, 0xD8);


			d0 = _mm256_shuffle_epi32(b, 0x00);//32 bit
			d0 = _mm256_srlv_epi32(d0, shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, one);
			d1 = _mm256_and_si256(d1, one);
			d2 = _mm256_and_si256(d2, one);
			d3 = _mm256_and_si256(d3, one);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			d0 = _mm256_permute4x64_epi64(d0, 0xD8);
			d1 = _mm256_permute4x64_epi64(d2, 0xD8);

			c0 = _mm256_sub_epi16(c0, d0);
			c1 = _mm256_sub_epi16(c1, d1);

			_mm256_storeu_si256((__m256i*) & r->coeffs[1024 * j + 16 * i + 0], c0);//
			_mm256_storeu_si256((__m256i*) & r->coeffs[1024 * j + 64 + 16 * i], c1);//64


			d0 = _mm256_shuffle_epi32(a, 0x55);
			d0 = _mm256_srlv_epi32(d0, shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, one);
			d1 = _mm256_and_si256(d1, one);
			d2 = _mm256_and_si256(d2, one);
			d3 = _mm256_and_si256(d3, one);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			c0 = _mm256_permute4x64_epi64(d0, 0xD8);
			c1 = _mm256_permute4x64_epi64(d2, 0xD8);

			d0 = _mm256_shuffle_epi32(b, 0x55);
			d0 = _mm256_srlv_epi32(d0, shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, one);
			d1 = _mm256_and_si256(d1, one);
			d2 = _mm256_and_si256(d2, one);
			d3 = _mm256_and_si256(d3, one);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			d0 = _mm256_permute4x64_epi64(d0, 0xD8);
			d1 = _mm256_permute4x64_epi64(d2, 0xD8);

			c0 = _mm256_sub_epi16(c0, d0);
			c1 = _mm256_sub_epi16(c1, d1);

			_mm256_storeu_si256((__m256i*) & r->coeffs[1024 * j + 128 + 16 * i], c0);//128
			_mm256_storeu_si256((__m256i*) & r->coeffs[1024 * j + 192 + 16 * i], c1);//192

			d0 = _mm256_shuffle_epi32(a, 0xAA);
			d0 = _mm256_srlv_epi32(d0, shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, one);
			d1 = _mm256_and_si256(d1, one);
			d2 = _mm256_and_si256(d2, one);
			d3 = _mm256_and_si256(d3, one);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			c0 = _mm256_permute4x64_epi64(d0, 0xD8);
			c1 = _mm256_permute4x64_epi64(d2, 0xD8);

			d0 = _mm256_shuffle_epi32(b, 0xAA);
			d0 = _mm256_srlv_epi32(d0, shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, one);
			d1 = _mm256_and_si256(d1, one);
			d2 = _mm256_and_si256(d2, one);
			d3 = _mm256_and_si256(d3, one);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			d0 = _mm256_permute4x64_epi64(d0, 0xD8);
			d1 = _mm256_permute4x64_epi64(d2, 0xD8);

			c0 = _mm256_sub_epi16(c0, d0);
			c1 = _mm256_sub_epi16(c1, d1);

			_mm256_storeu_si256((__m256i*) & r->coeffs[1024 * j + 256 + 16 * i], c0);//256
			_mm256_storeu_si256((__m256i*) & r->coeffs[1024 * j + 320 + 16 * i], c1);//320

			d0 = _mm256_shuffle_epi32(a, 0xFF);
			d0 = _mm256_srlv_epi32(d0, shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, one);
			d1 = _mm256_and_si256(d1, one);
			d2 = _mm256_and_si256(d2, one);
			d3 = _mm256_and_si256(d3, one);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			c0 = _mm256_permute4x64_epi64(d0, 0xD8);
			c1 = _mm256_permute4x64_epi64(d2, 0xD8);

			d0 = _mm256_shuffle_epi32(b, 0xFF);
			d0 = _mm256_srlv_epi32(d0, shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, one);
			d1 = _mm256_and_si256(d1, one);
			d2 = _mm256_and_si256(d2, one);
			d3 = _mm256_and_si256(d3, one);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			d0 = _mm256_permute4x64_epi64(d0, 0xD8);
			d1 = _mm256_permute4x64_epi64(d2, 0xD8);

			c0 = _mm256_sub_epi16(c0, d0);
			c1 = _mm256_sub_epi16(c1, d1);

			_mm256_storeu_si256((__m256i*) & r->coeffs[1024 * j + 384 + 16 * i], c0);
			_mm256_storeu_si256((__m256i*) & r->coeffs[1024 * j + 448 + 16 * i], c1);//
		}
		for (i = 0; i < 4; i++)
		{
			a = _mm256_permute4x64_epi64(v[2 * i], 0xEE);
			b = _mm256_permute4x64_epi64(v[2 * i + 1], 0xEE);

			d0 = _mm256_shuffle_epi32(a, 0x00);//32 bit
			d0 = _mm256_srlv_epi32(d0, shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, one);
			d1 = _mm256_and_si256(d1, one);
			d2 = _mm256_and_si256(d2, one);
			d3 = _mm256_and_si256(d3, one);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			c0 = _mm256_permute4x64_epi64(d0, 0xD8);
			c1 = _mm256_permute4x64_epi64(d2, 0xD8);

			d0 = _mm256_shuffle_epi32(b, 0x00);//32 bit
			d0 = _mm256_srlv_epi32(d0, shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, one);
			d1 = _mm256_and_si256(d1, one);
			d2 = _mm256_and_si256(d2, one);
			d3 = _mm256_and_si256(d3, one);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			d0 = _mm256_permute4x64_epi64(d0, 0xD8);
			d1 = _mm256_permute4x64_epi64(d2, 0xD8);

			c0 = _mm256_sub_epi16(c0, d0);
			c1 = _mm256_sub_epi16(c1, d1);

			_mm256_storeu_si256((__m256i*) & r->coeffs[1024 * j + 512 + 16 * i], c0);
			_mm256_storeu_si256((__m256i*) & r->coeffs[1024 * j + 576 + 16 * i], c1);


			d0 = _mm256_shuffle_epi32(a, 0x55);
			d0 = _mm256_srlv_epi32(d0, shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, one);
			d1 = _mm256_and_si256(d1, one);
			d2 = _mm256_and_si256(d2, one);
			d3 = _mm256_and_si256(d3, one);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			c0 = _mm256_permute4x64_epi64(d0, 0xD8);
			c1 = _mm256_permute4x64_epi64(d2, 0xD8);

			d0 = _mm256_shuffle_epi32(b, 0x55);
			d0 = _mm256_srlv_epi32(d0, shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, one);
			d1 = _mm256_and_si256(d1, one);
			d2 = _mm256_and_si256(d2, one);
			d3 = _mm256_and_si256(d3, one);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			d0 = _mm256_permute4x64_epi64(d0, 0xD8);
			d1 = _mm256_permute4x64_epi64(d2, 0xD8);

			c0 = _mm256_sub_epi16(c0, d0);
			c1 = _mm256_sub_epi16(c1, d1);

			_mm256_storeu_si256((__m256i*) & r->coeffs[1024 * j + 640 + 16 * i], c0);
			_mm256_storeu_si256((__m256i*) & r->coeffs[1024 * j + 704 + 16 * i], c1);

			d0 = _mm256_shuffle_epi32(a, 0xAA);
			d0 = _mm256_srlv_epi32(d0, shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, one);
			d1 = _mm256_and_si256(d1, one);
			d2 = _mm256_and_si256(d2, one);
			d3 = _mm256_and_si256(d3, one);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			c0 = _mm256_permute4x64_epi64(d0, 0xD8);
			c1 = _mm256_permute4x64_epi64(d2, 0xD8);

			d0 = _mm256_shuffle_epi32(b, 0xAA);
			d0 = _mm256_srlv_epi32(d0, shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, one);
			d1 = _mm256_and_si256(d1, one);
			d2 = _mm256_and_si256(d2, one);
			d3 = _mm256_and_si256(d3, one);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			d0 = _mm256_permute4x64_epi64(d0, 0xD8);
			d1 = _mm256_permute4x64_epi64(d2, 0xD8);

			c0 = _mm256_sub_epi16(c0, d0);
			c1 = _mm256_sub_epi16(c1, d1);

			_mm256_storeu_si256((__m256i*) & r->coeffs[1024 * j + 768 + 16 * i], c0);
			_mm256_storeu_si256((__m256i*) & r->coeffs[1024 * j + 832 + 16 * i], c1);

			d0 = _mm256_shuffle_epi32(a, 0xFF);
			d0 = _mm256_srlv_epi32(d0, shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, one);
			d1 = _mm256_and_si256(d1, one);
			d2 = _mm256_and_si256(d2, one);
			d3 = _mm256_and_si256(d3, one);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			c0 = _mm256_permute4x64_epi64(d0, 0xD8);
			c1 = _mm256_permute4x64_epi64(d2, 0xD8);

			d0 = _mm256_shuffle_epi32(b, 0xFF);
			d0 = _mm256_srlv_epi32(d0, shift);
			d1 = _mm256_srli_epi32(d0, 8);
			d2 = _mm256_srli_epi32(d0, 16);
			d3 = _mm256_srli_epi32(d0, 24);

			d0 = _mm256_and_si256(d0, one);
			d1 = _mm256_and_si256(d1, one);
			d2 = _mm256_and_si256(d2, one);
			d3 = _mm256_and_si256(d3, one);

			d0 = _mm256_packus_epi32(d0, d1);
			d2 = _mm256_packus_epi32(d2, d3);
			d0 = _mm256_permute4x64_epi64(d0, 0xD8);
			d1 = _mm256_permute4x64_epi64(d2, 0xD8);

			c0 = _mm256_sub_epi16(c0, d0);
			c1 = _mm256_sub_epi16(c1, d1);

			_mm256_storeu_si256((__m256i*) & r->coeffs[1024 * j + 896 + 16 * i], c0);
			_mm256_storeu_si256((__m256i*) & r->coeffs[1024 * j + 960 + 16 * i], c1);
		}
	}
}
#elif NTT_DIM == 128
static void get_noisem_cbd1(poly *r,
							const uint8_t msg[SEED_BYTES],
							const uint8_t *seed,
							uint8_t nonce)
{
	uint8_t buf[8 * SEED_BYTES];
	uint8_t extseed[SEED_BYTES + 1];
	int i, j, k;
	int16_t a, b;

	for (i = 0; i < SEED_BYTES; i++)
		extseed[i] = seed[i];
	extseed[SEED_BYTES] = nonce;

	KDF(buf + SEED_BYTES,
		7 * SEED_BYTES,
		extseed,
		SEED_BYTES + 1);

	for (i = 0; i < SEED_BYTES; i++)
	{
		buf[i] = msg[i];
		for (j = 1; j < 8; j++)
			buf[i] ^= buf[SEED_BYTES * j + i];
	}

	for (i = 0; i < SEED_BYTES / 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			for (k = 0; k < 8; k++)
			{
				a = (buf[4 * i + j] >> k) & 0x01;
				b = (buf[SEED_BYTES + 4 * i + j] >> k) & 0x01;
				r->coeffs[128 * i + 8 * j + k] = a - b;

				a = (buf[2 * SEED_BYTES + 4 * i + j] >> k) & 0x01;
				b = (buf[3 * SEED_BYTES + 4 * i + j] >> k) & 0x01;
				r->coeffs[128 * i + 32 + 8 * j + k] = a - b;

				a = (buf[4 * SEED_BYTES + 4 * i + j] >> k) & 0x01;
				b = (buf[5 * SEED_BYTES + 4 * i + j] >> k) & 0x01;
				r->coeffs[128 * i + 64 + 8 * j + k] = a - b;

				a = (buf[6 * SEED_BYTES + 4 * i + j] >> k) & 0x01;
				b = (buf[7 * SEED_BYTES + 4 * i + j] >> k) & 0x01;
				r->coeffs[128 * i + 96 + 8 * j + k] = a - b;
			}
		}
	}
}

static void get_noisem_cbd2(poly *r, const uint8_t msg[SEED_BYTES],
						   const uint8_t *seed, uint8_t nonce)
{
	uint8_t buf[16 * SEED_BYTES];
	uint8_t extseed[SEED_BYTES + 1];
	int i, j;

	const __m256i vshift = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
	const __m256i vmask01 = _mm256_set1_epi8(0x01);
	const __m256i vmaskff = _mm256_set1_epi32(0xff);

	memcpy(extseed, seed, SEED_BYTES);
	extseed[SEED_BYTES] = nonce;

	KDF(buf + SEED_BYTES, 15 * SEED_BYTES, extseed, SEED_BYTES + 1);

#if SEED_BYTES == 16
	{
		__m128i v = _mm_loadu_si128((const __m128i *)msg);

		for (i = 1; i < 16; i++)
			v = _mm_xor_si128(v, _mm_loadu_si128((const __m128i *)&buf[i * SEED_BYTES]));

		_mm_storeu_si128((__m128i *)buf, v);
	}
#elif SEED_BYTES == 32
{
	__m256i v = _mm256_loadu_si256((const __m256i *)msg);

	for (i = 1; i < 16; i++)
		v = _mm256_xor_si256(v, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES]));

	_mm256_storeu_si256((__m256i *)buf, v);
}
#elif SEED_BYTES == 64
{
	__m256i v0 = _mm256_loadu_si256((const __m256i *)&msg[0]);
	__m256i v1 = _mm256_loadu_si256((const __m256i *)&msg[32]);

	for (i = 1; i < 16; i++)
	{
		v0 = _mm256_xor_si256(v0, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES]));
		v1 = _mm256_xor_si256(v1, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES + 32]));
	}

	_mm256_storeu_si256((__m256i *)&buf[0], v0);
	_mm256_storeu_si256((__m256i *)&buf[32], v1);
}
#else
#error "Unsupported SEED_BYTES"
#endif

	for (i = 0; i < SEED_BYTES / 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			const uint8_t *p = &buf[4 * j * SEED_BYTES + 4 * i];
			int16_t *out = &r->coeffs[128 * i + 32 * j];

			__m256i va0, va1, vb0, vb1;
			__m256i vt0, vt1, vt2, vt3;
			__m256i valo, vahi, vblo, vbhi;

			va0 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 0 * SEED_BYTES));
			va1 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 1 * SEED_BYTES));
			vb0 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 2 * SEED_BYTES));
			vb1 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 3 * SEED_BYTES));

			va0 = _mm256_and_si256(_mm256_srlv_epi32(va0, vshift), vmask01);
			va1 = _mm256_and_si256(_mm256_srlv_epi32(va1, vshift), vmask01);
			vb0 = _mm256_and_si256(_mm256_srlv_epi32(vb0, vshift), vmask01);
			vb1 = _mm256_and_si256(_mm256_srlv_epi32(vb1, vshift), vmask01);

			va0 = _mm256_add_epi8(va0, va1);
			vb0 = _mm256_add_epi8(vb0, vb1);

			vt0 = _mm256_and_si256(va0, vmaskff);
			vt1 = _mm256_and_si256(_mm256_srli_epi32(va0, 8), vmaskff);
			vt2 = _mm256_and_si256(_mm256_srli_epi32(va0, 16), vmaskff);
			vt3 = _mm256_and_si256(_mm256_srli_epi32(va0, 24), vmaskff);

			valo = _mm256_permute4x64_epi64(_mm256_packus_epi32(vt0, vt1), 0xd8);
			vahi = _mm256_permute4x64_epi64(_mm256_packus_epi32(vt2, vt3), 0xd8);

			vt0 = _mm256_and_si256(vb0, vmaskff);
			vt1 = _mm256_and_si256(_mm256_srli_epi32(vb0, 8), vmaskff);
			vt2 = _mm256_and_si256(_mm256_srli_epi32(vb0, 16), vmaskff);
			vt3 = _mm256_and_si256(_mm256_srli_epi32(vb0, 24), vmaskff);

			vblo = _mm256_permute4x64_epi64(_mm256_packus_epi32(vt0, vt1), 0xd8);
			vbhi = _mm256_permute4x64_epi64(_mm256_packus_epi32(vt2, vt3), 0xd8);

			_mm256_storeu_si256((__m256i *)&out[0], _mm256_sub_epi16(valo, vblo));
			_mm256_storeu_si256((__m256i *)&out[16], _mm256_sub_epi16(vahi, vbhi));
		}
	}
}
static void get_noisem_cbd3(poly *r, const uint8_t msg[SEED_BYTES],
						   const uint8_t *seed, uint8_t nonce)
{
	uint8_t buf[24 * SEED_BYTES];
	uint8_t extseed[SEED_BYTES + 1];
	int i, j;

	__m256i va0, va1, va2;
	__m256i vb0, vb1, vb2;
	__m256i vx, vy, vlo, vhi;

	const __m256i vshift = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
	const __m256i vmask01 = _mm256_set1_epi8(0x01);
	const __m256i vtranspose = _mm256_setr_epi8(
		0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15,
		0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15);

	for (i = 0; i < SEED_BYTES; i++)
		extseed[i] = seed[i];
	extseed[SEED_BYTES] = nonce;

	KDF(buf + SEED_BYTES, 23 * SEED_BYTES, extseed, SEED_BYTES + 1);

#if SEED_BYTES == 16
	{
		__m128i v = _mm_loadu_si128((const __m128i *)msg);

		for (i = 1; i < 24; i++)
			v = _mm_xor_si128(v, _mm_loadu_si128((const __m128i *)&buf[i * SEED_BYTES]));

		_mm_storeu_si128((__m128i *)buf, v);
	}
#elif SEED_BYTES == 32
	{
		__m256i v = _mm256_loadu_si256((const __m256i *)msg);

		for (i = 1; i < 24; i++)
			v = _mm256_xor_si256(v, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES]));

		_mm256_storeu_si256((__m256i *)buf, v);
	}
#elif SEED_BYTES == 64
	{
		__m256i v0 = _mm256_loadu_si256((const __m256i *)&msg[0]);
		__m256i v1 = _mm256_loadu_si256((const __m256i *)&msg[32]);

		for (i = 1; i < 24; i++)
		{
			v0 = _mm256_xor_si256(v0, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES]));
			v1 = _mm256_xor_si256(v1, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES + 32]));
		}

		_mm256_storeu_si256((__m256i *)&buf[0], v0);
		_mm256_storeu_si256((__m256i *)&buf[32], v1);
	}
#else
#error "Unsupported SEED_BYTES for NTT_DIM == 128"
#endif

	for (i = 0; i < SEED_BYTES / 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			const uint8_t *p = &buf[6 * j * SEED_BYTES + 4 * i];
			int16_t *out = &r->coeffs[128 * i + 32 * j];

			va0 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 0 * SEED_BYTES));
			va1 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 1 * SEED_BYTES));
			va2 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 2 * SEED_BYTES));
			vb0 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 3 * SEED_BYTES));
			vb1 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 4 * SEED_BYTES));
			vb2 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 5 * SEED_BYTES));

			va0 = _mm256_and_si256(_mm256_srlv_epi32(va0, vshift), vmask01);
			va1 = _mm256_and_si256(_mm256_srlv_epi32(va1, vshift), vmask01);
			va2 = _mm256_and_si256(_mm256_srlv_epi32(va2, vshift), vmask01);
			vb0 = _mm256_and_si256(_mm256_srlv_epi32(vb0, vshift), vmask01);
			vb1 = _mm256_and_si256(_mm256_srlv_epi32(vb1, vshift), vmask01);
			vb2 = _mm256_and_si256(_mm256_srlv_epi32(vb2, vshift), vmask01);

			vx = _mm256_add_epi8(_mm256_add_epi8(va0, va1), va2);
			vy = _mm256_add_epi8(_mm256_add_epi8(vb0, vb1), vb2);
			vx = _mm256_sub_epi8(vx, vy);

			vx = _mm256_shuffle_epi8(vx, vtranspose);
			vy = _mm256_permute2x128_si256(vx, vx, 0x01);
			vlo = _mm256_unpacklo_epi32(vx, vy);
			vhi = _mm256_unpackhi_epi32(vx, vy);

			_mm256_storeu_si256((__m256i *)&out[0],
				_mm256_cvtepi8_epi16(_mm256_castsi256_si128(vlo)));
			_mm256_storeu_si256((__m256i *)&out[16],
				_mm256_cvtepi8_epi16(_mm256_castsi256_si128(vhi)));
		}
	}
}
static void get_noisem_cbd4(poly *r, const uint8_t msg[SEED_BYTES], const uint8_t *seed, uint8_t nonce)
{
	uint8_t buf[32 * SEED_BYTES];
	uint8_t extseed[SEED_BYTES + 1];
	int i, j;

	const __m256i vshift = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
	const __m256i vmask01 = _mm256_set1_epi8(0x01);
	const __m256i vtranspose = _mm256_setr_epi8(
		0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15,
		0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15);

	for (i = 0; i < SEED_BYTES; i++)
		extseed[i] = seed[i];
	extseed[SEED_BYTES] = nonce;

	KDF(buf + SEED_BYTES, 31 * SEED_BYTES, extseed, SEED_BYTES + 1);

#if SEED_BYTES == 16
	{
		__m128i v = _mm_loadu_si128((const __m128i *)msg);
		for (i = 1; i < 32; i++)
			v = _mm_xor_si128(v, _mm_loadu_si128((const __m128i *)&buf[i * SEED_BYTES]));
		_mm_storeu_si128((__m128i *)buf, v);
	}
#elif SEED_BYTES == 32
	{
		__m256i v = _mm256_loadu_si256((const __m256i *)msg);
		for (i = 1; i < 32; i++)
			v = _mm256_xor_si256(v, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES]));
		_mm256_storeu_si256((__m256i *)buf, v);
	}
#elif SEED_BYTES == 64
	{
		__m256i v0 = _mm256_loadu_si256((const __m256i *)&msg[0]);
		__m256i v1 = _mm256_loadu_si256((const __m256i *)&msg[32]);
		for (i = 1; i < 32; i++)
		{
			v0 = _mm256_xor_si256(v0, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES]));
			v1 = _mm256_xor_si256(v1, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES + 32]));
		}
		_mm256_storeu_si256((__m256i *)&buf[0], v0);
		_mm256_storeu_si256((__m256i *)&buf[32], v1);
	}
#else
#error "Unsupported SEED_BYTES for NTT_DIM == 128"
#endif

	for (i = 0; i < SEED_BYTES / 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			const uint8_t *p = &buf[8 * j * SEED_BYTES + 4 * i];
			int16_t *out = &r->coeffs[128 * i + 32 * j];

			__m256i va0, va1, va2, va3;
			__m256i vb0, vb1, vb2, vb3;
			__m256i vx;
			__m128i vlo, vhi;

			va0 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 0 * SEED_BYTES));
			va1 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 1 * SEED_BYTES));
			va2 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 2 * SEED_BYTES));
			va3 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 3 * SEED_BYTES));
			vb0 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 4 * SEED_BYTES));
			vb1 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 5 * SEED_BYTES));
			vb2 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 6 * SEED_BYTES));
			vb3 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 7 * SEED_BYTES));

			va0 = _mm256_and_si256(_mm256_srlv_epi32(va0, vshift), vmask01);
			va1 = _mm256_and_si256(_mm256_srlv_epi32(va1, vshift), vmask01);
			va2 = _mm256_and_si256(_mm256_srlv_epi32(va2, vshift), vmask01);
			va3 = _mm256_and_si256(_mm256_srlv_epi32(va3, vshift), vmask01);
			vb0 = _mm256_and_si256(_mm256_srlv_epi32(vb0, vshift), vmask01);
			vb1 = _mm256_and_si256(_mm256_srlv_epi32(vb1, vshift), vmask01);
			vb2 = _mm256_and_si256(_mm256_srlv_epi32(vb2, vshift), vmask01);
			vb3 = _mm256_and_si256(_mm256_srlv_epi32(vb3, vshift), vmask01);

			va0 = _mm256_add_epi8(_mm256_add_epi8(va0, va1), _mm256_add_epi8(va2, va3));
			vb0 = _mm256_add_epi8(_mm256_add_epi8(vb0, vb1), _mm256_add_epi8(vb2, vb3));
			vx = _mm256_sub_epi8(va0, vb0);

			vx = _mm256_shuffle_epi8(vx, vtranspose);
			vlo = _mm256_castsi256_si128(vx);
			vhi = _mm256_extracti128_si256(vx, 1);

			_mm256_storeu_si256((__m256i *)&out[0],
				_mm256_cvtepi8_epi16(_mm_unpacklo_epi32(vlo, vhi)));
			_mm256_storeu_si256((__m256i *)&out[16],
				_mm256_cvtepi8_epi16(_mm_unpackhi_epi32(vlo, vhi)));
		}
	}
}
static void get_noisem_cbd7(poly *r, const uint8_t msg[SEED_BYTES], const uint8_t *seed, uint8_t nonce)
{
	uint8_t buf[56 * SEED_BYTES];
	uint8_t extseed[SEED_BYTES + 1];
	int i, j;

	const __m256i vshift = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
	const __m256i vmask01 = _mm256_set1_epi8(0x01);
	const __m256i vtranspose = _mm256_setr_epi8(
		0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15,
		0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15);

	memcpy(extseed, seed, SEED_BYTES);
	extseed[SEED_BYTES] = nonce;

	KDF(buf + SEED_BYTES, 55 * SEED_BYTES, extseed, SEED_BYTES + 1);

#if SEED_BYTES == 16
	{
		__m128i v = _mm_loadu_si128((const __m128i *)msg);

		for (i = 1; i < 56; i++)
			v = _mm_xor_si128(v, _mm_loadu_si128((const __m128i *)&buf[i * SEED_BYTES]));

		_mm_storeu_si128((__m128i *)buf, v);
	}
#elif SEED_BYTES == 32
	{
		__m256i v = _mm256_loadu_si256((const __m256i *)msg);

		for (i = 1; i < 56; i++)
			v = _mm256_xor_si256(v, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES]));

		_mm256_storeu_si256((__m256i *)buf, v);
	}
#elif SEED_BYTES == 64
	{
		__m256i v0 = _mm256_loadu_si256((const __m256i *)&msg[0]);
		__m256i v1 = _mm256_loadu_si256((const __m256i *)&msg[32]);

		for (i = 1; i < 56; i++)
		{
			v0 = _mm256_xor_si256(v0, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES]));
			v1 = _mm256_xor_si256(v1, _mm256_loadu_si256((const __m256i *)&buf[i * SEED_BYTES + 32]));
		}

		_mm256_storeu_si256((__m256i *)&buf[0], v0);
		_mm256_storeu_si256((__m256i *)&buf[32], v1);
	}
#else
#error "Unsupported SEED_BYTES for NTT_DIM == 128"
#endif

	for (i = 0; i < SEED_BYTES / 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			const uint8_t *p = &buf[14 * j * SEED_BYTES + 4 * i];
			int16_t *out = &r->coeffs[128 * i + 32 * j];

			__m256i va0, va1, va2, va3, va4, va5, va6;
			__m256i vb0, vb1, vb2, vb3, vb4, vb5, vb6;
			__m256i vx;
			__m128i vlo, vhi;

			va0 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 0 * SEED_BYTES));
			va1 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 1 * SEED_BYTES));
			va2 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 2 * SEED_BYTES));
			va3 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 3 * SEED_BYTES));
			va4 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 4 * SEED_BYTES));
			va5 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 5 * SEED_BYTES));
			va6 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 6 * SEED_BYTES));

			vb0 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 7 * SEED_BYTES));
			vb1 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 8 * SEED_BYTES));
			vb2 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 9 * SEED_BYTES));
			vb3 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 10 * SEED_BYTES));
			vb4 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 11 * SEED_BYTES));
			vb5 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 12 * SEED_BYTES));
			vb6 = _mm256_broadcastd_epi32(_mm_loadu_si32(p + 13 * SEED_BYTES));

			va0 = _mm256_and_si256(_mm256_srlv_epi32(va0, vshift), vmask01);
			va1 = _mm256_and_si256(_mm256_srlv_epi32(va1, vshift), vmask01);
			va2 = _mm256_and_si256(_mm256_srlv_epi32(va2, vshift), vmask01);
			va3 = _mm256_and_si256(_mm256_srlv_epi32(va3, vshift), vmask01);
			va4 = _mm256_and_si256(_mm256_srlv_epi32(va4, vshift), vmask01);
			va5 = _mm256_and_si256(_mm256_srlv_epi32(va5, vshift), vmask01);
			va6 = _mm256_and_si256(_mm256_srlv_epi32(va6, vshift), vmask01);

			vb0 = _mm256_and_si256(_mm256_srlv_epi32(vb0, vshift), vmask01);
			vb1 = _mm256_and_si256(_mm256_srlv_epi32(vb1, vshift), vmask01);
			vb2 = _mm256_and_si256(_mm256_srlv_epi32(vb2, vshift), vmask01);
			vb3 = _mm256_and_si256(_mm256_srlv_epi32(vb3, vshift), vmask01);
			vb4 = _mm256_and_si256(_mm256_srlv_epi32(vb4, vshift), vmask01);
			vb5 = _mm256_and_si256(_mm256_srlv_epi32(vb5, vshift), vmask01);
			vb6 = _mm256_and_si256(_mm256_srlv_epi32(vb6, vshift), vmask01);

			va0 = _mm256_add_epi8(_mm256_add_epi8(_mm256_add_epi8(va0, va1), _mm256_add_epi8(va2, va3)),
								  _mm256_add_epi8(_mm256_add_epi8(va4, va5), va6));
			vb0 = _mm256_add_epi8(_mm256_add_epi8(_mm256_add_epi8(vb0, vb1), _mm256_add_epi8(vb2, vb3)),
								  _mm256_add_epi8(_mm256_add_epi8(vb4, vb5), vb6));

			vx = _mm256_sub_epi8(va0, vb0);

			vx = _mm256_shuffle_epi8(vx, vtranspose);
			vlo = _mm256_castsi256_si128(vx);
			vhi = _mm256_extracti128_si256(vx, 1);

			_mm256_storeu_si256((__m256i *)&out[0],
				_mm256_cvtepi8_epi16(_mm_unpacklo_epi32(vlo, vhi)));
			_mm256_storeu_si256((__m256i *)&out[16],
				_mm256_cvtepi8_epi16(_mm_unpackhi_epi32(vlo, vhi)));
		}
	}
}
#endif

void poly_sample_f(poly *f, uint8_t *seed, uint8_t nonce)
{
#if ETA_F == 1
	poly_binomial_dist1(f, seed, nonce);
#elif ETA_F == 2
	poly_binomial_dist2(f, seed, nonce);
#elif ETA_F == 3
	poly_binomial_dist3(f, seed, nonce);
#elif ETA_F == 4
	poly_binomial_dist4(f, seed, nonce);
#elif ETA_F == 7
	poly_binomial_dist7(f, seed, nonce);
#elif ETA_F == 8
	poly_bias8_ternary(f, seed, nonce);
#elif ETA_F == 9
	poly_bias3_ternary(f, seed, nonce);
#endif
}

void poly_sample_g(poly *g, uint8_t *seed, uint8_t nonce)
{
#if ETA_G == 1
	poly_binomial_dist1(g, seed, nonce);
#elif ETA_G == 2
	poly_binomial_dist2(g, seed, nonce);
#elif ETA_G == 3
	poly_binomial_dist3(g, seed, nonce);
#elif ETA_G == 4
	poly_binomial_dist4(g, seed, nonce);
#elif ETA_G == 7
	poly_binomial_dist7(g, seed, nonce);
#elif ETA_G == 8
	poly_bias8_ternary(g, seed, nonce);
#elif ETA_G == 9
	poly_bias3_ternary(g, seed, nonce);
#endif
}

void poly_sample_r(poly *r, uint8_t *coins, uint8_t nonce)
{
#if ETA_R == 1
	poly_binomial_dist1(r, coins, nonce);
#elif ETA_R == 2
	poly_binomial_dist2(r, coins, nonce);
#elif ETA_R == 3
	poly_binomial_dist3(r, coins, nonce);
#elif ETA_R == 4
	poly_binomial_dist4(r, coins, nonce);
#elif ETA_R == 7
	poly_binomial_dist7(r, coins, nonce);
#elif ETA_R == 8
	poly_bias8_ternary(r, coins, nonce);
#elif ETA_R == 9
	poly_bias3_ternary(r, coins, nonce);
#endif
}

void poly_get_noisem(poly* r, const uint8_t msg[SEED_BYTES], const uint8_t* seed, uint8_t nonce)
{
#if ETA_E == 1
	get_noisem_cbd1(r,msg,seed,nonce);
#elif ETA_E == 2
	get_noisem_cbd2(r,msg,seed,nonce);
#elif ETA_E == 3
	get_noisem_cbd3(r,msg,seed,nonce);
#elif ETA_E == 4
	get_noisem_cbd4(r,msg,seed,nonce);
#elif ETA_E == 7
	get_noisem_cbd7(r,msg,seed,nonce);
#endif
}
