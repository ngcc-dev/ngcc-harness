#ifndef SIGN_REDUCE_H
#define SIGN_REDUCE_H

#include "params.h"
#include <stdint.h>

#if DARTS_MODE == 128 || DARTS_MODE == 256

#define MONT 114369      // 2^32 mod q
#define MONTSQ 7148      // 2^64 mod q
#define QINV 4244570369  // q^(-1) mod 2^32
#define QREC 32831       // 2^32 // Q for Barrett
#define DQREC 16415      // 2^32 // DQ for Barrett

#elif DARTS_MODE == 512

#define MONT  130976     // 2^32 mod q
#define MONTSQ 125151    // 2^64 mod q
#define QINV 2820670977  // q^(-1) mod 2^32
#define QREC 16480       // 2^32 // Q for Barrett
#define DQREC 8240       // 2^32 // DQ for Barrett

#endif  

#define montgomery_reduce DARTS_NAMESPACE(montgomery_reduce)
int32_t montgomery_reduce(int64_t a);

#define caddq DARTS_NAMESPACE(caddq)
int32_t caddq(int32_t a);

#define freeze DARTS_NAMESPACE(freeze)
int32_t freeze(int32_t a);

#define freeze_centered DARTS_NAMESPACE(freeze_centered)
int32_t freeze_centered(int32_t a);

#endif