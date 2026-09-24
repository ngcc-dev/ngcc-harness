#ifndef SPX_TFORS_H
#define SPX_TFORS_H

#include <stdint.h>

#include "params.h"
#include "context.h"

void tfors_sk_to_leaf(unsigned char *leaf, const unsigned char *sk,
                            const spx_ctx *ctx,
                            uint32_t tfors_leaf_addr[8]);

void tfors_sk_to_leafx4(unsigned char *leaf0,
                        unsigned char *leaf1,
                        unsigned char *leaf2,
                        unsigned char *leaf3,
                        const unsigned char *sk0,
                        const unsigned char *sk1,
                        const unsigned char *sk2,
                        const unsigned char *sk3,
                        const spx_ctx *ctx,
                        uint32_t tfors_leaf_addrx4[4*8]);

void message_to_indices(uint32_t *indices, const unsigned char *m, const spx_ctx *ctx);

/**
 * Signs a message m, deriving the secret key from sk_seed and the FTS address.
 * Assumes m contains at least SPX_TFORS_A * SPX_TFORS_K bits.
 */
#define tfors_sign SPX_NAMESPACE(tfors_sign)
void tfors_sign(unsigned char *sig, unsigned char *pk,
               const uint32_t *indices,
               const spx_ctx* ctx,
               const uint32_t tfors_addr[8]);

/**
 * Derives the TFORS public key from a signature.
 * This can be used for verification by comparing to a known public key, or to
 * subsequently verify a signature on the derived public key. The latter is the
 * typical use-case when used as an FTS below an OTS in a hypertree.
 * Assumes m contains at least SPX_TFORS_A * SPX_TFORS_K bits.
 */
#define tfors_pk_from_sig SPX_NAMESPACE(tfors_pk_from_sig)
void tfors_pk_from_sig(unsigned char *pk,
                      const unsigned char *sig, const unsigned char *m,
                      const spx_ctx* ctx,
                      const uint32_t tfors_addr[8]);

#endif