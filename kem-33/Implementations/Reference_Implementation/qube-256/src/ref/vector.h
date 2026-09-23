/**
 * @file vector.h
 * @brief Bit-vector helpers, PDF samplers, and constant-time utilities.
 */

#ifndef QUBE_VECTOR_H
#define QUBE_VECTOR_H

#include <stddef.h>
#include <stdint.h>
#include "symmetric.h"

void vect_set_zero(uint64_t *v, size_t words);
void vect_from_bytes(uint64_t *out, const uint8_t *in, size_t nbits);
void vect_to_bytes(uint8_t *out, const uint64_t *in, size_t nbits);
int vect_set_random(qube_xof_stream_t *ctx, uint64_t *v);
int vect_sample_fixed_weight(qube_xof_stream_t *ctx, uint64_t *v, uint32_t *support, uint16_t weight);
void vect_write_support_to_vector(uint64_t *v, const uint32_t *support, uint16_t weight);
void vect_add(uint64_t *o, const uint64_t *v1, const uint64_t *v2, uint32_t size);
uint8_t vect_compare(const uint8_t *v1, const uint8_t *v2, uint32_t size);
void vect_select(uint8_t *out, const uint8_t *a, const uint8_t *b, size_t len, uint8_t select_b);
void vect_truncate(uint64_t *v);
uint32_t vect_weight(const uint64_t *v, size_t words);
void vect_print(const uint64_t *v, uint32_t size);

#endif  // QUBE_VECTOR_H
