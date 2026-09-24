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
	0xd2a38e7aU, 0x3fff4d84U, 0xbc396d43U,
	0xa69ac6bfU, 0xda7ae854U, 0x36d0d36eU,
	0x7e528772U, 0x1ec56d67U, 0x2213dd0cU,
	0x5b98c2b5U, 0x8bb65892U, 0x1c1d3424U,
	0x3f6302c2U, 0x92aec6e1U, 0xd2b0d8fbU,
	0x29ca7c5cU, 0x227d75c3U, 0x89072e42U,
	0x1a362826U, 0x1d416f8cU, 0xfb9c5eecU,
	0x0f9ecb75U, 0x29bb8425U, 0x6f3d1adfU,
	0x08d5cc61U, 0x8b87a754U, 0xdd49939dU,
	0x04bd2950U, 0x59f0dc6eU, 0x9a387ccbU,
	0x026895aeU, 0xd1be6101U, 0xdba53116U,
	0x0128c0a1U, 0x6024e90dU, 0xade813b0U,
	0x00872ba1U, 0xc37e6d43U, 0x6f4c9258U,
	0x003a3e52U, 0xda6cf6d1U, 0x866f2fe3U,
	0x0017bb11U, 0x0106f01bU, 0xbb8c70b9U,
	0x000923b0U, 0x34c570b5U, 0xfecbab40U,
	0x00035379U, 0xf088e441U, 0x2ceba6f8U,
	0x000124bcU, 0x7681175fU, 0x533911f3U,
	0x00005f0dU, 0x8efb8f9eU, 0x345e028dU,
	0x00001d24U, 0xdc0cc354U, 0x3d6d844eU,
	0x0000086fU, 0xa8d3afb2U, 0xc4ebb552U,
	0x0000024eU, 0x18c1ff02U, 0xd4ad724cU,
	0x00000098U, 0x2c65d88cU, 0xfc9fe6f9U,
	0x00000025U, 0x08266d2aU, 0xc7399432U,
	0x00000008U, 0x80d08bfdU, 0x4d2a65b2U,
	0x00000001U, 0xd7937718U, 0x54414f93U,
	0x00000000U, 0x6060068cU, 0x01eb5e1aU,
	0x00000000U, 0x12943cdeU, 0xf2f53f4bU,
	0x00000000U, 0x0360d649U, 0x3b0b5d73U,
	0x00000000U, 0x009450c3U, 0x219c4645U,
	0x00000000U, 0x0017fce7U, 0x83b5fdc1U,
	0x00000000U, 0x0003a8a0U, 0x8204c575U,
	0x00000000U, 0x000086b6U, 0x47836773U,
	0x00000000U, 0x00001245U, 0x0836b6e9U,
	0x00000000U, 0x00000256U, 0x1480da58U,
	0x00000000U, 0x00000048U, 0x1c2a537cU,
	0x00000000U, 0x00000008U, 0x327872e6U,
	0x00000000U, 0x00000000U, 0xe0e5f785U,
	0x00000000U, 0x00000000U, 0x16b92b61U,
	0x00000000U, 0x00000000U, 0x022a1926U,
	0x00000000U, 0x00000000U, 0x0031c15bU,
	0x00000000U, 0x00000000U, 0x00043631U,
	0x00000000U, 0x00000000U, 0x00005607U,
	0x00000000U, 0x00000000U, 0x00000677U,
	0x00000000U, 0x00000000U, 0x00000074U,
	0x00000000U, 0x00000000U, 0x00000007U,
};

