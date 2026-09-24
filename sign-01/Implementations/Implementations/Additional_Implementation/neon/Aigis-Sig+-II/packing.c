#include "neon_compat.h"
#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "packing.h"
#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "packing.h"

#include <string.h>


/*************************************************
* pack the public key pk,
* where pk = rho|t1
**************************************************/
void pack_pk(uint8_t pk[PK_SIZE_PACKED],
	const uint8_t rho[SEEDBYTES],
	const polyveck *t1)
{
	int i;

	for (i = 0; i < SEEDBYTES; ++i)
		pk[i] = rho[i];
	pk += SEEDBYTES;

	for (i = 0; i < PARAM_K; ++i)
		polyt1_pack(pk + i * POLT1_SIZE_PACKED, t1->vec + i);
}
void unpack_pk(uint8_t rho[SEEDBYTES], polyveck *t1, const uint8_t pk[PK_SIZE_PACKED])
{
	int i;

	for (i = 0; i < SEEDBYTES; ++i)
		rho[i] = pk[i];
	pk += SEEDBYTES;

	for (i = 0; i < PARAM_K; ++i)
		polyt1_unpack(t1->vec + i, pk + i * POLT1_SIZE_PACKED);
}

/*************************************************
* pack the secret key sk,
* where sk = rho|key|hash(pk)|s1|s2|t0
**************************************************/
void pack_sk(uint8_t sk[SK_SIZE_PACKED],
	const uint8_t rho[SEEDBYTES],
	const uint8_t key[SEEDBYTES],
	const uint8_t hashpk[CRHBYTES],
	const polyvecl *s1,
	const polyveck *s2,
	const polyveck *t0)
{
	int i;

	//test

	//uint8_t buf[PARAM_L*POLETA1_SIZE_PACKED]

	for (i = 0; i < SEEDBYTES; ++i)
		sk[i] = rho[i];
	sk += SEEDBYTES;
	for (i = 0; i < SEEDBYTES; ++i)
		sk[i] = key[i];
	sk += SEEDBYTES;
	for (i = 0; i < CRHBYTES; ++i)
		sk[i] = hashpk[i];
	sk += CRHBYTES;

	for (i = 0; i < PARAM_L; ++i)
		polyeta1_pack(sk + i * POLETA1_SIZE_PACKED, s1->vec + i);
	sk += PARAM_L * POLETA1_SIZE_PACKED;
	
	
	for (i = 0; i < PARAM_K; ++i)
		polyeta2_pack(sk + i * POLETA2_SIZE_PACKED, s2->vec + i);
	sk += PARAM_K * POLETA2_SIZE_PACKED;
	
	
	for (i = 0; i < PARAM_K; ++i)
		polyt0_pack(sk + i * POLT0_SIZE_PACKED, t0->vec + i);
	
	
}

static void s1_unpack(uint8_t s1_table[PARAM_N * 3], uint8_t *a) {
	const int32_t eta1x2 = 2 * ETA1;
	for (int j = 0; j < PARAM_N / 4; ++j) {
		s1_table[4 * j + 2 * PARAM_N + 0] = s1_table[4 * j + 0]  =  a[j] & 0x03;
		s1_table[4 * j + 2 * PARAM_N + 1] = s1_table[4 * j + 1]  = (a[j] >> 2) & 0x03;
		s1_table[4 * j + 2 * PARAM_N + 2] = s1_table[4 * j + 2]  = (a[j] >> 4) & 0x03;
		s1_table[4 * j + 2 * PARAM_N + 3] = s1_table[4 * j + 3]  = (a[j] >> 6) & 0x03;

		s1_table[4 * j +  PARAM_N + 0] = eta1x2 - s1_table[4 * j + 0];
		s1_table[4 * j +  PARAM_N + 1] = eta1x2 - s1_table[4 * j + 1];
		s1_table[4 * j +  PARAM_N + 2] = eta1x2 - s1_table[4 * j + 2];
		s1_table[4 * j +  PARAM_N + 3] = eta1x2 - s1_table[4 * j + 3];
	}
}

