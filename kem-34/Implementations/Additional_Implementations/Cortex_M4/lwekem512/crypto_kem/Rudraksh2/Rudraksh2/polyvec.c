#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "reduce.h"
#include <stdint.h>
#include <stdio.h>


/*************************************************
 * Name:        polyvec_tobytes
 *
 * Description: Serialize vector of polynomials
 *
 * Arguments:   - uint8_t *r: pointer to output byte array
 *                            (needs space for KEM_POLYVECBYTES)
 *              - const polyvec *a: pointer to input vector of polynomials
 **************************************************/
void
polyvec_tobytes (uint8_t r[KEM_POLYVECBYTES], const polyvec *a)
{
	unsigned int i;
	for (i = 0; i < KEM_L; i++)
		poly_tobytes (r + i * KEM_POLYBYTES, &a->vec[i]);
}

/*************************************************
 * Name:        polyvec_frombytes
 *
 * Description: De-serialize vector of polynomials;
 *              inverse of polyvec_tobytes
 *
 * Arguments:   - uint8_t *r:       pointer to output byte array
 *              - const polyvec *a: pointer to input vector of polynomials
 *                                  (of length KEM_POLYVECBYTES)
 **************************************************/
void
polyvec_frombytes (polyvec *r, const uint8_t a[KEM_POLYVECBYTES])
{
	unsigned int i;
	for (i = 0; i < KEM_L; i++)
		poly_frombytes (&r->vec[i], a + i * KEM_POLYBYTES);
}

/*************************************************
 * Name:        polyvec_ntt
 *
 * Description: Apply forward NTT to all elements of a vector of polynomials
 *
 * Arguments:   - polyvec *r: pointer to in/output vector of polynomials
 **************************************************/
void
polyvec_ntt (polyvec *r)
{
	unsigned int i;
	for (i = 0; i < KEM_L; i++)
		{
			// Convert to mont
			// poly_tomont (&r->vec[i]);
			poly_ntt (&r->vec[i]);
		}
}

/*************************************************
 * Name:        polyvec_invntt_tomont
 *
 * Description: Apply inverse NTT to all elements of a vector of polynomials
 *              and multiply by Montgomery factor 2^16
 *
 * Arguments:   - polyvec *r: pointer to in/output vector of polynomials
 **************************************************/
void
polyvec_invntt_tomont (polyvec *r)
{
	unsigned int i;
	for (i = 0; i < KEM_L; i++)
		poly_invntt_tomont (&r->vec[i]);
}

/*************************************************
 * Name:        polyvec_basemul_acc_montgomery
 *
 * Description: Multiply elements of a and b in NTT domain, accumulate into r,
 *              and multiply by 2^-16.
 *
 * Input Domain: NTT+Montgomery
 *
 * Output Domain: NTT+Montgomery
 *
 * Arguments: - poly *r: pointer to output polynomial
 *            - const polyvec *a: pointer to first input vector of polynomials
 *            - const polyvec *b: pointer to second input vector of polynomials
 **************************************************/
void
polyvec_basemul_acc_montgomery (poly *r, const polyvec *a, const polyvec *b)
{
	int i, j;
	uint16_t t;
	for (j = 0; j < KEM_N; j++)
		{
			t = montgomery_reduce ((uint32_t)b->vec[0].coeffs[j] * MONT_2);
			r->coeffs[j] = montgomery_reduce ((uint32_t)a->vec[0].coeffs[j] * t);
			for (i = 1; i < KEM_L; i++)
				{
					t = montgomery_reduce ((uint32_t)b->vec[i].coeffs[j] * MONT_2);
					r->coeffs[j] += montgomery_reduce ((uint32_t)a->vec[i].coeffs[j] * t);
					r->coeffs[j] = barrett_reduce (r->coeffs[j]);
				}
		}
}

/*************************************************
 * Name:        polyvec_reduce
 *
 * Description: Applies Barrett reduction to each coefficient
 *              of each element of a vector of polynomials;
 *              for details of the Barrett reduction see comments in reduce.c
 *
 * Arguments:   - polyvec *r: pointer to input/output polynomial
 **************************************************/
void
polyvec_reduce (polyvec *r)
{
	unsigned int i;
	for (i = 0; i < KEM_L; i++)
		poly_reduce (&r->vec[i]);
}

/*************************************************
 * Name:        polyvec_add
 *
 * Description: Add vectors of polynomials
 *
 * Arguments: - polyvec *r: pointer to output vector of polynomials
 *            - const polyvec *a: pointer to first input vector of polynomials
 *            - const polyvec *b: pointer to second input vector of polynomials
 **************************************************/
void
polyvec_add (polyvec *r, const polyvec *a, const polyvec *b)
{
	unsigned int i;
	for (i = 0; i < KEM_L; i++)
		poly_add (&r->vec[i], &a->vec[i], &b->vec[i]);
}
