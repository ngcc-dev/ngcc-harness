#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "hash.h"
#include "thash.h"
#include "gwots.h"
#include "gwotsx1.h"
#include "address.h"
#include "params.h"

/*
 * This generates a GWOTS public key
 * It also generates the GWOTS signature if leaf_info indicates
 * that we're signing with this GWOTS key
 */
void gwots_gen_leafx1(unsigned char *dest,
                   const spx_ctx *ctx,
                   uint32_t leaf_idx, void *v_info) {
    struct leaf_info_x1 *info = v_info;
    uint32_t *leaf_addr = info->leaf_addr;
    uint32_t *pk_addr = info->pk_addr;
    unsigned int i, k;
    unsigned char pk_buffer[ SPX_GWOTS_BYTES ];
    unsigned char *buffer;
    uint32_t gwots_k_mask;

    if (leaf_idx == info->gwots_sign_leaf) {
        /* We're traversing the leaf that's signing; generate the GWOTS */
        /* signature */
        gwots_k_mask = 0;
    } else {
        /* Nope, we're just generating pk's; turn off the signature logic */
        gwots_k_mask = (uint32_t)~0;
    }

    set_keypair_addr( leaf_addr, leaf_idx );
    set_keypair_addr( pk_addr, leaf_idx );

    for (i = 0, buffer = pk_buffer; i < SPX_GWOTS_LEN; i++, buffer += SPX_N) {
        uint32_t w;
        if (i < SPX_GWOTS_W1_LEN) {
            w = SPX_GWOTS_W1;
        } else if (i < SPX_GWOTS_LEN1) {
            w = SPX_GWOTS_W2;
        } else {
            w = SPX_GWOTS_CHECKSUM_W;
        }
        uint32_t gwots_k = info->gwots_steps[i] | gwots_k_mask; /* Set gwots_k to */
            /* the step if we're generating a signature, ~0 if we're not */

        /* Start with the secret seed */
        set_chain_addr(leaf_addr, i);
        set_hash_addr(leaf_addr, 0);
        set_type(leaf_addr, SPX_ADDR_TYPE_GWOTSPRF);
 
        prf_addr(buffer, ctx, leaf_addr);

        set_type(leaf_addr, SPX_ADDR_TYPE_GWOTS);

        /* Iterate down the GWOTS chain */
        for (k=0;; k++) {
            /* Check if this is the value that needs to be saved as a */
            /* part of the GWOTS signature */
            if (k == gwots_k) {
                memcpy( info->gwots_sig + i * SPX_N, buffer, SPX_N );
            }

            /* Check if we hit the top of the chain */
            if (k == w - 1) break;

            /* Iterate one step on the chain */
            set_hash_addr(leaf_addr, k);

            thash(buffer, buffer, 1, ctx, leaf_addr);
        }
    }

    /* Do the final thash to generate the public keys */
    thash(dest, pk_buffer, SPX_GWOTS_LEN, ctx, pk_addr);
}



