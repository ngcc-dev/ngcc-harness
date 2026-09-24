#ifndef VOLEITH_KEYS_INC
#define VOLEITH_KEYS_INC

#include "aes.hpp"
#include "constants.hpp"
#include "voleith_keys.hpp"
#include "lynx_owf.hpp"
#include "parameters.hpp"

namespace sig
{

template <typename P> bool sig_unpack_secret_key(secret_key<P>* unpacked, const uint8_t* packed)
{
    constexpr auto sk_offset = SIG_IV_BYTES<P>;

    // Lynx: packed = seed || key
    memcpy(unpacked->pk.seed, packed, P::secpar_bytes);
    memcpy(&unpacked->sk, packed + sk_offset, P::secpar_bytes);

    return sig_compute_witness(unpacked);
}

template <typename P> void sig_pack_public_key(uint8_t* packed, const public_key<P>* unpacked)
{
    constexpr auto owf_output_offset = SIG_IV_BYTES<P>;
    // Lynx: seed || owf_output
    memcpy(packed, unpacked->seed, P::secpar_bytes);
    memcpy(packed + owf_output_offset, unpacked->owf_output, P::secpar_bytes);
}

template <typename P> void sig_unpack_public_key(public_key<P>* unpacked, const uint8_t* packed)
{
    constexpr auto owf_output_offset = SIG_IV_BYTES<P>;
    // Lynx: seed || owf_output
    memcpy(unpacked->seed, packed, P::secpar_bytes);
    memcpy(unpacked->owf_output, packed + owf_output_offset, P::secpar_bytes);
}

template <typename P> bool sig_compute_witness(secret_key<P>* sk)
{
    using OC = P::OWF_CONSTS;
    constexpr auto S = P::secpar_v;
    constexpr std::size_t lambda = P::secpar_bits;
    constexpr std::size_t lambda_bytes = P::secpar_bytes;

    uint8_t* w_ptr = (uint8_t*)&sk->witness;
    memcpy(w_ptr, &sk->sk, lambda_bytes);
    w_ptr += lambda_bytes;

    // Initialize matrices from seed
    lynx::lynx_init(sk->lynx_mats, sk->pk.seed, lambda_bytes);
    const auto& mats = sk->lynx_mats;

    // Load key as a GF(2^lambda) element
    using gf = poly_secpar<S>;
    gf k = gf::load(reinterpret_cast<const uint8_t*>(&sk->sk));
    gf c0 = gf::load(mats.C0.data());
    gf c1 = gf::load(mats.C1.data());

    // v1 = S1(k XOR c0)
    gf s1_input = k + c0;
    if constexpr (!is_owf_with_zero_sboxes(P::owf_v))
    {
        if (s1_input == gf::set_zero())
            return false;
    }
    gf v1 = lynx::lynx_poly_s1<S>(s1_input);

    // v2 = S2(L0(k) XOR c1) for lambda != 160
    //    = S2(L0(k) XOR c0) for lambda == 160
    constexpr std::size_t W = (lambda + 63) / 64;
    gf L0k = lynx::lynx_L<S>(reinterpret_cast<const uint64_t(*)[W]>(mats.L0.data()), k);
    gf s2_input;
    if constexpr (S == secpar::s160)
        s2_input = L0k + c0;
    else
        s2_input = L0k + c1;
    if constexpr (!is_owf_with_zero_sboxes(P::owf_v))
    {
        if (s2_input == gf::set_zero())
            return false;
    }
    gf v2 = lynx::lynx_poly_s2<S>(s2_input);

    // Store v1, v2 into witness
    v1.store(w_ptr);
    w_ptr += lambda_bytes;
    v2.store(w_ptr);
    w_ptr += lambda_bytes;

    // Compute OWF output: y = L1(v1) * L2(v2) + L3(k + v1 + v2)
    gf L1v1 = lynx::lynx_L<S>(reinterpret_cast<const uint64_t(*)[W]>(mats.L1.data()), v1);
    gf L2v2 = lynx::lynx_L<S>(reinterpret_cast<const uint64_t(*)[W]>(mats.L2.data()), v2);
    gf L3_sum = lynx::lynx_L<S>(reinterpret_cast<const uint64_t(*)[W]>(mats.L3.data()), k + v1 + v2);

    gf y;
    if constexpr (S == secpar::s256 || S == secpar::s384 || S == secpar::s512)
    {
        gf c2 = gf::load(mats.C2.data());
        gf inv_kc2 = [&]() {
            if constexpr (S == secpar::s256)
                return lynx::lynx_poly_inv_256(k + c2);
            else if constexpr (S == secpar::s384)
                return lynx::lynx_poly_inv_384(k + c2);
            else
                return lynx::lynx_poly_inv_512(k + c2);
        }();
        y = lynx::lynx_poly_mul<S>(inv_kc2, lynx::lynx_poly_mul<S>(L1v1, L2v2)) + L3_sum;
    }
    else
    {
        y = lynx::lynx_poly_mul<S>(L1v1, L2v2) + L3_sum;
    }
    y.store(sk->pk.owf_output);

    SIG_ASSERT(w_ptr - (uint8_t*)&sk->witness == OC::WITNESS_BITS / 8);
    memset(w_ptr, 0, sizeof(sk->witness) - OC::WITNESS_BITS / 8);
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