static void eta_1_unpack_avx2(uint8_t s1_table[PARAM_N * 3], uint8_t *a) {
	const __m256i mask = _mm256_set1_epi8(0x03);
	const __m256i eta1x2  = _mm256_set1_epi8(2 * ETA1);
	const __m256i eta1 = _mm256_set1_epi8(ETA1);
	__m256i f0,f1,f2,f3;
	__m256i g0,g1,g2,g3;
	uint8_t *pos;
	for (int j = 0; j < PARAM_N / 128; ++j) {
		f0 = _mm256_loadu_si256(a + j * 32);

		f1 = _mm256_and_si256(_mm256_srli_epi16(f0, 2),mask);
		f2 = _mm256_and_si256(_mm256_srli_epi16(f0, 4),mask);
		f3 = _mm256_and_si256(_mm256_srli_epi16(f0, 6),mask);
		f0 = _mm256_and_si256(f0, mask);

		g0 = _mm256_unpacklo_epi8(f0,f1);
		g1 = _mm256_unpackhi_epi8(f0,f1);
		g2 = _mm256_unpacklo_epi8(f2,f3);
		g3 = _mm256_unpackhi_epi8(f2,f3);

		f0 = _mm256_unpacklo_epi16(g0,g2);
		f1 = _mm256_unpackhi_epi16(g0,g2);
		f2 = _mm256_unpacklo_epi16(g1,g3);
		f3 = _mm256_unpackhi_epi16(g1,g3);

		g0 = _mm256_sub_epi8(eta1, f0);
		g1 = _mm256_sub_epi8(eta1, f1);
		g2 = _mm256_sub_epi8(eta1, f2);
		g3 = _mm256_sub_epi8(eta1, f3);

		f0 = _mm256_sub_epi8(f0, eta1);
		f1 = _mm256_sub_epi8(f1, eta1);
		f2 = _mm256_sub_epi8(f2, eta1);
		f3 = _mm256_sub_epi8(f3, eta1);

		pos = s1_table + 128 * j;

		_mm_storeu_si128(pos,_mm256_extracti128_si256(f0,0));
		_mm_storeu_si128(pos + 16,_mm256_extracti128_si256(f1,0));
		_mm_storeu_si128(pos + 32,_mm256_extracti128_si256(f2,0));
		_mm_storeu_si128(pos + 48,_mm256_extracti128_si256(f3,0));

		_mm_storeu_si128(pos + 64,_mm256_extracti128_si256(f0,1));
		_mm_storeu_si128(pos + 80,_mm256_extracti128_si256(f1,1));
		_mm_storeu_si128(pos + 96,_mm256_extracti128_si256(f2,1));
		_mm_storeu_si128(pos + 112,_mm256_extracti128_si256(f3,1));

		pos = s1_table + PARAM_N + 128 * j;

		_mm_storeu_si128(pos,_mm256_extracti128_si256(g0,0));
		_mm_storeu_si128(pos + 16,_mm256_extracti128_si256(g1,0));
		_mm_storeu_si128(pos + 32,_mm256_extracti128_si256(g2,0));
		_mm_storeu_si128(pos + 48,_mm256_extracti128_si256(g3,0));

		_mm_storeu_si128(pos + 64,_mm256_extracti128_si256(g0,1));
		_mm_storeu_si128(pos + 80,_mm256_extracti128_si256(g1,1));
		_mm_storeu_si128(pos + 96,_mm256_extracti128_si256(g2,1));
		_mm_storeu_si128(pos + 112,_mm256_extracti128_si256(g3,1));
	}
	memcpy(s1_table + 2 * PARAM_N, s1_table, PARAM_N);
}

