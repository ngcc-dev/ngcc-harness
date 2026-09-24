/**
 * @file symmetric.c
 * @brief Symmetric primitives backed by the API_PKC auxiliary functions.
 */

#include "symmetric.h"
#include "auxfunc.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static triq_xof_ctx prng_ctx;

static void store_xof_input(triq_xof_ctx *ctx, const uint8_t *input, uint32_t input_len, uint8_t domain) {
    if (input_len + 1U > TRIQ_XOF_INPUT_MAX) {
        memset(ctx, 0, sizeof(*ctx));
        return;
    }
    memcpy(ctx->input, input, input_len);
    ctx->input[input_len] = domain;
    ctx->input_len = input_len + 1U;
    ctx->offset = 0;
}

static void aux_xof_stream(triq_xof_ctx *ctx, uint8_t *output, uint32_t output_size) {
    const uint32_t step = (output_size + 7U) & ~7U;
    const uint32_t total = ctx->offset + step;
    uint8_t buf[total == 0 ? 1U : total];

    if (pseudoXOF((unsigned long long)total * 8ULL, ctx->input,
                  (unsigned long long)ctx->input_len * 8ULL, buf) != 0) {
        memset(output, 0, output_size);
    } else {
        memcpy(output, buf + ctx->offset, output_size);
    }
    ctx->offset += step;
}

static void aux_pseudohash_concat(uint8_t *output, uint32_t output_size,
                                  const uint8_t *a, uint32_t a_len,
                                  const uint8_t *b, uint32_t b_len,
                                  const uint8_t *c, uint32_t c_len,
                                  const uint8_t *d, uint32_t d_len,
                                  const uint8_t *e, uint32_t e_len,
                                  uint8_t domain) {
    const uint32_t msg_len = a_len + b_len + c_len + d_len + e_len + 1U;
    uint8_t msg[msg_len];
    uint32_t off = 0;

    if (a_len != 0) { memcpy(msg + off, a, a_len); off += a_len; }
    if (b_len != 0) { memcpy(msg + off, b, b_len); off += b_len; }
    if (c_len != 0) { memcpy(msg + off, c, c_len); off += c_len; }
    if (d_len != 0) { memcpy(msg + off, d, d_len); off += d_len; }
    if (e_len != 0) { memcpy(msg + off, e, e_len); off += e_len; }
    msg[off] = domain;

    if (output_size == 64 || output_size == 96 || output_size == 128) {
        if (pseudohash((int)output_size * 8, msg, (unsigned long long)msg_len * 8ULL, output) != 0) {
            memset(output, 0, output_size);
        }
    } else if (pseudoXOF((unsigned long long)output_size * 8ULL, msg,
                         (unsigned long long)msg_len * 8ULL, output) != 0) {
        memset(output, 0, output_size);
    }
}

void prng_init(uint8_t *entropy_input, uint8_t *personalization_string, uint32_t enlen, uint32_t perlen) {
    uint8_t input[96];

    if (enlen + perlen > sizeof(input)) {
        memset(&prng_ctx, 0, sizeof(prng_ctx));
        return;
    }

    memcpy(input, entropy_input, enlen);
    memcpy(input + enlen, personalization_string, perlen);
    store_xof_input(&prng_ctx, input, enlen + perlen, TRIQ_PRNG_DOMAIN);
}

void prng_get_bytes(uint8_t *output, uint32_t outlen) {
    aux_xof_stream(&prng_ctx, output, outlen);
}

void xof_init(triq_xof_ctx *xof_ctx, const uint8_t *seed, uint32_t seed_size) {
    store_xof_input(xof_ctx, seed, seed_size, TRIQ_XOF_DOMAIN);
}

void xof_get_bytes(triq_xof_ctx *xof_ctx, uint8_t *output, uint32_t output_size) {
    aux_xof_stream(xof_ctx, output, output_size);
}

void hash_i(uint8_t *output, const uint8_t *seed) {
    aux_pseudohash_concat(output, 2 * SEED_BYTES,
                          seed, SEED_BYTES,
                          NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                          TRIQ_I_FCT_DOMAIN);
}

void hash_h(uint8_t *output, const uint8_t ek_kem[PUBLIC_KEY_BYTES]) {
    aux_pseudohash_concat(output, SEED_BYTES,
                          ek_kem, PUBLIC_KEY_BYTES,
                          NULL, 0, NULL, 0, NULL, 0, NULL, 0,
                          TRIQ_H_FCT_DOMAIN);
}

void hash_g(uint8_t *output, const uint8_t hash_ek_kem[SEED_BYTES], const uint8_t m[PARAM_SECURITY_BYTES],
            const uint8_t salt[SALT_BYTES]) {
    aux_pseudohash_concat(output, SHARED_SECRET_BYTES + SEED_BYTES,
                          hash_ek_kem, SEED_BYTES,
                          m, PARAM_SECURITY_BYTES,
                          salt, SALT_BYTES,
                          NULL, 0, NULL, 0,
                          TRIQ_G_FCT_DOMAIN);
}

void hash_j(uint8_t *output, const uint8_t hash_ek_kem[SEED_BYTES], const uint8_t sigma[PARAM_SECURITY_BYTES],
            const ciphertext_kem_t *c_kem) {
    aux_pseudohash_concat(output, SHARED_SECRET_BYTES,
                          hash_ek_kem, SEED_BYTES,
                          sigma, PARAM_SECURITY_BYTES,
                          (const uint8_t *)c_kem->c_pke.u, VEC_N1N2_SIZE_BYTES,
                          (const uint8_t *)c_kem->c_pke.v, VEC_N_SIZE_BYTES,
                          c_kem->salt, SALT_BYTES,
                          TRIQ_J_FCT_DOMAIN);
}
