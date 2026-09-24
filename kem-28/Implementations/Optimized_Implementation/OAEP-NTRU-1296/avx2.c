#include <immintrin.h>
#include <stdint.h>
#include "avx2.h"
#include "ntt.h"
#include "reduce.h"

void poly_baseinv_prepare_asm(__m256i *den, const int16_t *a);
int poly_baseinv_batchinvert_asm(__m256i *den);
void poly_baseinv_finalize_asm(int16_t *r, const int16_t *a, const __m256i *den);

#define POLY_SERIAL_BLOCKS (NTRUOAEP_N / 8)
#define POLY_SERIAL_BYTES_PER_BLOCK 15
#define POLY_SERIAL_UNROLLED_BLOCKS (POLY_SERIAL_BLOCKS & ~7)

static inline __attribute__((always_inline))
void store64(uint8_t *r, uint64_t v)
{
	__builtin_memcpy(r, &v, sizeof(v));
}

static inline __attribute__((always_inline))
void store32(uint8_t *r, uint32_t v)
{
	__builtin_memcpy(r, &v, sizeof(v));
}

static inline __attribute__((always_inline))
void store16(uint8_t *r, uint16_t v)
{
	__builtin_memcpy(r, &v, sizeof(v));
}

static inline __attribute__((always_inline))
uint64_t pack4_15bit(__m128i coeffs)
{
	const __m128i shifts = _mm_setr_epi32(0, 7, 6, 5);
	const __m128i shuf0 = _mm_setr_epi8(
		0, 1, 5, 6, 9, 10, 13, 14,
		(char)0x80, (char)0x80, (char)0x80, (char)0x80,
		(char)0x80, (char)0x80, (char)0x80, (char)0x80);
	const __m128i shuf1 = _mm_setr_epi8(
		(char)0x80, 4, (char)0x80, 8, (char)0x80, 12,
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
	uint64_t lo = pack4_15bit(coeffs);
	uint64_t hi = pack4_15bit(_mm_srli_si128(coeffs, 8));
	uint64_t first = lo | (hi << 60);
	uint64_t rest = hi >> 4;

	store64(r, first);
	store32(r + 8, (uint32_t)rest);
	store16(r + 12, (uint16_t)(rest >> 32));
	r[14] = (uint8_t)(rest >> 48);
}

void poly_tobytes_avx2(uint8_t r[restrict NTRUOAEP_POLYBYTES],
                       const int16_t a[restrict NTRUOAEP_N])
{
	const __m256i q = _mm256_set1_epi16(NTRUOAEP_Q);
	int block;

	for(block = 0; block < POLY_SERIAL_UNROLLED_BLOCKS; block += 8)
	{
		const int16_t *ai = a + 8 * block;
		uint8_t *ri = r + POLY_SERIAL_BYTES_PER_BLOCK * block;
		__m256i v0 = _mm256_loadu_si256((const __m256i *)ai);
		__m256i v1 = _mm256_loadu_si256((const __m256i *)(ai + 16));
		__m256i v2 = _mm256_loadu_si256((const __m256i *)(ai + 32));
		__m256i v3 = _mm256_loadu_si256((const __m256i *)(ai + 48));

		v0 = _mm256_add_epi16(v0, _mm256_and_si256(_mm256_srai_epi16(v0, 15), q));
		v1 = _mm256_add_epi16(v1, _mm256_and_si256(_mm256_srai_epi16(v1, 15), q));
		v2 = _mm256_add_epi16(v2, _mm256_and_si256(_mm256_srai_epi16(v2, 15), q));
		v3 = _mm256_add_epi16(v3, _mm256_and_si256(_mm256_srai_epi16(v3, 15), q));
		poly_tobytes_pack8_avx2(ri, _mm256_castsi256_si128(v0));
		poly_tobytes_pack8_avx2(ri + POLY_SERIAL_BYTES_PER_BLOCK,
		                        _mm256_extracti128_si256(v0, 1));
		poly_tobytes_pack8_avx2(ri + 2 * POLY_SERIAL_BYTES_PER_BLOCK,
		                        _mm256_castsi256_si128(v1));
		poly_tobytes_pack8_avx2(ri + 3 * POLY_SERIAL_BYTES_PER_BLOCK,
		                        _mm256_extracti128_si256(v1, 1));
		poly_tobytes_pack8_avx2(ri + 4 * POLY_SERIAL_BYTES_PER_BLOCK,
		                        _mm256_castsi256_si128(v2));
		poly_tobytes_pack8_avx2(ri + 5 * POLY_SERIAL_BYTES_PER_BLOCK,
		                        _mm256_extracti128_si256(v2, 1));
		poly_tobytes_pack8_avx2(ri + 6 * POLY_SERIAL_BYTES_PER_BLOCK,
		                        _mm256_castsi256_si128(v3));
		poly_tobytes_pack8_avx2(ri + 7 * POLY_SERIAL_BYTES_PER_BLOCK,
		                        _mm256_extracti128_si256(v3, 1));
	}

	for(; block < POLY_SERIAL_BLOCKS; block += 2)
	{
		const int16_t *ai = a + 8 * block;
		uint8_t *ri = r + POLY_SERIAL_BYTES_PER_BLOCK * block;
		__m256i v = _mm256_loadu_si256((const __m256i *)ai);

		v = _mm256_add_epi16(v, _mm256_and_si256(_mm256_srai_epi16(v, 15), q));
		poly_tobytes_pack8_avx2(ri, _mm256_castsi256_si128(v));
		poly_tobytes_pack8_avx2(ri + POLY_SERIAL_BYTES_PER_BLOCK,
		                        _mm256_extracti128_si256(v, 1));
	}
}

static inline __m128i pack_8x32_to_8x16(__m256i a)
{
	return _mm256_castsi256_si128(_mm256_permute4x64_epi64(
		_mm256_packs_epi32(a, a), 0x08));
}

static inline void poly_frombytes_unpack8_avx2(int16_t *r, const uint8_t *a)
{
	const __m256i shuf = _mm256_setr_epi8(
		0, 1, 2, 3, 1, 2, 3, 4, 3, 4, 5, 6, 5, 6, 7, 8,
		7, 8, 9, 10, 9, 10, 11, 12, 11, 12, 13, 14,
		13, 14, (char)0x80, (char)0x80);
	const __m256i shifts = _mm256_setr_epi32(0, 7, 6, 5, 4, 3, 2, 1);
	const __m256i mask = _mm256_set1_epi32(0x7fff);
	__m128i lo = _mm_loadl_epi64((const __m128i *)a);
	__m128i hi = _mm_loadl_epi64((const __m128i *)(a + 7));
	__m128i in128 = _mm_or_si128(lo, _mm_slli_si128(hi, 7));
	__m256i in = _mm256_broadcastsi128_si256(in128);
	__m256i coeffs = _mm256_and_si256(
		_mm256_srlv_epi32(_mm256_shuffle_epi8(in, shuf), shifts), mask);

	_mm_storeu_si128((__m128i *)r, pack_8x32_to_8x16(coeffs));
}

void poly_frombytes_avx2(int16_t r[restrict NTRUOAEP_N],
                         const uint8_t a[restrict NTRUOAEP_POLYBYTES])
{
	int block;

	for(block = 0; block < POLY_SERIAL_UNROLLED_BLOCKS; block += 8)
	{
		const uint8_t *ai = a + POLY_SERIAL_BYTES_PER_BLOCK * block;
		int16_t *ri = r + 8 * block;

		poly_frombytes_unpack8_avx2(ri, ai);
		poly_frombytes_unpack8_avx2(ri + 8, ai + POLY_SERIAL_BYTES_PER_BLOCK);
		poly_frombytes_unpack8_avx2(ri + 16, ai + 2 * POLY_SERIAL_BYTES_PER_BLOCK);
		poly_frombytes_unpack8_avx2(ri + 24, ai + 3 * POLY_SERIAL_BYTES_PER_BLOCK);
		poly_frombytes_unpack8_avx2(ri + 32, ai + 4 * POLY_SERIAL_BYTES_PER_BLOCK);
		poly_frombytes_unpack8_avx2(ri + 40, ai + 5 * POLY_SERIAL_BYTES_PER_BLOCK);
		poly_frombytes_unpack8_avx2(ri + 48, ai + 6 * POLY_SERIAL_BYTES_PER_BLOCK);
		poly_frombytes_unpack8_avx2(ri + 56, ai + 7 * POLY_SERIAL_BYTES_PER_BLOCK);
	}

	for(; block < POLY_SERIAL_BLOCKS; block++)
		poly_frombytes_unpack8_avx2(r + 8 * block,
		                            a + POLY_SERIAL_BYTES_PER_BLOCK * block);
}

#define CBD1_GROUPS (NTRUOAEP_N / 256)
#define CBD1_HALF_BYTES (NTRUOAEP_N / 8)
#define CBD1_TAIL_BYTES 2

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

	for(int i = 0; i < CBD1_GROUPS; i++)
	{
		__m256i t1 = _mm256_loadu_si256((const __m256i *)(buf + 32 * i));
		__m256i t2 = _mm256_loadu_si256((const __m256i *)(buf + 32 * i + CBD1_HALF_BYTES));

		CBD1_ROWS(store_cbd1_8_lanes, r + 256 * i, t1, t2, 16);
	}

	for(int byte = 0; byte < CBD1_TAIL_BYTES; byte++)
	{
		uint32_t t1 = buf[32 * CBD1_GROUPS + byte];
		uint32_t t2 = buf[CBD1_HALF_BYTES + 32 * CBD1_GROUPS + byte];

		for(int j = 0; j < 8; j++)
		{
			r[256 * CBD1_GROUPS + 8 * byte + j] = (int16_t)((t1 & 1) - (t2 & 1));
			t1 >>= 1;
			t2 >>= 1;
		}
	}
}

