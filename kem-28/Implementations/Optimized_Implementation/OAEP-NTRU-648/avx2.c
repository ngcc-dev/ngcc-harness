#include "avx2.h"

#include <immintrin.h>
#include "ntt.h"
#include "reduce.h"

extern const int32_t oaep648_baseinv_zetas8[27][8];

void poly_baseinv_prepare_asm(int16_t *r, __m256i *den, const int16_t *a);
int poly_baseinv_batchinvert_asm(__m256i *den);
void poly_baseinv_finalize_asm(int16_t *r, const int16_t *num, const __m256i *den);

const int16_t zetas[] = {
	 1375, 
	  661,-3144, -246, -953, 
	
	 3144, 2794, 3168, 3240, 376, 3104, 2709, -376, 3504, -237, 1280, -2709, 

	  728, 3454,  749, 2861,  -782,   806, -1000, -1363, -2076, -2806, -1579, -2764,
	-2475, 2489, -674, 1556, -2304, -2513, -1297, -2032, -2861, -2711, -1001,  1579, 
	 2311, 1000, -1217,  45,  -517, -1602,  2639,   604,  1800, -2967,   -32,  2304, 
	 
	   81,-1149,  3321,-2172,   740,  1770, -2510, -1312,  -330,   982, -2456, -2599, 
	-2074, 3237, -3022,  870,  -966,  2699,  1733,  -190, -2243, -2053, -1264,  2638, 
	 3227,  371,   365,    6,  -240,   582,   342,  -471, -2957, -3428,   657,  1415, 
	  758, 1332, -3186,-2611,   490,  -594, -1084, -1058, -3494, -2577,  2975, -2588, 
	 1566,  313,   845,-1158,  1970, -2995, 
	  
	  1025, 2342,  2038,  2268,  409,  2519, 2447,  -163,   747, -3154,  896, -2991, 
	  149, -1027, -3307,  2752,  497,  -530, 1070, -3221, -2602, -2273, -926,  1635, 
	  1676, 1174,  -218, -1380, 2248, -1074, 1598, -1688, -1569,  1872, -794,   775, 
	  -184, -1651, 1181,  -634, 1818, -3469,  547, -1108, -3547,  3094,  866,  -488, 
	  1974, 2374,  623,   1066,  538,  1689, -1836, -139,  1468,  2515, -1379, 2847, 
	  2654,  671, -932,   3143, 2041,  2973,  2472, 1040,  -351, -3530, -1482, 2522, 
	  3248, -1232, 1010,   343, 1096,  -667,   136, 3345, -1990,  2511,   311, 3473, 
	   521,  1020, 1438,  2111,  446,  1091,  992, -1667, -3091, -3259, -415, -2082, 
	   168, 2574, -2874,   253, -3390,-2321, -516, -1769, -1809,  -509,  452, -1260, 1357
};


/*************************************************
* Name:        fqmul
*
* Description: Multiplication followed by Montgomery reduction
*
* Arguments:   - int16_t a: first factor
*              - int16_t b: second factor
*
* Returns 16-bit integer congruent to a*b*R^{-1} mod q
**************************************************/
static inline int16_t fqmul(int16_t a, int16_t b)
{
	return montgomery_reduce((int32_t)a*b);
}

static inline __attribute__((always_inline))
void poly_tobytes_store64(uint8_t *r, uint64_t v)
{
	__builtin_memcpy(r, &v, sizeof(v));
}

static inline __attribute__((always_inline))
void poly_tobytes_store32(uint8_t *r, uint32_t v)
{
	__builtin_memcpy(r, &v, sizeof(v));
}

static inline __attribute__((always_inline))
uint64_t poly_tobytes_pack4_lo(__m128i coeffs)
{
	const __m128i shifts = _mm_setr_epi32(0, 5, 2, 7);
	const __m128i shuf0 = _mm_setr_epi8(
		0, 1, 5, 6, 9, 13, 14, (char)0x80,
		(char)0x80, (char)0x80, (char)0x80, (char)0x80,
		(char)0x80, (char)0x80, (char)0x80, (char)0x80);
	const __m128i shuf1 = _mm_setr_epi8(
		(char)0x80, 4, (char)0x80, 8, 12, (char)0x80,
		(char)0x80, (char)0x80, (char)0x80, (char)0x80,
		(char)0x80, (char)0x80, (char)0x80, (char)0x80,
		(char)0x80, (char)0x80);
	__m128i x = _mm_cvtepu16_epi32(coeffs);
	__m128i shifted = _mm_sllv_epi32(x, shifts);
	__m128i packed = _mm_or_si128(_mm_shuffle_epi8(shifted, shuf0),
	                              _mm_shuffle_epi8(shifted, shuf1));

	return (uint64_t)_mm_cvtsi128_si64(packed);
}

static inline __attribute__((always_inline))
uint64_t poly_tobytes_pack4_hi(__m128i coeffs)
{
	const __m128i shifts = _mm_setr_epi32(4, 1, 6, 3);
	const __m128i shuf0 = _mm_setr_epi8(
		0, 1, 2, 5, 9, 10, 13, (char)0x80,
		(char)0x80, (char)0x80, (char)0x80, (char)0x80,
		(char)0x80, (char)0x80, (char)0x80, (char)0x80);
	const __m128i shuf1 = _mm_setr_epi8(
		(char)0x80, (char)0x80, 4, 8, (char)0x80, 12,
		(char)0x80, (char)0x80, (char)0x80, (char)0x80,
		(char)0x80, (char)0x80, (char)0x80, (char)0x80,
		(char)0x80, (char)0x80);
	__m128i x = _mm_cvtepu16_epi32(coeffs);
	__m128i shifted = _mm_sllv_epi32(x, shifts);
	__m128i packed = _mm_or_si128(_mm_shuffle_epi8(shifted, shuf0),
	                              _mm_shuffle_epi8(shifted, shuf1));

	return (uint64_t)_mm_cvtsi128_si64(packed);
}

static inline __attribute__((always_inline))
void poly_tobytes_pack8_avx2(uint8_t *r, __m128i coeffs)
{
	uint64_t lo = poly_tobytes_pack4_lo(coeffs);
	uint64_t hi = poly_tobytes_pack4_hi(_mm_srli_si128(coeffs, 8));
	uint64_t first = (lo & 0x0000ffffffffffffULL)
	                 | ((((lo >> 48) & 0xff) | (hi & 0xff)) << 48)
	                 | ((hi & 0xff00ULL) << 48);
	uint64_t rest = hi >> 16;

	poly_tobytes_store64(r, first);
	poly_tobytes_store32(r + 8, (uint32_t)rest);
	r[12] = (uint8_t)(rest >> 32);
}

void poly_tobytes_avx2(uint8_t r[restrict NTRUOAEP_POLYBYTES],
                       const int16_t a[restrict NTRUOAEP_N])
{
	const __m256i q = _mm256_set1_epi16(NTRUOAEP_Q);
	const __m128i q128 = _mm_set1_epi16(NTRUOAEP_Q);
	int block;

	for(block = 0; block < NTRUOAEP_N / 8 - 1; block += 2)
	{
		const int16_t *ai = a + 8 * block;
		uint8_t *ri = r + 13 * block;
		__m256i v = _mm256_loadu_si256((const __m256i *)ai);

		v = _mm256_add_epi16(v, _mm256_and_si256(_mm256_srai_epi16(v, 15), q));
		poly_tobytes_pack8_avx2(ri, _mm256_castsi256_si128(v));
		poly_tobytes_pack8_avx2(ri + 13, _mm256_extracti128_si256(v, 1));
	}
	{
		__m128i v = _mm_loadu_si128((const __m128i *)(a + 8 * block));

		v = _mm_add_epi16(v, _mm_and_si128(_mm_srai_epi16(v, 15), q128));
		poly_tobytes_pack8_avx2(r + 13 * block, v);
	}
}

static inline void poly_frombytes_unpack8(int16_t *r, const uint8_t *a)
{
	r[0] = a[0] | (((int16_t)a[1] & 0x1f) << 8);
	r[1] = (a[1] >> 5) | ((int16_t)a[2] << 3) | (((int16_t)a[3] & 0x03) << 11);
	r[2] = (a[3] >> 2) | (((int16_t)a[4] & 0x7f) << 6);
	r[3] = (a[4] >> 7) | ((int16_t)a[5] << 1) | (((int16_t)a[6] & 0x0f) << 9);
	r[4] = (a[6] >> 4) | ((int16_t)a[7] << 4) | (((int16_t)a[8] & 0x01) << 12);
	r[5] = (a[8] >> 1) | (((int16_t)a[9] & 0x3f) << 7);
	r[6] = (a[9] >> 6) | ((int16_t)a[10] << 2) | (((int16_t)a[11] & 0x07) << 10);
	r[7] = (a[11] >> 3) | ((int16_t)a[12] << 5);
}

static inline __m128i pack_8x32_to_8x16(__m256i a)
{
	return _mm256_castsi256_si128(_mm256_permute4x64_epi64(
		_mm256_packs_epi32(a, a), 0x08));
}

static inline void poly_frombytes_unpack8_avx2(int16_t *r, const uint8_t *a)
{
	const __m256i shuf = _mm256_setr_epi8(
		0, 1, 2, 3, 1, 2, 3, 4, 3, 4, 5, 6, 4, 5, 6, 7,
		6, 7, 8, 9, 8, 9, 10, 11, 9, 10, 11, 12, 11, 12, 13, 14);
	const __m256i shifts = _mm256_setr_epi32(0, 5, 2, 7, 4, 1, 6, 3);
	const __m256i mask = _mm256_set1_epi32(0x1fff);
	__m128i in128 = _mm_loadu_si128((const __m128i *)a);
	__m256i in = _mm256_broadcastsi128_si256(in128);
	__m256i coeffs = _mm256_and_si256(
		_mm256_srlv_epi32(_mm256_shuffle_epi8(in, shuf), shifts), mask);

	_mm_storeu_si128((__m128i *)r, pack_8x32_to_8x16(coeffs));
}

void poly_frombytes_avx2(int16_t r[restrict NTRUOAEP_N],
                         const uint8_t a[restrict NTRUOAEP_POLYBYTES])
{
	int block;

	for(block = 0; block < NTRUOAEP_N / 8 - 1; block += 8)
	{
		const uint8_t *ai = a + 13 * block;
		int16_t *ri = r + 8 * block;

		poly_frombytes_unpack8_avx2(ri, ai);
		poly_frombytes_unpack8_avx2(ri + 8, ai + 13);
		poly_frombytes_unpack8_avx2(ri + 16, ai + 26);
		poly_frombytes_unpack8_avx2(ri + 24, ai + 39);
		poly_frombytes_unpack8_avx2(ri + 32, ai + 52);
		poly_frombytes_unpack8_avx2(ri + 40, ai + 65);
		poly_frombytes_unpack8_avx2(ri + 48, ai + 78);
		poly_frombytes_unpack8_avx2(ri + 56, ai + 91);
	}

	for(; block < NTRUOAEP_N / 8; block++)
		poly_frombytes_unpack8(r + 8 * block, a + 13 * block);
}


