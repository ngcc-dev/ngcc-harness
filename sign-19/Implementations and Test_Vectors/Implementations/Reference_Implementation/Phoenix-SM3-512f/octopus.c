#include "octopus.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "context.h"
#include "address.h"
#include "thash.h"
#include "utils.h"
#include "tfors.h"

// remove duplicates
static uint32_t Deduplicate(uint32_t *indices, uint32_t len) {
    uint32_t idx = 1;
    if (len == 0) return 0;
    
    for (uint32_t i = 1; i < len; i++) {
        if (indices[i] != indices[i-1]) {
            indices[idx++] = indices[i];
        }
    }
    return idx;
}

void octopus_compute_auth_count(const uint32_t *indices, uint32_t len, uint32_t *auth_count) {
    uint32_t current[SPX_TFORS_K];
    uint32_t current_len = len, parent_len = 0;
    uint32_t count = 0;

    memcpy(current, indices, len * sizeof(uint32_t));

    for (uint32_t level = 0; level < SPX_TFORS_HEIGHT; level++) {
        uint32_t i = 0;
        parent_len = 0;

        while (i < current_len) {
            uint32_t idx = current[i];

            if (i + 1 < current_len && current[i + 1] == (idx ^ 1)) {
                i += 2;
            } else {
                count++;
                i += 1;
            }
            current[parent_len++] = idx / 2;
        }
        if (parent_len == 0) break;
        current_len = Deduplicate(current, parent_len);
    }
    *auth_count = count;
}

void octopus_compute(octopus_auth *auth,
                     const uint32_t *indices)
{
    uint32_t current_indices[SPX_TFORS_K];
    uint32_t parent_indices[SPX_TFORS_K];
    uint32_t current_len = SPX_TFORS_K;
    uint32_t tfors_height = SPX_TFORS_HEIGHT;

    memcpy(current_indices, indices, SPX_TFORS_K * sizeof(uint32_t));   
    auth->count = 0;
    
    // process from leaf layer to root layer    
    // add sibling nodes to auth indices
    for (uint32_t level = 0; level < tfors_height; level++) {

        uint32_t parent_len = 0;
        uint32_t i = 0;
        
        while (i < current_len) {
            uint32_t node_idx = current_indices[i];
            uint32_t parent_idx = node_idx / 2;
            uint32_t sibling_idx = node_idx ^ 1;
            
            // add parent node to auth indices
            parent_indices[parent_len++] = parent_idx;
            
            // check if sibling node is an auth index   
            if (i + 1 < current_len && current_indices[i + 1] == sibling_idx) {
                i += 2;
            } else {
                if (level < tfors_height) {
                    auth->entries[auth->count].level = level;
                    auth->entries[auth->count].index = sibling_idx;
                    auth->count++;
                }
                i += 1;
            }
        }
        
        if (level == tfors_height - 1) {
            break;
        }
        
        if (parent_len > 0) {
            parent_len = Deduplicate(parent_indices, parent_len);
            memcpy(current_indices, parent_indices, parent_len * sizeof(uint32_t));
            current_len = parent_len;
        } else {
            return;
        }
    }
}   

