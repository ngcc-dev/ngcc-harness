/*
 * endian compatibility functions adapted from UBLOCKITH
 */

#ifndef COMPAT_ENDIAN_H
#define COMPAT_ENDIAN_H

#include <stdint.h>
#include "macros.h"

static inline uint16_t bswap16(uint16_t x) {
  return ((x & 0xff00) >> 8) | ((x & 0x00ff) << 8);
}

static inline uint32_t bswap32(uint32_t x) {
  return ((x & 0xff000000) >> 24) | ((x & 0x00ff0000) >> 8) | ((x & 0x0000ff00) << 8) |
         ((x & 0x000000ff) << 24);
}

static inline uint64_t bswap64(uint64_t x) {
  return ((x & UINT64_C(0xff00000000000000)) >> 56) | ((x & UINT64_C(0x00ff000000000000)) >> 40) |
         ((x & UINT64_C(0x0000ff0000000000)) >> 24) | ((x & UINT64_C(0x000000ff00000000)) >> 8) |
         ((x & UINT64_C(0x00000000ff000000)) << 8) | ((x & UINT64_C(0x0000000000ff0000)) << 24) |
         ((x & UINT64_C(0x000000000000ff00)) << 40) | ((x & UINT64_C(0x00000000000000ff)) << 56);
}
#define htobe16(x) bswap16((x))
#define htole16(x) ((uint16_t)(x))
#define be16toh(x) bswap16((x))
#define le16toh(x) ((uint16_t)(x))

#define htobe32(x) bswap32((x))
#define htole32(x) ((uint32_t)(x))
#define be32toh(x) bswap32((x))
#define le32toh(x) ((uint32_t)(x))

#define htobe64(x) bswap64((x))
#define htole64(x) ((uint64_t)(x))
#define be64toh(x) bswap64((x))
#define le64toh(x) ((uint64_t)(x))

#endif