static inline __m128i montgomery_reduce_8x32(__m256i a)
{
	const __m256i q = _mm256_set1_epi32(NTRUOAEP_Q);
	const __m256i qinv = _mm256_set1_epi32(QINV);
	const __m256i mask = _mm256_set1_epi32(0xffff);
	__m256i t = _mm256_and_si256(_mm256_mullo_epi32(a, qinv), mask);

	t = _mm256_slli_epi32(t, 16);
	t = _mm256_srai_epi32(t, 16);
	a = _mm256_srai_epi32(_mm256_sub_epi32(a, _mm256_mullo_epi32(t, q)), 16);

	return pack_8x32_to_8x16(a);
}

static inline __m256i montgomery_reduce_8x32_full(__m256i a)
{
	const __m256i q = _mm256_set1_epi32(NTRUOAEP_Q);
	const __m256i qinv = _mm256_set1_epi32(QINV);
	const __m256i mask = _mm256_set1_epi32(0xffff);
	__m256i t = _mm256_and_si256(_mm256_mullo_epi32(a, qinv), mask);

	t = _mm256_slli_epi32(t, 16);
	t = _mm256_srai_epi32(t, 16);
	return _mm256_srai_epi32(_mm256_sub_epi32(a, _mm256_mullo_epi32(t, q)), 16);
}

static inline __m256i fqmul_8x32(__m256i a, __m256i b)
{
	return montgomery_reduce_8x32_full(_mm256_mullo_epi32(a, b));
}

static inline __m256i fqmul_const_16x16_precomp(__m256i a, __m256i cv,
                                                __m256i q, __m256i qinv,
                                                __m256i sign, __m256i one)
{
	__m256i prod_lo = _mm256_mullo_epi16(a, cv);
	__m256i prod_hi = _mm256_mulhi_epi16(a, cv);
	__m256i t = _mm256_mullo_epi16(prod_lo, qinv);
	__m256i tq_lo = _mm256_mullo_epi16(t, q);
	__m256i tq_hi = _mm256_mulhi_epi16(t, q);
	__m256i borrow = _mm256_cmpgt_epi16(_mm256_xor_si256(tq_lo, sign),
	                                    _mm256_xor_si256(prod_lo, sign));

	borrow = _mm256_and_si256(borrow, one);
	return _mm256_sub_epi16(_mm256_sub_epi16(prod_hi, tq_hi), borrow);
}

static inline __m256i fqmul_const_16x16(__m256i a, int16_t c)
{
	return fqmul_const_16x16_precomp(a, _mm256_set1_epi16(c),
	                                 _mm256_set1_epi16(NTRUOAEP_Q),
	                                 _mm256_set1_epi16(QINV),
	                                 _mm256_set1_epi16((short)0x8000),
	                                 _mm256_set1_epi16(1));
}

static inline __m256i montgomery_reduce_16x16(__m256i a)
{
	const __m256i one = _mm256_set1_epi32(1);
	__m128i lo16 = _mm256_castsi256_si128(a);
	__m128i hi16 = _mm256_extracti128_si256(a, 1);
	__m256i lo32 = _mm256_cvtepi16_epi32(lo16);
	__m256i hi32 = _mm256_cvtepi16_epi32(hi16);
	__m128i lo = montgomery_reduce_8x32(_mm256_mullo_epi32(lo32, one));
	__m128i hi = montgomery_reduce_8x32(_mm256_mullo_epi32(hi32, one));

	return _mm256_inserti128_si256(_mm256_castsi128_si256(lo), hi, 1);
}

static inline __m256i barrett_reduce_16x16(__m256i a)
{
	const __m256i v = _mm256_set1_epi32(((1 << 27) + NTRUOAEP_Q / 2) / NTRUOAEP_Q);
	const __m256i half = _mm256_set1_epi32(1 << 26);
	const __m256i q = _mm256_set1_epi32(NTRUOAEP_Q);
	__m128i lo16 = _mm256_castsi256_si128(a);
	__m128i hi16 = _mm256_extracti128_si256(a, 1);
	__m256i lo32 = _mm256_cvtepi16_epi32(lo16);
	__m256i hi32 = _mm256_cvtepi16_epi32(hi16);
	__m256i tlo = _mm256_srai_epi32(_mm256_add_epi32(_mm256_mullo_epi32(v, lo32), half), 27);
	__m256i thi = _mm256_srai_epi32(_mm256_add_epi32(_mm256_mullo_epi32(v, hi32), half), 27);

	lo32 = _mm256_sub_epi32(lo32, _mm256_mullo_epi32(tlo, q));
	hi32 = _mm256_sub_epi32(hi32, _mm256_mullo_epi32(thi, q));

	return _mm256_inserti128_si256(_mm256_castsi128_si256(pack_8x32_to_8x16(lo32)),
	                               pack_8x32_to_8x16(hi32), 1);
}

static inline __m256i barrett_reduce_8x32_full(__m256i a)
{
	const __m256i v = _mm256_set1_epi32(((1 << 27) + NTRUOAEP_Q / 2) / NTRUOAEP_Q);
	const __m256i half = _mm256_set1_epi32(1 << 26);
	const __m256i q = _mm256_set1_epi32(NTRUOAEP_Q);
	__m256i t = _mm256_srai_epi32(_mm256_add_epi32(_mm256_mullo_epi32(v, a), half), 27);

	return _mm256_sub_epi32(a, _mm256_mullo_epi32(t, q));
}

static inline __m256i load8_contiguous_i16(const int16_t *a)
{
	return _mm256_cvtepi16_epi32(_mm_loadu_si128((const __m128i *)a));
}

static inline __m256i load4_contiguous_i16(const int16_t *a)
{
	return _mm256_cvtepi16_epi32(_mm_loadl_epi64((const __m128i *)a));
}

static inline void store8_contiguous_i16(int16_t *r, __m256i v)
{
	_mm_storeu_si128((__m128i *)r, pack_8x32_to_8x16(v));
}

static inline void store4_contiguous_i16(int16_t *r, __m256i v)
{
	_mm_storel_epi64((__m128i *)r, pack_8x32_to_8x16(v));
}

static inline __m256i load8_tail_coeffs(const int16_t *r, int block, int coeff)
{
	return _mm256_set_epi32(r[(block + 7) * 12 + coeff],
	                        r[(block + 6) * 12 + coeff],
	                        r[(block + 5) * 12 + coeff],
	                        r[(block + 4) * 12 + coeff],
	                        r[(block + 3) * 12 + coeff],
	                        r[(block + 2) * 12 + coeff],
	                        r[(block + 1) * 12 + coeff],
	                        r[(block + 0) * 12 + coeff]);
}

static inline __m256i load8_tail_zetas(const int16_t *zeta, int block)
{
	return _mm256_cvtepi16_epi32(_mm_loadu_si128((const __m128i *)(zeta + block)));
}

static inline __m256i load8_tail_zetas_even(const int16_t *zeta, int block)
{
	return _mm256_set_epi32(zeta[2 * (block + 7)],
	                        zeta[2 * (block + 6)],
	                        zeta[2 * (block + 5)],
	                        zeta[2 * (block + 4)],
	                        zeta[2 * (block + 3)],
	                        zeta[2 * (block + 2)],
	                        zeta[2 * (block + 1)],
	                        zeta[2 * (block + 0)]);
}

static inline __m256i load8_tail_zetas_odd(const int16_t *zeta, int block)
{
	return _mm256_set_epi32(zeta[2 * (block + 7) + 1],
	                        zeta[2 * (block + 6) + 1],
	                        zeta[2 * (block + 5) + 1],
	                        zeta[2 * (block + 4) + 1],
	                        zeta[2 * (block + 3) + 1],
	                        zeta[2 * (block + 2) + 1],
	                        zeta[2 * (block + 1) + 1],
	                        zeta[2 * (block + 0) + 1]);
}

static inline void store8_tail_coeffs(int16_t *r, int block, int coeff, __m256i v)
{
	__m128i t = pack_8x32_to_8x16(v);

	r[(block + 0) * 12 + coeff] = (int16_t)_mm_extract_epi16(t, 0);
	r[(block + 1) * 12 + coeff] = (int16_t)_mm_extract_epi16(t, 1);
	r[(block + 2) * 12 + coeff] = (int16_t)_mm_extract_epi16(t, 2);
	r[(block + 3) * 12 + coeff] = (int16_t)_mm_extract_epi16(t, 3);
	r[(block + 4) * 12 + coeff] = (int16_t)_mm_extract_epi16(t, 4);
	r[(block + 5) * 12 + coeff] = (int16_t)_mm_extract_epi16(t, 5);
	r[(block + 6) * 12 + coeff] = (int16_t)_mm_extract_epi16(t, 6);
	r[(block + 7) * 12 + coeff] = (int16_t)_mm_extract_epi16(t, 7);
}

static inline __m256i load8_i16_zeroupper(const int16_t *a)
{
	return _mm256_inserti128_si256(_mm256_setzero_si256(),
	                               _mm_loadu_si128((const __m128i *)a), 0);
}

static inline __m256i load4_i16_zeroupper(const int16_t *a)
{
	return _mm256_inserti128_si256(_mm256_setzero_si256(),
	                               _mm_loadl_epi64((const __m128i *)a), 0);
}

static inline void store8_i16(int16_t *r, __m256i v)
{
	_mm_storeu_si128((__m128i *)r, _mm256_castsi256_si128(v));
}

static inline void store4_i16(int16_t *r, __m256i v)
{
	_mm_storel_epi64((__m128i *)r, _mm256_castsi256_si128(v));
}

static inline __m256i pack_cbd1_lanes(__m256i lo, __m256i hi)
{
	const __m256i shuf = _mm256_setr_epi8(
		0, 1, 8, 9, 2, 3, 10, 11, 4, 5, 12, 13, 6, 7, 14, 15,
		0, 1, 8, 9, 2, 3, 10, 11, 4, 5, 12, 13, 6, 7, 14, 15);

	return _mm256_shuffle_epi8(_mm256_packs_epi32(lo, hi), shuf);
}

static inline void store_cbd1_8_lanes(int16_t *r, __m256i lo, __m256i hi)
{
	_mm256_storeu_si256((__m256i *)r, pack_cbd1_lanes(lo, hi));
}

static inline void store_cbd1_4_lanes(int16_t *r, __m256i lo, __m256i hi)
{
	_mm_storeu_si128((__m128i *)r, _mm256_castsi256_si128(pack_cbd1_lanes(lo, hi)));
}

static inline __m256i srlv_32(__m256i a, int shift)
{
	return _mm256_srlv_epi32(a, _mm256_set1_epi32(shift));
}

#define CBD1_ROW(STORE, OUT, T1, T2, SHIFT) do { \
	__m256i lo = _mm256_sub_epi32( \
		_mm256_and_si256(_mm256_srli_epi32((T1), (SHIFT)), one), \
		_mm256_and_si256(_mm256_srli_epi32((T2), (SHIFT)), one)); \
	__m256i hi = _mm256_sub_epi32( \
		_mm256_and_si256(_mm256_srli_epi32((T1), 16 + (SHIFT)), one), \
		_mm256_and_si256(_mm256_srli_epi32((T2), 16 + (SHIFT)), one)); \
	STORE((OUT), lo, hi); \
} while(0)

