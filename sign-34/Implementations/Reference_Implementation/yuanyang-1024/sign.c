/*
 * yuanyang-1024 signing.
 *
 * This follows the current Python reference online path:
 *   1. decode the secret key and derive B_hat_inv from B and u_hat,
 *   2. hash m || h once, then hash salt || seed to the challenge point,
 *   3. sample a nearby lattice point with the hybrid sampler using
 *      A_hat, derived B_hat_inv, Sigma_delta-derived rejection correction,
 *      and u_hat,
 *   4. emit salt || compress(s1), where s1 is the first signature half.
 *
 * The one-dimensional Gaussian sampler now follows Falcon's
 * gaussian0_sampler/BerExp/sampler structure, adapted to Yuanyang's
 * table-specific sigmas and without the sigma_min/sigma ccs factor.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "drng.h"
#include "yuanyang_inner.h"
#include "prng.h"
#include "sampler.h"
#include "sign_ntt.h"

#define YUANYANG_SIGN_MAX_ATTEMPTS   128u
#define YUANYANG_PRESAMPLER_L_LOG2     2


extern DRNG_ctx drng_algorithm;

typedef enum {
	YUANYANG_SAMPLE_REJECT_NONE = 0,
	YUANYANG_SAMPLE_REJECT_DELTA1,
	YUANYANG_SAMPLE_REJECT_DELTA2
} yuanyang_sample_reject_reason;

static void
record_sample_rejection(
	yuanyang_sign_stats *stats, yuanyang_sample_reject_reason reject_reason)
{
	if (stats == NULL) {
		return;
	}
	switch (reject_reason) {
	case YUANYANG_SAMPLE_REJECT_DELTA1:
		stats->delta1_reject++;
		break;
	case YUANYANG_SAMPLE_REJECT_DELTA2:
		stats->delta2_reject++;
		break;
	case YUANYANG_SAMPLE_REJECT_NONE:
	default:
		break;
	}
}

static int
drng_randombytes_sign(void *ctx, unsigned char *buf, unsigned long long len_bytes)
{
	return get_random_number((DRNG_ctx *)ctx, buf, 8u * len_bytes);
}

static int
check_signature_norm(
	const int16_t s1[YUANYANG_D], const int32_t s2[YUANYANG_D])
{
	uint64_t norm_sq;
	uint64_t rejected;

	norm_sq = 0;
	rejected = 0;
	/*
	 * Signing and verification use the same bound.  Keep scanning all
	 * coefficients after the first excess so this rejection step has one
	 * simple constant-shape loop.
	 */
	for (size_t u = 0; u < YUANYANG_D; u++) {
		int64_t x1, x2;

		x1 = (int64_t)s1[u];
		x2 = (int64_t)s2[u];
		norm_sq += (uint64_t)(x1 * x1);
		norm_sq += (uint64_t)(x2 * x2);
		rejected |= (uint64_t)(norm_sq > YUANYANG_REJECTION_BOUND_SQ);
	}
	return rejected == 0;
}

static fpr
fpr_from_i64_mul(int64_t num, fpr multiplier)
{
	fpr x;

	x.v = (uint64_t)num;
	return fpr_mul(x, multiplier);
}

static fpr_q62
theta_div_q62(fpr center)
{
	fpr_q62 cm1, delta;

	/*
	 * Algorithmically this is:
	 *   (1 + (2*cos - 2)*den0) * (1 + (2*cos - 2)*den1).
	 * We keep the bounded Q2.62 path in range by using:
	 *   (1 + (cos - 1)*2*den0) * (1 + (cos - 1)*2*den1).
	 * The encoded second denominator is exactly zero for this parameter set,
	 * hence its factor is exactly one and need not be evaluated.
	 */
	cm1 = fpr_q62_sub(fpr_cos_2pi_q62(center), fpr_q62_one);
	delta = fpr_q62_add(fpr_q62_one,
		fpr_q62_mul(cm1, fpr_q62_yuanyang_theta_den[0]));
	return fpr_q62_clamp01(delta);
}

