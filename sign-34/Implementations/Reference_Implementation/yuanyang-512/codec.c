/*
 * yuanyang-512 reference byte encodings.
 *
 *   PK = q || encode(h),
 *   SK = expanded fast-signing key
 *        (encode(h) || f || g || F || G || u_hat || A_hat_lower
 *         || Sigma_delta),
 *   Sn = salt || compressed s1.
 *
 * Floating precomputations are encoded as scaled signed integers.  u_hat uses
 * scale 2^11 on int16, A_hat uses scale 2^22 on int32, and Sigma_delta uses
 * scale 2^22 on packed signed 22-bit two's-complement coefficients.  The
 * Cholesky factor A_hat is lower-triangular, so its A01 block is reconstructed
 * as zero instead of serialized. T is derived from Sigma_delta during signing.
 */

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>

#include "yuanyang_inner.h"

#define YUANYANG_SIG_LOW_MASK       ((1u << YUANYANG_SIG_LOW_BITS) - 1u)
#define YUANYANG_SIGMA_DELTA_MASK   ((1u << YUANYANG_SIGMA_DELTA_BITS) - 1u)
#define YUANYANG_RANS_BYTE_L        (1u << 23)
#define YUANYANG_RANS_SCALE_BITS    12u
#define YUANYANG_RANS_SCALE         (1u << YUANYANG_RANS_SCALE_BITS)
#define YUANYANG_RANS_SCALE_MASK    (YUANYANG_RANS_SCALE - 1u)
#if YUANYANG_SIG_LOW_BITS != 5u
#error The embedded yuanyang-512 rANS model is generated for ell=5.
#endif
#if YUANYANG_REJECTION_BOUND != 2337u
#error The embedded yuanyang-512 rANS model assumes rejection bound 2337.
#endif
#define YUANYANG_RANS_SYMBOLS \
	((YUANYANG_REJECTION_BOUND >> YUANYANG_SIG_LOW_BITS) + 1u)
#define YUANYANG_RANS_AUX_BITS \
	((size_t)YUANYANG_D * (size_t)(YUANYANG_SIG_LOW_BITS + 1u))
#define YUANYANG_RANS_AUX_BYTES     ((YUANYANG_RANS_AUX_BITS + 7u) >> 3)

struct yuanyang_rans_model {
	const uint16_t *freq;
	const uint16_t *start;
	unsigned symbols;
};

/*
 * rANS high-part model for yuanyang-512 signatures with ell=5.  Low bits and
 * signs are stored raw; rANS encodes abs(s1[i]) >> ell.  The table has (2337 >>
 * 5) + 1 = 74 symbols so every coefficient allowed by the signature norm bound
 *      is representable; the one-count tail keeps rare high values decodable
 *      instead of creating data-dependent unsupported symbols.
 *
 * Frequencies were quantized to a 4096-count total from deterministic
 * 2000-signature dev distribution-probe samples. The default payload is 535
 * bytes (561 with salt), which gives a 0.862% rejection probability from the
 * compression. More details on the distribution and tradeoffs for different
 * payload sizes are yuanyang_params.h.
 */
static const uint16_t yuanyang_rans_freq[YUANYANG_RANS_SYMBOLS] = {
	1425u, 1181u, 778u, 410u, 172u, 54u, 9u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u
};

static const uint16_t yuanyang_rans_start[YUANYANG_RANS_SYMBOLS] = {
	0u, 1425u, 2606u, 3384u, 3794u, 3966u, 4020u, 4029u,
	4030u, 4031u, 4032u, 4033u, 4034u, 4035u, 4036u, 4037u,
	4038u, 4039u, 4040u, 4041u, 4042u, 4043u, 4044u, 4045u,
	4046u, 4047u, 4048u, 4049u, 4050u, 4051u, 4052u, 4053u,
	4054u, 4055u, 4056u, 4057u, 4058u, 4059u, 4060u, 4061u,
	4062u, 4063u, 4064u, 4065u, 4066u, 4067u, 4068u, 4069u,
	4070u, 4071u, 4072u, 4073u, 4074u, 4075u, 4076u, 4077u,
	4078u, 4079u, 4080u, 4081u, 4082u, 4083u, 4084u, 4085u,
	4086u, 4087u, 4088u, 4089u, 4090u, 4091u, 4092u, 4093u,
	4094u, 4095u
};

static const struct yuanyang_rans_model yuanyang_rans_model = {
	yuanyang_rans_freq,
	yuanyang_rans_start,
	YUANYANG_RANS_SYMBOLS
};

uint16_t
yuanyang_load_u16_le(const unsigned char *src)
{
	return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}