static const uint32_t yuanyang_small_rcdt[] = {
	0x70a7a9a8U, 0xf6f7faa1U, 0x081eceedU,
	0x17824d31U, 0x2e7f24d0U, 0x2b10cc6dU,
	0x02113dafU, 0x88faa8f4U, 0xdf3bc744U,
	0x0012a0abU, 0xe42fac28U, 0x798c8a71U,
	0x000041caU, 0x98cfb6d2U, 0x57496e2eU,
	0x0000005aU, 0x5740e77fU, 0x715a149cU,
	0x00000000U, 0x30131198U, 0x90d67aabU,
	0x00000000U, 0x0009e6e9U, 0x13dd947eU,
	0x00000000U, 0x000000c9U, 0xfe9cf451U,
	0x00000000U, 0x00000000U, 0x0639d790U,
	0x00000000U, 0x00000000U, 0x00001300U,
};

/*
 * Exact BerExp thresholds for the zero-centered large sampler, indexed by the
 * large RCDT output z0 in [0, 46].  Here the exponent is always
 * (2*z0 + 1)/(2*sigma_large^2), so evaluating the Q63 exponential polynomial
 * for every coefficient is redundant. These values are generated with the
 * same fpr_mul()/fpr_expm_p63() operations used by berexp(); the bytewise
 * comparison and random-byte consumption remain unchanged.
 */
static const uint64_t yuanyang_large_zero_berexp[] = {
	UINT64_C(0xff868492c56944ef), UINT64_C(0xfe943a8eaf5010ef),
	UINT64_C(0xfda2d647a374b2ad), UINT64_C(0xfcb256e3cb57c2bb),
	UINT64_C(0xfbc2bb8a1f079e2f), UINT64_C(0xfad40362645c8bf3),
	UINT64_C(0xf9e62d952e359bcf), UINT64_C(0xf8f9394bdbb63e7b),
	UINT64_C(0xf80d25b0978495ff), UINT64_C(0xf721f1ee57087dbb),
	UINT64_C(0xf6379d30d9ab4973), UINT64_C(0xf54e26a4a8183a99),
	UINT64_C(0xf4658d77137dab3f), UINT64_C(0xf37dd0d634ceedf1),
	UINT64_C(0xf296eff0ec06e1e7), UINT64_C(0xf1b0e9f6df6b3abf),
	UINT64_C(0xf0cbbe187ad07b37), UINT64_C(0xefe76b86eedea22d),
	UINT64_C(0xef03f17430568935), UINT64_C(0xee214f12f757f42b),
	UINT64_C(0xed3f8396bea8511f), UINT64_C(0xec5e8e33c2fa27cf),
	UINT64_C(0xeb7e6e1f0235383f), UINT64_C(0xea9f228e3abf4793),
	UINT64_C(0xe9c0aab7eac59ab1), UINT64_C(0xe8e305d34f871de5),
	UINT64_C(0xe8063318649f3901), UINT64_C(0xe72a31bfe3514f3d),
	UINT64_C(0xe64f010341d4ea3d), UINT64_C(0xe574a01cb2a28f9d),
	UINT64_C(0xe49b0e4723c14063), UINT64_C(0xe3c24abe3e14a1b3),
	UINT64_C(0xe2ea54be64abce17), UINT64_C(0xe2132b84b410ced9),
	UINT64_C(0xe13cce4f0198bcb3), UINT64_C(0xe0673c5bdab48751),
	UINT64_C(0xdf9274ea844262db), UINT64_C(0xdebe773af9dfdb2b),
	UINT64_C(0xddeb428ded3c8bd1), UINT64_C(0xdd18d624c56d7c73),
	UINT64_C(0xdc4731419e4120d9), UINT64_C(0xdb7653274793fc19),
	UINT64_C(0xdaa63b1944a5e631), UINT64_C(0xd9d6e85bcb6ff38f),
	UINT64_C(0xd9085a33c3fafde5), UINT64_C(0xd83a8fe6c7b6cd97),
	UINT64_C(0xd76d88bb20d1e361)
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
	 */
#if YUANYANG_BEREXP_POSITIVE_ARITH
	/*
	 * x is nonnegative here.  Avoid the generic signed wrappers, and use the
	 * fact that fpr_of(s)*log(2) has no discarded fractional product bits.
	 */
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

