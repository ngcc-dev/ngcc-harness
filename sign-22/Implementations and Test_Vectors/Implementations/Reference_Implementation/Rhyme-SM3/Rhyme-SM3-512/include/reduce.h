#ifndef RHYME_REDUCE_H
#define RHYME_REDUCE_H
#include <stdint.h>
#include "params.h"

/* Montgomery: R = 2^32.  montgomery_reduce(a) = a * R^-1 mod q, |a| < 2^31 * q */
static inline int32_t montgomery_reduce(int64_t a) {
    int32_t t = (int32_t)((uint32_t)a * (uint32_t)RHYME_QINV);
    t = (int32_t)((a - (int64_t)t * Q) >> 32);
    return t;          /* in (-q, q) */
}

/* multiply + Montgomery reduce: returns a*b*R^-1 mod q */
static inline int32_t fqmul(int32_t a, int32_t b) {
    return montgomery_reduce((int64_t)a * b);
}

/* centered reduction to (-q/2, q/2] for any int32 */
static inline int32_t creduce(int32_t a) {
    int32_t t = a % Q;
    if (t > Q / 2) t -= Q;
    if (t < -(Q / 2)) t += Q;
    return t;
}

/* to [0, q) */
static inline int32_t freeze(int32_t a) {
    int32_t t = a % Q;
    if (t < 0) t += Q;
    return t;
}

/* to [0, 2q) */
static inline int32_t freeze2q(int32_t a) {
    int32_t t = a % DQ;
    if (t < 0) t += DQ;
    return t;
}
#endif
