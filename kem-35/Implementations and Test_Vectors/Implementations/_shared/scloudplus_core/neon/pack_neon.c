/**
 * @file pack_neon.c
 * @brief NEON byte serialization for Scloud+ keys and ciphertext components.
 *
 * The public key matrix B and ciphertext matrices C1/C2 are elements of
 * Z/qZ with q = 2^10.  They are serialized in row-major order using the
 * algorithm-level 10-bit little-endian packing: every four coefficients
 * occupy five bytes.  This file is therefore part of the byte-level
 * interoperability contract for pk and ct.
 *
 * The secret matrix S is different: its coefficients are sampled from
 * {-1,0,1}.  The implementation stores S with ShortPack, a lossless two-bit
 * two's-complement encoding: 0 -> 00, 1 -> 01, and -1 -> 11.  This keeps the
 * KEM private key short while preserving the exact mathematical secret matrix.
 *
 * The PACK_LINEAR_Q10_FIXED and UNPACK_LINEAR_Q10_FIXED macros deliberately
 * receive compile-time counts at their call sites.  Earlier generic helpers
 * used a run-time count parameter and compiled poorly on some platforms; the
 * fixed-count form gives the compiler enough information to specialize the
 * pk, C1, and C2 loops without changing the serialized format.
 */

#include "pack.h"
#include "scloudplus_param_common.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(__aarch64__)
#include <arm_neon.h>
#define SCLOUDPLUS_PACK_HAS_NEON_UNPACK10 1
#endif

/**
 * @brief Pack four q-coefficients into five little-endian bytes.
 *
 * Coefficient a occupies bits [0,9], b occupies [10,19], c occupies [20,29],
 * and d occupies [30,39].  Only the low 10 bits of each input are serialized.
 */
static void pack4_q10(uint16_t a, uint16_t b, uint16_t c, uint16_t d,
					  uint8_t out[5])
{
	const uint64_t w = ((uint64_t)(a & 0x03ffU) << 0U) |
					   ((uint64_t)(b & 0x03ffU) << 10U) |
					   ((uint64_t)(c & 0x03ffU) << 20U) |
					   ((uint64_t)(d & 0x03ffU) << 30U);

	out[0] = (uint8_t)(w >> 0U);
	out[1] = (uint8_t)(w >> 8U);
	out[2] = (uint8_t)(w >> 16U);
	out[3] = (uint8_t)(w >> 24U);
	out[4] = (uint8_t)(w >> 32U);
}

/**
 * @brief Inverse of pack4_q10 for one five-byte group.
 */
static void unpack4_q10(const uint8_t in[5], uint16_t *a, uint16_t *b,
						uint16_t *c, uint16_t *d)
{
	const uint64_t w = ((uint64_t)in[0] << 0U) |
					   ((uint64_t)in[1] << 8U) |
					   ((uint64_t)in[2] << 16U) |
					   ((uint64_t)in[3] << 24U) |
					   ((uint64_t)in[4] << 32U);

	*a = (uint16_t)((w >> 0U) & 0x03ffU);
	*b = (uint16_t)((w >> 10U) & 0x03ffU);
	*c = (uint16_t)((w >> 20U) & 0x03ffU);
	*d = (uint16_t)((w >> 30U) & 0x03ffU);
}

#if defined(SCLOUDPLUS_PACK_HAS_NEON_UNPACK10)
/**
 * @brief Decode eight 10-bit coefficients from ten little-endian bytes.
 *
 * The helper uses a 16-byte table lookup window to decode ten useful bytes.
 * Callers therefore use it only when at least sixteen input bytes remain and
 * keep the short tail on the scalar path.
 */
static inline uint16x8_t unpack10_x8_neon(const uint8_t *in)
{
	static const uint8_t gather_data[16] = {
		0, 1, 1, 2, 2, 3, 3, 4, 5, 6, 6, 7, 7, 8, 8, 9};
	static const int16_t shift_data[8] = {
		0, -2, -4, -6, 0, -2, -4, -6};
	const uint8x16_t bytes = vld1q_u8(in);
	const uint8x16_t gather = vld1q_u8(gather_data);
	const uint16x8_t windows = vreinterpretq_u16_u8(vqtbl1q_u8(bytes, gather));
	const uint16x8_t mask10 = vdupq_n_u16(0x03FF);

	/*
	 * The gathered 16-bit windows contain the target coefficient at bit
	 * positions 0, 2, 4, or 6.  Signed per-lane shifts remove the offset in
	 * one vector operation, avoiding four mask/or lanes of the older decoder.
	 */
	return vandq_u16(vshlq_u16(windows, vld1q_s16(shift_data)), mask10);
}