static void s2_unpack(s2Word s2_table[PARAM_N * 3], uint8_t *a ) {
	const int32_t eta2x2 = 2 * ETA2;
	for (int i = 0; i < PARAM_N / 4; ++i) {
		#if PARAMS == 1
		s2_table[4 * i + 2 * PARAM_N + 0] = s2_table[4 * i + 0] = a[2 * i] & 0x0F;
		s2_table[4 * i + 2 * PARAM_N + 1] = s2_table[4 * i + 1] = (a[2 * i] >> 4);
		s2_table[4 * i + 2 * PARAM_N + 2] = s2_table[4 * i + 2] = a[2 * i + 1] & 0x0F;
		s2_table[4 * i + 2 * PARAM_N + 3] = s2_table[4 * i + 3] = (a[2 * i + 1] >> 4);
		#else
		s2_table[4 * i + 2 * PARAM_N + 0] = s2_table[4 * i + 0]  = a[i] & 0x03;
		s2_table[4 * i + 2 * PARAM_N + 1] = s2_table[4 * i + 1]  = (a[i] >> 2) & 0x03;
		s2_table[4 * i + 2 * PARAM_N + 2] = s2_table[4 * i + 2]  = (a[i] >> 4) & 0x03;
		s2_table[4 * i + 2 * PARAM_N + 3] = s2_table[4 * i + 3]  = (a[i] >> 6) & 0x03;
		#endif
		s2_table[4 * i +  PARAM_N + 0] = eta2x2 - s2_table[4 * i + 0];
		s2_table[4 * i +  PARAM_N + 1] = eta2x2 - s2_table[4 * i + 1];
		s2_table[4 * i +  PARAM_N + 2] = eta2x2 - s2_table[4 * i + 2];
		s2_table[4 * i +  PARAM_N + 3] = eta2x2 - s2_table[4 * i + 3];
	}
}

static void s2_unpack_avx2(s2Word s2_table[PARAM_N * 3], uint8_t *a ) {
	#if PARAMS == 1
	const __m256i mask = _mm256_set1_epi8(0x0f);
	const __m256i eta2x2  = _mm256_set1_epi8(2 * ETA2);
	const __m256i eta2 = _mm256_set1_epi8(ETA2);
	__m256i f0,f1,f2,f3;
	__m256i g0,g1,g2,g3;
	uint16_t *pos;
	for (int j = 0; j < PARAM_N / 64; ++j) {
		f0 = _mm256_loadu_si256(a + j * 32);

		f1 = _mm256_and_si256(_mm256_srli_epi16(f0, 4),mask);
		f0 = _mm256_and_si256(f0, mask);

		f2 = _mm256_unpacklo_epi8(f0,f1);
		f3 = _mm256_unpackhi_epi8(f0,f1);

		f0 = _mm256_sub_epi8(eta2, f2);
		f1 = _mm256_sub_epi8(eta2, f3);

		f2 = _mm256_sub_epi8(f2, eta2);
		f3 = _mm256_sub_epi8(f3, eta2);

		pos = s2_table + 64 * j;
		_mm256_storeu_si256(pos ,_mm256_cvtepi8_epi16(_mm256_extracti128_si256(f2,0)));
		_mm256_storeu_si256(pos + 16,_mm256_cvtepi8_epi16(_mm256_extracti128_si256(f3,0)));
		_mm256_storeu_si256(pos + 32,_mm256_cvtepi8_epi16(_mm256_extracti128_si256(f2,1)));
		_mm256_storeu_si256(pos + 48,_mm256_cvtepi8_epi16(_mm256_extracti128_si256(f3,1)));

		pos = s2_table + PARAM_N + 64 * j;
		_mm256_storeu_si256(pos ,_mm256_cvtepi8_epi16(_mm256_extracti128_si256(f0,0)));
		_mm256_storeu_si256(pos + 16,_mm256_cvtepi8_epi16(_mm256_extracti128_si256(f1,0)));
		_mm256_storeu_si256(pos + 32,_mm256_cvtepi8_epi16(_mm256_extracti128_si256(f0,1)));
		_mm256_storeu_si256(pos + 48,_mm256_cvtepi8_epi16(_mm256_extracti128_si256(f1,1)));
	}
	memcpy(s2_table + 2 * PARAM_N, s2_table, PARAM_N * 2);
	#else
	eta_1_unpack_avx2(s2_table, a);
	#endif
}

