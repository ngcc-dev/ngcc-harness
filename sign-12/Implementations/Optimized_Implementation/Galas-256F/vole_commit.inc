#ifndef VOLE_COMMIT_INC
#define VOLE_COMMIT_INC

#include "vole_commit.hpp"

#include "hash.hpp"
#include "prgs.hpp"
#include "small_vole.hpp"
#include "vector_com.inc"
#include <cstdalign>
#include <cstdlib>

namespace faest
{

template <secpar S>
static void hash_hashed_leaves_all_same_size(hash_state* __restrict__ hasher,
                                             const unsigned char* __restrict__ hashed_leaves,
                                             size_t num_trees, size_t leaves_total_size)
{
    for (size_t i = 0; (i + 4) <= num_trees; i += 4)
    {
        const void* to_hash[4];
        for (size_t j = 0; j < 4; ++j, hashed_leaves += leaves_total_size)
            to_hash[j] = hashed_leaves;

        block_2secpar<S> leaves_hashes[4];
        hash_state_x4 leaves_hasher;
        leaves_hasher.init(S);
        leaves_hasher.update(to_hash, leaves_total_size);
        leaves_hasher.update_1_byte(1);
        leaves_hasher.finalize_4(&leaves_hashes[0], &leaves_hashes[1], &leaves_hashes[2],
                                 &leaves_hashes[3], sizeof(leaves_hashes[0]));

        hasher->update(&leaves_hashes[0], sizeof(leaves_hashes));
    }

    for (size_t i = num_trees - (num_trees % 4); i < num_trees; ++i, hashed_leaves += leaves_total_size)
    {
        block_2secpar<S> leaves_hash;
        hash_state leaves_hasher;
        leaves_hasher.init(S);
        leaves_hasher.update(hashed_leaves, leaves_total_size);
        leaves_hasher.update_byte(1);
        leaves_hasher.finalize(&leaves_hash, sizeof(leaves_hash));

        hasher->update(&leaves_hash, sizeof(leaves_hash));
    }
}

template <typename P>
static void hash_hashed_leaves(const unsigned char* __restrict__ hashed_leaves,
                               uint8_t* __restrict__ hash_of_hashes)
{
    using CP = P::CONSTS;
    using VC = P::CONSTS::VEC_COM;

    hash_state hasher;
    hasher.init(P::secpar_v);
    hash_hashed_leaves_all_same_size<P::secpar_v>(&hasher, hashed_leaves, VC::NUM_MAX_K,
                                                  P::leaf_hash_t::hash_len << VC::MAX_K);
    hash_hashed_leaves_all_same_size<P::secpar_v>(
        &hasher, hashed_leaves + P::leaf_hash_t::hash_len * ((size_t)VC::NUM_MAX_K << VC::MAX_K),
        VC::NUM_MIN_K, P::leaf_hash_t::hash_len << VC::MIN_K);
    hasher.update_byte(1);
    hasher.finalize(hash_of_hashes, CP::VOLE_COMMIT_CHECK_SIZE);
}

template <typename P>
void vole_commit(block_secpar<P::secpar_v> seed, const typename P::tree_prg_t::iv_t& iv,
                 block_secpar<P::secpar_v>* __restrict__ forest,
                 unsigned char* __restrict__ hashed_leaves, vole_block* __restrict__ u,
                 vole_block* __restrict__ v, uint8_t* __restrict__ commitment,
                 uint8_t* __restrict__ check)
{
    using CP = P::CONSTS;
    using VC = P::CONSTS::VEC_COM;
    constexpr auto S = P::secpar_v;

    block_secpar<P::secpar_v>* leaves = reinterpret_cast<block_secpar<P::secpar_v>*>(aligned_alloc(
        alignof(block_secpar<S>), P::bavc_t::COMMIT_LEAVES * sizeof(block_secpar<S>)));
    P::bavc_t::commit(seed, iv, forest, leaves, hashed_leaves);

    hash_hashed_leaves<P>(hashed_leaves, check);

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

    if constexpr (P::zero_bits_in_delta_v > 0)
    {
        // Clear unused VOLE columns (corresponding to 0 bits of Delta).
        memset(v, 0, CP::VOLE_COL_BLOCKS * P::zero_bits_in_delta_v * sizeof(*v));
    }

    free(leaves);
}

template <typename P>
bool vole_reconstruct(const typename P::tree_prg_t::iv_t& iv, vole_block* __restrict__ q, const uint8_t* delta_bytes,
                      const uint8_t* __restrict__ commitment, const uint8_t* __restrict__ opening,
                      uint8_t* __restrict__ check)
{
    using CP = P::CONSTS;
    using VC = P::CONSTS::VEC_COM;
    constexpr auto S = P::secpar_v;

    block_secpar<S>* leaves = reinterpret_cast<block_secpar<S>*>(aligned_alloc(
        alignof(block_secpar<S>), P::bavc_t::COMMIT_LEAVES * sizeof(block_secpar<S>)));
    unsigned char* hashed_leaves = reinterpret_cast<unsigned char*>(aligned_alloc(
        alignof(block_2secpar<S>), P::bavc_t::COMMIT_LEAVES * P::leaf_hash_t::hash_len));

    if (!P::bavc_t::verify(iv, opening, delta_bytes, leaves, hashed_leaves))
    {
        free(leaves);
        free(hashed_leaves);
        return false;
    }
    hash_hashed_leaves<P>(hashed_leaves, check);

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

    if constexpr (P::zero_bits_in_delta_v > 0)
    {
        // Clear unused VOLE columns (corresponding to 0 bits of Delta).
        memset(q, 0, CP::VOLE_COL_BLOCKS * P::zero_bits_in_delta_v * sizeof(*q));
    }

    free(hashed_leaves);
    free(leaves);
    return true;
}

} // namespace faest

#endif