#define CBD1_ROWS(STORE, OUT, T1, T2, STRIDE) do { \
	CBD1_ROW(STORE, (OUT) + 0 * (STRIDE), (T1), (T2), 0); \
	CBD1_ROW(STORE, (OUT) + 1 * (STRIDE), (T1), (T2), 1); \
	CBD1_ROW(STORE, (OUT) + 2 * (STRIDE), (T1), (T2), 2); \
	CBD1_ROW(STORE, (OUT) + 3 * (STRIDE), (T1), (T2), 3); \
	CBD1_ROW(STORE, (OUT) + 4 * (STRIDE), (T1), (T2), 4); \
	CBD1_ROW(STORE, (OUT) + 5 * (STRIDE), (T1), (T2), 5); \
	CBD1_ROW(STORE, (OUT) + 6 * (STRIDE), (T1), (T2), 6); \
	CBD1_ROW(STORE, (OUT) + 7 * (STRIDE), (T1), (T2), 7); \
	CBD1_ROW(STORE, (OUT) + 8 * (STRIDE), (T1), (T2), 8); \
	CBD1_ROW(STORE, (OUT) + 9 * (STRIDE), (T1), (T2), 9); \
	CBD1_ROW(STORE, (OUT) + 10 * (STRIDE), (T1), (T2), 10); \
	CBD1_ROW(STORE, (OUT) + 11 * (STRIDE), (T1), (T2), 11); \
	CBD1_ROW(STORE, (OUT) + 12 * (STRIDE), (T1), (T2), 12); \
	CBD1_ROW(STORE, (OUT) + 13 * (STRIDE), (T1), (T2), 13); \
	CBD1_ROW(STORE, (OUT) + 14 * (STRIDE), (T1), (T2), 14); \
	CBD1_ROW(STORE, (OUT) + 15 * (STRIDE), (T1), (T2), 15); \
} while(0)

void poly_cbd1_avx2(int16_t r[NTRUOAEP_N], const uint8_t buf[NTRUOAEP_N / 4])
{
	const __m256i one = _mm256_set1_epi32(1);

	for(int i = 0; i < 2; i++)
	{
		__m256i t1 = _mm256_loadu_si256((const __m256i *)(buf + 32 * i));
		__m256i t2 = _mm256_loadu_si256((const __m256i *)(buf + 32 * i + 81));

		CBD1_ROWS(store_cbd1_8_lanes, r + 256 * i, t1, t2, 16);
	}

	{
		__m256i t1 = _mm256_inserti128_si256(_mm256_setzero_si256(),
		                                     _mm_loadu_si128((const __m128i *)(buf + 64)), 0);
		__m256i t2 = _mm256_inserti128_si256(_mm256_setzero_si256(),
		                                     _mm_loadu_si128((const __m128i *)(buf + 145)), 0);

		CBD1_ROWS(store_cbd1_4_lanes, r + 512, t1, t2, 8);
	}

	{
		uint32_t t1 = (uint32_t)buf[80];
		uint32_t t2 = (uint32_t)buf[161];

		for(int j = 0; j < 8; j++)
		{
			r[640 + j] = (int16_t)((t1 & 0x1) - (t2 & 0x1));
			t1 >>= 1;
			t2 >>= 1;
		}
	}
}

static inline void split_cbd1_coeffs_8(const int16_t *a, __m256i *even, __m256i *odd)
{
	const __m256i even_idx = _mm256_setr_epi32(0, 2, 4, 6, 0, 0, 0, 0);
	const __m256i odd_idx = _mm256_setr_epi32(1, 3, 5, 7, 0, 0, 0, 0);
	__m256i packed = _mm256_loadu_si256((const __m256i *)a);
	__m256i lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(packed));
	__m256i hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(packed, 1));
	__m256i even_lo = _mm256_permutevar8x32_epi32(lo, even_idx);
	__m256i even_hi = _mm256_permutevar8x32_epi32(hi, even_idx);
	__m256i odd_lo = _mm256_permutevar8x32_epi32(lo, odd_idx);
	__m256i odd_hi = _mm256_permutevar8x32_epi32(hi, odd_idx);

	*even = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm256_castsi256_si128(even_lo)),
	                                _mm256_castsi256_si128(even_hi), 1);
	*odd = _mm256_inserti128_si256(_mm256_castsi128_si256(_mm256_castsi256_si128(odd_lo)),
	                               _mm256_castsi256_si128(odd_hi), 1);
}

static inline void split_cbd1_coeffs_4(const int16_t *a, __m256i *even, __m256i *odd)
{
	const __m256i even_idx = _mm256_setr_epi32(0, 2, 4, 6, 0, 0, 0, 0);
	const __m256i odd_idx = _mm256_setr_epi32(1, 3, 5, 7, 0, 0, 0, 0);
	__m256i coeffs = _mm256_cvtepi16_epi32(_mm_loadu_si128((const __m128i *)a));

	*even = _mm256_permutevar8x32_epi32(coeffs, even_idx);
	*odd = _mm256_permutevar8x32_epi32(coeffs, odd_idx);
}

static inline void cbd1_inv_update(__m256i bits, __m256i coeffs, int shift,
                                   __m256i *out, __m256i *err)
{
	const __m256i one = _mm256_set1_epi32(1);
	__m256i diff = _mm256_sub_epi32(_mm256_and_si256(bits, one), coeffs);
	__m256i out_bits = _mm256_sllv_epi32(_mm256_and_si256(diff, one),
	                                     _mm256_set1_epi32(shift));

	*err = _mm256_or_si256(*err, diff);
	*out = _mm256_or_si256(*out, out_bits);
}

int poly_cbd1_inv_avx2(uint8_t *w, const int16_t a[NTRUOAEP_N],
                       const uint8_t buf[NTRUOAEP_N / 8])
{
	uint32_t err_words[8] __attribute__((aligned(32)));
	uint32_t r = 0;

	for(int i = 0; i < 2; i++)
	{
		__m256i t = _mm256_loadu_si256((const __m256i *)(buf + 32 * i));
		__m256i out = _mm256_setzero_si256();
		__m256i err = _mm256_setzero_si256();

		for(int l = 0; l < 16; l++)
		{
			__m256i even;
			__m256i odd;

			split_cbd1_coeffs_8(a + 256 * i + 16 * l, &even, &odd);
			cbd1_inv_update(srlv_32(t, l), even, l, &out, &err);
			cbd1_inv_update(srlv_32(t, 16 + l), odd, 16 + l, &out, &err);
		}

		_mm256_storeu_si256((__m256i *)(w + 32 * i), out);
		_mm256_store_si256((__m256i *)err_words, err);
		for(int j = 0; j < 8; j++)
			r |= err_words[j];
	}

	{
		__m256i t = _mm256_inserti128_si256(_mm256_setzero_si256(),
		                                    _mm_loadu_si128((const __m128i *)(buf + 64)), 0);
		__m256i out = _mm256_setzero_si256();
		__m256i err = _mm256_setzero_si256();

		for(int l = 0; l < 16; l++)
		{
			__m256i even;
			__m256i odd;

			split_cbd1_coeffs_4(a + 512 + 8 * l, &even, &odd);
			cbd1_inv_update(srlv_32(t, l), even, l, &out, &err);
			cbd1_inv_update(srlv_32(t, 16 + l), odd, 16 + l, &out, &err);
		}

		_mm_storeu_si128((__m128i *)(w + 64), _mm256_castsi256_si128(out));
		_mm256_store_si256((__m256i *)err_words, err);
		for(int j = 0; j < 4; j++)
			r |= err_words[j];
	}

	{
		uint32_t t2 = (uint32_t)buf[80];
		uint32_t t3 = 0;

		for(int j = 0; j < 8; j++)
		{
			uint32_t t4 = t2 & 0x1;

			t4 = t4 - a[640 + j];
			r |= t4;
			t3 ^= (t4 & 0x1) << j;
			t2 >>= 1;
		}

		w[80] = (uint8_t)t3;
	}

	r = r >> 1;
	r = (uint32_t)((-(uint64_t)r) >> 63);

	return (int)r;
}

static inline void sotp_inv_update(__m256i t1_bits, __m256i t2_bits,
                                   __m256i coeffs, int shift,
                                   __m256i *out, __m256i *err)
{
	const __m256i one = _mm256_set1_epi32(1);
	__m256i decoded = _mm256_add_epi32(coeffs, _mm256_and_si256(t2_bits, one));
	__m256i out_bits = _mm256_sllv_epi32(
		_mm256_and_si256(_mm256_xor_si256(decoded, t1_bits), one),
		_mm256_set1_epi32(shift));

	*err = _mm256_or_si256(*err, decoded);
	*out = _mm256_or_si256(*out, out_bits);
}

int poly_sotp_inv_avx2(uint8_t *msg, const int16_t a[NTRUOAEP_N],
                       const uint8_t *buf)
{
	uint32_t err_words[8] __attribute__((aligned(32)));
	uint32_t r = 0;

	for(int i = 0; i < 2; i++)
	{
		__m256i t1 = _mm256_loadu_si256((const __m256i *)(buf + 32 * i));
		__m256i t2 = _mm256_loadu_si256((const __m256i *)(buf + 32 * i + NTRUOAEP_N / 8));
		__m256i out = _mm256_setzero_si256();
		__m256i err = _mm256_setzero_si256();

		for(int l = 0; l < 16; l++)
		{
			__m256i even;
			__m256i odd;

			split_cbd1_coeffs_8(a + 256 * i + 16 * l, &even, &odd);
			sotp_inv_update(srlv_32(t1, l), srlv_32(t2, l), even, l, &out, &err);
			sotp_inv_update(srlv_32(t1, 16 + l), srlv_32(t2, 16 + l), odd, 16 + l, &out, &err);
		}

		_mm256_storeu_si256((__m256i *)(msg + 32 * i), out);
		_mm256_store_si256((__m256i *)err_words, err);
		for(int j = 0; j < 8; j++)
			r |= err_words[j];
	}

	{
		__m256i t1 = _mm256_inserti128_si256(_mm256_setzero_si256(),
		                                     _mm_loadu_si128((const __m128i *)(buf + 64)), 0);
		__m256i t2 = _mm256_inserti128_si256(_mm256_setzero_si256(),
		                                     _mm_loadu_si128((const __m128i *)(buf + 64 + NTRUOAEP_N / 8)), 0);
		__m256i out = _mm256_setzero_si256();
		__m256i err = _mm256_setzero_si256();

		for(int l = 0; l < 16; l++)
		{
			__m256i even;
			__m256i odd;

			split_cbd1_coeffs_4(a + 512 + 8 * l, &even, &odd);
			sotp_inv_update(srlv_32(t1, l), srlv_32(t2, l), even, l, &out, &err);
			sotp_inv_update(srlv_32(t1, 16 + l), srlv_32(t2, 16 + l), odd, 16 + l, &out, &err);
		}

		_mm_storeu_si128((__m128i *)(msg + 64), _mm256_castsi256_si128(out));
		_mm256_store_si256((__m256i *)err_words, err);
		for(int j = 0; j < 4; j++)
			r |= err_words[j];
	}

	{
		uint32_t t1 = (uint32_t)buf[80];
		uint32_t t2 = (uint32_t)buf[161];
		uint32_t t3 = 0;

		for(int j = 0; j < 8; j++)
		{
			uint32_t t4 = t2 & 0x1;

			t4 = (uint32_t)a[640 + j] + t4;
			r |= t4;
			t4 = (t4 ^ t1) & 0x1;
			t3 ^= t4 << j;
			t1 >>= 1;
			t2 >>= 1;
		}

		msg[80] = (uint8_t)t3;
	}

	r = r >> 1;
	r = (uint32_t)((-(uint64_t)r) >> 63);

	return (int)r;
}

