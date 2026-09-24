/*
 *  SPDX-License-Identifier: MIT
 */

#ifndef SYDO_COMPAT_ENDIAN_H
#define SYDO_COMPAT_ENDIAN_H

#include <stdint.h>

static inline uint16_t bswap16(uint16_t x) {
  return (uint16_t)(((x & UINT16_C(0xff00)) >> 8) | ((x & UINT16_C(0x00ff)) << 8));
}

static inline uint32_t bswap32(uint32_t x) {
  return ((x & UINT32_C(0xff000000)) >> 24) | ((x & UINT32_C(0x00ff0000)) >> 8) |
         ((x & UINT32_C(0x0000ff00)) << 8) | ((x & UINT32_C(0x000000ff)) << 24);
}

static inline uint64_t bswap64(uint64_t x) {
  return ((x & UINT64_C(0xff00000000000000)) >> 56) |
         ((x & UINT64_C(0x00ff000000000000)) >> 40) |
         ((x & UINT64_C(0x0000ff0000000000)) >> 24) |
         ((x & UINT64_C(0x000000ff00000000)) >> 8) |
         ((x & UINT64_C(0x00000000ff000000)) << 8) |
         ((x & UINT64_C(0x0000000000ff0000)) << 24) |
         ((x & UINT64_C(0x000000000000ff00)) << 40) |
         ((x & UINT64_C(0x00000000000000ff)) << 56);
}

#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) &&                                    \
    (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define SYDO_IS_BIG_ENDIAN
#else
#define SYDO_IS_LITTLE_ENDIAN
#endif

#if defined(SYDO_IS_LITTLE_ENDIAN)
#define htobe16(x) bswap16((uint16_t)(x))
#define htole16(x) ((uint16_t)(x))
#define be16toh(x) bswap16((uint16_t)(x))
#define le16toh(x) ((uint16_t)(x))

#define htobe32(x) bswap32((uint32_t)(x))
#define htole32(x) ((uint32_t)(x))
#define be32toh(x) bswap32((uint32_t)(x))
#define le32toh(x) ((uint32_t)(x))

#define htobe64(x) bswap64((uint64_t)(x))
#define htole64(x) ((uint64_t)(x))
#define be64toh(x) bswap64((uint64_t)(x))
#define le64toh(x) ((uint64_t)(x))
#else
#define htobe16(x) ((uint16_t)(x))
#define htole16(x) bswap16((uint16_t)(x))
#define be16toh(x) ((uint16_t)(x))
#define le16toh(x) bswap16((uint16_t)(x))

#define htobe32(x) ((uint32_t)(x))
#define htole32(x) bswap32((uint32_t)(x))
#define be32toh(x) ((uint32_t)(x))
#define le32toh(x) bswap32((uint32_t)(x))

#define htobe64(x) ((uint64_t)(x))
#define htole64(x) bswap64((uint64_t)(x))
#define be64toh(x) ((uint64_t)(x))
#define le64toh(x) bswap64((uint64_t)(x))
#endif

#endif