static inline void split_cbd1_coeffs_8_packed(__m256i packed,
                                              __m256i *even, __m256i *odd)
{
	const __m256i even_idx = _mm256_setr_epi32(0, 2, 4, 6, 0, 0, 0, 0);
	const __m256i odd_idx = _mm256_setr_epi32(1, 3, 5, 7, 0, 0, 0, 0);
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

static inline void split_cbd1_coeffs_8(const int16_t *a, __m256i *even, __m256i *odd)
{
	split_cbd1_coeffs_8_packed(_mm256_loadu_si256((const __m256i *)a), even, odd);
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

	for(int i = 0; i < CBD1_GROUPS; i++)
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

	for(int byte = 0; byte < CBD1_TAIL_BYTES; byte++)
	{
		uint32_t t2 = buf[32 * CBD1_GROUPS + byte];
		uint32_t t3 = 0;

		for(int j = 0; j < 8; j++)
		{
			uint32_t t4 = t2 & 1;

			t4 = t4 - a[256 * CBD1_GROUPS + 8 * byte + j];
			r |= t4;
			t3 ^= (t4 & 1) << j;
			t2 >>= 1;
		}

		w[32 * CBD1_GROUPS + byte] = (uint8_t)t3;
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

	for(int i = 0; i < CBD1_GROUPS; i++)
	{
		__m256i t1 = _mm256_loadu_si256((const __m256i *)(buf + 32 * i));
		__m256i t2 = _mm256_loadu_si256((const __m256i *)(buf + 32 * i + CBD1_HALF_BYTES));
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

	for(int byte = 0; byte < CBD1_TAIL_BYTES; byte++)
	{
		uint32_t t1 = buf[32 * CBD1_GROUPS + byte];
		uint32_t t2 = buf[CBD1_HALF_BYTES + 32 * CBD1_GROUPS + byte];
		uint32_t t3 = 0;

		for(int j = 0; j < 8; j++)
		{
			uint32_t t4 = t2 & 1;

			t4 = (uint32_t)a[256 * CBD1_GROUPS + 8 * byte + j] + t4;
			r |= t4;
			t3 ^= ((t4 ^ t1) & 1) << j;
			t1 >>= 1;
			t2 >>= 1;
		}

		msg[32 * CBD1_GROUPS + byte] = (uint8_t)t3;
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

static inline __m256i poly_sub_high_reduce_16x16(__m256i a)
{
	const __m256i q = _mm256_set1_epi16(NTRUOAEP_Q);
	const __m256i upper = _mm256_set1_epi16(NTRUOAEP_Q / 2 - 1);
	__m256i ge_upper = _mm256_cmpgt_epi16(a, upper);

	a = _mm256_sub_epi16(a, _mm256_and_si256(ge_upper, q));

	return a;
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

static inline __m256i fqmul_16x16(__m256i a, __m256i b)
{
	const __m256i q = _mm256_set1_epi16(NTRUOAEP_Q);
	const __m256i qinv = _mm256_set1_epi16(QINV);
	const __m256i sign = _mm256_set1_epi16((int16_t)0x8000);
	const __m256i one = _mm256_set1_epi16(1);
	__m256i prod_lo = _mm256_mullo_epi16(a, b);
	__m256i prod_hi = _mm256_mulhi_epi16(a, b);
	__m256i t = _mm256_mullo_epi16(prod_lo, qinv);
	__m256i tq_lo = _mm256_mullo_epi16(t, q);
	__m256i tq_hi = _mm256_mulhi_epi16(t, q);
	__m256i borrow = _mm256_cmpgt_epi16(_mm256_xor_si256(tq_lo, sign),
	                                    _mm256_xor_si256(prod_lo, sign));

	borrow = _mm256_and_si256(borrow, one);
	return _mm256_sub_epi16(_mm256_sub_epi16(prod_hi, tq_hi), borrow);
}

static inline __m256i fqmul_const_16x16(__m256i a, int32_t c)
{
	const __m256i cv = _mm256_set1_epi32(c);
	__m256i lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(a));
	__m256i hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(a, 1));
	__m128i rlo = montgomery_reduce_8x32(_mm256_mullo_epi32(lo, cv));
	__m128i rhi = montgomery_reduce_8x32(_mm256_mullo_epi32(hi, cv));

	return _mm256_inserti128_si256(_mm256_castsi128_si256(rlo), rhi, 1);
}

static inline __m256i montgomery_reduce_16x16(__m256i a)
{
	__m256i lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(a));
	__m256i hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(a, 1));
	__m128i rlo = montgomery_reduce_8x32(lo);
	__m128i rhi = montgomery_reduce_8x32(hi);

	return _mm256_inserti128_si256(_mm256_castsi128_si256(rlo), rhi, 1);
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

		_mm256_storeu_si256((__m256i *)(r + i),
		                     poly_sub_high_reduce_16x16(_mm256_sub_epi16(av0, bv0)));
		_mm256_storeu_si256((__m256i *)(r + i + 16),
		                     poly_sub_high_reduce_16x16(_mm256_sub_epi16(av1, bv1)));
		_mm256_storeu_si256((__m256i *)(r + i + 32),
		                     poly_sub_high_reduce_16x16(_mm256_sub_epi16(av2, bv2)));
		_mm256_storeu_si256((__m256i *)(r + i + 48),
		                     poly_sub_high_reduce_16x16(_mm256_sub_epi16(av3, bv3)));
	}
	for(; i <= NTRUOAEP_N - 16; i += 16)
	{
		__m256i av = _mm256_loadu_si256((const __m256i *)(a + i));
		__m256i bv = _mm256_loadu_si256((const __m256i *)(b + i));
		_mm256_storeu_si256((__m256i *)(r + i),
		                     poly_sub_high_reduce_16x16(_mm256_sub_epi16(av, bv)));
	}
	for(; i < NTRUOAEP_N; i++)
	{
		int16_t t = (int16_t)(a[i] - b[i]);
		int16_t mask = -(int16_t)((uint16_t)(NTRUOAEP_Q / 2 - 1 - t) >> 15);
		r[i] = (int16_t)(t - (NTRUOAEP_Q & mask));
	}
}

void poly_triple_avx2(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N])
{
	unsigned int i;

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
		_mm256_storeu_si256((__m256i *)(r + i), fqmul_const_16x16(av, -3797));
	}
	for(; i < NTRUOAEP_N; i++)
		r[i] = montgomery_reduce((int32_t)a[i] * -3797);
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

#define BASEINV_VECTORS (NTRUOAEP_N / (2 * 8))

int poly_baseinv_asm(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N])
{
	__m256i den[BASEINV_VECTORS];

	poly_baseinv_prepare_asm(den, a);
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
