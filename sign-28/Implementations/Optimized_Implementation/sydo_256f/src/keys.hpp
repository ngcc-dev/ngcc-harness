#ifndef SYDO_KEYS_HPP
#define SYDO_KEYS_HPP

#include "block.hpp"
#include "polynomials.hpp"
#include "parameters.hpp"
#include "prgs.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace sydo
{

// pk size
template <typename P>
constexpr std::size_t SYDO_PACKED_PUBLIC_KEY_BYTES = P::packed_public_key_bytes;

template <typename P>
constexpr std::size_t SYDO_UNPACKED_PUBLIC_KEY_BYTES = P::unpacked_public_key_bytes;

template <typename P>
constexpr std::size_t SYDO_PUBLIC_KEY_BYTES = SYDO_UNPACKED_PUBLIC_KEY_BYTES<P>;

template <typename P> constexpr std::size_t SYDO_PUBLIC_KEY_VECTOR_ELEMENTS = P::y_vector_len;

// sk size
template <typename P>
constexpr std::size_t SYDO_PACKED_SECRET_KEY_BYTES = P::packed_secret_key_bytes;

template <typename P>
constexpr std::size_t SYDO_UNPACKED_SECRET_KEY_BYTES = P::unpacked_secret_key_bytes;

template <typename P> constexpr std::size_t SYDO_WITNESS_BYTES = P::witness_bytes;
template <typename P>
constexpr std::size_t SYDO_WITNESS_EXTENDED_SUBVECTOR_BITS =
    P::witness_extended_subvector_bits;
template <typename P>
constexpr std::size_t SYDO_WITNESS_EXTENDED_BITS =
    P::witness_rows * SYDO_WITNESS_EXTENDED_SUBVECTOR_BITS<P>;
template <typename P>
constexpr std::size_t SYDO_WITNESS_EXTENDED_BYTES = (SYDO_WITNESS_EXTENDED_BITS<P> + 7) / 8;
template <typename P> constexpr std::size_t SYDO_SEED_SK_BYTES = P::secret_seed_bytes;
template <typename P>
constexpr std::size_t SYDO_KEYGEN_RANDOM_SEED_BYTES = SYDO_SEED_SK_BYTES<P> + P::secpar_bytes;

template <typename P>
constexpr std::size_t SYDO_SECRET_KEY_BYTES = SYDO_UNPACKED_SECRET_KEY_BYTES<P>;

template <typename P> struct sydo_public_key
{
    typename P::prg_t::key_t seed_pk;
    std::array<block_secpar<P::secpar_v>, SYDO_PUBLIC_KEY_VECTOR_ELEMENTS<P>> y{};
};

template <typename P> struct sydo_secret_key
{
    sydo_public_key<P> pk;
    typename P::prg_t::key_t seed_sk;
    std::array<uint8_t, SYDO_WITNESS_BYTES<P>> witness{};
};

template <typename P>
void sydo_pack_public_key(uint8_t* packed, const sydo_public_key<P>* unpacked);

template <typename P>
void sydo_unpack_public_key(sydo_public_key<P>* unpacked, const uint8_t* packed);

template <typename P>
void sydo_pack_secret_key(uint8_t* packed, const sydo_secret_key<P>* unpacked);

template <typename P>
bool sydo_unpack_secret_key(sydo_secret_key<P>* unpacked, const uint8_t* packed);

template <typename P> bool sydo_compute_witness(const sydo_secret_key<P>* sk);

template <typename P>
bool sydo_unpack_sk_and_get_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed,
                                     sydo_secret_key<P>* sk);

template <typename P> bool sydo_seckey(const uint8_t* sk_packed);

template <typename P> bool sydo_pubkey(uint8_t* pk_packed, const uint8_t* sk_packed);

} // namespace sydo

#include "keys.inc"

#endif
