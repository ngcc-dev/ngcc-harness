#include "api.h"
#include "drng.h"
#include "params.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NTESTS 100
#define SEED_LEN_BYTES 64
DRNG_ctx drng_algorithm;

static int
test_seed (unsigned char seed[SEED_LEN_BYTES])
{
	unsigned char pk[2][KEM_PUBLICKEYBYTES];
	unsigned long long pk_len_bytes = KEM_PUBLICKEYBYTES;
	unsigned char sk[2][KEM_SECRETKEYBYTES];
	unsigned long long sk_len_bytes = KEM_SECRETKEYBYTES;
	unsigned char ct[2][KEM_CIPHERTEXTBYTES];
	unsigned long long ct_len_bytes = KEM_CIPHERTEXTBYTES;
	unsigned char key_a[2][KEM_SSBYTES];
	unsigned char key_b[2][KEM_SSBYTES];
	unsigned long long ss_len_bytes = KEM_SSBYTES;

	for (int i = 0; i < 2; i++)
		{
			// Initialise the proper random context for the runs.
			init_random_number (&drng_algorithm, seed, SEED_LEN_BYTES);

			// Alice generates a public key
			kem_keygen (pk[i], &pk_len_bytes, sk[i], &sk_len_bytes);

			// Bob derives a secret key and creates a response
			kem_enc (pk[i], pk_len_bytes, key_b[i], &ss_len_bytes, ct[i],
							 &ct_len_bytes);

			// Alice uses Bobs response to get her shared key
			kem_dec (sk[i], sk_len_bytes, ct[i], ct_len_bytes, key_a[i],
							 &ss_len_bytes);
		}

	if (memcmp (pk[0], pk[1], KEM_PUBLICKEYBYTES) != 0)
		{
			printf (
					"ERROR: test_seed failed, public keys with same seed don't match.");
			return 1;
		}
	if (memcmp (sk[0], sk[1], KEM_SECRETKEYBYTES) != 0)
		{
			printf (
					"ERROR: test_seed failed, secret keys with same seed don't match.");
			return 1;
		}
	if (memcmp (ct[0], ct[1], KEM_CIPHERTEXTBYTES) != 0)
		{
			printf ("ERROR: test_seed failed, ciphertexts with same seed don't "
							"match.");
			return 1;
		}
	if (memcmp (key_a[0], key_a[1], KEM_SSBYTES) != 0)
		{
			printf (
					"ERROR: test_seed failed, shared secret a with same seed doesn't "
					"match.");
			return 1;
		}
	if (memcmp (key_b[0], key_b[1], KEM_SSBYTES) != 0)
		{
			printf (
					"ERROR: test_seed failed, shared secret b with same seed doesn't "
					"match.");
			return 1;
		}

	return 0;
}

int
main (void)
{
	unsigned int i;
	int r;

	// Randomness
	// For generating the seed, we assume for testing purposes
	// memory is sufficiently random.
	DRNG_ctx drng_seed;
	unsigned char seed[SEED_LEN_BYTES];

	// Initialise the "randomly", using the drng_seed memory as the random ctx.
	get_random_number (&drng_seed, seed, SEED_LEN_BYTES * 8);

	// Initialise the proper random context for the runs.
	init_random_number (&drng_seed, seed, SEED_LEN_BYTES);

	for (i = 0; i < NTESTS; i++)
		{
			// Initialise the proper random context for the runs.
			get_random_number (&drng_seed, seed, SEED_LEN_BYTES);

			r = test_seed (seed);
			if (r)
				return 1;
		}

	printf ("KEM_SECRETKEYBYTES:  %d\n", KEM_SECRETKEYBYTES);
	printf ("KEM_PUBLICKEYBYTES:  %d\n", KEM_PUBLICKEYBYTES);
	printf ("KEM_CIPHERTEXTBYTES: %d\n", KEM_CIPHERTEXTBYTES);

	return 0;
}
