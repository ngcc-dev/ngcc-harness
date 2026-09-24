/**
 * @file gf2x.h
 * @brief Header file for gf2x.c
 */

#ifndef MITO_GF2X_H
#define MITO_GF2X_H

#include <stdint.h>
#include <string.h>
#include "parameters.h"

void vect_mul_1(uint64_t* o, const uint64_t* a1, const uint64_t* a2);
void vect_mul(uint64_t z[PARAM_L][VEC_N_SIZE_64], const uint64_t h[PARAM_L][VEC_N_SIZE_64], const uint64_t y[PARAM_L][VEC_N_SIZE_64]);


#endif  // MITO_GF2X_H
