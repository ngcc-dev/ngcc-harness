#ifndef SMALL_VOLE_HPP
#define SMALL_VOLE_HPP

#include "block.hpp"
#include <cassert>

namespace faest
{

// Given 2**k PRG keys and the chosen VOLE input u, generate the VOLE correlation v and a correction
// c to send to the receiver. u and c must be volumn vectors, while v must be a column-major matrix.
// They must all be VOLE_COL_BLOCKS vole_blocks tall, while v must have k rows. k must be at least
// VOLE_WIDTH_SHIFT. The input keys must be permuted according to vole_permute_key_index, i.e.,
// keys[i] = original_keys[vole_permute_key_index(i)];
template <typename P>
void vole_sender(unsigned int k, const block_secpar<P::secpar_v>* __restrict__ keys,
                 typename P::vole_prg_t::iv_t iv, typename P::vole_prg_t::tweak_t tweak,
                 const vole_block* __restrict__ u, vole_block* __restrict__ v,
                 vole_block* __restrict__ c);

// Given 2**k PRG keys, the secret delta, and the correction string c, generate the VOLE correlation
// q. c and q are stored similarly to c and v in vole_sender. A k-bit delta is represented as k
// bytes in little endian, with each byte being either 0 or 0xff. Input must by permuted by XOR with
// Delta, and then with vole_permute_key_index, i.e.,
// keys[i] = original_keys[vole_permute_key_index(i) ^ delta];
template <typename P>
void vole_receiver(unsigned int k, const block_secpar<P::secpar_v>* __restrict__ keys,
                   typename P::vole_prg_t::iv_t iv, typename P::vole_prg_t::tweak_t tweak,
                   const vole_block* __restrict__ c, vole_block* __restrict__ q,
                   const uint8_t* __restrict__ delta);

template <typename P>
void vole_receiver_apply_correction(size_t row_blocks, size_t cols,
                                    const vole_block* __restrict__ c, vole_block* __restrict__ q,
                                    const uint8_t* __restrict__ delta);

} // namespace faest

#endif
