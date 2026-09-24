/*
 * yuanyang-2048 reference byte encodings.
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

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

#include "yuanyang_inner.h"

#define YUANYANG_SIG_LOW_MASK       ((1u << YUANYANG_SIG_LOW_BITS) - 1u)
#define YUANYANG_SIGMA_DELTA_MASK   ((1u << YUANYANG_SIGMA_DELTA_BITS) - 1u)
#define YUANYANG_RANS_BYTE_L        (1u << 23)
#define YUANYANG_RANS_SCALE_BITS    16u
#define YUANYANG_RANS_SCALE         (1u << YUANYANG_RANS_SCALE_BITS)
#define YUANYANG_RANS_SCALE_MASK    (YUANYANG_RANS_SCALE - 1u)
#if YUANYANG_SIG_LOW_BITS != 4u
#error The yuanyang-2048 rANS model is generated for ell=4.
#endif
#if YUANYANG_REJECTION_BOUND != 7641u
#error The yuanyang-2048 rANS model assumes rejection bound 7641.
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
 * Generated from 2,000 signatures (4,096,000 coefficients) with one-count
 * smoothing over the complete allowed high-part range.  With ell = 4, the
 * table has floor(7641 / 16) + 1 = 478 symbols; its size comes from the
 * rejection bound, not from the rANS normalization total.
 *
 * Frequencies sum to 2^16 = 65536.  This 16-bit normalization reduces the
 * quantization and tail-smoothing overhead seen with the earlier 2^12 model,
 * while remaining within the uint32_t state bounds used by codec.c.
 */
static const uint16_t yuanyang_rans_freq[YUANYANG_RANS_SYMBOLS] = {
	7066u, 7148u, 6878u, 6495u, 6009u, 5448u, 4834u, 4200u,
	3595u, 2990u, 2463u, 1989u, 1565u, 1210u, 919u, 679u,
	496u, 355u, 249u, 173u, 116u, 76u, 51u, 31u,
	21u, 12u, 8u, 5u, 3u, 2u, 2u, 2u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u, 1u, 1u,
	1u, 1u, 1u, 1u, 1u, 1u
};