static int
ring_samp(
	int32_t out[YUANYANG_D], fpr_q62 *delta, const fpr center[YUANYANG_D],
	prng *rng)
{
	fpr_q62 acc;

	acc = fpr_q62_one;
	for (size_t u = 0; u < YUANYANG_D; u++) {
		int rc;
		int z;
		fpr_q62 theta;

		rc = yuanyang_samplerz_small(&z, center[u], rng);
		if (rc != YUANYANG_SUCCESS) {
			return rc;
		}
		out[u] = z;
		/*
		 * Per-coefficient timing is opt-in because clock reads are comparable
		 * to the cost of theta_div_q62 itself.
		 */
		{
			theta = theta_div_q62(center[u]);
		}
		acc = fpr_q62_mul(acc, theta);
	}
	*delta = acc;
	return YUANYANG_SUCCESS;
}

static int
pre_sampler(
	yuanyang_sign_presample *out,
	const yuanyang_sign_ntt_precomp *ntt_precomp,
	prng *rng)
{
	int32_t x0[YUANYANG_D], x1[YUANYANG_D];
	int32_t p0_scaled[YUANYANG_D], p1_scaled[YUANYANG_D];
	int64_t p0_q22[YUANYANG_D], p1_q22[YUANYANG_D];
	fpr p0_center[YUANYANG_D], p1_center[YUANYANG_D];

	for (size_t u = 0; u < YUANYANG_D; u++) {
		int rc;

		rc = yuanyang_samplerz_large_zero(&x0[u], rng);
		if (rc != YUANYANG_SUCCESS) {
			return rc;
		}
		rc = yuanyang_samplerz_large_zero(&x1[u], rng);
		if (rc != YUANYANG_SUCCESS) {
			return rc;
		}
	}

	/*
	 * A_hat is stored at 2^22 scale.  A_hat*x can exceed one auxiliary NTT
	 * prime, so this product uses the two-prime CRT path.
	 */
	{
		yuanyang_sign_ntt_a_hat_mul(p0_q22, p1_q22, ntt_precomp, x0, x1);
	}
	for (size_t u = 0; u < YUANYANG_D; u++) {
		p0_center[u] = fpr_from_scaled_i64(
			p0_q22[u], YUANYANG_A_HAT_PRECISION_BITS);
		p1_center[u] = fpr_from_scaled_i64(
			p1_q22[u], YUANYANG_A_HAT_PRECISION_BITS);
	}

	for (size_t u = 0; u < YUANYANG_D; u++) {
		int rc;

		rc = yuanyang_samplerz_large(&p0_scaled[u], p0_center[u], rng);
		if (rc != YUANYANG_SUCCESS) {
			return rc;
		}
		rc = yuanyang_samplerz_large(&p1_scaled[u], p1_center[u], rng);
		if (rc != YUANYANG_SUCCESS) {
			return rc;
		}
	}

	for (size_t u = 0; u < YUANYANG_D; u++) {
		out->p0_scaled[u] = p0_scaled[u];
		out->p1_scaled[u] = p1_scaled[u];
	}
	return YUANYANG_SUCCESS;
}

/*
 * Initialize caller-owned presample storage. The buffer object only keeps
 * indices into the provided slot array; it never allocates or owns memory.
 */
void
yuanyang_sign_presample_buffer_init(
	yuanyang_sign_presample_buffer *buf,
	yuanyang_sign_presample *slots,
	size_t capacity)
{
	if (buf == NULL) {
		return;
	}
	buf->slots = slots;
	buf->capacity = capacity;
	buf->head = 0;
	buf->count = 0;
}

/*
 * Offline phase for buffered signing.
 *
 * Fill the ring buffer up to target_count presamples using only the expanded
 * signing key precomputation and the supplied PRNG. No message, salt, or
 * challenge is consumed here, so batch code can run this before the online
 * signing phase. Existing buffered entries are preserved, and target_count is
 * capped to the caller-provided capacity.
 */
int
yuanyang_sign_presample_buffer_fill_ntt(
	yuanyang_sign_presample_buffer *buf,
	const yuanyang_sign_ntt_precomp *ntt_precomp,
	prng *rng,
	size_t target_count)
{
	if (buf == NULL || buf->slots == NULL || buf->capacity == 0
		|| ntt_precomp == NULL || rng == NULL)
	{
		return YUANYANG_BADARG;
	}
	if (target_count > buf->capacity) {
		target_count = buf->capacity;
	}
	while (buf->count < target_count) {
		size_t tail;
		int rc;

		tail = buf->head + buf->count;
		if (tail >= buf->capacity) {
			tail -= buf->capacity;
		}
		{
			rc = pre_sampler(&buf->slots[tail], ntt_precomp, rng);
		}
		if (rc != YUANYANG_SUCCESS) {
			return rc;
		}
		buf->count++;
	}
	return YUANYANG_SUCCESS;
}

