/*
* SM4-ECB wrapper for the OWF.
*/

#include "sm4.h"
#include "compat.h"
#include <string.h>

#ifndef PRG_BLOCK_SIZE
#define PRG_BLOCK_SIZE SM4_BLOCK_SIZE
#endif

static unsigned int sm4_detect_acce_impl(void) {
    return SM4_ACCE_IMPL_SCALAR;
}

int SM4_set_key_acce(const uint8_t* key, SM4_KEY* ks, unsigned int* impl) {
    unsigned int chosen_impl = SM4_ACCE_IMPL_SCALAR;

    if (!key || !ks) {
        return -1;
    }

    chosen_impl = sm4_detect_acce_impl();

    if (SM4_set_key(key, ks) == 0) {
        return -1;
    }

    if (impl) {
        *impl = chosen_impl;
    }

    return 0;
}

int SM4_encrypt_acce(const uint8_t* in, uint8_t* out, const SM4_KEY* ks, size_t blocks) {
    if (!in || !out || !ks) {
        return -1;
    }

    for (; blocks; --blocks, in += PRG_BLOCK_SIZE, out += PRG_BLOCK_SIZE) {
        SM4_encrypt(in, out, ks);
    }
    return 0;
}

/* ================================================================== *
 *  Generic SM4-ECB PRG wrapper (public)
 * ================================================================== */

int generic_sm4_ecb_new(generic_sm4_ecb_t* ctx, const uint8_t* key, unsigned int seclvl) {
    (void) seclvl; /* SM4 supports 128-bit keys only in this code */
    if (!ctx || !key) return -1;
    if (SM4_set_key_acce(key, &ctx->ks, &ctx->acce_impl) != 0) return -1;
    ctx->seclvl = seclvl;
    return 0;
}

int generic_sm4_ecb_encrypt(generic_sm4_ecb_t* ctx, uint8_t* ciphertext, const uint8_t* plaintext,
                            size_t blocks) {
    if (!ctx) return -1;
    return SM4_encrypt_acce(plaintext, ciphertext, &ctx->ks, blocks);
}

void generic_sm4_ecb_free(generic_sm4_ecb_t* ctx) {
    if (!ctx) return;
    explicit_bzero(&ctx->ks, sizeof(ctx->ks));
    ctx->acce_impl = SM4_ACCE_IMPL_SCALAR;
}
