/* Phoenix utility functions
 *
 * Common helper functions for Phoenix signature scheme.
 */

#include <string.h>

#include "utils.h"
#include "params.h"
#include "hash.h"
#include "thash.h"
#include "address.h"

/* Convert unsigned long long to big-endian byte array */
void ull_to_bytes(unsigned char *out_bytes, unsigned int out_len,
                  unsigned long long val)
{
    int byte_idx;

    /* Write bytes from LSB to MSB for big-endian order */
    for (byte_idx = (signed int)out_len - 1; byte_idx >= 0; byte_idx--) {
        out_bytes[byte_idx] = val & 0xff;
        val = val >> 8;
    }
}

/* Convert uint32 to 4-byte big-endian array */
void u32_to_bytes(unsigned char *out_bytes, uint32_t val)
{
    out_bytes[0] = (unsigned char)(val >> 24);
    out_bytes[1] = (unsigned char)(val >> 16);
    out_bytes[2] = (unsigned char)(val >> 8);
    out_bytes[3] = (unsigned char)val;
}

/* Convert big-endian byte array to unsigned long long */
unsigned long long bytes_to_ull(const unsigned char *in_bytes, unsigned int in_len)
{
    unsigned long long result = 0;
    unsigned int byte_idx;

    for (byte_idx = 0; byte_idx < in_len; byte_idx++) {
        result = (result << 8) | in_bytes[byte_idx];
    }
    return result;
}

/* Compute root node from leaf and authentication path */
void compute_root(unsigned char *root, const unsigned char *leaf,
                  uint32_t leaf_idx, uint32_t idx_offset,
                  const unsigned char *auth_path, uint32_t tree_height,
                  const spx_ctx *ctx, uint32_t addr[8])
{
    uint32_t layer_idx;
    unsigned char node_buf[2 * SPX_N];
    const unsigned char *path_ptr = auth_path;

    /* Place leaf and first auth path node based on leaf position parity */
    if (leaf_idx & 1) {
        memcpy(node_buf + SPX_N, leaf, SPX_N);
        memcpy(node_buf, path_ptr, SPX_N);
    } else {
        memcpy(node_buf, leaf, SPX_N);
        memcpy(node_buf + SPX_N, path_ptr, SPX_N);
    }
    path_ptr += SPX_N;

    /* Hash up the tree combining nodes with auth path */
    for (layer_idx = 0; layer_idx < tree_height - 1; layer_idx++) {
        leaf_idx >>= 1;
        idx_offset >>= 1;
        set_tree_height(addr, layer_idx + 1);
        set_tree_index(addr, leaf_idx + idx_offset);

        /* Combine with next auth path node based on position parity */
        if (leaf_idx & 1) {
            thash(node_buf + SPX_N, node_buf, 2, ctx, addr);
            memcpy(node_buf, path_ptr, SPX_N);
        } else {
            thash(node_buf, node_buf, 2, ctx, addr);
            memcpy(node_buf + SPX_N, path_ptr, SPX_N);
        }
        path_ptr += SPX_N;
    }

    /* Final root hash without auth path node */
    leaf_idx >>= 1;
    idx_offset >>= 1;
    set_tree_height(addr, tree_height);
    set_tree_index(addr, leaf_idx + idx_offset);
    thash(root, node_buf, 2, ctx, addr);
}

/* Compute Merkle tree root and authentication path using TreeHash algorithm */
void treehash(unsigned char *root, unsigned char *auth_path, const spx_ctx* ctx,
              uint32_t leaf_idx, uint32_t idx_offset, uint32_t tree_height,
              void (*gen_leaf)(
                 unsigned char* /* leaf */,
                 const spx_ctx* /* ctx */,
                 uint32_t /* addr_idx */, const uint32_t[8] /* tree_addr */),
              uint32_t tree_addr[8])
{
    SPX_VLA(uint8_t, stack, (tree_height+1)*SPX_N);
    SPX_VLA(unsigned int, node_heights, tree_height+1);
    unsigned int stack_pos = 0;
    uint32_t leaf_pos;
    uint32_t parent_idx;

    for (leaf_pos = 0; leaf_pos < (uint32_t)(1 << tree_height); leaf_pos++) {
        /* Generate and push next leaf node onto stack */
        gen_leaf(stack + stack_pos*SPX_N, ctx, leaf_pos + idx_offset, tree_addr);
        stack_pos++;
        node_heights[stack_pos - 1] = 0;

        /* Check if this leaf is needed for auth path */
        if ((leaf_idx ^ 0x1) == leaf_pos) {
            memcpy(auth_path, stack + (stack_pos - 1)*SPX_N, SPX_N);
        }

        /* Merge equal-height nodes on stack */
        while (stack_pos >= 2 && node_heights[stack_pos - 1] == node_heights[stack_pos - 2]) {
            /* Compute parent node index */
            parent_idx = (leaf_pos >> (node_heights[stack_pos - 1] + 1));

            /* Set address for parent node hash */
            set_tree_height(tree_addr, node_heights[stack_pos - 1] + 1);
            set_tree_index(tree_addr,
                           parent_idx + (idx_offset >> (node_heights[stack_pos-1] + 1)));

            /* Hash top two stack nodes into parent */
            thash(stack + (stack_pos - 2)*SPX_N,
                  stack + (stack_pos - 2)*SPX_N, 2, ctx, tree_addr);
            stack_pos--;
            node_heights[stack_pos - 1]++;

            /* Check if parent node is needed for auth path */
            if (((leaf_idx >> node_heights[stack_pos - 1]) ^ 0x1) == parent_idx) {
                memcpy(auth_path + node_heights[stack_pos - 1]*SPX_N,
                       stack + (stack_pos - 1)*SPX_N, SPX_N);
            }
        }
    }
    memcpy(root, stack, SPX_N);
}