/*
 * Online presample consumer.
 *
 * Take one presample from the buffer for the current sampling attempt. If no
 * usable buffer was supplied, or if the buffer is empty, this falls back to
 * computing exactly one presample with the signing PRNG; this preserves the
 * standalone signing behavior while allowing batch callers to prefill deeper
 * buffers.
 */
static int
presample_buffer_get(
	yuanyang_sign_presample *out,
	yuanyang_sign_presample_buffer *buf,
	const yuanyang_sign_ntt_precomp *ntt_precomp,
	prng *rng)
{
	int rc;

	if (out == NULL) {
		return YUANYANG_BADARG;
	}
	if (buf == NULL || buf->slots == NULL || buf->capacity == 0) {
		return pre_sampler(out, ntt_precomp, rng);
	}
	if (buf->count == 0) {
		/*
		 * Online signing needs exactly one presample for this attempt.  Callers
		 * that want a deeper offline queue refill it explicitly before signing.
		 */
		rc = yuanyang_sign_presample_buffer_fill_ntt(buf, ntt_precomp,
			rng, 1);
		if (rc != YUANYANG_SUCCESS) {
			return rc;
		}
	}
	*out = buf->slots[buf->head];
	buf->head++;
	if (buf->head == buf->capacity) {
		buf->head = 0;
	}
	buf->count--;
	return YUANYANG_SUCCESS;
}

static fpr_q62
delta2_probability_ntt(
	const yuanyang_sign_ntt_precomp *ntt_precomp,
	const int32_t err0[YUANYANG_D],
	const int32_t err1[YUANYANG_D])
{
	fpr weight;
	fpr exponent;
	fpr_q62 probability;

	weight = yuanyang_sign_ntt_delta2_weight(ntt_precomp, err0, err1);
	exponent.v = fpr_mul(weight, fpr_yuanyang_inv_2sqrsigma_sig).v >> YUANYANG_EXTRABITS;
	probability = fpr_q62_mul(
		fpr_expm_p62(exponent), fpr_q62_yuanyang_weak_inv_C);

	return probability;
}

static int
accept_delta_probability(
	fpr_q62 probability,
	prng *rng,
	yuanyang_sample_reject_reason reason,
	yuanyang_sample_reject_reason *reject_reason)
{
	/*
	 * This is the single Bernoulli draw used by each Delta rejection step.
	 * Keeping it as a named helper makes the call sites read like Algorithm 6
	 * while preserving the exact RNG consumption order.
	 */
	if (yuanyang_accept_probability_q62(probability, rng)) {
		return 1;
	}
	if (reject_reason != NULL) {
		*reject_reason = reason;
	}
	return 0;
}

static void
recover_sampler_centers(
	fpr hat0[YUANYANG_D], fpr hat1[YUANYANG_D],
	const yuanyang_sign_presample *presample,
	const yuanyang_sign_ntt_precomp *ntt_precomp,
	const uint16_t challenge[YUANYANG_D])
{
	/*
	 * Recover the two one-dimensional sampler centers from
	 * B_hat^{-1}(c - p).  B_hat^{-1} already includes q^{-1} in coefficient
	 * form; only the fixed-point and presampler-L scales remain in the
	 * denominator after the CRT product.
	 */
	int32_t centered0_L[YUANYANG_D], centered1_L[YUANYANG_D];
	int64_t hat0_num_q11_L[YUANYANG_D];
	int64_t hat1_num_q11_L[YUANYANG_D];
	/* Inverse of q, scaled by L*2^11. */
	const fpr q_scaled_inv = { 0x1d402838374c0000 };

	for (size_t u = 0; u < YUANYANG_D; u++) {
		centered0_L[u] = -presample->p0_scaled[u];
		centered1_L[u] = ((int32_t)challenge[u]
			<< YUANYANG_PRESAMPLER_L_LOG2) - presample->p1_scaled[u];
	}
	{
		yuanyang_sign_ntt_b_hat_inv_mul_crt(
			hat0_num_q11_L, hat1_num_q11_L, ntt_precomp,
			centered0_L, centered1_L);
	}
	for (size_t u = 0; u < YUANYANG_D; u++) {
		hat0[u] = fpr_from_i64_mul(hat0_num_q11_L[u], q_scaled_inv);
		hat1[u] = fpr_from_i64_mul(hat1_num_q11_L[u], q_scaled_inv);
	}
}

