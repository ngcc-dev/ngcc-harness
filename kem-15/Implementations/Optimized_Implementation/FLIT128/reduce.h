#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>
#include "params.h"

#if KEM_MODE == 512
#define MONT 2285 // 2^16 mod 3329
#define MONTSQ 1353 // 2^32 mod 3329
#define MONTSQINV 1929 // 2^-32 mod 3329
#define QINV 62209 // 3329^-1 mod 2^16
#define FREEZE_BITSHIFT 24
#else
#define MONT 171 // 2^16 mod 769
#define MONTSQ 19 // 2^32 mod 769
#define MONTSQINV 81 // 2 ^-32 mod 769
#define QINV 64769 // 769^-1 mod 2^16
#define FREEZE_BITSHIFT 24
#endif

#define montgomery_reduce KEM_NAMESPACE(montgomery_reduce)
int16_t montgomery_reduce(int32_t a);

#define freeze KEM_NAMESPACE(freeze)
int16_t freeze(int16_t a);

#define freeze_centered KEM_NAMESPACE(freeze_centered)
int16_t freeze_centered(int16_t a);


#endif