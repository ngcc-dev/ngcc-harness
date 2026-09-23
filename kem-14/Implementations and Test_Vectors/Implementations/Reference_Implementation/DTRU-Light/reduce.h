#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>
#include "params.h"

extern int16_t fq_inverse_table[DTRU_Q];


#define MONT 171   // 2^16 mod 769
#define QINV -767   // 769^(-1) mod 2^16
#define BARRETT_V 21817 // (2^24 / 769 = 21816.92..) rounded to nearest integer

int16_t montgomery_reduce(int32_t a);
int16_t barrett_reduce(int16_t a);
int16_t fqcsubq(int16_t a);
int16_t fqmul(int16_t a, int16_t b);
int16_t fqinv(int16_t a);

#endif
