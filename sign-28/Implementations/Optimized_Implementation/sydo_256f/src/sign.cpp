#include "api_common.inc"
#include "profile.hpp"
#include <cstdlib>

namespace sydo
{

template <typename P>
bool sydo_sign(uint8_t* signature, const uint8_t* msg, size_t msg_len, const uint8_t* sk_packed,
               const uint8_t* random_seed, size_t random_seed_len)
{
    using CP = P::CONSTS;
    using OC = P::OWF_CONSTS;
    constexpr auto S = P::secpar_v;

    sydo_secret_key<P> sk;
    uint8_t pk_packed[SYDO_PACKED_PUBLIC_KEY_BYTES<P>];
    if (!sydo_unpack_sk_and_get_pubkey<P>(pk_packed, sk_packed, &sk))
        return false;

    hash_state hasher;

    block_2secpar<S> mu;
    {
        profile::scope timer(profile::SIGN_MU);
        hasher.init(S);
        hasher.reserve(SYDO_PACKED_PUBLIC_KEY_BYTES<P> + msg_len + 1);
        hasher.update(pk_packed, SYDO_PACKED_PUBLIC_KEY_BYTES<P>);
        hasher.update(msg, msg_len);
        hasher.update_byte(8 + 0);
        hasher.finalize(&mu, sizeof(mu));
    }

    block_secpar<S> seed;
    typename P::vole_prg_t::iv_t iv;
    std::array<uint8_t, sizeof(seed) + sizeof(iv)> seed_iv;
    {
        profile::scope timer(profile::SIGN_SEED_IV);
        hasher.init(S);
        hasher.reserve(sizeof(sk.seed_sk) + sizeof(mu) + random_seed_len + 1);
        hasher.update(&sk.seed_sk, sizeof(sk.seed_sk));
        hasher.update(&mu, sizeof(mu));
        if (random_seed)
            hasher.update(random_seed, random_seed_len);
        hasher.update_byte(3);
        hasher.finalize(seed_iv.data(), sizeof(seed_iv));
        memcpy(&seed, seed_iv.data(), sizeof(seed));
        memcpy(&iv, &seed_iv[sizeof(seed)], sizeof(iv));
    }

    block_secpar<S>* forest = reinterpret_cast<block_secpar<S>*>(
        aligned_alloc(alignof(block_secpar<S>), P::bavc_t::COMMIT_NODES * sizeof(block_secpar<S>)));
    constexpr size_t hashed_leaves_bytes = P::bavc_t::COMMIT_LEAVES * P::leaf_hash_t::hash_len;
    constexpr size_t hashed_leaves_align = alignof(block_2secpar<S>);
    constexpr size_t hashed_leaves_alloc_bytes =
        ((hashed_leaves_bytes + 1 + hashed_leaves_align - 1) / hashed_leaves_align) *
        hashed_leaves_align;
    unsigned char* hashed_leaves = reinterpret_cast<unsigned char*>(
        aligned_alloc(hashed_leaves_align, hashed_leaves_alloc_bytes));
    vole_block* u = reinterpret_cast<vole_block*>(
        aligned_alloc(alignof(vole_block), CP::VOLE_COL_BLOCKS * sizeof(vole_block)));
    vole_block* v = reinterpret_cast<vole_block*>(aligned_alloc(
        alignof(vole_block), P::secpar_bits * CP::VOLE_COL_BLOCKS * sizeof(vole_block)));
    uint8_t vole_commit_check[CP::VOLE_COMMIT_CHECK_SIZE];

    sydo_vole_commit<P>(seed, iv, forest, hashed_leaves, u, v, signature, vole_commit_check);

    std::array<uint8_t, CP::VOLE_CHECK::CHALLENGE_BYTES> chal1;
    {
        profile::scope timer(profile::SIGN_CHAL1);
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
        profile::scope timer(profile::SIGN_VOLE_CHECK_CHAL2);
        hasher.init(S);
        hasher.reserve(sizeof(chal1) + CP::VOLE_CHECK::PROOF_BYTES + OC::WITNESS_BITS / 8 + 1);
        hasher.update(chal1.data(), sizeof(chal1));

        uint8_t* vole_check_proof = signature + CP::VOLE_COMMIT_SIZE;
        sydo_vole_check_sender<P>(u, v, chal1.data(), vole_check_proof, hasher);

        uint8_t* correction = vole_check_proof + CP::VOLE_CHECK::PROOF_BYTES;
        size_t remainder = (OC::WITNESS_BITS / 8) % (16 * CP::VOLE_BLOCK);
        constexpr size_t correction_block_bytes = sizeof(vole_block);
        const uint8_t* u_bytes = reinterpret_cast<const uint8_t*>(u);
        for (size_t i = 0; i < CP::WITNESS_BLOCKS - (remainder != 0); ++i)
        {
            const size_t off = i * correction_block_bytes;
            memcpy(correction + off, u_bytes + off, correction_block_bytes);
            for (size_t j = 0; j < correction_block_bytes; ++j)
                correction[off + j] ^= sk.witness[off + j];
        }
        if (remainder)
        {
            const size_t off = (CP::WITNESS_BLOCKS - 1) * correction_block_bytes;
            memcpy(correction + off, u_bytes + off, remainder);
            for (size_t j = 0; j < remainder; ++j)
                correction[off + j] ^= sk.witness[off + j];
        }

        hasher.update(correction, OC::WITNESS_BITS / 8);
        hasher.update_byte(8 + 2);
        hasher.finalize(chal2.data(), sizeof(chal2));
    }

    uint8_t* vole_check_proof = signature + CP::VOLE_COMMIT_SIZE;
    uint8_t* correction = vole_check_proof + CP::VOLE_CHECK::PROOF_BYTES;

    constexpr size_t u_extended_bytes =
        P::witness_extended_bytes + (OC::QS_DEGREE - 1) * P::secpar_bytes;
    constexpr size_t u_extended_alloc_bytes =
        ((u_extended_bytes + sizeof(vole_block) - 1) / sizeof(vole_block)) * sizeof(vole_block);
    uint8_t* u_extended =
        reinterpret_cast<uint8_t*>(aligned_alloc(alignof(vole_block), u_extended_alloc_bytes));
    uint8_t* qs_proof = correction + OC::WITNESS_BITS / 8;
    std::array<uint8_t, CP::QS::CHECK_BYTES> qs_check;
    {
        profile::scope timer(profile::SIGN_QS_OWF_PROVE);
        block_secpar<S>* macs = nullptr;
        block_secpar<S>* macs_extended = nullptr;
        {
            profile::scope alloc_timer(profile::SIGN_QS_ALLOC);
            macs = reinterpret_cast<block_secpar<S>*>(aligned_alloc(
                alignof(block_secpar<S>), CP::QUICKSILVER_ROWS_PADDED * sizeof(block_secpar<S>)));
            macs_extended = reinterpret_cast<block_secpar<S>*>(aligned_alloc(
                alignof(block_secpar<S>),
                CP::QUICKSILVER_ROWS_PADDED_EXTENDED * sizeof(block_secpar<S>)));
        }
        {
            profile::scope prep_timer(profile::SIGN_QS_PREP);
            memset(u_extended, 0, u_extended_alloc_bytes);
            sydo_detail::compute_witness<P>(u_extended, sk.witness.data());
            memcpy(u_extended + P::witness_extended_bytes,
                   reinterpret_cast<const uint8_t*>(u) + OC::WITNESS_BITS / 8,
                   (OC::QS_DEGREE - 1) * P::secpar_bytes);
            free(u);
            sydo_detail::compute_macs_extended<P>(macs_extended, v, macs);
            free(macs);
            free(v);
        }

        const uint64_t qs_init_start = profile::now_ns();
        quicksilver_state<S, false, OC::QS_DEGREE, P::quicksilver_term_v> qs(
            u_extended, macs_extended, OC::OWF_NUM_CONSTRAINTS, chal2.data());
        profile::add_time(profile::SIGN_QS_STATE_INIT, profile::now_ns() - qs_init_start);
        uint8_t folded_poly_coeffs[OC::QS_DEGREE * sydo_secpar_to_bytes(S)] = {0};
        {
            profile::scope owf_timer(profile::SIGN_OWF_CONSTRAINTS_TOTAL);
            sydo_owf_constraints(&qs, &sk.pk, folded_poly_coeffs);
        }

        {
            profile::scope prove_timer(profile::SIGN_QS_PROVE);
            qs.prove(P::witness_extended_bytes * 8, qs_proof, qs_check.data());
            for (size_t deg = 0; deg < OC::QS_DEGREE; ++deg)
            {
                uint8_t* dst_coeff = nullptr;
                if (deg == 0)
                    dst_coeff = qs_check.data();
                else
                    dst_coeff = qs_proof + (deg - 1) * P::secpar_bytes;

                const uint8_t* src_coeff = folded_poly_coeffs + deg * P::secpar_bytes;
                for (size_t j = 0; j < P::secpar_bytes; ++j)
                    dst_coeff[j] ^= src_coeff[j];
            }
        }
        {
            profile::scope free_timer(profile::SIGN_QS_FREE);
            free(macs_extended);
            free(u_extended);
        }
    }
    uint8_t* veccom_open_start = qs_proof + CP::QS::PROOF_BYTES;
    uint8_t* delta = veccom_open_start + P::bavc_t::OPEN_SIZE;

    uint8_t* iv_dst = delta + sizeof(block_secpar<S>);
    memcpy(iv_dst, &iv, sizeof(iv));
    uint8_t* grinding_counter_dst = iv_dst + sizeof(iv);

    if constexpr (!P::use_grinding)
    {
        profile::scope timer(profile::SIGN_GRIND_OPEN);
        hasher.init(S);
        hasher.reserve(sizeof(chal2) + CP::QS::CHECK_BYTES + CP::QS::PROOF_BYTES + 1);
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
        profile::scope timer(profile::SIGN_GRIND_OPEN);
        hash_state_x8 grinding_hasher;
        grinding_hasher.init(S);
        grinding_hasher.reserve_shared(sizeof(chal2) + CP::QS::CHECK_BYTES + CP::QS::PROOF_BYTES);
        grinding_hasher.update_1(chal2.data(), sizeof(chal2));
        grinding_hasher.update_1(qs_check.data(), CP::QS::CHECK_BYTES);
        grinding_hasher.update_1(qs_proof, CP::QS::PROOF_BYTES);
        uint32_t counter;
        bool open_success = grind_and_open<typename P::bavc_t>(
            forest, hashed_leaves, delta, veccom_open_start, &grinding_hasher, &counter);
        SYDO_ASSERT(open_success);
        if (!open_success)
        {
            free(forest);
            free(hashed_leaves);
            return false;
        }
        grinding_counter_dst[0] = counter;
        grinding_counter_dst[1] = counter >> 8;
        grinding_counter_dst[2] = counter >> 16;
        grinding_counter_dst[3] = counter >> 24;
    }

    free(forest);
    free(hashed_leaves);

    SYDO_ASSERT(grinding_counter_dst + P::grinding_counter_size ==
                 signature + SYDO_SIGNATURE_BYTES<P>);

    return true;
}

#define SYDO_INSTANTIATE_SIGN(...)                                                               \
    template bool sydo_sign<__VA_ARGS__>(uint8_t*, const uint8_t*, size_t, const uint8_t*,       \
                                           const uint8_t*, size_t)

#if defined(SYDO_API_PARAM_TYPE)
SYDO_INSTANTIATE_SIGN(SYDO_API_PARAM_TYPE);
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

SYDO_INSTANTIATE_SIGN(sydo_128_s);
SYDO_INSTANTIATE_SIGN(sydo_128_f);
#if defined(SYDO_ENABLE_CST_PROFILE)
SYDO_INSTANTIATE_SIGN(sydo_128_s_cst);
SYDO_INSTANTIATE_SIGN(sydo_128_f_cst);
#endif
SYDO_INSTANTIATE_SIGN(sydo_160_s);
SYDO_INSTANTIATE_SIGN(sydo_160_f);
SYDO_INSTANTIATE_SIGN(sydo_192_s);
SYDO_INSTANTIATE_SIGN(sydo_192_f);
SYDO_INSTANTIATE_SIGN(sydo_256_s);
SYDO_INSTANTIATE_SIGN(sydo_256_f);
SYDO_INSTANTIATE_SIGN(sydo_512_s);
SYDO_INSTANTIATE_SIGN(sydo_512_f);

#endif

#undef SYDO_INSTANTIATE_SIGN

} // namespace sydo