void octopus_compute_auth_paths(unsigned char root[SPX_N],
                                unsigned char *sig,
                                const uint32_t *indices,
                                const spx_ctx *ctx,
                                uint32_t tree_addr[8])
{
    uint32_t tree_height = SPX_TFORS_HEIGHT;
    uint32_t max_idx = (1u << tree_height) - 1;

    octopus_auth auth;
    octopus_compute(&auth, indices);

    /* This is where we keep the intermediate nodes */
    SPX_VLA(uint8_t, stack, tree_height*SPX_N);

    
    for (uint32_t idx = 0; idx <= max_idx; idx++) {
        unsigned char current[2 * SPX_N];
        unsigned char sk[SPX_N];
            
        set_tree_height(tree_addr, 0);
        set_tree_index(tree_addr, idx);
        set_type(tree_addr, SPX_ADDR_TYPE_TFORSPRF);
        prf_addr(sk, ctx, tree_addr);
            
        set_type(tree_addr, SPX_ADDR_TYPE_TFORSTREE);
        tfors_sk_to_leaf(&current[SPX_N], sk, ctx, tree_addr);
        
        // traverse upward
        uint32_t node_idx = idx;
        uint32_t h;
        
        for (h = 0;; h++) {
            
            if (h == tree_height) {
                /* We hit the root; return it */
                memcpy( root, &current[SPX_N], SPX_N );
                return;
            }

            // check if sibling node is an auth index   
            for (uint32_t i = 0; i < auth.count; i++) {
                if (auth.entries[i].level == h && 
                    auth.entries[i].index == node_idx) {
                    memcpy(sig + i * SPX_N,  &current[SPX_N], SPX_N);
                    break;
                }
            }
            
            // If it's the left child and not the last leaf, stopping moving upward 
            if (((node_idx & 1) == 0 && idx < max_idx)) {
                break;
            }
            
            /* Need to merge upward */
            unsigned char *left = &stack[h * SPX_N];
            memcpy(&current[0], left, SPX_N);

            // uint32_t parent_idx = node_idx / 2;
            set_tree_height(tree_addr, h + 1);
            set_tree_index(tree_addr, node_idx / 2);                
            thash(&current[SPX_N], &current[0], 2, ctx, tree_addr);
            node_idx = node_idx / 2;
        }
        
        // push current node to stack
        memcpy(&stack[h * SPX_N], &current[SPX_N], SPX_N);
    }
    sig += auth.count * SPX_N;

}

void octopus_recompute_root(unsigned char root[SPX_N],
                               const unsigned char *sig,
                               const uint32_t *indices,
                               uint32_t leaf_count,
                               const octopus_auth *auth,
                               const unsigned char *leaf_hashes,
                               const spx_ctx *ctx,
                               uint32_t tree_addr[8])
{
    node_entry current_nodes[SPX_TFORS_K];
    uint32_t current_len = leaf_count;
    unsigned char buffer[2 * SPX_N];
    
    // initialize current nodes with leaf hashes
    for (uint32_t i = 0; i < leaf_count; i++) {
        current_nodes[i].index = indices[i];
        memcpy(current_nodes[i].hash, leaf_hashes + i * SPX_N, SPX_N);
    }
    
    uint32_t auth_ptr = 0; 
    
    for (uint32_t level = 0; level < SPX_TFORS_HEIGHT; level++) {
        node_entry parent_nodes[current_len];
        uint32_t parent_len = 0;
        uint32_t i = 0;
       
        while (i < current_len) {
            uint32_t idx = current_nodes[i].index;
            
            if (i + 1 < current_len && current_nodes[i + 1].index == (idx ^ 1)) {
                memcpy(buffer, current_nodes[i++].hash, SPX_N);
                memcpy(buffer + SPX_N, current_nodes[i++].hash, SPX_N);
            } else if (idx & 1) {
                // read auth hash from auth path
                memcpy(buffer + SPX_N, current_nodes[i++].hash, SPX_N);
                memcpy(buffer, sig + auth_ptr * SPX_N, SPX_N);
                auth_ptr++;
            } else {
                memcpy(buffer, current_nodes[i++].hash, SPX_N);
                memcpy(buffer + SPX_N, sig + auth_ptr * SPX_N, SPX_N);
                auth_ptr++;
            }
            
            // compute parent node hash
            set_tree_height(tree_addr, level + 1);
            set_tree_index(tree_addr, idx / 2);
            thash(parent_nodes[parent_len].hash, buffer, 2, ctx, tree_addr);
            parent_nodes[parent_len++].index = idx / 2;
        }
        memcpy(current_nodes, parent_nodes, parent_len * sizeof(node_entry));
        current_len = parent_len;
    }
    memcpy(root, current_nodes[0].hash, SPX_N);
}