/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include <stdint.h>
#include <string.h>
#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "params.h"
#include "kem.h"
#include "indcpa.h"
#include "verify.h"
#include "symmetric.h"

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

#define KEM_API_SUCCESS 0
#define KEM_API_DECAP_FAILURE -1
#define KEM_API_INVALID_ARGUMENT -2
#define KEM_API_LENGTH_MISMATCH -3
#define KEM_API_DRNG_FAILURE -4
#define KEM_API_CORE_FAILURE -5

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

static int get_drng_bytes(unsigned char *out, unsigned long long out_len_bytes)
{
	if (get_random_number(&drng_algorithm, out, out_len_bytes * 8ULL) != 0)
		return KEM_API_DRNG_FAILURE;
	return KEM_API_SUCCESS;
}

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
	unsigned char coins[2 * KYBER_SYMBYTES];

	if (pk == NULL || pk_len_bytes == NULL || sk == NULL || sk_len_bytes == NULL)
		return KEM_API_INVALID_ARGUMENT;

	*pk_len_bytes = kem_get_pk_len_bytes();
	*sk_len_bytes = kem_get_sk_len_bytes();

	if (get_drng_bytes(coins, sizeof(coins)) != KEM_API_SUCCESS)
		return KEM_API_DRNG_FAILURE;

	if (crypto_kem_keypair_derand(pk, sk, coins) != 0)
		return KEM_API_CORE_FAILURE;

	return KEM_API_SUCCESS;
}

int kem_enc(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes)
{
	unsigned char coins[KYBER_INDCPA_MSGBYTES];

	if (pk == NULL || ss == NULL || ss_len_bytes == NULL || ct == NULL || ct_len_bytes == NULL)
		return KEM_API_INVALID_ARGUMENT;
	if (pk_len_bytes != kem_get_pk_len_bytes())
		return KEM_API_LENGTH_MISMATCH;

	*ss_len_bytes = kem_get_ss_len_bytes();
	*ct_len_bytes = kem_get_ct_len_bytes();

	if (get_drng_bytes(coins, sizeof(coins)) != KEM_API_SUCCESS)
		return KEM_API_DRNG_FAILURE;

	if (crypto_kem_enc_derand(ct, ss, pk, coins) != 0)
		return KEM_API_CORE_FAILURE;

	return KEM_API_SUCCESS;
}

int kem_dec(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
	int fail;
	uint8_t buf[KYBER_INDCPA_MSGBYTES + PREFIXHASHBYTES];
	uint8_t kr[2 * KYBER_SYMBYTES];
	uint8_t cmp[KYBER_CIPHERTEXTBYTES];
	const uint8_t *pk;

	if (sk == NULL || ct == NULL || ss == NULL || ss_len_bytes == NULL)
		return KEM_API_INVALID_ARGUMENT;
	if (sk_len_bytes != kem_get_sk_len_bytes() || ct_len_bytes != kem_get_ct_len_bytes())
		return KEM_API_LENGTH_MISMATCH;

	pk = sk + KYBER_INDCPA_SECRETKEYBYTES;

	indcpa_dec(buf, ct, sk);
	memcpy(buf + KYBER_INDCPA_MSGBYTES,
	       sk + KYBER_SECRETKEYBYTES - PREFIXHASHBYTES - KYBER_SYMBYTES,
	       PREFIXHASHBYTES);
	// buf[0] = 0;
	// hash_g(kr, buf, KYBER_INDCPA_MSGBYTES + PREFIXHASHBYTES);
	// buf[0] = 1;
	// hash_g(kr + KYBER_SYMBYTES, buf, KYBER_INDCPA_MSGBYTES + PREFIXHASHBYTES);
	hash_g(kr, buf, KYBER_INDCPA_MSGBYTES + PREFIXHASHBYTES);
	indcpa_enc(cmp, buf, pk, kr + KYBER_SYMBYTES);

	fail = verify(ct, cmp, KYBER_CIPHERTEXTBYTES);

	rkprf(ss, sk + KYBER_SECRETKEYBYTES - KYBER_SYMBYTES, ct);
	cmov(ss, kr, KYBER_SSBYTES, (uint8_t)!fail);
	*ss_len_bytes = kem_get_ss_len_bytes();

	if (fail != 0)
		return KEM_API_DECAP_FAILURE;

	return KEM_API_SUCCESS;
}