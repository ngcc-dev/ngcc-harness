/**
 * @file symmetric_api_pkc.c
 * @brief API_PKC-backed PRNG, XOF and hash wrappers for HARE.
 *
 * The HARE algorithms call prng_*, xof_* and hash_* through symmetric.h. This
 * implementation maps that call surface to the ICCS API_PKC auxiliary routines
 * used for KAT generation and preliminary self-evaluation.
 */

#include "symmetric.h"

#if !defined(HARE_SYMMETRIC_MODE_B)
#error "symmetric_api_pkc.c requires -DHARE_SYMMETRIC_MODE_B"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "auxfunc.h"
#include "drng.h"

extern DRNG_ctx drng_algorithm;

/** Domain tag for deterministic PRNG framing. */
#define MODEB_PRNG_DOMAIN 0xB0u
/** Domain tag for deterministic XOF framing. */
#define MODEB_XOF_DOMAIN 0xB1u
/** Domain tag for H(ek). */
#define MODEB_H_DOMAIN 0xB2u
/** Domain tag for I(seed). */
#define MODEB_I_DOMAIN 0xB3u
/** Domain tag for G(H(ek)||m||salt). */
#define MODEB_G_DOMAIN 0xB4u
/** Domain tag for J(H(ek)||sigma||c). */
#define MODEB_J_DOMAIN 0xB5u

/**
 * Concatenate domain-separated seed material into a caller-provided buffer.
 */
static int modeb_join_seed_material(uint8_t domain_tag,
                                    const uint8_t *a, size_t a_len,
                                    const uint8_t *b, size_t b_len,
                                    const uint8_t *c, size_t c_len,
                                    const uint8_t *d, size_t d_len,
                                    uint8_t *out, size_t out_len) {
    size_t offset = 0;

    if (out_len == 0u || out == NULL) {
        return -1;
    }
    out[offset++] = domain_tag;

    if (offset + a_len + b_len + c_len + d_len > out_len) {
        return -1;
    }
    if (a != NULL && a_len > 0u) {
        memcpy(out + offset, a, a_len);
        offset += a_len;
    }
    if (b != NULL && b_len > 0u) {
        memcpy(out + offset, b, b_len);
        offset += b_len;
    }
    if (c != NULL && c_len > 0u) {
        memcpy(out + offset, c, c_len);
        offset += c_len;
    }
    if (d != NULL && d_len > 0u) {
        memcpy(out + offset, d, d_len);
        offset += d_len;
    }
    return (int)offset;
}

/**
 * Initialize the generic PRNG call surface; API_PKC DRNG is initialized by the harness.
 */
void prng_init(uint8_t *entropy_input, uint8_t *personalization_string, uint32_t enlen, uint32_t perlen) {
    (void)entropy_input;
    (void)personalization_string;
    (void)enlen;
    (void)perlen;
}

/**
 * Draw bytes from the API_PKC DRNG, zeroing output if the auxiliary call fails.
 */
void prng_get_bytes(uint8_t *output, uint32_t outlen) {
    if (get_random_number(&drng_algorithm, output, (unsigned long long)outlen * 8ULL) != 0) {
        memset(output, 0, outlen);
    }
}

/**
 * Initialize a deterministic XOF stream from a seed.
 */
void xof_init(shake256_xof_ctx *xof_ctx, const uint8_t *seed, uint32_t seed_size) {
    memset(xof_ctx, 0, sizeof(*xof_ctx));
    if (seed_size >= SEED_BYTES) {
        memcpy(xof_ctx->seed, seed, SEED_BYTES);
    } else {
        memcpy(xof_ctx->seed, seed, seed_size);
    }
    xof_ctx->consumed_bytes = 0;
}

/**
 * Squeeze bytes from the deterministic XOF stream while preserving stream offset.
 */
void xof_get_bytes(shake256_xof_ctx *xof_ctx, uint8_t *output, uint32_t output_size) {
    uint8_t *all = NULL;
    size_t total = (size_t)xof_ctx->consumed_bytes + (size_t)output_size;
    uint8_t *framed = NULL;
    int framed_len = 0;

    framed = (uint8_t *)malloc(1u + SEED_BYTES);
    all = (uint8_t *)malloc(total);
    if (framed == NULL || all == NULL) {
        free(framed);
        free(all);
        memset(output, 0, output_size);
        return;
    }

    framed_len = modeb_join_seed_material(MODEB_XOF_DOMAIN,
                                          xof_ctx->seed, SEED_BYTES,
                                          NULL, 0u,
                                          NULL, 0u,
                                          NULL, 0u,
                                          framed, 1u + SEED_BYTES);
    if (framed_len <= 0 ||
        pseudoXOF((unsigned long long)total * 8ULL, framed, (unsigned long long)framed_len * 8ULL, all) != 0) {
        free(framed);
        free(all);
        memset(output, 0, output_size);
        return;
    }

    memcpy(output, all + xof_ctx->consumed_bytes, output_size);
    xof_ctx->consumed_bytes += output_size;
    free(framed);
    free(all);
}

