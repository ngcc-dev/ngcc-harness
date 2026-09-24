#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "tfors.h"
// #include "utils.h"
// #include "utilsx1.h"
#include "hash.h"
#include "thash.h"
#include "address.h"
#include "octopus.h"

static void tfors_gen_sk(unsigned char *sk, const spx_ctx *ctx,
                        uint32_t tfors_leaf_addr[8])
{
    prf_addr(sk, ctx, tfors_leaf_addr);
}

void tfors_sk_to_leaf(unsigned char *leaf, const unsigned char *sk,
                            const spx_ctx *ctx,
                            uint32_t tfors_leaf_addr[8])
{
    thash(leaf, sk, 1, ctx, tfors_leaf_addr);
}

struct tfors_gen_leaf_info {
    uint32_t leaf_addrx[8];
};


static void sort_indices(uint32_t *indices)
{
    for (uint32_t i = 1; i < SPX_TFORS_K; i++) {
        uint32_t idx = indices[i];
        int j = i - 1;
        while (j >= 0 && indices[j] > idx) {
            indices[j + 1] = indices[j];
            j--;
        }
        indices[j + 1] = idx;
    }
}

/**
 * Generates TFORS leaf indices from a message.
 * First uses H2 to generate indices1 in [0, SPX_TFORS_K_PRIME-1] from the
 * first SPX_N bytes of the message.
 * Then extracts indices2 in [0, 2^SPX_TFORS_A -1] from the full message bitstream.
 * Final leaf index: indices[i] = indices1[i] + SPX_TFORS_K_PRIME * indices2[i].
 *
 * Assumes m has length at least max(8*SPX_N, SPX_TFORS_A * SPX_TFORS_K) bits.
 * Assumes indices has space for SPX_TFORS_K integers.
 */
void message_to_indices(uint32_t *indices, const unsigned char *m, const spx_ctx *ctx) {
    uint32_t indices1[SPX_TFORS_K];
    uint32_t indices2[SPX_TFORS_K];
    unsigned int i, j;
    
    /* Derive indices1 using H2 from first SPX_N bytes of message */
    h2_generate_indices(indices1, m, ctx);
    
    /* Extract indices2 from the remaining bits (after SPX_N bytes) */
    unsigned int offset = 0;
    
    for (i = 0; i < SPX_TFORS_K; i++) {
        indices2[i] = 0;
        for (j = 0; j < SPX_TFORS_A; j++) {
            indices2[i] |= ((m[offset >> 3] >> (offset & 0x7)) & 1u) << (unsigned int)j;
            offset++;
        }
        indices[i] = indices1[i] + SPX_TFORS_K_PRIME * indices2[i];
    }

    sort_indices(indices);
}

/**
 * Signs a message m, deriving the secret key from sk_seed and the FTS address.
 * Assumes m contains at least MAX{8*SPX_N, SPX_TFORS_A * SPX_TFORS_K} bits.
 */
void tfors_sign(unsigned char *sig, unsigned char *pk,
               const uint32_t *indices,
               const spx_ctx *ctx,
               const uint32_t tfors_addr[8])
{
    uint32_t tfors_tree_addr[8] = {0};
    unsigned char leaf_sk[SPX_TFORS_K][SPX_N];
    unsigned int i;

    copy_keypair_addr(tfors_tree_addr, tfors_addr);

    // generate leaf keys
    for (i = 0; i < SPX_TFORS_K; i++) {
        set_tree_height(tfors_tree_addr, 0);
        set_tree_index(tfors_tree_addr, indices[i]);
        set_type(tfors_tree_addr, SPX_ADDR_TYPE_TFORSPRF);

        /* Include the secret key part that produces the selected leaf node. */
        tfors_gen_sk(sig, ctx, tfors_tree_addr);
        set_type(tfors_tree_addr, SPX_ADDR_TYPE_TFORSTREE);
        memcpy(leaf_sk[i], sig, SPX_N);
        sig += SPX_N;
    }
    
    // generate auth paths
    octopus_compute_auth_paths(pk, sig, indices, ctx, tfors_tree_addr);
}

void tfors_pk_from_sig(unsigned char *pk,
                      const unsigned char *sig, const unsigned char *m,
                      const spx_ctx* ctx,
                      const uint32_t tfors_addr[8])
{
    uint32_t indices[SPX_TFORS_K];
    unsigned char leaf[SPX_TFORS_K][SPX_N];
    uint32_t tfors_leaf_addr[8] = {0};
    const unsigned char *sig_ptr = sig;

    copy_keypair_addr(tfors_leaf_addr, tfors_addr);
    set_type(tfors_leaf_addr, SPX_ADDR_TYPE_TFORSTREE);
    set_tree_height(tfors_leaf_addr, 0);

    message_to_indices(indices, m, ctx);

    // generate leaf hashes
    for (uint32_t i = 0; i < SPX_TFORS_K; i++) {
        set_tree_index(tfors_leaf_addr, indices[i]);        
        tfors_sk_to_leaf(leaf[i], sig_ptr, ctx, tfors_leaf_addr);
        sig_ptr += SPX_N;
    }

    // generate auth indices
    octopus_auth auth;
    octopus_compute(&auth, indices);
    octopus_recompute_root(pk, sig_ptr, indices, SPX_TFORS_K, &auth,
                          (const unsigned char*)leaf,
                          ctx, tfors_leaf_addr);
}