static inline void unpack_linear_q10_neon(const uint8_t *in, size_t count,
										  uint16_t *out)
{
	size_t i = 0U;

	for (; i + 16U <= count; i += 8U)
	{
		vst1q_u16(out + i, unpack10_x8_neon(in));
		in += 10U;
	}
	for (; i < count; i += 4U)
	{
		unpack4_q10(in, &out[i + 0U], &out[i + 1U],
					&out[i + 2U], &out[i + 3U]);
		in += 5U;
	}
}
#endif

#if defined(SCLOUDPLUS_PACK_HAS_NEON_UNPACK10)
#define IF_NEON_UNPACK10(IN, COUNT, OUT) unpack_linear_q10_neon((IN), (COUNT), (OUT));
#define IF_NOT_NEON_UNPACK10(CODE)
#else
#define IF_NEON_UNPACK10(IN, COUNT, OUT)
#define IF_NOT_NEON_UNPACK10(CODE) CODE
#endif

/**
 * @brief Fixed-count row-major Pack10 loop.
 *
 * COUNT is always a public parameter expression such as m*nbar or mbar*n.
 * All supported dimensions are multiples of four, so the loop has no tail
 * case and the byte length is exactly 5*COUNT/4.
 */
#define PACK_LINEAR_Q10_FIXED(IN, COUNT, OUT)                           \
	do                                                                  \
	{                                                                   \
		const uint16_t *pack_in_ = (IN);                                \
		uint8_t *pack_out_ = (OUT);                                     \
		for (size_t pack_i_ = 0U; pack_i_ < (size_t)(COUNT);            \
			 pack_i_ += 4U)                                             \
		{                                                               \
			pack4_q10(pack_in_[pack_i_ + 0U], pack_in_[pack_i_ + 1U],   \
					  pack_in_[pack_i_ + 2U], pack_in_[pack_i_ + 3U],   \
					  pack_out_);                                       \
			pack_out_ += 5U;                                            \
		}                                                               \
	} while (0)

/**
 * @brief Fixed-count row-major Unpack10 loop.
 */
#define UNPACK_LINEAR_Q10_FIXED(IN, COUNT, OUT)                         \
	do                                                                  \
	{                                                                   \
		const uint8_t *unpack_in_ = (IN);                               \
		uint16_t *unpack_out_ = (OUT);                                  \
		IF_NEON_UNPACK10(unpack_in_, (size_t)(COUNT), unpack_out_)      \
		IF_NOT_NEON_UNPACK10(                                           \
		for (size_t unpack_i_ = 0U; unpack_i_ < (size_t)(COUNT);        \
			 unpack_i_ += 4U)                                           \
		{                                                               \
			unpack4_q10(unpack_in_, &unpack_out_[unpack_i_ + 0U],       \
						&unpack_out_[unpack_i_ + 1U],                  \
						&unpack_out_[unpack_i_ + 2U],                  \
						&unpack_out_[unpack_i_ + 3U]);                 \
			unpack_in_ += 5U;                                           \
		})                                                              \
	} while (0)

/**
 * @brief Reduce a public matrix buffer to q by keeping the low 10 bits.
 */
static void mod_q_vec(const uint16_t *in, size_t count, uint16_t *out)
{
	for (size_t i = 0U; i < count; i++)
	{
		out[i] = (uint16_t)(in[i] & scloudplus_q_mask);
	}
}

/**
 * @brief ShortPack for the secret matrix S.
 *
 * S is stored row-major.  Each coefficient is interpreted as a signed ternary
 * integer and serialized by its two low bits.  Thus -1 is encoded as 3
 * because the in-memory uint16_t representation of -1 is 0xffff.  Four
 * coefficients occupy one byte.  This encoding is private-key serialization
 * only; public q-elements still use Pack10.
 */
void pack_sk(const uint16_t *S, uint8_t *sk)
{
	size_t i = 0U;
	const size_t count = (size_t)scloudplus_n * scloudplus_nbar;

#if defined(SCLOUDPLUS_PACK_HAS_NEON_UNPACK10)
	const uint16x8_t mask2 = vdupq_n_u16(0x0003U);

	for (; i + 32U <= count; i += 32U)
	{
		const uint16x8x4_t s = vld4q_u16(S + i);
		const uint16x8_t s0 = vandq_u16(s.val[0], mask2);
		const uint16x8_t s1 = vandq_u16(s.val[1], mask2);
		const uint16x8_t s2 = vandq_u16(s.val[2], mask2);
		const uint16x8_t s3 = vandq_u16(s.val[3], mask2);
		const uint16x8_t packed =
			vorrq_u16(vorrq_u16(s0, vshlq_n_u16(s1, 2)),
					  vorrq_u16(vshlq_n_u16(s2, 4), vshlq_n_u16(s3, 6)));

		vst1_u8(sk + (i >> 2U), vmovn_u16(packed));
	}
#endif
	for (; i < count; i += 4U)
	{
		const uint8_t s0 = (uint8_t)(S[i + 0U] & 0x03U);
		const uint8_t s1 = (uint8_t)(S[i + 1U] & 0x03U);
		const uint8_t s2 = (uint8_t)(S[i + 2U] & 0x03U);
		const uint8_t s3 = (uint8_t)(S[i + 3U] & 0x03U);

		sk[i >> 2U] = (uint8_t)(s0 | (uint8_t)(s1 << 2U) |
						  (uint8_t)(s2 << 4U) | (uint8_t)(s3 << 6U));
	}
}

