#include "params.h"
#include "reduce.h"
#include <stdint.h>
#include <stdio.h>

/*************************************************
 * Name:        montgomery_reduce
 *
 * Description: Montgomery reduction; given a 32-bit integer a, computes
 *              16-bit unsigned integer congruent to a * R^-1 mod q, where
 * R=2^18
 *
 * Arguments:   - uint32_t a: input integer to be reduced;
 *                           has to be in {q2^17,...,q2^18-1}
 *
 * Returns:     integer in {-q+1,...,q-1} congruent to a * R^-1 modulo q.
 **************************************************/
uint16_t
montgomery_reduce (uint32_t a)
{
	uint32_t u;

	u = (a * QINV);
	u &= ((1 << RLOG) - 1);
	u *= KEM_Q;
	a = a + u;
	return a >> RLOG;
}

// https://en.wikipedia.org/wiki/Barrett_reduction
/*************************************************
 * Name:        barrett_reduce
 *
 * Description: Barrett reduction; given a 16-bit integer a, computes at most
 * 14 bits centered representative congruent to a mod q in {0,...,2q}
 *
 * Arguments:   - uint16_t a: input integer to be reduced
 *
 * Returns:     integer in {0,...,q-1} congruent to a modulo q.
 **************************************************/
uint16_t
barrett_reduce (uint16_t a)
{
	uint32_t u;
	u = (a * 19) >> 16;
	u *= KEM_Q;
	a = a - u;
	return a;
}

/*************************************************
 * Name:        freeze
 *
 * Description: Full reduction; given a 16-bit integer a, computes
 *              unsigned integer a mod q.
 *
 * Arguments:   - uint16_t x: input unsigned integer to be reduced
 *
 * Returns:     unsigned integer in {0,...,q-1} congruent to a modulo q.
 **************************************************/
uint16_t
freeze (uint16_t x)
{
	uint16_t m, r;
	int16_t c;
	r = barrett_reduce (x);

	m = r - KEM_Q;
	c = m;
	c >>= 15;
	r = m ^ ((r ^ m) & c);

	return r;
}
