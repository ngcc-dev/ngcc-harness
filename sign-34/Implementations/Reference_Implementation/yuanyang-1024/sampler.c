#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "sampler.h"


#define YUANYANG_RCDT_PREC_BYTES   12u /* 96 bits of precision in RCDT */

#ifndef YUANYANG_SAMPLER_CENTER_SNAP_Q39
#define YUANYANG_SAMPLER_CENTER_SNAP_Q39 1024
#endif


/*
 * RCDT thresholds are stored as high/middle/low 32-bit limbs.  This matches
 * gaussian0_sampler(), which draws a 96-bit integer and compares it limb-wise.
 * The large table is generated for sigma = 4*eta; the small table for eta.
 */
static const uint32_t yuanyang_large_rcdt[] = {
	0xd0712d47U, 0xf90f9accU, 0xceb47b06U,
	0xa26cddeaU, 0xb9ad4a51U, 0x8f901a05U,
	0x78bd0970U, 0xeb4ff9c2U, 0xdc79b2b6U,
	0x5561c60fU, 0x0456698bU, 0xc908e448U,
	0x394e7179U, 0xcd991446U, 0xf1a555c0U,
	0x246f163bU, 0x92839984U, 0x094cf2e6U,
	0x15e7e4a5U, 0xb66957e8U, 0x78828ea9U,
	0x0c703bdeU, 0x784ef6beU, 0xf960fb48U,
	0x06a98696U, 0xfe202db0U, 0x52a7fb78U,
	0x035cdddbU, 0xfd8155f3U, 0x021161ddU,
	0x019925bbU, 0x7b55504aU, 0x83d0d0d5U,
	0x00b6f908U, 0x8f47428cU, 0x2940b1e5U,
	0x004cf2b3U, 0x5de26e7cU, 0xcf131bf3U,
	0x001e6a51U, 0x4627bd52U, 0xdfd2098fU,
	0x000b4b7aU, 0xefdd4745U, 0x48f5a3faU,
	0x0003f069U, 0x8ead420fU, 0xcf69bd48U,
	0x00014a2cU, 0xbff1c0e1U, 0x9a99f37aU,
	0x00006576U, 0xe7ed9a1cU, 0x6f17a08bU,
	0x00001d41U, 0xf9c3fef0U, 0x3ef31a40U,
	0x000007eaU, 0x22988610U, 0x13a1c366U,
	0x00000202U, 0x15a3b62cU, 0x1164c435U,
	0x0000007aU, 0x5276837bU, 0x008fdec4U,
	0x0000001bU, 0x4a6a06a5U, 0xe29b6913U,
	0x00000005U, 0xb5510f33U, 0x74121352U,
	0x00000001U, 0x1e8679c5U, 0x87fda57bU,
	0x00000000U, 0x34a86d56U, 0x360c1689U,
	0x00000000U, 0x0911e7f2U, 0xd4f21b5bU,
	0x00000000U, 0x0176ca4dU, 0x6a42ebceU,
	0x00000000U, 0x0038b071U, 0xb9d23d08U,
	0x00000000U, 0x000808cbU, 0x580d5640U,
	0x00000000U, 0x0001111eU, 0x5d982a58U,
	0x00000000U, 0x000021faU, 0x5801ba17U,
	0x00000000U, 0x000003f5U, 0xc7aecc73U,
	0x00000000U, 0x0000006eU, 0xaf8e32d2U,
	0x00000000U, 0x0000000bU, 0x520557ddU,
	0x00000000U, 0x00000001U, 0x15a2c5b9U,
	0x00000000U, 0x00000000U, 0x18e9e63dU,
	0x00000000U, 0x00000000U, 0x02180f53U,
	0x00000000U, 0x00000000U, 0x002a331aU,
	0x00000000U, 0x00000000U, 0x00031c80U,
	0x00000000U, 0x00000000U, 0x000036feU,
	0x00000000U, 0x00000000U, 0x0000038dU,
	0x00000000U, 0x00000000U, 0x00000036U,
	0x00000000U, 0x00000000U, 0x00000002U,
};

static const uint32_t yuanyang_small_rcdt[] = {
	0x6b39cb5aU, 0xd8e3d812U, 0x4fb78bbaU,
	0x1365d789U, 0x45cb8e8dU, 0x85050006U,
	0x015408f3U, 0xd8860fa9U, 0xa5424454U,
	0x00085c3fU, 0x4b880324U, 0x566ef4ccU,
	0x00001288U, 0x7b7f1e4dU, 0xc286efe9U,
	0x0000000eU, 0x5ef5c2feU, 0x05ec8494U,
	0x00000000U, 0x03e3797dU, 0xc621d2bbU,
	0x00000000U, 0x00005deaU, 0x861f9779U,
	0x00000000U, 0x00000003U, 0x169ebab2U,
	0x00000000U, 0x00000000U, 0x00090fafU,
	0x00000000U, 0x00000000U, 0x00000009U,
};

