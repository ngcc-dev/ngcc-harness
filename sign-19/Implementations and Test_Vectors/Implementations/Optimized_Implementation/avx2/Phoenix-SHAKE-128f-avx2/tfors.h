/* Phoenix TFORS (Threshold FORS) interface
 *
 * Few-time signature scheme with threshold authentication structure.
 */

#ifndef PH_TFORS_H
#define PH_TFORS_H

#include <stdint.h>

#include "params.h"
#include "context.h"

/* Convert TFORS secret key to leaf node via thash */
void tfors_sk_to_leaf(unsigned char *leaf, const unsigned char *sk,
                            const spx_ctx *ctx,
                            uint32_t tfors_leaf_addr[8]);

/* Convert 4 TFORS secret keys to leaf nodes in parallel */
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

/* Derive TFORS leaf indices from message using H2 and bit extraction */
void message_to_indices(uint32_t *indices, const unsigned char *msg, const spx_ctx *ctx);

/* Generate TFORS signature with secret keys and authentication paths */
#define tfors_sign SPX_NAMESPACE(tfors_sign)
void tfors_sign(unsigned char *sig, unsigned char *pk,
               const uint32_t *indices,
               const spx_ctx* ctx,
               const uint32_t tfors_addr[8]);

/* Verify TFORS signature and recover public key */
#define tfors_pk_from_sig SPX_NAMESPACE(tfors_pk_from_sig)
void tfors_pk_from_sig(unsigned char *pk,
                      const unsigned char *sig, const unsigned char *msg,
                      const spx_ctx* ctx,
                      const uint32_t tfors_addr[8]);

#endif /* PH_TFORS_H */