#include <stdint.h>
#include <string.h>

#include "address.h"
#include "params.h"
#include "utils.h"

/**
 * Sets the Merkle tree layer index.
 * @param layer_addr Address structure to modify
 * @param layer The layer index (0 to SPX_D-1)
 */
void set_layer_addr(uint32_t layer_addr[8], uint32_t layer)
{
    ((unsigned char *)layer_addr)[PH_OFFSET_LAYER] = (unsigned char)layer;
}

/**
 * Sets the subtree identifier within the current layer.
 * @param tree_addr Address structure to modify
 * @param tree The subtree tree address
 */
void set_tree_addr(uint32_t tree_addr[8], uint64_t tree)
{
#if (SPX_TREE_HEIGHT * (SPX_D - 1)) > 64
    #error Tree address is not supported for SPX_D > 64
#endif
    ull_to_bytes(&((unsigned char *)tree_addr)[PH_OFFSET_TREE], 8, tree);
}

/**
 * Sets the address type for hash operations.
 * Different hash computations use different address types to prevent collisions.
 * @param tree_addr Address structure to modify
 * @param addr_type One of the PH_ADDR_TYPE_* constants
 */
void set_addr_type(uint32_t tree_addr[8], uint32_t addr_type)
{
    ((unsigned char *)tree_addr)[PH_OFFSET_ADDR_TYPE] = (unsigned char)addr_type;
}

/**
 * Copies the layer and tree fields between address structures.
 * Used when multiple hash operations share the same tree context.
 * @param dest Destination address structure
 * @param src Source address structure
 */
void copy_tree_addr(uint32_t dest[8], const uint32_t src[8])
{
    memcpy(dest, src, PH_OFFSET_TREE + 8);
}

/* =========================================================================
 * GWOTS (Winternitz One-Time Signature) Address Operations
 * These functions manage addresses for GWOTS key pairs and chain operations
 * ========================================================================= */

/**
 * Sets the GWOTS key pair index within the leaf nodes.
 * Each leaf corresponds to a unique GWOTS key pair.
 * @param keypair_addr Address structure to modify
 * @param keypair The key pair index
 */
void set_keypair_addr(uint32_t keypair_addr[8], uint32_t keypair)
{
    u32_to_bytes(&((unsigned char *)keypair_addr)[PH_OFFSET_KEYPAIR_ADDR], keypair);
}

/**
 * Copies layer, tree, and keypair fields between address structures.
 * Enables parallel operations on the same GWOTS key pair.
 * @param dest Destination address structure
 * @param src Source address structure
 */
void copy_keypair_addr(uint32_t dest[8], const uint32_t src[8])
{
    memcpy(dest, src, PH_OFFSET_TREE + 8);
    memcpy((unsigned char *)dest + PH_OFFSET_KEYPAIR_ADDR,
           (unsigned char *)src + PH_OFFSET_KEYPAIR_ADDR, 4);
}

/* =========================================================================
 * Winternitz Chain Address Operations
 * ========================================================================= */

/**
 * Sets the chain index within the GWOTS signature.
 * Different chains use different Winternitz parameters (W1 or W2).
 * @param chain_addr Address structure to modify
 * @param gwots_chain The chain index
 */
void set_chain_addr(uint32_t chain_addr[8], uint32_t gwots_chain)
{
    ((unsigned char *)chain_addr)[PH_OFFSET_CHAIN_ADDR] = (unsigned char)gwots_chain;
}

/**
 * Sets the position within a Winternitz chain.
 * Indicates how many hash iterations have been performed.
 * @param hash_addr Address structure to modify
 * @param hash The hash iteration index
 */
void set_hash_addr(uint32_t hash_addr[8], uint32_t hash)
{
    ((unsigned char *)hash_addr)[PH_OFFSET_HASH_ADDR] = (unsigned char)hash;
}

/* =========================================================================
 * Tree Traversal Address Operations
 * Used for both TFORS authentication paths and Merkle tree computations
 * ========================================================================= */

/**
 * Sets the current height within the TFORS or Merkle tree.
 * @param tfors_addr Address structure to modify
 * @param tfors_tree_height Height level in the tree
 */
void set_tree_height(uint32_t tfors_addr[8], uint32_t tfors_tree_height)
{
    ((unsigned char *)tfors_addr)[PH_OFFSET_TREE_HEIGHT] = (unsigned char)tfors_tree_height;
}

/**
 * Sets the horizontal position (index) within the current tree level.
 * @param tfors_addr Address structure to modify
 * @param tfors_tree_index Node index at the current height
 */
void set_tree_index(uint32_t tfors_addr[8], uint32_t tfors_tree_index)
{
    u32_to_bytes(&((unsigned char *)tfors_addr)[PH_OFFSET_TREE_INDEX], tfors_tree_index);
}
