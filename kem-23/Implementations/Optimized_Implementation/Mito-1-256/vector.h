/**
 * @file vector.h
 * @brief Header file for vector.c
 */

#ifndef MITO_VECTOR_H
#define MITO_VECTOR_H

#include <stdint.h>
#include "symmetric.h"

static const uint64_t BITMASK = (1ULL << (PARAM_N & 63)) - 1;      /*Create a mask*/

void vect_sample_fixed_weight(DRNG_ctx *ctx, uint64_t v[][VEC_N_SIZE_64], const int* weight, int dim);
void vect_set_random(DRNG_ctx *ctx, uint64_t *v);

void vect_add(uint64_t *o, const uint64_t *v1, const uint64_t *v2, uint32_t size);
uint8_t vect_compare(const uint8_t *v1, const uint8_t *v2, uint32_t size);
void vect_truncate(uint64_t *v);

#endif  // MITO_VECTOR_H
