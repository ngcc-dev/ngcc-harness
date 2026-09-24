#ifndef INV_H
#define INV_H

#include <stdint.h>
#include "params.h"
int rq_inverse_avx(int16_t finv[], int16_t f[], const int16_t *qdata);
#endif