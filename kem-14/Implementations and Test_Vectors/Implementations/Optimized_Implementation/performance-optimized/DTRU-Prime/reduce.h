#ifndef REDUCE_H
#define REDUCE_H

#include <stdint.h>
#include "params.h"


#define MONT 992 // 2^16 mod 2017
#define QINV 31777 // 2017^-1 mod 2^16
#define BARRETT_V 1064692 // (2^31 / 2017 = 1064691.94..) rounded to nearest integer

// extern int16_t fqinv_table[DTRU_Q];

int16_t montgomery_reduce(int32_t a);
int16_t barrett_reduce(int32_t a);
int16_t fqcsubq(int16_t a);
int16_t fqmul(int16_t a, int16_t b);
int16_t fqinv(int16_t a);

#endif
