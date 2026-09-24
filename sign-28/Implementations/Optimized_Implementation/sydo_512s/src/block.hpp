#ifndef BLOCK_HPP
#define BLOCK_HPP

#include "parameters.hpp"

#include "avx2/block_impl.hpp"

namespace sydo
{

static_assert(sizeof(block128) == 16, "Padding in block128.");
static_assert(sizeof(block192) == 24, "Padding in block192.");
static_assert(sizeof(block256) == 32, "Padding in block256.");
static_assert(sizeof(block384) == 48, "Padding in block384.");
static_assert(sizeof(block512) == 64, "Padding in block512.");
static_assert(sizeof(block1024) == 128, "Padding in block1024.");

template <secpar S> using block_secpar = block<secpar_to_bits(S)>;

template <secpar S> using block_2secpar = block<2 * secpar_to_bits(S)>;

} // namespace sydo

#endif
