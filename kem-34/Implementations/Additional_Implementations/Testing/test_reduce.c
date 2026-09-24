#include "drng.h"
#include "reduce.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NTESTS 100
#define SEED_LEN_BYTES 64
DRNG_ctx drng_algorithm;

static int
test_barrett_reduce ()
{
	printf ("--- BARRETT REDUCTION TESTS ---\n");
	// Test all the basic barrett reductions
	// The barrett should just bring it down to 13 bits
	uint16_t real_val = 1;
	for (uint16_t red_val = 1; red_val < UINT16_MAX; red_val++)
		{
			uint16_t b_val = barrett_reduce (red_val);
			// Check bit length
			// if ((b_val & mask) != 0)
			if (b_val > 2 * KEM_Q)
				{
					printf ("ERROR: Barrett reduction bit reduction failed for %d, "
									"expected %d got %d\n",
									red_val, real_val, b_val);
					return -1;
				}
			// Check freeze
			uint16_t red_b_val = b_val % KEM_Q;
			b_val = freeze (b_val);
			if (b_val != red_b_val || b_val != real_val)
				{
					printf ("ERROR: Freeze reduction modulus failed for %d, got freeze "
									"val %d, reduction val %d, expected %d\n",
									red_val, b_val, red_b_val, real_val);
					return -1;
				}
			real_val++;
			if (real_val == KEM_Q)
				real_val = 0;
		}
	printf ("PASSED BARRETT REDUCTION TESTS\n");
	return 0;
}

static int
test_montgomery_reduce ()
{
	printf ("--- MONTGOMERY REDUCTION TESTS ---\n");
	uint32_t max = KEM_Q * (1 << RLOG);
	// Test montgomery reduction
	for (int i = 0; i < NTESTS; i++)
		{
			uint16_t a, b;
			uint8_t rand[4] = { 0 };
			get_random_number (&drng_algorithm, rand, 4 * 8);
			memcpy (&a, rand, 2);
			memcpy (&b, &rand[2], 2);
			// This is a limit of montgomery
			if (a > max || b > max)
				continue;
			uint16_t prod = ((uint32_t)a * b) % KEM_Q;

			// Convert to montgomery form
			uint16_t mont_form_conv = (1ULL << 2 * RLOG) % KEM_Q;
			uint16_t mont_form_a = montgomery_reduce ((uint32_t)a * mont_form_conv);
			uint16_t mont_form_b = montgomery_reduce ((uint32_t)b * mont_form_conv);
			// Montgomery multiplication
			uint16_t montgomery_prod
					= montgomery_reduce ((uint32_t)mont_form_a * mont_form_b);
			// Out of montgomery form
			// uint32_t f = (montgomery_prod << RLOG) % KEM_Q;
			montgomery_prod = montgomery_reduce ((uint32_t)montgomery_prod);

			// Because the output of montgomery_reduce can be in the range 0...2Q
			montgomery_prod = freeze (montgomery_prod);

			if (prod != montgomery_prod)
				{
					printf ("ERROR: Montgomery product does not match correct product. "
									"Got %d, expected %d\n",
									montgomery_prod, prod);
					return -1;
				}
		}

	printf ("PASSED MONTGOMERY REDUCTION TESTS\n");
	return 0;
}

int
main (void)
{
	DRNG_ctx drng_seed;
	unsigned char seed[SEED_LEN_BYTES];
	int ret = 0;
	// Initialise the "randomly", using the drng_seed memory as the random ctx.
	get_random_number (&drng_seed, seed, SEED_LEN_BYTES * 8);

	// Initialise the proper random context for the runs.
	init_random_number (&drng_algorithm, seed, SEED_LEN_BYTES);

	ret |= test_barrett_reduce ();
	ret |= test_montgomery_reduce ();

	return ret;
}
