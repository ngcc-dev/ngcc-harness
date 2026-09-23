#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>
#include "params.h"

extern int16_t fq_inverse_table[DTRU_Q];


#define MONT 3310   // 2^16 mod 3457
#define QINV 12929   // 3457^(-1) mod 2^16
#define BARRETT_V 19412 // (2^26 / 3457 = 19412.45..) rounded to nearest integer

int16_t montgomery_reduce(int32_t a);
int16_t barrett_reduce(int16_t a);
int16_t fqcsubq(int16_t a);
int16_t fqmul(int16_t a, int16_t b);
int16_t fqinv(int16_t a);

#endif