static int
sample_z2_and_prepare_z1_center(
	int32_t z2[YUANYANG_D],
	yuanyang_sign_ntt_q1_poly *z2_ntt,
	fpr c1_prime[YUANYANG_D],
	fpr_q62 *delta_prime,
	const fpr hat0[YUANYANG_D], const fpr hat1[YUANYANG_D],
	const yuanyang_sign_ntt_precomp *ntt_precomp,
	prng *rng)
{
	int64_t z2_u_hat_q11[YUANYANG_D];
	int rc;

	{
		rc = ring_samp(z2, delta_prime, hat1, rng);
	}
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}

	/*
	 * Algorithm 6 samples z2 first.  The second center is
	 * c1' = hat0 + u_hat*z2, and z2 is kept in Q1 NTT form for the later
	 * B*(z1,z2) materialization.
	 */
	{
		yuanyang_sign_ntt_u_hat_mul_prepare(z2_u_hat_q11,
			z2_ntt, ntt_precomp, z2);
	}
	for (size_t u = 0; u < YUANYANG_D; u++) {
		c1_prime[u] = fpr_add(hat0[u],
			fpr_from_scaled_i64(z2_u_hat_q11[u],
				YUANYANG_U_HAT_PRECISION_BITS));
	}
	return YUANYANG_SUCCESS;
}

static void
materialize_lattice_point_and_delta2_error(
	int32_t lattice0[YUANYANG_D],
	int32_t lattice1[YUANYANG_D],
	int32_t delta2_err1[YUANYANG_D],
	const yuanyang_sign_ntt_precomp *ntt_precomp,
	const int32_t z1[YUANYANG_D],
	const yuanyang_sign_ntt_q1_poly *z2_ntt,
	const uint16_t challenge[YUANYANG_D])
{
	/*
	 * After Delta1 accepts, materialize v = B(z1,z2).  Delta2 compares the
	 * sampled lattice point with c, so only the second coordinate needs the
	 * explicit c subtraction here: err = (v0, v1 - c).
	 */
	{
		yuanyang_sign_ntt_basis_mul_with_z2_ntt(
			lattice0, lattice1, ntt_precomp, z1, z2_ntt);
	}
	for (size_t u = 0; u < YUANYANG_D; u++) {
		delta2_err1[u] = lattice1[u] - (int32_t)challenge[u];
	}
}

static fpr_q62
delta2_probability_for_rejection(
	const yuanyang_sign_ntt_precomp *ntt_precomp,
	const int32_t lattice0[YUANYANG_D],
	const int32_t delta2_err1[YUANYANG_D])
{
	fpr_q62 delta2;

	{
		delta2 = delta2_probability_ntt(ntt_precomp, lattice0, delta2_err1);
	}
	return delta2;
}

static int
sample_lattice_point(
	int *accepted,
	yuanyang_sample_reject_reason *reject_reason,
	int32_t v0[YUANYANG_D], int32_t v1[YUANYANG_D],
	const yuanyang_sign_ntt_precomp *ntt_precomp,
	yuanyang_sign_presample_buffer *presamples,
	const uint16_t challenge[YUANYANG_D],
	prng *rng)
{
	yuanyang_sign_presample presample;
	fpr hat0[YUANYANG_D], hat1[YUANYANG_D];
	fpr c1_prime[YUANYANG_D];
	int32_t z1[YUANYANG_D], z2[YUANYANG_D];
	int32_t lattice0[YUANYANG_D], lattice1[YUANYANG_D];
	int32_t delta2_err1[YUANYANG_D];
	yuanyang_sign_ntt_q1_poly z2_ntt;
	fpr_q62 delta_prime, delta_double_prime, delta1;
	fpr_q62 delta2;
	int rc;

	*accepted = 0;
	if (reject_reason != NULL) {
		*reject_reason = YUANYANG_SAMPLE_REJECT_NONE;
	}

	rc = presample_buffer_get(&presample, presamples, ntt_precomp, rng);
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}

	recover_sampler_centers(hat0, hat1, &presample, ntt_precomp,
		challenge);
	rc = sample_z2_and_prepare_z1_center(z2, &z2_ntt, c1_prime,
		&delta_prime, hat0, hat1, ntt_precomp, rng);
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}

	{
		rc = ring_samp(z1, &delta_double_prime, c1_prime, rng);
	}
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}

	/* Algorithm 6: Delta1 rejection is tested before materializing B(z1,z2). */
	delta1 = fpr_q62_mul(delta_prime, delta_double_prime);
	if (!accept_delta_probability(delta1, rng,
			YUANYANG_SAMPLE_REJECT_DELTA1, reject_reason))
	{
		return YUANYANG_SUCCESS;
	}
	materialize_lattice_point_and_delta2_error(
		lattice0, lattice1, delta2_err1, ntt_precomp,
		z1, &z2_ntt, challenge);
	delta2 = delta2_probability_for_rejection(ntt_precomp, lattice0,
		delta2_err1);
	if (!accept_delta_probability(delta2, rng,
			YUANYANG_SAMPLE_REJECT_DELTA2, reject_reason))
	{
		return YUANYANG_SUCCESS;
	}
	memcpy(v0, lattice0, sizeof(int32_t) * YUANYANG_D);
	memcpy(v1, lattice1, sizeof(int32_t) * YUANYANG_D);
	*accepted = 1;
	return YUANYANG_SUCCESS;
}