static void
store_u16_le(unsigned char *dst, uint16_t x)
{
	dst[0] = (unsigned char)x;
	dst[1] = (unsigned char)(x >> 8);
}

static int8_t
load_i8(const unsigned char *src)
{
	return (int8_t)src[0];
}

static void
store_i8(unsigned char *dst, int8_t x)
{
	dst[0] = (unsigned char)x;
}

static int16_t
load_i16_le(const unsigned char *src)
{
	uint16_t x;

	x = yuanyang_load_u16_le(src);
	if (x >= 0x8000u) {
		return (int16_t)((int32_t)x - 0x10000);
	}
	return (int16_t)x;
}

static void
store_i16_le(unsigned char *dst, int16_t x)
{
	store_u16_le(dst, (uint16_t)x);
}

static int32_t
load_i32_le(const unsigned char *src)
{
	uint32_t x;

	x = (uint32_t)src[0]
		| ((uint32_t)src[1] << 8)
		| ((uint32_t)src[2] << 16)
		| ((uint32_t)src[3] << 24);
	if (x >= 0x80000000u) {
		return (int32_t)((int64_t)x - 0x100000000ll);
	}
	return (int32_t)x;
}

static void
store_i32_le(unsigned char *dst, int32_t x)
{
	uint32_t ux;

	ux = (uint32_t)x;
	dst[0] = (unsigned char)ux;
	dst[1] = (unsigned char)(ux >> 8);
	dst[2] = (unsigned char)(ux >> 16);
	dst[3] = (unsigned char)(ux >> 24);
}

static int
scale_fpr_to_i32(int32_t *dst, fpr x, unsigned int bit_scale)
{
	int64_t z;

	z = fpr_rint_to_scaled(x, bit_scale);
	if (z < INT32_MIN || z > INT32_MAX) {
		return 0;
	}
	*dst = (int32_t)z;
	return 1;
}

static int
decode_scaled_i16_poly(
	fpr *dst, const unsigned char *src, size_t n, unsigned int bit_scale)
{
	for (size_t u = 0; u < n; u++) {
		dst[u] = fpr_from_scaled_i64(load_i16_le(src + 2u * u), bit_scale);
	}
	return 1;
}

static int
encode_scaled_i16_poly(
	unsigned char *dst, const fpr *src, size_t n, unsigned int bit_scale)
{
	for (size_t u = 0; u < n; u++) {
		int32_t z;

		if (!scale_fpr_to_i32(&z, src[u], bit_scale)
			|| z < INT16_MIN || z > INT16_MAX)
		{
			return 0;
		}
		store_i16_le(dst + 2u * u, (int16_t)z);
	}
	return 1;
}

static uint32_t
read_bits_le(const unsigned char *src, size_t bit_pos, unsigned bits)
{
	size_t byte_pos;
	unsigned shift;
	uint32_t x;

	/*
	 * Private-key fields use at most 22 bits. Three bytes always provide the
	 * first 24 bits; a fourth is needed only when the public bit offset makes
	 * the field cross that boundary. Avoiding an unconditional fourth load
	 * also keeps the final packed field strictly within its input buffer.
	 */
	byte_pos = bit_pos >> 3;
	shift = (unsigned)bit_pos & 7u;
	x = (uint32_t)src[byte_pos]
		| ((uint32_t)src[byte_pos + 1u] << 8)
		| ((uint32_t)src[byte_pos + 2u] << 16);
	if (shift + bits > 24u) {
		x |= (uint32_t)src[byte_pos + 3u] << 24;
	}
	x >>= shift;
	return x & ((UINT32_C(1) << bits) - UINT32_C(1));
}

static void
write_bits_le(
	unsigned char *dst, size_t bit_pos, unsigned bits, uint32_t value)
{
	for (unsigned u = 0; u < bits; u++) {
		size_t p;
		unsigned char mask;

		p = bit_pos + u;
		mask = (unsigned char)(1u << (p & 7u));
		if (((value >> u) & 1u) != 0) {
			dst[p >> 3] |= mask;
		} else {
			dst[p >> 3] = (unsigned char)(dst[p >> 3]
				& (unsigned char)~mask);
		}
	}
}

static int32_t
decode_signed_bits(uint32_t x, unsigned bits)
{
	uint32_t modulus;

	modulus = UINT32_C(1) << bits;
	x &= modulus - 1u;
	if ((x & (modulus >> 1)) != 0) {
		return (int32_t)((int64_t)x - (int64_t)modulus);
	}
	return (int32_t)x;
}

static int
decode_scaled_i32_poly(
	fpr *dst, const unsigned char *src, size_t n, unsigned int bit_scale)
{
	for (size_t u = 0; u < n; u++) {
		dst[u] = fpr_from_scaled_i64(load_i32_le(src + 4u * u), bit_scale);
	}
	return 1;
}