/*
 * Exact BerExp thresholds for the zero-centered large sampler, indexed by the
 * large RCDT output z0 in [0, 44].  Here the exponent is always
 * (2*z0 + 1)/(2*sigma_large^2), so evaluating the Q63 exponential polynomial
 * for every coefficient is redundant. These values are generated with the
 * same fpr_mul()/fpr_expm_p63() operations used by berexp(); the bytewise
 * comparison and random-byte consumption remain unchanged.
 */
static const uint64_t yuanyang_large_zero_berexp[] = {
	UINT64_C(0xf7b45ee7ed167571), UINT64_C(0xe7e9531c00854071),
	UINT64_C(0xd9200e1b7ed438f3), UINT64_C(0xcb4820757330f56f),
	UINT64_C(0xbe5226faf839d129), UINT64_C(0xb22fb9a4b2a43177),
	UINT64_C(0xa6d35b8f7651422b), UINT64_C(0x9c306bfe4942bc57),
	UINT64_C(0x923b18511a6523e1), UINT64_C(0x88e84ee092232d7f),
	UINT64_C(0x802db2b0615eb66d), UINT64_C(0x78018fea61ce136e),
	UINT64_C(0x705ad115ba24fb38), UINT64_C(0x6930f4fe08d47a2c),
	UINT64_C(0x627c053f5cd87c94), UINT64_C(0x5c348d6c7a31e095),
	UINT64_C(0x565392c593cd3cd2), UINT64_C(0x50d28c7644f297ee),
	UINT64_C(0x4bab5c522ab7042e), UINT64_C(0x46d848080aa83089),
	UINT64_C(0x4253f2c3f7b047dc), UINT64_C(0x3e195739618d2c63),
	UINT64_C(0x3a23c20e6ecf6936), UINT64_C(0x366ecca26d2c4de3),
	UINT64_C(0x32f6582989b2cf97), UINT64_C(0x2fb6891860b5db79),
	UINT64_C(0x2cabc2da4e50b82e), UINT64_C(0x29d2a3cdbb653aea),
	UINT64_C(0x27280181f0c24311), UINT64_C(0x24a8e53242f5cb80),
	UINT64_C(0x2252887aad7e05ed), UINT64_C(0x2022524233e49734),
	UINT64_C(0x1e15d3d79b125e9c), UINT64_C(0x1c2ac63d452be6e7),
	UINT64_C(0x1a5f07a12e856202), UINT64_C(0x18b098fe3b8a71ba),
	UINT64_C(0x171d9be435210793), UINT64_C(0x15a45063fc358351),
	UINT64_C(0x1443131da551ef41), UINT64_C(0x12f85b6e52d441b5),
	UINT64_C(0x11c2b9bbc7a076fc), UINT64_C(0x10a0d5dbcd347071),
	UINT64_C(0x0f916d95a6f0bc28), UINT64_C(0x0e93533be9646e0e),
	UINT64_C(0x0da56c5d27732280)
};

static int
get_u64(
	uint64_t *out, prng *rng)
{

	if (out == NULL || rng == NULL) {
		return YUANYANG_BADARG;
	}
	*out = prng_get_u64(rng);
	if (rng->status != YUANYANG_SUCCESS) {
		return rng->status;
	}
	return YUANYANG_SUCCESS;
}

static int
get_u8(
	unsigned *out, prng *rng)
{

	if (out == NULL || rng == NULL) {
		return YUANYANG_BADARG;
	}
	*out = prng_get_u8(rng);
	if (rng->status != YUANYANG_SUCCESS) {
		return rng->status;
	}
	return YUANYANG_SUCCESS;
}

static int
random_uniform_q62(
	uint64_t *out, prng *rng);

/*
 * Signing centers can be exact integers mathematically, but reach this point
 * through fixed-point transforms. A tiny residue far below the BerExp input
 * grid must not change floor() from k to k-1 across backends.
 */
static fpr
canonicalize_integer_center(fpr mu)
{
	int64_t nearest, diff;
	uint64_t mag, threshold;
	uint64_t mask;
	fpr z;

	nearest = fpr_rint(mu);
	z = fpr_of(nearest);

	diff = yy_fpr_i64(mu.v - z.v);
	diff >>= (YUANYANG_FPR_FRAC_BITS - 39u);
	mag = yy_fpr_abs_i64(diff);
	threshold = (uint64_t)YUANYANG_SAMPLER_CENTER_SNAP_Q39;
	mask = (uint64_t)0 - (uint64_t)(mag <= threshold);
	mu.v = (z.v & mask) | (mu.v & ~mask);
	return mu;
}

