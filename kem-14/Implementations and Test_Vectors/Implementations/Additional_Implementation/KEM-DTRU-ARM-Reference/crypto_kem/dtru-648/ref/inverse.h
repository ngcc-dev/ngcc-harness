#ifndef INVERSE_H
#define INVERSE_H

#include <stdint.h>
#include "ntt.h"
#include "params.h"

void uint32_divmod_uint14(uint32_t *y, uint16_t *r, uint32_t x, uint16_t m);
int rq_inverse(int16_t finv[ROOT_DIMENSION], const int16_t f[ROOT_DIMENSION], const int16_t zeta);
int rq_inverse_opt(int16_t finv[ROOT_DIMENSION], const int16_t f[ROOT_DIMENSION], const int16_t zeta);
int base_inv_opt(int16_t finv[3], const int16_t f[3], const int16_t zeta);
#endif