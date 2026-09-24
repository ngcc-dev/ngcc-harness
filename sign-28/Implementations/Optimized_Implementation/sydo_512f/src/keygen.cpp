#include "api_common.inc"

namespace sydo
{

template <typename P> bool sydo_derive_public_key(uint8_t* pk, const uint8_t* sk)
{
    return sydo_pubkey<P>(pk, sk);
}

template <typename P>
bool sydo_keygen(uint8_t* pk, uint8_t* sk, const uint8_t* random_seed,
                 std::size_t random_seed_len)
{
    if (!pk || !sk || !random_seed)
        return false;
    if (random_seed_len != SYDO_KEYGEN_RANDOM_SEED_BYTES<P>)
        return false;

    sydo_secret_key<P> unpacked_sk{};
    const uint8_t* seed_sk = random_seed;
    const uint8_t* seed_pk = random_seed + SYDO_SEED_SK_BYTES<P>;

    unpacked_sk.seed_sk = P::prg_t::key_t::set_zero();
    unpacked_sk.pk.seed_pk = P::prg_t::key_t::set_zero();
    memcpy(&unpacked_sk.seed_sk, seed_sk, SYDO_SEED_SK_BYTES<P>);
    memcpy(&unpacked_sk.pk.seed_pk, seed_pk, P::secpar_bytes);

    std::array<uint8_t, sydo_detail::rsd_pos_stream_bytes<P>> x_positions{};
    sydo_detail::sydo_sample_x_positions_from_seed<P>(
        x_positions.data(), reinterpret_cast<const uint8_t*>(&unpacked_sk.seed_sk),
        SYDO_SEED_SK_BYTES<P>);

    if (!sydo_detail::compute_witness_from_x_positions<P>(&unpacked_sk.witness,
                                                            x_positions.data()))
        return false;

    sydo_detail::sydo_compute_y_from_x_positions_on_demand<P>(
        &unpacked_sk.pk, x_positions.data(), unpacked_sk.pk.seed_pk);

    sydo_pack_secret_key<P>(sk, &unpacked_sk);
    sydo_pack_public_key<P>(pk, &unpacked_sk.pk);
    return true;
}

#define SYDO_INSTANTIATE_KEYGEN(...)                                                             \
    template bool sydo_derive_public_key<__VA_ARGS__>(uint8_t*, const uint8_t*);                 \
    template bool sydo_keygen<__VA_ARGS__>(uint8_t*, uint8_t*, const uint8_t*, std::size_t)

#if defined(SYDO_API_PARAM_TYPE)
SYDO_INSTANTIATE_KEYGEN(SYDO_API_PARAM_TYPE);
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

SYDO_INSTANTIATE_KEYGEN(sydo_128_s);
SYDO_INSTANTIATE_KEYGEN(sydo_128_f);
#if defined(SYDO_ENABLE_CST_PROFILE)
SYDO_INSTANTIATE_KEYGEN(sydo_128_s_cst);
SYDO_INSTANTIATE_KEYGEN(sydo_128_f_cst);
#endif
SYDO_INSTANTIATE_KEYGEN(sydo_160_s);
SYDO_INSTANTIATE_KEYGEN(sydo_160_f);
SYDO_INSTANTIATE_KEYGEN(sydo_192_s);
SYDO_INSTANTIATE_KEYGEN(sydo_192_f);
SYDO_INSTANTIATE_KEYGEN(sydo_256_s);
SYDO_INSTANTIATE_KEYGEN(sydo_256_f);
SYDO_INSTANTIATE_KEYGEN(sydo_512_s);
SYDO_INSTANTIATE_KEYGEN(sydo_512_f);

#endif

#undef SYDO_INSTANTIATE_KEYGEN

} // namespace sydo
