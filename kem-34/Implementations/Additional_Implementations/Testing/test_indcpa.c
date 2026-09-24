#include "drng.h"
#include "indcpa.h"
#include "symmetric.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NTESTS 100
#define SEED_LEN_BYTES 64
DRNG_ctx drng_algorithm;

static int
test_encrypt (uint8_t pk[KEM_INDCPA_PUBLICKEYBYTES],
							uint8_t sk[KEM_INDCPA_SECRETKEYBYTES])
{
	uint8_t m[KEM_INDCPA_MSGBYTES];
	uint8_t dec_m[KEM_INDCPA_MSGBYTES];
	uint8_t c[KEM_INDCPA_BYTES];
	uint8_t ret = 0;
	uint8_t buf[2 * KEM_SYMBYTES];
	/* Will contain key, coins */
	uint8_t kr[2 * KEM_SYMBYTES];

	// Generate random message
	for (int i = 0; i < KEM_INDCPA_MSGBYTES; i++)
		{
			get_random_number (&drng_algorithm, &m[i], 8);
		}

	// Generate coins
	get_random_number (&drng_algorithm, buf, KEM_SYMBYTES * 8);

	/* Multitarget countermeasure for coins + contributory KEM */
	hash_h (buf + KEM_SYMBYTES, pk, KEM_PUBLICKEYBYTES);

	hash_g (kr, buf, 2 * KEM_SYMBYTES);

	// Encrypt message
	indcpa_enc (c, m, pk, kr + KEM_SYMBYTES);

	// Decrypt message
	indcpa_dec (dec_m, c, sk);

	if (memcmp (m, dec_m, KEM_INDCPA_MSGBYTES) != 0)
		{
			printf ("ERROR: Message did not decrypt correctly\n");
			ret = 1;
		}
	//for (int i = 0; i < KEM_INDCPA_MSGBYTES; i++)
	//{
	//	printf ("m[%d] = %02x, dec_m[%d] = %02x\n", i, m[i], i, dec_m[i]);
	//}
	

	return ret;
}

int
main (void)
{
	unsigned int i;
	int r = 0;
	unsigned char pk[KEM_INDCPA_PUBLICKEYBYTES];
	unsigned char sk[KEM_INDCPA_SECRETKEYBYTES];

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
			indcpa_keypair (pk, sk);
			r |= test_encrypt (pk, sk);
			if (r)
				return 1;
		}

	printf ("KEM_SECRETKEYBYTES:  %d\n", KEM_SECRETKEYBYTES);
	printf ("KEM_PUBLICKEYBYTES:  %d\n", KEM_PUBLICKEYBYTES);
	printf ("KEM_CIPHERTEXTBYTES: %d\n", KEM_CIPHERTEXTBYTES);

	return 0;
}
