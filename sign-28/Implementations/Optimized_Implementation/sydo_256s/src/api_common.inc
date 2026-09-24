#ifndef SYDO_API_COMMON_INC
#define SYDO_API_COMMON_INC

#include "sydo.hpp"
#include "keys.inc"
#include "sd_owf_proof.hpp"

#include "gfsmall.hpp"
#include "hash.hpp"
#include "polynomials.hpp"
#include "prgs.hpp"
#include "small_vole.hpp"
#include "transpose.hpp"
#include "transpose_secpar.hpp"
#include "util.hpp"
#include "vector_com.inc"
#include "vole_commit.hpp"

#include <array>
#include <cstdlib>
#include <cstring>

namespace sydo
{

namespace sydo_detail
{

template <typename P> struct primitive_bridge
{
    using prg_t = typename P::prg_t;
    using leaf_hash_t = typename P::leaf_hash_t;
    using seed_block_t = typename P::seed_block_t;
    using digest_block_t = typename P::digest_block_t;
    using field_t = typename P::field_t;
    using wide_field_t = typename P::wide_field_t;
    using vole_block_t = vole_block;

    constexpr static std::size_t matrix_rows = P::secpar_bits;
    constexpr static std::size_t matrix_cols = P::secpar_bits;
};

template <typename P>
inline void compute_macs_extended(block_secpar<P::secpar_v>* macs_extended, const vole_block* v,
                                  block_secpar<P::secpar_v>* macs)
{
    constexpr auto S = P::secpar_v;
    using row_t = block_secpar<P::secpar_v>;
    using CP = typename P::CONSTS;

    constexpr std::size_t subvector_bits = P::input_reduced_bits;
    constexpr std::size_t ext_subvector_bits = P::witness_extended_subvector_bits;
    constexpr std::size_t witness_bits = P::OWF_CONSTS::WITNESS_BITS;
    constexpr std::size_t ext_witness_bits = P::witness_extended_bits;
    constexpr std::size_t ext_witness_bits_aligned = P::witness_extended_bytes * 8;
    constexpr std::size_t tail_bits = (P::OWF_CONSTS::QS_DEGREE - 1) * P::secpar_bits;

    static_assert(CP::QUICKSILVER_ROWS_PADDED % sydo_transpose_bits_rows == 0, "");
    sydo_transpose_secpar<S>(v, macs, CP::VOLE_COL_STRIDE, CP::QUICKSILVER_ROWS_PADDED);

    if constexpr (P::elementary_encoding_v == elementary_encoding::hamming_ball)
    {
        static_assert(subvector_bits == ext_subvector_bits,
                      "Hamming-ball witnesses are stored without block compression.");
        for (std::size_t i = 0; i < ext_witness_bits; ++i)
            macs_extended[i] = macs[i];
    }
    else
    {
        for (std::size_t subvector_idx = 0; subvector_idx < P::witness_rows; ++subvector_idx)
        {
            const std::size_t sub_base = subvector_idx * subvector_bits;
            const std::size_t sub_ext_base = subvector_idx * ext_subvector_bits;

            std::size_t raw_off = 0;
            std::size_t ext_off = 0;
            for (std::size_t b = 0; b < P::num_var_blocks; ++b)
            {
                const std::size_t reduced_len = P::block_sizes[b] - 1;
                row_t t = row_t::set_zero();
                for (std::size_t k = 0; k < reduced_len; ++k)
                {
                    const auto r = macs[sub_base + raw_off + k];
                    macs_extended[sub_ext_base + ext_off + k] = r;
                    t = t ^ r;
                }
                macs_extended[sub_ext_base + ext_off + reduced_len] = t;

                raw_off += reduced_len;
                ext_off += reduced_len + 1;
            }
        }
    }

    for (std::size_t i = ext_witness_bits; i < ext_witness_bits_aligned; ++i)
        macs_extended[i] = row_t::set_zero();

    for (std::size_t i = 0; i < tail_bits; ++i)
        macs_extended[ext_witness_bits_aligned + i] = macs[witness_bits + i];

    for (std::size_t i = ext_witness_bits_aligned + tail_bits;
         i < CP::QUICKSILVER_ROWS_PADDED_EXTENDED; ++i)
    {
        macs_extended[i] = row_t::set_zero();
    }

}

template <typename P>
inline void compute_macs_extended_verifier(block_secpar<P::secpar_v>* macs_extended,
                                           const vole_block* q,
                                           block_secpar<P::secpar_v>* macs,
                                           block_secpar<P::secpar_v> delta_block)
{
    constexpr auto S = P::secpar_v;
    using row_t = block_secpar<P::secpar_v>;
    using CP = typename P::CONSTS;

    constexpr std::size_t subvector_bits = P::input_reduced_bits;
    constexpr std::size_t ext_subvector_bits = P::witness_extended_subvector_bits;
    constexpr std::size_t witness_bits = P::OWF_CONSTS::WITNESS_BITS;
    constexpr std::size_t ext_witness_bits = P::witness_extended_bits;
    constexpr std::size_t ext_witness_bits_aligned = P::witness_extended_bytes * 8;
    constexpr std::size_t tail_bits = (P::OWF_CONSTS::QS_DEGREE - 1) * P::secpar_bits;

    static_assert(CP::QUICKSILVER_ROWS_PADDED % sydo_transpose_bits_rows == 0, "");
    sydo_transpose_secpar<S>(q, macs, CP::VOLE_COL_STRIDE, CP::QUICKSILVER_ROWS_PADDED);

    if constexpr (P::elementary_encoding_v == elementary_encoding::hamming_ball)
    {
        static_assert(subvector_bits == ext_subvector_bits,
                      "Hamming-ball witnesses are stored without block compression.");
        for (std::size_t i = 0; i < ext_witness_bits; ++i)
            macs_extended[i] = macs[i];
    }
    else
    {
        for (std::size_t subvector_idx = 0; subvector_idx < P::witness_rows; ++subvector_idx)
        {
            const std::size_t sub_base = subvector_idx * subvector_bits;
            const std::size_t sub_ext_base = subvector_idx * ext_subvector_bits;

            std::size_t raw_off = 0;
            std::size_t ext_off = 0;
            for (std::size_t b = 0; b < P::num_var_blocks; ++b)
            {
                const std::size_t reduced_len = P::block_sizes[b] - 1;
                row_t t = ((P::block_wts[b] & 1u) != 0) ? delta_block : row_t::set_zero();
                for (std::size_t k = 0; k < reduced_len; ++k)
                {
                    const auto r = macs[sub_base + raw_off + k];
                    macs_extended[sub_ext_base + ext_off + k] = r;
                    t = t ^ r;
                }
                macs_extended[sub_ext_base + ext_off + reduced_len] = t;

                raw_off += reduced_len;
                ext_off += reduced_len + 1;
            }
        }
    }

    for (std::size_t i = ext_witness_bits; i < ext_witness_bits_aligned; ++i)
        macs_extended[i] = row_t::set_zero();

    for (std::size_t i = 0; i < tail_bits; ++i)
        macs_extended[ext_witness_bits_aligned + i] = macs[witness_bits + i];

    for (std::size_t i = ext_witness_bits_aligned + tail_bits;
         i < CP::QUICKSILVER_ROWS_PADDED_EXTENDED; ++i)
    {
        macs_extended[i] = row_t::set_zero();
    }

}

template <typename P> struct signing_context
{
    using bridge = primitive_bridge<P>;
    using CP = typename P::CONSTS;

    std::array<uint8_t, SYDO_PACKED_PUBLIC_KEY_BYTES<P>> public_key{};
    std::array<uint8_t, CP::SALT_BYTES> salt{};
    std::array<uint8_t, CP::DIGEST_BYTES> message_digest{};
    std::array<uint8_t, CP::COMMITMENT_BYTES> commitment{};
    std::array<uint8_t, CP::RESPONSE_BYTES> response{};
};

} // namespace sydo_detail

} // namespace sydo

#endif
