/**
 * @file symmetric.c
 * @brief QUBE-SM3 wrappers over auxfunc.c.
 */

#include "symmetric.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "auxfunc.h"
#include "crypto_memset.h"

#define QUBE_DOMAIN_PRG 0x01
#define QUBE_DOMAIN_H   0x02
#define QUBE_DOMAIN_G   0x03
#define QUBE_DOMAIN_J   0x04
#define QUBE_DOMAIN_XOF 0x05

static int hash_bytes(uint8_t *out, size_t out_len, const uint8_t *msg, size_t msg_len, uint8_t domain);
static int append_domain(uint8_t **out, size_t *out_len, const uint8_t *msg, size_t msg_len, uint8_t domain);

static int append_domain(uint8_t **out, size_t *out_len, const uint8_t *msg, size_t msg_len, uint8_t domain) {
    uint8_t *buf = (uint8_t *)malloc(msg_len + 1);
    if (buf == NULL) {
        return -2;
    }
    memcpy(buf, msg, msg_len);
    buf[msg_len] = domain;
    *out = buf;
    *out_len = msg_len + 1;
    return 0;
}

static int hash_bytes(uint8_t *out, size_t out_len, const uint8_t *msg, size_t msg_len, uint8_t domain) {
    uint8_t *tagged = NULL;
    size_t tagged_len = 0;
    uint8_t tmp[128] = {0};
    int ret;

    ret = append_domain(&tagged, &tagged_len, msg, msg_len, domain);
    if (ret != 0) {
        return ret;
    }

    if (out_len <= 32) {
        ret = sm3hash(256, tagged, (unsigned long long)tagged_len * 8, tmp);
    } else if (out_len <= 64) {
        ret = pseudohash(512, tagged, (unsigned long long)tagged_len * 8, tmp);
    } else if (out_len <= 96) {
        ret = pseudohash(768, tagged, (unsigned long long)tagged_len * 8, tmp);
    } else if (out_len <= 128) {
        ret = pseudohash(1024, tagged, (unsigned long long)tagged_len * 8, tmp);
    } else {
        ret = -4;
    }

    if (ret == 0) {
        memcpy(out, tmp, out_len);
    }

    memset_zero(tmp, sizeof tmp);
    memset_zero(tagged, tagged_len);
    free(tagged);
    return ret;
}

int qube_sym_prg(uint8_t out[2 * SEED_BYTES], const uint8_t rho[SEED_BYTES]) {
    return hash_bytes(out, 2 * SEED_BYTES, rho, SEED_BYTES, QUBE_DOMAIN_PRG);
}

int qube_sym_h(uint8_t out[SEED_BYTES], const uint8_t *pk) {
    return hash_bytes(out, SEED_BYTES, pk, PUBLIC_KEY_BYTES, QUBE_DOMAIN_H);
}

int qube_sym_g(uint8_t out[2 * SEED_BYTES], const uint8_t m[PARAM_SECURITY_BYTES],
               const uint8_t sigma[SALT_BYTES], const uint8_t h_pk[SEED_BYTES]) {
    uint8_t input[PARAM_SECURITY_BYTES + SALT_BYTES + SEED_BYTES];
    int ret;

    memcpy(input, m, PARAM_SECURITY_BYTES);
    memcpy(input + PARAM_SECURITY_BYTES, sigma, SALT_BYTES);
    memcpy(input + PARAM_SECURITY_BYTES + SALT_BYTES, h_pk, SEED_BYTES);

    ret = hash_bytes(out, 2 * SEED_BYTES, input, sizeof input, QUBE_DOMAIN_G);
    memset_zero(input, sizeof input);
    return ret;
}

int qube_sym_j(uint8_t out[SHARED_SECRET_BYTES], const uint8_t rho_k[SEED_BYTES],
               const uint8_t *ct, const uint8_t h_pk[SEED_BYTES]) {
    const size_t input_len = SEED_BYTES + CIPHERTEXT_BYTES + SEED_BYTES;
    uint8_t *input = (uint8_t *)malloc(input_len);
    int ret;

    if (input == NULL) {
        return -2;
    }

    memcpy(input, rho_k, SEED_BYTES);
    memcpy(input + SEED_BYTES, ct, CIPHERTEXT_BYTES);
    memcpy(input + SEED_BYTES + CIPHERTEXT_BYTES, h_pk, SEED_BYTES);

    ret = hash_bytes(out, SHARED_SECRET_BYTES, input, input_len, QUBE_DOMAIN_J);
    memset_zero(input, input_len);
    free(input);
    return ret;
}

