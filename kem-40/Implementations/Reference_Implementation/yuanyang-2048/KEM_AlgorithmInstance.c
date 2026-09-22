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
#include "yy_kem_inner.h"

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

unsigned long long kem_get_pk_len_bytes()
{
	return yy_get_pk_len_bytes();
}

unsigned long long kem_get_sk_len_bytes()
{
	return yy_get_sk_len_bytes();
}

unsigned long long kem_get_ss_len_bytes()
{
	return yy_get_ss_len_bytes();
}

unsigned long long kem_get_ct_len_bytes()
{
	return yy_get_ciphertext_bytes();
}

int kem_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	return yy_kem_keygen(pk,pk_len_bytes,sk,sk_len_bytes);
}

int kem_enc(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes)
{
	return yy_encapsulate_API(pk,pk_len_bytes,ss,ss_len_bytes,ct,ct_len_bytes);
}

int kem_dec(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
	return yy_decapsulate_API(sk,sk_len_bytes,ct,ct_len_bytes,ss,ss_len_bytes);
}
