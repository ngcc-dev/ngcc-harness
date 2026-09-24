#ifndef FAEST_INC
#define FAEST_INC

#include "faest.hpp"
#include "faest_keys.inc"

#include "hash.hpp"
#include "owf_proof.hpp"
#include "quicksilver.hpp"
#include "small_vole.hpp"
#include "transpose_secpar.hpp"
#include "util.hpp"
#include "vector_com.inc"
#include "vole_check.hpp"
#include "vole_commit.hpp"
#include <cassert>
#include <cstdalign>
#include <cstdlib>
#include <type_traits>

namespace faest
{

template <typename P>
bool faest_sign(uint8_t* signature, const uint8_t* msg, size_t msg_len, const uint8_t* sk_packed,
                const uint8_t* random_seed, size_t random_seed_len)
{
    using CP = P::CONSTS;
    using OC = P::OWF_CONSTS;
    constexpr auto S = P::secpar_v;

    secret_key<P> sk;
    uint8_t pk_packed[FAEST_PUBLIC_KEY_BYTES<P>];
    if (!faest_unpack_sk_and_get_pubkey<P>(pk_packed, sk_packed, &sk))
        return false;

    // TODO: Do we need to domain separate by the faest parameters?

    // SHAKE hash state the we will keep reusing.
    hash_state hasher;

    // mu <- H_2^0(pk || msg)
    block_2secpar<S> mu;
    hasher.init(S);
    hasher.update(pk_packed, FAEST_PUBLIC_KEY_BYTES<P>);
    hasher.update(msg, msg_len);
    hasher.update_byte(8 + 0);
    hasher.finalize(&mu, sizeof(mu));

    static_assert(std::is_same_v<typename P::tree_prg_t::iv_t, typename P::vole_prg_t::iv_t>);

    // (r, iv^pre) <- H_3(sk || mu || rho)
    block_secpar<S> seed; // r
    typename P::tree_prg_t::iv_t iv;
    typename P::tree_prg_t::iv_t iv_pre;
    std::array<uint8_t, sizeof(seed) + sizeof(iv_pre)> seed_iv_pre;
    hasher.init(S);
    hasher.update(&sk.sk, sizeof(sk.sk));
    hasher.update(&mu, sizeof(mu));
    if (random_seed) // rho (NB: spec assumes rho is always given and \lambda bit long)
        hasher.update(random_seed, random_seed_len);
    hasher.update_byte(3);
    hasher.finalize(seed_iv_pre.data(), sizeof(seed_iv_pre));
    memcpy(&seed, seed_iv_pre.data(), sizeof(seed));
    memcpy(&iv_pre, &seed_iv_pre[sizeof(seed)], sizeof(iv_pre));

    // iv <- H_4(iv^pre)
    hasher.init(S);
    hasher.update(&iv_pre, sizeof(iv_pre));
    hasher.update_byte(4);
    hasher.finalize(reinterpret_cast<uint8_t*>(&iv), sizeof(iv));

    block_secpar<S>* forest = reinterpret_cast<block_secpar<S>*>(
        aligned_alloc(alignof(block_secpar<S>), P::bavc_t::COMMIT_NODES * sizeof(block_secpar<S>)));
    unsigned char* hashed_leaves = reinterpret_cast<unsigned char*>(aligned_alloc(
        alignof(block_2secpar<S>), P::bavc_t::COMMIT_LEAVES * P::leaf_hash_t::hash_len));
    vole_block* u = reinterpret_cast<vole_block*>(
        aligned_alloc(alignof(vole_block), CP::VOLE_COL_BLOCKS * sizeof(vole_block)));
    vole_block* v = reinterpret_cast<vole_block*>(aligned_alloc(
        alignof(vole_block), P::secpar_bits * CP::VOLE_COL_BLOCKS * sizeof(vole_block)));
    uint8_t vole_commit_check[CP::VOLE_COMMIT_CHECK_SIZE];

    vole_commit<P>(seed, iv, forest, hashed_leaves, u, v, signature, vole_commit_check);

    // chall_1 <- H_2^1(mu || com || c_1 || ... || c_\tau-1 || iv)
    std::array<uint8_t, CP::VOLE_CHECK::CHALLENGE_BYTES> chal1;
    hasher.init(S);
    hasher.update(&mu, sizeof(mu));
    hasher.update(vole_commit_check, CP::VOLE_COMMIT_CHECK_SIZE);
    hasher.update(signature, CP::VOLE_COMMIT_SIZE);
    hasher.update(&iv, sizeof(iv));
    hasher.update_byte(8 + 1);
    hasher.finalize(chal1.data(), sizeof(chal1));

    // chall_2 <- H_2^2(chall_1 || \tilde{u} || h_V || d)
    std::array<uint8_t, CP::QS::CHALLENGE_BYTES> chal2;
    hasher.init(S);
    hasher.update(chal1.data(), sizeof(chal1));

    uint8_t* vole_check_proof = signature + CP::VOLE_COMMIT_SIZE;
    // vole_check_sender hashes u_tilde and V_tilde
    vole_check_sender<P>(u, v, chal1.data(), vole_check_proof, hasher);

    uint8_t* correction = vole_check_proof + CP::VOLE_CHECK::PROOF_BYTES;
    size_t remainder = (OC::WITNESS_BITS / 8) % (16 * CP::VOLE_BLOCK);
    for (size_t i = 0; i < CP::WITNESS_BLOCKS - (remainder != 0); ++i)
    {
        vole_block correction_i = u[i] ^ sk.witness[i];
        memcpy(correction + i * sizeof(vole_block), &correction_i, sizeof(vole_block));
    }
    if (remainder)
    {
        vole_block correction_i = u[CP::WITNESS_BLOCKS - 1] ^ sk.witness[CP::WITNESS_BLOCKS - 1];
        memcpy(correction + (CP::WITNESS_BLOCKS - 1) * sizeof(vole_block), &correction_i,
               remainder);
    }

    // continue with H_2^2
    hasher.update(correction, OC::WITNESS_BITS / 8);
    hasher.update_byte(8 + 2);
    hasher.finalize(chal2.data(), sizeof(chal2));

    block_secpar<S>* macs = reinterpret_cast<block_secpar<S>*>(aligned_alloc(
        alignof(block_secpar<S>), CP::QUICKSILVER_ROWS_PADDED * sizeof(block_secpar<S>)));

    memcpy(&u[0], &sk.witness[0], OC::WITNESS_BITS / 8);
    static_assert(CP::QUICKSILVER_ROWS_PADDED % TRANSPOSE_BITS_ROWS == 0, "");
    transpose_secpar<S>(v, macs, CP::VOLE_COL_STRIDE, CP::QUICKSILVER_ROWS_PADDED);
    free(v);

    quicksilver_state<S, false, OC::QS_DEGREE> qs((uint8_t*)&u[0], macs, OC::OWF_NUM_CONSTRAINTS,
                                                  chal2.data());
    owf_constraints(&qs, &sk.pk);

    uint8_t* qs_proof = correction + OC::WITNESS_BITS / 8;
    std::array<uint8_t, CP::QS::CHECK_BYTES> qs_check;
    qs.prove(OC::WITNESS_BITS, qs_proof, qs_check.data());
    free(macs);
    free(u);

    uint8_t* veccom_open_start = qs_proof + CP::QS::PROOF_BYTES;
    uint8_t* delta = veccom_open_start + P::bavc_t::OPEN_SIZE;

    uint8_t* iv_pre_dst = delta + sizeof(block_secpar<S>);
    memcpy(iv_pre_dst, &iv_pre, sizeof(iv_pre));
    uint8_t* grinding_counter_dst = iv_pre_dst + sizeof(iv_pre);

    if constexpr (!P::use_grinding)
    {
        // chall_3 <- H_2^3(chall_2 || \tilde{a}_0 || \tilde{a}_1 || \tilde{a}_2)
        hasher.init(S);
        hasher.update(chal2.data(), sizeof(chal2));
        hasher.update(qs_check.data(), CP::QS::CHECK_BYTES);
        hasher.update(qs_proof, CP::QS::PROOF_BYTES);
        hasher.update_byte(8 + 3);
        hasher.finalize(delta, sizeof(block_secpar<S>));

        std::array<uint8_t, P::delta_bits_v> delta_bytes;
        expand_bits_to_bytes(delta_bytes.data(), P::delta_bits_v, delta);

        P::bavc_t::open(forest, hashed_leaves, delta_bytes.data(), veccom_open_start);
    }
    else
    {
        // chall_3 <- H_2^3(chall_2 || \tilde{a}_0 || \tilde{a}_1 || \tilde{a}_2 || ctr)
        // Initialize a 4x hasher and hash the common input prefix.
        hash_state_x4 grinding_hasher;
        grinding_hasher.init(S);
        grinding_hasher.update_1(chal2.data(), sizeof(chal2));
        grinding_hasher.update_1(qs_check.data(), CP::QS::CHECK_BYTES);
        grinding_hasher.update_1(qs_proof, CP::QS::PROOF_BYTES);
        uint32_t counter;
        bool open_success = grind_and_open<typename P::bavc_t>(
            forest, hashed_leaves, delta, veccom_open_start, &grinding_hasher, &counter);
        // Opening fails with a negligible probability, so we can assume it succeeds.
        FAEST_ASSERT(open_success);
        if (!open_success) return false;
        // Store counter in the signature.
        grinding_counter_dst[0] = counter;
        grinding_counter_dst[1] = counter >> 8;
        grinding_counter_dst[2] = counter >> 16;
        grinding_counter_dst[3] = counter >> 24;
    }

    free(forest);
    free(hashed_leaves);

    FAEST_ASSERT(grinding_counter_dst + P::grinding_counter_size == signature + FAEST_SIGNATURE_BYTES<P>);

    return true;
}

template <typename P>
bool faest_verify(const uint8_t* signature, const uint8_t* msg, size_t msg_len,
                  const uint8_t* pk_packed)
{
    using CP = P::CONSTS;
    using OC = P::OWF_CONSTS;
    constexpr auto S = P::secpar_v;

    // SHAKE hash state the we will keep reusing.
    hash_state hasher;

    // mu <- H_2^0(pk || msg)
    block_2secpar<S> mu;
    hasher.init(S);
    hasher.update(pk_packed, FAEST_PUBLIC_KEY_BYTES<P>);
    hasher.update(msg, msg_len);
    hasher.update_byte(8 + 0);
    hasher.finalize(&mu, sizeof(mu));

    static_assert(std::is_same_v<typename P::tree_prg_t::iv_t, typename P::vole_prg_t::iv_t>);
    typename P::tree_prg_t::iv_t iv;

    const uint8_t* vole_check_proof = signature + CP::VOLE_COMMIT_SIZE;
    const uint8_t* correction = vole_check_proof + CP::VOLE_CHECK::PROOF_BYTES;
    const uint8_t* qs_proof = correction + OC::WITNESS_BITS / 8;
    const uint8_t* veccom_open_start = qs_proof + CP::QS::PROOF_BYTES;
    const uint8_t* delta = veccom_open_start + P::bavc_t::OPEN_SIZE;
    const uint8_t* iv_pre_ptr = delta + sizeof(block_secpar<S>);
    const uint8_t* counter = iv_pre_ptr + sizeof(iv);

    // Check that the prover actually did its grinding.
    for (size_t i = secpar_to_bits(S) - 1; i >= CP::VEC_COM::delta_bits_v; --i)
        if ((delta[i / 8] >> (i % 8)) & 1)
            return false;

    // iv <- H_4(iv^pre)
    hasher.init(S);
    hasher.update(iv_pre_ptr, sizeof(iv));
    hasher.update_byte(4);
    hasher.finalize(reinterpret_cast<uint8_t*>(&iv), sizeof(iv));

    std::array<uint8_t, P::delta_bits_v> delta_bytes;
    expand_bits_to_bytes(delta_bytes.data(), P::delta_bits_v, delta);

    vole_block* q = reinterpret_cast<vole_block*>(aligned_alloc(
        alignof(vole_block), P::secpar_bits * CP::VOLE_COL_BLOCKS * sizeof(vole_block)));
    uint8_t vole_commit_check[CP::VOLE_COMMIT_CHECK_SIZE];

    if (!vole_reconstruct<P>(iv, q, delta_bytes.data(), signature, veccom_open_start,
                             vole_commit_check))
    {
        free(q);
        return false;
    }

    // chall_1 <- H_2^1(mu || com || c_1 || ... || c_\tau-1 || iv)
    std::array<uint8_t, CP::VOLE_CHECK::CHALLENGE_BYTES> chal1;
    hasher.init(S);
    hasher.update(&mu, sizeof(mu));
    hasher.update(vole_commit_check, CP::VOLE_COMMIT_CHECK_SIZE);
    hasher.update(signature, CP::VOLE_COMMIT_SIZE);
    hasher.update(&iv, sizeof(iv));
    hasher.update_byte(8 + 1);
    hasher.finalize(chal1.data(), sizeof(chal1));


    // chall_2 <- H_2^2(chall_1 || \tilde{u} || h_V || d)
    std::array<uint8_t, CP::QS::CHALLENGE_BYTES> chal2;
    hasher.init(S);
    hasher.update(chal1.data(), sizeof(chal1));
    hasher.update(vole_check_proof, CP::VOLE_CHECK::PROOF_BYTES);

    // vole_check_receiver hashes D
    vole_check_receiver<P>(q, delta_bytes.data(), chal1.data(), vole_check_proof, hasher);

    // continue with H_2^2
    hasher.update(correction, OC::WITNESS_BITS / 8);
    hasher.update_byte(8 + 2);
    hasher.finalize(chal2.data(), sizeof(chal2));

    std::array<vole_block, CP::WITNESS_BLOCKS> correction_blocks;
    memcpy(&correction_blocks, correction, OC::WITNESS_BITS / 8);
    memset(reinterpret_cast<uint8_t*>(correction_blocks.data()) + OC::WITNESS_BITS / 8, 0,
           sizeof(correction_blocks) - OC::WITNESS_BITS / 8);
    vole_receiver_apply_correction<P>(CP::WITNESS_BLOCKS, P::delta_bits_v, correction_blocks.data(), q,
                                      delta_bytes.data());

    block_secpar<S>* macs = reinterpret_cast<block_secpar<S>*>(
        aligned_alloc(alignof(block_secpar<S>), CP::VOLE_ROWS_PADDED * sizeof(block_secpar<S>)));
    transpose_secpar<S>(q, macs, CP::VOLE_COL_STRIDE, CP::QUICKSILVER_ROWS_PADDED);
    free(q);

    block_secpar<S> delta_block;
    memcpy(&delta_block, delta, sizeof(delta_block));

    public_key<P> pk;
    faest_unpack_public_key(&pk, pk_packed);

    quicksilver_state<S, true, OC::QS_DEGREE> qs(macs, OC::OWF_NUM_CONSTRAINTS, delta_block, chal2.data());
    owf_constraints(&qs, &pk);

    std::array<uint8_t, CP::QS::CHECK_BYTES> qs_check;
    qs.verify(OC::WITNESS_BITS, qs_proof, qs_check.data());
    free(macs);

    // chall_3' <- H_2^3(chall_2 || \tilde{a}_0 || \tilde{a}_1 || \tilde{a}_2 [|| ctr])
    block_secpar<S> delta_check;
    hasher.init(S);
    hasher.update(chal2.data(), sizeof(chal2));
    hasher.update(qs_check.data(), CP::QS::CHECK_BYTES);
    hasher.update(qs_proof, CP::QS::PROOF_BYTES);
    if constexpr (P::use_grinding)
    {
        hasher.update(counter, P::grinding_counter_size);
    }
    hasher.update_byte(8 + 3);
    hasher.finalize(&delta_check, sizeof(delta_check));

    return memcmp(delta, &delta_check, sizeof(delta_check)) == 0;
}

} // namespace faest

#endif