/**
 * Compute the public-key hash H(ek).
 */
void hash_h(uint8_t *output, const uint8_t pk[PUBLIC_KEY_BYTES]) {
    uint8_t *framed = NULL;
    int framed_len = 0;

    framed = (uint8_t *)malloc(1u + PUBLIC_KEY_BYTES);
    if (framed == NULL) {
        memset(output, 0, SEED_BYTES);
        return;
    }
    framed_len = modeb_join_seed_material(MODEB_H_DOMAIN,
                                          pk, PUBLIC_KEY_BYTES,
                                          NULL, 0u,
                                          NULL, 0u,
                                          NULL, 0u,
                                          framed, 1u + PUBLIC_KEY_BYTES);
    if (framed_len <= 0 ||
        pseudoXOF((unsigned long long)SEED_BYTES * 8ULL, framed, (unsigned long long)framed_len * 8ULL, output) != 0) {
        memset(output, 0, SEED_BYTES);
    }
    free(framed);
}

/**
 * Compute I(seed), producing dk and ek seed material for HARE-PKE.
 */
void hash_i(uint8_t *output, const uint8_t *seed) {
    uint8_t *framed = NULL;
    int framed_len = 0;

    framed = (uint8_t *)malloc(1u + SEED_BYTES);
    if (framed == NULL) {
        memset(output, 0, 2u * SEED_BYTES);
        return;
    }
    framed_len = modeb_join_seed_material(MODEB_I_DOMAIN,
                                          seed, SEED_BYTES,
                                          NULL, 0u,
                                          NULL, 0u,
                                          NULL, 0u,
                                          framed, 1u + SEED_BYTES);
    if (framed_len <= 0 ||
        pseudoXOF((unsigned long long)(2u * SEED_BYTES) * 8ULL, framed, (unsigned long long)framed_len * 8ULL, output) != 0) {
        memset(output, 0, 2u * SEED_BYTES);
    }
    free(framed);
}

/**
 * Compute G(H(ek)||m||salt), producing shared secret K and encryption seed theta.
 */
void hash_g(uint8_t *output, const uint8_t hash_ek_kem[SEED_BYTES], const uint8_t m[PARAM_SECURITY_BYTES],
            const uint8_t salt[SALT_BYTES]) {
    uint8_t *framed = NULL;
    int framed_len = 0;
    const size_t out_len = SHARED_SECRET_BYTES + SEED_BYTES;
    const size_t in_len = 1u + SEED_BYTES + PARAM_SECURITY_BYTES + SALT_BYTES;

    framed = (uint8_t *)malloc(in_len);
    if (framed == NULL) {
        memset(output, 0, out_len);
        return;
    }
    framed_len = modeb_join_seed_material(MODEB_G_DOMAIN,
                                          hash_ek_kem, SEED_BYTES,
                                          m, PARAM_SECURITY_BYTES,
                                          salt, SALT_BYTES,
                                          NULL, 0u,
                                          framed, in_len);
    if (framed_len <= 0 ||
        pseudoXOF((unsigned long long)out_len * 8ULL, framed, (unsigned long long)framed_len * 8ULL, output) != 0) {
        memset(output, 0, out_len);
    }
    free(framed);
}

/**
 * Compute J(H(ek)||sigma||c), the FO implicit-rejection fallback key.
 */
void hash_j(uint8_t *output, const uint8_t hash_ek_kem[SEED_BYTES], const uint8_t sigma[PARAM_SECURITY_BYTES],
            const uint8_t *c_kem) {
    uint8_t *framed = NULL;
    int framed_len = 0;
    const size_t out_len = SHARED_SECRET_BYTES;
    const size_t in_len = 1u + SEED_BYTES + PARAM_SECURITY_BYTES + CIPHERTEXT_BYTES;
    size_t offset = 0;

    framed = (uint8_t *)malloc(in_len);
    if (framed == NULL) {
        memset(output, 0, out_len);
        return;
    }

    framed[offset++] = MODEB_J_DOMAIN;
    memcpy(framed + offset, hash_ek_kem, SEED_BYTES);
    offset += SEED_BYTES;
    memcpy(framed + offset, sigma, PARAM_SECURITY_BYTES);
    offset += PARAM_SECURITY_BYTES;
    memcpy(framed + offset, c_kem, CIPHERTEXT_BYTES);
    offset += CIPHERTEXT_BYTES;

    framed_len = (int)offset;
    if (pseudoXOF((unsigned long long)out_len * 8ULL, framed, (unsigned long long)framed_len * 8ULL, output) != 0) {
        memset(output, 0, out_len);
    }
    free(framed);
}