static int
encode_scaled_i32_poly(
	unsigned char *dst, const fpr *src, size_t n, unsigned int bit_scale)
{
	for (size_t u = 0; u < n; u++) {
		int32_t z;

		if (!scale_fpr_to_i32(&z, src[u], bit_scale)) {
			return 0;
		}
		store_i32_le(dst + 4u * u, z);
	}
	return 1;
}

static int
decode_scaled_sigma_delta_mat2(
	fpr dst[YUANYANG_MAT2_SIZE],
	const unsigned char *src, size_t *off)
{
	const unsigned char *base;
	size_t bit_pos;

	base = src + *off;
	bit_pos = 0;
	for (size_t i = 0; i < YUANYANG_MAT2_POLYS; i++) {
		fpr *poly;

		poly = YUANYANG_MAT2_POLY(dst, i);
		for (size_t u = 0; u < YUANYANG_D; u++) {
			uint32_t encoded;
			int32_t z;

			encoded = read_bits_le(base, bit_pos,
				YUANYANG_SIGMA_DELTA_BITS);
			z = decode_signed_bits(encoded, YUANYANG_SIGMA_DELTA_BITS);
			poly[u] = fpr_from_scaled_i64(z, YUANYANG_SIGMA_DELTA_BITS);
			bit_pos += YUANYANG_SIGMA_DELTA_BITS;
		}
	}
	*off += YUANYANG_SIGMA_DELTA_MATRIX2X2_BYTES;
	return 1;
}

static int
encode_scaled_sigma_delta_mat2(
	unsigned char *dst, size_t *off, const fpr src[YUANYANG_MAT2_SIZE])
{
	unsigned char *base;
	size_t bit_pos;

	base = dst + *off;
	memset(base, 0, YUANYANG_SIGMA_DELTA_MATRIX2X2_BYTES);
	bit_pos = 0;
	for (size_t i = 0; i < YUANYANG_MAT2_POLYS; i++) {
		const fpr *poly;

		poly = YUANYANG_MAT2_POLY(src, i);
		for (size_t u = 0; u < YUANYANG_D; u++) {
			int32_t z;

			if (!scale_fpr_to_i32(&z, poly[u], YUANYANG_SIGMA_DELTA_BITS))
			{
				return 0;
			}
			write_bits_le(base, bit_pos, YUANYANG_SIGMA_DELTA_BITS,
				(uint32_t)z & YUANYANG_SIGMA_DELTA_MASK);
			bit_pos += YUANYANG_SIGMA_DELTA_BITS;
		}
	}
	*off += YUANYANG_SIGMA_DELTA_MATRIX2X2_BYTES;
	return 1;
}

static int
decode_scaled_a_hat_lower(
	fpr dst[YUANYANG_MAT2_SIZE],
	const unsigned char *src, size_t *off)
{
	if (!decode_scaled_i32_poly(YUANYANG_MAT2_POLY(dst, YUANYANG_MAT2_00),
			src + *off, YUANYANG_D, YUANYANG_A_HAT_PRECISION_BITS))
	{
		return 0;
	}
	*off += YUANYANG_D * YUANYANG_A_HAT_ENCODED_BYTES;
	for (size_t u = 0; u < YUANYANG_D; u++) {
		YUANYANG_MAT2_POLY(dst, YUANYANG_MAT2_01)[u] = fpr_zero;
	}
	if (!decode_scaled_i32_poly(YUANYANG_MAT2_POLY(dst, YUANYANG_MAT2_10),
			src + *off, YUANYANG_D, YUANYANG_A_HAT_PRECISION_BITS))
	{
		return 0;
	}
	*off += YUANYANG_D * YUANYANG_A_HAT_ENCODED_BYTES;
	if (!decode_scaled_i32_poly(YUANYANG_MAT2_POLY(dst, YUANYANG_MAT2_11),
			src + *off, YUANYANG_D, YUANYANG_A_HAT_PRECISION_BITS))
	{
		return 0;
	}
	*off += YUANYANG_D * YUANYANG_A_HAT_ENCODED_BYTES;
	return 1;
}

