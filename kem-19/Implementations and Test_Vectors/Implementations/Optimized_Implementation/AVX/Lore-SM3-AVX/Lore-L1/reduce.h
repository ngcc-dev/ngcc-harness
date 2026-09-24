#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

#define montgomery_reduce LORE_NAMESPACE(montgomery_reduce)
static inline int16_t montgomery_reduce(int64_t a) {
    int32_t t;
    int16_t v;
    v = (int16_t)(a * (-255));
    t = (int64_t)v * LORE_Q;
    t = (int32_t)(a - t);
    t >>= 16;
    return (int16_t)t;
}

#define barrett_reduce LORE_NAMESPACE(barrett_reduce)
static inline int16_t barrett_reduce(int16_t a) {
    int16_t t = a;
    t = (t & 0xFF) - (t >> 8);
    t = (t & 0xFF) - (t >> 8);
    return (int16_t)t;
}

#define poly_reduce LORE_NAMESPACE(poly_reduce)
void poly_reduce(poly *r);

#endif