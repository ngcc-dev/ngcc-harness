#ifndef FAEST_KEYS_HPP
#define FAEST_KEYS_HPP

#include "aes.hpp"
#include "block.hpp"
#include "constants.hpp"
#include "parameters.hpp"

namespace faest
{

// The size of the input to the OWF.
template <typename P>
constexpr std::size_t FAEST_IV_BYTES = []
{
    // The size of the input of the OWF.
    if constexpr (is_owf_with_aes_ecb(P::owf_v))
    {
        if constexpr (is_owf_with_ctr_input(P::owf_v))
            // one AES block
            return P::OWF_CONSTS::OWF_BLOCK_SIZE;
        else
            // one or two AES blocks
            return P::OWF_CONSTS::OWF_BLOCKS * P::OWF_CONSTS::OWF_BLOCK_SIZE;
    }
    else if constexpr (is_owf_with_aes_em(P::owf_v))
        // one AES/Rijndael block
        return P::secpar_bytes;
    else if constexpr (is_owf_galas_owf(P::owf_v))
        return P::secpar_bytes;
    else
        static_assert(false, "unsupported OWF");
}();

// The size of the FAEST secret key is the size of the secret key and the input to the OWF.
template <typename P>
constexpr std::size_t FAEST_SECRET_KEY_BYTES = P::secpar_bytes + FAEST_IV_BYTES<P>;

// The size of the public key is the size of the input and the output of the OWF.
template <typename P>
constexpr std::size_t FAEST_PUBLIC_KEY_BYTES =
    FAEST_IV_BYTES<P> + P::OWF_CONSTS::OWF_BLOCKS * P::OWF_CONSTS::OWF_BLOCK_SIZE;

template <typename P> struct public_key;

template <typename P>
    requires(is_owf_with_aes_ecb(P::owf_v))
struct public_key<P>

{
    P::OWF_CONSTS::block_t owf_input[P::OWF_CONSTS::OWF_BLOCKS];
    P::OWF_CONSTS::block_t owf_output[P::OWF_CONSTS::OWF_BLOCKS];
};

template <typename P>
    requires(is_owf_with_aes_em(P::owf_v))
struct public_key<P>
{
    block_secpar<P::secpar_v> owf_input[1];
    rijndael_round_keys<P::secpar_v> fixed_key;
    P::OWF_CONSTS::block_t owf_output[P::OWF_CONSTS::OWF_BLOCKS];
};

template <typename P>
    requires(is_owf_galas_owf(P::owf_v))
struct public_key<P>

{
    P::OWF_CONSTS::block_t owf_input[P::OWF_CONSTS::OWF_BLOCKS];
    P::OWF_CONSTS::block_t owf_output[P::OWF_CONSTS::OWF_BLOCKS];
};

template <typename P> struct secret_key;

template <typename P>
    requires(is_owf_with_aes_ecb(P::owf_v))
struct secret_key<P>
{
    public_key<P> pk;
    block_secpar<P::secpar_v> sk;
    aes_round_keys<P::secpar_v> round_keys;
    vole_block witness[P::CONSTS::WITNESS_BLOCKS];
};

template <typename P>
    requires(is_owf_with_aes_em(P::owf_v))
struct secret_key<P>
{
    public_key<P> pk;
    block_secpar<P::secpar_v> sk;
    vole_block witness[P::CONSTS::WITNESS_BLOCKS];
};

template <typename P>
    requires(is_owf_galas_owf(P::owf_v))
struct secret_key<P>
{
    public_key<P> pk;
    block_secpar<P::secpar_v> sk;
    vole_block witness[P::CONSTS::WITNESS_BLOCKS];
};

template <typename P> bool faest_unpack_secret_key(secret_key<P>* unpacked, const uint8_t* packed);
template <typename P> void faest_pack_public_key(uint8_t* packed, const public_key<P>* unpacked);
template <typename P> void faest_unpack_public_key(public_key<P>* unpacked, const uint8_t* packed);
template <typename P> bool faest_compute_witness(secret_key<P>* sk);
template <typename P>
bool faest_unpack_sk_and_get_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed,
                                    secret_key<P>* sk);

// Check if a byte string is a valid secret key. sk_packed must be FAEST_SECRET_KEY_BYTES long.
template <typename P> bool faest_seckey(const uint8_t* sk_packed);

// Find the public key corresponding to a given secret key. Returns true if sk_packed is a valid
// secret key, and false otherwise. For key generation, this function is intended to be called
// repeatedly on random values of sk_packed until a valid key is found. pk_packed must be
// FAEST_PUBLIC_KEY_BYTES long, while sk_packed must be FAEST_SECRET_KEY_BYTES long.
template <typename P> bool faest_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed);

} // namespace faest

#endif // FAEST_KEYS_H
