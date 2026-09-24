#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>
#include "params.h"
#if (Q==8380417)
#define MONT -4186625 // 2^32 % Q
#define QINV 58728449 // q^(-1) mod 2^32
#elif (Q==2081281)
#define MONT -796688 // 2^32 % Q
#define QINV -2031862271 // q^(-1) mod 2^32
#define BARRETT_MU 2063 // floor(2^32 / Q)
#endif
#define montgomery_reduce COMPASS_SIG_NAMESPACE(montgomery_reduce)
int32_t montgomery_reduce(int64_t a);

#define reduce32 COMPASS_SIG_NAMESPACE(reduce32)
int32_t reduce32(int32_t a);

#define caddq COMPASS_SIG_NAMESPACE(caddq)
int32_t caddq(int32_t a);

#define freeze COMPASS_SIG_NAMESPACE(freeze)
int32_t freeze(int32_t a);

#endif
