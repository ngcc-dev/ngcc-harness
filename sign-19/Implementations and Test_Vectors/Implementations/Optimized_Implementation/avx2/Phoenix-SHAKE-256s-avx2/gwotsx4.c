#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "hash.h"
#include "hashx4.h"
#include "thash.h"
#include "thashx4.h"
#include "gwots.h"
#include "gwotsx4.h"
#include "address.h"
#include "params.h"

/* Generate 4 parallel GWOTS public keys with optional signature. */
void gwots_gen_leafx4(unsigned char *dest,
                   const spx_ctx *ctx,
                   uint32_t leaf_idx, void *v_info)
{
    struct leaf_info_x4 *info = v_info;
    uint32_t *leaf_addr = info->leaf_addr;
    uint32_t *pk_addr = info->pk_addr;
    unsigned int chain_idx, step_idx, lane_idx;
    unsigned char pk_buf[ 4 * SPX_GWOTS_BYTES ];
    unsigned gwots_stride = SPX_GWOTS_BYTES;
    unsigned char *chain_buf;
    uint32_t gwots_k_mask;
    unsigned gwots_sign_lane;

    /* Determine signing mode and signature lane */
    if (((leaf_idx ^ info->gwots_sign_leaf) & ~3) == 0) {
        gwots_k_mask = 0;
        gwots_sign_lane = info->gwots_sign_leaf & 3;
    } else {
        gwots_k_mask = (uint32_t)~0;
        gwots_sign_lane = 0;
    }

    /* Initialize address registers for 4 lanes */
    for (lane_idx = 0; lane_idx < 4; lane_idx++) {
        set_keypair_addr(leaf_addr + lane_idx*8, leaf_idx + lane_idx);
        set_keypair_addr(pk_addr + lane_idx*8, leaf_idx + lane_idx);
    }

    /* Process each chain position */
    for (chain_idx = 0, chain_buf = pk_buf; chain_idx < SPX_GWOTS_LEN; chain_idx++, chain_buf += SPX_N) {
        /* Determine chain length parameter */
        uint32_t chain_w;
        if (chain_idx < SPX_GWOTS_W1_LEN) {
            chain_w = SPX_GWOTS_W1;
        } else if (chain_idx < SPX_GWOTS_LEN1) {
            chain_w = SPX_GWOTS_W2;
        } else {
            chain_w = SPX_GWOTS_CHECKSUM_W;
        }

        uint32_t gwots_k_val = info->gwots_steps[chain_idx] | gwots_k_mask;

        /* Initialize chains with secret seeds */
        for (lane_idx = 0; lane_idx < 4; lane_idx++) {
            set_chain_addr(leaf_addr + lane_idx*8, chain_idx);
            set_hash_addr(leaf_addr + lane_idx*8, 0);
            set_addr_type(leaf_addr + lane_idx*8, SPX_ADDR_TYPE_GWOTSPRF);
        }
        prf_addrx4(chain_buf + 0*gwots_stride,
                   chain_buf + 1*gwots_stride,
                   chain_buf + 2*gwots_stride,
                   chain_buf + 3*gwots_stride,
                   ctx, leaf_addr);

        for (lane_idx = 0; lane_idx < 4; lane_idx++) {
            set_addr_type(leaf_addr + lane_idx*8, SPX_ADDR_TYPE_GWOTS);
        }

        /* Iterate down the chain */
        for (step_idx = 0; ; step_idx++) {
            /* Save signature at specified step */
            if (step_idx == gwots_k_val) {
                memcpy(info->gwots_sig + chain_idx * SPX_N,
                       chain_buf + gwots_sign_lane*gwots_stride, SPX_N);
            }

            /* Stop at chain top */
            if (step_idx == chain_w - 1) break;

            /* Parallel thash for all 4 lanes */
            for (lane_idx = 0; lane_idx < 4; lane_idx++) {
                set_hash_addr(leaf_addr + lane_idx*8, step_idx);
            }
            thashx4(chain_buf + 0*gwots_stride,
                    chain_buf + 1*gwots_stride,
                    chain_buf + 2*gwots_stride,
                    chain_buf + 3*gwots_stride,
                    chain_buf + 0*gwots_stride,
                    chain_buf + 1*gwots_stride,
                    chain_buf + 2*gwots_stride,
                    chain_buf + 3*gwots_stride, 1, ctx, leaf_addr);
        }
    }

    /* Final compression for 4 parallel public keys */
    thashx4(dest + 0*SPX_N,
            dest + 1*SPX_N,
            dest + 2*SPX_N,
            dest + 3*SPX_N,
            pk_buf + 0*gwots_stride,
            pk_buf + 1*gwots_stride,
            pk_buf + 2*gwots_stride,
            pk_buf + 3*gwots_stride, SPX_GWOTS_LEN, ctx, pk_addr);
}
