/*
 * yuanyang-512 key generation
 *
 * This file is the entry point for the keygen. It will:
 *   - Generate f and g using PairGen
 *   - Compute the public key h = g/f in Z_q[X]/(X^d + 1), for q = 2689.
 *   - Call NTRUSolve to compute F and G
 *   - Compute the perturbation decomposition / weak-smoothness precomputations
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "drng.h"
#include "yuanyang_inner.h"
#include "security_loss.h"
#include "ntrugen/src/ntrugen.h"
#include "prng.h"
#include "poly_inv_ntt.h"

#define YUANYANG_KEYGEN_MAX_ATTEMPTS  256u

extern DRNG_ctx drng_algorithm;

static int
public_key_from_pair(uint16_t h[YUANYANG_D],
	const int8_t f[YUANYANG_D], const int8_t g[YUANYANG_D])
{
	uint16_t f_inv[YUANYANG_D];
	uint16_t f_q[YUANYANG_D];
	int16_t g_16[YUANYANG_D];

	for (size_t i = 0; i < YUANYANG_D; i++) {
		f_q[i] = f[i] < 0
			? (uint16_t)(YUANYANG_Q + (int16_t)f[i])
			: (uint16_t)f[i];
		g_16[i] = (int16_t) g[i];
	}

	if (poly_inv_xn1_q(f_inv, f_q, YUANYANG_D) != 0) {
		return 0;
	}
	yuanyang_mul_mod_xn_plus_1(h, f_inv, g_16);

	return 1;
}

static int
drng_randombytes(void *ctx, unsigned char *buf, unsigned long long len_bytes)
{
	return get_random_number((DRNG_ctx *)ctx, buf, 8u * len_bytes);
}

int
yuanyang_keygen_core_with_stats(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes,
	yuanyang_keygen_stats *stats)
{
	yuanyang_compact_sk decoded;
	yuanyang_expanded_sk expanded;
	prng rng;
	int32_t F[YUANYANG_D] = {0}, G[YUANYANG_D] = {0};

	if (stats != NULL) {
		memset(stats, 0, sizeof *stats);
	}
	if (pk_len_bytes != NULL) {
		*pk_len_bytes = YUANYANG_PK_BYTES;
	}
	if (sk_len_bytes != NULL) {
		*sk_len_bytes = YUANYANG_SK_BYTES;
	}
	if (pk == NULL || pk_len_bytes == NULL || sk == NULL || sk_len_bytes == NULL) {
		return YUANYANG_BADARG;
	}
	prng_init(&rng, drng_randombytes, &drng_algorithm);
	if (prng_status(&rng) != YUANYANG_SUCCESS) {
		return prng_status(&rng);
	}

	for (unsigned attempt = 0; attempt < YUANYANG_KEYGEN_MAX_ATTEMPTS; attempt++) {
		int rc;

		if (stats != NULL) {
			stats->attempts = attempt + 1u;
		}

		rc = yuanyang_pairgen(decoded.f, decoded.g, &rng);
		if (rc != YUANYANG_SUCCESS) {
			return rc;
		}

		/*
		 * The signature specification rejects pairs whose partial FFO profile
		 * gives too much RO-simulation SecurityLoss. Keep the check at key
		 * generation level so pair generation remains a pure sampler.
		 */
		if (!yuanyang_security_loss_accept(decoded.f, decoded.g)) {
			if (stats != NULL) {
				stats->security_loss_reject++;
			}
			continue;
		}

		if (!public_key_from_pair(decoded.h, decoded.f, decoded.g)) {
			if (stats != NULL) {
				stats->f_not_invertible_mod_q++;
			}
			continue;
		}

		/*
		 * ntrugen binds bigint limbs directly into this workspace.  Some of
		 * those values start with a logical length of zero and are extended
		 * later, so the backing limbs must initially be zero.
		 */
		uint64_t *tmp_buff = calloc(1u, (size_t)1u << 20);
		if (tmp_buff == NULL) {
			return YUANYANG_MEMORY_ERROR;
		}
		rc = ntrugen(decoded.f, decoded.g, F, G,
			 YUANYANG_LOGD, YUANYANG_Q, tmp_buff);
		free(tmp_buff);
		if (rc != NTRUGEN_OK) {
			if (stats != NULL) {
				if (rc == NTRUGEN_ERR_GCD) {
					stats->ntrusolve_gcd++;
				} else {
					stats->ntrusolve_other++;
				}
			}
			continue;
		}

		int basis_fits = 1;
		for (size_t i = 0; i < YUANYANG_D; i++) {
			int32_t yy_F, yy_G;

			/*
			 * ntrugen() returns a Bezout pair for f*F + g*G = q.
			 * YuanYang stores the NTRU basis as f*G - g*F = q.
			 */
			yy_F = -G[i];
			yy_G = F[i];
			if (yy_F < -127 || yy_F > 127
				|| yy_G < -127 || yy_G > 127)
			{
				if (stats != NULL) {
					stats->fg_not_int8++;
				}
				basis_fits = 0;
				break;
			}
			decoded.F[i] = (int8_t)yy_F;
			decoded.G[i] = (int8_t)yy_G;
		}
		if (!basis_fits) {
			continue;
		}
		if (stats != NULL) {
			stats->ntrusolve_ref++;
		}

		/*
		 * Export the compact basis with only the deterministic signing
		 * precomputations that are part of the secret key.  B_hat^{-1} is
		 * derived by sign() from B and u_hat.
		 */
		rc = yuanyang_expand_private_key(&expanded, &decoded);
		if (rc != YUANYANG_SUCCESS) {
			if (stats != NULL) {
				stats->perturbation++;
			}
			continue;
		}

		rc = yuanyang_encode_public_key(pk, *pk_len_bytes, decoded.h);
		if (rc != YUANYANG_SUCCESS) {
			return rc;
		}
		rc = yuanyang_encode_private_key(sk, sk_len_bytes, &expanded);
		return rc;
	}
	return YUANYANG_KEYGEN_FAILED;
}

int
yuanyang_keygen_core(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	return yuanyang_keygen_core_with_stats(
		pk, pk_len_bytes, sk, sk_len_bytes, NULL);
}