static int
encode_scaled_a_hat_lower(
	unsigned char *dst, size_t *off, const fpr src[YUANYANG_MAT2_SIZE])
{
	if (!encode_scaled_i32_poly(dst + *off,
			YUANYANG_MAT2_POLY(src, YUANYANG_MAT2_00),
			YUANYANG_D, YUANYANG_A_HAT_PRECISION_BITS))
	{
		return 0;
	}
	*off += YUANYANG_D * YUANYANG_A_HAT_ENCODED_BYTES;
	if (!encode_scaled_i32_poly(dst + *off,
			YUANYANG_MAT2_POLY(src, YUANYANG_MAT2_10),
			YUANYANG_D, YUANYANG_A_HAT_PRECISION_BITS))
	{
		return 0;
	}
	*off += YUANYANG_D * YUANYANG_A_HAT_ENCODED_BYTES;
	if (!encode_scaled_i32_poly(dst + *off,
			YUANYANG_MAT2_POLY(src, YUANYANG_MAT2_11),
			YUANYANG_D, YUANYANG_A_HAT_PRECISION_BITS))
	{
		return 0;
	}
	*off += YUANYANG_D * YUANYANG_A_HAT_ENCODED_BYTES;
	return 1;
}

/*
 * Compress the public key by encoding each coefficient of h mod b.
 * The output length in bytes is ceil(ceil(n/k)*ceil(log(b^k))/8)
 * The YuanYang public key uses b=q and k=4, matching the public-key packing.
 */
#if defined(__GNUC__) || defined(__clang__)
#define ASSUME(cond) do { if (!(cond)) __builtin_unreachable(); } while (0)
typedef uint64_t v4u64 __attribute__((vector_size(32)));
#else
#define ASSUME(cond) ((void)0)
#endif


void
yuanyang_encode_uniform(
	const uint16_t *tab, unsigned int n, unsigned int b, unsigned int k, uint8_t *output)
{
	ASSUME(k<=4 && n%512==0);
#if defined(__GNUC__) || defined(__clang__)
	if(k==1){
		int nb_bits = ceil(log(b)*k/log(2));
		const unsigned int qn = n >> 2;
		const unsigned int out_qn = (qn * nb_bits) >> 3;

		const uint16_t *t0 = tab;
		const uint16_t *t1 = tab + qn;
		const uint16_t *t2 = tab + 2 * qn;
		const uint16_t *t3 = tab + 3 * qn;

		uint8_t *o0 = output;
		uint8_t *o1 = output + out_qn;
		uint8_t *o2 = output + 2 * out_qn;
		uint8_t *o3 = output + 3 * out_qn;

		v4u64 acc = { 0, 0, 0, 0 };
		unsigned int acc_bits = 0;

		for (unsigned int i = 0; i < qn; i++) {
			v4u64 v = {
				t0[i],
				t1[i],
				t2[i],
				t3[i]
			};

			acc |= v << acc_bits;
			acc_bits += nb_bits;

			while (acc_bits >= 8) {
				*o0++ = (uint8_t)acc[0];
				*o1++ = (uint8_t)acc[1];
				*o2++ = (uint8_t)acc[2];
				*o3++ = (uint8_t)acc[3];

				acc >>= 8;
				acc_bits -= 8;
			}
		}
		return;
	}
	ASSUME(k==4 && n%512==0);
#endif
	uint64_t word = 0;
	unsigned int nb = 0, nb_res_bits = 0;
	uint_fast8_t res_bits = 0;

	for(uint_fast16_t i = 0; i < n; i++) {
		word = word*b + tab[i];
		nb++;
		if (nb == k || i == n-1) {
			*output = res_bits | ((word << nb_res_bits) % 256);
			output++;
			int nb_bits = ceil(log(b)*k/log(2));
			word >>= 8-nb_res_bits;
			nb_bits -= 8-nb_res_bits;
			while(nb_bits >= 8){
				*output = word%256;
				output++;
				word >>= 8;
				nb_bits -= 8;
			}
			res_bits = word;
			nb_res_bits = nb_bits;
			nb = 0;
			word = 0;
		}
	}
}

