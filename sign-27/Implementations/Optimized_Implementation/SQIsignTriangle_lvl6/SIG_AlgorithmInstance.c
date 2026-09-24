/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.

------------------------------------------------------------------------------
API_PKC SIG interface implementation for the SQIsignTriangle digital signature.

This file is the thin wrapper that maps the API_PKC programming interface
(sig_keygen / sig_sign / sig_verify, declared in SIG_AlgorithmInstance.h) onto
the SQIsignTriangle C reference NIST API (crypto_sign_keypair / crypto_sign /
crypto_sign_open, declared in the bundled sqisigntriangle/.../api.h).

The parameter-set sizes (CRYPTO_PUBLICKEYBYTES, CRYPTO_SECRETKEYBYTES,
CRYPTO_BYTES) and the three crypto_* entry points are selected at compile time
through the SQISIGN_VARIANT / api.h include path configured by CMakeLists.txt,
so this same source file serves every SQIsignTriangle parameter set
(lvl1 / lvl2 / lvl5 / lvl6).
*/

#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include "api.h"

#include <stdlib.h>
#include <string.h>

/* The deterministic RNG instance that KAT_SIG.c seeds before each key pair.
   It is defined in KAT_SIG.c and shared here. */
extern DRNG_ctx drng_algorithm;

/* ---------------------------------------------------------------------------
   randombytes() override.

   SQIsignTriangle draws all of its randomness through randombytes().  For
   known-answer testing we must route that randomness through the ICCS DRNG so
   the output is reproducible from the published seed.  get_random_number()
   takes a length in BITS, hence the (* 8).  This definition is linked into the
   final executable and takes precedence over the library's default
   /dev/urandom implementation.
--------------------------------------------------------------------------- */
int
randombytes(unsigned char *x, unsigned long long xlen)
{
	return get_random_number(&drng_algorithm, x, xlen * 8);
}

/* SQIsignTriangle's NIST harness expects this symbol; the API_PKC DRNG is
   seeded by KAT_SIG.c via init_random_number(), so this is a no-op. */
void
randombytes_init(unsigned char *entropy_input,
                 unsigned char *personalization_string,
                 int security_strength)
{
	(void)entropy_input;
	(void)personalization_string;
	(void)security_strength;
}

/* Claimed byte length of the public key for this parameter set. */
unsigned long long
sig_get_pk_len_bytes(void)
{
	return CRYPTO_PUBLICKEYBYTES;
}

/* Claimed byte length of the private key for this parameter set. */
unsigned long long
sig_get_sk_len_bytes(void)
{
	return CRYPTO_SECRETKEYBYTES;
}

/* Claimed byte length of the signature for this parameter set. */
unsigned long long
sig_get_sn_len_bytes(void)
{
	return CRYPTO_BYTES;
}

/* Key generation: produce a (public key, private key) pair.
   Returns 0 on success, or a negative error code (-1..-99). */
int
sig_keygen(unsigned char *pk,
           unsigned long long *pk_len_bytes,
           unsigned char *sk,
           unsigned long long *sk_len_bytes)
{
	int ret;

	if (pk == NULL || pk_len_bytes == NULL || sk == NULL || sk_len_bytes == NULL)
		return -2;

	ret = crypto_sign_keypair(pk, sk);
	if (ret != 0)
		return -3;

	*pk_len_bytes = CRYPTO_PUBLICKEYBYTES;
	*sk_len_bytes = CRYPTO_SECRETKEYBYTES;
	return 0;
}

/* Signature generation.
   SQIsignTriangle's crypto_sign() produces a "signed message" sm = signature ‖
   message of length CRYPTO_BYTES + m_len_bytes.  The API_PKC interface wants the
   detached signature only, so we keep the first CRYPTO_BYTES bytes.
   Returns 0 on success, or a negative error code (-1..-99). */
int
sig_sign(unsigned char *sk,
         unsigned long long sk_len_bytes,
         unsigned char *m,
         unsigned long long m_len_bytes,
         unsigned char *sn,
         unsigned long long *sn_len_bytes)
{
	unsigned char *sm;
	unsigned long long smlen = 0;
	int ret;

	if (sk == NULL || m == NULL || sn == NULL || sn_len_bytes == NULL)
		return -2;
	if (sk_len_bytes != CRYPTO_SECRETKEYBYTES)
		return -4;

	sm = (unsigned char *)malloc(CRYPTO_BYTES + m_len_bytes);
	if (sm == NULL)
		return -5;

	ret = crypto_sign(sm, &smlen, m, m_len_bytes, sk);
	if (ret != 0 || smlen != CRYPTO_BYTES + m_len_bytes) {
		free(sm);
		return -3;
	}

	memcpy(sn, sm, CRYPTO_BYTES);
	*sn_len_bytes = CRYPTO_BYTES;
	free(sm);
	return 0;
}

/* Signature verification.
   crypto_sign_open() consumes a signed message sm = signature ‖ message, so we
   reassemble it from the detached signature and the message, then check that
   the recovered message matches.
   Returns 0 if valid, -1 if invalid, or another negative code (-2..-99) on
   internal error. */
int
sig_verify(unsigned char *pk,
           unsigned long long pk_len_bytes,
           unsigned char *sn,
           unsigned long long sn_len_bytes,
           unsigned char *m,
           unsigned long long m_len_bytes)
{
	unsigned char *sm;
	unsigned char *opened;
	unsigned long long opened_len = 0;
	int ret;

	if (pk == NULL || sn == NULL || m == NULL)
		return -2;
	if (pk_len_bytes != CRYPTO_PUBLICKEYBYTES || sn_len_bytes != CRYPTO_BYTES)
		return -4;

	sm = (unsigned char *)malloc(sn_len_bytes + m_len_bytes);
	opened = (unsigned char *)malloc(m_len_bytes == 0 ? 1 : m_len_bytes);
	if (sm == NULL || opened == NULL) {
		free(sm);
		free(opened);
		return -5;
	}

	memcpy(sm, sn, sn_len_bytes);
	memcpy(sm + sn_len_bytes, m, m_len_bytes);
	ret = crypto_sign_open(opened, &opened_len, sm, sn_len_bytes + m_len_bytes, pk);
	if (ret != 0 || opened_len != m_len_bytes || memcmp(opened, m, m_len_bytes) != 0)
		ret = -1;
	else
		ret = 0;

	free(opened);
	free(sm);
	return ret;
}
