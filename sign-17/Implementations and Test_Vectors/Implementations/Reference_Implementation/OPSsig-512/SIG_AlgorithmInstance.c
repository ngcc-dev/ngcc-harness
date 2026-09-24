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
#include "params.h"
#include "sign.h"

// DRNG_ctx for generating pseudorandom numbers within the SIG scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

unsigned long long sig_get_pk_len_bytes()
{
	return CRYPTO_PUBLICKEYBYTES;
}

unsigned long long sig_get_sk_len_bytes()
{
	return CRYPTO_SECRETKEYBYTES;
}

unsigned long long sig_get_sn_len_bytes()
{
	return CRYPTO_BYTES;
}

int sig_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	if(pk == NULL || sk == NULL || pk_len_bytes == NULL || sk_len_bytes == NULL)
		return -2;

	*pk_len_bytes = CRYPTO_PUBLICKEYBYTES;
	*sk_len_bytes = CRYPTO_SECRETKEYBYTES;
	return crypto_sign_keypair(pk, sk);
}

int sig_sign(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes)
{
	size_t outlen = 0;

	if(sk == NULL || m == NULL || sn == NULL || sn_len_bytes == NULL)
		return -2;
	if(sk_len_bytes != CRYPTO_SECRETKEYBYTES)
		return -3;

	if(crypto_sign_signature(sn, &outlen, m, (size_t)m_len_bytes, NULL, 0, sk))
		return -4;

	*sn_len_bytes = (unsigned long long)outlen;
	return 0;
}

int sig_verify(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *sn, unsigned long long sn_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes)
{
	if(pk == NULL || sn == NULL || m == NULL)
		return -2;
	if(pk_len_bytes != CRYPTO_PUBLICKEYBYTES)
		return -3;
	return crypto_sign_verify(sn, (size_t)sn_len_bytes, m, (size_t)m_len_bytes, NULL, 0, pk);
}
