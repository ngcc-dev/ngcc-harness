#ifndef VOLEITH_KEYS_INC
#define VOLEITH_KEYS_INC

#include "aes.hpp"
#include "constants.hpp"
#include "parameters.hpp"
#include "rsd_owf.hpp"
#include "voleith_keys.hpp"

namespace sig
{

template <typename P> bool sig_unpack_secret_key(secret_key<P>* unpacked, const uint8_t* packed)
{
    constexpr auto sk_offset = SIG_IV_BYTES<P>;

    // Resolved-Alpha: packed = seed || secret key material.
    memcpy(unpacked->pk.seed, packed, P::secpar_bytes);
    memcpy(&unpacked->sk, packed + sk_offset, P::secpar_bytes);

    return sig_compute_witness(unpacked);
}

template <typename P> void sig_pack_public_key(uint8_t* packed, const public_key<P>* unpacked)
{
    constexpr auto owf_output_offset = SIG_IV_BYTES<P>;
    memcpy(packed, unpacked->seed, P::secpar_bytes);
    memcpy(packed + owf_output_offset, unpacked->owf_output, P::OWF_CONSTS::OWF_OUTPUT_BYTES);
}

template <typename P> void sig_unpack_public_key(public_key<P>* unpacked, const uint8_t* packed)
{
    constexpr auto owf_output_offset = SIG_IV_BYTES<P>;
    memcpy(unpacked->seed, packed, P::secpar_bytes);
    memcpy(unpacked->owf_output, packed + owf_output_offset, P::OWF_CONSTS::OWF_OUTPUT_BYTES);
}

template <typename P> bool sig_compute_witness(secret_key<P>* sk)
{
    using OC = P::OWF_CONSTS;

    static_assert(is_owf_with_rsd(P::owf_v), "Resolved-Alpha supports only the RSD OWF");

    uint8_t* w_ptr = reinterpret_cast<uint8_t*>(&sk->witness);
    const auto* seed_sk = reinterpret_cast<const uint8_t*>(&sk->sk);

    const auto& matrix_b = rsd::cached_matrix_b<P>(sk->pk.seed);
    rsd::owf_and_extend_witness_with_matrix<P>(sk->pk.owf_output, w_ptr, seed_sk,
                                               matrix_b.data());
    memset(w_ptr + OC::WITNESS_BITS / 8, 0, sizeof(sk->witness) - OC::WITNESS_BITS / 8);
    return true;
}

template <typename P>
bool sig_unpack_sk_and_get_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed, secret_key<P>* sk)
{
    if (!sig_unpack_secret_key(sk, sk_packed))
        return false;

    sig_pack_public_key(pk_packed, &sk->pk);
    return true;
}

template <typename P> bool sig_seckey(const uint8_t* sk_packed)
{
    secret_key<P> sk;
    return sig_unpack_secret_key<P>(&sk, sk_packed);
}

template <typename P> bool sig_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed)
{
    secret_key<P> sk;
    if (!sig_unpack_sk_and_get_pubkey(pk_packed, sk_packed, &sk))
        return false;
    return true;
}

} // namespace sig

#endif
