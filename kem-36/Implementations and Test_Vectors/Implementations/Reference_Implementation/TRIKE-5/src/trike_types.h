#ifndef TRIKE_TYPES_H
#define TRIKE_TYPES_H

#include <stdint.h>
#include <stddef.h>

#include "trike_params.h"

typedef struct {
    uint8_t r2[R_SIZE_BYTES];
    uint8_t sigma[M_SIZE_BYTES];
} public_key_t;

typedef struct {
    uint32_t h0_idx[PARAM_D], h1_idx[PARAM_D], h2_idx[PARAM_D];
    uint8_t h0[R_SIZE_BYTES];
    uint8_t t0[R_SIZE_BYTES], r2[R_SIZE_BYTES];
    uint8_t sigma[M_SIZE_BYTES], sigma2[M_SIZE_BYTES];
} secret_key_t;

typedef struct {
    uint8_t u[R_SIZE_BYTES];
    uint8_t v[R_SIZE_BYTES];
    uint8_t c2[M_SIZE_BYTES];
} ciphertext_t;

typedef struct {
    uint8_t ss[M_SIZE_BYTES];
} shared_secret_t;

#endif