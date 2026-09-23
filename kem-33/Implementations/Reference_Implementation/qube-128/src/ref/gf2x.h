/**
 * @file gf2x.h
 * @brief Arithmetic in R = F_2[X]/(X^r - 1).
 */

#ifndef QUBE_GF2X_H
#define QUBE_GF2X_H

#include <stdint.h>
#include "parameters.h"

void ring_mul_by_support(uint64_t *out, const uint64_t *dense, const uint32_t *support, uint16_t weight);
void vect_mul(uint64_t *out, const uint64_t *a, const uint64_t *b);

#endif  // QUBE_GF2X_H
