#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>

static inline __m128i coeff_bit_pair(__m128i x, unsigned int high,
									 unsigned int low)
{
	const __m128i ones = _mm_set1_epi16(1);
	const __m128i lhs = _mm_and_si128(_mm_srli_epi16(x, high), ones);
	const __m128i rhs = _mm_and_si128(_mm_srli_epi16(x, low), ones);

	return _mm_sub_epi16(lhs, rhs);
}

static inline __m128i coeff_zero_pair(__m128i x, unsigned int high,
									  unsigned int low)
{
	const __m128i ones = _mm_set1_epi16(1);
	const __m128i zero = _mm_setzero_si128();
	const __m128i lhs_bits = _mm_and_si128(_mm_srli_epi16(x, low),
										   _mm_set1_epi16(3));
	const __m128i rhs_bits = _mm_and_si128(_mm_srli_epi16(x, high),
										   _mm_set1_epi16(3));
	const __m128i lhs = _mm_and_si128(_mm_cmpeq_epi16(lhs_bits, zero), ones);
	const __m128i rhs = _mm_and_si128(_mm_cmpeq_epi16(rhs_bits, zero), ones);

	return _mm_sub_epi16(lhs, rhs);
}

static inline void store_bd2_8(const uint8_t *bytes, uint16_t *out)
{
	const __m128i input = _mm_loadl_epi64((const __m128i *)bytes);
	const __m128i x = _mm_cvtepu8_epi16(input);
	const __m128i c0 = coeff_bit_pair(x, 1U, 0U);
	const __m128i c1 = coeff_bit_pair(x, 3U, 2U);
	const __m128i c2 = coeff_bit_pair(x, 5U, 4U);
	const __m128i c3 = coeff_bit_pair(x, 7U, 6U);
	const __m128i a01_lo = _mm_unpacklo_epi16(c0, c1);
	const __m128i a23_lo = _mm_unpacklo_epi16(c2, c3);
	const __m128i a01_hi = _mm_unpackhi_epi16(c0, c1);
	const __m128i a23_hi = _mm_unpackhi_epi16(c2, c3);

	_mm_storeu_si128((__m128i *)(out + 0U), _mm_unpacklo_epi32(a01_lo, a23_lo));
	_mm_storeu_si128((__m128i *)(out + 8U), _mm_unpackhi_epi32(a01_lo, a23_lo));
	_mm_storeu_si128((__m128i *)(out + 16U), _mm_unpacklo_epi32(a01_hi, a23_hi));
	_mm_storeu_si128((__m128i *)(out + 24U), _mm_unpackhi_epi32(a01_hi, a23_hi));
}

static inline void store_bd4_8(const uint8_t *bytes, uint16_t *out)
{
	const __m128i input = _mm_loadl_epi64((const __m128i *)bytes);
	const __m128i x = _mm_cvtepu8_epi16(input);
	const __m128i c0 = coeff_zero_pair(x, 2U, 0U);
	const __m128i c1 = coeff_zero_pair(x, 6U, 4U);

	_mm_storeu_si128((__m128i *)(out + 0U), _mm_unpacklo_epi16(c0, c1));
	_mm_storeu_si128((__m128i *)(out + 8U), _mm_unpackhi_epi16(c0, c1));
}

void scloudplus_sample_bd2_backend(const uint8_t *bytes, size_t byte_count,
								   uint16_t *out)
{
	for (size_t i = 0U; i < byte_count; i += 8U)
	{
		store_bd2_8(bytes + i, out + 4U * i);
	}
}

void scloudplus_sample_bd4_backend(const uint8_t *bytes, size_t byte_count,
								   uint16_t *out)
{
	for (size_t i = 0U; i < byte_count; i += 8U)
	{
		store_bd4_8(bytes + i, out + 2U * i);
	}
}

static inline uint8_t avx2_accept_lt_u32(uint32_t candidate, uint32_t limit)
{
	return (uint8_t)((candidate - limit) >> 31U);
}

static inline uint8_t avx2_is_zero_u32(uint32_t value)
{
	return (uint8_t)((((value | (0U - value)) >> 31U) ^ 1U) & 1U);
}

static inline uint32_t lane_mask_epi16(__m128i cmp)
{
	const uint32_t mask = (uint32_t)_mm_movemask_epi8(cmp);
	uint32_t out = 0U;

	for (unsigned int lane = 0U; lane < 8U; lane++)
	{
		out |= ((mask >> (2U * lane)) & 1U) << lane;
	}
	return out;
}

