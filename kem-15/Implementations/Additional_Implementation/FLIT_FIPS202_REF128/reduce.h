#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>
#include "params.h"

#if KEM_MODE == 128 || KEM_MODE == 256
#define MONT 171 // 2^16 mod q
#define MONTSQ 19 // 2^32 mod q
#define QINV 64769 // q^-1 mod 2^16
#define QREC 85 // 2^16 // q
#elif KEM_MODE == 512
#define MONT 2285 // 2^16 mod q
#define MONTSQ 1353 // 2^32 mod q
#define QINV 62209 // q^-1 mod 2^16
#define QREC 19 // 2^16 // q
#endif

#define montgomery_reduce KEM_NAMESPACE(montgomery_reduce)
int16_t montgomery_reduce(int32_t a);

#define freeze KEM_NAMESPACE(freeze)
int16_t freeze(int16_t a);

#define freeze_centered KEM_NAMESPACE(freeze_centered)
int16_t freeze_centered(int16_t a);

#endif