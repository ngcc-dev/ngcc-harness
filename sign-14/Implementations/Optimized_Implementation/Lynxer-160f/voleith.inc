#ifndef VOLEITH_INC
#define VOLEITH_INC

#include "voleith.hpp"
#include "voleith_keys.inc"

#include "hash.hpp"
#include "owf_proof.hpp"
#include "quicksilver.hpp"
#include "small_vole.hpp"
#include "transpose_secpar.hpp"
#include "util.hpp"
#include "vector_com.inc"
#include "vole_check.hpp"
#include "vole_commit.hpp"
#include "vole_key_index_permutation.hpp"
#include <array>
#include <cassert>
#include <cstdlib>

namespace sig
{

// Signature layout helpers (matching lynx-ref voleith_impl.c)
// Layout: [c_1..c_{τ-1}][u_tilde][d][a1_tilde][decom_i][chall_3][iv][ctr]

template <typename P>
constexpr std::size_t ELL_HAT_BITS_V =
    P::OWF_CONSTS::WITNESS_BITS + P::OWF_CONSTS::QS_DEGREE * P::secpar_bits + 16; // UNIVERSAL_HASH_B_BITS

template <typename P>
constexpr std::size_t ELL_HAT_BYTES_V = (ELL_HAT_BITS_V<P> + 7) / 8;

template <typename P>
constexpr std::size_t SIG_U_TILDE_BYTES_V = P::secpar_bytes + 2; // UNIVERSAL_HASH_B

template <typename P>
constexpr std::size_t sig_c_offset(unsigned int index) { return index * ELL_HAT_BYTES_V<P>; }

template <typename P>
constexpr std::size_t sig_u_tilde_offset() { return (P::tau_v - 1) * ELL_HAT_BYTES_V<P>; }

template <typename P>
constexpr std::size_t sig_d_offset() { return sig_u_tilde_offset<P>() + SIG_U_TILDE_BYTES_V<P>; }

template <typename P>
constexpr std::size_t sig_a1_tilde_offset() { return sig_d_offset<P>() + P::OWF_CONSTS::WITNESS_BITS / 8; }

template <typename P>
constexpr std::size_t sig_decom_offset() {
    return sig_a1_tilde_offset<P>() + P::secpar_bytes;
}

template <typename P>
constexpr std::size_t sig_chall3_offset() {
    return sig_decom_offset<P>() + P::bavc_t::OPEN_SIZE;
}

template <typename P>
constexpr std::size_t sig_iv_offset() { return sig_chall3_offset<P>() + P::secpar_bytes; }

template <typename P>
constexpr std::size_t sig_ctr_offset() { return sig_iv_offset<P>() + 15; }

template <typename P>
constexpr std::size_t sig_total_size() { return sig_ctr_offset<P>() + P::grinding_counter_size; }

template <typename P>
bool voleith_sign(uint8_t* signature, const uint8_t* msg, size_t msg_len, const uint8_t* sk_packed,
                const uint8_t* random_seed, size_t random_seed_len)
{
    using CP = P::CONSTS;
    using OC = P::OWF_CONSTS;
    constexpr auto S = P::secpar_v;
    constexpr size_t lambda_bytes = P::secpar_bytes;
    constexpr size_t max_csp = lambda_bytes; // Use template param directly

    secret_key<P> sk;
    uint8_t pk_packed[SIG_PUBLIC_KEY_BYTES<P>];
    if (!sig_unpack_sk_and_get_pubkey<P>(pk_packed, sk_packed, &sk))
        return false;

    // === Step 1: mu, bavc_s = H(domain=0 || owf_input || owf_output || msg) ===
    // pk_packed = owf_input || owf_output for Lynx
    constexpr size_t owf_io_size = SIG_PUBLIC_KEY_BYTES<P>;
    uint8_t mu[2 * max_csp];
    uint8_t bavc_s[max_csp];
    uint8_t mu_bavc_s[3 * max_csp];
    {
        hash_state h;
        h.init(S);
        h.update_byte(0);
        h.update(pk_packed, owf_io_size);
        h.update(msg, msg_len);
        h.finalize(mu_bavc_s, 3 * lambda_bytes);
    }
    memcpy(mu, mu_bavc_s, 2 * lambda_bytes);
    memcpy(bavc_s, mu_bavc_s + 2 * lambda_bytes, lambda_bytes);

    // === Step 2: root_key, iv = H(domain=1 || owf_key || mu || rho) ===
    block_secpar<S> root_key;
    uint8_t iv[16];
    {
        hash_state h;
        h.init(S);
        h.update_byte(1);
        h.update(&sk.sk, lambda_bytes);
        h.update(mu, 2 * lambda_bytes);
        if (random_seed)
            h.update(random_seed, random_seed_len);
        h.finalize(reinterpret_cast<uint8_t*>(&root_key), lambda_bytes);
        h.finalize(iv, 15);
    }

    // === Step 3: VOLE Commit (ref-compatible) ===
    // === Step 3: VOLE Commit ===
    block_secpar<S>* forest = reinterpret_cast<block_secpar<S>*>(
        aligned_alloc(alignof(block_secpar<S>), P::bavc_t::COMMIT_NODES * sizeof(block_secpar<S>)));
    unsigned char* hashed_leaves = reinterpret_cast<unsigned char*>(aligned_alloc(
        alignof(block_2secpar<S>), P::bavc_t::COMMIT_LEAVES * P::leaf_hash_t::hash_len));
    vole_block* u = reinterpret_cast<vole_block*>(
        aligned_alloc(alignof(vole_block), CP::VOLE_COL_BLOCKS * sizeof(vole_block)));
    vole_block* v = reinterpret_cast<vole_block*>(aligned_alloc(
        alignof(vole_block), P::secpar_bits * CP::VOLE_COL_BLOCKS * sizeof(vole_block)));
    uint8_t vole_commit_check[CP::VOLE_COMMIT_CHECK_SIZE];

    block128 iv_block;
    memset(&iv_block, 0, sizeof(iv_block));
    memcpy(&iv_block, iv, 15);
    vole_commit<P>(root_key, iv_block, forest, hashed_leaves, u, v, signature, mu, bavc_s,
                   vole_commit_check);

    // === Step 4: chall_1 = H(domain=9 || hcom || c_1 || ... || c_{τ-1}) ===
    constexpr size_t chall_1_bytes = 5 * max_csp + 8;
    uint8_t chall_1[chall_1_bytes];
    {
        hash_state h;
        h.init(S);
        h.update_byte(9);
        h.update(vole_commit_check, CP::VOLE_COMMIT_CHECK_SIZE);
        h.update(signature, CP::VOLE_COMMIT_SIZE);
        h.finalize(chall_1, 5 * lambda_bytes + 8);
    }

    // === Step 5: u_tilde = vole_hash(chall_1, u), V_tilde updates ===
    uint8_t* u_tilde_ptr = signature + sig_u_tilde_offset<P>();
    hash_state h2;
    h2.init(S);
    h2.update_byte(10);
    h2.update(chall_1, 5 * lambda_bytes + 8);
    vole_check_sender<P>(u, v, chall_1, u_tilde_ptr, h2);

    // === Step 6: d = witness XOR u ===
    uint8_t* d_ptr = signature + sig_d_offset<P>();
    constexpr size_t witness_bytes = OC::WITNESS_BITS / 8;
    {
        for (size_t i = 0; i < witness_bytes; ++i)
            d_ptr[i] = ((uint8_t*)u)[i] ^ ((uint8_t*)&sk.witness)[i];
    }

    // === Step 7: chall_2 = finalize hash with d ===
    constexpr size_t chall_2_bytes = 3 * max_csp + 8;
    uint8_t chall_2[chall_2_bytes];
    h2.update(d_ptr, witness_bytes);
    h2.finalize(chall_2, 3 * lambda_bytes + 8);

    // === Step 8: QuickSilver prove ===
    block_secpar<S>* macs = reinterpret_cast<block_secpar<S>*>(aligned_alloc(
        alignof(block_secpar<S>), CP::QUICKSILVER_ROWS_PADDED * sizeof(block_secpar<S>)));
    memset(macs, 0, CP::QUICKSILVER_ROWS_PADDED * sizeof(block_secpar<S>));

    memcpy(&u[0], &sk.witness[0], witness_bytes);
    static_assert(CP::QUICKSILVER_ROWS_PADDED % TRANSPOSE_BITS_ROWS == 0, "");
    transpose_secpar<S>(v, macs, CP::VOLE_COL_STRIDE, CP::QUICKSILVER_ROWS_PADDED);
    free(v);

    quicksilver_state<S, false, OC::QS_DEGREE> qs((uint8_t*)&u[0], macs, OC::OWF_NUM_CONSTRAINTS,
                                                  chall_2);
    owf_constraints(&qs, &sk);

    uint8_t* a1_tilde_ptr = signature + sig_a1_tilde_offset<P>();
    uint8_t a0_tilde_local[lambda_bytes];
    qs.prove(OC::WITNESS_BITS, a1_tilde_ptr, a0_tilde_local);
    free(macs);
    free(u);

    // === Step 9: Challenge 3 grinding ===
    uint8_t* chall_3_out = signature + sig_chall3_offset<P>();
    uint8_t* iv_out = signature + sig_iv_offset<P>();
    uint8_t* ctr_out = signature + sig_ctr_offset<P>();

    memcpy(iv_out, iv, 15);

    uint32_t counter = 0;
    hash_state_x4 h3;
    h3.init(S);
    h3.update_1_byte(11);
    h3.update_1(chall_2, 3 * lambda_bytes + 8);
    h3.update_1(a1_tilde_ptr, lambda_bytes);
    h3.update_1(a0_tilde_local, lambda_bytes);

    bool open_success = grind_and_open<typename P::bavc_t>(
        forest, hashed_leaves, chall_3_out, signature + sig_decom_offset<P>(), &h3, &counter,
        false);
    if (open_success)
    {
        ctr_out[0] = counter;
        ctr_out[1] = counter >> 8;
        ctr_out[2] = counter >> 16;
        ctr_out[3] = counter >> 24;
    }

    SIG_ASSERT(open_success);
    if (!open_success) return false;

    free(forest);
    free(hashed_leaves);

    SIG_ASSERT(ctr_out + P::grinding_counter_size == signature + SIG_SIGNATURE_BYTES<P>);

    return true;
}

template <typename P>
bool voleith_verify(const uint8_t* signature, const uint8_t* msg, size_t msg_len,
                  const uint8_t* pk_packed)
{
    using CP = P::CONSTS;
    using OC = P::OWF_CONSTS;
    constexpr auto S = P::secpar_v;
    constexpr size_t lambda_bytes = P::secpar_bytes;
    constexpr size_t max_csp = lambda_bytes;

    hash_state hasher;

    // Extract signature components
    const uint8_t* c_i = signature + sig_c_offset<P>(0);
    const uint8_t* u_tilde_ptr = signature + sig_u_tilde_offset<P>();
    const uint8_t* d_ptr = signature + sig_d_offset<P>();
    const uint8_t* a1_tilde_ptr = signature + sig_a1_tilde_offset<P>();
    const uint8_t* decom_i_ptr = signature + sig_decom_offset<P>();
    const uint8_t* chall_3_sig = signature + sig_chall3_offset<P>();
    const uint8_t* iv_sig = signature + sig_iv_offset<P>();
    const uint8_t* ctr_sig = signature + sig_ctr_offset<P>();

    // Check POW grinding bits
    for (size_t i = P::secpar_bits - 1; i >= P::delta_bits_v; --i)
        if ((chall_3_sig[i / 8] >> (i % 8)) & 1)
            return false;

    // === Step 1: mu, bavc_s = H(domain=0 || owf_input || owf_output || msg) ===
    constexpr size_t owf_io_size = SIG_PUBLIC_KEY_BYTES<P>;
    uint8_t mu[2 * max_csp];
    uint8_t bavc_s[max_csp];
    uint8_t mu_bavc_s[3 * max_csp];
    hasher.init(S);
    hasher.update_byte(0);
    hasher.update(pk_packed, owf_io_size);
    hasher.update(msg, msg_len);
    hasher.finalize(mu_bavc_s, 3 * lambda_bytes);
    memcpy(mu, mu_bavc_s, 2 * lambda_bytes);
    memcpy(bavc_s, mu_bavc_s + 2 * lambda_bytes, lambda_bytes);

    std::array<uint8_t, P::delta_bits_v> delta_bytes;
    expand_bits_to_bytes(delta_bytes.data(), P::delta_bits_v, chall_3_sig);

    vole_block* q = reinterpret_cast<vole_block*>(aligned_alloc(
        alignof(vole_block), P::secpar_bits * CP::VOLE_COL_BLOCKS * sizeof(vole_block)));
    uint8_t vole_commit_check[CP::VOLE_COMMIT_CHECK_SIZE];

    block128 iv_block;
    memset(&iv_block, 0, sizeof(iv_block));
    memcpy(&iv_block, iv_sig, 15);

    // === Step 2: VOLE Reconstruct ===
    const bool reconstruct_ok =
        vole_reconstruct<P>(iv_block, q, delta_bytes.data(), c_i, decom_i_ptr, mu, bavc_s,
                            vole_commit_check);
    if (!reconstruct_ok)
    {
        free(q);
        return false;
    }

    // === Step 3: chall_1 = H(domain=9 || hcom || c_i) ===
    constexpr size_t chall_1_bytes = 5 * max_csp + 8;
    uint8_t chall_1[chall_1_bytes];
    hasher.init(S);
    hasher.update_byte(9);
    hasher.update(vole_commit_check, CP::VOLE_COMMIT_CHECK_SIZE);
    hasher.update(c_i, CP::VOLE_COMMIT_SIZE);
    hasher.finalize(chall_1, 5 * lambda_bytes + 8);

    // === Step 4: Recompute chall_2 ===
    constexpr size_t chall_2_bytes = 3 * max_csp + 8;
    uint8_t chall_2[chall_2_bytes];

    hasher.init(S);
    hasher.update_byte(10);
    hasher.update(chall_1, 5 * lambda_bytes + 8);
    hasher.update(u_tilde_ptr, SIG_U_TILDE_BYTES_V<P>);

    vole_check_receiver<P>(q, delta_bytes.data(), chall_1, u_tilde_ptr, hasher);

    hasher.update(d_ptr, OC::WITNESS_BITS / 8);
    hasher.finalize(chall_2, 3 * lambda_bytes + 8);

    // === Step 5: Apply correction to q ===
    {
        std::array<vole_block, CP::WITNESS_BLOCKS> correction_blocks;
        memcpy(&correction_blocks, d_ptr, OC::WITNESS_BITS / 8);
        memset(reinterpret_cast<uint8_t*>(correction_blocks.data()) + OC::WITNESS_BITS / 8, 0,
               sizeof(correction_blocks) - OC::WITNESS_BITS / 8);
        vole_receiver_apply_correction<P>(CP::WITNESS_BLOCKS, P::delta_bits_v,
                                          correction_blocks.data(), q, delta_bytes.data());
    }

    // === Step 6: QuickSilver verify ===
    block_secpar<S>* macs = reinterpret_cast<block_secpar<S>*>(
        aligned_alloc(alignof(block_secpar<S>), CP::VOLE_ROWS_PADDED * sizeof(block_secpar<S>)));
    transpose_secpar<S>(q, macs, CP::VOLE_COL_STRIDE, CP::QUICKSILVER_ROWS_PADDED);
    free(q);

    block_secpar<S> delta_block;
    memcpy(&delta_block, chall_3_sig, sizeof(delta_block));

    public_key<P> pk;
    sig_unpack_public_key(&pk, pk_packed);

    quicksilver_state<S, true, OC::QS_DEGREE> qs(macs, OC::OWF_NUM_CONSTRAINTS, delta_block, chall_2);
    owf_constraints(&qs, &pk);

    uint8_t a0_tilde_local[lambda_bytes];
    qs.verify(OC::WITNESS_BITS, a1_tilde_ptr, a0_tilde_local);
    free(macs);

    // === Step 7: Recompute chall_3 and compare ===
    uint8_t chall_3_check[lambda_bytes];
    hasher.init(S);
    hasher.update_byte(11);
    hasher.update(chall_2, 3 * lambda_bytes + 8);
    hasher.update(a1_tilde_ptr, lambda_bytes);
    hasher.update(a0_tilde_local, lambda_bytes);
    if constexpr (P::use_grinding)
    {
        hasher.update(ctr_sig, P::grinding_counter_size);
    }
    hasher.finalize(chall_3_check, lambda_bytes);

    return memcmp(chall_3_sig, chall_3_check, lambda_bytes) == 0;
}

} // namespace sig

#endif