static inline void short_poly_pack16_avx2(uint8_t *buf, const int16_t *a,
                                          __m256i one, __m256i byte_weights)
{
	__m256i v = _mm256_add_epi16(_mm256_loadu_si256((const __m256i *)a), one);
	__m256i bytes = _mm256_packus_epi16(v, _mm256_setzero_si256());
	__m256i pairs;
	__m256i sums;
	__m128i packed32;
	__m128i packed16;
	__m128i packed8;

	bytes = _mm256_permute4x64_epi64(bytes, 0xd8);
	pairs = _mm256_maddubs_epi16(bytes, byte_weights);
	sums = _mm256_madd_epi16(pairs, one);
	packed32 = _mm256_castsi256_si128(sums);
	packed16 = _mm_packus_epi32(packed32, packed32);
	packed8 = _mm_packus_epi16(packed16, packed16);

	_mm_storeu_si32(buf, packed8);
}

static inline void short_poly_pack32_avx2(uint8_t *buf, const int16_t *a,
                                          __m256i one, __m256i byte_weights)
{
	__m256i v0 = _mm256_add_epi16(_mm256_loadu_si256((const __m256i *)a), one);
	__m256i v1 = _mm256_add_epi16(_mm256_loadu_si256((const __m256i *)(a + 16)), one);
	__m256i bytes = _mm256_packus_epi16(v0, v1);
	__m256i pairs;
	__m256i sums;
	__m128i packed32;
	__m128i packed16;

	bytes = _mm256_permute4x64_epi64(bytes, 0xd8);
	pairs = _mm256_maddubs_epi16(bytes, byte_weights);
	sums = _mm256_madd_epi16(pairs, one);
	packed32 = _mm_packus_epi32(_mm256_castsi256_si128(sums),
	                            _mm256_extracti128_si256(sums, 1));
	packed16 = _mm_packus_epi16(packed32, packed32);

	_mm_storel_epi64((__m128i *)buf, packed16);
}

static inline void short_poly_pack64_avx2(uint8_t *buf, const int16_t *a,
                                          __m256i one, __m256i byte_weights)
{
	__m256i v0 = _mm256_add_epi16(_mm256_loadu_si256((const __m256i *)a), one);
	__m256i v1 = _mm256_add_epi16(_mm256_loadu_si256((const __m256i *)(a + 16)), one);
	__m256i v2 = _mm256_add_epi16(_mm256_loadu_si256((const __m256i *)(a + 32)), one);
	__m256i v3 = _mm256_add_epi16(_mm256_loadu_si256((const __m256i *)(a + 48)), one);
	__m256i bytes0 = _mm256_permute4x64_epi64(_mm256_packus_epi16(v0, v1), 0xd8);
	__m256i bytes1 = _mm256_permute4x64_epi64(_mm256_packus_epi16(v2, v3), 0xd8);
	__m256i pairs0 = _mm256_maddubs_epi16(bytes0, byte_weights);
	__m256i pairs1 = _mm256_maddubs_epi16(bytes1, byte_weights);
	__m256i sums0 = _mm256_madd_epi16(pairs0, one);
	__m256i sums1 = _mm256_madd_epi16(pairs1, one);
	__m128i packed0 = _mm_packus_epi32(_mm256_castsi256_si128(sums0),
	                                   _mm256_extracti128_si256(sums0, 1));
	__m128i packed1 = _mm_packus_epi32(_mm256_castsi256_si128(sums1),
	                                   _mm256_extracti128_si256(sums1, 1));
	__m128i packed = _mm_packus_epi16(packed0, packed1);

	_mm_storeu_si128((__m128i *)buf, packed);
}

static inline void short_poly_pack8_scalar(uint8_t *buf, const int16_t *a)
{
	uint8_t c;

	c = (uint8_t)(a[0] + 1);
	c = (uint8_t)(c + ((a[1] + 1) << 2));
	c = (uint8_t)(c + ((a[2] + 1) << 4));
	c = (uint8_t)(c + ((a[3] + 1) << 6));
	buf[0] = c;

	c = (uint8_t)(a[4] + 1);
	c = (uint8_t)(c + ((a[5] + 1) << 2));
	c = (uint8_t)(c + ((a[6] + 1) << 4));
	c = (uint8_t)(c + ((a[7] + 1) << 6));
	buf[1] = c;
}

void short_poly_to_bytes_avx2(uint8_t *buf, const int16_t a[NTRUOAEP_N])
{
	const __m256i one = _mm256_set1_epi16(1);
	const __m256i byte_weights = _mm256_set1_epi32(0x40100401);
	int i;

	for(i = 0; i + 64 <= NTRUOAEP_N; i += 64)
		short_poly_pack64_avx2(buf + i / 4, a + i, one, byte_weights);

	if(i + 32 <= NTRUOAEP_N)
	{
		short_poly_pack32_avx2(buf + i / 4, a + i, one, byte_weights);
		i += 32;
	}

	if(i + 16 <= NTRUOAEP_N)
	{
		short_poly_pack16_avx2(buf + i / 4, a + i, one, byte_weights);
		i += 16;
	}

	if(i < NTRUOAEP_N)
		short_poly_pack8_scalar(buf + i / 4, a + i);
}

static inline void store6_i16(int16_t *r, __m256i v)
{
	__m128i lo = _mm256_castsi256_si128(v);

	_mm_storel_epi64((__m128i *)r, lo);
	_mm_storeu_si32(r + 4, _mm_srli_si128(lo, 8));
}

static inline __m256i invntt_head_step3_6(__m256i v, __m256i z,
                                          __m256i q, __m256i qinv,
                                          __m256i sign, __m256i one)
{
	__m256i hi = _mm256_bsrli_epi128(v, 6);
	__m256i sum = _mm256_add_epi16(v, hi);
	__m256i diff = fqmul_const_16x16_precomp(_mm256_sub_epi16(hi, v),
	                                         z, q, qinv, sign, one);

	return _mm256_blend_epi16(sum, _mm256_bslli_epi128(diff, 6), 0x38);
}

static inline void ntt_radix3_noreduce_vec(__m256i v0, __m256i v1, __m256i v2,
                                           __m256i z1, __m256i z2,
                                           __m256i q, __m256i qinv,
                                           __m256i sign, __m256i one,
                                           __m256i *o0, __m256i *o1, __m256i *o2)
{
	const __m256i omega = _mm256_set1_epi16(-714);
	__m256i t1 = fqmul_const_16x16_precomp(v1, z1, q, qinv, sign, one);
	__m256i t2 = fqmul_const_16x16_precomp(v2, z2, q, qinv, sign, one);
	__m256i t3 = fqmul_const_16x16_precomp(_mm256_sub_epi16(t1, t2),
	                                       omega, q, qinv, sign, one);

	*o2 = _mm256_sub_epi16(_mm256_sub_epi16(v0, t1), t3);
	*o1 = _mm256_add_epi16(_mm256_sub_epi16(v0, t2), t3);
	*o0 = _mm256_add_epi16(_mm256_add_epi16(v0, t1), t2);
}

#define BASEINV_VECTORS (NTRUOAEP_N / (3 * 8))

int poly_baseinv_asm(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N])
{
	__m256i den[BASEINV_VECTORS];

	poly_baseinv_prepare_asm(r, den, a);
	if(poly_baseinv_batchinvert_asm(den))
		return 1;

	poly_baseinv_finalize_asm(r, a, den);
	return 0;
}

int poly_baseinv_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N])
{
	return poly_baseinv_asm(r, a);
}

#undef BASEINV_VECTORS
void poly_basemul_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N],
                       const int16_t b[NTRUOAEP_N])
{
	poly_basemul_asm(r, a, b);
}
void poly_baseadd_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N],
                       const int16_t c[NTRUOAEP_N])
{
	poly_baseadd_asm(r, a, c);
}

void poly_sub_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N],
                   const int16_t b[NTRUOAEP_N])
{
	unsigned int i;

	for(i = 0; i <= NTRUOAEP_N - 64; i += 64)
	{
		__m256i av0 = _mm256_loadu_si256((const __m256i *)(a + i));
		__m256i av1 = _mm256_loadu_si256((const __m256i *)(a + i + 16));
		__m256i av2 = _mm256_loadu_si256((const __m256i *)(a + i + 32));
		__m256i av3 = _mm256_loadu_si256((const __m256i *)(a + i + 48));
		__m256i bv0 = _mm256_loadu_si256((const __m256i *)(b + i));
		__m256i bv1 = _mm256_loadu_si256((const __m256i *)(b + i + 16));
		__m256i bv2 = _mm256_loadu_si256((const __m256i *)(b + i + 32));
		__m256i bv3 = _mm256_loadu_si256((const __m256i *)(b + i + 48));

		_mm256_storeu_si256((__m256i *)(r + i), _mm256_sub_epi16(av0, bv0));
		_mm256_storeu_si256((__m256i *)(r + i + 16), _mm256_sub_epi16(av1, bv1));
		_mm256_storeu_si256((__m256i *)(r + i + 32), _mm256_sub_epi16(av2, bv2));
		_mm256_storeu_si256((__m256i *)(r + i + 48), _mm256_sub_epi16(av3, bv3));
	}
	for(; i <= NTRUOAEP_N - 16; i += 16)
	{
		__m256i av = _mm256_loadu_si256((const __m256i *)(a + i));
		__m256i bv = _mm256_loadu_si256((const __m256i *)(b + i));
		_mm256_storeu_si256((__m256i *)(r + i), _mm256_sub_epi16(av, bv));
	}
	for(; i < NTRUOAEP_N; i++)
		r[i] = a[i] - b[i];
}

