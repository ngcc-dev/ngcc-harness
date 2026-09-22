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

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

unsigned long long kem_get_pk_len_bytes()
{
	return PUBLIC_KEY_BYTES;
}

unsigned long long kem_get_sk_len_bytes()
{
	return SECRET_KEY_BYTES;
}

unsigned long long kem_get_ss_len_bytes()
{
	return SHARED_SECRET_BYTES;
}

unsigned long long kem_get_ct_len_bytes()
{
	return CIPHERTEXT_BYTES;
}

int kem_keygen(
	unsigned char* pk, unsigned long long* pk_len_bytes,
	unsigned char* sk, unsigned long long* sk_len_bytes)
{
	*pk_len_bytes = PUBLIC_KEY_BYTES;
	*sk_len_bytes = SECRET_KEY_BYTES;
	return crypto_kem_keypair(pk, sk);
}

int kem_enc(
	unsigned char* pk, unsigned long long pk_len_bytes,
	unsigned char* ss, unsigned long long* ss_len_bytes,
	unsigned char* ct, unsigned long long* ct_len_bytes)
{
	*ss_len_bytes = SHARED_SECRET_BYTES;
	*ct_len_bytes = CIPHERTEXT_BYTES;
	return crypto_kem_enc(ct, ss, pk);
}

int kem_dec(
	unsigned char* sk, unsigned long long sk_len_bytes,
	unsigned char* ct, unsigned long long ct_len_bytes,
	unsigned char* ss, unsigned long long* ss_len_bytes)
{
	*ss_len_bytes = SHARED_SECRET_BYTES;
	return crypto_kem_dec(ss, ct, sk);
}