#ifndef SYDO_KEYGEN_HPP
#define SYDO_KEYGEN_HPP

#include <cstddef>
#include <cstdint>

namespace sydo
{

template <typename P>
bool sydo_keygen(uint8_t* pk, uint8_t* sk, const uint8_t* random_seed,
                 std::size_t random_seed_len);

template <typename P> bool sydo_derive_public_key(uint8_t* pk, const uint8_t* sk);

} // namespace sydo

#endif
