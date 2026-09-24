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
#include "api.h"
#include "drng.h"
#include "symmetric.h"

extern DRNG_ctx drng_algorithm;

static int triq_seed_prng(void)
{
	unsigned char entropy[48] = {0};
	unsigned char personalization[48] = {0};

	if (get_random_number(&drng_algorithm, entropy, sizeof(entropy) * 8ULL) != 0)
		return -11;
	if (get_random_number(&drng_algorithm, personalization, sizeof(personalization) * 8ULL) != 0)
		return -12;

	prng_init(entropy, personalization, (uint32_t)sizeof(entropy), (uint32_t)sizeof(personalization));
	return 0;
}

unsigned long long kem_get_pk_len_bytes(void)
{
	return CRYPTO_PUBLICKEYBYTES;
}

unsigned long long kem_get_sk_len_bytes(void)
{
	return CRYPTO_SECRETKEYBYTES;
}

unsigned long long kem_get_ss_len_bytes(void)
{
	return CRYPTO_BYTES;
}

unsigned long long kem_get_ct_len_bytes(void)
{
	return CRYPTO_CIPHERTEXTBYTES;
}

int kem_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	int ret = triq_seed_prng();
	if (ret != 0)
		return ret;
	if (crypto_kem_keypair(pk, sk) != 0)
		return -21;
	*pk_len_bytes = CRYPTO_PUBLICKEYBYTES;
	*sk_len_bytes = CRYPTO_SECRETKEYBYTES;
	return 0;
}

int kem_enc(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes)
{
	int ret;

	if (pk_len_bytes != CRYPTO_PUBLICKEYBYTES)
		return -31;
	ret = triq_seed_prng();
	if (ret != 0)
		return ret;
	if (crypto_kem_enc(ct, ss, pk) != 0)
		return -32;
	*ss_len_bytes = CRYPTO_BYTES;
	*ct_len_bytes = CRYPTO_CIPHERTEXTBYTES;
	return 0;
}

int kem_dec(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
	if (sk_len_bytes != CRYPTO_SECRETKEYBYTES)
		return -41;
	if (ct_len_bytes != CRYPTO_CIPHERTEXTBYTES)
		return -42;
	if (crypto_kem_dec(ss, ct, sk) != 0)
		return -43;
	*ss_len_bytes = CRYPTO_BYTES;
	return 0;
}
