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
    ctx->cached = 0;
}

static void aux_xof_stream(triq_xof_ctx *ctx, uint8_t *output, uint32_t output_size) {
    /* Serve from cache if possible */
    if (output_size <= ctx->cached) {
        memcpy(output, ctx->cache, output_size);
        memmove(ctx->cache, ctx->cache + output_size, ctx->cached - output_size);
        ctx->cached -= output_size;
        ctx->offset += output_size;
        return;
    }

    /* Drain remaining cache first */
    uint32_t taken = 0;
    if (ctx->cached > 0) {
        memcpy(output, ctx->cache, ctx->cached);
        taken = ctx->cached;
        ctx->offset += ctx->cached;
        ctx->cached = 0;
    }

    /* For requests larger than cache, generate directly */
    const uint32_t remaining = output_size - taken;
    const uint32_t step = (remaining + 7U) & ~7U;
    const uint32_t total = ctx->offset + step;
    uint8_t buf[total == 0 ? 1U : total];
    if (pseudoXOF((unsigned long long)total * 8ULL, ctx->input,
                  (unsigned long long)ctx->input_len * 8ULL, buf) != 0) {
        memset(output + taken, 0, remaining);
    } else {
        memcpy(output + taken, buf + ctx->offset, remaining);
    }
    ctx->offset += step;

    /* Pre-fetch cache for the next request */
    const uint32_t next_total = ctx->offset + TRIQ_XOF_CACHE_SIZE;
    uint8_t cache_buf[next_total == 0 ? 1U : next_total];
    if (pseudoXOF((unsigned long long)next_total * 8ULL, ctx->input,
                  (unsigned long long)ctx->input_len * 8ULL, cache_buf) == 0) {
        memcpy(ctx->cache, cache_buf + ctx->offset, TRIQ_XOF_CACHE_SIZE);
        ctx->cached = TRIQ_XOF_CACHE_SIZE;
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
    uint8_t msg[SEED_BYTES + 1U];
    memcpy(msg, seed, SEED_BYTES);
    msg[SEED_BYTES] = TRIQ_I_FCT_DOMAIN;
    const uint32_t mlen = SEED_BYTES + 1U;
    const uint32_t outlen = 2 * SEED_BYTES;
    if (outlen <= 32) {
        uint8_t _buf[32];
        sm3hash(256, msg, (unsigned long long)mlen * 8ULL, _buf);
        memcpy(output, _buf, outlen);
    } else {
        pseudoXOF((unsigned long long)outlen * 8ULL, msg,
                  (unsigned long long)mlen * 8ULL, output);
    }
}

void hash_h(uint8_t *output, const uint8_t ek_kem[PUBLIC_KEY_BYTES]) {
    uint8_t msg[PUBLIC_KEY_BYTES + 1U];
    memcpy(msg, ek_kem, PUBLIC_KEY_BYTES);
    msg[PUBLIC_KEY_BYTES] = TRIQ_H_FCT_DOMAIN;
    const uint32_t mlen = PUBLIC_KEY_BYTES + 1U;
    const uint32_t outlen = SEED_BYTES;
    if (outlen <= 32) {
        uint8_t _buf[32];
        sm3hash(256, msg, (unsigned long long)mlen * 8ULL, _buf);
        memcpy(output, _buf, outlen);
    } else {
        pseudoXOF((unsigned long long)outlen * 8ULL, msg,
                  (unsigned long long)mlen * 8ULL, output);
    }
}

void hash_g(uint8_t *output, const uint8_t hash_ek_kem[SEED_BYTES], const uint8_t m[PARAM_SECURITY_BYTES],
            const uint8_t salt[SALT_BYTES]) {
    const uint32_t total = SEED_BYTES + PARAM_SECURITY_BYTES + SALT_BYTES + 1U;
    uint8_t msg[total];
    uint32_t off = 0;
    memcpy(msg + off, hash_ek_kem, SEED_BYTES); off += SEED_BYTES;
    memcpy(msg + off, m, PARAM_SECURITY_BYTES); off += PARAM_SECURITY_BYTES;
    memcpy(msg + off, salt, SALT_BYTES); off += SALT_BYTES;
    msg[off] = TRIQ_G_FCT_DOMAIN;
    const uint32_t outlen = SHARED_SECRET_BYTES + SEED_BYTES;
    if (outlen <= 32) {
        uint8_t _buf[32];
        if (sm3hash(256, msg, (unsigned long long)total * 8ULL, _buf) != 0) {
            memset(output, 0, outlen);
        } else { memcpy(output, _buf, outlen); }
    } else {
        /* outlen should be 32 for TriQ-128 with SEED=16, SS=16 */
        uint8_t _buf[128];
        if (pseudohash(1024, msg, (unsigned long long)total * 8ULL, _buf) != 0) {
            memset(output, 0, outlen);
        } else { memcpy(output, _buf, outlen); }
    }
}

void hash_j(uint8_t *output, const uint8_t hash_ek_kem[SEED_BYTES], const uint8_t sigma[PARAM_SECURITY_BYTES],
            const ciphertext_kem_t *c_kem) {
    const uint32_t total = SEED_BYTES + PARAM_SECURITY_BYTES + VEC_N_SIZE_BYTES + VEC_N1N2_SIZE_BYTES + SALT_BYTES + 1U;
    uint8_t msg[total];
    uint32_t off = 0;
    memcpy(msg + off, hash_ek_kem, SEED_BYTES); off += SEED_BYTES;
    memcpy(msg + off, sigma, PARAM_SECURITY_BYTES); off += PARAM_SECURITY_BYTES;
    memcpy(msg + off, (const uint8_t *)c_kem->c_pke.u, VEC_N_SIZE_BYTES); off += VEC_N_SIZE_BYTES;
    memcpy(msg + off, (const uint8_t *)c_kem->c_pke.v, VEC_N1N2_SIZE_BYTES); off += VEC_N1N2_SIZE_BYTES;
    memcpy(msg + off, c_kem->salt, SALT_BYTES); off += SALT_BYTES;
    msg[off] = TRIQ_J_FCT_DOMAIN;
    const uint32_t outlen = SHARED_SECRET_BYTES;
    if (outlen <= 32) {
        uint8_t _buf[32];
        if (sm3hash(256, msg, (unsigned long long)total * 8ULL, _buf) != 0) {
            memset(output, 0, outlen);
        } else { memcpy(output, _buf, outlen); }
    } else {
        if (pseudoXOF((unsigned long long)outlen * 8ULL, msg,
                      (unsigned long long)total * 8ULL, output) != 0) {
            memset(output, 0, outlen);
        }
    }
}