void polyt0_unpack(poly *r, const uint8_t *a)
{
	int i;
	__m256i tmp;
	int pos;

	const __m256i d8x = _mm256_set1_epi32(1 << (PARAM_D - 1));
#if PARAM_D == 13
	const __m256i permu = _mm256_set_epi32(0, 3, 2, 1, 3, 2, 1, 0);
	__m256i mask = _mm256_set1_epi32(0x1FFF);
	const __m256i idx8 = _mm256_set_epi8(10, 9, 8, 7, 8, 7, 6, 5,
										 7, 6, 5, 4, 5, 4, 3, 2,
										 7, 6, 5, 4, 6, 5, 4, 3,
										 4, 3, 2, 1, 3, 2, 1, 0);
	const __m256i shift = _mm256_set_epi32(3, 6, 1, 4, 7, 2, 5, 0);
#elif PARAM_D == 15
	const __m256i permu = _mm256_set_epi32(0, 3, 2, 1, 3, 2, 1, 0);
	__m256i mask = _mm256_set1_epi32(0x7FFF);
	const __m256i idx8 = _mm256_set_epi8(12, 11, 10, 9, 10, 9, 8, 7,
										 8, 7, 6, 5, 6, 5, 4, 3,
										 8, 7, 6, 5, 6, 5, 4, 3,
										 4, 3, 2, 1, 3, 2, 1, 0);
	const __m256i shift = _mm256_set_epi32(1, 2, 3, 4, 5, 6, 7, 0);
#endif

	pos = 0;
	for (i = 0; i < PARAM_N; i += 8)
	{
		tmp = _mm256_loadu_si256((__m256i *)&a[pos]);
		tmp = _mm256_permutevar8x32_epi32(tmp, permu);
		tmp = _mm256_shuffle_epi8(tmp, idx8);
		tmp = _mm256_srlv_epi32(tmp, shift);
		tmp = _mm256_and_si256(tmp, mask);
		tmp = _mm256_sub_epi32(d8x, tmp);
		_mm256_store_si256((__m256i *)&r->coeffs[i], tmp);
		pos += PARAM_D;
	}
}

