/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "kem_qube.h"
#include "drng.h"
#include "api.h"
#include "stdint.h"

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

unsigned long long kem_get_pk_len_bytes()
{
	return CRYPTO_PUBLICKEYBYTES;
}

unsigned long long kem_get_sk_len_bytes()
{
	return CRYPTO_SECRETKEYBYTES;
}

unsigned long long kem_get_ss_len_bytes()
{
	return CRYPTO_BYTES;
}

unsigned long long kem_get_ct_len_bytes()
{
	return CRYPTO_CIPHERTEXTBYTES;
}

int kem_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
    crypto_kem_keypair(pk, sk);
    *pk_len_bytes=kem_get_pk_len_bytes();
    *sk_len_bytes=kem_get_sk_len_bytes();
	return 0;
}

int kem_enc(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes)
{
    crypto_kem_enc(ct, ss, pk);
    *ss_len_bytes=kem_get_ss_len_bytes();
    *ct_len_bytes=kem_get_ct_len_bytes();
	return 0;
}

int kem_dec(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
    crypto_kem_dec(ss, ct, sk);
    *ss_len_bytes=kem_get_ss_len_bytes();
	return 0;
}