static void
derive_signature_halves(
	int16_t s1[YUANYANG_D], int32_t s2[YUANYANG_D],
	const uint16_t challenge[YUANYANG_D],
	const int32_t v0[YUANYANG_D], const int32_t v1[YUANYANG_D])
{
	/*
	 * Specification signing step s = c - v.  Only s1 is serialized; s2 is
	 * kept here solely for the rejection-bound check and is recomputed by
	 * verification from c, h, and s1.
	 */
	for (size_t u = 0; u < YUANYANG_D; u++) {
		s1[u] = (int16_t)(-v0[u]);
		s2[u] = (int32_t)challenge[u] - v1[u];
	}
}

/*
 * Shared signing body for both standalone and batch-precomputed paths.
 *
 * The message-dependent work stays here.  The optional presample buffer splits
 * sampling into an offline key-only producer and this online consumer; passing
 * NULL selects a local one-slot buffer and matches the normal signing path.
 */
static int
sign_core_with_precomp(
	const yuanyang_expanded_sk *expanded,
	const yuanyang_sign_ntt_precomp *ntt_precomp,
	yuanyang_sign_presample_buffer *presamples,
	const unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes,
	yuanyang_sign_stats *stats)
{
	unsigned char seed[32];
	uint16_t challenge[YUANYANG_D];
	yuanyang_sign_presample fallback_slot;
	yuanyang_sign_presample_buffer fallback_presamples;
	prng rng;
	int rc;

	if (sn_len_bytes != NULL) {
		*sn_len_bytes = YUANYANG_SIGNATURE_BYTES;
	}
	if (presamples == NULL) {
		yuanyang_sign_presample_buffer_init(&fallback_presamples,
			&fallback_slot, 1);
		presamples = &fallback_presamples;
	}

	/* Algorithm 6: bind the message to the public key before salting. */
	{
		rc = yuanyang_hash_message_with_public_key(seed, m, m_len_bytes,
			expanded->compact.h);
	}
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}

	prng_init(&rng, drng_randombytes_sign, &drng_algorithm);
	if (prng_status(&rng) != YUANYANG_SUCCESS) {
		return prng_status(&rng);
	}

	for (unsigned attempt = 0; attempt < YUANYANG_SIGN_MAX_ATTEMPTS; attempt++) {
		int32_t v0[YUANYANG_D], v1[YUANYANG_D];
		int32_t s2[YUANYANG_D];
		int16_t s1[YUANYANG_D];
		int accepted;
		yuanyang_sample_reject_reason reject_reason;

		if (stats != NULL) {
			stats->attempts++;
		}

		rc = prng_get_bytes(&rng, sn, YUANYANG_SALT_BYTES);
		if (rc != YUANYANG_SUCCESS) {
			return rc;
		}
		/*
		 * Algorithm 6: compute c from the fresh salt and the message/public-key
		 * seed.  The XOF output is parsed by rejection modulo q.
		 */
		{
			rc = yuanyang_hash_to_challenge(challenge, sn, seed);
		}
		if (rc != YUANYANG_SUCCESS) {
			return rc;
		}
		/*
		 * Algorithm 6 hybrid sampling: sample a lattice point v around c.
		 * Delta1/Delta2 rejection is internal to sample_lattice_point(); a
		 * rejected attempt loops back with a new salt and challenge.
		 */
		{
			rc = sample_lattice_point(&accepted, &reject_reason, v0, v1,
				ntt_precomp, presamples, challenge, &rng);
		}
		if (rc != YUANYANG_SUCCESS) {
			return rc;
		}
		if (!accepted) {
			record_sample_rejection(stats, reject_reason);
			continue;
		}

		{
			derive_signature_halves(s1, s2, challenge, v0, v1);
		}

		/* Final rejection: keep signatures inside the specified norm ball. */
		{
			accepted = check_signature_norm(s1, s2);
		}
		if (!accepted) {
			if (stats != NULL) {
				stats->squared_norm_reject++;
			}
			continue;
		}

		{
			rc = yuanyang_encode_signature_s1(sn, YUANYANG_SIGNATURE_BYTES, s1);
		}
		if (rc != YUANYANG_SUCCESS) {
			if (stats != NULL) {
				stats->compression_reject++;
			}
			continue;
		}

		if (stats != NULL) {
			stats->success++;
		}
		*sn_len_bytes = YUANYANG_SIGNATURE_BYTES;
		return YUANYANG_SUCCESS;
	}

	if (stats != NULL) {
		stats->max_attempts_exhausted++;
	}
	return YUANYANG_SIGN_FAILED;
}