void poly_triple_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N])
{
	int i;

	for(i = 0; i <= NTRUOAEP_N - 64; i += 64)
	{
		__m256i av0 = _mm256_loadu_si256((const __m256i *)(a + i));
		__m256i av1 = _mm256_loadu_si256((const __m256i *)(a + i + 16));
		__m256i av2 = _mm256_loadu_si256((const __m256i *)(a + i + 32));
		__m256i av3 = _mm256_loadu_si256((const __m256i *)(a + i + 48));

		_mm256_storeu_si256((__m256i *)(r + i),
		                     _mm256_add_epi16(_mm256_add_epi16(av0, av0), av0));
		_mm256_storeu_si256((__m256i *)(r + i + 16),
		                     _mm256_add_epi16(_mm256_add_epi16(av1, av1), av1));
		_mm256_storeu_si256((__m256i *)(r + i + 32),
		                     _mm256_add_epi16(_mm256_add_epi16(av2, av2), av2));
		_mm256_storeu_si256((__m256i *)(r + i + 48),
		                     _mm256_add_epi16(_mm256_add_epi16(av3, av3), av3));
	}
	for(; i <= NTRUOAEP_N - 16; i += 16)
	{
		__m256i av = _mm256_loadu_si256((const __m256i *)(a + i));
		_mm256_storeu_si256((__m256i *)(r + i),
		                     _mm256_add_epi16(_mm256_add_epi16(av, av), av));
	}
	for(; i < NTRUOAEP_N; i++)
		r[i] = 3 * a[i];
}

static inline __attribute__((always_inline))
__m256i crepmod3_16x16_avx2(__m256i v)
{
	const __m256i q = _mm256_set1_epi16(NTRUOAEP_Q);
	const __m256i qminus1_half = _mm256_set1_epi16((NTRUOAEP_Q - 1) / 2);
	const __m256i qplus1_half = _mm256_set1_epi16((NTRUOAEP_Q + 1) / 2);
	const __m256i third = _mm256_set1_epi16(10923);
#if NTRUOAEP_Q == 7129
	const __m256i negtwo = _mm256_set1_epi16(-2);
	const __m256i two = _mm256_set1_epi16(2);
	const __m256i three = _mm256_set1_epi16(3);
#endif
	__m256i quotient;

	v = _mm256_add_epi16(v, _mm256_and_si256(_mm256_srai_epi16(v, 15), q));
	v = _mm256_sub_epi16(v, qminus1_half);
	v = _mm256_add_epi16(v, _mm256_and_si256(_mm256_srai_epi16(v, 15), q));
	v = _mm256_sub_epi16(v, qplus1_half);

	quotient = _mm256_mulhrs_epi16(v, third);
	v = _mm256_sub_epi16(v, _mm256_add_epi16(quotient,
	                                        _mm256_add_epi16(quotient, quotient)));
#if NTRUOAEP_Q == 7129
	v = _mm256_sub_epi16(v, _mm256_and_si256(_mm256_cmpeq_epi16(v, two), three));
	return _mm256_add_epi16(v, _mm256_and_si256(_mm256_cmpeq_epi16(v, negtwo), three));
#else
	return v;
#endif
}

void poly_crepmod3_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N])
{
	int i;

	for(i = 0; i <= NTRUOAEP_N - 64; i += 64)
	{
		__m256i v0 = _mm256_loadu_si256((const __m256i *)(a + i));
		__m256i v1 = _mm256_loadu_si256((const __m256i *)(a + i + 16));
		__m256i v2 = _mm256_loadu_si256((const __m256i *)(a + i + 32));
		__m256i v3 = _mm256_loadu_si256((const __m256i *)(a + i + 48));

		_mm256_storeu_si256((__m256i *)(r + i), crepmod3_16x16_avx2(v0));
		_mm256_storeu_si256((__m256i *)(r + i + 16), crepmod3_16x16_avx2(v1));
		_mm256_storeu_si256((__m256i *)(r + i + 32), crepmod3_16x16_avx2(v2));
		_mm256_storeu_si256((__m256i *)(r + i + 48), crepmod3_16x16_avx2(v3));
	}
	for(; i <= NTRUOAEP_N - 16; i += 16)
	{
		__m256i v = _mm256_loadu_si256((const __m256i *)(a + i));

		_mm256_storeu_si256((__m256i *)(r + i), crepmod3_16x16_avx2(v));
	}
#if (NTRUOAEP_N % 16) != 0
	for(; i < NTRUOAEP_N; i++)
	{
		int16_t v = a[i];
		v += (v >> 15) & NTRUOAEP_Q;
		v -= (NTRUOAEP_Q - 1) / 2;
		v += (v >> 15) & NTRUOAEP_Q;
		v -= (NTRUOAEP_Q + 1) / 2;
		v = (v >> 8) + (v & 255);
		v = (v >> 4) + (v & 15);
		v = (v >> 2) + (v & 3);
		v = (v >> 2) + (v & 3);
		v -= 3;
		v += ((v + 1) >> 15) & 3;
		r[i] = v;
	}
#endif
}

void poly_tomontgomery_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N])
{
	int i;

	for(i = 0; i <= NTRUOAEP_N - 16; i += 16)
	{
		__m256i av = _mm256_loadu_si256((const __m256i *)(a + i));
		_mm256_storeu_si256((__m256i *)(r + i), fqmul_const_16x16(av, 1440));
	}
	for(; i < NTRUOAEP_N; i++)
		r[i] = montgomery_reduce((int32_t)a[i] * 1440);
}

void poly_frommontgomery_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N])
{
	int i;

	for(i = 0; i <= NTRUOAEP_N - 16; i += 16)
	{
		__m256i av = _mm256_loadu_si256((const __m256i *)(a + i));
		_mm256_storeu_si256((__m256i *)(r + i), montgomery_reduce_16x16(av));
	}
	for(; i < NTRUOAEP_N; i++)
		r[i] = montgomery_reduce(a[i]);
}

static void ntt_first_two_layers_avx2(int16_t r[NTRUOAEP_N],
                                      const int16_t a[NTRUOAEP_N])
{
	const __m256i q = _mm256_set1_epi16(NTRUOAEP_Q);
	const __m256i qinv = _mm256_set1_epi16(QINV);
	const __m256i sign = _mm256_set1_epi16((short)0x8000);
	const __m256i one = _mm256_set1_epi16(1);
	const __m256i zfirst = _mm256_set1_epi16(zetas[1]);
	const __m256i zlo1 = _mm256_set1_epi16(zetas[2]);
	const __m256i zlo2 = _mm256_set1_epi16(zetas[3]);
	const __m256i zhi1 = _mm256_set1_epi16(zetas[4]);
	const __m256i zhi2 = _mm256_set1_epi16(zetas[5]);
	int i;

	for(i = 0; i <= NTRUOAEP_N / 6 - 16; i += 16)
	{
		__m256i a0 = _mm256_loadu_si256((const __m256i *)(a + i));
		__m256i a1 = _mm256_loadu_si256((const __m256i *)(a + i + NTRUOAEP_N / 6));
		__m256i a2 = _mm256_loadu_si256((const __m256i *)(a + i + NTRUOAEP_N / 3));
		__m256i b0 = _mm256_loadu_si256((const __m256i *)(a + i + NTRUOAEP_N / 2));
		__m256i b1 = _mm256_loadu_si256((const __m256i *)(a + i + 2 * NTRUOAEP_N / 3));
		__m256i b2 = _mm256_loadu_si256((const __m256i *)(a + i + 5 * NTRUOAEP_N / 6));
		__m256i t0 = fqmul_const_16x16_precomp(b0, zfirst, q, qinv, sign, one);
		__m256i t1 = fqmul_const_16x16_precomp(b1, zfirst, q, qinv, sign, one);
		__m256i t2 = fqmul_const_16x16_precomp(b2, zfirst, q, qinv, sign, one);
		__m256i lo0 = _mm256_add_epi16(a0, t0);
		__m256i lo1 = _mm256_add_epi16(a1, t1);
		__m256i lo2 = _mm256_add_epi16(a2, t2);
		__m256i hi0 = _mm256_sub_epi16(_mm256_add_epi16(a0, b0), t0);
		__m256i hi1 = _mm256_sub_epi16(_mm256_add_epi16(a1, b1), t1);
		__m256i hi2 = _mm256_sub_epi16(_mm256_add_epi16(a2, b2), t2);
		__m256i o0, o1, o2;

		ntt_radix3_noreduce_vec(lo0, lo1, lo2, zlo1, zlo2,
		                        q, qinv, sign, one, &o0, &o1, &o2);
		_mm256_storeu_si256((__m256i *)(r + i), o0);
		_mm256_storeu_si256((__m256i *)(r + i + NTRUOAEP_N / 6), o1);
		_mm256_storeu_si256((__m256i *)(r + i + NTRUOAEP_N / 3), o2);

		ntt_radix3_noreduce_vec(hi0, hi1, hi2, zhi1, zhi2,
		                        q, qinv, sign, one, &o0, &o1, &o2);
		_mm256_storeu_si256((__m256i *)(r + i + NTRUOAEP_N / 2), o0);
		_mm256_storeu_si256((__m256i *)(r + i + 2 * NTRUOAEP_N / 3), o1);
		_mm256_storeu_si256((__m256i *)(r + i + 5 * NTRUOAEP_N / 6), o2);
	}
	if(i <= NTRUOAEP_N / 6 - 8)
	{
		__m256i a0 = load8_i16_zeroupper(a + i);
		__m256i a1 = load8_i16_zeroupper(a + i + NTRUOAEP_N / 6);
		__m256i a2 = load8_i16_zeroupper(a + i + NTRUOAEP_N / 3);
		__m256i b0 = load8_i16_zeroupper(a + i + NTRUOAEP_N / 2);
		__m256i b1 = load8_i16_zeroupper(a + i + 2 * NTRUOAEP_N / 3);
		__m256i b2 = load8_i16_zeroupper(a + i + 5 * NTRUOAEP_N / 6);
		__m256i t0 = fqmul_const_16x16_precomp(b0, zfirst, q, qinv, sign, one);
		__m256i t1 = fqmul_const_16x16_precomp(b1, zfirst, q, qinv, sign, one);
		__m256i t2 = fqmul_const_16x16_precomp(b2, zfirst, q, qinv, sign, one);
		__m256i lo0 = _mm256_add_epi16(a0, t0);
		__m256i lo1 = _mm256_add_epi16(a1, t1);
		__m256i lo2 = _mm256_add_epi16(a2, t2);
		__m256i hi0 = _mm256_sub_epi16(_mm256_add_epi16(a0, b0), t0);
		__m256i hi1 = _mm256_sub_epi16(_mm256_add_epi16(a1, b1), t1);
		__m256i hi2 = _mm256_sub_epi16(_mm256_add_epi16(a2, b2), t2);
		__m256i o0, o1, o2;

		ntt_radix3_noreduce_vec(lo0, lo1, lo2, zlo1, zlo2,
		                        q, qinv, sign, one, &o0, &o1, &o2);
		store8_i16(r + i, o0);
		store8_i16(r + i + NTRUOAEP_N / 6, o1);
		store8_i16(r + i + NTRUOAEP_N / 3, o2);

		ntt_radix3_noreduce_vec(hi0, hi1, hi2, zhi1, zhi2,
		                        q, qinv, sign, one, &o0, &o1, &o2);
		store8_i16(r + i + NTRUOAEP_N / 2, o0);
		store8_i16(r + i + 2 * NTRUOAEP_N / 3, o1);
		store8_i16(r + i + 5 * NTRUOAEP_N / 6, o2);
		i += 8;
	}
	if(i < NTRUOAEP_N / 6)
	{
		__m256i a0 = load4_i16_zeroupper(a + i);
		__m256i a1 = load4_i16_zeroupper(a + i + NTRUOAEP_N / 6);
		__m256i a2 = load4_i16_zeroupper(a + i + NTRUOAEP_N / 3);
		__m256i b0 = load4_i16_zeroupper(a + i + NTRUOAEP_N / 2);
		__m256i b1 = load4_i16_zeroupper(a + i + 2 * NTRUOAEP_N / 3);
		__m256i b2 = load4_i16_zeroupper(a + i + 5 * NTRUOAEP_N / 6);
		__m256i t0 = fqmul_const_16x16_precomp(b0, zfirst, q, qinv, sign, one);
		__m256i t1 = fqmul_const_16x16_precomp(b1, zfirst, q, qinv, sign, one);
		__m256i t2 = fqmul_const_16x16_precomp(b2, zfirst, q, qinv, sign, one);
		__m256i lo0 = _mm256_add_epi16(a0, t0);
		__m256i lo1 = _mm256_add_epi16(a1, t1);
		__m256i lo2 = _mm256_add_epi16(a2, t2);
		__m256i hi0 = _mm256_sub_epi16(_mm256_add_epi16(a0, b0), t0);
		__m256i hi1 = _mm256_sub_epi16(_mm256_add_epi16(a1, b1), t1);
		__m256i hi2 = _mm256_sub_epi16(_mm256_add_epi16(a2, b2), t2);
		__m256i o0, o1, o2;

		ntt_radix3_noreduce_vec(lo0, lo1, lo2, zlo1, zlo2,
		                        q, qinv, sign, one, &o0, &o1, &o2);
		store4_i16(r + i, o0);
		store4_i16(r + i + NTRUOAEP_N / 6, o1);
		store4_i16(r + i + NTRUOAEP_N / 3, o2);

		ntt_radix3_noreduce_vec(hi0, hi1, hi2, zhi1, zhi2,
		                        q, qinv, sign, one, &o0, &o1, &o2);
		store4_i16(r + i + NTRUOAEP_N / 2, o0);
		store4_i16(r + i + 2 * NTRUOAEP_N / 3, o1);
		store4_i16(r + i + 5 * NTRUOAEP_N / 6, o2);
	}
}

