#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>
#include "params.h"

/* For power-of-2 q, reduction is bitmask & (q-1) */
#define montgomery_reduce(a) ((uint16_t)((a) & (PARAM_Q - 1)))
#define barrett_reduce(a)    ((uint16_t)((a) & (PARAM_Q - 1)))

#endif