static inline uint16_t unpack_sk_coeff(uint8_t value)
{
	const uint16_t x = (uint16_t)(value & 0x03U);
	const uint16_t sign = (uint16_t)(x >> 1U);
	const uint16_t sign_mask = (uint16_t)(0U - sign);

	return (uint16_t)(x | (sign_mask & 0xfffcU));
}

/**
 * @brief Inverse of pack_sk.
 */
void unpack_sk(const uint8_t *sk, uint16_t *S)
{
	size_t i = 0U;
	const size_t count = (size_t)scloudplus_n * scloudplus_nbar;

#if defined(SCLOUDPLUS_PACK_HAS_NEON_UNPACK10)
	const uint16x8_t mask2 = vdupq_n_u16(0x0003U);
	const uint16x8_t sign_bit = vdupq_n_u16(0x0001U);
	const uint16x8_t sign_ext = vdupq_n_u16(0xfffcU);

	for (; i + 32U <= count; i += 32U)
	{
		const uint16x8_t packed = vmovl_u8(vld1_u8(sk + (i >> 2U)));
		uint16x8x4_t out;

		out.val[0] = vandq_u16(packed, mask2);
		out.val[1] = vandq_u16(vshrq_n_u16(packed, 2), mask2);
		out.val[2] = vandq_u16(vshrq_n_u16(packed, 4), mask2);
		out.val[3] = vandq_u16(vshrq_n_u16(packed, 6), mask2);
		for (size_t lane = 0U; lane < 4U; lane++)
		{
			const uint16x8_t sign =
				vceqq_u16(vandq_u16(vshrq_n_u16(out.val[lane], 1), sign_bit),
						  sign_bit);
			out.val[lane] = vorrq_u16(out.val[lane], vandq_u16(sign, sign_ext));
		}
		vst4q_u16(S + i, out);
	}
#endif
	for (; i < count; i += 4U)
	{
		const uint8_t packed = sk[i >> 2U];

		S[i + 0U] = unpack_sk_coeff(packed);
		S[i + 1U] = unpack_sk_coeff((uint8_t)(packed >> 2U));
		S[i + 2U] = unpack_sk_coeff((uint8_t)(packed >> 4U));
		S[i + 3U] = unpack_sk_coeff((uint8_t)(packed >> 6U));
	}
}

/**
 * @brief Serialize the public matrix B in pk after seedA.
 */
void pack_pk(const uint16_t *B, uint8_t *pk)
{
	PACK_LINEAR_Q10_FIXED(B, (size_t)scloudplus_m * scloudplus_nbar, pk);
}

/**
 * @brief Parse the public matrix B from pk.
 */
void unpack_pk(const uint8_t *pk, uint16_t *B)
{
	UNPACK_LINEAR_Q10_FIXED(pk, (size_t)scloudplus_m * scloudplus_nbar, B);
}

/**
 * @brief Canonicalize C1 coefficients before ciphertext packing.
 */
void reduce_c1(const uint16_t *C, uint16_t *out)
{
	mod_q_vec(C, (size_t)scloudplus_mbar * scloudplus_n, out);
}

/**
 * @brief Canonicalize C2 coefficients before ciphertext packing.
 */
void reduce_c2(const uint16_t *C, uint16_t *out)
{
	mod_q_vec(C, (size_t)scloudplus_mbar * scloudplus_nbar, out);
}


/**
 * @brief Serialize C1 in row-major Pack10 order.
 */
void pack_c1(const uint16_t *C, uint8_t *out)
{
	PACK_LINEAR_Q10_FIXED(C, (size_t)scloudplus_mbar * scloudplus_n, out);
}

/**
 * @brief Parse C1 from row-major Pack10 order.
 */
void unpack_c1(const uint8_t *in, uint16_t *C)
{
	UNPACK_LINEAR_Q10_FIXED(in, (size_t)scloudplus_mbar * scloudplus_n, C);
}

/**
 * @brief Serialize C2 in row-major Pack10 order immediately after C1.
 */
void pack_c2(const uint16_t *C, uint8_t *out)
{
	PACK_LINEAR_Q10_FIXED(C, (size_t)scloudplus_mbar * scloudplus_nbar, out);
}

/**
 * @brief Parse C2 from row-major Pack10 order.
 */
void unpack_c2(const uint8_t *in, uint16_t *C)
{
	UNPACK_LINEAR_Q10_FIXED(in, (size_t)scloudplus_mbar * scloudplus_nbar, C);
}
