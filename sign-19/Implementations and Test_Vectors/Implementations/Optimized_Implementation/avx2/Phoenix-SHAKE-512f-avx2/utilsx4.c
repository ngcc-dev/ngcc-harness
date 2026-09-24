/* Phoenix 4-way parallel treehash utility
 *
 * Compute Merkle tree root and authentication path using 4-way AVX2 parallel TreeHash.
 * Each logical node represents 4 consecutive real tree nodes.
 */

#include <string.h>

#include "utils.h"
#include "utilsx4.h"
#include "params.h"
#include "thashx4.h"
#include "address.h"

/* Compute Merkle root and auth path using 4-way parallel TreeHash */
void treehashx4(unsigned char *root, unsigned char *auth_path,
                const spx_ctx *ctx,
                uint32_t leaf_idx, uint32_t idx_offset,
                uint32_t tree_height,
                void (*gen_leafx4)(
                   unsigned char* /* Where to write the leaves */,
                   const spx_ctx*,
                   uint32_t idx, void *info),
                uint32_t tree_addrx4[4*8],
                void *info)
{
    SPX_VLA(unsigned char, node_stackx4, tree_height * 4 * SPX_N);
    uint32_t left_offset = 0, prev_left_offset = 0;

    uint32_t logical_idx;
    uint32_t last_logical_idx = (1 << (tree_height-2)) - 1;
    for (logical_idx = 0;; logical_idx++) {
        unsigned char node_buf[4*SPX_N];
        gen_leafx4(node_buf, ctx, 4*logical_idx + idx_offset, info);

        uint32_t node_offset = idx_offset;
        uint32_t node_idx = logical_idx;
        uint32_t target_leaf = leaf_idx;
        uint32_t layer_idx;

        for (layer_idx = 0;; layer_idx++, node_idx >>= 1, target_leaf >>= 1) {
            if (layer_idx >= tree_height - 2) {
                if (layer_idx == tree_height) {
                    memcpy(root, &node_buf[3*SPX_N], SPX_N);
                    return;
                }
                prev_left_offset = left_offset;
                left_offset = 4 - (1 << (tree_height - layer_idx - 1));
            }

            if (layer_idx == tree_height) {
                memcpy(root, &node_buf[3*SPX_N], SPX_N);
                return;
            }

            if ((((node_idx << 2) ^ target_leaf) & ~0x3) == 0) {
                memcpy(&auth_path[layer_idx * SPX_N],
                       &node_buf[(((target_leaf&3)^1) + prev_left_offset) * SPX_N],
                       SPX_N);
            }

            if ((node_idx & 1) == 0 && logical_idx < last_logical_idx) {
                break;
            }

            node_offset >>= 1;
            int lane_idx;
            for (lane_idx = 0; lane_idx < 4; lane_idx++) {
                set_tree_height(tree_addrx4 + lane_idx*8, layer_idx + 1);
                set_tree_index(tree_addrx4 + lane_idx*8,
                     (4/2) * (node_idx&~1) + lane_idx - left_offset + node_offset);
            }

            unsigned char *left_node = &node_stackx4[layer_idx * 4 * SPX_N];
            thashx4(&node_buf[0 * SPX_N],
                    &node_buf[1 * SPX_N],
                    &node_buf[2 * SPX_N],
                    &node_buf[3 * SPX_N],
                    &left_node[0 * SPX_N],
                    &left_node[2 * SPX_N],
                    &node_buf[0 * SPX_N],
                    &node_buf[2 * SPX_N],
                    2, ctx, tree_addrx4);
        }

        memcpy(&node_stackx4[layer_idx * 4 * SPX_N], node_buf, 4 * SPX_N);
    }
}