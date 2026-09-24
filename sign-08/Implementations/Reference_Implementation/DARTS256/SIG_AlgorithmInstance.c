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
#include "sign.h"
#include "params.h"
#include "drng.h"

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
	return CRYPTO_SIGNATUREBYTES;
}

int sig_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	int ret = crypto_sign_keypair(pk, sk);
	if (ret == 0) {
		*pk_len_bytes = CRYPTO_PUBLICKEYBYTES;
		*sk_len_bytes = CRYPTO_SECRETKEYBYTES;
	}
	return ret;
}

int sig_sign(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes,
	unsigned char *sn, unsigned long long *sn_len_bytes)
{
	(void)sk_len_bytes; // ICCS API 传入私钥长度，内部不需要使用

	size_t siglen = 0;
	int ret = crypto_sign_signature(sn, &siglen, m, (size_t)m_len_bytes, sk);
	if (ret == 0) {
		*sn_len_bytes = (unsigned long long)siglen;
	}
	return ret;
}

int sig_verify(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *sn, unsigned long long sn_len_bytes,
	unsigned char *m, unsigned long long m_len_bytes)
{
	(void)pk_len_bytes; // ICCS API 传入公钥长度，内部不需要使用

	return crypto_sign_verify(sn, (size_t)sn_len_bytes, m, (size_t)m_len_bytes, pk);
}