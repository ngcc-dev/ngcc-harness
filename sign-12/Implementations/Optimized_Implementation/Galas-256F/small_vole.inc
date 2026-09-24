#ifndef SMALL_VOLE_INC
#define SMALL_VOLE_INC

#include <cassert>

#include "aes.hpp"
#include "block.hpp"
#include "prgs.hpp"
#include "small_vole.hpp"
#include "util.hpp"

namespace faest
{

namespace detail
{

template <typename P> static constexpr std::size_t COL_LEN = CONSTANTS<P>::VOLE_COL_BLOCKS;

// TODO: probably can ditch most of the "restrict"s in inlined functions.

// There are two different methods for efficiently reducing the PRG outputs (equivalent to computing
// a Hamming code syndrome): Divide and conquer, and a straight-line method based on Gray's code.
// These are both transposes of Hadamard encoding circuits given in "the Exact Lower Bounds of
// Encoding Circuit Sizes of Hamming Codes and Hadamard Codes". This implementation combines these
// two methods. Divide and conquer is used at the lowest level, as it is inherently parallel and has
// a fixed access pattern when unrolled. Above it, the Gray's code method is used, as it needs very
// little temporary storage.

// Output: v (or q) in in_out[1, ..., depth], and u in in_out[0].
template <typename P> static ALWAYS_INLINE void xor_reduce(vole_block* in_out)
{
    using CP = CONSTANTS<P>;
#ifdef __GNUC__
#pragma GCC unroll(5)
#endif
    for (size_t i = 0; i < CP::VOLE_WIDTH_SHIFT; i++)
    {
        size_t stride = 1 << i;
#ifdef __GNUC__
#pragma GCC unroll(32)
#endif
        for (size_t j = 0; j < CP::VOLE_WIDTH; j += 2 * stride)
        {
#ifdef __GNUC__
#pragma GCC unroll(5)
#endif
            for (size_t d = 0; d <= i; ++d)
                in_out[j + d] = in_out[j + d] ^ in_out[j + d + stride];
            in_out[j + i + 1] = in_out[j + stride];
        }
    }
}

template <typename P>
static ALWAYS_INLINE void convert_prg_output(vole_block* prg_output,
                                             const typename P::vole_prg_t::block_t* raw_prg_output,
                                             size_t blocks)
{
    using CP = CONSTANTS<P>;
    if constexpr (CP::PRG_VOLE_BLOCKS == 2)
    {
#ifdef __GNUC__
#pragma GCC unroll(32)
#endif
        for (size_t i = 0; i < blocks; ++i)
            prg_output[i] = block256::from_2_block128(raw_prg_output[CP::PRG_VOLE_BLOCKS * i],
                                                      raw_prg_output[CP::PRG_VOLE_BLOCKS * i + 1]);
    }
    else
    {
        memcpy(prg_output, raw_prg_output, blocks * sizeof(vole_block));
    }
}

template <typename P, bool ignore_0>
static ALWAYS_INLINE void process_prg_output(size_t j, unsigned int output_col, vole_block* accum,
                                             vole_block* vq,
                                             const typename P::vole_prg_t::block_t* raw_prg_output)
{
    using CP = CONSTANTS<P>;
    vole_block prg_output[CP::VOLE_WIDTH];
    if constexpr (!ignore_0)
        convert_prg_output<P>(prg_output, raw_prg_output, CP::VOLE_WIDTH);
    else
    {
        prg_output[0] = vole_block::set_zero();
        convert_prg_output<P>(&prg_output[1], raw_prg_output, CP::VOLE_WIDTH - 1);
    }

    xor_reduce<P>(prg_output);

    if constexpr (!ignore_0)
        accum[j] = accum[j] ^ prg_output[0];

    for (size_t col = 0; col < CP::VOLE_WIDTH_SHIFT; ++col)
        vq[COL_LEN<P> * col + j] = vq[COL_LEN<P> * col + j] ^ prg_output[col + 1];

    if constexpr (!ignore_0)
        // Grey's codes method. output_col is the index of the bit that will change when
        // incrementing the Gray's code.
        vq[COL_LEN<P> * output_col + j] = vq[COL_LEN<P> * output_col + j] ^ accum[j];
}

// Sender and receiver merged together, since they share most of the same code.
template <typename P, bool receiver>
static ALWAYS_INLINE void
vole(unsigned int k, const block_secpar<P::secpar_v>* __restrict__ keys,
     typename P::vole_prg_t::iv_t iv, typename P::vole_prg_t::tweak_t tweak,
     const vole_block* __restrict__ u_or_c_in, vole_block* __restrict__ vq,
     vole_block* __restrict__ c_out, const uint8_t* __restrict__ delta)
{
    using CP = CONSTANTS<P>;

    vole_block accum[COL_LEN<P>];
    memset(&accum[0], 0, COL_LEN<P> * sizeof(vole_block));

    if (receiver && u_or_c_in)
    {
        vole_block* q_ptr = vq;
        for (unsigned int col = 0; col < k; ++col)
            for (size_t j = 0; j < COL_LEN<P>; ++j)
                *(q_ptr++) = u_or_c_in[j] & vole_block::set_all_8(delta[col]);
    }
    else
    {
        memset(&vq[0], 0, COL_LEN<P> * k * sizeof(vole_block));
    }

    typename P::vole_prg_t::expanded_key_t expanded_keys[CP::VOLE_WIDTH];
    typename P::vole_prg_t::block_t raw_prg_output[CP::VOLE_WIDTH * CP::PRG_VOLE_BLOCKS];

    size_t i = 0;
    if constexpr (receiver)
    {
        // Handle first iteration separately, since the 0th PRG key is a dummy.

        P::vole_prg_t::template init<CP::VOLE_WIDTH - 1, CP::PRG_VOLE_BLOCKS>(
            &keys[1], expanded_keys, iv, tweak, 0, raw_prg_output);
        process_prg_output<P, true>(0, 0, accum, vq, raw_prg_output);

        for (size_t j = 1; j < COL_LEN<P>; ++j)
        {
            P::vole_prg_t::template gen<CP::VOLE_WIDTH - 1, CP::PRG_VOLE_BLOCKS>(
                expanded_keys, iv, tweak, j * CP::PRG_VOLE_BLOCKS, raw_prg_output);
            process_prg_output<P, true>(j, 0, accum, vq, raw_prg_output);
        }

        i = CP::VOLE_WIDTH;
    }

    for (; i < (size_t)1 << k; i += CP::VOLE_WIDTH)
    {
        // Bitwise or is to make output_col be k - 1 when i + VOLE_WIDTH = 2**k, rather than k.
        unsigned int output_col = count_trailing_zeros((i + CP::VOLE_WIDTH) | (1 << (k - 1)));

        P::vole_prg_t::template init<CP::VOLE_WIDTH, CP::PRG_VOLE_BLOCKS>(
            &keys[i], expanded_keys, iv, tweak, 0, raw_prg_output);
        process_prg_output<P, false>(0, output_col, accum, vq, raw_prg_output);

        for (size_t j = 1; j < COL_LEN<P>; ++j)
        {
            P::vole_prg_t::template gen<CP::VOLE_WIDTH, CP::PRG_VOLE_BLOCKS>(
                expanded_keys, iv, tweak, j * CP::PRG_VOLE_BLOCKS, raw_prg_output);
            process_prg_output<P, false>(j, output_col, accum, vq, raw_prg_output);
        }
    }

    if constexpr (!receiver)
    {
        if (u_or_c_in)
            for (size_t j = 0; j < COL_LEN<P>; ++j)
                c_out[j] = u_or_c_in[j] ^ accum[j];
        else
            memcpy(c_out, accum, sizeof(accum));
    }
}

} // namespace detail

template <typename P>
void vole_sender(unsigned int k, const block_secpar<P::secpar_v>* __restrict__ keys,
                 typename P::vole_prg_t::iv_t iv, typename P::vole_prg_t::tweak_t tweak,
                 const vole_block* __restrict__ u, vole_block* __restrict__ v,
                 vole_block* __restrict__ c)
{
    detail::vole<P, false>(k, keys, iv, tweak, u, v, c, NULL);
}

template <typename P>
void vole_receiver(unsigned int k, const block_secpar<P::secpar_v>* __restrict__ keys,
                   typename P::vole_prg_t::iv_t iv, typename P::vole_prg_t::tweak_t tweak,
                   const vole_block* __restrict__ c, vole_block* __restrict__ q,
                   const uint8_t* __restrict__ delta)
{
    detail::vole<P, true>(k, keys, iv, tweak, c, q, NULL, delta);
}

template <typename P>
void vole_receiver_apply_correction(size_t row_blocks, size_t cols,
                                    const vole_block* __restrict__ c, vole_block* __restrict__ q,
                                    const uint8_t* __restrict__ delta)
{
    for (unsigned int col = 0; col < cols; ++col)
        for (size_t j = 0; j < row_blocks; ++j)
            q[col * detail::COL_LEN<P> + j] =
                q[col * detail::COL_LEN<P> + j] ^ c[j] & vole_block::set_all_8(delta[col]);
}

} // namespace faest

#endif
