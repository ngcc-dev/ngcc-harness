#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "tfors.h"
#include "hash.h"
#include "hashx4.h"
#include "thash.h"
#include "thashx4.h"
#include "address.h"
#include "octopus.h"

/* Generate TFORS secret key using PRF */
static void tfors_gen_sk(unsigned char *sk, const spx_ctx *ctx,
                        uint32_t tfors_leaf_addr[8])
{
    prf_addr(sk, ctx, tfors_leaf_addr);
}

/* Convert TFORS secret key to leaf node via thash */
void tfors_sk_to_leaf(unsigned char *leaf, const unsigned char *sk,
                            const spx_ctx *ctx,
                            uint32_t tfors_leaf_addr[8])
{
    thash(leaf, sk, 1, ctx, tfors_leaf_addr);
}

/* Generate 4 TFORS secret keys in parallel using PRF */
static void tfors_gen_skx4(unsigned char *sk0,
                           unsigned char *sk1,
                           unsigned char *sk2,
                           unsigned char *sk3,
                           const spx_ctx *ctx,
                           uint32_t tfors_leaf_addrx4[4*8])
{
    prf_addrx4(sk0, sk1, sk2, sk3, ctx, tfors_leaf_addrx4);
}

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
                        uint32_t tfors_leaf_addrx4[4*8])
{
    thashx4(leaf0, leaf1, leaf2, leaf3,
            sk0, sk1, sk2, sk3,
            1, ctx, tfors_leaf_addrx4);
}

struct tfors_gen_leaf_info {
    uint32_t leaf_addrx[8];
};

/* Insertion sort for leaf indices */
static void sort_indices(uint32_t *indices)
{
    for (uint32_t pos_idx = 1; pos_idx < SPX_TFORS_K; pos_idx++) {
        uint32_t cur_val = indices[pos_idx];
        int cmp_idx = pos_idx - 1;
        while (cmp_idx >= 0 && indices[cmp_idx] > cur_val) {
            indices[cmp_idx + 1] = indices[cmp_idx];
            cmp_idx--;
        }
        indices[cmp_idx + 1] = cur_val;
    }
}

/* Derive TFORS leaf indices from message using H2 and bit extraction */
void message_to_indices(uint32_t *indices, const unsigned char *msg, const spx_ctx *ctx)
{
    uint32_t indices_h2[SPX_TFORS_K];
    uint32_t indices_bits[SPX_TFORS_K];
    unsigned int tree_idx, bit_idx;
    unsigned int bit_offset = 0;

    /* First layer: H2 hash from first SPX_N bytes */
    h2_generate_indices(indices_h2, msg, ctx);

    /* Second layer: direct bit extraction */
    for (tree_idx = 0; tree_idx < SPX_TFORS_K; tree_idx++) {
        indices_bits[tree_idx] = 0;
        for (bit_idx = 0; bit_idx < SPX_TFORS_A; bit_idx++) {
            indices_bits[tree_idx] |= ((msg[bit_offset >> 3] >> (bit_offset & 0x7)) & 1u) << (unsigned int)bit_idx;
            bit_offset++;
        }
        indices[tree_idx] = indices_h2[tree_idx] + SPX_TFORS_K_PRIME * indices_bits[tree_idx];
    }

    sort_indices(indices);
}

/* Generate TFORS signature with secret keys and authentication paths */
void tfors_sign(unsigned char *sig, unsigned char *pk,
               const uint32_t *indices,
               const spx_ctx *ctx,
               const uint32_t tfors_addr[8])
{
    uint32_t tfors_tree_addr[8] = {0};
    unsigned char leaf_sk[SPX_TFORS_K][SPX_N];
    unsigned int key_idx;

    copy_keypair_addr(tfors_tree_addr, tfors_addr);

    /* Generate secret keys for selected leaves */
    for (key_idx = 0; key_idx < SPX_TFORS_K; key_idx++) {
        set_tree_height(tfors_tree_addr, 0);
        set_tree_index(tfors_tree_addr, indices[key_idx]);
        set_addr_type(tfors_tree_addr, SPX_ADDR_TYPE_TFORSPRF);

        tfors_gen_sk(sig, ctx, tfors_tree_addr);
        set_addr_type(tfors_tree_addr, SPX_ADDR_TYPE_TFORSTREE);
        memcpy(leaf_sk[key_idx], sig, SPX_N);
        sig += SPX_N;
    }

    /* Compute authentication paths using octopus structure */
    octopus_compute_auth_pathsx4(pk, sig, indices, ctx, tfors_tree_addr);
}

/* Verify TFORS signature and recover public key */
void tfors_pk_from_sig(unsigned char *pk,
                      const unsigned char *sig, const unsigned char *msg,
                      const spx_ctx* ctx,
                      const uint32_t tfors_addr[8])
{
    uint32_t indices[SPX_TFORS_K];
    unsigned char leaf[SPX_TFORS_K][SPX_N];
    uint32_t tfors_leaf_addr[8] = {0};
    const unsigned char *sig_ptr = sig;

    copy_keypair_addr(tfors_leaf_addr, tfors_addr);
    set_addr_type(tfors_leaf_addr, SPX_ADDR_TYPE_TFORSTREE);
    set_tree_height(tfors_leaf_addr, 0);

    message_to_indices(indices, msg, ctx);

    /* Compute leaf hashes from signature secret keys */
    for (uint32_t leaf_idx = 0; leaf_idx < SPX_TFORS_K; leaf_idx++) {
        set_tree_index(tfors_leaf_addr, indices[leaf_idx]);
        tfors_sk_to_leaf(leaf[leaf_idx], sig_ptr, ctx, tfors_leaf_addr);
        sig_ptr += SPX_N;
    }

    /* Reconstruct root using octopus authentication structure */
    octopus_auth auth;
    octopus_compute(&auth, indices);
    octopus_recompute_root(pk, sig_ptr, indices, SPX_TFORS_K, &auth,
                          (const unsigned char*)leaf,
                          ctx, tfors_leaf_addr);
}