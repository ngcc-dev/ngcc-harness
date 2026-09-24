#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "sampler.h"


#define YUANYANG_RCDT_PREC_BYTES   12u /* 96 bits of precision in RCDT */

#ifndef YUANYANG_SAMPLER_CENTER_SNAP_Q39
#define YUANYANG_SAMPLER_CENTER_SNAP_Q39 1024
#endif

#ifndef YUANYANG_BEREXP_POSITIVE_ARITH
#define YUANYANG_BEREXP_POSITIVE_ARITH 1
#endif


/*
 * RCDT thresholds are stored as high/middle/low 32-bit limbs.  This matches
 * gaussian0_sampler(), which draws a 96-bit integer and compares it limb-wise.
 * The large table is generated for sigma = 4*eta; the small table for eta.
 */
static const uint32_t yuanyang_large_rcdt[] = {
	0xce503fb8U, 0x2bf9fc6bU, 0x88b900fbU,
	0x9e65c9ddU, 0x51e6535eU, 0x46db7bdfU,
	0x736c8f43U, 0x7bf6d15dU, 0x822b6b4bU,
	0x4f94bfc5U, 0x0a8f7aeeU, 0xd5a9d95eU,
	0x33c73983U, 0xb76a6d03U, 0xfce5c221U,
	0x1fb8c3b4U, 0xd77825e8U, 0x5c8a9f2fU,
	0x124423c9U, 0xc9d3367eU, 0x8155778fU,
	0x09def8d6U, 0x0ffc3c9fU, 0xe291c7feU,
	0x04ffeedbU, 0xa2661cd7U, 0x057b4a4fU,
	0x025f0053U, 0x839e514aU, 0x098c7c08U,
	0x010d4aafU, 0x5eccd9e6U, 0xfe956d82U,
	0x006fad4dU, 0xc98a463cU, 0x153db316U,
	0x002b43daU, 0x043be9c4U, 0xa3cf5e51U,
	0x000fa65dU, 0x2de92b55U, 0x0059f8d0U,
	0x00054875U, 0x70288c23U, 0xc20db91fU,
	0x0001a9ebU, 0x320acc2cU, 0x8f6199b4U,
	0x00007d16U, 0x2aad04bdU, 0xccdcfcdfU,
	0x0000223fU, 0xbe857857U, 0x1acf4e95U,
	0x000008bdU, 0x84aed6d2U, 0xfb61f525U,
	0x00000214U, 0x185c3df1U, 0x8050062cU,
	0x00000075U, 0xe36133e2U, 0x8ce91cc5U,
	0x00000018U, 0x545de7c6U, 0x8a9820ceU,
	0x00000004U, 0xad2a532aU, 0xf7fa81d7U,
	0x00000000U, 0xd648da3fU, 0xad547ca0U,
	0x00000000U, 0x23b6a4bfU, 0x33e40e56U,
	0x00000000U, 0x058aa1c6U, 0x7188ac8fU,
	0x00000000U, 0x00cceb4eU, 0x86ca6c0eU,
	0x00000000U, 0x001b8da0U, 0x7d1db494U,
	0x00000000U, 0x000372c4U, 0x09ac00bfU,
	0x00000000U, 0x000066d2U, 0xb972999bU,
	0x00000000U, 0x00000b25U, 0x636b064cU,
	0x00000000U, 0x0000011fU, 0xd6ede1adU,
	0x00000000U, 0x0000001bU, 0x04fc46a2U,
	0x00000000U, 0x00000002U, 0x5c2b98f5U,
	0x00000000U, 0x00000000U, 0x311a1c8eU,
	0x00000000U, 0x00000000U, 0x03b683deU,
	0x00000000U, 0x00000000U, 0x0042df2fU,
	0x00000000U, 0x00000000U, 0x00046081U,
	0x00000000U, 0x00000000U, 0x00004439U,
	0x00000000U, 0x00000000U, 0x000003dcU,
	0x00000000U, 0x00000000U, 0x00000033U,
	0x00000000U, 0x00000000U, 0x00000002U,
};

static const uint32_t yuanyang_small_rcdt[] = {
	0x66117e58U, 0x90ddd5ffU, 0x8bc7817aU,
	0x0fef0056U, 0x50c52244U, 0x8f79959eU,
	0x00d79aadU, 0xd5b94cd4U, 0xb90d9c7fU,
	0x0003a904U, 0x1b3d2d5fU, 0x4e2e9ef6U,
	0x00000505U, 0x661025c7U, 0x080ad9bbU,
	0x00000002U, 0x2983e39cU, 0x7821ff61U,
	0x00000000U, 0x004ab0d5U, 0x1ce53e2cU,
	0x00000000U, 0x00000328U, 0x13bf11bbU,
	0x00000000U, 0x00000000U, 0x0ab19f01U,
	0x00000000U, 0x00000000U, 0x00000b58U,
};

/*
 * Exact BerExp thresholds for the zero-centered large sampler, indexed by the
 * large RCDT output z0 in [0, 42].  Here the exponent is always
 * (2*z0 + 1)/(2*sigma_large^2), so evaluating the Q63 exponential polynomial
 * for every coefficient is redundant. These values are generated with the
 * same fpr_mul()/fpr_expm_p63() operations used by berexp(); the bytewise
 * comparison and random-byte consumption remain unchanged.
 */
