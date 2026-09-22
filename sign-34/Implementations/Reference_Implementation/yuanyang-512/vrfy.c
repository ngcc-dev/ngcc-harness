/*
 * yuanyang-512 verification.
 *
 * This mirrors
 * API_PKC/Implementations/Additional_Implementation/python/sign.py:
 *   1. decode PK q || pack(h) and signature salt || compress(s1),
 *   2. compute seed = SM3(m || h),
 *   3. compute c = KDF-SM3(salt || seed) parsed by rejection mod q,
 *   4. recover s2 = c + h*s1 mod q, center-lift it,
 *   5. check ||(s1, s2)||^2 <= rejection_bound^2.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "yuanyang_inner.h"

static int
signature_and_key_values_fit_ntt_input(
	const int16_t s1[YUANYANG_D],
	const uint16_t h[YUANYANG_D])
{
	/*
	 * Verification computes h*s1 with the shared NTT implementation. The
	 * decoded public key is expected to be modulo q and the compressed
	 * signature half should stay in the normal signature range; this explicit
	 * guard keeps malformed inputs from reaching large-coefficient products.
	 */
	for (size_t i = 0; i < YUANYANG_D; i++) {
		if (s1[i] > (int16_t)YUANYANG_Q || h[i] > (uint16_t)YUANYANG_Q) {
			return 0;
		}
	}
	return 1;
}

static void
recover_second_signature_half(
	int32_t s2[YUANYANG_D], uint16_t hs1[YUANYANG_D],
	const uint16_t c[YUANYANG_D],
	const uint16_t h[YUANYANG_D],
	const int16_t s1[YUANYANG_D])
{
	/*
	 * Verification relation: c = s2 - h*s1 mod q, hence
	 * s2 = c + h*s1 mod q. Center lifting gives the integer representative
	 * used by the final Euclidean norm check.
	 */
	yuanyang_mul_mod_xn_plus_1(hs1, h, s1);
	for (size_t i = 0; i < YUANYANG_D; i++) {
		uint16_t s2_mod_q;

		s2_mod_q = (uint16_t)(((unsigned)c[i] + (unsigned)hs1[i])
			% YUANYANG_Q);
		s2[i] = (int32_t)yuanyang_center_lift_q(s2_mod_q);
	}
}

static int
signature_norm_is_within_bound(
	const int16_t s1[YUANYANG_D],
	const int32_t s2[YUANYANG_D])
{
	uint64_t norm_sq;

	norm_sq = 0;
	for (size_t i = 0; i < YUANYANG_D; i++) {
		norm_sq += (uint64_t)((int32_t)s1[i] * (int32_t)s1[i]);
		norm_sq += (uint64_t)(s2[i] * s2[i]);
		if (norm_sq > YUANYANG_REJECTION_BOUND_SQ) {
			return 0;
		}
	}
	return 1;
}

int
yuanyang_verify_core(
	const unsigned char *pk, unsigned long long pk_len_bytes,
	const unsigned char *sn, unsigned long long sn_len_bytes,
	const unsigned char *m, unsigned long long m_len_bytes)
{
	uint16_t h[YUANYANG_D];
	uint16_t c[YUANYANG_D];
	uint16_t hs1[YUANYANG_D];
	int16_t s1[YUANYANG_D];
	int32_t s2[YUANYANG_D];
	unsigned char seed[32];
	int accepted;
	int rc;
	if (pk == NULL || sn == NULL || m == NULL) {
		return YUANYANG_BADARG;
	}
	if (sn_len_bytes != YUANYANG_SIGNATURE_BYTES) {
		return YUANYANG_INVALID_SIGNATURE;
	}

	/* Signature layout is salt || compressed s1; decoding keeps the salt in sn. */
	{
		rc = yuanyang_decode_signature_s1(s1, sn, sn_len_bytes);
	}
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}

	rc = yuanyang_decode_public_key(h, pk, pk_len_bytes);
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}
	if (!signature_and_key_values_fit_ntt_input(s1, h)) {
		return YUANYANG_INVALID_SIGNATURE;
	}

	{
		rc = yuanyang_hash_message_with_public_key(seed, m, m_len_bytes, h);
	}
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}

	/*
	 * Rebuild the challenge from the stored salt and the message/public-key
	 * seed. This mirrors signing; no signature coefficient is hashed here.
	 */
	{
		rc = yuanyang_hash_to_challenge(c, sn, seed);
	}
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}

	{
		recover_second_signature_half(s2, hs1, c, h, s1);
	}

	{
		accepted = signature_norm_is_within_bound(s1, s2);
	}
	if (!accepted) {
		return YUANYANG_INVALID_SIGNATURE;
	}
	return YUANYANG_SUCCESS;
}

