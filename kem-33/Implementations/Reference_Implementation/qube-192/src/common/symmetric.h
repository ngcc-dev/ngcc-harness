/**
 * @file symmetric.h
 * @brief QUBE-SM3 wrappers over the API_PKC auxiliary functions.
 */

#ifndef QUBE_SYMMETRIC_H
#define QUBE_SYMMETRIC_H

#include <stddef.h>
#include <stdint.h>
#include "parameters.h"

typedef struct {
    uint8_t seed[SEED_BYTES];
    size_t seed_len;
    uint8_t *buf;
    size_t buf_len;
    size_t bit_pos;
} qube_xof_stream_t;

int qube_sym_prg(uint8_t out[2 * SEED_BYTES], const uint8_t rho[SEED_BYTES]);
int qube_sym_h(uint8_t out[SEED_BYTES], const uint8_t *pk);
int qube_sym_g(uint8_t out[2 * SEED_BYTES], const uint8_t m[PARAM_SECURITY_BYTES],
               const uint8_t sigma[SALT_BYTES], const uint8_t h_pk[SEED_BYTES]);
int qube_sym_j(uint8_t out[SHARED_SECRET_BYTES], const uint8_t rho_k[SEED_BYTES],
               const uint8_t *ct, const uint8_t h_pk[SEED_BYTES]);
int qube_sym_xof(uint8_t *out, size_t out_len, const uint8_t seed[SEED_BYTES]);

int qube_xof_stream_init(qube_xof_stream_t *ctx, const uint8_t seed[SEED_BYTES]);
void qube_xof_stream_release(qube_xof_stream_t *ctx);
int qube_xof_stream_read_bits(qube_xof_stream_t *ctx, unsigned nbits, uint32_t *value);
int qube_xof_stream_read_bytes(qube_xof_stream_t *ctx, uint8_t *out, size_t out_len);

#endif  // QUBE_SYMMETRIC_H
