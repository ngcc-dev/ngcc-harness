#ifndef SYDO_HPP
#define SYDO_HPP

#include <cstddef>
#include <cstdint>

#include "keys.hpp"

namespace sydo
{

template <typename P>
constexpr std::size_t SYDO_SIGNATURE_BYTES = P::CONSTS::SIGNATURE_BYTES;

template <typename P>
bool sydo_keygen(uint8_t* pk, uint8_t* sk, const uint8_t* random_seed,
                 std::size_t random_seed_len);

template <typename P> bool sydo_derive_public_key(uint8_t* pk, const uint8_t* sk);

template <typename P>
bool sydo_sign(uint8_t* signature, const uint8_t* msg, size_t msg_len, const uint8_t* sk,
                 const uint8_t* random_seed, size_t random_seed_len);

template <typename P>
bool sydo_verify(const uint8_t* signature, const uint8_t* msg, size_t msg_len, const uint8_t* pk);

} // namespace sydo

#endif