static int
gaussian0_sampler(
	int *out, const uint32_t *table, size_t table_len,
	prng *rng)
{
	uint64_t lo;
	uint32_t v0, v1, v2;
	unsigned b0, b1, b2, b3;
	size_t u;
	int z;
	int rc;


	/*
	 * Get a random 96-bit value, into low/middle/high 32-bit limbs v0..v2.
	 */
	rc = get_u64(&lo, rng);
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}
	rc = get_u8(&b0, rng);
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}
	rc = get_u8(&b1, rng);
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}
	rc = get_u8(&b2, rng);
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}
	rc = get_u8(&b3, rng);
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}
	v0 = (uint32_t)lo;
	v1 = (uint32_t)(lo >> 32);
	v2 = (uint32_t)b0
		| ((uint32_t)b1 << 8)
		| ((uint32_t)b2 << 16)
		| ((uint32_t)b3 << 24);
	z = 0;
	for (u = 0; u < table_len; u += 3) {
		uint32_t w0, w1, w2, cc;

		w0 = table[u + 2];
		w1 = table[u + 1];
		w2 = table[u + 0];
		cc = (uint32_t)(((uint64_t)v0 - (uint64_t)w0) >> 63);
		cc = (uint32_t)(((uint64_t)v1 - (uint64_t)w1 - (uint64_t)cc) >> 63);
		cc = (uint32_t)(((uint64_t)v2 - (uint64_t)w2 - (uint64_t)cc) >> 63);
		z += (int)cc;
	}
	*out = z;
	return YUANYANG_SUCCESS;
}

static int
berexp_compare(
	int *accepted, uint64_t z,
	prng *rng)
{
	int rc;
	uint32_t w;
	int i;
	uint8_t byte;

	i = 64;
	do {
		unsigned b;

		i -= 8;
		rc = get_u8(&b, rng);
		if (rc != YUANYANG_SUCCESS) {
			return rc;
		}
		byte = (uint8_t)b;
		w = (uint32_t)byte - ((uint32_t)(z >> i) & 0xFF);
	} while (!w && i > 0);
	*accepted = (int)(w >> 31);
	return YUANYANG_SUCCESS;
}

static int
berexp(
	int *accepted, fpr x,
	prng *rng)
{
	int s;
	int rc;
	fpr r;
	uint32_t sw;
	uint64_t z;

	if (accepted == NULL) {
		return YUANYANG_BADARG;
	}
	if (fpr_lt(x, fpr_zero)) {
		x = fpr_zero;
	}


	/*
	 * Falcon's BerExp() supports an extra scaling factor ccs to absorb the
	 * sigma_min / sigma ratio when sampling against a wider base Gaussian.
	 * Yuanyang uses table-specific base samplers with no such rescaling, so the
	 * probability is just exp(-x).
	 *
	 * Split x = s*log(2) + r so that exp(-x) can be evaluated as
	 * exp(-r) / 2^s, with r in the range where the fixed-point exponential
	 * approximation is intended to operate.
	 */
	s = (int)fpr_trunc(fpr_mul(x, fpr_inv_log2));
	r.v = x.v- ((s*780414346020669ll)>>(50-YUANYANG_FPR_FRAC_BITS));
	sw = (uint32_t)s;
	sw ^= (sw ^ 63u) & -((63u - sw) >> 31);
	s = (int)sw;

	z = ((fpr_expm_p63(r) << 1) - 1u) >> s;
	rc = berexp_compare(accepted, z, rng);
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}

	return YUANYANG_SUCCESS;
}

