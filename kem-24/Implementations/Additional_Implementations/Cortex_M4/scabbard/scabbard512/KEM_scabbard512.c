/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "KEM_scabbard512.h"
#include "params.h"
#include "drng.h"
#include "indcpa.h"
#include "auxfunc.h"
#include "verify.h"

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

unsigned long long kem_get_pk_len_bytes()
{
	return SCABBARD_PUBLICKEYBYTES;
}

unsigned long long kem_get_sk_len_bytes()
{
	return SCABBARD_SECRETKEYBYTES;
}

unsigned long long kem_get_ss_len_bytes()
{
	return SCABBARD_SSBYTES;
}

unsigned long long kem_get_ct_len_bytes()
{
	return SCABBARD_CIPHERTEXTBYTES;
}

int kem_keygen(
	unsigned char *pk, unsigned long long *pk_len_bytes,
	unsigned char *sk, unsigned long long *sk_len_bytes)
{
	size_t i;

	*pk_len_bytes = kem_get_pk_len_bytes();
	*sk_len_bytes = kem_get_sk_len_bytes();

	indcpa_keypair(pk, sk);
	for (i = 0; i < SCABBARD_INDCPA_PUBLICKEYBYTES; i++)
		sk[SCABBARD_INDCPA_SECRETKEYBYTES + i] = pk[i];
	
	/* H(pk) */
	pseudoXOF(SCABBARD_SYMBYTES * 8, pk, *pk_len_bytes * 8, sk + SCABBARD_SECRETKEYBYTES - 2 * SCABBARD_SYMBYTES);

	/* pseudo-random output on reject */
	get_random_number(&drng_algorithm, sk + SCABBARD_SECRETKEYBYTES - SCABBARD_SYMBYTES, SCABBARD_SYMBYTES * 8);

	return 0;
}

int kem_enc(
	unsigned char *pk, unsigned long long pk_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes,
	unsigned char *ct, unsigned long long *ct_len_bytes)
{ 
	uint8_t buf[2 * SCABBARD_SYMBYTES]; /* random message + H(pk)*/
  	uint8_t kr[2 * SCABBARD_SYMBYTES]; /* Will contain key, random coins */

	*ss_len_bytes = kem_get_ss_len_bytes();
	*ct_len_bytes = kem_get_ct_len_bytes();

	/* buf[0:31] <- random message m' */
	get_random_number(&drng_algorithm, buf, SCABBARD_SYMBYTES * 8);
	pseudoXOF(SCABBARD_SYMBYTES * 8, buf, SCABBARD_SYMBYTES * 8, buf); /* Don't release system RNG output */

	/* buf[32:63] <- H(pk) */
	pseudoXOF(SCABBARD_SYMBYTES * 8, pk, pk_len_bytes * 8, buf + SCABBARD_SYMBYTES);

	/* buf[0:63] <- H(m' || H(pk))*/
	pseudoXOF(2 * SCABBARD_SYMBYTES * 8, buf, 2 * SCABBARD_SYMBYTES * 8, kr);

	/* random coins is in kr[32:63] */
	indcpa_enc(ct, buf, pk, kr + SCABBARD_SYMBYTES);

	/* overwrite random coins with H(ct) */
	pseudoXOF(SCABBARD_SYMBYTES * 8, ct, *ct_len_bytes * 8, kr + SCABBARD_SYMBYTES);

	/* ss <- KDF(K || H(ct)) */
	pseudoXOF(*ss_len_bytes * 8, kr, 2 * SCABBARD_SYMBYTES * 8, ss);

	return 0;
}

int kem_dec(
	unsigned char *sk, unsigned long long sk_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
	size_t i;

	int fail;
	uint8_t buf[2 * SCABBARD_SYMBYTES]; 
  	uint8_t kr[2 * SCABBARD_SYMBYTES];
	const uint8_t *pk = sk + SCABBARD_INDCPA_SECRETKEYBYTES;

	*ss_len_bytes = kem_get_ss_len_bytes();

	indcpa_dec(buf, ct, sk);
	for(i = 0; i < SCABBARD_SYMBYTES; i++)
		buf[SCABBARD_SYMBYTES + i] = sk[sk_len_bytes - 2 * SCABBARD_SYMBYTES + i]; 
	
	pseudoXOF(2 * SCABBARD_SYMBYTES * 8, buf, 2 * SCABBARD_SYMBYTES * 8, kr);
	fail = indcpa_enc_cmp(ct, buf, pk, kr + SCABBARD_SYMBYTES);
/* kr[32:63] <- H(ct) */
	pseudoXOF(SCABBARD_SYMBYTES * 8, ct, ct_len_bytes * 8, kr + SCABBARD_SYMBYTES);
	cmov(kr, sk + sk_len_bytes - SCABBARD_SYMBYTES, SCABBARD_SYMBYTES, fail);
	pseudoXOF(*ss_len_bytes * 8, kr, 2 * SCABBARD_SYMBYTES * 8, ss);

	return 0;
}