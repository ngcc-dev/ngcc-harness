/*
Copyright (c) 2026 Hang Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences

Alignment-agnostic, endian-explicit I/O helpers.
  _le  = little-endian  (signature packing, XOF output parsing)
  _be  = big-endian     (cSHAKE block counters)
  _n   = partial-width, 1..8 bytes
*/
#ifndef ENDIAN_H
#define ENDIAN_H

#include <stdint.h>

/* ===== Little-endian loads ===== */

static inline uint16_t load16_le(const unsigned char *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static inline uint32_t load24_le(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
}

static inline uint64_t load64_le(const unsigned char *p) {
    return (uint64_t)p[0]
         | ((uint64_t)p[1] <<  8)
         | ((uint64_t)p[2] << 16)
         | ((uint64_t)p[3] << 24)
         | ((uint64_t)p[4] << 32)
         | ((uint64_t)p[5] << 40)
         | ((uint64_t)p[6] << 48)
         | ((uint64_t)p[7] << 56);
}

/* ===== Little-endian stores ===== */

static inline void store16_le(unsigned char *p, uint16_t v) {
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
}

static inline void store64_le(unsigned char *p, uint64_t v) {
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >>  8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
    p[4] = (uint8_t)(v >> 32);
    p[5] = (uint8_t)(v >> 40);
    p[6] = (uint8_t)(v >> 48);
    p[7] = (uint8_t)(v >> 56);
}

/* ===== Big-endian store ===== */

static inline void store32_be(unsigned char *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >>  8);
    p[3] = (uint8_t)(v);
}

/* ===== Partial-width LE ===== */

static inline uint64_t load64_le_n(const unsigned char *p, int n) {
    uint64_t v = 0;
    int i;
    for (i = 0; i < n; i++) v |= (uint64_t)p[i] << (i * 8);
    return v;
}

static inline void store64_le_n(unsigned char *p, uint64_t v, int n) {
    int i;
    for (i = 0; i < n; i++) p[i] = (uint8_t)(v >> (i * 8));
}

#endif
