#ifndef VOLEITH_KEYS_HPP
#define VOLEITH_KEYS_HPP

#include "aes.hpp"
#include "block.hpp"
#include "constants.hpp"
#include "lynx_matrices.hpp"
#include "parameters.hpp"

namespace sig
{

// The size of the input to the OWF.
template <typename P>
constexpr std::size_t SIG_IV_BYTES = []
{
    if constexpr (is_owf_with_lynx(P::owf_v))
        return P::secpar_bytes;
    else
        static_assert(dependent_false<P::owf_v>, "unsupported OWF");
}();

// The secret key stores the OWF input and the secret key material.
template <typename P>
constexpr std::size_t SIG_SECRET_KEY_BYTES = P::secpar_bytes + SIG_IV_BYTES<P>;

// The size of the public key is the size of the input and the output of the OWF.
template <typename P>
constexpr std::size_t SIG_PUBLIC_KEY_BYTES =
    SIG_IV_BYTES<P> + P::OWF_CONSTS::OWF_BLOCKS * P::OWF_CONSTS::OWF_BLOCK_SIZE;

template <typename P> struct public_key;

template <typename P>
    requires(is_owf_with_lynx(P::owf_v))
struct public_key<P>
{
    // Seed for matrix derivation (lambda bytes)
    uint8_t seed[P::secpar_bytes];
    // OWF output y (lambda bytes)
    uint8_t owf_output[P::secpar_bytes];
};

template <typename P> struct secret_key;

template <typename P>
    requires(is_owf_with_lynx(P::owf_v))
struct secret_key<P>
{
    public_key<P> pk;
    block_secpar<P::secpar_v> sk;
    lynx::lynx_matrices<P::secpar_v> lynx_mats;
    vole_block witness[P::CONSTS::WITNESS_BLOCKS];
};

template <typename P> bool sig_unpack_secret_key(secret_key<P>* unpacked, const uint8_t* packed);
template <typename P> void sig_pack_public_key(uint8_t* packed, const public_key<P>* unpacked);
template <typename P> void sig_unpack_public_key(public_key<P>* unpacked, const uint8_t* packed);
template <typename P> bool sig_compute_witness(secret_key<P>* sk);
template <typename P>
bool sig_unpack_sk_and_get_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed,
                                    secret_key<P>* sk);

// Check if a byte string is a valid secret key. sk_packed must be SIG_SECRET_KEY_BYTES long.
template <typename P> bool sig_seckey(const uint8_t* sk_packed);

// Find the public key corresponding to a given secret key. Returns true if sk_packed is a valid
// secret key, and false otherwise. For key generation, this function is intended to be called
// repeatedly on random values of sk_packed until a valid key is found. pk_packed must be
// SIG_PUBLIC_KEY_BYTES long, while sk_packed must be SIG_SECRET_KEY_BYTES long.
template <typename P> bool sig_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed);

} // namespace sig

#endif // VOLEITH_KEYS_HPP
