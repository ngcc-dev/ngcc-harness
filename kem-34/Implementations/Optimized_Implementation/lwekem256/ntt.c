#include "ntt.h"
#include "params.h"
#include "reduce.h"
#include <stdint.h>
#include <stdio.h>

/*************************************************
 * Name:        ntt
 *
 * Description: Computes negacyclic number-theoretic transform (NTT) of
 *              a polynomial (vector of 64 coefficients) in place;
 *              inputs assumed to be in normal order, output in bitreversed
 * order
 *
 * Arguments:   - uint16_t *p: pointer to in/output polynomial
 **************************************************/
void
ntt (uint16_t p[KEM_N])
{
	ntt_avx(p, qdata);
}

/*************************************************
 * Name:        invntt
 *
 * Description: Computes inverse of negacyclic number-theoretic transform (NTT)
 * of a polynomial (vector of 64 coefficients) in place; inputs assumed to be
 * in bitreversed order, output in normal order
 *
 * Arguments:   - uint16_t *a: pointer to in/output polynomial
 **************************************************/
void
invntt (uint16_t a[KEM_N])
{
	invntt_avx(a, qdata);
}
