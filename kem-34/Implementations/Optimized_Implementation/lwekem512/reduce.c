#include "params.h"
#include "reduce.h"
#include <assert.h>
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
 * Returns:     integer in {0,...,2q-1} congruent to a * R^-1 modulo q.
 **************************************************/
uint16_t
montgomery_reduce (uint32_t a)
{
	assert( a < (KEM_Q << RLOG) );
	uint32_t u;

	u = (a * QINV);
	u &= ((1 << RLOG) - 1);
	u *= KEM_Q;
	a = a + u;
	return a >> RLOG;
}

// uint16_t
// montgomery_reduce(uint32_t a)
// {

//     uint32_t u = (a * QINV) & ((1u << RLOG) - 1u);

//     uint32_t t = (uint32_t)(((uint64_t)a + (uint64_t)u * KEM_Q) >> RLOG);

//     // t = (t < KEM_Q) ? t : t - KEM_Q;
//     // For constant time:
//     uint32_t r = t - KEM_Q;
//     uint32_t mask = -(r >> 31);
//     t = r + (KEM_Q & mask);

//     return (uint16_t)t;
// }

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
	// a = (a % 7681);
	// if (a < 0) a += 7681;
	// return a;

	uint32_t u;
	u = (a * KEM_2K_Q) >> 16;
	u *= KEM_Q;
	a = a - u;
	assert (a < (KEM_Q << 1));
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
