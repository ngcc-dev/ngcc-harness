#ifndef VOLE_COMMIT_HPP
#define VOLE_COMMIT_HPP

#include "block.hpp"

namespace faest
{

// Run the vector commitment and the small vole protocols.
// - `forest` must be `VECTOR_COMMIT_NODES` blocks long.
// - `hashed_leaves` must be VECTOR_COMMIT_LEAVES * P::leaf_hash_t::hash_len bytes long.
// - `u` must be `VOLE_COL_BLOCKS` long
// - `v` must be `SECURITY_PARAM * VOLE_COL_BLOCKS` long
// - `commitment` must be `(BITS_PER_WITNESS - 1) * VOLE_ROWS / 8` long
// - `check` must be `2 * SECURITY_PARAM / 8` long
template <typename P>
void vole_commit(block_secpar<P::secpar_v> seed, const typename P::tree_prg_t::iv_t& iv,
                  block_secpar<P::secpar_v>* __restrict__ forest,
                  unsigned char* __restrict__ hashed_leaves, vole_block* __restrict__ u,
                  vole_block* __restrict__ v, uint8_t* __restrict__ commitment,
                  uint8_t* __restrict__ check);

// - `q` must be `SECURITY_PARAM * VOLE_COL_BLOCKS` long
// - `delta_bytes` must be `SECURITY_PARAM` long
// - `commitment` must be `(BITS_PER_WITNESS - 1) * VOLE_ROWS / 8` long
// - `check` must be `2 * SECURITY_PARAM / 8` long
// - `opening` must be `VECTOR_OPEN_SIZE` long
// Returns false if the BAVC opening is not well-formed.
template <typename P>
bool vole_reconstruct(const typename P::tree_prg_t::iv_t& iv, vole_block* __restrict__ q, const uint8_t* delta_bytes,
                       const uint8_t* __restrict__ commitment, const uint8_t* __restrict__ opening,
                       uint8_t* __restrict__ check);

} // namespace faest

#endif