static void ntt_radix3_layer_avx2(int16_t r[NTRUOAEP_N], int start, int step,
                                  int16_t zeta1, int16_t zeta2)
{
	const __m256i q = _mm256_set1_epi16(NTRUOAEP_Q);
	const __m256i qinv = _mm256_set1_epi16(QINV);
	const __m256i sign = _mm256_set1_epi16((short)0x8000);
	const __m256i one = _mm256_set1_epi16(1);
	const __m256i z1 = _mm256_set1_epi16(zeta1);
	const __m256i z2 = _mm256_set1_epi16(zeta2);
	const __m256i omega = _mm256_set1_epi16(-714);
	int i;

	for(i = start; i <= start + step - 16; i += 16)
	{
		__m256i r0 = _mm256_loadu_si256((const __m256i *)(r + i));
		__m256i r1 = _mm256_loadu_si256((const __m256i *)(r + i + step));
		__m256i r2 = _mm256_loadu_si256((const __m256i *)(r + i + 2 * step));
		__m256i t1 = fqmul_const_16x16_precomp(r1, z1, q, qinv, sign, one);
		__m256i t2 = fqmul_const_16x16_precomp(r2, z2, q, qinv, sign, one);
		__m256i t3 = fqmul_const_16x16_precomp(_mm256_sub_epi16(t1, t2), omega, q, qinv, sign, one);

		_mm256_storeu_si256((__m256i *)(r + i + 2 * step),
		                     _mm256_sub_epi16(_mm256_sub_epi16(r0, t1), t3));
		_mm256_storeu_si256((__m256i *)(r + i + step),
		                     _mm256_add_epi16(_mm256_sub_epi16(r0, t2), t3));
		_mm256_storeu_si256((__m256i *)(r + i),
		                     _mm256_add_epi16(_mm256_add_epi16(r0, t1), t2));
	}
	if(i <= start + step - 8)
	{
		__m256i r0 = load8_i16_zeroupper(r + i);
		__m256i r1 = load8_i16_zeroupper(r + i + step);
		__m256i r2 = load8_i16_zeroupper(r + i + 2 * step);
		__m256i t1 = fqmul_const_16x16_precomp(r1, z1, q, qinv, sign, one);
		__m256i t2 = fqmul_const_16x16_precomp(r2, z2, q, qinv, sign, one);
		__m256i t3 = fqmul_const_16x16_precomp(_mm256_sub_epi16(t1, t2), omega, q, qinv, sign, one);

		store8_i16(r + i + 2 * step,
		           _mm256_sub_epi16(_mm256_sub_epi16(r0, t1), t3));
		store8_i16(r + i + step,
		           _mm256_add_epi16(_mm256_sub_epi16(r0, t2), t3));
		store8_i16(r + i,
		           _mm256_add_epi16(_mm256_add_epi16(r0, t1), t2));
		i += 8;
	}
	if(i <= start + step - 4)
	{
		__m256i r0 = load4_i16_zeroupper(r + i);
		__m256i r1 = load4_i16_zeroupper(r + i + step);
		__m256i r2 = load4_i16_zeroupper(r + i + 2 * step);
		__m256i t1 = fqmul_const_16x16_precomp(r1, z1, q, qinv, sign, one);
		__m256i t2 = fqmul_const_16x16_precomp(r2, z2, q, qinv, sign, one);
		__m256i t3 = fqmul_const_16x16_precomp(_mm256_sub_epi16(t1, t2), omega, q, qinv, sign, one);

		store4_i16(r + i + 2 * step,
		           _mm256_sub_epi16(_mm256_sub_epi16(r0, t1), t3));
		store4_i16(r + i + step,
		           _mm256_add_epi16(_mm256_sub_epi16(r0, t2), t3));
		store4_i16(r + i,
		           _mm256_add_epi16(_mm256_add_epi16(r0, t1), t2));
		i += 4;
	}
	for(; i < start + step; i++)
	{
		int16_t t1 = montgomery_reduce((int32_t)zeta1 * r[i + step]);
		int16_t t2 = montgomery_reduce((int32_t)zeta2 * r[i + 2 * step]);
		int16_t t3 = montgomery_reduce((int32_t)-714 * (t1 - t2));

		r[i + 2 * step] = r[i] - t1 - t3;
		r[i + step] = r[i] - t2 + t3;
		r[i] = r[i] + t1 + t2;
	}
}

static void ntt_radix3_layer12_avx2(int16_t r[NTRUOAEP_N], int start,
                                    int16_t zeta1, int16_t zeta2)
{
	const __m256i z1 = _mm256_set1_epi32(zeta1);
	const __m256i z2 = _mm256_set1_epi32(zeta2);
	const __m256i omega = _mm256_set1_epi32(-714);
	int i;

	for(i = start; i <= start + 12 - 8; i += 8)
	{
		__m256i r0 = load8_contiguous_i16(r + i);
		__m256i r1 = load8_contiguous_i16(r + i + 12);
		__m256i r2 = load8_contiguous_i16(r + i + 24);
		__m256i t1 = fqmul_8x32(r1, z1);
		__m256i t2 = fqmul_8x32(r2, z2);
		__m256i t3 = fqmul_8x32(_mm256_sub_epi32(t1, t2), omega);

		store8_contiguous_i16(r + i + 24,
		                      barrett_reduce_8x32_full(_mm256_sub_epi32(_mm256_sub_epi32(r0, t1), t3)));
		store8_contiguous_i16(r + i + 12,
		                      barrett_reduce_8x32_full(_mm256_add_epi32(_mm256_sub_epi32(r0, t2), t3)));
		store8_contiguous_i16(r + i,
		                      barrett_reduce_8x32_full(_mm256_add_epi32(_mm256_add_epi32(r0, t1), t2)));
	}
	if(i < start + 12)
	{
		__m256i r0 = load4_contiguous_i16(r + i);
		__m256i r1 = load4_contiguous_i16(r + i + 12);
		__m256i r2 = load4_contiguous_i16(r + i + 24);
		__m256i t1 = fqmul_8x32(r1, z1);
		__m256i t2 = fqmul_8x32(r2, z2);
		__m256i t3 = fqmul_8x32(_mm256_sub_epi32(t1, t2), omega);

		store4_contiguous_i16(r + i + 24,
		                      barrett_reduce_8x32_full(_mm256_sub_epi32(_mm256_sub_epi32(r0, t1), t3)));
		store4_contiguous_i16(r + i + 12,
		                      barrett_reduce_8x32_full(_mm256_add_epi32(_mm256_sub_epi32(r0, t2), t3)));
		store4_contiguous_i16(r + i,
		                      barrett_reduce_8x32_full(_mm256_add_epi32(_mm256_add_epi32(r0, t1), t2)));
		i += 4;
	}
	for(; i < start + 12; i++)
	{
		int16_t t1 = montgomery_reduce((int32_t)zeta1 * r[i + 12]);
		int16_t t2 = montgomery_reduce((int32_t)zeta2 * r[i + 24]);
		int16_t t3 = montgomery_reduce((int32_t)-714 * (t1 - t2));

		r[i + 24] = barrett_reduce(r[i] - t1 - t3);
		r[i + 12] = barrett_reduce(r[i] - t2 + t3);
		r[i] = barrett_reduce(r[i] + t1 + t2);
	}
}

