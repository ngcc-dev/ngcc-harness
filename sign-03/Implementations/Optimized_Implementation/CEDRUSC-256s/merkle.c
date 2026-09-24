#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "utils_avx2.h"
#include "wots.h"
#include "wots_avx2.h"
#include "merkle.h"
#include "address.h"
#include "params.h"
#include "thash.h"
#include "thashx8.h"


/*
 * This generates a Merkle signature (WOTS signature followed by the Merkle
 * authentication path).  This is in this file because most of the complexity
 * is involved with the WOTS signature; the Merkle authentication path logic
 * is mostly hidden in treehash_avx2.
 */ 
void merkle_sign(uint8_t *sig, unsigned char *root,
                 const spx_ctx *ctx,
                 uint32_t wots_addr[8], uint32_t tree_addr[8],
                 uint32_t idx_leaf, uint32_t *counter_out, uint32_t merkle_tree_height)
{
#define MAX_HASH_TRIALS_WOTS (1 << (20))
    unsigned char *auth_path = sig + SPX_WOTS_BYTES;
    uint32_t tree_addr_batch[4*8] = { 0 };
    struct leaf_info_avx2 info = { 0 };
    unsigned steps[ SPX_WOTS_LEN ];
    unsigned char bitmask[SPX_N];
    int j;

    /*Initial paramaters for custom thash & counter search*/
    unsigned char digest[SPX_N];
    uint32_t counter = 0;
    int csum;
    uint32_t to_sign = ~0;
    uint32_t mask =  (~0U << (8-WOTS_ZERO_BITS));

    /*Initialize parameters for actual sign*/
    for (j = 0; j < 4; j++) {
        set_type(tree_addr_batch + 8*j, SPX_ADDR_TYPE_HASHTREE);
        set_type(info.leaf_addr + 8*j, SPX_ADDR_TYPE_WOTS);
        set_type(info.pk_addr + 8*j, SPX_ADDR_TYPE_WOTSPK);
        copy_subtree_addr(tree_addr_batch + 8*j, tree_addr);
        copy_subtree_addr(info.leaf_addr + 8*j, wots_addr);
        copy_subtree_addr(info.pk_addr + 8*j, wots_addr);
    }

    /* Code for counter search */
    *counter_out = 0;
    if (idx_leaf != to_sign) {
        /*Set thash address for custom hash*/
        uint32_t pk_addr[8] = {0};
        copy_subtree_addr(pk_addr, wots_addr);
        set_keypair_addr(pk_addr, idx_leaf);
        set_type(pk_addr, SPX_ADDR_TYPE_COMPRESS_WOTS);
        thash_init_bitmask(bitmask, 1, ctx, pk_addr);

        /*Search for correct counter */
        while (1) {
            if (counter <= MAX_HASH_TRIALS_WOTS - 8u) {
                unsigned char digestx8[8][SPX_N];
                uint32_t pk_addrx8[8 * 8];

                for (unsigned int lane = 0; lane < 8; lane++) {
                    memcpy(pk_addrx8 + 8 * lane, pk_addr, 8 * sizeof(uint32_t));
                    ull_to_bytes(((unsigned char *)(pk_addrx8 + 8 * lane)) + SPX_OFFSET_COUNTER,
                                 COUNTER_SIZE, counter + 1u + lane);
                }

                thashx8(digestx8[0], digestx8[1], digestx8[2], digestx8[3],
                        digestx8[4], digestx8[5], digestx8[6], digestx8[7],
                        root, root, root, root, root, root, root, root,
                        1, ctx, pk_addrx8);

                for (unsigned int lane = 0; lane < 8; lane++) {
                    if (((digestx8[lane][SPX_N - 1]) & (mask)) == 0) {
                        csum = chain_lengths(steps, digestx8[lane]);
                        if (csum == WANTED_CHECKSUM) {
                            counter += 1u + lane;
                            *counter_out = counter;
                            goto counter_found;
                        }
                    }
                }

                counter += 8u;
                continue;
            }
            counter++;
            if (counter > MAX_HASH_TRIALS_WOTS)
                return;
            ull_to_bytes(((unsigned char *) (pk_addr))+(SPX_OFFSET_COUNTER) , COUNTER_SIZE, counter);
            thash_fin(digest, root, 1, ctx, pk_addr, bitmask); 
            if (((digest[SPX_N-1]) & (mask))==0)
            {
                csum = chain_lengths(steps, digest);
                if (csum == WANTED_CHECKSUM) 
                {
                    *counter_out = counter;
                    break;
                }
            }

        }
counter_found:
        ;
    }
    /* In this case we only try to generate the PK so no need to find the counter */
    else
    {
        memset(steps, 0, sizeof(steps));
    }
    info.wots_sig = sig;
    info.wots_steps = steps;

    info.wots_sign_leaf = idx_leaf;

    treehash_avx2(root, auth_path, ctx,
                  idx_leaf, 0,
                  merkle_tree_height,
                  wots_gen_leaf_avx2,
                  tree_addr_batch, &info);
}

/* Compute root node of the top-most subtree. */
void merkle_gen_root(unsigned char *root, const spx_ctx *ctx)
{
    /* We do not need the auth path in key generation, but it simplifies the
       code to have just one treehash routine that computes both root and path
       in one function. */
    unsigned char auth_path[SPX_TREE_HEIGHT * SPX_N + SPX_WOTS_BYTES];
    uint32_t top_tree_addr[8] = {0};
    uint32_t wots_addr[8] = {0};
    uint32_t counter;

    set_layer_addr(top_tree_addr, SPX_D - 1);
    set_layer_addr(wots_addr, SPX_D - 1);

    merkle_sign(auth_path, root, ctx,
                wots_addr, top_tree_addr,
                ~0 /* ~0 means "don't bother generating an auth path */,
                &counter,SPX_TREE_HEIGHT);
}
