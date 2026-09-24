#include <stdint.h>
#include "params.h"
#include "reduce.h"

/*************************************************
* Name:        montgomery_reduce
*
* Description: Montgomery reduction; given a 32-bit integer a, computes
*              16-bit integer congruent to a * R^-1 mod q, where R=2^16
*
* Arguments:   - int32_t a: input integer to be reduced;
*                           has to be in {-q2^15,...,q2^15-1}
*
* Returns:     integer in {-q+1,...,q-1} congruent to a * R^-1 modulo q.
**************************************************/
int16_t montgomery_reduce(int32_t a)
{
  int16_t t;

  t = (int16_t)a*QINV;
  t = (a - (int32_t)t*WEAVER_Q) >> 16;
  return t;
}

/*************************************************
* Name:        barrett_reduce
*
* Description: Barrett reduction; given a 16-bit integer a, computes
*              centered representative congruent to a mod 2q in {-q,...,q}
*
* Arguments:   - int16_t a: input integer to be reduced
*
* Returns:     integer in {-q,...,q} congruent to a modulo 2q.
**************************************************/
int16_t barrett_reduce(int16_t a) {
  int16_t t;
  const int16_t v = ((1<<26) + WEAVER_Q/2)/WEAVER_Q;

  t  = ((int32_t)v*a + (1<<25)) >> 26;
  t *= WEAVER_Q;
  return a - t;
}
/*************************************************
* Name:        barrett_reduce_ex
*
* Description: Barrett reduction; given a 16-bit integer a, computes
*              positive representative congruent to a mod+ q/2 in {0,...,q/2}
*
* Arguments:   - int16_t a: input integer to be reduced
*
* Returns:     integer in {0,...,q/2} congruent to a mod+ q/2.
**************************************************/
int16_t barrett_reduce_ex(int16_t a) {
  int16_t t, u;
  const int16_t v = ((1<<25) + WEAVER_Q/4)/WEAVER_HALFQ;

  t  = (int32_t)v*a >> 25;
  t *= WEAVER_HALFQ;
  u = a - t;
  u += ((int16_t)u >> 15) & WEAVER_HALFQ;
  return u;
}
