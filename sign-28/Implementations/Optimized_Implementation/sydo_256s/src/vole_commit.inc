#ifndef VOLE_COMMIT_INC
#define VOLE_COMMIT_INC

#include "vole_commit.hpp"

#include "hash.hpp"
#include "profile.hpp"
#include "prgs.hpp"
#include "small_vole.hpp"
#include "vector_com.inc"
#include <cstdalign>
#include <cstdlib>

namespace sydo
{

template <secpar S>
static void hash_hashed_leaves_all_same_size(hash_state* __restrict__ hasher,
                                             unsigned char* __restrict__ hashed_leaves,
                                             size_t num_trees, size_t leaves_total_size)
{
    for (size_t i = 0; i < num_trees; ++i, hashed_leaves += leaves_total_size)
    {
        block_2secpar<S> leaves_hash;
        auxfunc_pseudo_xof_bytes_with_suffix(sizeof(leaves_hash) * 8, hashed_leaves,
                                             leaves_total_size, 1, (unsigned char*)&leaves_hash);

        hasher->update(&leaves_hash, sizeof(leaves_hash));
    }
}

template <typename P>
static void hash_hashed_leaves(unsigned char* __restrict__ hashed_leaves,
                               uint8_t* __restrict__ hash_of_hashes)
{
    using CP = P::CONSTS;
    using VC = P::CONSTS::VEC_COM;

    hash_state hasher;
    hasher.init(P::secpar_v);
    hasher.reserve((VC::NUM_MAX_K + VC::NUM_MIN_K) * sizeof(block_2secpar<P::secpar_v>) + 1);
    hash_hashed_leaves_all_same_size<P::secpar_v>(&hasher, hashed_leaves, VC::NUM_MAX_K,
                                                  P::leaf_hash_t::hash_len << VC::MAX_K);
    hash_hashed_leaves_all_same_size<P::secpar_v>(
        &hasher,
        hashed_leaves + P::leaf_hash_t::hash_len * ((size_t)VC::NUM_MAX_K << VC::MAX_K),
        VC::NUM_MIN_K, P::leaf_hash_t::hash_len << VC::MIN_K);
    hasher.update_byte(1);
    hasher.finalize(hash_of_hashes, CP::VOLE_COMMIT_CHECK_SIZE);
}

template <typename P>
void vole_commit(block_secpar<P::secpar_v> seed, typename P::vole_prg_t::iv_t iv,
                 block_secpar<P::secpar_v>* __restrict__ forest,
                 unsigned char* __restrict__ hashed_leaves, vole_block* __restrict__ u,
                 vole_block* __restrict__ v, uint8_t* __restrict__ commitment,
                 uint8_t* __restrict__ check)
{
    using CP = P::CONSTS;
    using VC = P::CONSTS::VEC_COM;
    constexpr auto S = P::secpar_v;

    block_secpar<S>* leaves = reinterpret_cast<block_secpar<S>*>(aligned_alloc(
        alignof(block_secpar<S>), P::bavc_t::COMMIT_LEAVES * sizeof(block_secpar<S>)));
    {
        profile::scope timer(profile::SIGN_VEC_COMMIT);
        P::bavc_t::commit(seed, iv, forest, leaves, hashed_leaves);
    }

    {
        profile::scope timer(profile::SIGN_HASH_LEAVES);
        hash_hashed_leaves<P>(hashed_leaves, check);
    }

    {
        profile::scope timer(profile::SIGN_VOLE_SENDER);
        vole_block correction[CP::VOLE_COL_BLOCKS];
        block_secpar<S>* leaves_iter = leaves;
        for (size_t i = 0; i < P::tau_v; ++i)
        {
            unsigned int k = i < VC::NUM_MAX_K ? VC::MAX_K : VC::MIN_K;
            auto tweak = ((typename P::vole_prg_t::tweak_t) 1 << 31) + i;
            if (!i)
                vole_sender<P>(k, leaves_iter, iv, tweak, NULL, v, u);
            else
            {
                vole_sender<P>(k, leaves_iter, iv, tweak, u, v, correction);
                memcpy(commitment, correction, CP::VOLE_ROWS / 8);
                commitment += CP::VOLE_ROWS / 8;
            }

            leaves_iter += (size_t)1 << k;
            v += CP::VOLE_COL_BLOCKS * k;
        }

        if constexpr (P::unused_delta_bits_v > 0)
        {
            // Clear unused VOLE columns (corresponding to 0 bits of Delta).
            memset(v, 0, CP::VOLE_COL_BLOCKS * P::unused_delta_bits_v * sizeof(*v));
        }
    }

    free(leaves);
}

template <typename P>
bool vole_reconstruct(typename P::vole_prg_t::iv_t iv, vole_block* __restrict__ q, const uint8_t* delta_bytes,
                      const uint8_t* __restrict__ commitment, const uint8_t* __restrict__ opening,
                      uint8_t* __restrict__ check)
{
    using CP = P::CONSTS;
    using VC = P::CONSTS::VEC_COM;
    constexpr auto S = P::secpar_v;

    block_secpar<S>* leaves = reinterpret_cast<block_secpar<S>*>(aligned_alloc(
        alignof(block_secpar<S>), P::bavc_t::COMMIT_LEAVES * sizeof(block_secpar<S>)));
    constexpr size_t hashed_leaves_bytes = P::bavc_t::COMMIT_LEAVES * P::leaf_hash_t::hash_len;
    constexpr size_t hashed_leaves_align = alignof(block_2secpar<S>);
    constexpr size_t hashed_leaves_alloc_bytes =
        ((hashed_leaves_bytes + 1 + hashed_leaves_align - 1) / hashed_leaves_align) *
        hashed_leaves_align;
    unsigned char* hashed_leaves = reinterpret_cast<unsigned char*>(
        aligned_alloc(hashed_leaves_align, hashed_leaves_alloc_bytes));

    {
        profile::scope timer(profile::VERIFY_VEC_VERIFY);
        if (!P::bavc_t::verify(iv, opening, delta_bytes, leaves, hashed_leaves))
        {
            free(leaves);
            free(hashed_leaves);
            return false;
        }
    }
    {
        profile::scope timer(profile::VERIFY_HASH_LEAVES);
        hash_hashed_leaves<P>(hashed_leaves, check);
    }

    {
        profile::scope timer(profile::VERIFY_VOLE_RECEIVER);
        vole_block correction[CP::VOLE_COL_BLOCKS];
        if (CP::VOLE_COL_BLOCKS * sizeof(vole_block) != CP::VOLE_ROWS / 8)
            correction[CP::VOLE_COL_BLOCKS - 1] = vole_block::set_zero();

        block_secpar<S>* leaves_iter = leaves;
        for (size_t i = 0; i < P::tau_v; ++i)
        {
            unsigned int k = i < VC::NUM_MAX_K ? VC::MAX_K : VC::MIN_K;
            auto tweak = ((typename P::vole_prg_t::tweak_t) 1 << 31) + i;
            if (!i)
                vole_receiver<P>(k, leaves_iter, iv, tweak, NULL, q, delta_bytes);
            else
            {
                memcpy(correction, commitment, CP::VOLE_ROWS / 8);
                commitment += CP::VOLE_ROWS / 8;
                vole_receiver<P>(k, leaves_iter, iv, tweak, correction, q, delta_bytes);
            }

            leaves_iter += (size_t)1 << k;
            q += CP::VOLE_COL_BLOCKS * k;
            delta_bytes += k;
        }

        if constexpr (P::unused_delta_bits_v > 0)
        {
            // Clear unused VOLE columns (corresponding to 0 bits of Delta).
            memset(q, 0, CP::VOLE_COL_BLOCKS * P::unused_delta_bits_v * sizeof(*q));
        }
    }

    free(hashed_leaves);
    free(leaves);
    return true;
}

} // namespace sydo

#endif
