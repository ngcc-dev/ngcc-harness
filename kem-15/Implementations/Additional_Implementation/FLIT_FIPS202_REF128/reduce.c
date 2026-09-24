#include <stdint.h>
#include "params.h"
#include "reduce.h"

/*************************************************
* Name:        montgomery_reduce
*
* Description: Montgomery reduction; given a 32-bit integer a, computes
*              16-bit integer congruent to a * R^-1 mod q,
*              where R=2^16
*
* Arguments:   - int32_t a: input integer to be reduced;
*                           has to be in {-q2^15,...,q2^15-1}
*
* Returns:     integer in {-q+1,...,q-1} congruent to a * R^-1 modulo q.
**************************************************/
int16_t montgomery_reduce(int32_t a)
{
    int32_t t;
    int16_t u;

    u = a * QINV;
    t = (int32_t)u * Q;
    t = a - t;
    t >>= 16;
    return t;
}

/*************************************************
 * Name:        freeze
 *
 * Description: For finite field element a, compute standard
 *              representative r = a mod^+ Q.
 *
 * Arguments:   - int16_t: finite field element a
 *
 * Returns r.
 **************************************************/
int16_t freeze(int16_t a) {
    int32_t t = (int32_t)a * QREC;
    t = t >> 16;
    t = a - t * Q;             // -2Q <  t < 2Q
    t += (t >> 15) & DQ;       //   0 <= t < 2Q
    t -= ~((t - Q) >> 15) & Q; //   0 <= t < Q
    return t;
}

/*************************************************
 * Name:        freeze_centered
 *
 * Description: For finite field element a, compute centered
 *              representative r = a mod^± Q.
 *
 * Arguments:   - int16_t: finite field element a
 *
 * Returns r.
 **************************************************/
int16_t freeze_centered(int16_t a) {
    int16_t t = freeze(a);        //        0 <= t < Q
    int16_t b = t - (Q >> 1);     //             t > (Q-1)/2 ?
    t -= Q & -(b > 0);            // -(Q-1)/2 <= t <= (Q-1)/2
    return t;
}