static void invntt_radix3_layer_avx2(int16_t r[NTRUOAEP_N], int start, int step,
                                     int16_t zeta1, int16_t zeta2)
{
	const __m256i q = _mm256_set1_epi16(NTRUOAEP_Q);
	const __m256i qinv = _mm256_set1_epi16(QINV);
	const __m256i sign = _mm256_set1_epi16((short)0x8000);
	const __m256i one = _mm256_set1_epi16(1);
	const __m256i z1 = _mm256_set1_epi16(zeta1);
	const __m256i z2 = _mm256_set1_epi16(zeta2);
	const __m256i omega = _mm256_set1_epi16(-714);
	int i;

	for(i = start; i <= start + step - 16; i += 16)
	{
		__m256i r0 = _mm256_loadu_si256((const __m256i *)(r + i));
		__m256i r1 = _mm256_loadu_si256((const __m256i *)(r + i + step));
		__m256i r2 = _mm256_loadu_si256((const __m256i *)(r + i + 2 * step));
		__m256i t1 = fqmul_const_16x16_precomp(_mm256_sub_epi16(r1, r0), omega, q, qinv, sign, one);
		__m256i t2 = fqmul_const_16x16_precomp(_mm256_add_epi16(_mm256_sub_epi16(r2, r0), t1), z1, q, qinv, sign, one);
		__m256i t3 = fqmul_const_16x16_precomp(_mm256_sub_epi16(_mm256_sub_epi16(r2, r1), t1), z2, q, qinv, sign, one);

		_mm256_storeu_si256((__m256i *)(r + i),
		                     barrett_reduce_16x16(_mm256_add_epi16(_mm256_add_epi16(r0, r1), r2)));
		_mm256_storeu_si256((__m256i *)(r + i + step), t2);
		_mm256_storeu_si256((__m256i *)(r + i + 2 * step), t3);
	}
	if(i <= start + step - 8)
	{
		__m256i r0 = load8_i16_zeroupper(r + i);
		__m256i r1 = load8_i16_zeroupper(r + i + step);
		__m256i r2 = load8_i16_zeroupper(r + i + 2 * step);
		__m256i t1 = fqmul_const_16x16_precomp(_mm256_sub_epi16(r1, r0), omega, q, qinv, sign, one);
		__m256i t2 = fqmul_const_16x16_precomp(_mm256_add_epi16(_mm256_sub_epi16(r2, r0), t1), z1, q, qinv, sign, one);
		__m256i t3 = fqmul_const_16x16_precomp(_mm256_sub_epi16(_mm256_sub_epi16(r2, r1), t1), z2, q, qinv, sign, one);

		store8_i16(r + i,
		           barrett_reduce_16x16(_mm256_add_epi16(_mm256_add_epi16(r0, r1), r2)));
		store8_i16(r + i + step, t2);
		store8_i16(r + i + 2 * step, t3);
		i += 8;
	}
	if(i <= start + step - 4)
	{
		__m256i r0 = load4_i16_zeroupper(r + i);
		__m256i r1 = load4_i16_zeroupper(r + i + step);
		__m256i r2 = load4_i16_zeroupper(r + i + 2 * step);
		__m256i t1 = fqmul_const_16x16_precomp(_mm256_sub_epi16(r1, r0), omega, q, qinv, sign, one);
		__m256i t2 = fqmul_const_16x16_precomp(_mm256_add_epi16(_mm256_sub_epi16(r2, r0), t1), z1, q, qinv, sign, one);
		__m256i t3 = fqmul_const_16x16_precomp(_mm256_sub_epi16(_mm256_sub_epi16(r2, r1), t1), z2, q, qinv, sign, one);

		store4_i16(r + i,
		           barrett_reduce_16x16(_mm256_add_epi16(_mm256_add_epi16(r0, r1), r2)));
		store4_i16(r + i + step, t2);
		store4_i16(r + i + 2 * step, t3);
		i += 4;
	}
	for(; i < start + step; i++)
	{
		int16_t t1 = montgomery_reduce((int32_t)-714 * (r[i + step] - r[i]));
		int16_t t2 = montgomery_reduce((int32_t)zeta1 * (r[i + 2 * step] - r[i] + t1));
		int16_t t3 = montgomery_reduce((int32_t)zeta2 * (r[i + 2 * step] - r[i + step] - t1));

		r[i] = barrett_reduce(r[i] + r[i + step] + r[i + 2 * step]);
		r[i + step] = t2;
		r[i + 2 * step] = t3;
	}
}

static void invntt_radix3_layer12_avx2(int16_t r[NTRUOAEP_N], int start,
                                       int16_t zeta1, int16_t zeta2)
{
	int i;

	for(i = start; i <= start + 12 - 8; i += 8)
	{
		__m256i r0 = load8_i16_zeroupper(r + i);
		__m256i r1 = load8_i16_zeroupper(r + i + 12);
		__m256i r2 = load8_i16_zeroupper(r + i + 24);
		__m256i t1 = fqmul_const_16x16(_mm256_sub_epi16(r1, r0), -714);
		__m256i t2 = fqmul_const_16x16(_mm256_add_epi16(_mm256_sub_epi16(r2, r0), t1), zeta1);
		__m256i t3 = fqmul_const_16x16(_mm256_sub_epi16(_mm256_sub_epi16(r2, r1), t1), zeta2);

		store8_i16(r + i,
		           barrett_reduce_16x16(_mm256_add_epi16(_mm256_add_epi16(r0, r1), r2)));
		store8_i16(r + i + 12, t2);
		store8_i16(r + i + 24, t3);
	}
	if(i < start + 12)
	{
		__m256i r0 = load4_i16_zeroupper(r + i);
		__m256i r1 = load4_i16_zeroupper(r + i + 12);
		__m256i r2 = load4_i16_zeroupper(r + i + 24);
		__m256i t1 = fqmul_const_16x16(_mm256_sub_epi16(r1, r0), -714);
		__m256i t2 = fqmul_const_16x16(_mm256_add_epi16(_mm256_sub_epi16(r2, r0), t1), zeta1);
		__m256i t3 = fqmul_const_16x16(_mm256_sub_epi16(_mm256_sub_epi16(r2, r1), t1), zeta2);

		store4_i16(r + i,
		           barrett_reduce_16x16(_mm256_add_epi16(_mm256_add_epi16(r0, r1), r2)));
		store4_i16(r + i + 12, t2);
		store4_i16(r + i + 24, t3);
		i += 4;
	}
	for(; i < start + 12; i++)
	{
		int16_t t1 = montgomery_reduce((int32_t)-714 * (r[i + 12] - r[i]));
		int16_t t2 = montgomery_reduce((int32_t)zeta1 * (r[i + 24] - r[i] + t1));
		int16_t t3 = montgomery_reduce((int32_t)zeta2 * (r[i + 24] - r[i + 12] - t1));

		r[i] = barrett_reduce(r[i] + r[i + 12] + r[i + 24]);
		r[i + 12] = t2;
		r[i + 24] = t3;
	}
}

static void ntt_radix2_tail_scalar(int16_t r[NTRUOAEP_N], const int16_t *zeta_step6,
                                   const int16_t *zeta_step3, int block, int nblocks)
{
	for(; block < nblocks; block++)
	{
		const int start = block * 12;
		const int16_t z6 = zeta_step6[block];
		const int16_t z3_lo = zeta_step3[2 * block];
		const int16_t z3_hi = zeta_step3[2 * block + 1];
		int16_t lo[6], hi[6];

		for(int j = 0; j < 6; j++)
		{
			const int16_t a = r[start + j];
			const int16_t b = r[start + 6 + j];
			const int16_t t = montgomery_reduce((int32_t)z6 * b);

			lo[j] = a + t;
			hi[j] = a - t;
		}

		for(int j = 0; j < 3; j++)
		{
			int16_t t = montgomery_reduce((int32_t)z3_lo * lo[j + 3]);
			r[start + j] = barrett_reduce(lo[j] + t);
			r[start + 3 + j] = barrett_reduce(lo[j] - t);

			t = montgomery_reduce((int32_t)z3_hi * hi[j + 3]);
			r[start + 6 + j] = barrett_reduce(hi[j] + t);
			r[start + 9 + j] = barrett_reduce(hi[j] - t);
		}
	}
}

static void ntt_radix2_tail_avx2(int16_t r[NTRUOAEP_N], const int16_t *zeta_step6,
                                 const int16_t *zeta_step3)
{
	const int nblocks = NTRUOAEP_N / 12;
	int block;

	for(block = 0; block <= nblocks - 8; block += 8)
	{
		const __m256i z6 = load8_tail_zetas(zeta_step6, block);
		const __m256i z3_lo = load8_tail_zetas_even(zeta_step3, block);
		const __m256i z3_hi = load8_tail_zetas_odd(zeta_step3, block);
		__m256i lo[6], hi[6];

		for(int j = 0; j < 6; j++)
		{
			const __m256i a = load8_tail_coeffs(r, block, j);
			const __m256i b = load8_tail_coeffs(r, block, j + 6);
			const __m256i t = fqmul_8x32(b, z6);

			lo[j] = _mm256_add_epi32(a, t);
			hi[j] = _mm256_sub_epi32(a, t);
		}

		for(int j = 0; j < 3; j++)
		{
			__m256i t = fqmul_8x32(lo[j + 3], z3_lo);

			store8_tail_coeffs(r, block, j,
			                   barrett_reduce_8x32_full(_mm256_add_epi32(lo[j], t)));
			store8_tail_coeffs(r, block, j + 3,
			                   barrett_reduce_8x32_full(_mm256_sub_epi32(lo[j], t)));

			t = fqmul_8x32(hi[j + 3], z3_hi);
			store8_tail_coeffs(r, block, j + 6,
			                   barrett_reduce_8x32_full(_mm256_add_epi32(hi[j], t)));
			store8_tail_coeffs(r, block, j + 9,
			                   barrett_reduce_8x32_full(_mm256_sub_epi32(hi[j], t)));
		}
	}

	ntt_radix2_tail_scalar(r, zeta_step6, zeta_step3, block, nblocks);
}

static void invntt_radix2_head_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N],
                                    const int16_t *zeta_step3, const int16_t *zeta_step6)
{
	const __m256i q = _mm256_set1_epi16(NTRUOAEP_Q);
	const __m256i qinv = _mm256_set1_epi16(QINV);
	const __m256i sign = _mm256_set1_epi16((short)0x8000);
	const __m256i one = _mm256_set1_epi16(1);

	for(int block = 0; block < NTRUOAEP_N / 12; block++)
	{
		const int start = block * 12;
		const __m256i z3_lo = _mm256_set1_epi16(zeta_step3[-2 * block]);
		const __m256i z3_hi = _mm256_set1_epi16(zeta_step3[-2 * block - 1]);
		const __m256i z6 = _mm256_set1_epi16(zeta_step6[-block]);
		__m256i lo = invntt_head_step3_6(load8_i16_zeroupper(a + start),
		                                 z3_lo, q, qinv, sign, one);
		__m256i hi = invntt_head_step3_6(load8_i16_zeroupper(a + start + 6),
		                                 z3_hi, q, qinv, sign, one);

		store6_i16(r + start, barrett_reduce_16x16(_mm256_add_epi16(lo, hi)));
		store6_i16(r + start + 6,
		           fqmul_const_16x16_precomp(_mm256_sub_epi16(hi, lo),
		                                     z6, q, qinv, sign, one));
	}
}