static const uint64_t yuanyang_large_zero_berexp[] = {
	UINT64_C(0xf6e0825e7813b817), UINT64_C(0xe5983fed29e5e463),
	UINT64_C(0xd585b43a287a0623), UINT64_C(0xc69330eb1b2149e5),
	UINT64_C(0xb8ac8c3104cb30f1), UINT64_C(0xabbf059532c294b9),
	UINT64_C(0x9fb92cad9be6c017), UINT64_C(0x948ac9969105667d),
	UINT64_C(0x8a24c712027acf95), UINT64_C(0x80791e2ed7f1e3c7),
	UINT64_C(0x777ac35ce7fb0a54), UINT64_C(0x6f1d94d4093b73d3),
	UINT64_C(0x67564a367fd11037), UINT64_C(0x601a6558b3ef7946),
	UINT64_C(0x59602419a9563867), UINT64_C(0x531e73391fcf5674),
	UINT64_C(0x4d4ce2199adca33f), UINT64_C(0x47e3975dccfaaa96),
	UINT64_C(0x42db46520ad27c13), UINT64_C(0x3e2d25137dec1ab8),
	UINT64_C(0x39d2e367ce820538), UINT64_C(0x35c6a238e9d2947e),
	UINT64_C(0x3202eba968348d11), UINT64_C(0x2e82abb6e38cbf8f),
	UINT64_C(0x2b4129604e8e8dc4), UINT64_C(0x283a00470f26313f),
	UINT64_C(0x25691ac244098095), UINT64_C(0x22caac5c376dea47),
	UINT64_C(0x205b2cb2901ce178), UINT64_C(0x1e1752b2575244ee),
	UINT64_C(0x1bfc102965bbb00a), UINT64_C(0x1a068da73cff8510),
	UINT64_C(0x183426a7be95e626), UINT64_C(0x16826602942070da),
	UINT64_C(0x14ef029a7a3f9c37), UINT64_C(0x1377dc47f504a7f8),
	UINT64_C(0x121af8fb454605be), UINT64_C(0x10d68211c0a4f1ba),
	UINT64_C(0x0fa8c1daf3638445), UINT64_C(0x0e90214a33a7bdaf),
	UINT64_C(0x0d8b25d189a77cf8), UINT64_C(0x0c986f63081cfdbe),
	UINT64_C(0x0bb6b695e41ab500)
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
* Signing centers can be exact integers mathematically, but reach this
* point through FFT/iFFT arithmetic.  A residue far below the BerExp input
* grid must not change floor() from k to k-1 across backends.
Technically, this is only mandatory if we have multiple fpr backend.
*/
static fpr
canonicalize_integer_center(fpr mu)
{
	int64_t nearest, diff;
	uint64_t mag, threshold;
	fpr z;

	nearest = fpr_rint(mu);
	z = fpr_of(nearest);
	uint64_t mask;

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
	 */
#if YUANYANG_BEREXP_POSITIVE_ARITH
	/* x is nonnegative; fpr_of(s)*log(2) has no discarded product bits. */
	r.v = yy_mul64_rshift_rne(x.v, fpr_inv_log2.v,
		YUANYANG_FPR_FRAC_BITS);
	s = (int)(r.v >> YUANYANG_FPR_FRAC_BITS);
	r.v = x.v - (uint64_t)(unsigned)s * fpr_log2.v;
#else
	s = (int)fpr_trunc(fpr_mul(x, fpr_inv_log2));
	r.v = x.v- ((s*780414346020669ll)>>(50-YUANYANG_FPR_FRAC_BITS));
#endif
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
			* Rejection sampling. We want a Gaussian centered on r;
			* but we sampled against a Gaussian centered on b (0 or
			* 1). But we know that z is always in the range where
			* our sampling distribution is greater than the Gaussian
			* distribution, so rejection works.
			*
			* We got z with distribution:
			*    G(z) = exp(-((z-b)^2)/(2*sigma0^2))
			* We target distribution:
			*    S(z) = exp(-((z-r)^2)/(2*sigma^2))
			* Rejection sampling works by keeping the value z with
			* probability S(z)/G(z), and starting again otherwise.
			* This requires S(z) <= G(z), which is the case here.
			* Thus, we simply need to keep our z with probability:
			*    P = exp(-x)
			* where:
			*    x = ((z-r)^2)/(2*sigma^2) - ((z-b)^2)/(2*sigma0^2)
			* Here sigma == sigma0 which makes it a bit easier
			*
			* Here, we scale up the Bernouilli distribution, which
			* makes rejection more probable, but makes rejection
			* rate sufficiently decorrelated from the Gaussian
			* center and standard deviation that the whole sampler
			* can be said to be constant-time.
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
			int accepted;

			/*
			 * We replaced the repeated `fpr_expm_p63()` evaluation in the
			 * zero-centered large sampler with a 36-entry table indexed by its
			 * bounded RCDT output `z0`. Each entry is the exact 64-bit
			 * threshold produced by the previous integer fixed-point
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
		yuanyang_large_rcdt, sizeof yuanyang_large_rcdt / sizeof yuanyang_large_rcdt[0],
		rng);
}

int
yuanyang_samplerz_large_zero(
	int *out, prng *rng)
{
	return sampler_zero(out,
		yuanyang_large_rcdt, sizeof yuanyang_large_rcdt / sizeof yuanyang_large_rcdt[0],
		rng);
}

int
yuanyang_samplerz_small(
	int *out, fpr mu, prng *rng)
{
	return sampler(out, mu,
		fpr_yuanyang_inv_2sqreta,
		yuanyang_small_rcdt, sizeof yuanyang_small_rcdt / sizeof yuanyang_small_rcdt[0],
		rng);
}

