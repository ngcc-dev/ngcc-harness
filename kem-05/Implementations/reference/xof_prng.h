/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/

#ifndef XOF_PRNG_H
#define XOF_PRNG_H

#include "types.h"

#define XOF_PRNG_WORDS \
    (((2ULL * DV) > T1) ? (2ULL * DV) : T1)
#define XOF_PRNG_OUTPUT_SIZE \
    (XOF_PRNG_WORDS * sizeof(uint32_t))

typedef struct xof_prng_state_s
{
    uint8_t buffer[XOF_PRNG_OUTPUT_SIZE];
    size_t pos;
} xof_prng_state_t;

status_t xof_prng_init(
    IN const uint8_t *input,
    IN uint64_t input_len,
    OUT xof_prng_state_t *state);

status_t xof_prng_generate(
    OUT uint8_t *output,
    IN OUT xof_prng_state_t *state,
    IN uint32_t output_len);

#endif
