#include "api_common.inc"
#include "profile.hpp"
#include <cstdlib>

namespace sydo
{

template <typename P>
bool sydo_verify(const uint8_t* signature, const uint8_t* msg, size_t msg_len, const uint8_t* pk)
{
    using CP = typename P::CONSTS;
    using OC = typename P::OWF_CONSTS;
    constexpr auto S = P::secpar_v;

    if (!signature || !pk || (!msg && msg_len != 0))
        return false;

    hash_state hasher;

    block_2secpar<S> mu;
    {
        profile::scope timer(profile::VERIFY_MU_IV);
        hasher.init(S);
        hasher.reserve(SYDO_PACKED_PUBLIC_KEY_BYTES<P> + msg_len + 1);
        hasher.update(pk, SYDO_PACKED_PUBLIC_KEY_BYTES<P>);
        hasher.update(msg, msg_len);
        hasher.update_byte(8 + 0);
        hasher.finalize(&mu, sizeof(mu));
    }

    typename P::vole_prg_t::iv_t iv;

    const uint8_t* vole_check_proof = signature + CP::VOLE_COMMIT_SIZE;
    const uint8_t* correction = vole_check_proof + CP::VOLE_CHECK::PROOF_BYTES;
    const uint8_t* qs_proof = correction + OC::WITNESS_BITS / 8;
    const uint8_t* veccom_open_start = qs_proof + CP::QS::PROOF_BYTES;
    const uint8_t* delta = veccom_open_start + P::bavc_t::OPEN_SIZE;
    const uint8_t* iv_ptr = delta + sizeof(block_secpar<S>);
    const uint8_t* counter = iv_ptr + sizeof(iv);

    for (size_t i = sydo_secpar_to_bits(S) - 1; i >= P::delta_bits_v; --i)
    {
        if ((delta[i / 8] >> (i % 8)) & 1)
            return false;
    }

    memcpy(&iv, iv_ptr, sizeof(iv));

    std::array<uint8_t, P::delta_bits_v> delta_bytes;
    ::sydo::expand_bits_to_bytes(delta_bytes.data(), P::delta_bits_v, delta);

    vole_block* q = reinterpret_cast<vole_block*>(aligned_alloc(
        alignof(vole_block), P::secpar_bits * CP::VOLE_COL_BLOCKS * sizeof(vole_block)));
    uint8_t vole_commit_check[CP::VOLE_COMMIT_CHECK_SIZE];

    bool reconstruct_ok = false;
    {
        reconstruct_ok = ::sydo::vole_reconstruct<P>(iv, q, delta_bytes.data(), signature,
                                                     veccom_open_start, vole_commit_check);
    }
    if (!reconstruct_ok)
    {
        free(q);
        return false;
    }

    std::array<uint8_t, CP::VOLE_CHECK::CHALLENGE_BYTES> chal1;
    {
        profile::scope timer(profile::VERIFY_CHAL1);
        hasher.init(S);
        hasher.reserve(sizeof(mu) + CP::VOLE_COMMIT_CHECK_SIZE + CP::VOLE_COMMIT_SIZE +
                       sizeof(iv) + 1);
        hasher.update(&mu, sizeof(mu));
        hasher.update(vole_commit_check, CP::VOLE_COMMIT_CHECK_SIZE);
        hasher.update(signature, CP::VOLE_COMMIT_SIZE);
        hasher.update(&iv, sizeof(iv));
        hasher.update_byte(8 + 1);
        hasher.finalize(chal1.data(), sizeof(chal1));
    }

    std::array<uint8_t, CP::QS::CHALLENGE_BYTES> chal2;
    {
        profile::scope timer(profile::VERIFY_VOLE_CHECK_CHAL2);
        hasher.init(S);
        hasher.reserve(sizeof(chal1) + CP::VOLE_CHECK::PROOF_BYTES + OC::WITNESS_BITS / 8 + 1);
        hasher.update(chal1.data(), sizeof(chal1));
        hasher.update(vole_check_proof, CP::VOLE_CHECK::PROOF_BYTES);
        ::sydo::vole_check_receiver<P>(q, delta_bytes.data(), chal1.data(), vole_check_proof,
                                       hasher);
        hasher.update(correction, OC::WITNESS_BITS / 8);
        hasher.update_byte(8 + 2);
        hasher.finalize(chal2.data(), sizeof(chal2));
    }

    std::array<uint8_t, CP::QS::CHECK_BYTES> qs_check;
    {
        profile::scope timer(profile::VERIFY_QS_OWF_VERIFY);
        block_secpar<S>* macs = reinterpret_cast<block_secpar<S>*>(aligned_alloc(
            alignof(block_secpar<S>), CP::QUICKSILVER_ROWS_PADDED * sizeof(block_secpar<S>)));
        block_secpar<S>* macs_extended = reinterpret_cast<block_secpar<S>*>(aligned_alloc(
            alignof(block_secpar<S>),
            CP::QUICKSILVER_ROWS_PADDED_EXTENDED * sizeof(block_secpar<S>)));
        block_secpar<S> delta_block;
        sydo_public_key<P> unpacked_pk;
        {
            profile::scope prep_timer(profile::VERIFY_QS_PREP);
            std::array<vole_block, CP::WITNESS_BLOCKS> correction_blocks{};
            memcpy(correction_blocks.data(), correction, OC::WITNESS_BITS / 8);
            ::sydo::vole_receiver_apply_correction<P>(CP::WITNESS_BLOCKS, P::delta_bits_v,
                                                       correction_blocks.data(), q,
                                                       delta_bytes.data());

            memcpy(&delta_block, delta, sizeof(delta_block));
            sydo_detail::compute_macs_extended_verifier<P>(macs_extended, q, macs, delta_block);
            free(q);
            free(macs);

            sydo_unpack_public_key<P>(&unpacked_pk, pk);
        }

        quicksilver_state<S, true, OC::QS_DEGREE, P::quicksilver_term_v> qs(
            macs_extended, OC::OWF_NUM_CONSTRAINTS, delta_block, chal2.data());
        std::array<uint8_t, OC::QS_DEGREE * P::secpar_bytes> folded_poly_coeffs{};
        sydo_owf_constraints<P, true>(&qs, &unpacked_pk, folded_poly_coeffs.data());

        {
            profile::scope verify_timer(profile::VERIFY_QS_VERIFY);
            qs.verify(P::witness_extended_bytes * 8, qs_proof, qs_check.data());
            for (size_t j = 0; j < P::secpar_bytes; ++j)
                qs_check[j] ^= folded_poly_coeffs[j];
        }
        free(macs_extended);
    }
    block_secpar<S> delta_check;
    {
        profile::scope timer(profile::VERIFY_DELTA_CHECK);
        hasher.init(S);
        hasher.reserve(sizeof(chal2) + CP::QS::CHECK_BYTES + CP::QS::PROOF_BYTES +
                       P::grinding_counter_size + 1);
        hasher.update(chal2.data(), sizeof(chal2));
        hasher.update(qs_check.data(), CP::QS::CHECK_BYTES);
        hasher.update(qs_proof, CP::QS::PROOF_BYTES);
        if constexpr (P::use_grinding)
            hasher.update(counter, P::grinding_counter_size);
        hasher.update_byte(8 + 3);
        hasher.finalize(&delta_check, sizeof(delta_check));
    }

    return memcmp(delta, &delta_check, sizeof(delta_check)) == 0;
}

#define SYDO_INSTANTIATE_VERIFY(...)                                                             \
    template bool sydo_verify<__VA_ARGS__>(const uint8_t*, const uint8_t*, size_t, const uint8_t*)

#if defined(SYDO_API_PARAM_TYPE)
SYDO_INSTANTIATE_VERIFY(SYDO_API_PARAM_TYPE);
#else
#if defined(SYDO_ENABLE_CST_PROFILE)
using sydo_128_s_cst =
    sydo_parameter_set<secpar::s128, 11, owf::rsd, prg::aes_ctr, prg::aes_ctr,
                         leaf_hash::aes_ctr_stat_bind, 7,
                         std::pair<bavc, std::size_t>{bavc::one_tree, 102},
                         quicksilver_term::constant>;
using sydo_128_f_cst =
    sydo_parameter_set<secpar::s128, 16, owf::rsd, prg::aes_ctr, prg::aes_ctr,
                         leaf_hash::aes_ctr_stat_bind, 8,
                         std::pair<bavc, std::size_t>{bavc::one_tree, 110},
                         quicksilver_term::constant>;
#endif

SYDO_INSTANTIATE_VERIFY(sydo_128_s);
SYDO_INSTANTIATE_VERIFY(sydo_128_f);
#if defined(SYDO_ENABLE_CST_PROFILE)
SYDO_INSTANTIATE_VERIFY(sydo_128_s_cst);
SYDO_INSTANTIATE_VERIFY(sydo_128_f_cst);
#endif
SYDO_INSTANTIATE_VERIFY(sydo_160_s);
SYDO_INSTANTIATE_VERIFY(sydo_160_f);
SYDO_INSTANTIATE_VERIFY(sydo_192_s);
SYDO_INSTANTIATE_VERIFY(sydo_192_f);
SYDO_INSTANTIATE_VERIFY(sydo_256_s);
SYDO_INSTANTIATE_VERIFY(sydo_256_f);
SYDO_INSTANTIATE_VERIFY(sydo_512_s);
SYDO_INSTANTIATE_VERIFY(sydo_512_f);

#endif

#undef SYDO_INSTANTIATE_VERIFY

} // namespace sydo
