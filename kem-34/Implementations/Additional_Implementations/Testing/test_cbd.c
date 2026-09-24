#include "cbd.h"
#include "drng.h"
#include "params.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NTESTS 1
#define SEED_LEN_BYTES 64
DRNG_ctx drng_algorithm;

static void
bytes_to_bits (uint8_t *out_arr, uint8_t *arr, size_t l)
{
	for (size_t i = 0; i < l; i++)
		{
			for (int j = 0; j < 8; j++)
				{
					out_arr[8 * i + j] = arr[i] & 1;
					arr[i] >>= 1;
				}
		}
}

static void
basic_cbd_eta (poly *r, const unsigned char *buf)
{
	uint8_t bit_r[8 * KEM_N * KEM_ETA];
	bytes_to_bits (bit_r, (uint8_t *)buf, 64 * KEM_ETA);
	for (int i = 0; i < KEM_N; i++)
		{
			int x = 0;
			int y = 0;
			for (int j = 0; j < KEM_ETA; j++)
				{
					x += bit_r[2 * i * KEM_ETA + j];
					y += bit_r[2 * i * KEM_ETA + KEM_ETA + j];
				}
			r->coeffs[i] = ((x - y) % KEM_Q) + KEM_Q;
		}
}

int
test_cbd ()
{
	poly p, true_poly;
	uint8_t buf[KEM_ETA * KEM_N];

	get_random_number (&drng_algorithm, buf, (KEM_ETA * KEM_N) * 8);

	poly_cbd_eta (&p, buf);

	basic_cbd_eta (&true_poly, buf);

	if (memcmp (p.coeffs, true_poly.coeffs, KEM_N * sizeof (uint16_t)) != 0)
		{
			printf ("ERROR: test_cbd fails to generate according to spec\n");
			for (int i = 0; i < KEM_N; i++)
				{
					if (p.coeffs[i] != true_poly.coeffs[i])
						{
							printf (
									"\tcoefficient %d doesn't match: given %d, spec value %d\n",
									i, p.coeffs[i], true_poly.coeffs[i]);
						}
				}
			return 1;
		}
	return 0;
}

int
main (void)
{
	int ret = 0;

	// Randomness
	// For generating the seed, we assume for testing purposes
	// memory is sufficiently random.
	DRNG_ctx drng_seed;
	unsigned char seed[SEED_LEN_BYTES];

	// Initialise the "randomly", using the drng_seed memory as the random ctx.
	get_random_number (&drng_seed, seed, SEED_LEN_BYTES * 8);

	// Init drng_rand
	init_random_number (&drng_algorithm, seed, SEED_LEN_BYTES);

	// Check packing and compression
	for (int i = 0; i < NTESTS; i++)
		{
			ret |= test_cbd ();
		}

	printf ("KEM_SECRETKEYBYTES:  %d\n", KEM_SECRETKEYBYTES);
	printf ("KEM_PUBLICKEYBYTES:  %d\n", KEM_PUBLICKEYBYTES);
	printf ("KEM_CIPHERTEXTBYTES: %d\n", KEM_CIPHERTEXTBYTES);

	return ret;
}
