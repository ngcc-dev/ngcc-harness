/*
 * yuanyang-1024 verification.
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
	uint64_t norm_sq;
	int rc;
	if (pk == 0 || sn == 0 || m == 0) {
		return YUANYANG_BADARG;
	}
	if (sn_len_bytes != YUANYANG_SIGNATURE_BYTES) {
		return YUANYANG_INVALID_SIGNATURE;
	}

	rc = yuanyang_decode_signature_s1(s1, sn, sn_len_bytes);
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}

	rc = yuanyang_decode_public_key(h, pk, pk_len_bytes);
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}
	/* Add a check on the signature and public key values to ensure nothing
	* malicious is going on: since we use the NTT with a very large q trick to
	* compute s2, one could forge malicious large coefficient signature/h to
	* overflow the modulus.
	*
	* We know that all coeffs must be less than q. A tighter bound could be
	* checked f needed, but comparing to q is enough for this specific issue
	*/
	for (size_t i = 0; i < YUANYANG_D; i++)
	{
		if (s1[i] > (int16_t)YUANYANG_Q || h[i] > (int16_t)YUANYANG_Q) {
			return YUANYANG_INVALID_SIGNATURE;
		}
	}

	rc = yuanyang_hash_message_with_public_key(seed, m, m_len_bytes, h);
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}

	rc = yuanyang_hash_to_challenge(c, sn, seed);
	if (rc != YUANYANG_SUCCESS) {
		return rc;
	}

	yuanyang_mul_mod_xn_plus_1(hs1, h, s1);
	for (size_t i = 0; i < YUANYANG_D; i++) {
		uint16_t s2_mod_q;

		s2_mod_q = (uint16_t)(((unsigned)c[i] + (unsigned)hs1[i]) % YUANYANG_Q);
		s2[i] = (int32_t)yuanyang_center_lift_q(s2_mod_q);
	}

	norm_sq = 0;
	for (size_t i = 0; i < YUANYANG_D; i++) {
		norm_sq += (uint64_t)((int32_t)s1[i] * (int32_t)s1[i]);
		norm_sq += (uint64_t)(s2[i] * s2[i]);
		if (norm_sq > YUANYANG_REJECTION_BOUND_SQ) {
			return YUANYANG_INVALID_SIGNATURE;
		}
	}
	return YUANYANG_SUCCESS;
}