static const uint16_t yuanyang_rans_start[YUANYANG_RANS_SYMBOLS] = {
	0u, 7066u, 14214u, 21092u, 27587u, 33596u, 39044u, 43878u,
	48078u, 51673u, 54663u, 57126u, 59115u, 60680u, 61890u, 62809u,
	63488u, 63984u, 64339u, 64588u, 64761u, 64877u, 64953u, 65004u,
	65035u, 65056u, 65068u, 65076u, 65081u, 65084u, 65086u, 65088u,
	65090u, 65091u, 65092u, 65093u, 65094u, 65095u, 65096u, 65097u,
	65098u, 65099u, 65100u, 65101u, 65102u, 65103u, 65104u, 65105u,
	65106u, 65107u, 65108u, 65109u, 65110u, 65111u, 65112u, 65113u,
	65114u, 65115u, 65116u, 65117u, 65118u, 65119u, 65120u, 65121u,
	65122u, 65123u, 65124u, 65125u, 65126u, 65127u, 65128u, 65129u,
	65130u, 65131u, 65132u, 65133u, 65134u, 65135u, 65136u, 65137u,
	65138u, 65139u, 65140u, 65141u, 65142u, 65143u, 65144u, 65145u,
	65146u, 65147u, 65148u, 65149u, 65150u, 65151u, 65152u, 65153u,
	65154u, 65155u, 65156u, 65157u, 65158u, 65159u, 65160u, 65161u,
	65162u, 65163u, 65164u, 65165u, 65166u, 65167u, 65168u, 65169u,
	65170u, 65171u, 65172u, 65173u, 65174u, 65175u, 65176u, 65177u,
	65178u, 65179u, 65180u, 65181u, 65182u, 65183u, 65184u, 65185u,
	65186u, 65187u, 65188u, 65189u, 65190u, 65191u, 65192u, 65193u,
	65194u, 65195u, 65196u, 65197u, 65198u, 65199u, 65200u, 65201u,
	65202u, 65203u, 65204u, 65205u, 65206u, 65207u, 65208u, 65209u,
	65210u, 65211u, 65212u, 65213u, 65214u, 65215u, 65216u, 65217u,
	65218u, 65219u, 65220u, 65221u, 65222u, 65223u, 65224u, 65225u,
	65226u, 65227u, 65228u, 65229u, 65230u, 65231u, 65232u, 65233u,
	65234u, 65235u, 65236u, 65237u, 65238u, 65239u, 65240u, 65241u,
	65242u, 65243u, 65244u, 65245u, 65246u, 65247u, 65248u, 65249u,
	65250u, 65251u, 65252u, 65253u, 65254u, 65255u, 65256u, 65257u,
	65258u, 65259u, 65260u, 65261u, 65262u, 65263u, 65264u, 65265u,
	65266u, 65267u, 65268u, 65269u, 65270u, 65271u, 65272u, 65273u,
	65274u, 65275u, 65276u, 65277u, 65278u, 65279u, 65280u, 65281u,
	65282u, 65283u, 65284u, 65285u, 65286u, 65287u, 65288u, 65289u,
	65290u, 65291u, 65292u, 65293u, 65294u, 65295u, 65296u, 65297u,
	65298u, 65299u, 65300u, 65301u, 65302u, 65303u, 65304u, 65305u,
	65306u, 65307u, 65308u, 65309u, 65310u, 65311u, 65312u, 65313u,
	65314u, 65315u, 65316u, 65317u, 65318u, 65319u, 65320u, 65321u,
	65322u, 65323u, 65324u, 65325u, 65326u, 65327u, 65328u, 65329u,
	65330u, 65331u, 65332u, 65333u, 65334u, 65335u, 65336u, 65337u,
	65338u, 65339u, 65340u, 65341u, 65342u, 65343u, 65344u, 65345u,
	65346u, 65347u, 65348u, 65349u, 65350u, 65351u, 65352u, 65353u,
	65354u, 65355u, 65356u, 65357u, 65358u, 65359u, 65360u, 65361u,
	65362u, 65363u, 65364u, 65365u, 65366u, 65367u, 65368u, 65369u,
	65370u, 65371u, 65372u, 65373u, 65374u, 65375u, 65376u, 65377u,
	65378u, 65379u, 65380u, 65381u, 65382u, 65383u, 65384u, 65385u,
	65386u, 65387u, 65388u, 65389u, 65390u, 65391u, 65392u, 65393u,
	65394u, 65395u, 65396u, 65397u, 65398u, 65399u, 65400u, 65401u,
	65402u, 65403u, 65404u, 65405u, 65406u, 65407u, 65408u, 65409u,
	65410u, 65411u, 65412u, 65413u, 65414u, 65415u, 65416u, 65417u,
	65418u, 65419u, 65420u, 65421u, 65422u, 65423u, 65424u, 65425u,
	65426u, 65427u, 65428u, 65429u, 65430u, 65431u, 65432u, 65433u,
	65434u, 65435u, 65436u, 65437u, 65438u, 65439u, 65440u, 65441u,
	65442u, 65443u, 65444u, 65445u, 65446u, 65447u, 65448u, 65449u,
	65450u, 65451u, 65452u, 65453u, 65454u, 65455u, 65456u, 65457u,
	65458u, 65459u, 65460u, 65461u, 65462u, 65463u, 65464u, 65465u,
	65466u, 65467u, 65468u, 65469u, 65470u, 65471u, 65472u, 65473u,
	65474u, 65475u, 65476u, 65477u, 65478u, 65479u, 65480u, 65481u,
	65482u, 65483u, 65484u, 65485u, 65486u, 65487u, 65488u, 65489u,
	65490u, 65491u, 65492u, 65493u, 65494u, 65495u, 65496u, 65497u,
	65498u, 65499u, 65500u, 65501u, 65502u, 65503u, 65504u, 65505u,
	65506u, 65507u, 65508u, 65509u, 65510u, 65511u, 65512u, 65513u,
	65514u, 65515u, 65516u, 65517u, 65518u, 65519u, 65520u, 65521u,
	65522u, 65523u, 65524u, 65525u, 65526u, 65527u, 65528u, 65529u,
	65530u, 65531u, 65532u, 65533u, 65534u, 65535u
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
static unsigned
uniform_chunk_bits(int b, int k)
{
	uint64_t limit;
	unsigned bits;

	limit = 1;
	for (int i = 0; i < k; i++) {
		limit *= (uint64_t)b;
	}
	limit--;
	bits = 0;
	while (limit != 0) {
		bits++;
		limit >>= 1;
	}
	return bits;
}

void
yuanyang_encode_uniform(
	const uint16_t *tab, unsigned int n, unsigned int b, unsigned int k, uint8_t *output)
{
	uint64_t word = 0;
	unsigned int nb = 0, nb_res_bits = 0;
	uint_fast8_t res_bits = 0;

	for(uint_fast16_t i = 0; i < n; i++) {
		word = word*b + tab[i];
		nb++;
		if (nb == k || i == n-1) {
			int nb_bits = (int)uniform_chunk_bits(b, k);
			*output = res_bits | ((word << nb_res_bits) % 256);
			output++;
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
	if (nb_res_bits != 0) {
		*output = res_bits % 256;
	}
}

void
yuanyang_decode_uniform(
	uint16_t *tab, unsigned int n, unsigned int b, unsigned int k, const uint8_t *input)
{
	uint64_t word = 0;
	int nb_res_bits = 0;
	uint_fast8_t res_bits = 0, nb_bits = uniform_chunk_bits(b, k);

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
