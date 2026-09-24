/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include "api.h"

#include <stdint.h>

// DRNG_ctx for generating pseudorandom numbers within the SIG scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

#define SIG_SUCCESS 0
#define SIG_INVALID_SIGNATURE -1
#define SIG_INVALID_ARGUMENT -2
#define SIG_RANDOM_FAILED -3
#define SIG_CRYPTO_FAILED -4

unsigned long long sig_get_pk_len_bytes(void)
{
	return CRYPTO_PUBLICKEYBYTES;
}

unsigned long long sig_get_sk_len_bytes(void)
{
	return CRYPTO_SECRETKEYBYTES;
}

unsigned long long sig_get_sn_len_bytes(void)
{
	return CRYPTO_BYTES;
}

int sig_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	unsigned char seed[CRYPTO_SEEDBYTES];

	if (pk == 0 || pk_len_bytes == 0 || sk == 0 || sk_len_bytes == 0)
	{
		return SIG_INVALID_ARGUMENT;
	}

	if (get_random_number(&drng_algorithm, seed, CRYPTO_SEEDBYTES * 8) != 0)
	{
		return SIG_RANDOM_FAILED;
	}

	if (crypto_sign_seed_keypair(pk, sk, seed) != 0)
	{
		return SIG_CRYPTO_FAILED;
	}

	*pk_len_bytes = CRYPTO_PUBLICKEYBYTES;
	*sk_len_bytes = CRYPTO_SECRETKEYBYTES;
	return SIG_SUCCESS;
}

int sig_sign(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes)
{
	size_t siglen;
	size_t tfors_siglen;

	if (sk == 0 || m == 0 || sn == 0 || sn_len_bytes == 0)
	{
		return SIG_INVALID_ARGUMENT;
	}
	if (sk_len_bytes != CRYPTO_SECRETKEYBYTES)
	{
		return SIG_INVALID_ARGUMENT;
	}
	if (m_len_bytes > (unsigned long long)SIZE_MAX)
	{
		return SIG_INVALID_ARGUMENT;
	}

	if (crypto_sign_signature(sn, &siglen, &tfors_siglen, m, (size_t)m_len_bytes, sk) != 0)
	{
		return SIG_CRYPTO_FAILED;
	}

	(void)tfors_siglen;
	*sn_len_bytes = siglen;
	return SIG_SUCCESS;
}

int sig_verify(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *sn, unsigned long long sn_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes)
{
	int ret;

	if (pk == 0 || sn == 0 || m == 0)
	{
		return SIG_INVALID_ARGUMENT;
	}
	if (pk_len_bytes != CRYPTO_PUBLICKEYBYTES)
	{
		return SIG_INVALID_ARGUMENT;
	}
	if (sn_len_bytes > (unsigned long long)SIZE_MAX ||
		m_len_bytes > (unsigned long long)SIZE_MAX)
	{
		return SIG_INVALID_ARGUMENT;
	}

	ret = crypto_sign_verify(sn, (size_t)sn_len_bytes, m, (size_t)m_len_bytes, pk);
	if (ret == 0)
	{
		return SIG_SUCCESS;
	}
	if (ret == -1)
	{
		return SIG_INVALID_SIGNATURE;
	}
	return SIG_CRYPTO_FAILED;
}
