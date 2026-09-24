#include "parameters.h"
#include "reduce.h"
#include <stdint.h>

int16_t DKE_montgomery_reduce(int32_t a) {
    int16_t t;
    t = (int16_t)a * DKE_QINV;
    t = (a - (int32_t)t * DKE_Q) >> 16;
    return t;
}

int16_t DKE_barrett_reduce(int16_t a) {
    int16_t t;
    const int16_t v = (int16_t)(((1 << 26) + DKE_Q / 2) / DKE_Q);
    t  = (int16_t)(((int32_t)v * a + (1 << 25)) >> 26);
    t *= DKE_Q;
    return a - t;
}
