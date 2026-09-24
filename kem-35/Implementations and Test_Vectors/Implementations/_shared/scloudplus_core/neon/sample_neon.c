#include <arm_neon.h>
#include <stddef.h>
#include <stdint.h>

static inline uint16x8_t coeff_bit_pair(uint16x8_t x, unsigned int high,
										unsigned int low)
{
	const uint16x8_t ones = vdupq_n_u16(1U);
	const uint16x8_t lhs = vandq_u16(vshlq_u16(x, vdupq_n_s16(-(int)high)),
									 ones);
	const uint16x8_t rhs = vandq_u16(vshlq_u16(x, vdupq_n_s16(-(int)low)),
									 ones);

	return vsubq_u16(lhs, rhs);
}

static inline uint16x8_t coeff_zero_pair(uint16x8_t x, unsigned int high,
										 unsigned int low)
{
	const uint16x8_t ones = vdupq_n_u16(1U);
	const uint16x8_t three = vdupq_n_u16(3U);
	const uint16x8_t zero = vdupq_n_u16(0U);
	const uint16x8_t lhs_bits = vandq_u16(vshlq_u16(x, vdupq_n_s16(-(int)low)),
										  three);
	const uint16x8_t rhs_bits = vandq_u16(vshlq_u16(x, vdupq_n_s16(-(int)high)),
										  three);
	const uint16x8_t lhs = vandq_u16(vceqq_u16(lhs_bits, zero), ones);
	const uint16x8_t rhs = vandq_u16(vceqq_u16(rhs_bits, zero), ones);

	return vsubq_u16(lhs, rhs);
}

static inline void store_bd2_8(const uint8_t *bytes, uint16_t *out)
{
	const uint16x8_t x = vmovl_u8(vld1_u8(bytes));
	uint16x8x4_t coeffs;

	coeffs.val[0] = coeff_bit_pair(x, 1U, 0U);
	coeffs.val[1] = coeff_bit_pair(x, 3U, 2U);
	coeffs.val[2] = coeff_bit_pair(x, 5U, 4U);
	coeffs.val[3] = coeff_bit_pair(x, 7U, 6U);
	vst4q_u16(out, coeffs);
}

static inline void store_bd4_8(const uint8_t *bytes, uint16_t *out)
{
	const uint16x8_t x = vmovl_u8(vld1_u8(bytes));
	uint16x8x2_t coeffs;

	coeffs.val[0] = coeff_zero_pair(x, 2U, 0U);
	coeffs.val[1] = coeff_zero_pair(x, 6U, 4U);
	vst2q_u16(out, coeffs);
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

static inline uint8_t neon_accept_lt_u32(uint32_t candidate, uint32_t limit)
{
	return (uint8_t)((candidate - limit) >> 31U);
}

static inline uint8_t neon_is_zero_u32(uint32_t value)
{
	return (uint8_t)((((value | (0U - value)) >> 31U) ^ 1U) & 1U);
}

static size_t collect_bd6_bits_24(const uint8_t *bytes, uint8_t *bits)
{
	const uint8x8x3_t b = vld3_u8(bytes);
	const uint8x8_t seven = vdup_n_u8(7U);
	const uint8x8_t six = vdup_n_u8(6U);
	const uint8x8_t zero = vdup_n_u8(0U);
	uint8x8_t candidate[8];
	uint8_t accept[8][8];
	uint8_t is_zero[8][8];
	size_t count = 0U;

	candidate[0] = vand_u8(b.val[0], seven);
	candidate[1] = vand_u8(vshr_n_u8(b.val[0], 3), seven);
	candidate[2] = vand_u8(vorr_u8(vshr_n_u8(b.val[0], 6),
									vshl_n_u8(b.val[1], 2)),
						   seven);
	candidate[3] = vand_u8(vshr_n_u8(b.val[1], 1), seven);
	candidate[4] = vand_u8(vshr_n_u8(b.val[1], 4), seven);
	candidate[5] = vand_u8(vorr_u8(vshr_n_u8(b.val[1], 7),
									vshl_n_u8(b.val[2], 1)),
						   seven);
	candidate[6] = vand_u8(vshr_n_u8(b.val[2], 2), seven);
	candidate[7] = vand_u8(vshr_n_u8(b.val[2], 5), seven);

	for (unsigned int lane = 0U; lane < 8U; lane++)
	{
		vst1_u8(accept[lane], vclt_u8(candidate[lane], six));
		vst1_u8(is_zero[lane], vceq_u8(candidate[lane], zero));
	}

	for (unsigned int group = 0U; group < 8U; group++)
	{
		for (unsigned int lane = 0U; lane < 8U; lane++)
		{
			bits[count] = (uint8_t)(is_zero[lane][group] >> 7U);
			count += (uint8_t)(accept[lane][group] >> 7U);
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

			bits[count] = neon_is_zero_u32(candidate);
			count += neon_accept_lt_u32(candidate, 6U);
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

static size_t collect_bd12_bits_16(const uint8_t *bytes, uint8_t *bits)
{
	const uint8x16_t input = vld1q_u8(bytes);
	const uint8x16_t mask = vdupq_n_u8(0x0FU);
	const uint8x16_t twelve = vdupq_n_u8(12U);
	const uint8x16_t zero = vdupq_n_u8(0U);
	const uint8x16_t lo = vandq_u8(input, mask);
	const uint8x16_t hi = vandq_u8(vshrq_n_u8(input, 4), mask);
	uint8_t accept_lo[16];
	uint8_t accept_hi[16];
	uint8_t zero_lo[16];
	uint8_t zero_hi[16];
	size_t count = 0U;

	vst1q_u8(accept_lo, vcltq_u8(lo, twelve));
	vst1q_u8(accept_hi, vcltq_u8(hi, twelve));
	vst1q_u8(zero_lo, vceqq_u8(lo, zero));
	vst1q_u8(zero_hi, vceqq_u8(hi, zero));

	for (unsigned int i = 0U; i < 16U; i++)
	{
		bits[count] = (uint8_t)(zero_lo[i] >> 7U);
		count += (uint8_t)(accept_lo[i] >> 7U);
		bits[count] = (uint8_t)(zero_hi[i] >> 7U);
		count += (uint8_t)(accept_hi[i] >> 7U);
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

		bits[count] = neon_is_zero_u32(lo);
		count += neon_accept_lt_u32(lo, 12U);
		bits[count] = neon_is_zero_u32(hi);
		count += neon_accept_lt_u32(hi, 12U);
	}
	return count;
}

size_t scloudplus_sample_bd12_bits_backend(const uint8_t *bytes,
										   size_t byte_count, uint8_t *bits)
{
	size_t i = 0U;
	size_t count = 0U;

	for (; (i + 16U) <= byte_count; i += 16U)
	{
		count += collect_bd12_bits_16(bytes + i, bits + count);
	}
	count += collect_bd12_bits_scalar(bytes + i, byte_count - i, bits + count);
	return count;
}
