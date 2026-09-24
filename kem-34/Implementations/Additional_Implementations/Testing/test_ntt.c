#include "drng.h"
#include "ntt.h"
#include "poly.h"
#include "polyvec.h"
#include "reduce.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NTESTS 100
#define SEED_LEN_BYTES 64
DRNG_ctx drng_algorithm;

// Generate a random polynomial
static void
rand_poly (poly *res)
{
	uint16_t val;
	unsigned char rand[2];
	// Generate polynomial with coefficients mod Q
	for (int i = 0; i < KEM_N; i++)
		{
			get_random_number (&drng_algorithm, rand, 2 * 8);
			memcpy (&val, rand, 2);
			val = val % KEM_Q;
			res->coeffs[i] = val;
		}
}

// Generate a random polynomial vector
static void
rand_polyvec (polyvec *res)
{
	for (int i = 0; i < KEM_L; i++)
		{
			rand_poly (&res->vec[i]);
		}
}

// Do negacyclic convolution
static void
basic_negacyclic_convolution (poly *res, poly *g, poly *h)
{
	for (int k = 0; k < KEM_N; k++)
		{
			uint32_t c_k_1 = 0, c_k_2 = 0;
			res->coeffs[k] = 0;
			for (int i = 0; i < KEM_N; i++)
				{
					if (i <= k)
						{
							c_k_1 = (c_k_1 + (uint32_t)g->coeffs[i] * h->coeffs[k - i])
											% KEM_Q;
						}
					else
						{
							c_k_2
									= (c_k_2 + (uint32_t)g->coeffs[i] * h->coeffs[k + KEM_N - i])
										% KEM_Q;
						}
				}
			res->coeffs[k] = (uint16_t)((c_k_1 + KEM_Q) - c_k_2) % KEM_Q;
		}
}

// Multiply and accumulate two vectors of polynomials
static void
ncconv_acc (poly *res, polyvec *g, polyvec *h)
{
	for (int i = 0; i < KEM_L; i++)
		{
			poly res_i;
			// Multiply two polynomials at i
			basic_negacyclic_convolution (&res_i, &g->vec[i], &h->vec[i]);
			// Accumulate in res
			poly_add (res, res, &res_i);
			poly_freeze (res);
		}
}

static int
test_ntt_conversion ()
{
	poly v, ntt_v, inv_v;

	for (int t = 0; t < NTESTS; t++)
		{
			rand_poly (&v);
			memcpy (&inv_v, &v, sizeof (poly));

			// Transform using ntt
			ntt (inv_v.coeffs);

			// Save ntt version for debug
			memcpy (&ntt_v, &inv_v, sizeof (poly));

			// Reverse using intt
			invntt (inv_v.coeffs);

			// freeze?
			for (int i = 0; i < KEM_N; i++)
				{
					inv_v.coeffs[i] = freeze (inv_v.coeffs[i]);
				}

			// Check
			if (memcmp (&v, &inv_v, sizeof (poly)) != 0)
				{
					printf (
							"ERROR: NTT/INTT fails on generated polynomial! Test %d/%d\n", t,
							NTESTS);
					for (int i = 0; i < KEM_N; i++)
						{
							if (v.coeffs[i] != inv_v.coeffs[i])
								{
									printf ("ERROR: Mismatch at coefficient %d.\n\tOriginal "
													"Value: %d\n\tNTT Value: %d\n\tINVNTT Value: %d\n",
													i, v.coeffs[i], ntt_v.coeffs[i], inv_v.coeffs[i]);
								}
						}
					return 1;
				}
		}

	return 0;
}

static int
test_ntt_arithmetic ()
{
	poly poly_a, poly_b, poly_res, poly_res_calc;

	// Zero out res
	memset (&poly_res, 0, sizeof (poly));
	memset (&poly_res_calc, 0, sizeof (poly));

	// Generate random polynomials
	rand_poly (&poly_a);
	rand_poly (&poly_b);

	// Calculate desired answer
	basic_negacyclic_convolution (&poly_res, &poly_a, &poly_b);

	// Transform using ntt
	ntt (poly_a.coeffs);
	ntt (poly_b.coeffs);

	// Multiply
	poly_basemul_montgomery (&poly_res_calc, &poly_a, &poly_b);

	// Reverse using intt
	invntt (poly_res_calc.coeffs);

	// freeze?
	for (int i = 0; i < KEM_N; i++)
		{
			poly_res_calc.coeffs[i] = freeze (poly_res_calc.coeffs[i]);
		}

	if (memcmp (&poly_res_calc, &poly_res, sizeof (poly)) != 0)
		{
			printf ("ERROR: Polynomial multiplication in NTT form failed\n");
			for (int i = 0; i < KEM_N; i++)
				{
					if (poly_res_calc.coeffs[i] != poly_res.coeffs[i])
						{
							printf ("ERROR: Mismatch at coefficient %d.\n\tOriginal "
											"poly_res: %d\n\tpoly_res_calc: %d\n",
											i, poly_res.coeffs[i], poly_res_calc.coeffs[i]);
						}
				}
			return 1;
		}

	polyvec polyvec_a, polyvec_b;

	// Zero out res
	memset (&poly_res, 0, sizeof (poly));
	memset (&poly_res_calc, 0, sizeof (poly));

	// Generate random polyvecs
	rand_polyvec (&polyvec_a);
	rand_polyvec (&polyvec_b);

	// Calculate desired answer
	ncconv_acc (&poly_res, &polyvec_a, &polyvec_b);

	// Transform using ntt
	polyvec_ntt (&polyvec_a);
	polyvec_ntt (&polyvec_b);

	// Multiply
	polyvec_basemul_acc_montgomery (&poly_res_calc, &polyvec_a, &polyvec_b);

	// Reverse using intt
	invntt (poly_res_calc.coeffs);

	// freeze?
	for (int i = 0; i < KEM_N; i++)
		{
			poly_res_calc.coeffs[i] = freeze (poly_res_calc.coeffs[i]);
		}

	if (memcmp (&poly_res_calc, &poly_res, sizeof (poly)) != 0)
		{
			printf ("ERROR: Polynomial vector multiplication and accumulation in "
							"NTT form failed\n");
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

	// Initialise the "randomly", using the drng_seed memory as the random
	// ctx.
	get_random_number (&drng_seed, seed, SEED_LEN_BYTES * 8);

	// Initialise the proper random context for the runs.
	init_random_number (&drng_algorithm, seed, SEED_LEN_BYTES);

	// Test ntt
	ret |= test_ntt_conversion ();
	ret |= test_ntt_arithmetic ();

	printf ("KEM_SECRETKEYBYTES:  %d\n", KEM_SECRETKEYBYTES);
	printf ("KEM_PUBLICKEYBYTES:  %d\n", KEM_PUBLICKEYBYTES);
	printf ("KEM_CIPHERTEXTBYTES: %d\n", KEM_CIPHERTEXTBYTES);

	return ret;
}
