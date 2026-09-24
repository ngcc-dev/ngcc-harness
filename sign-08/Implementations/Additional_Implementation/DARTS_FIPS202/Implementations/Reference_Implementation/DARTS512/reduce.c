#include "reduce.h"
#include "params.h"
#include <stdint.h>

/*************************************************
 * Name:        montgomery_reduce
 *
 * Description: For finite field element a with -2^{31}Q <= a <= Q*2^31,
 *              compute r \equiv a*2^{-32} (mod Q) such that -Q < r < Q.
 *
 * Arguments:   - int64_t: finite field element a
 *
 * Returns r.
 **************************************************/
int32_t montgomery_reduce(int64_t a) {
    int32_t t;

    t = (int64_t)(int32_t)a * QINV;
    t = (a - (int64_t)t * Q) >> 32;
    return t;
}

/*************************************************
 * Name:        caddq
 *
 * Description: Add Q if input coefficient is negative.
 *
 * Arguments:   - int32_t: finite field element a
 *
 * Returns r.
 **************************************************/
int32_t caddq(int32_t a) {
    a += (a >> 31) & Q;
    return a;
}

/*************************************************
 * Name:        freeze
 *
 * Description: For finite field element a, compute standard
 *              representative r = a mod^+ Q.
 *
 * Arguments:   - int32_t: finite field element a
 *
 * Returns r.
 **************************************************/
int32_t freeze(int32_t a) {
    int64_t t = (int64_t)a * QREC;
    t = t >> 32;
    t = a - t * Q;             // -2Q <  t < 2Q
    t += (t >> 31) & DQ;       //   0 <= t < 2Q
    t -= ~((t - Q) >> 31) & Q; //   0 <= t < Q
    return t;
}

/*************************************************
 * Name:        freeze_centered
 *
 * Description: For finite field element a, compute centered
 *              representative r = a mod^± Q.
 *
 * Arguments:   - int32_t: finite field element a
 *
 * Returns r.
 **************************************************/
int32_t freeze_centered(int32_t a) {
    int32_t t = freeze(a);        //        0 <= t < Q
    int32_t b = t - (Q >> 1);     //             t > (Q-1)/2 ?
    t -= Q & -(b > 0);            // -(Q-1)/2 <= t <= (Q-1)/2
    return t;
}
