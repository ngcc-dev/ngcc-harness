#include "api.h"
#include "cbd.h"
#include "drng.h"
#include "indcpa.h"
#include "packing.h"
#include "polyvec.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NTESTS 100
#define SEED_LEN_BYTES 64
DRNG_ctx drng_algorithm;

static int
test_packing_pk (unsigned char *pk)
{
	uint8_t repacked_pk[KEM_PUBLICKEYBYTES];
	polyvec pk_poly;
	uint8_t seed[KEM_SYMBYTES];

	// Unpack the bytes to a polynomial
	unpack_pk (&pk_poly, seed, pk);

	// Re-pack the public key
	pack_pk (repacked_pk, &pk_poly, seed);

	// Check
	if (memcmp (pk, repacked_pk, KEM_PUBLICKEYBYTES) != 0)
		{
			printf ("ERROR pack_pk fails\n");
			return 1;
		}

	return 0;
}

static int
test_packing_sk (unsigned char *sk)
{
	uint8_t repacked_sk[KEM_INDCPA_SECRETKEYBYTES];
	polyvec sk_poly;

	// Unpack it
	unpack_sk (&sk_poly, sk);

	// Re-pack the public key
	pack_sk (repacked_sk, &sk_poly);

	// Check
	if (memcmp (sk, repacked_sk, KEM_INDCPA_SECRETKEYBYTES) != 0)
		{
			printf ("ERROR pack_pk fails\n");
			return 1;
		}

	return 0;
}

static int
test_packing_ct (unsigned char *ct)
{
	polyvec b;
	poly v;
	uint8_t repacked_ct[KEM_INDCPA_BYTES];

	// Unpack it
	unpack_ciphertext (&b, &v, ct);

	// Re-pack the public key
	pack_ciphertext (repacked_ct, &b, &v);

	// Check
	if (memcmp (ct, repacked_ct, KEM_INDCPA_BYTES) != 0)
		{
			printf ("ERROR pack_ciphertext fails\n");
			return 1;
		}

	return 0;
}

int
main (void)
{
	unsigned char pk[KEM_PUBLICKEYBYTES];
	unsigned long long pk_len;
	unsigned char sk[KEM_SECRETKEYBYTES];
	unsigned long long sk_len;
	unsigned char ct[KEM_CIPHERTEXTBYTES];
	unsigned long long ct_len;
	unsigned char ss[KEM_SSBYTES];
	unsigned long long ss_len;
	int ret = 0;

	// Randomness
	// For generating the seed, we assume for testing purposes
	// memory is sufficiently random.
	DRNG_ctx drng_seed;
	unsigned char seed[SEED_LEN_BYTES];

	// Initialise the "randomly", using the drng_seed memory as the random ctx.
	get_random_number (&drng_seed, seed, SEED_LEN_BYTES * 8);

	// Initialise the proper random context for the runs.
	init_random_number (&drng_algorithm, seed, SEED_LEN_BYTES);

	// Initialize keys
	kem_keygen (pk, &pk_len, sk, &sk_len);

	// Check packing of keys
	ret |= test_packing_pk (pk);
	ret |= test_packing_sk (sk);

	// Encapsulate
	kem_enc (pk, pk_len, ss, &ss_len, ct, &ct_len);

	// Check packing of ciphertext
	ret |= test_packing_ct (ct);

	printf ("KEM_SECRETKEYBYTES:  %d\n", KEM_SECRETKEYBYTES);
	printf ("KEM_PUBLICKEYBYTES:  %d\n", KEM_PUBLICKEYBYTES);
	printf ("KEM_CIPHERTEXTBYTES: %d\n", KEM_CIPHERTEXTBYTES);

	return ret;
}
