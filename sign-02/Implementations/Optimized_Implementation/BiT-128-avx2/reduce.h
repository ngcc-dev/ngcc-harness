/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
*/
#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>
#include "params.h"

/* ---------------------------------------------------------------
   Mod-q reduction and format conversion — consolidated interface.

     reduce_modq           fast, requires a < 3q        → int16_t  [0, q)
     reduce_barrett        Barrett, arbitrary int32     → uint16_t [0, q)
     reduce_signed_modq     fast, requires |a| < 2q     → int16_t  [-q/2, q/2)
     reduce_signed_barrett Barrett, arbitrary int32     → int16_t  [-q/2, q/2)
     reduce_to_unsigned     signed → unsigned           → uint16_t [0, q)
     reduce_to_signed       unsigned → signed           → int16_t  [-q/2, q/2)
   --------------------------------------------------------------- */

/* ---- fast: a < 3q, result in [0, q) --------------------------------- */
static inline int16_t reduce_modq(int32_t a) {
    a -= BIT_Q;
    a += (a >> 31) & BIT_Q;
    return (int16_t)a;
}

/* ---- Barrett: arbitrary a, result in [0, q) -------------------------- */
uint16_t reduce_barrett(int32_t a);

/* ---- fast: |a| < 2q, result in [-q/2, q/2) ------------------------- */
static inline int16_t reduce_signed_modq(int32_t a) {
    a += BIT_Q & ((a + BIT_Q_HALF) >> 31);
    a -= BIT_Q & ((BIT_Q_HALF - a) >> 31);
    return (int16_t)a;
}

/* ---- format conversion ---------------------------------------------- */
static inline uint16_t reduce_to_unsigned(int16_t c) {
    return (uint16_t)(c + ((c >> 15) & BIT_Q));
}

static inline int16_t reduce_to_signed(uint16_t t) {
    int32_t t32 = t;
    int32_t mask = ((BIT_Q / 2) - t32) >> 31;
    return (int16_t)(t32 - (BIT_Q & mask));
}

/* ---- Barrett: arbitrary a, result in [-q/2, q/2) -------------------- */
static inline int16_t reduce_signed_barrett(int32_t a) {
    return reduce_to_signed(reduce_barrett(a));
}

#endif
