#include "api.h"
#include "drng.h"
#include <stdint.h>

#define SEED_LEN_BYTES 64
DRNG_ctx drng_algorithm;

static int
basic_run ()
{
	unsigned char pk[KEM_PUBLICKEYBYTES];
	unsigned long long pk_len_bytes = KEM_PUBLICKEYBYTES;
	unsigned char sk[KEM_SECRETKEYBYTES];
	unsigned long long sk_len_bytes = KEM_SECRETKEYBYTES;
	unsigned char ct[KEM_CIPHERTEXTBYTES];
	unsigned long long ct_len_bytes = KEM_CIPHERTEXTBYTES;
	unsigned char key_a[KEM_SSBYTES];
	unsigned char key_b[KEM_SSBYTES];
	unsigned long long ss_len_bytes = KEM_SSBYTES;

	// Alice generates a public key
	kem_keygen (pk, &pk_len_bytes, sk, &sk_len_bytes);

	//// Bob derives a secret key and creates a response
	kem_enc (pk, pk_len_bytes, key_b, &ss_len_bytes, ct, &ct_len_bytes);

	//// Alice uses Bobs response to get her shared key
	kem_dec (sk, sk_len_bytes, ct, ct_len_bytes, key_a, &ss_len_bytes);

	return 0;
}

int
main (void)
{
	// Randomness
	// For generating the seed, we assume for testing purposes
	// memory is sufficiently random.
	DRNG_ctx drng_seed;
	unsigned char seed[SEED_LEN_BYTES];

	// Initialise the "randomly", using the drng_seed memory as the random ctx.
	get_random_number (&drng_seed, seed, SEED_LEN_BYTES * 8);

	// Initialise the proper random context for the runs.
	init_random_number (&drng_algorithm, seed, SEED_LEN_BYTES);

	basic_run ();

	return 0;
}
