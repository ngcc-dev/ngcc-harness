#ifndef SYDO_SIGN_HPP
#define SYDO_SIGN_HPP

#include <cstddef>
#include <cstdint>

namespace sydo
{

template <typename P>
constexpr std::size_t SYDO_SIGNATURE_BYTES = P::CONSTS::SIGNATURE_BYTES;

template <typename P>
bool sydo_sign(uint8_t* signature, const uint8_t* msg, size_t msg_len, const uint8_t* sk,
               const uint8_t* random_seed, size_t random_seed_len);

} // namespace sydo

#endif
