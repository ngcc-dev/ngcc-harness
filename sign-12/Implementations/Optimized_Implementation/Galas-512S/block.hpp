#ifndef BLOCK_HPP
#define BLOCK_HPP

#include "parameters.hpp"

#include <array>
#include <cstring>

#include "avx2/block_impl.hpp"

namespace faest
{

static_assert(sizeof(block128) == 16, "Padding in block128.");
static_assert(sizeof(block192) == 24, "Padding in block192.");
static_assert(sizeof(block256) == 32, "Padding in block256.");
static_assert(sizeof(block384) == 48, "Padding in block384.");
static_assert(sizeof(block512) == 64, "Padding in block512.");

template <secpar S> using block_secpar = block<secpar_to_bits(S)>;

template <secpar S> using block_2secpar = block<2 * secpar_to_bits(S)>;

struct sig_iv_t
{
    std::array<uint8_t, SIG_IV_BYTES> bytes{};

    sig_iv_t() = default;

    template <std::size_t bits>
    explicit sig_iv_t(const block<bits>& b)
    {
        constexpr std::size_t in_bytes = bits / 8;
        constexpr std::size_t copy_bytes = in_bytes < SIG_IV_BYTES ? in_bytes : SIG_IV_BYTES;
        std::memcpy(bytes.data(), &b, copy_bytes);
    }
};

static_assert(sizeof(sig_iv_t) == SIG_IV_BYTES, "Padding in sig_iv_t.");

} // namespace faest

#endif
