#ifndef INVERSE_H
#define INVERSE_H

#include <stdint.h>
#include "params.h"

int rq_inverse(int16_t *finv, const int16_t *f);
int16_t fq_freeze(int32_t x);

void int32_divmod_uint14(int32_t *y, uint16_t *r, int32_t x, uint16_t m);
uint16_t uint32_mod_uint14(uint32_t x, uint16_t m);
int rq_inverse(int16_t *finv, const int16_t *f);
#endif