void unpack_sk(uint8_t rho[SEEDBYTES],
	uint8_t key[SEEDBYTES],
	uint8_t hashpk[CRHBYTES],
	uint8_t s1_table[PARAM_L][PARAM_N * 3],
	s2Word s2_table[PARAM_K][PARAM_N * 3],
	polyveck *t0,
	const uint8_t sk[SK_SIZE_PACKED])
{
	int i;
	uint16_t s11_table[PARAM_L][PARAM_N * 3];

	for (i = 0; i < SEEDBYTES; ++i)
		rho[i] = sk[i];
	sk += SEEDBYTES;
	for (i = 0; i < SEEDBYTES; ++i)
		key[i] = sk[i];
	sk += SEEDBYTES;
	for (i = 0; i < CRHBYTES; ++i)
		hashpk[i] = sk[i];

	sk += CRHBYTES;

	for (int j = 0; j < PARAM_L; j++)
		eta_1_unpack_avx2(s1_table[j], sk + j * POLETA1_SIZE_PACKED);
	sk += PARAM_L * POLETA1_SIZE_PACKED;

	for (i = 0; i < PARAM_K; ++i)
		s2_unpack_avx2(s2_table[i], sk + i * POLETA2_SIZE_PACKED);
	sk += PARAM_K * POLETA2_SIZE_PACKED;

	for (i = 0; i < PARAM_K; ++i)
		polyt0_unpack(t0->vec + i, sk + i * POLT0_SIZE_PACKED);
}
/*************************************************
* pack the signature sm,
* where sm = z|h|cseed
**************************************************/
static uint8_t pack4bits_avx2(uint8_t *sm, uint8_t *t, int k)
{
	int i;
	__m256i f,g;
	ALIGN(32) uint8_t tmp[32];
	const __m256i index = _mm256_set_epi8(0,0,0,0,0,0,0,0,14,12,10,8,6,4,2,0,
										 0,0,0,0,0,0,0,0,14,12,10,8,6,4,2,0);
	f = _mm256_load_si256(t);
	g = _mm256_srli_epi16(f,4);
	f = _mm256_xor_si256(f,g);
	f = _mm256_shuffle_epi8(f,index);
	f = _mm256_permute4x64_epi64(f,0x08);
	_mm256_store_si256(tmp,f);
#if PARAMS == 3
	__m256i f1,g1;
	f1 = _mm256_load_si256(t+32);
	g1 = _mm256_srli_epi16(f1,4);
	f1 = _mm256_xor_si256(f1,g1);
	f1 = _mm256_shuffle_epi8(f1,index);
	f1 = _mm256_permute4x64_epi64(f1,0x08);
	_mm_store_si128(tmp+16,_mm256_castsi256_si128(f1));
#endif
	memcpy(sm,tmp,k);

	return (k + 1) >> 1;
}
int unpack4bits_avx2(uint8_t *t, uint8_t *sm, int k)
{
	int i;
	__m256i f,g,e;
	const __m256i mask = _mm256_set1_epi16(0x0f0f);
	ALIGN(32) uint8_t tmp[64] = {0};
	int len = (k + 1) >> 1;
	f = _mm256_loadu_si256(sm);
	g = _mm256_srli_epi16(f,4);
	f = _mm256_and_si256(f,mask);
	g = _mm256_and_si256(g,mask);

	e = _mm256_unpacklo_epi8(f,g);
	f = _mm256_unpackhi_epi8(f,g);

	_mm_store_si128(tmp,_mm256_castsi256_si128(e));
	_mm_store_si128(tmp + 16,_mm256_castsi256_si128(f));
#if PARAMS == 3
	_mm_store_si128(tmp + 32,_mm256_extracti128_si256(e, 1));
	_mm_store_si128(tmp + 48,_mm256_extracti128_si256(f, 1));
#endif
	memcpy(t,tmp,k);

	return len;
}
static int pack6bits_avx2(uint8_t *sm, uint8_t *t, int k)
{
	__m256i f,g,e;
	const __m256i mask0 = _mm256_set1_epi16(0x00ff);
	const __m256i mask1 = _mm256_set1_epi32(0x0000ffff);
	const __m256i index = _mm256_set_epi8(0,0,0,0,14,13,12,10,9,8,6,5,4,2,1,0,
		0,0,0,0,14,13,12,10,9,8,6,5,4,2,1,0);
	ALIGN(32) uint8_t tmp[192];
	for (int i = 0; i < (k + 31)/32; i++)
	{
		f = _mm256_load_si256(t + i * 32);
		g = _mm256_andnot_si256(mask0, f);
		g = _mm256_srli_epi16(g, 2);
		e = _mm256_and_si256(f,mask0);
		f = _mm256_xor_si256(e,g);

		g = _mm256_andnot_si256(mask1, f);
		g = _mm256_srli_epi32(g, 4);
		e = _mm256_and_si256(f,mask1);
		f = _mm256_xor_si256(e,g);

		f= _mm256_shuffle_epi8(f,index);
		_mm_storeu_si128(tmp + i * 24, _mm256_castsi256_si128(f));
		_mm_storeu_si128(tmp + i * 24 + 12,_mm256_extracti128_si256(f,1));
	}
	int len = (k * 6 + 7) >> 3;
	tmp[len - 1] &= (1U << (8 - (k & 3) * 2)) - 1U;
	memcpy(sm,tmp,len);
	return len;
}
int unpack6bits_avx2(uint32_t *t, uint8_t *sm, int k)
{
	int i,j;
	int len = (k * 6 + 7) / 8;
	__m256i f;

	const __m256i mask = _mm256_set1_epi32(0x3f);
	const __m256i offset = _mm256_set_epi32(18,12,6,0,18,12,6,0);
	const __m256i idx1 = _mm256_setr_epi8(0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,
	3,4,5,6,3,4,5,6,3,4,5,6,3,4,5,6);
	ALIGN(32) uint32_t tmp[8];

	for (i = 0; i < k >> 3; i++)
	{
		f = _mm256_loadu_si256(sm + i * 6);
		f = _mm256_permute4x64_epi64(f, 0x00);
		f = _mm256_shuffle_epi8(f, idx1);
		f = _mm256_srlv_epi32(f, offset);
		f = _mm256_and_si256(f, mask);
		_mm256_store_si256(t + i * 8, f);
	}
	if(k & 0x7) {
		f = _mm256_loadu_si256(sm + i * 6);
		f = _mm256_permute4x64_epi64(f, 0x00);
		f = _mm256_shuffle_epi8(f, idx1);
		f = _mm256_srlv_epi32(f, offset);
		f = _mm256_and_si256(f, mask);
		_mm256_store_si256(tmp, f);
		memcpy(t + i * 8, tmp, (k & 0x7) << 2);
	}
	return len;
}
static int pack_h(uint8_t *sm, const polyveck *h)
{
	int i, j, k, r;
	int len;
	ALIGN(32) uint8_t pos[OMEGA]; // pos in each sec
	const int secs = PARAM_N / SEC; // num of secs per poly
	ALIGN(32) uint8_t t[secs * PARAM_K]; // num of 1s per sec
	int start;
	k = 0;
	uint8_t max = 0;
	for (i = 0; i < PARAM_K; ++i)
	{
		for (j = 0; j < PARAM_N / SEC; j++)
		{
			start = k;
			for (r = 0; r < SEC; r++)
				if (h->vec[i].coeffs[SEC * j + r])
					pos[k++] = r;
			t[secs * i + j] = k - start;
			if (t[secs * i + j] != 0)
				max = secs * i + j + 1;
		}
	}
	sm[0] = max;
	sm += 1;
	len = pack4bits_avx2(sm, t, max);
	sm += len;
	len += pack6bits_avx2(sm, pos, k) + 1;
	return len;
}