int qube_sym_xof(uint8_t *out, size_t out_len, const uint8_t seed[SEED_BYTES]) {
    uint8_t *tagged = NULL;
    size_t tagged_len = 0;
    int ret;

    ret = append_domain(&tagged, &tagged_len, seed, SEED_BYTES, QUBE_DOMAIN_XOF);
    if (ret != 0) {
        return ret;
    }

    ret = pseudoXOF((unsigned long long)out_len * 8, tagged, (unsigned long long)tagged_len * 8, out);
    memset_zero(tagged, tagged_len);
    free(tagged);
    return ret;
}

int qube_xof_stream_init(qube_xof_stream_t *ctx, const uint8_t seed[SEED_BYTES]) {
    if (ctx == NULL || seed == NULL) {
        return -1;
    }
    memset(ctx, 0, sizeof *ctx);
    memcpy(ctx->seed, seed, SEED_BYTES);
    ctx->seed_len = SEED_BYTES;
    return 0;
}

void qube_xof_stream_release(qube_xof_stream_t *ctx) {
    if (ctx == NULL) {
        return;
    }
    if (ctx->buf != NULL) {
        memset_zero(ctx->buf, ctx->buf_len);
        free(ctx->buf);
    }
    memset_zero(ctx, sizeof *ctx);
}

static int stream_ensure(qube_xof_stream_t *ctx, size_t bits_needed) {
    const size_t needed_bytes = (ctx->bit_pos + bits_needed + 7) / 8;
    const size_t old_len = ctx->buf_len;
    size_t new_len = ctx->buf_len;
    uint8_t *new_buf;
    int ret;

    if (needed_bytes <= ctx->buf_len) {
        return 0;
    }

    if (new_len == 0) {
        new_len = 32;
    }
    while (new_len < needed_bytes) {
        if (new_len > SIZE_MAX / 2) {
            return -2;
        }
        new_len *= 2;
    }

    new_buf = (uint8_t *)realloc(ctx->buf, new_len);
    if (new_buf == NULL) {
        return -2;
    }
    ctx->buf = new_buf;
    if (new_len > old_len) {
        memset(ctx->buf + old_len, 0, new_len - old_len);
    }
    ctx->buf_len = new_len;
    ret = qube_sym_xof(ctx->buf, new_len, ctx->seed);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int qube_xof_stream_read_bits(qube_xof_stream_t *ctx, unsigned nbits, uint32_t *value) {
    uint32_t v = 0;
    int ret;

    if (ctx == NULL || value == NULL || nbits > 31) {
        return -1;
    }

    ret = stream_ensure(ctx, nbits);
    if (ret != 0) {
        return ret;
    }

    for (unsigned i = 0; i < nbits; i++) {
        const size_t pos = ctx->bit_pos + i;
        const uint32_t bit = (uint32_t)((ctx->buf[pos >> 3] >> (pos & 7)) & 1U);
        v |= bit << i;
    }

    ctx->bit_pos += nbits;
    *value = v;
    return 0;
}

int qube_xof_stream_read_bytes(qube_xof_stream_t *ctx, uint8_t *out, size_t out_len) {
    int ret;

    if (ctx == NULL || out == NULL) {
        return -1;
    }

    ret = stream_ensure(ctx, out_len * 8);
    if (ret != 0) {
        return ret;
    }

    if ((ctx->bit_pos & 7U) == 0) {
        memcpy(out, ctx->buf + (ctx->bit_pos >> 3), out_len);
    } else {
        for (size_t i = 0; i < out_len; i++) {
            uint8_t v = 0;
            for (unsigned bit = 0; bit < 8; bit++) {
                const size_t pos = ctx->bit_pos + i * 8 + bit;
                v |= (uint8_t)(((ctx->buf[pos >> 3] >> (pos & 7)) & 1U) << bit);
            }
            out[i] = v;
        }
    }

    ctx->bit_pos += out_len * 8;
    return 0;
}
