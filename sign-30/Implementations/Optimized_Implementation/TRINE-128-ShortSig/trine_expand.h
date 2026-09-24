#ifndef TRINE_EXPAND_H
#define TRINE_EXPAND_H

#include <stdint.h>

#include "params.h"

typedef enum
{
  TRINE_MATRIX_A = 0,
  TRINE_MATRIX_B = 1,
  TRINE_MATRIX_C = 2
} trine_matrix_role_t;

int trine_expand_public_seed(
    uint8_t out_public_seed[TRINE_public_seed_bytes],
    const uint8_t secret_seed[TRINE_secret_seed_bytes]);

int trine_expand_base_form(
    Fq *out_base_form,
    const uint8_t public_seed[TRINE_public_seed_bytes],
    int n);

int trine_expand_secret_matrix_pair_vartime(
    Fq *out_matrix,
    Fq *out_inverse,
    const uint8_t secret_seed[TRINE_secret_seed_bytes],
    trine_matrix_role_t role,
    uint32_t index,
    int n);

#endif