void
yuanyang_decode_uniform(
	uint16_t *tab, unsigned int n, unsigned int b, unsigned int k, const uint8_t *input)
{
	ASSUME(k<=4);
#if defined(__GNUC__) || defined(__clang__)
	if(k==1){
		const unsigned int qn = n >> 2;
		const unsigned int nb_bits = ceil(log(b) / log(2));
		const unsigned int in_qn = (qn * nb_bits) >> 3;

		const uint8_t *i0 = input;
		const uint8_t *i1 = input + in_qn;
		const uint8_t *i2 = input + 2 * in_qn;
		const uint8_t *i3 = input + 3 * in_qn;

		uint16_t *t0 = tab;
		uint16_t *t1 = tab + qn;
		uint16_t *t2 = tab + 2 * qn;
		uint16_t *t3 = tab + 3 * qn;

		v4u64 acc = { 0, 0, 0, 0 };
		unsigned int acc_bits = 0;

		const v4u64 mask = {
			(1ull << nb_bits) - 1,
			(1ull << nb_bits) - 1,
			(1ull << nb_bits) - 1,
			(1ull << nb_bits) - 1
		};

		for (unsigned int i = 0; i < qn; i++) {
			while (acc_bits < nb_bits) {
				v4u64 v = {
					*i0++,
					*i1++,
					*i2++,
					*i3++
				};

				acc |= v << acc_bits;
				acc_bits += 8;
			}

			v4u64 x = acc & mask;

			t0[i] = (uint16_t)x[0];
			t1[i] = (uint16_t)x[1];
			t2[i] = (uint16_t)x[2];
			t3[i] = (uint16_t)x[3];

			acc >>= nb_bits;
			acc_bits -= nb_bits;
		}
		return;
	}
	ASSUME(k==4);
#endif
	uint64_t word = 0;
	int nb_res_bits = 0;
	uint_fast8_t res_bits = 0, nb_bits = ceil(log(b)*k/log(2));

	for(uint_fast16_t i = 0; i < (n+k-1)/k; i++){
		word = res_bits;
		while(nb_res_bits < nb_bits){
			word |= (0ull+(*input)) << nb_res_bits;
			nb_res_bits += 8;
			input++;
		}
		res_bits = word >> nb_bits;
		nb_res_bits -= nb_bits;
		word = word % (1ull << nb_bits);
		for(int_fast8_t j = k-1; j >= 0; j--){
			if(i*k+j < n)
				tab[i*k+j] = word % b;
			word /= b;
		}
	}

}

int
yuanyang_encode_public_key(
	unsigned char *pk, unsigned long long pk_len_bytes,
	const uint16_t h[YUANYANG_D])
{
	if (pk == 0 || h == 0 || pk_len_bytes != YUANYANG_PK_BYTES) {
		return YUANYANG_BADARG;
	}
	store_u16_le(pk, YUANYANG_Q);
	memset(pk + YUANYANG_PK_PREFIX_BYTES, 0, YUANYANG_H_BYTES);
	yuanyang_encode_uniform(h, YUANYANG_D, YUANYANG_Q,
		YUANYANG_PK_BLOCK_COEFFS, pk + YUANYANG_PK_PREFIX_BYTES);
	return YUANYANG_SUCCESS;
}

int
yuanyang_decode_public_key(
	uint16_t h[YUANYANG_D],
	const unsigned char *pk, unsigned long long pk_len_bytes)
{
	if (pk == 0 || pk_len_bytes != YUANYANG_PK_BYTES) {
		return YUANYANG_INVALID_SIGNATURE;
	}
	if (yuanyang_load_u16_le(pk) != YUANYANG_Q) {
		return YUANYANG_INVALID_SIGNATURE;
	}
	yuanyang_decode_uniform(h, YUANYANG_D, YUANYANG_Q,
		YUANYANG_PK_BLOCK_COEFFS, pk + YUANYANG_PK_PREFIX_BYTES);
	for (size_t i = 0; i < YUANYANG_D; i++) {
		if (h[i] >= YUANYANG_Q) {
			return YUANYANG_INVALID_SIGNATURE;
		}
	}
	return YUANYANG_SUCCESS;
}

int
yuanyang_decode_private_key(
	yuanyang_expanded_sk *decoded,
	const unsigned char *sk, unsigned long long sk_len_bytes)
{
	size_t off;

	if (decoded == 0 || sk == 0 || sk_len_bytes != YUANYANG_SK_BYTES) {
		return YUANYANG_BADARG;
	}
	memset(decoded, 0, sizeof *decoded);
	off = 0;
	yuanyang_decode_uniform(decoded->compact.h, YUANYANG_D, YUANYANG_Q,
		YUANYANG_PK_BLOCK_COEFFS, sk + off);
	off += YUANYANG_H_BYTES;
	for (size_t i = 0; i < YUANYANG_D; i++) {
		if (decoded->compact.h[i] >= YUANYANG_Q) {
			return YUANYANG_BADARG;
		}
	}
	for (size_t i = 0; i < YUANYANG_D; i++, off += 1u) {
		decoded->compact.f[i] = load_i8(sk + off);
	}
	for (size_t i = 0; i < YUANYANG_D; i++, off += 1u) {
		decoded->compact.g[i] = load_i8(sk + off);
	}
	for (size_t i = 0; i < YUANYANG_D; i++, off += 1u) {
		decoded->compact.F[i] = load_i8(sk + off);
	}
	for (size_t i = 0; i < YUANYANG_D; i++, off += 1u) {
		decoded->compact.G[i] = load_i8(sk + off);
	}
	if (!decode_scaled_i16_poly(decoded->u_hat, sk + off, YUANYANG_D,
		YUANYANG_U_HAT_PRECISION_BITS))
	{
		return YUANYANG_BADARG;
	}
	off += YUANYANG_U_HAT_POLY_BYTES;
	if (!decode_scaled_a_hat_lower(decoded->A_hat, sk, &off))
	{
		return YUANYANG_BADARG;
	}
	if (!decode_scaled_sigma_delta_mat2(decoded->sigma_delta, sk, &off))
	{
		return YUANYANG_BADARG;
	}
	if (off != YUANYANG_SK_BYTES) {
		return YUANYANG_BADARG;
	}
	return YUANYANG_SUCCESS;
}

