/* Phoenix single-lane treehash utility
 *
 * Compute Merkle tree root and authentication path using TreeHash.
 */

#include <string.h>

#include "utils.h"
#include "utilsx1.h"
#include "params.h"
#include "thash.h"
#include "address.h"

/* Compute Merkle root and auth path using single-lane TreeHash */
void treehashx1(unsigned char *root, unsigned char *auth_path,
                const spx_ctx* ctx,
                uint32_t leaf_idx, uint32_t idx_offset,
                uint32_t tree_height,
                void (*gen_leaf)(
                   unsigned char* /* Where to write the leaves */,
                   const spx_ctx* /* ctx */,
                   uint32_t idx, void *info),
                uint32_t tree_addr[8],
                void *info)
{
    SPX_VLA(uint8_t, node_stack, tree_height*SPX_N);

    uint32_t leaf_pos;
    uint32_t last_leaf_idx = (uint32_t)((1 << tree_height) - 1);
    for (leaf_pos = 0;; leaf_pos++) {
        unsigned char node_buf[2*SPX_N];
        gen_leaf(&node_buf[SPX_N], ctx, leaf_pos + idx_offset, info);

        uint32_t node_offset = idx_offset;
        uint32_t node_idx = leaf_pos;
        uint32_t target_leaf = leaf_idx;
        uint32_t layer_idx;

        for (layer_idx = 0;; layer_idx++, node_idx >>= 1, target_leaf >>= 1) {
            if (layer_idx == tree_height) {
                memcpy(root, &node_buf[SPX_N], SPX_N);
                return;
            }

            if ((node_idx ^ target_leaf) == 0x01) {
                memcpy(&auth_path[layer_idx * SPX_N], &node_buf[SPX_N], SPX_N);
            }

            if ((node_idx & 1) == 0 && leaf_pos < last_leaf_idx) {
                break;
            }

            node_offset >>= 1;
            set_tree_height(tree_addr, layer_idx + 1);
            set_tree_index(tree_addr, node_idx/2 + node_offset);

            unsigned char *left_node = &node_stack[layer_idx * SPX_N];
            memcpy(&node_buf[0], left_node, SPX_N);
            thash(&node_buf[1 * SPX_N], &node_buf[0 * SPX_N], 2, ctx, tree_addr);
        }

        memcpy(&node_stack[layer_idx * SPX_N], &node_buf[SPX_N], SPX_N);
    }
}