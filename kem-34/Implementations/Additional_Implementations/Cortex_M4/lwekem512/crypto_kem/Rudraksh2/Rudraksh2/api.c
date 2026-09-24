/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "api.h"
#include "drng.h"

// Scheme Specific Includes
#include "drng.h"
#include "indcpa.h"
#include "params.h"
#include "symmetric.h"
#include "verify.h"

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

int
kem_keygen (unsigned char *pk, unsigned long long *pk_len_bytes,
						unsigned char *sk, unsigned long long *sk_len_bytes)
{
	size_t i;

	indcpa_keypair (pk, sk);

	for (i = 0; i < KEM_INDCPA_PUBLICKEYBYTES; i++)
		sk[i + KEM_INDCPA_SECRETKEYBYTES] = pk[i];

	hash_h (sk + KEM_SECRETKEYBYTES - 2 * KEM_SYMBYTES, pk, KEM_PUBLICKEYBYTES);

	/* Value z for pseudo-random output on reject */
	get_random_number (&drng_algorithm, sk + KEM_SECRETKEYBYTES - KEM_SYMBYTES,
										 KEM_SYMBYTES * 8);

	return 0;
}

int
kem_enc (unsigned char *pk, unsigned long long pk_len_bytes, unsigned char *ss,
				 unsigned long long *ss_len_bytes, unsigned char *ct,
				 unsigned long long *ct_len_bytes)
{
	uint8_t buf[2 * KEM_SYMBYTES];
	/* Will contain key, coins */
	uint8_t kr[2 * KEM_SYMBYTES];

	get_random_number (&drng_algorithm, buf, KEM_SYMBYTES * 8);

	/* Multitarget countermeasure for coins + contributory KEM */
	hash_h (buf + KEM_SYMBYTES, pk, KEM_PUBLICKEYBYTES);

	hash_g (kr, buf, 2 * KEM_SYMBYTES);

	/* coins are in kr+KEM_SYMBYTES */
	indcpa_enc (ct, buf, pk, kr + KEM_SYMBYTES);

	// /* overwrite coins in kr with H(c) */
	// hash_h(kr+KEM_SYMBYTES, ct, KEM_CIPHERTEXTBYTES);

	/* hash concatenation of pre-k and H(c) to k */
	// kdf(ss, kr, 2*KEM_SYMBYTES);
	memcpy (ss, kr, KEM_SYMBYTES);

	return 0;
}

int
kem_dec (unsigned char *sk, unsigned long long sk_len_bytes, unsigned char *ct,
				 unsigned long long ct_len_bytes, unsigned char *ss,
				 unsigned long long *ss_len_bytes)
{
	int fail;
	// uint8_t buf[2 * KEM_SYMBYTES];
	uint8_t buf[KEM_SYMBYTES];
	/* Will contain key, coins */
	uint8_t kr[2 * KEM_SYMBYTES];
	// uint8_t cmp[KEM_CIPHERTEXTBYTES];
	const uint8_t *pk = sk + KEM_INDCPA_SECRETKEYBYTES;
	// uint8_t ct1[KEM_CIPHERTEXTBYTES + KEM_SYMBYTES];

	indcpa_dec (buf, ct, sk);
	memcpy(kr, buf, KEM_SYMBYTES);

	/* Multitarget countermeasure for coins + contributory KEM */
	memcpy(kr+KEM_SYMBYTES, sk+KEM_SECRETKEYBYTES-2*KEM_SYMBYTES, KEM_SYMBYTES);
		
	hash_g (kr, kr, 2 * KEM_SYMBYTES);

	/* coins are in kr+KEM_SYMBYTES */
	fail = indcpa_enc_cmp (ct, buf, pk, kr + KEM_SYMBYTES);

	hash_h(ss, ct, KEM_CIPHERTEXTBYTES);
 	hash_h(ss, sk+KEM_SECRETKEYBYTES-KEM_SYMBYTES, KEM_SYMBYTES);

	cmov (ss, kr, KEM_SYMBYTES, (uint8_t) (1-fail));


	return 0;
}
