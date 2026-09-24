#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>
#include "params.h"

#if WEAVER_N == 128
// q = 3329
#define MONT -1044 // 2^16 mod q
#define QINV -3327 // q^-1 mod 2^16

#elif WEAVER_N == 256 || WEAVER_N == 512
// q = 7681
#define MONT -3593  // 2^16 mod q
#define QINV -7679 // q^-1 mod 2^16
#endif

#define montgomery_reduce WEAVER_NAMESPACE(_montgomery_reduce)
int16_t montgomery_reduce(int32_t a);

#define barrett_reduce WEAVER_NAMESPACE(_barrett_reduce)
int16_t barrett_reduce(int16_t a);

#define barrett_reduce_ex WEAVER_NAMESPACE(_barrett_reduce_ex)
int16_t barrett_reduce_ex(int16_t a);

#endif
