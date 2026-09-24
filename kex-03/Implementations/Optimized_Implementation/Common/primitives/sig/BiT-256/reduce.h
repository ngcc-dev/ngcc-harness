/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
*/
#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>
#include "params.h"

/* ---- fast: a in [0, 2q) or [-q, q)+q, result in [0, q) --------------- */
static inline int32_t reduce_modq(int32_t a) {
    a -= BIT_Q;
    a += (a >> 31) & BIT_Q;
    return a;
}

/* ---- Barrett: arbitrary a, result in [0, q) -------------------------- */
uint32_t reduce_barrett(int64_t a);

/* ---- fast: |a| < 2q, result in [-q/2, q/2) ------------------------- */
static inline int32_t reduce_signed_modq(int64_t a) {
    a += BIT_Q & ((a + BIT_Q_HALF) >> 63);
    a -= BIT_Q & ((BIT_Q_HALF - a) >> 63);
    return (int32_t)a;
}

/* ---- format conversion ---------------------------------------------- */
static inline uint32_t reduce_to_unsigned(int32_t c) {
    return (uint32_t)(c + ((c >> 31) & BIT_Q));
}

static inline int32_t reduce_to_signed(uint32_t t) {
    int64_t t64 = t;
    int64_t mask = ((BIT_Q / 2) - t64) >> 63;
    return (int32_t)(t64 - (BIT_Q & mask));
}

/* ---- Barrett: arbitrary a, result in [-q/2, q/2) -------------------- */
static inline int32_t reduce_signed_barrett(int64_t a) {
    return reduce_to_signed(reduce_barrett(a));
}

#endif
