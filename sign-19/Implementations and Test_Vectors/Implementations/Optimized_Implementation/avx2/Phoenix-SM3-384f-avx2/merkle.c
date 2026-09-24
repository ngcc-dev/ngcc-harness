#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "utilsx1.h"
#include "utilsx4.h"
#include "gwots.h"
#include "gwotsx1.h"
#include "thash.h"
#include "merkle.h"
#include "address.h"
#include "params.h"

/*
 * This generates a Merkle signature (GWOTS signature followed by the Merkle
 * authentication path).  This is in this file because most of the complexity
 * is involved with the GWOTS signature; the Merkle authentication path logic
 * is mostly hidden in treehashx8
 */
void merkle_sign(uint8_t *sig, unsigned char *root,
                 const spx_ctx *ctx,
                 uint32_t gwots_addr[8], uint32_t tree_addr[8],
                 uint32_t idx_leaf, uint32_t *counter_out)
{
#define MAX_HASH_TRIALS_GWOTS (1 << (20))
    unsigned char *auth_path = sig + SPX_GWOTS_BYTES;
    struct leaf_info_x4 info = {0};
    unsigned steps[SPX_GWOTS_LEN];
    unsigned char bitmask[SPX_N];
    uint32_t tree_addrx4[4 * 8] = {0};
    int j;

    /*Initial paramaters for custom thash & counter search*/
    unsigned char digest[SPX_N];
    uint32_t counter = 0;
    int csum;
    uint32_t to_sign = ~0;

    /*Initialize parameters for actual sign*/
    set_type(&tree_addr[0], SPX_ADDR_TYPE_HASHTREE);
    for (j = 0; j < 4; j++) {
        set_type(&tree_addrx4[8 * j], SPX_ADDR_TYPE_HASHTREE);
        set_type(&info.leaf_addr[8 * j], SPX_ADDR_TYPE_GWOTS);
        set_type(&info.pk_addr[8 * j], SPX_ADDR_TYPE_GWOTSPK);
        copy_subtree_addr(&tree_addrx4[8 * j], tree_addr);
        copy_subtree_addr(&info.leaf_addr[8 * j], gwots_addr);
        copy_subtree_addr(&info.pk_addr[8 * j], gwots_addr);
    }

    /* Code for counter search */
    *counter_out = 0;
    if (idx_leaf != to_sign)
    {
        /*Set thash address for custom hash*/
        uint32_t *pk_addr = info.pk_addr;
        set_keypair_addr(pk_addr, idx_leaf);
        set_type(pk_addr, SPX_ADDR_TYPE_COMPRESS_GWOTS);
        thash_init_bitmask(bitmask, 1, ctx, pk_addr);

        /*Search for correct counter */
        while (1)
        {
            counter++;
            if (counter > MAX_HASH_TRIALS_GWOTS)
                return;
            ull_to_bytes(((unsigned char *)(pk_addr)) + (SPX_OFFSET_COUNTER), COUNTER_SIZE, counter);
            thash_fin(digest, root, 1, ctx, pk_addr, bitmask);
            csum = chain_lengths(steps, digest);

            if (csum >= GWOTS_SUM_BASE && csum <= (GWOTS_SUM_BASE + GWOTS_SUM_RANGE))
            {
                if (SPX_GWOTS_LEN2 > 0) {
                    steps[SPX_GWOTS_LEN1] = (unsigned int)(csum - GWOTS_SUM_BASE);
                }

                *counter_out = counter;
                break;
            }
        }

        /*Restore initial parameters for tree hash*/
        set_type(pk_addr, SPX_ADDR_TYPE_GWOTSPK);
        ull_to_bytes(((unsigned char *)(pk_addr)) + (SPX_OFFSET_COUNTER), COUNTER_SIZE, 0);
    }
    /* In this case we only try to generate the PK so no need to find the counter */
    else
    {
        chain_lengths(steps, root);
        if (SPX_GWOTS_LEN2 > 0)
        {
            steps[SPX_GWOTS_LEN1] = 0;
        }
    }
    info.gwots_sig = sig;
    info.gwots_steps = steps;

    info.gwots_sign_leaf = idx_leaf;

    treehashx4(root, auth_path, ctx,
               idx_leaf, 0,
               SPX_TREE_HEIGHT,
               gen_leafx4,
               tree_addrx4, &info);
}

/* Compute root node of the top-most subtree. */
void merkle_gen_root(unsigned char *root, const spx_ctx *ctx)
{
    /* We do not need the auth path in key generation, but it simplifies the
       code to have just one treehash routine that computes both root and path
       in one function. */
    unsigned char auth_path[SPX_TREE_HEIGHT * SPX_N + SPX_GWOTS_BYTES];
    uint32_t top_tree_addr[8] = {0};
    uint32_t gwots_addr[8] = {0};
    uint32_t counter;

    set_layer_addr(top_tree_addr, SPX_D - 1);
    set_layer_addr(gwots_addr, SPX_D - 1);

    merkle_sign(auth_path, root, ctx,
                gwots_addr, top_tree_addr,
                ~0 /* ~0 means "don't bother generating an auth path */,
                &counter);
}