static int
sampler(
	int *out, fpr mu, fpr inv_2sigma_sq,
	const uint32_t *table, size_t table_len,
	prng *rng)
{
	int s;
	fpr r;

	if (out == NULL) {
		return YUANYANG_BADARG;
	}

	/*
	 * This matches Falcon's sampler() structure:
	 *   mu = s + r, 0 <= r < 1
	 * then draw from a half-Gaussian base sampler followed by BerExp.
	 *
	 * In Yuanyang, each exported sampler already uses the matching RCDT for its
	 * target sigma, so there is no sigma_min/sigma factor and no ccs term.
	 */
	mu = canonicalize_integer_center(mu);
	s = (int)fpr_floor(mu);
	r = fpr_sub(mu, fpr_of(s));


	for (;;) {
		int z0, z;
		unsigned entropy, bit;
		fpr x;
		int accepted;
		int rc;

		/* If we need to restart, we can draw the next sign bit from this value */
		rc = get_u8(&entropy, rng);
		if (rc != YUANYANG_SUCCESS) {
			return rc;
		}
		for (int i = 0; i < 8; i++) {
			rc = gaussian0_sampler(&z0, table, table_len, rng);
			if (rc != YUANYANG_SUCCESS) {
				return rc;
			}
			bit = (entropy >> i) & 1;
			z = bit + ((bit << 1) - 1) * z0;

			/*
			 * Falcon-style discrete-Gaussian correction.  The base RCDT draws
			 * z around b in {0, 1}; the target fractional center is r.  Since
			 * this sampler uses the RCDT that already matches its target sigma,
			 * the acceptance probability is simply exp(-x), where:
			 *
			 *   x = ((z-r)^2 - (z-b)^2) / (2*sigma^2)
			 *
			 * The candidate formula below encodes b through the sampled sign
			 * bit: b = bit and z = bit + (2*bit - 1)*z0.
			 */
			x = fpr_sqr(fpr_sub(fpr_of(z), r));
			x = fpr_sub(x, fpr_sqr(fpr_of((int64_t)z0)));
			x = fpr_mul(x, inv_2sigma_sq);
			rc = berexp(&accepted, x, rng);
			if (rc != YUANYANG_SUCCESS) {
				return rc;
			}
			if (accepted) {
				*out = s + z;
				return YUANYANG_SUCCESS;
			}
		}
	}
}

static int
sampler_zero(
	int *out,
	const uint32_t *table, size_t table_len,
	prng *rng)
{

	if (out == NULL) {
		return YUANYANG_BADARG;
	}


	for (;;) {
		unsigned entropy;
		int rc;

		rc = get_u8(&entropy, rng);
		if (rc != YUANYANG_SUCCESS) {
			return rc;
		}
		for (int i = 0; i < 8; i++) {
			int z0, z;
			int accepted;
			unsigned bit;

			rc = gaussian0_sampler(&z0, table, table_len, rng);
			if (rc != YUANYANG_SUCCESS) {
				return rc;
			}
			bit = (entropy >> i) & 1u;
			if (bit == 0u) {
				*out = -z0;
				return YUANYANG_SUCCESS;
			}
			z = z0 + 1;
			/*
			 * With mu = 0 and b = 1:
			 *   x = ((z0 + 1)^2 - z0^2) / (2*sigma^2)
			 *     = (2*z0 + 1) / (2*sigma^2).
			 * For b = 0, x is zero and the sample is accepted directly.
			 */
			/*
			 * We replaced the repeated `fpr_expm_p63()` evaluation in the
			 * zero-centered large sampler with a parameter-specific table
			 * indexed by its bounded RCDT output `z0`.  Each entry is the
			 * exact 64-bit threshold produced by the previous integer fixed-point
			 * `fpr_mul`/`BerExp` calculation for
			 * `(2*z0+1)/(2*sigma_large^2)`.
			 */
			rc = berexp_compare(&accepted,
				yuanyang_large_zero_berexp[z0], rng);
			if (rc != YUANYANG_SUCCESS) {
				return rc;
			}
			if (accepted) {
				*out = z;
				return YUANYANG_SUCCESS;
			}
		}
	}
}

static int
random_uniform_q62(
	uint64_t *out, prng *rng)
{
	uint64_t sample;
	int rc;

	rc = get_u64(&sample, rng);
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}
	*out = sample >> 2;
	return YUANYANG_SUCCESS;
}

int
yuanyang_accept_probability_q62(fpr_q62 probability, prng *rng)
{
	uint64_t u, threshold;

	threshold = fpr_q62_to_u62(probability);
	u = 0;
	if (random_uniform_q62(&u, rng) != YUANYANG_SUCCESS) {
		return 0;
	}
	return u < threshold;
}

int
yuanyang_samplerz_large(
	int *out, fpr mu, prng *rng)
{
	return sampler(out, mu,
		fpr_yuanyang_inv_2sqr_large_sampler_sigma,
		yuanyang_large_rcdt,
		sizeof yuanyang_large_rcdt / sizeof yuanyang_large_rcdt[0],
		rng);
}

int
yuanyang_samplerz_large_zero(
	int *out, prng *rng)
{
	return sampler_zero(out,
		yuanyang_large_rcdt,
		sizeof yuanyang_large_rcdt / sizeof yuanyang_large_rcdt[0],
		rng);
}

int
yuanyang_samplerz_small(
	int *out, fpr mu, prng *rng)
{
	return sampler(out, mu,
		fpr_yuanyang_inv_2sqreta,
		yuanyang_small_rcdt,
		sizeof yuanyang_small_rcdt / sizeof yuanyang_small_rcdt[0],
		rng);
}