int
yuanyang_encode_private_key(
	unsigned char *sk, unsigned long long *sk_len_bytes,
	const yuanyang_expanded_sk *decoded)
{
	size_t off;

	if (sk_len_bytes != 0) {
		*sk_len_bytes = YUANYANG_SK_BYTES;
	}
	if (sk == 0 || decoded == 0) {
		return YUANYANG_BADARG;
	}
	off = 0;
	for (size_t i = 0; i < YUANYANG_D; i++) {
		if (decoded->compact.h[i] >= YUANYANG_Q) {
			return YUANYANG_BADARG;
		}
	}
	memset(sk + off, 0, YUANYANG_H_BYTES);
	yuanyang_encode_uniform(decoded->compact.h, YUANYANG_D, YUANYANG_Q,
		YUANYANG_PK_BLOCK_COEFFS, sk + off);
	off += YUANYANG_H_BYTES;
	for (size_t i = 0; i < YUANYANG_D; i++, off += 1u) {
		store_i8(sk + off, decoded->compact.f[i]);
	}
	for (size_t i = 0; i < YUANYANG_D; i++, off += 1u) {
		store_i8(sk + off, decoded->compact.g[i]);
	}
	for (size_t i = 0; i < YUANYANG_D; i++, off += 1u) {
		store_i8(sk + off, decoded->compact.F[i]);
	}
	for (size_t i = 0; i < YUANYANG_D; i++, off += 1u) {
		store_i8(sk + off, decoded->compact.G[i]);
	}
	if (!encode_scaled_i16_poly(sk + off, decoded->u_hat, YUANYANG_D,
		YUANYANG_U_HAT_PRECISION_BITS))
	{
		return YUANYANG_BADARG;
	}
	off += YUANYANG_U_HAT_POLY_BYTES;
	if (!encode_scaled_a_hat_lower(sk, &off, decoded->A_hat))
	{
		return YUANYANG_BADARG;
	}
	if (!encode_scaled_sigma_delta_mat2(sk, &off, decoded->sigma_delta))
	{
		return YUANYANG_BADARG;
	}
	if (off != YUANYANG_SK_BYTES) {
		return YUANYANG_BADARG;
	}
	return YUANYANG_SUCCESS;
}

static int
write_sig_bit(unsigned char *buf, size_t bit_len, size_t *bit_pos, unsigned bit)
{
	size_t pos;

	pos = *bit_pos;
	if (pos >= bit_len) {
		return 0;
	}
	if (bit != 0) {
		buf[pos >> 3] |= (unsigned char)(1u << (7u - (pos & 7u)));
	}
	*bit_pos = pos + 1u;
	return 1;
}

static int
read_sig_bit(const unsigned char *buf, size_t bit_len, size_t *bit_pos, unsigned *bit)
{
	size_t pos;

	pos = *bit_pos;
	if (pos >= bit_len) {
		return 0;
	}
	*bit = (unsigned)((buf[pos >> 3] >> (7u - (pos & 7u))) & 1u);
	*bit_pos = pos + 1u;
	return 1;
}

static uint32_t
load_u32_le(const unsigned char *src)
{
	return (uint32_t)src[0]
		| ((uint32_t)src[1] << 8)
		| ((uint32_t)src[2] << 16)
		| ((uint32_t)src[3] << 24);
}

static void
store_u32_le(unsigned char *dst, uint32_t x)
{
	dst[0] = (unsigned char)x;
	dst[1] = (unsigned char)(x >> 8);
	dst[2] = (unsigned char)(x >> 16);
	dst[3] = (unsigned char)(x >> 24);
}

static int
yuanyang_rans_enc_put(
	uint32_t *state, unsigned char **pptr, unsigned char *limit,
	const struct yuanyang_rans_model *model, unsigned sym)
{
	uint32_t freq, start, x_max;

	freq = model->freq[sym];
	start = model->start[sym];
	x_max = ((YUANYANG_RANS_BYTE_L >> YUANYANG_RANS_SCALE_BITS) << 8)
		* freq;
	while (*state >= x_max) {
		if (*pptr <= limit) {
			return 0;
		}
		*(--(*pptr)) = (unsigned char)*state;
		*state >>= 8;
	}
	*state = ((*state / freq) << YUANYANG_RANS_SCALE_BITS)
		+ (*state % freq) + start;
	return 1;
}

