#include "api.h"
#include "cpucycles.h"
#include "drng.h"
#include "indcpa.h"
#include "kex.h"
#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "speed_print.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ntt.h"

#define NTESTS 1000
#define SEED_LEN_BYTES 64
DRNG_ctx drng_algorithm;

uint64_t t[NTESTS];
// uint8_t seed[KEM_SYMBYTES] = {0};
uint8_t seed[KEM_SYMBYTES]
		= { 3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8, 9, 7, 9, 3 };

int
main ()
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

	// Randomness
	// For generating the seed, we assume for testing purposes
	// memory is sufficiently random.
	DRNG_ctx drng_seed;
	unsigned char seed[SEED_LEN_BYTES];

	// Initialise the "randomly", using the drng_seed memory as the random ctx.
	get_random_number (&drng_seed, seed, SEED_LEN_BYTES * 8);

	// Initialise the proper random context for the runs.
	init_random_number (&drng_algorithm, seed, SEED_LEN_BYTES);

	for (int i = 0; i < NTESTS; i++)
		{
			t[i] = cpucycles ();
			kem_keygen (pk, &pk_len_bytes, sk, &sk_len_bytes);
		}
	print_results ("espada128_kem_keygen: ", t, NTESTS);

	for (int i = 0; i < NTESTS; i++)
		{
			t[i] = cpucycles ();
			kem_enc (pk, pk_len_bytes, key_b, &ss_len_bytes, ct, &ct_len_bytes);
		}
	print_results ("espada128_kem_enc: ", t, NTESTS);

	for (int i = 0; i < NTESTS; i++)
		{
			t[i] = cpucycles ();
			kem_dec (sk, sk_len_bytes, ct, ct_len_bytes, key_a, &ss_len_bytes);
		}
	print_results ("espada128_kem_dec: ", t, NTESTS);
	return 0;
}