/*
 * Batch/internal signing entry point for callers that have already decoded the
 * secret key, computed the NTT key precomputation, and optionally filled a
 * presample buffer. Submission-facing APIs continue to go through
 * yuanyang_sign_core_with_stats().
 */
int
yuanyang_sign_core_precomputed_ntt_buffered(
	const yuanyang_expanded_sk *expanded,
	const yuanyang_sign_ntt_precomp *ntt_precomp,
	yuanyang_sign_presample_buffer *presamples,
	const unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes,
	yuanyang_sign_stats *stats)
{
	if (sn_len_bytes != NULL) {
		*sn_len_bytes = YUANYANG_SIGNATURE_BYTES;
	}
	if (expanded == NULL || ntt_precomp == NULL || m == NULL
		|| sn == NULL || sn_len_bytes == NULL)
	{
		return YUANYANG_BADARG;
	}
	if (stats != NULL) {
		memset(stats, 0, sizeof *stats);
	}
	return sign_core_with_precomp(expanded, ntt_precomp, presamples,
		m, m_len_bytes, sn, sn_len_bytes, stats);
}

int
yuanyang_sign_core_precomputed_ntt(
	const yuanyang_expanded_sk *expanded,
	const yuanyang_sign_ntt_precomp *ntt_precomp,
	const unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes,
	yuanyang_sign_stats *stats)
{
	yuanyang_sign_presample one_slot;
	yuanyang_sign_presample_buffer presamples;

	yuanyang_sign_presample_buffer_init(&presamples, &one_slot, 1);
	return yuanyang_sign_core_precomputed_ntt_buffered(
		expanded, ntt_precomp, &presamples, m, m_len_bytes,
		sn, sn_len_bytes, stats);
}

int
yuanyang_sign_core_with_stats(
	const unsigned char *sk, unsigned long long sk_len_bytes,
	const unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes,
	yuanyang_sign_stats *stats)
{
	yuanyang_expanded_sk expanded;
	yuanyang_sign_ntt_precomp ntt_precomp;
	int rc;

	if (sn_len_bytes != NULL) {
		*sn_len_bytes = YUANYANG_SIGNATURE_BYTES;
	}
	if (sk == NULL || m == NULL || sn == NULL || sn_len_bytes == NULL) {
		return YUANYANG_BADARG;
	}
	if (stats != NULL) {
		memset(stats, 0, sizeof *stats);
	}

	rc = yuanyang_decode_private_key(&expanded, sk, sk_len_bytes);
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}

	/*
	 * Precomputation of key-related values. It stays outside the rejection
	 * loop and inside the standalone signing call, so the submission-facing
	 * API keeps the same cost model and behavior.
	 */
	{
		yuanyang_sign_ntt_precompute(&ntt_precomp, &expanded);
	}

	return yuanyang_sign_core_precomputed_ntt(
		&expanded, &ntt_precomp, m, m_len_bytes,
		sn, sn_len_bytes, stats);
}

int
yuanyang_sign_core(
	const unsigned char *sk, unsigned long long sk_len_bytes,
	const unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes)
{
	return yuanyang_sign_core_with_stats(sk, sk_len_bytes, m, m_len_bytes,
		sn, sn_len_bytes, NULL);
}