uint32_t hsub_u8(uint8_t*t) {
	__m256i f = _mm256_load_si256(t);
	f = _mm256_sad_epu8(f,_mm256_setzero_si256());
	__m128i g = _mm_add_epi64(_mm256_extracti128_si256(f,1), _mm256_castsi256_si128(f));
	return _mm_extract_epi32(g,2) + _mm_cvtsi128_si32(g);
}

uint8_t unpack_h(polyveck *h, uint8_t *sm)
{
	int i,j,k, r;
	ALIGN(32) uint32_t pos[OMEGA];
	ALIGN(32) uint8_t t[64] = {0};
	int start;
	int shift = 0;
	k = 0;
	int max = sm[0];

	for (int i = 0; i < PARAM_K; i++)
		for (int j = 0; j < PARAM_N; j++)
			h->vec[i].coeffs[j] = 0;

	sm += 1;
	sm += unpack4bits_avx2(t, sm, max);

	k = hsub_u8(t);
	#if PARAMS == 3
	k += hsub_u8(t + 32);
	#endif
	
	k = 0;
	for (i = 0; i < max; i++)
		k += t[i];

	unpack6bits_avx2(pos, sm, k);

	r = 0;
	for (k = 0; k < max; k++)
	{
		i = k / (PARAM_N / SEC);
		j = k % (PARAM_N / SEC);
		for (start = 0; start < t[k]; start++)
			h->vec[i].coeffs[SEC * j + pos[r++]] = 1;
	}

	return 0;
}

int pack_sig(uint8_t *sm, const polyvecl *z, const uint8_t *cseed, const polyveck *h)
{
	int32_t i, j, k, pos;
	int sig_len;
	
	for (i = 0; i < PARAM_L; ++i)
		polyz_pack(sm + i * POLZ_SIZE_PACKED, z->vec + i);
	sm += PARAM_L * POLZ_SIZE_PACKED;

	/* Encode cseed */
	for (i = 0; i < SEEDBYTES; i++)
		sm[i] = cseed[i];
	sm += SEEDBYTES;

	//pack h
	sig_len = pack_h(sm, h);
	sig_len += PARAM_L * POLZ_SIZE_PACKED + SEEDBYTES;

	return sig_len;
}

uint8_t unpack_sig(polyvecl *z, polyveck *h, uint8_t *cseed,
	const uint8_t *sm)
{
	int32_t i, j, k, pos;
	uint8_t b;

	for (i = 0; i < PARAM_L; ++i)
		polyz_unpack(z->vec + i, sm + i * POLZ_SIZE_PACKED);
	sm += PARAM_L * POLZ_SIZE_PACKED;

	/* Decode cseed */
	for (i = 0; i < SEEDBYTES; ++i)
		cseed[i] = sm[i];
	sm += SEEDBYTES;

	/* Decode h */
	b = unpack_h(h, sm);

	return b;
}