static size_t collect_bd6_bits_24(const uint8_t *bytes, uint8_t *bits)
{
	const __m128i seven = _mm_set1_epi16(7);
	const __m128i six = _mm_set1_epi16(6);
	const __m128i zero = _mm_setzero_si128();
	const __m128i b0 = _mm_setr_epi16(bytes[0], bytes[3], bytes[6], bytes[9],
									  bytes[12], bytes[15], bytes[18], bytes[21]);
	const __m128i b1 = _mm_setr_epi16(bytes[1], bytes[4], bytes[7], bytes[10],
									  bytes[13], bytes[16], bytes[19], bytes[22]);
	const __m128i b2 = _mm_setr_epi16(bytes[2], bytes[5], bytes[8], bytes[11],
									  bytes[14], bytes[17], bytes[20], bytes[23]);
	__m128i candidate[8];
	uint32_t accept[8];
	uint32_t is_zero[8];
	size_t count = 0U;

	candidate[0] = _mm_and_si128(b0, seven);
	candidate[1] = _mm_and_si128(_mm_srli_epi16(b0, 3), seven);
	candidate[2] = _mm_and_si128(_mm_or_si128(_mm_srli_epi16(b0, 6),
											  _mm_slli_epi16(b1, 2)), seven);
	candidate[3] = _mm_and_si128(_mm_srli_epi16(b1, 1), seven);
	candidate[4] = _mm_and_si128(_mm_srli_epi16(b1, 4), seven);
	candidate[5] = _mm_and_si128(_mm_or_si128(_mm_srli_epi16(b1, 7),
											  _mm_slli_epi16(b2, 1)), seven);
	candidate[6] = _mm_and_si128(_mm_srli_epi16(b2, 2), seven);
	candidate[7] = _mm_and_si128(_mm_srli_epi16(b2, 5), seven);

	for (unsigned int lane = 0U; lane < 8U; lane++)
	{
		accept[lane] = lane_mask_epi16(_mm_cmpgt_epi16(six, candidate[lane]));
		is_zero[lane] = lane_mask_epi16(_mm_cmpeq_epi16(candidate[lane], zero));
	}

	for (unsigned int group = 0U; group < 8U; group++)
	{
		for (unsigned int lane = 0U; lane < 8U; lane++)
		{
			const uint8_t keep = (uint8_t)((accept[lane] >> group) & 1U);

			bits[count] = (uint8_t)((is_zero[lane] >> group) & 1U);
			count += keep;
		}
	}
	return count;
}

static size_t collect_bd6_bits_scalar(const uint8_t *bytes, size_t byte_count,
									  uint8_t *bits)
{
	size_t count = 0U;

	for (size_t i = 0U; i + 3U <= byte_count; i += 3U)
	{
		const uint32_t chunk = (uint32_t)bytes[i + 0U] |
							   ((uint32_t)bytes[i + 1U] << 8U) |
							   ((uint32_t)bytes[i + 2U] << 16U);

		for (unsigned int lane = 0U; lane < 8U; lane++)
		{
			const uint32_t candidate = (chunk >> (3U * lane)) & 0x07U;

			bits[count] = avx2_is_zero_u32(candidate);
			count += avx2_accept_lt_u32(candidate, 6U);
		}
	}
	return count;
}

size_t scloudplus_sample_bd6_bits_backend(const uint8_t *bytes,
										  size_t byte_count, uint8_t *bits)
{
	size_t i = 0U;
	size_t count = 0U;

	for (; (i + 24U) <= byte_count; i += 24U)
	{
		count += collect_bd6_bits_24(bytes + i, bits + count);
	}
	count += collect_bd6_bits_scalar(bytes + i, byte_count - i, bits + count);
	return count;
}

static size_t collect_bd12_bits_32(const uint8_t *bytes, uint8_t *bits)
{
	const __m256i input =
		_mm256_loadu_si256((const __m256i *)(const void *)bytes);
	const __m256i mask = _mm256_set1_epi8(0x0F);
	const __m256i twelve = _mm256_set1_epi8(12);
	const __m256i zero = _mm256_setzero_si256();
	const __m256i lo = _mm256_and_si256(input, mask);
	const __m256i hi = _mm256_and_si256(_mm256_srli_epi16(input, 4), mask);
	const uint32_t accept_lo =
		(uint32_t)_mm256_movemask_epi8(_mm256_cmpgt_epi8(twelve, lo));
	const uint32_t accept_hi =
		(uint32_t)_mm256_movemask_epi8(_mm256_cmpgt_epi8(twelve, hi));
	const uint32_t zero_lo =
		(uint32_t)_mm256_movemask_epi8(_mm256_cmpeq_epi8(lo, zero));
	const uint32_t zero_hi =
		(uint32_t)_mm256_movemask_epi8(_mm256_cmpeq_epi8(hi, zero));
	size_t count = 0U;

	for (unsigned int i = 0U; i < 32U; i++)
	{
		const uint8_t keep_lo = (uint8_t)((accept_lo >> i) & 1U);
		const uint8_t keep_hi = (uint8_t)((accept_hi >> i) & 1U);

		bits[count] = (uint8_t)((zero_lo >> i) & 1U);
		count += keep_lo;
		bits[count] = (uint8_t)((zero_hi >> i) & 1U);
		count += keep_hi;
	}
	return count;
}

static size_t collect_bd12_bits_scalar(const uint8_t *bytes, size_t byte_count,
									   uint8_t *bits)
{
	size_t count = 0U;

	for (size_t i = 0U; i < byte_count; i++)
	{
		const uint32_t lo = bytes[i] & 0x0FU;
		const uint32_t hi = bytes[i] >> 4U;

		bits[count] = avx2_is_zero_u32(lo);
		count += avx2_accept_lt_u32(lo, 12U);
		bits[count] = avx2_is_zero_u32(hi);
		count += avx2_accept_lt_u32(hi, 12U);
	}
	return count;
}

size_t scloudplus_sample_bd12_bits_backend(const uint8_t *bytes,
										   size_t byte_count, uint8_t *bits)
{
	size_t i = 0U;
	size_t count = 0U;

	for (; (i + 32U) <= byte_count; i += 32U)
	{
		count += collect_bd12_bits_32(bytes + i, bits + count);
	}
	count += collect_bd12_bits_scalar(bytes + i, byte_count - i, bits + count);
	return count;
}
