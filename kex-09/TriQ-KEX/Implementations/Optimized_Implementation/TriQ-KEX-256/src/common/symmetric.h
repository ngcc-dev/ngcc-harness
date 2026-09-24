/**
 * @file symmetric.h
 * @brief Symmetric primitive adapter used by the TriQ-KEM reference code.
 */

#ifndef TRIQ_SYMMETRIC_H
#define TRIQ_SYMMETRIC_H

#include <stdint.h>
#include "data_structures.h"
#include "parameters.h"

/*
 * API_PKC provides stateless auxiliary functions.  This context stores the
 * absorbed XOF input and a byte offset so the existing streaming sampler code
 * can use the required API_PKC pseudoXOF without changing its call pattern.
 */
#define TRIQ_XOF_INPUT_MAX 128
typedef struct {
    uint8_t input[TRIQ_XOF_INPUT_MAX];
    uint32_t input_len;
    uint32_t offset;
} triq_xof_ctx;

#define TRIQ_PRNG_DOMAIN 0
#define TRIQ_XOF_DOMAIN 1
#define TRIQ_G_FCT_DOMAIN 0
#define TRIQ_H_FCT_DOMAIN 1
#define TRIQ_I_FCT_DOMAIN 2
#define TRIQ_J_FCT_DOMAIN 3

void prng_init(uint8_t *entropy_input, uint8_t *personalization_string, uint32_t enlen, uint32_t perlen);
void prng_get_bytes(uint8_t *output, uint32_t outlen);

void xof_init(triq_xof_ctx *xof_ctx, const uint8_t *seed, uint32_t seed_size);
void xof_get_bytes(triq_xof_ctx *xof_ctx, uint8_t *output, uint32_t output_size);

void hash_g(uint8_t *output, const uint8_t h_ek[SEED_BYTES], const uint8_t m[VEC_K_SIZE_BYTES],
            const uint8_t salt[SALT_BYTES]);
void hash_h(uint8_t *output, const uint8_t pk[PUBLIC_KEY_BYTES]);
void hash_i(uint8_t *output, const uint8_t *seed);
void hash_j(uint8_t *output, const uint8_t h_ek[SEED_BYTES], const uint8_t sigma[VEC_K_SIZE_BYTES],
            const ciphertext_kem_t *c_kem);

#endif
