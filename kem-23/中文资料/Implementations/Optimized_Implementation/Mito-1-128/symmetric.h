/**
 * @file symmetric.h
 * @brief Header file of symmetric.c
 */

#ifndef MITO_SYMMETRIC_H
#define MITO_SYMMETRIC_H

#include <stdint.h>
#include "drng.h"
#include "auxfunc.h"
#include "parameters.h"
#include "data_structures.h"

void prng_init(uint8_t *entropy_input, uint8_t *personalization_string, uint32_t enlen, uint32_t perlen);
void prng_get_bytes(uint8_t *output, uint32_t outlen);

void xof_init(DRNG_ctx *xof_ctx, const uint8_t *seed, uint32_t seed_size);
void xof_get_bytes(DRNG_ctx *xof_ctx, uint8_t *output, uint32_t output_size);

void hash_g(uint8_t *output, const uint8_t h_ek[SEED_BYTES], const uint8_t m[PARAM_K_BYTES], const uint8_t salt[SALT_BYTES]);
void hash_h(uint8_t *output, const uint8_t pk[PUBLIC_KEY_BYTES]);
void hash_i(uint8_t *output, const uint8_t *seed);
void hash_j(uint8_t *output, const uint8_t h_ek[SEED_BYTES], const uint8_t sigma[PARAM_K_BYTES], const ciphertext_kem_t *c_kem);

#endif  // MITO_SYMMETRIC_H