static void invntt_last_layer_avx2(int16_t r[NTRUOAEP_N])
{
	const __m256i q = _mm256_set1_epi16(NTRUOAEP_Q);
	const __m256i qinv = _mm256_set1_epi16(QINV);
	const __m256i sign = _mm256_set1_epi16((short)0x8000);
	const __m256i one = _mm256_set1_epi16(1);
	const __m256i c2394 = _mm256_set1_epi16(2394);
	const __m256i c2383 = _mm256_set1_epi16(2383);
	const __m256i cm2363 = _mm256_set1_epi16(-2363);
	int i;

	for(i = 0; i <= NTRUOAEP_N / 2 - 16; i += 16)
	{
		__m256i lo = _mm256_loadu_si256((const __m256i *)(r + i));
		__m256i hi = _mm256_loadu_si256((const __m256i *)(r + i + NTRUOAEP_N / 2));
		__m256i t1 = _mm256_add_epi16(lo, hi);
		__m256i t2 = fqmul_const_16x16_precomp(_mm256_sub_epi16(lo, hi), c2394, q, qinv, sign, one);

		_mm256_storeu_si256((__m256i *)(r + i),
		                     fqmul_const_16x16_precomp(_mm256_sub_epi16(t1, t2), c2383, q, qinv, sign, one));
		_mm256_storeu_si256((__m256i *)(r + i + NTRUOAEP_N / 2),
		                     fqmul_const_16x16_precomp(t2, cm2363, q, qinv, sign, one));
	}
	if(i <= NTRUOAEP_N / 2 - 8)
	{
		__m256i lo = load8_i16_zeroupper(r + i);
		__m256i hi = load8_i16_zeroupper(r + i + NTRUOAEP_N / 2);
		__m256i t1 = _mm256_add_epi16(lo, hi);
		__m256i t2 = fqmul_const_16x16_precomp(_mm256_sub_epi16(lo, hi), c2394, q, qinv, sign, one);

		store8_i16(r + i, fqmul_const_16x16_precomp(_mm256_sub_epi16(t1, t2), c2383, q, qinv, sign, one));
		store8_i16(r + i + NTRUOAEP_N / 2, fqmul_const_16x16_precomp(t2, cm2363, q, qinv, sign, one));
		i += 8;
	}
	if(i <= NTRUOAEP_N / 2 - 4)
	{
		__m256i lo = load4_i16_zeroupper(r + i);
		__m256i hi = load4_i16_zeroupper(r + i + NTRUOAEP_N / 2);
		__m256i t1 = _mm256_add_epi16(lo, hi);
		__m256i t2 = fqmul_const_16x16_precomp(_mm256_sub_epi16(lo, hi), c2394, q, qinv, sign, one);

		store4_i16(r + i, fqmul_const_16x16_precomp(_mm256_sub_epi16(t1, t2), c2383, q, qinv, sign, one));
		store4_i16(r + i + NTRUOAEP_N / 2, fqmul_const_16x16_precomp(t2, cm2363, q, qinv, sign, one));
		i += 4;
	}
	for(; i < NTRUOAEP_N / 2; i++)
	{
		int16_t t1 = r[i] + r[i + NTRUOAEP_N / 2];
		int16_t t2 = montgomery_reduce((int32_t)2394 * (r[i] - r[i + NTRUOAEP_N / 2]));

		r[i] = montgomery_reduce((int32_t)2383 * (t1 - t2));
		r[i + NTRUOAEP_N / 2] = montgomery_reduce((int32_t)-2363 * t2);
	}
}

static void invntt_normalized_last_layer_avx2(int16_t r[NTRUOAEP_N])
{
	const __m256i q = _mm256_set1_epi16(NTRUOAEP_Q);
	const __m256i qinv = _mm256_set1_epi16(QINV);
	const __m256i sign = _mm256_set1_epi16((short)0x8000);
	const __m256i one = _mm256_set1_epi16(1);
	const __m256i c2394 = _mm256_set1_epi16(2394);
	const __m256i cm2601 = _mm256_set1_epi16(-2601);
	const __m256i c1927 = _mm256_set1_epi16(1927);
	int i;

	for(i = 0; i <= NTRUOAEP_N / 2 - 16; i += 16)
	{
		__m256i lo = _mm256_loadu_si256((const __m256i *)(r + i));
		__m256i hi = _mm256_loadu_si256((const __m256i *)(r + i + NTRUOAEP_N / 2));
		__m256i t1 = _mm256_add_epi16(lo, hi);
		__m256i t2 = fqmul_const_16x16_precomp(_mm256_sub_epi16(lo, hi), c2394, q, qinv, sign, one);

		_mm256_storeu_si256((__m256i *)(r + i),
		                     fqmul_const_16x16_precomp(_mm256_sub_epi16(t1, t2), cm2601, q, qinv, sign, one));
		_mm256_storeu_si256((__m256i *)(r + i + NTRUOAEP_N / 2),
		                     fqmul_const_16x16_precomp(t2, c1927, q, qinv, sign, one));
	}
	if(i <= NTRUOAEP_N / 2 - 8)
	{
		__m256i lo = load8_i16_zeroupper(r + i);
		__m256i hi = load8_i16_zeroupper(r + i + NTRUOAEP_N / 2);
		__m256i t1 = _mm256_add_epi16(lo, hi);
		__m256i t2 = fqmul_const_16x16_precomp(_mm256_sub_epi16(lo, hi), c2394, q, qinv, sign, one);

		store8_i16(r + i, fqmul_const_16x16_precomp(_mm256_sub_epi16(t1, t2), cm2601, q, qinv, sign, one));
		store8_i16(r + i + NTRUOAEP_N / 2, fqmul_const_16x16_precomp(t2, c1927, q, qinv, sign, one));
		i += 8;
	}
	if(i <= NTRUOAEP_N / 2 - 4)
	{
		__m256i lo = load4_i16_zeroupper(r + i);
		__m256i hi = load4_i16_zeroupper(r + i + NTRUOAEP_N / 2);
		__m256i t1 = _mm256_add_epi16(lo, hi);
		__m256i t2 = fqmul_const_16x16_precomp(_mm256_sub_epi16(lo, hi), c2394, q, qinv, sign, one);

		store4_i16(r + i, fqmul_const_16x16_precomp(_mm256_sub_epi16(t1, t2), cm2601, q, qinv, sign, one));
		store4_i16(r + i + NTRUOAEP_N / 2, fqmul_const_16x16_precomp(t2, c1927, q, qinv, sign, one));
		i += 4;
	}
	for(; i < NTRUOAEP_N / 2; i++)
	{
		int16_t t1 = r[i] + r[i + NTRUOAEP_N / 2];
		int16_t t2 = montgomery_reduce((int32_t)2394 * (r[i] - r[i + NTRUOAEP_N / 2]));

		r[i] = montgomery_reduce((int32_t)-2601 * (t1 - t2));
		r[i + NTRUOAEP_N / 2] = montgomery_reduce((int32_t)1927 * t2);
	}
}


/*
 * ntt(), invntt(), and invntt_normalized() are implemented as monolithic
 * assembly entrypoints in asm/ntt_full.s and asm/invntt_full.s.
 */

/*************************************************
* Name:        fqinv
*
* Description: Inversion via exponentiated by ord-1
*
* Arguments:   - int16_t a: first factor a = x mod q
*
* Returns 16-bit integer congruent to x^{-1} * R^2 mod q
**************************************************/
static int16_t fqinv(int16_t a)
{
	int x1, x2, x3, x4;

	x1 = fqmul(a, a);
	x2 = fqmul(x1, a);
	x3 = fqmul(x2, x2);
	x3 = fqmul(x3, x3);
	x3 = fqmul(x3, x3);
	x1 = fqmul(x3, x1);

	x3 = fqmul(x3, x2);
	x4 = fqmul(x3, x3);
	x4 = fqmul(x4, x4);
	x4 = fqmul(x4, x4);
	x4 = fqmul(x4, x4);
	x4 = fqmul(x4, x4);
	x4 = fqmul(x4, x1);
	
	x4 = fqmul(x4, x4);
	x4 = fqmul(x4, x4);
	x4 = fqmul(x4, x2);
	x4 = fqmul(x4, x4);
	x4 = fqmul(x4, a);

	return x4;

}


/*************************************************
* Name:        baseinv
*
* Description: Inversion of polynomial in Zq[X]/(X^3-zeta)
*              used for inversion of element in Rq in NTT domain
*
* Arguments:   - int16_t r[3]: pointer to the output polynomial
*              - const int16_t a[3]: pointer to the input polynomial
*              - int16_t zeta: integer defining the reduction polynomial
**************************************************/
int baseinv(int16_t r[3], const int16_t a[3], int16_t zeta)
{

	int16_t t0,t1,t2,t3,t4;

	t0 = montgomery_reduce(a[1]*a[1]-3*a[0]*a[2]);
	t4 = montgomery_reduce(a[2]*zeta);
	t1 = montgomery_reduce(t4*t4);
	t2 = montgomery_reduce(t0*a[1]); 

	t3 = montgomery_reduce(a[0]*a[0]);
	t3 = montgomery_reduce(t1*a[2]+t3*a[0]+t2*zeta);

	if(t3 == 0) return 1;

	t3 = fqinv(t3); 
	t0 = montgomery_reduce(a[1]*a[1]-a[0]*a[2]);
	
	
	t2 = fqmul(t4,t3);
	t1 = fqmul(a[0],t3);

	r[0] = montgomery_reduce(-a[1]*t2+a[0]*t1);
	r[1] = montgomery_reduce(a[2]*t2-a[1]*t1);
	r[2] = fqmul(t0,t3);
	
	return 0;
}

/* a-> a/R montgomery form */
/*************************************************
* Name:        basemul
*
* Description: Multiplication of polynomials in Zq[X]/(X^3-zeta)
*              used for multiplication of elements in Rq in NTT domain.
*
* Arguments:   - int16_t r[3]: pointer to the output polynomial
*              - const int16_t a[3]: pointer to the first factor
*              - const int16_t b[3]: pointer to the second factor
*              - int16_t zeta: integer defining the reduction polynomial
*
* a,b,zeta are all in montgomery form, returns in montgomery form
**************************************************/
void basemul(int16_t r[3], const int16_t a[3], const int16_t b[3], int16_t zeta)
{
	r[0] = montgomery_reduce(a[1] * b[2] + a[2] * b[1]);
	r[1] = montgomery_reduce(a[2] * b[2]);

	r[0] = montgomery_reduce(r[0]*zeta+a[0]*b[0]);
	r[1] = montgomery_reduce(r[1]*zeta+a[0]*b[1]+a[1]*b[0]);
	r[2] = montgomery_reduce(a[2]*b[0]+a[1]*b[1]+a[0]*b[2]);
}


/*************************************************
* Name:        basemul_add
*
* Description: Multiplication then addition of polynomials in Zq[X]/(X^3-zeta)
*              used for multiplication of elements in Rq in NTT domain
*
* Arguments:   - int16_t c[3]: pointer to the output polynomial
*              - const int16_t a[3]: pointer to the first factor
*              - const int16_t b[3]: pointer to the second factor
*              - const int16_t c[3]: pointer to the third factor
*              - int16_t zeta: integer defining the reduction polynomial
*
* a,b,c,zeta are all in montgomery form, returns in montgomery form
**************************************************/
void basemul_add(int16_t r[3], const int16_t a[3], const int16_t b[3], const int16_t c[3], int16_t zeta)
{
	r[0] = montgomery_reduce(a[1] * b[2] + a[2] * b[1]);
	r[1] = montgomery_reduce(a[2] * b[2]);

	r[0] = montgomery_reduce(c[0]*1375+r[0]*zeta+a[0]*b[0]);
	r[1] = montgomery_reduce(c[1]*1375+r[1]*zeta+a[0]*b[1]+a[1]*b[0]);
	r[2] = montgomery_reduce(c[2]*1375+a[2]*b[0]+a[1]*b[1]+a[0]*b[2]);
}