static int
yuanyang_rans_find_symbol(
	uint32_t low,
	const struct yuanyang_rans_model *model, unsigned *sym)
{
	unsigned lo, hi;

	lo = 0;
	hi = model->symbols;
	while (lo + 1u < hi) {
		unsigned mid;

		mid = lo + ((hi - lo) >> 1);
		if ((uint32_t)model->start[mid] <= low) {
			lo = mid;
		} else {
			hi = mid;
		}
	}
	if (low >= (uint32_t)model->start[lo] + (uint32_t)model->freq[lo]) {
		return 0;
	}
	*sym = lo;
	return 1;
}

static int
yuanyang_rans_dec_get(
	uint32_t *state, const unsigned char **pptr, const unsigned char *end,
	const struct yuanyang_rans_model *model, unsigned *sym)
{
	uint32_t low, high;

	low = *state & YUANYANG_RANS_SCALE_MASK;
	if (!yuanyang_rans_find_symbol(low, model, sym)) {
		return 0;
	}
	high = *state >> YUANYANG_RANS_SCALE_BITS;
	*state = (uint32_t)model->freq[*sym] * high + low
		- model->start[*sym];
	while (*state < YUANYANG_RANS_BYTE_L) {
		if (*pptr >= end) {
			return 0;
		}
		*state = (*state << 8) | *(*pptr)++;
	}
	return 1;
}

static int
find_last_one_bit(const unsigned char *buf, size_t bit_len, size_t *bit_pos)
{
	size_t bytes;

	bytes = (bit_len + 7u) >> 3;
	while (bytes > 0) {
		unsigned char x;

		x = buf[--bytes];
		if (x != 0) {
			for (unsigned k = 8u; k-- > 0;) {
				if ((x & (unsigned char)(1u << (7u - k))) != 0) {
					*bit_pos = (bytes << 3) + k;
					return 1;
				}
			}
		}
	}
	return 0;
}

static int
yuanyang_encode_signature_s1_rans(
	unsigned char *sn, unsigned long long sn_len_bytes,
	const int16_t s1[YUANYANG_D])
{
	unsigned char aux[YUANYANG_RANS_AUX_BYTES];
	unsigned char rans[YUANYANG_COMP_SIG_BYTES];
	unsigned char *dst, *ptr;
	size_t aux_bit_pos, max_rans_bytes, rans_len;
	uint32_t state;

	if (sn == 0 || s1 == 0 || sn_len_bytes != YUANYANG_SIGNATURE_BYTES) {
		return YUANYANG_BADARG;
	}
	if (YUANYANG_COMP_SIG_BYTES <= YUANYANG_RANS_AUX_BYTES) {
		return YUANYANG_INVALID_SIGNATURE;
	}
	max_rans_bytes = YUANYANG_COMP_SIG_BYTES - YUANYANG_RANS_AUX_BYTES - 1u;
	if (max_rans_bytes < 4u) {
		return YUANYANG_INVALID_SIGNATURE;
	}

	memset(aux, 0, sizeof aux);
	aux_bit_pos = 0;
	for (size_t i = 0; i < YUANYANG_D; i++) {
		unsigned sign, low, abs_x;

		if (s1[i] == INT16_MIN) {
			return YUANYANG_INVALID_SIGNATURE;
		}
		sign = s1[i] < 0;
		abs_x = (unsigned)(sign ? -s1[i] : s1[i]);
		if (abs_x > 0x7FFFu) {
			return YUANYANG_INVALID_SIGNATURE;
		}
		low = abs_x & YUANYANG_SIG_LOW_MASK;
		if (!write_sig_bit(aux, 8u * sizeof aux, &aux_bit_pos, sign)) {
			return YUANYANG_INVALID_SIGNATURE;
		}
		for (unsigned j = 0; j < YUANYANG_SIG_LOW_BITS; j++) {
			unsigned bit;

			bit = (low >> (YUANYANG_SIG_LOW_BITS - 1u - j)) & 1u;
			if (!write_sig_bit(aux, 8u * sizeof aux, &aux_bit_pos, bit)) {
				return YUANYANG_INVALID_SIGNATURE;
			}
		}
	}

	state = YUANYANG_RANS_BYTE_L;
	memset(rans, 0, sizeof rans);
	ptr = rans + max_rans_bytes;
	for (size_t u = YUANYANG_D; u-- > 0;) {
		unsigned sign, abs_x, high;

		sign = s1[u] < 0;
		abs_x = (unsigned)(sign ? -s1[u] : s1[u]);
		high = abs_x >> YUANYANG_SIG_LOW_BITS;
		if (high >= yuanyang_rans_model.symbols) {
			return YUANYANG_INVALID_SIGNATURE;
		}
		if (!yuanyang_rans_enc_put(&state, &ptr, rans + 4u,
			&yuanyang_rans_model, high))
		{
			return YUANYANG_INVALID_SIGNATURE;
		}
	}
	if (ptr < rans + 4u) {
		return YUANYANG_INVALID_SIGNATURE;
	}
	ptr -= 4u;
	store_u32_le(ptr, state);
	rans_len = (size_t)((rans + max_rans_bytes) - ptr);

	dst = sn + YUANYANG_SALT_BYTES;
	memset(dst, 0, YUANYANG_COMP_SIG_BYTES);
	memcpy(dst, ptr, rans_len);
	memcpy(dst + rans_len, aux, sizeof aux);
	dst[rans_len + sizeof aux] = 0x80u;
	return YUANYANG_SUCCESS;
}

