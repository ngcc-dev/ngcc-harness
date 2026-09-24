/**
 * @file vector.h
 * @brief Header file for vector.c
 */

#ifndef TRIQ_OPT_VECTOR_H
#define TRIQ_OPT_VECTOR_H

#include <immintrin.h>
#include <stdint.h>
#include "symmetric.h"

void vect_generate_random_support1(triq_xof_ctx *ctx, uint32_t *support, uint16_t weight);
void vect_generate_random_support2(triq_xof_ctx *ctx, uint32_t *support, uint16_t weight);
void vect_write_support_to_vector(__m256i *v, uint32_t *support, uint16_t weight);
void vect_sample_fixed_weight1(triq_xof_ctx *ctx, __m256i *v, uint16_t weight);
void vect_sample_fixed_weight2(triq_xof_ctx *ctx, __m256i *v, uint16_t weight);
void vect_set_random(triq_xof_ctx *ctx, uint64_t *v);

uint8_t vect_check_bounded_density(const uint32_t *support, uint16_t weight,
                                   uint16_t L, uint16_t gamma);
void vect_sample_fixed_weight_bd(triq_xof_ctx *ctx, __m256i *v,
                                 uint16_t weight, uint16_t L, uint16_t gamma,
                                 uint16_t N_max);

void vect_add(uint64_t *o, const uint64_t *v1, const uint64_t *v2, uint32_t size);
uint8_t vect_compare(const uint8_t *v1, const uint8_t *v2, uint32_t size);
void vect_truncate(uint64_t *v);

void vect_print(const uint64_t *v, const uint32_t size);

#endif  // TRIQ_OPT_VECTOR_H

