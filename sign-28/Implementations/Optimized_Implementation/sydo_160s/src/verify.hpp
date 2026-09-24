#ifndef SYDO_VERIFY_HPP
#define SYDO_VERIFY_HPP

#include <cstddef>
#include <cstdint>

namespace sydo
{

template <typename P>
bool sydo_verify(const uint8_t* signature, const uint8_t* msg, size_t msg_len, const uint8_t* pk);

} // namespace sydo

#endif