static int
yuanyang_decode_signature_s1_rans(
	int16_t s1[YUANYANG_D],
	const unsigned char *sn, unsigned long long sn_len_bytes)
{
	const unsigned char *src, *ptr, *end, *aux;
	size_t marker_pos, rans_bits, rans_len, aux_bit_pos;
	uint32_t state;

	if (s1 == 0 || sn == 0 || sn_len_bytes != YUANYANG_SIGNATURE_BYTES) {
		return YUANYANG_INVALID_SIGNATURE;
	}
	src = sn + YUANYANG_SALT_BYTES;
	if (!find_last_one_bit(src, 8u * YUANYANG_COMP_SIG_BYTES, &marker_pos)) {
		return YUANYANG_INVALID_SIGNATURE;
	}
	if (marker_pos < YUANYANG_RANS_AUX_BITS) {
		return YUANYANG_INVALID_SIGNATURE;
	}
	rans_bits = marker_pos - YUANYANG_RANS_AUX_BITS;
	if ((rans_bits & 7u) != 0) {
		return YUANYANG_INVALID_SIGNATURE;
	}
	rans_len = rans_bits >> 3;
	if (rans_len < 4u || rans_len + YUANYANG_RANS_AUX_BYTES
		>= YUANYANG_COMP_SIG_BYTES)
	{
		return YUANYANG_INVALID_SIGNATURE;
	}

	state = load_u32_le(src);
	if (state < YUANYANG_RANS_BYTE_L
		|| state >= (YUANYANG_RANS_BYTE_L << 8))
	{
		return YUANYANG_INVALID_SIGNATURE;
	}
	ptr = src + 4u;
	end = src + rans_len;
	aux = end;
	aux_bit_pos = 0;
	for (size_t i = 0; i < YUANYANG_D; i++) {
		unsigned high, sign, low, bit;
		uint32_t abs_x;

		if (!yuanyang_rans_dec_get(&state, &ptr, end,
			&yuanyang_rans_model, &high))
		{
			return YUANYANG_INVALID_SIGNATURE;
		}
		if (!read_sig_bit(aux, YUANYANG_RANS_AUX_BITS,
			&aux_bit_pos, &sign))
		{
			return YUANYANG_INVALID_SIGNATURE;
		}
		low = 0;
		for (unsigned j = 0; j < YUANYANG_SIG_LOW_BITS; j++) {
			if (!read_sig_bit(aux, YUANYANG_RANS_AUX_BITS,
				&aux_bit_pos, &bit))
			{
				return YUANYANG_INVALID_SIGNATURE;
			}
			low = (low << 1) | bit;
		}
		abs_x = ((uint32_t)high << YUANYANG_SIG_LOW_BITS) | low;
		if ((abs_x == 0 && sign != 0) || abs_x > 0x7FFFu) {
			return YUANYANG_INVALID_SIGNATURE;
		}
		s1[i] = sign != 0 ? -(int16_t)abs_x : (int16_t)abs_x;
	}
	if (state != YUANYANG_RANS_BYTE_L || ptr != end) {
		return YUANYANG_INVALID_SIGNATURE;
	}
	return YUANYANG_SUCCESS;
}

int
yuanyang_encode_signature_s1(
	unsigned char *sn, unsigned long long sn_len_bytes,
	const int16_t s1[YUANYANG_D])
{
	return yuanyang_encode_signature_s1_rans(sn, sn_len_bytes, s1);
}

int
yuanyang_decode_signature_s1(
	int16_t s1[YUANYANG_D],
	const unsigned char *sn, unsigned long long sn_len_bytes)
{
	return yuanyang_decode_signature_s1_rans(s1, sn, sn_len_bytes);
}

