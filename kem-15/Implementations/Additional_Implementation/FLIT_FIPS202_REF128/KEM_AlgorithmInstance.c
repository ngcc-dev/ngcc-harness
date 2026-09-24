/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "params.h"
#include "kem.h"

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

unsigned long long kem_get_pk_len_bytes()
{
	return KEM_PUBLICKEYBYTES;
}

unsigned long long kem_get_sk_len_bytes()
{
	return KEM_SECRETKEYBYTES;
}

unsigned long long kem_get_ss_len_bytes()
{
	return KEM_MSGBYTES;
}

unsigned long long kem_get_ct_len_bytes()
{
	return KEM_CIPHERTEXTBYTES;
}

int kem_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	int ret;
	*pk_len_bytes = KEM_PUBLICKEYBYTES;
	*sk_len_bytes = KEM_SECRETKEYBYTES;
	ret = crypto_kem_keypair(pk, sk);
	return ret;
}

int kem_enc(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes)
{
	int ret;
	(void)pk_len_bytes;
	*ss_len_bytes = KEM_MSGBYTES;
	*ct_len_bytes = KEM_CIPHERTEXTBYTES;
	ret = crypto_kem_enc(ct, ss, pk);
	return ret;
}

int kem_dec(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
	int ret;
	(void)sk_len_bytes;
	(void)ct_len_bytes;
	*ss_len_bytes = KEM_MSGBYTES;
	ret = crypto_kem_dec(ss, ct, sk);
	return ret;
}