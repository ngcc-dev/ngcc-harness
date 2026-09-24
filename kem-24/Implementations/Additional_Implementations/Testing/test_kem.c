#include "api.h"
#include "drng.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NTESTS 100
#define SEED_LEN_BYTES 64
DRNG_ctx drng_algorithm;

static int
test_keys ()
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

	// Bob derives a secret key and creates a response
	kem_enc (pk, pk_len_bytes, key_b, &ss_len_bytes, ct, &ct_len_bytes);

	// Alice uses Bobs response to get her shared key
	kem_dec (sk, sk_len_bytes, ct, ct_len_bytes, key_a, &ss_len_bytes);

	for (int i = 0; i < KEM_SSBYTES; i++)
		{
			if (key_a[i] != key_b[i])
				{
					printf ("key_a[%d] = %02x, key_b[%d] = %02x\n", i, key_a[i], i,
									key_b[i]);
					printf ("ERROR keys, test: %d\n", i);
					return 1;
				}
		}

	return 0;
}

static int
test_invalid_sk_a ()
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
	uint8_t ret = 0;

	// Alice generates a public key
	kem_keygen (pk, &pk_len_bytes, sk, &sk_len_bytes);

	// Bob derives a secret key and creates a response
	kem_enc (pk, pk_len_bytes, key_b, &ss_len_bytes, ct, &ct_len_bytes);

	// Replace secret key with random values
	get_random_number (&drng_algorithm, sk, KEM_SECRETKEYBYTES);

	// Alice uses Bobs response to get her shared key
	kem_dec (sk, sk_len_bytes, ct, ct_len_bytes, key_a, &ss_len_bytes);

	if (memcmp (key_a, key_b, KEM_SSBYTES) == 0)
		{
			printf ("ERROR: partial invalid sk allowed decaps\n");
			ret = 1;
		}

	// Replace secret key with random values
	get_random_number (&drng_algorithm, sk, KEM_SECRETKEYBYTES * 8);

	// Alice uses Bobs response to get her shared key
	kem_dec (sk, sk_len_bytes, ct, ct_len_bytes, key_a, &ss_len_bytes);

	if (memcmp (key_a, key_b, KEM_SSBYTES) == 0)
		{
			printf ("ERROR: full invalid sk allowed decaps\n");
			ret = 1;
		}

	return ret;
}

static int
test_invalid_ciphertext ()
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

	uint8_t b;
	size_t pos;

	do
		{
			get_random_number (&drng_algorithm, &b, 8);
		}
	while (!b);
	get_random_number (&drng_algorithm, (uint8_t *)&pos, sizeof (size_t) * 8);

	// Alice generates a public key
	kem_keygen (pk, &pk_len_bytes, sk, &sk_len_bytes);

	// Bob derives a secret key and creates a response
	kem_enc (pk, pk_len_bytes, key_b, &ss_len_bytes, ct, &ct_len_bytes);

	// Change some byte in the ciphertext (i.e., encapsulated key)
	ct[pos % KEM_CIPHERTEXTBYTES] ^= b;

	// Alice uses Bobs response to get her shared key
	kem_dec (sk, sk_len_bytes, ct, ct_len_bytes, key_a, &ss_len_bytes);

	if (memcmp (key_a, key_b, KEM_SSBYTES) == 0)
		{
			printf ("ERROR invalid ciphertext\n");
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
	init_random_number (&drng_algorithm, seed, SEED_LEN_BYTES);

	for (i = 0; i < NTESTS; i++)
		{
			r = test_keys ();
			r |= test_invalid_sk_a ();
			r |= test_invalid_ciphertext ();
			if (r)
				return 1;
		}

	printf ("KEM_SECRETKEYBYTES:  %d\n", KEM_SECRETKEYBYTES);
	printf ("KEM_PUBLICKEYBYTES:  %d\n", KEM_PUBLICKEYBYTES);
	printf ("KEM_CIPHERTEXTBYTES: %d\n", KEM_CIPHERTEXTBYTES);

	return 0;
}
