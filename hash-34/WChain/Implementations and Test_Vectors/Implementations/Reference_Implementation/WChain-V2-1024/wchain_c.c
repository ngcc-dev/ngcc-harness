#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "wchain_c.h"

#if !defined(WCHAIN_DISABLE_SIMD)
#include <immintrin.h>
#endif

#if defined(__GNUC__)
#define WCHAIN_UNUSED __attribute__((unused))
#else
#define WCHAIN_UNUSED
#endif

WCHAIN_UNUSED static const uint16_t PI16[24] = {
    0x243f, 0x6a88, 0x85a3, 0x08d3, 0x1319, 0x8a2e,
    0x0370, 0x7344, 0xa409, 0x3822, 0x299f, 0x31d0,
    0x082e, 0xfa98, 0xec4e, 0x6c89, 0x4528, 0x21e6,
    0x38d0, 0x1377, 0xbe54, 0x66cf, 0x34e9, 0x0c6c,
};

#define WCHAIN_V1_ROUNDS 18
#define WCHAIN_V2_ROUNDS 24

#define PI16_R(c, n) ((uint64_t)(uint16_t)(c) << (n))

static const uint64_t PI16_ROT[24][18] = {
    {PI16_R(0x243f, 0), PI16_R(0x243f, 1), PI16_R(0x243f, 2), PI16_R(0x243f, 3), PI16_R(0x243f, 4), PI16_R(0x243f, 5), PI16_R(0x243f, 6), PI16_R(0x243f, 7), PI16_R(0x243f, 8), PI16_R(0x243f, 9), PI16_R(0x243f, 10), PI16_R(0x243f, 11), PI16_R(0x243f, 12), PI16_R(0x243f, 13), PI16_R(0x243f, 14), PI16_R(0x243f, 15), PI16_R(0x243f, 16), PI16_R(0x243f, 17)},
    {PI16_R(0x6a88, 0), PI16_R(0x6a88, 1), PI16_R(0x6a88, 2), PI16_R(0x6a88, 3), PI16_R(0x6a88, 4), PI16_R(0x6a88, 5), PI16_R(0x6a88, 6), PI16_R(0x6a88, 7), PI16_R(0x6a88, 8), PI16_R(0x6a88, 9), PI16_R(0x6a88, 10), PI16_R(0x6a88, 11), PI16_R(0x6a88, 12), PI16_R(0x6a88, 13), PI16_R(0x6a88, 14), PI16_R(0x6a88, 15), PI16_R(0x6a88, 16), PI16_R(0x6a88, 17)},
    {PI16_R(0x85a3, 0), PI16_R(0x85a3, 1), PI16_R(0x85a3, 2), PI16_R(0x85a3, 3), PI16_R(0x85a3, 4), PI16_R(0x85a3, 5), PI16_R(0x85a3, 6), PI16_R(0x85a3, 7), PI16_R(0x85a3, 8), PI16_R(0x85a3, 9), PI16_R(0x85a3, 10), PI16_R(0x85a3, 11), PI16_R(0x85a3, 12), PI16_R(0x85a3, 13), PI16_R(0x85a3, 14), PI16_R(0x85a3, 15), PI16_R(0x85a3, 16), PI16_R(0x85a3, 17)},
    {PI16_R(0x08d3, 0), PI16_R(0x08d3, 1), PI16_R(0x08d3, 2), PI16_R(0x08d3, 3), PI16_R(0x08d3, 4), PI16_R(0x08d3, 5), PI16_R(0x08d3, 6), PI16_R(0x08d3, 7), PI16_R(0x08d3, 8), PI16_R(0x08d3, 9), PI16_R(0x08d3, 10), PI16_R(0x08d3, 11), PI16_R(0x08d3, 12), PI16_R(0x08d3, 13), PI16_R(0x08d3, 14), PI16_R(0x08d3, 15), PI16_R(0x08d3, 16), PI16_R(0x08d3, 17)},
    {PI16_R(0x1319, 0), PI16_R(0x1319, 1), PI16_R(0x1319, 2), PI16_R(0x1319, 3), PI16_R(0x1319, 4), PI16_R(0x1319, 5), PI16_R(0x1319, 6), PI16_R(0x1319, 7), PI16_R(0x1319, 8), PI16_R(0x1319, 9), PI16_R(0x1319, 10), PI16_R(0x1319, 11), PI16_R(0x1319, 12), PI16_R(0x1319, 13), PI16_R(0x1319, 14), PI16_R(0x1319, 15), PI16_R(0x1319, 16), PI16_R(0x1319, 17)},
    {PI16_R(0x8a2e, 0), PI16_R(0x8a2e, 1), PI16_R(0x8a2e, 2), PI16_R(0x8a2e, 3), PI16_R(0x8a2e, 4), PI16_R(0x8a2e, 5), PI16_R(0x8a2e, 6), PI16_R(0x8a2e, 7), PI16_R(0x8a2e, 8), PI16_R(0x8a2e, 9), PI16_R(0x8a2e, 10), PI16_R(0x8a2e, 11), PI16_R(0x8a2e, 12), PI16_R(0x8a2e, 13), PI16_R(0x8a2e, 14), PI16_R(0x8a2e, 15), PI16_R(0x8a2e, 16), PI16_R(0x8a2e, 17)},
    {PI16_R(0x0370, 0), PI16_R(0x0370, 1), PI16_R(0x0370, 2), PI16_R(0x0370, 3), PI16_R(0x0370, 4), PI16_R(0x0370, 5), PI16_R(0x0370, 6), PI16_R(0x0370, 7), PI16_R(0x0370, 8), PI16_R(0x0370, 9), PI16_R(0x0370, 10), PI16_R(0x0370, 11), PI16_R(0x0370, 12), PI16_R(0x0370, 13), PI16_R(0x0370, 14), PI16_R(0x0370, 15), PI16_R(0x0370, 16), PI16_R(0x0370, 17)},
    {PI16_R(0x7344, 0), PI16_R(0x7344, 1), PI16_R(0x7344, 2), PI16_R(0x7344, 3), PI16_R(0x7344, 4), PI16_R(0x7344, 5), PI16_R(0x7344, 6), PI16_R(0x7344, 7), PI16_R(0x7344, 8), PI16_R(0x7344, 9), PI16_R(0x7344, 10), PI16_R(0x7344, 11), PI16_R(0x7344, 12), PI16_R(0x7344, 13), PI16_R(0x7344, 14), PI16_R(0x7344, 15), PI16_R(0x7344, 16), PI16_R(0x7344, 17)},
    {PI16_R(0xa409, 0), PI16_R(0xa409, 1), PI16_R(0xa409, 2), PI16_R(0xa409, 3), PI16_R(0xa409, 4), PI16_R(0xa409, 5), PI16_R(0xa409, 6), PI16_R(0xa409, 7), PI16_R(0xa409, 8), PI16_R(0xa409, 9), PI16_R(0xa409, 10), PI16_R(0xa409, 11), PI16_R(0xa409, 12), PI16_R(0xa409, 13), PI16_R(0xa409, 14), PI16_R(0xa409, 15), PI16_R(0xa409, 16), PI16_R(0xa409, 17)},
    {PI16_R(0x3822, 0), PI16_R(0x3822, 1), PI16_R(0x3822, 2), PI16_R(0x3822, 3), PI16_R(0x3822, 4), PI16_R(0x3822, 5), PI16_R(0x3822, 6), PI16_R(0x3822, 7), PI16_R(0x3822, 8), PI16_R(0x3822, 9), PI16_R(0x3822, 10), PI16_R(0x3822, 11), PI16_R(0x3822, 12), PI16_R(0x3822, 13), PI16_R(0x3822, 14), PI16_R(0x3822, 15), PI16_R(0x3822, 16), PI16_R(0x3822, 17)},
    {PI16_R(0x299f, 0), PI16_R(0x299f, 1), PI16_R(0x299f, 2), PI16_R(0x299f, 3), PI16_R(0x299f, 4), PI16_R(0x299f, 5), PI16_R(0x299f, 6), PI16_R(0x299f, 7), PI16_R(0x299f, 8), PI16_R(0x299f, 9), PI16_R(0x299f, 10), PI16_R(0x299f, 11), PI16_R(0x299f, 12), PI16_R(0x299f, 13), PI16_R(0x299f, 14), PI16_R(0x299f, 15), PI16_R(0x299f, 16), PI16_R(0x299f, 17)},
    {PI16_R(0x31d0, 0), PI16_R(0x31d0, 1), PI16_R(0x31d0, 2), PI16_R(0x31d0, 3), PI16_R(0x31d0, 4), PI16_R(0x31d0, 5), PI16_R(0x31d0, 6), PI16_R(0x31d0, 7), PI16_R(0x31d0, 8), PI16_R(0x31d0, 9), PI16_R(0x31d0, 10), PI16_R(0x31d0, 11), PI16_R(0x31d0, 12), PI16_R(0x31d0, 13), PI16_R(0x31d0, 14), PI16_R(0x31d0, 15), PI16_R(0x31d0, 16), PI16_R(0x31d0, 17)},
    {PI16_R(0x082e, 0), PI16_R(0x082e, 1), PI16_R(0x082e, 2), PI16_R(0x082e, 3), PI16_R(0x082e, 4), PI16_R(0x082e, 5), PI16_R(0x082e, 6), PI16_R(0x082e, 7), PI16_R(0x082e, 8), PI16_R(0x082e, 9), PI16_R(0x082e, 10), PI16_R(0x082e, 11), PI16_R(0x082e, 12), PI16_R(0x082e, 13), PI16_R(0x082e, 14), PI16_R(0x082e, 15), PI16_R(0x082e, 16), PI16_R(0x082e, 17)},
    {PI16_R(0xfa98, 0), PI16_R(0xfa98, 1), PI16_R(0xfa98, 2), PI16_R(0xfa98, 3), PI16_R(0xfa98, 4), PI16_R(0xfa98, 5), PI16_R(0xfa98, 6), PI16_R(0xfa98, 7), PI16_R(0xfa98, 8), PI16_R(0xfa98, 9), PI16_R(0xfa98, 10), PI16_R(0xfa98, 11), PI16_R(0xfa98, 12), PI16_R(0xfa98, 13), PI16_R(0xfa98, 14), PI16_R(0xfa98, 15), PI16_R(0xfa98, 16), PI16_R(0xfa98, 17)},
    {PI16_R(0xec4e, 0), PI16_R(0xec4e, 1), PI16_R(0xec4e, 2), PI16_R(0xec4e, 3), PI16_R(0xec4e, 4), PI16_R(0xec4e, 5), PI16_R(0xec4e, 6), PI16_R(0xec4e, 7), PI16_R(0xec4e, 8), PI16_R(0xec4e, 9), PI16_R(0xec4e, 10), PI16_R(0xec4e, 11), PI16_R(0xec4e, 12), PI16_R(0xec4e, 13), PI16_R(0xec4e, 14), PI16_R(0xec4e, 15), PI16_R(0xec4e, 16), PI16_R(0xec4e, 17)},
    {PI16_R(0x6c89, 0), PI16_R(0x6c89, 1), PI16_R(0x6c89, 2), PI16_R(0x6c89, 3), PI16_R(0x6c89, 4), PI16_R(0x6c89, 5), PI16_R(0x6c89, 6), PI16_R(0x6c89, 7), PI16_R(0x6c89, 8), PI16_R(0x6c89, 9), PI16_R(0x6c89, 10), PI16_R(0x6c89, 11), PI16_R(0x6c89, 12), PI16_R(0x6c89, 13), PI16_R(0x6c89, 14), PI16_R(0x6c89, 15), PI16_R(0x6c89, 16), PI16_R(0x6c89, 17)},
    {PI16_R(0x4528, 0), PI16_R(0x4528, 1), PI16_R(0x4528, 2), PI16_R(0x4528, 3), PI16_R(0x4528, 4), PI16_R(0x4528, 5), PI16_R(0x4528, 6), PI16_R(0x4528, 7), PI16_R(0x4528, 8), PI16_R(0x4528, 9), PI16_R(0x4528, 10), PI16_R(0x4528, 11), PI16_R(0x4528, 12), PI16_R(0x4528, 13), PI16_R(0x4528, 14), PI16_R(0x4528, 15), PI16_R(0x4528, 16), PI16_R(0x4528, 17)},
    {PI16_R(0x21e6, 0), PI16_R(0x21e6, 1), PI16_R(0x21e6, 2), PI16_R(0x21e6, 3), PI16_R(0x21e6, 4), PI16_R(0x21e6, 5), PI16_R(0x21e6, 6), PI16_R(0x21e6, 7), PI16_R(0x21e6, 8), PI16_R(0x21e6, 9), PI16_R(0x21e6, 10), PI16_R(0x21e6, 11), PI16_R(0x21e6, 12), PI16_R(0x21e6, 13), PI16_R(0x21e6, 14), PI16_R(0x21e6, 15), PI16_R(0x21e6, 16), PI16_R(0x21e6, 17)},
    {PI16_R(0x38d0, 0), PI16_R(0x38d0, 1), PI16_R(0x38d0, 2), PI16_R(0x38d0, 3), PI16_R(0x38d0, 4), PI16_R(0x38d0, 5), PI16_R(0x38d0, 6), PI16_R(0x38d0, 7), PI16_R(0x38d0, 8), PI16_R(0x38d0, 9), PI16_R(0x38d0, 10), PI16_R(0x38d0, 11), PI16_R(0x38d0, 12), PI16_R(0x38d0, 13), PI16_R(0x38d0, 14), PI16_R(0x38d0, 15), PI16_R(0x38d0, 16), PI16_R(0x38d0, 17)},
    {PI16_R(0x1377, 0), PI16_R(0x1377, 1), PI16_R(0x1377, 2), PI16_R(0x1377, 3), PI16_R(0x1377, 4), PI16_R(0x1377, 5), PI16_R(0x1377, 6), PI16_R(0x1377, 7), PI16_R(0x1377, 8), PI16_R(0x1377, 9), PI16_R(0x1377, 10), PI16_R(0x1377, 11), PI16_R(0x1377, 12), PI16_R(0x1377, 13), PI16_R(0x1377, 14), PI16_R(0x1377, 15), PI16_R(0x1377, 16), PI16_R(0x1377, 17)},
    {PI16_R(0xbe54, 0), PI16_R(0xbe54, 1), PI16_R(0xbe54, 2), PI16_R(0xbe54, 3), PI16_R(0xbe54, 4), PI16_R(0xbe54, 5), PI16_R(0xbe54, 6), PI16_R(0xbe54, 7), PI16_R(0xbe54, 8), PI16_R(0xbe54, 9), PI16_R(0xbe54, 10), PI16_R(0xbe54, 11), PI16_R(0xbe54, 12), PI16_R(0xbe54, 13), PI16_R(0xbe54, 14), PI16_R(0xbe54, 15), PI16_R(0xbe54, 16), PI16_R(0xbe54, 17)},
    {PI16_R(0x66cf, 0), PI16_R(0x66cf, 1), PI16_R(0x66cf, 2), PI16_R(0x66cf, 3), PI16_R(0x66cf, 4), PI16_R(0x66cf, 5), PI16_R(0x66cf, 6), PI16_R(0x66cf, 7), PI16_R(0x66cf, 8), PI16_R(0x66cf, 9), PI16_R(0x66cf, 10), PI16_R(0x66cf, 11), PI16_R(0x66cf, 12), PI16_R(0x66cf, 13), PI16_R(0x66cf, 14), PI16_R(0x66cf, 15), PI16_R(0x66cf, 16), PI16_R(0x66cf, 17)},
    {PI16_R(0x34e9, 0), PI16_R(0x34e9, 1), PI16_R(0x34e9, 2), PI16_R(0x34e9, 3), PI16_R(0x34e9, 4), PI16_R(0x34e9, 5), PI16_R(0x34e9, 6), PI16_R(0x34e9, 7), PI16_R(0x34e9, 8), PI16_R(0x34e9, 9), PI16_R(0x34e9, 10), PI16_R(0x34e9, 11), PI16_R(0x34e9, 12), PI16_R(0x34e9, 13), PI16_R(0x34e9, 14), PI16_R(0x34e9, 15), PI16_R(0x34e9, 16), PI16_R(0x34e9, 17)},
    {PI16_R(0x0c6c, 0), PI16_R(0x0c6c, 1), PI16_R(0x0c6c, 2), PI16_R(0x0c6c, 3), PI16_R(0x0c6c, 4), PI16_R(0x0c6c, 5), PI16_R(0x0c6c, 6), PI16_R(0x0c6c, 7), PI16_R(0x0c6c, 8), PI16_R(0x0c6c, 9), PI16_R(0x0c6c, 10), PI16_R(0x0c6c, 11), PI16_R(0x0c6c, 12), PI16_R(0x0c6c, 13), PI16_R(0x0c6c, 14), PI16_R(0x0c6c, 15), PI16_R(0x0c6c, 16), PI16_R(0x0c6c, 17)},
};

#undef PI16_R

WCHAIN_UNUSED static const int P9[9] = {0, 7, 5, 3, 1, 8, 6, 4, 2};
WCHAIN_UNUSED static const int P9_INV[9] = {0, 4, 8, 3, 7, 2, 6, 1, 5};
static const int ME_ROT[9] = {0, 1, 10, 20, 25, 32, 41, 46, 50};

static const int L_IDX[9][5] = {
    {0, 1, 2, 3, 7}, {1, 2, 3, 4, 8}, {2, 3, 4, 5, 0},
    {3, 4, 5, 6, 1}, {4, 5, 6, 7, 2}, {5, 6, 7, 8, 3},
    {6, 7, 8, 0, 4}, {7, 8, 0, 1, 5}, {8, 0, 1, 2, 6},
};

static const int L_ROT[9][5] = {
    {5, 22, 3, 28, 27}, {39, 59, 45, 3, 9}, {54, 6, 20, 8, 48},
    {0, 57, 55, 46, 28}, {61, 38, 0, 44, 52}, {29, 10, 37, 19, 7},
    {56, 5, 50, 27, 13}, {40, 10, 11, 54, 14}, {3, 38, 36, 17, 13},
};

static const int R_ROT[9][5] = {
    {16, 33, 14, 39, 38}, {60, 16, 2, 24, 30}, {9, 25, 39, 27, 3},
    {21, 14, 12, 3, 49}, {40, 17, 43, 23, 31}, {34, 15, 42, 24, 12},
    {59, 8, 53, 30, 16}, {50, 20, 21, 0, 24}, {8, 43, 41, 22, 18},
};

static inline uint64_t rotl64(uint64_t x, int n) {
    n &= 63;
    return n == 0 ? x : ((x << n) | (x >> (64 - n)));
}

static uint64_t load64_be(const uint8_t *p) {
    uint64_t x;
    memcpy(&x, p, sizeof(x));
    return __builtin_bswap64(x);
}

static uint64_t load64_raw(const uint8_t *p) {
    uint64_t x;
    memcpy(&x, p, sizeof(x));
    return x;
}

#if defined(__GNUC__)
__attribute__((unused))
#endif
static void store64_raw(uint8_t *p, uint64_t x) {
    memcpy(p, &x, sizeof(x));
}

static void store64_be(uint8_t *p, uint64_t x) {
    x = __builtin_bswap64(x);
    memcpy(p, &x, sizeof(x));
}

static void sbox9_rot_u64(uint64_t x[9]) {
    uint64_t a[9];
    for (int i = 0; i < 9; ++i) a[i] = rotl64(x[i], ME_ROT[i]);
    for (int i = 0; i < 9; ++i) x[i] = a[i] ^ (a[(i + 1) % 9] | ~a[(i + 2) % 9]);
}

WCHAIN_UNUSED static void sbox9_iota_u64(uint64_t y[9], const uint64_t x[9], uint64_t c, int rot_base) {
    uint64_t a[9];
    for (int i = 0; i < 9; ++i) a[i] = x[i] ^ rotl64(c, rot_base + i);
    for (int i = 0; i < 9; ++i) y[i] = a[i] ^ (a[(i + 1) % 9] | ~a[(i + 2) % 9]);
}

static inline void sbox9_inplace_u64(uint64_t x[9]) {
    const uint64_t a0 = x[0], a1 = x[1], a2 = x[2], a3 = x[3], a4 = x[4];
    const uint64_t a5 = x[5], a6 = x[6], a7 = x[7], a8 = x[8];
    x[0] = a0 ^ (a1 | ~a2);
    x[1] = a1 ^ (a2 | ~a3);
    x[2] = a2 ^ (a3 | ~a4);
    x[3] = a3 ^ (a4 | ~a5);
    x[4] = a4 ^ (a5 | ~a6);
    x[5] = a5 ^ (a6 | ~a7);
    x[6] = a6 ^ (a7 | ~a8);
    x[7] = a7 ^ (a8 | ~a0);
    x[8] = a8 ^ (a0 | ~a1);
}

static inline void pi_mix_v1_u64(uint64_t x[9]) {
    const uint64_t a0 = x[0], a1 = x[1], a2 = x[2], a3 = x[3], a4 = x[4];
    const uint64_t a5 = x[5], a6 = x[6], a7 = x[7], a8 = x[8];
    x[0] = rotl64(a0, 5)  ^ rotl64(a4, 22) ^ rotl64(a8, 3)  ^ rotl64(a3, 28) ^ rotl64(a1, 27);
    x[1] = rotl64(a4, 39) ^ rotl64(a8, 59) ^ rotl64(a3, 45) ^ rotl64(a7, 3)  ^ rotl64(a5, 9);
    x[2] = rotl64(a8, 54) ^ rotl64(a3, 6)  ^ rotl64(a7, 20) ^ rotl64(a2, 8)  ^ rotl64(a0, 48);
    x[3] = a3             ^ rotl64(a7, 57) ^ rotl64(a2, 55) ^ rotl64(a6, 46) ^ rotl64(a4, 28);
    x[4] = rotl64(a7, 61) ^ rotl64(a2, 38) ^ a6             ^ rotl64(a1, 44) ^ rotl64(a8, 52);
    x[5] = rotl64(a2, 29) ^ rotl64(a6, 10) ^ rotl64(a1, 37) ^ rotl64(a5, 19) ^ rotl64(a3, 7);
    x[6] = rotl64(a6, 56) ^ rotl64(a1, 5)  ^ rotl64(a5, 50) ^ rotl64(a0, 27) ^ rotl64(a7, 13);
    x[7] = rotl64(a1, 40) ^ rotl64(a5, 10) ^ rotl64(a0, 11) ^ rotl64(a4, 54) ^ rotl64(a2, 14);
    x[8] = rotl64(a5, 3)  ^ rotl64(a0, 38) ^ rotl64(a4, 36) ^ rotl64(a8, 17) ^ rotl64(a6, 13);
}

static inline void pi_cross_mix_v2_u64(uint64_t x[18]) {
    const uint64_t l0 = x[0], l1 = x[1], l2 = x[2], l3 = x[3], l4 = x[4];
    const uint64_t l5 = x[5], l6 = x[6], l7 = x[7], l8 = x[8];
    const uint64_t r0 = x[9], r1 = x[10], r2 = x[11], r3 = x[12], r4 = x[13];
    const uint64_t r5 = x[14], r6 = x[15], r7 = x[16], r8 = x[17];

    const uint64_t cl0 = r0, cl1 = r3, cl2 = l8, cl3 = r4, cl4 = r7;
    const uint64_t cl5 = l2, cl6 = r6, cl7 = l1, cl8 = l5;
    const uint64_t cr0 = l0, cr1 = l3, cr2 = r8, cr3 = l4, cr4 = l7;
    const uint64_t cr5 = r2, cr6 = l6, cr7 = r1, cr8 = r5;

    x[0]  = rotl64(cl0, 5)  ^ rotl64(cl1, 22) ^ rotl64(cl2, 3)  ^ rotl64(cl3, 28) ^ rotl64(cl7, 27);
    x[1]  = rotl64(cl1, 39) ^ rotl64(cl2, 59) ^ rotl64(cl3, 45) ^ rotl64(cl4, 3)  ^ rotl64(cl8, 9);
    x[2]  = rotl64(cl2, 54) ^ rotl64(cl3, 6)  ^ rotl64(cl4, 20) ^ rotl64(cl5, 8)  ^ rotl64(cl0, 48);
    x[3]  = cl3             ^ rotl64(cl4, 57) ^ rotl64(cl5, 55) ^ rotl64(cl6, 46) ^ rotl64(cl1, 28);
    x[4]  = rotl64(cl4, 61) ^ rotl64(cl5, 38) ^ cl6             ^ rotl64(cl7, 44) ^ rotl64(cl2, 52);
    x[5]  = rotl64(cl5, 29) ^ rotl64(cl6, 10) ^ rotl64(cl7, 37) ^ rotl64(cl8, 19) ^ rotl64(cl3, 7);
    x[6]  = rotl64(cl6, 56) ^ rotl64(cl7, 5)  ^ rotl64(cl8, 50) ^ rotl64(cl0, 27) ^ rotl64(cl4, 13);
    x[7]  = rotl64(cl7, 40) ^ rotl64(cl8, 10) ^ rotl64(cl0, 11) ^ rotl64(cl1, 54) ^ rotl64(cl5, 14);
    x[8]  = rotl64(cl8, 3)  ^ rotl64(cl0, 38) ^ rotl64(cl1, 36) ^ rotl64(cl2, 17) ^ rotl64(cl6, 13);
    x[9]  = rotl64(cr0, 16) ^ rotl64(cr1, 33) ^ rotl64(cr2, 14) ^ rotl64(cr3, 39) ^ rotl64(cr7, 38);
    x[10] = rotl64(cr1, 60) ^ rotl64(cr2, 16) ^ rotl64(cr3, 2)  ^ rotl64(cr4, 24) ^ rotl64(cr8, 30);
    x[11] = rotl64(cr2, 9)  ^ rotl64(cr3, 25) ^ rotl64(cr4, 39) ^ rotl64(cr5, 27) ^ rotl64(cr0, 3);
    x[12] = rotl64(cr3, 21) ^ rotl64(cr4, 14) ^ rotl64(cr5, 12) ^ rotl64(cr6, 3)  ^ rotl64(cr1, 49);
    x[13] = rotl64(cr4, 40) ^ rotl64(cr5, 17) ^ rotl64(cr6, 43) ^ rotl64(cr7, 23) ^ rotl64(cr2, 31);
    x[14] = rotl64(cr5, 34) ^ rotl64(cr6, 15) ^ rotl64(cr7, 42) ^ rotl64(cr8, 24) ^ rotl64(cr3, 12);
    x[15] = rotl64(cr6, 59) ^ rotl64(cr7, 8)  ^ rotl64(cr8, 53) ^ rotl64(cr0, 30) ^ rotl64(cr4, 16);
    x[16] = rotl64(cr7, 50) ^ rotl64(cr8, 20) ^ rotl64(cr0, 21) ^ cr1             ^ rotl64(cr5, 24);
    x[17] = rotl64(cr8, 8)  ^ rotl64(cr0, 43) ^ rotl64(cr1, 41) ^ rotl64(cr2, 22) ^ rotl64(cr6, 18);
}

WCHAIN_UNUSED static void mix_left_u64(uint64_t y[9], const uint64_t x[9]) {
    for (int out = 0; out < 9; ++out) {
        uint64_t v = 0;
        for (int k = 0; k < 5; ++k) v ^= rotl64(x[L_IDX[out][k]], L_ROT[out][k]);
        y[out] = v;
    }
}

WCHAIN_UNUSED static void mix_right_u64(uint64_t y[9], const uint64_t x[9]) {
    for (int out = 0; out < 9; ++out) {
        uint64_t v = 0;
        for (int k = 0; k < 5; ++k) v ^= rotl64(x[L_IDX[out][k]], R_ROT[out][k]);
        y[out] = v;
    }
}

WCHAIN_UNUSED static void round_v1_u64(uint64_t x[9], int round_index) {
    const uint64_t *rc = PI16_ROT[round_index];
    x[0] ^= rc[0];
    x[1] ^= rc[1];
    x[2] ^= rc[2];
    x[3] ^= rc[3];
    x[4] ^= rc[4];
    x[5] ^= rc[5];
    x[6] ^= rc[6];
    x[7] ^= rc[7];
    x[8] ^= rc[8];
    sbox9_inplace_u64(x);
    pi_mix_v1_u64(x);
}

static inline void round_v1_inject_u64(uint64_t x[9], const uint64_t inject[9], int round_index) {
    const uint64_t *rc = PI16_ROT[round_index];
    x[0] ^= inject[0] ^ rc[0];
    x[1] ^= inject[1] ^ rc[1];
    x[2] ^= inject[2] ^ rc[2];
    x[3] ^= inject[3] ^ rc[3];
    x[4] ^= inject[4] ^ rc[4];
    x[5] ^= inject[5] ^ rc[5];
    x[6] ^= inject[6] ^ rc[6];
    x[7] ^= inject[7] ^ rc[7];
    x[8] ^= inject[8] ^ rc[8];
    sbox9_inplace_u64(x);
    pi_mix_v1_u64(x);
}

static void round_v2_u64(uint64_t x[18], int round_index) {
    const uint64_t *rc = PI16_ROT[round_index];
    for (int i = 0; i < 18; ++i) x[i] ^= rc[i];
    sbox9_inplace_u64(x);
    sbox9_inplace_u64(x + 9);
    pi_cross_mix_v2_u64(x);
}

WCHAIN_UNUSED static void mef_v1_u64(uint64_t x[9]) {
    sbox9_rot_u64(x);
}

static inline void mef_v1_xor_u64(uint64_t out[9], const uint64_t prev1[9], const uint64_t prev2[9]) {
    const uint64_t a0 = prev1[0];
    const uint64_t a1 = rotl64(prev1[1], 1);
    const uint64_t a2 = rotl64(prev1[2], 10);
    const uint64_t a3 = rotl64(prev1[3], 20);
    const uint64_t a4 = rotl64(prev1[4], 25);
    const uint64_t a5 = rotl64(prev1[5], 32);
    const uint64_t a6 = rotl64(prev1[6], 41);
    const uint64_t a7 = rotl64(prev1[7], 46);
    const uint64_t a8 = rotl64(prev1[8], 50);
    out[0] = (a0 ^ (a1 | ~a2)) ^ prev2[0];
    out[1] = (a1 ^ (a2 | ~a3)) ^ prev2[1];
    out[2] = (a2 ^ (a3 | ~a4)) ^ prev2[2];
    out[3] = (a3 ^ (a4 | ~a5)) ^ prev2[3];
    out[4] = (a4 ^ (a5 | ~a6)) ^ prev2[4];
    out[5] = (a5 ^ (a6 | ~a7)) ^ prev2[5];
    out[6] = (a6 ^ (a7 | ~a8)) ^ prev2[6];
    out[7] = (a7 ^ (a8 | ~a0)) ^ prev2[7];
    out[8] = (a8 ^ (a0 | ~a1)) ^ prev2[8];
}

static void mef_v2_u64(uint64_t x[18]) {
    for (int half = 0; half < 2; ++half) {
        uint64_t *h = x + half * 9;
        const uint64_t a0 = h[0];
        const uint64_t a1 = rotl64(h[1], 1);
        const uint64_t a2 = rotl64(h[2], 10);
        const uint64_t a3 = rotl64(h[3], 20);
        const uint64_t a4 = rotl64(h[4], 25);
        const uint64_t a5 = rotl64(h[5], 32);
        const uint64_t a6 = rotl64(h[6], 41);
        const uint64_t a7 = rotl64(h[7], 46);
        const uint64_t a8 = rotl64(h[8], 50);
        h[0] = a0 ^ (a1 | ~a2);
        h[1] = a1 ^ (a2 | ~a3);
        h[2] = a2 ^ (a3 | ~a4);
        h[3] = a3 ^ (a4 | ~a5);
        h[4] = a4 ^ (a5 | ~a6);
        h[5] = a5 ^ (a6 | ~a7);
        h[6] = a6 ^ (a7 | ~a8);
        h[7] = a7 ^ (a8 | ~a0);
        h[8] = a8 ^ (a0 | ~a1);
    }
}

static void load_state9(uint64_t s[9], const uint8_t *p) {
    for (int i = 0; i < 9; ++i) s[i] = load64_be(p + 8 * i);
}

static void load_state18(uint64_t s[18], const uint8_t *p) {
    for (int i = 0; i < 18; ++i) s[i] = load64_be(p + 8 * i);
}

static inline void wchain_v1_compress_loaded(uint64_t out[9], const uint64_t v[9], uint64_t b0[9], uint64_t b1[9]) {
    uint64_t b2[9], b_final[9], x[9];
    uint64_t *b_prev2 = b0;
    uint64_t *b_prev1 = b1;
    uint64_t *b_cur = b2;

    memcpy(x, v, sizeof(x));

    for (int j = 1; j <= WCHAIN_V1_ROUNDS; ++j) {
        const uint64_t *inj = b_prev2;
        if (j == 2) {
            inj = b_prev1;
        } else if (j > 2) {
            mef_v1_xor_u64(b_cur, b_prev1, b_prev2);
            inj = b_cur;
        }
        round_v1_inject_u64(x, inj, j - 1);
        if (j > 2) {
            uint64_t *tmp = b_prev2;
            b_prev2 = b_prev1;
            b_prev1 = b_cur;
            b_cur = tmp;
        }
    }

    mef_v1_xor_u64(b_final, b_prev1, b_prev2);
    for (int i = 0; i < 9; ++i) out[i] = x[i] ^ b_final[i] ^ v[i];
}

void wchain_v1_compress_c(uint64_t out[9], const uint64_t v[9], const uint8_t block[144]) {
    uint64_t b0[9], b1[9];
    load_state9(b0, block);
    load_state9(b1, block + 72);
    wchain_v1_compress_loaded(out, v, b0, b1);
}

static inline void wchain_v2_compress_loaded(uint64_t out[18], const uint64_t v[18], uint64_t b0[18], uint64_t b1[18]) {
    uint64_t b2[18], b_final[18], x[18];
    uint64_t *b_prev2 = b0;
    uint64_t *b_prev1 = b1;
    uint64_t *b_cur = b2;

    memcpy(x, v, sizeof(x));
    for (int j = 1; j <= WCHAIN_V2_ROUNDS; ++j) {
        const uint64_t *inj = b_prev2;
        if (j == 2) {
            inj = b_prev1;
        } else if (j > 2) {
            memcpy(b_cur, b_prev1, 18 * sizeof(uint64_t));
            mef_v2_u64(b_cur);
            for (int i = 0; i < 18; ++i) b_cur[i] ^= b_prev2[i];
            inj = b_cur;
        }
        for (int i = 0; i < 18; ++i) x[i] ^= inj[i];
        round_v2_u64(x, j - 1);
        if (j > 2) {
            uint64_t *tmp = b_prev2;
            b_prev2 = b_prev1;
            b_prev1 = b_cur;
            b_cur = tmp;
        }
    }
    memcpy(b_final, b_prev1, sizeof(b_final));
    mef_v2_u64(b_final);
    for (int i = 0; i < 18; ++i) out[i] = x[i] ^ (b_final[i] ^ b_prev2[i]) ^ v[i];
}

void wchain_v2_compress_c(uint64_t out[18], const uint64_t v[18], const uint8_t block[288]) {
    uint64_t b0[18], b1[18];
    load_state18(b0, block);
    load_state18(b1, block + 144);
    wchain_v2_compress_loaded(out, v, b0, b1);
}

static inline void wchain_v1_absorb_block_c(const uint8_t block[144], uint64_t sigma_words[18],
                                            uint64_t h_prev2[9], uint64_t h_prev1[9]) {
    uint64_t b0[9], b1[9], c[9];
    for (int i = 0; i < 9; ++i) {
        const uint64_t w0 = load64_raw(block + 8 * i);
        const uint64_t w1 = load64_raw(block + 72 + 8 * i);
        sigma_words[i] ^= w0;
        sigma_words[9 + i] ^= w1;
        b0[i] = __builtin_bswap64(w0);
        b1[i] = __builtin_bswap64(w1);
    }
    wchain_v1_compress_loaded(c, h_prev1, b0, b1);
    for (int i = 0; i < 9; ++i) {
        const uint64_t old_h1 = h_prev1[i];
        h_prev1[i] = c[i] ^ h_prev2[i];
        h_prev2[i] = old_h1;
    }
}

static inline void wchain_v2_absorb_block_c(const uint8_t block[288], uint64_t sigma_words[36],
                                            uint64_t h_prev2[18], uint64_t h_prev1[18]) {
    uint64_t b0[18], b1[18], c[18];
    for (int i = 0; i < 18; ++i) {
        const uint64_t w0 = load64_raw(block + 8 * i);
        const uint64_t w1 = load64_raw(block + 144 + 8 * i);
        sigma_words[i] ^= w0;
        sigma_words[18 + i] ^= w1;
        b0[i] = __builtin_bswap64(w0);
        b1[i] = __builtin_bswap64(w1);
    }
    wchain_v2_compress_loaded(c, h_prev1, b0, b1);
    for (int i = 0; i < 18; ++i) {
        const uint64_t old_h1 = h_prev1[i];
        h_prev1[i] = c[i] ^ h_prev2[i];
        h_prev2[i] = old_h1;
    }
}

void wchain_v1_hash_bits_c(const uint8_t *msg, unsigned long long msg_len_bits, uint8_t digest[64]) {
    uint64_t sigma_words[18] = {0};
    uint8_t last[144];
    uint64_t h_prev2[9] = {0}, h_prev1[9] = {0}, h[9];
    const size_t full_bytes = (size_t)(msg_len_bits >> 3);
    const unsigned partial_bits = (unsigned)(msg_len_bits & 7u);
    size_t off = 0;

    while (full_bytes - off >= 144) {
        wchain_v1_absorb_block_c(msg + off, sigma_words, h_prev2, h_prev1);
        off += 144;
    }

    const size_t rem = full_bytes - off;
    memset(last, 0, sizeof(last));
    if (rem != 0) memcpy(last, msg + off, rem);
    if (partial_bits != 0) {
        const uint8_t mask = (uint8_t)(0xffu << (8u - partial_bits));
        last[rem] = (uint8_t)((msg[off + rem] & mask) | (0x80u >> partial_bits));
    } else {
        last[rem] = 0x80;
    }
    wchain_v1_absorb_block_c(last, sigma_words, h_prev2, h_prev1);
    uint64_t b0[9], b1[9];
    for (int i = 0; i < 9; ++i) {
        b0[i] = __builtin_bswap64(sigma_words[i]);
        b1[i] = __builtin_bswap64(sigma_words[9 + i]);
    }
    wchain_v1_compress_loaded(h, h_prev1, b0, b1);
    for (int i = 0; i < 8; ++i) store64_be(digest + 8 * i, h[i]);
}

void wchain_v1_hash_c(const uint8_t *msg, size_t len, uint8_t digest[64]) {
    wchain_v1_hash_bits_c(msg, (unsigned long long)len * 8ull, digest);
}

void wchain_v2_hash_bits_c(const uint8_t *msg, unsigned long long msg_len_bits, uint8_t digest[128]) {
    uint64_t sigma_words[36] = {0};
    uint8_t last[288];
    uint64_t h_prev2[18] = {0}, h_prev1[18] = {0}, h[18];
    const size_t full_bytes = (size_t)(msg_len_bits >> 3);
    const unsigned partial_bits = (unsigned)(msg_len_bits & 7u);
    size_t off = 0;

    while (full_bytes - off >= 288) {
        wchain_v2_absorb_block_c(msg + off, sigma_words, h_prev2, h_prev1);
        off += 288;
    }

    const size_t rem = full_bytes - off;
    memset(last, 0, sizeof(last));
    if (rem != 0) memcpy(last, msg + off, rem);
    if (partial_bits != 0) {
        const uint8_t mask = (uint8_t)(0xffu << (8u - partial_bits));
        last[rem] = (uint8_t)((msg[off + rem] & mask) | (0x80u >> partial_bits));
    } else {
        last[rem] = 0x80;
    }
    wchain_v2_absorb_block_c(last, sigma_words, h_prev2, h_prev1);
    uint64_t b0[18], b1[18];
    for (int i = 0; i < 18; ++i) {
        b0[i] = __builtin_bswap64(sigma_words[i]);
        b1[i] = __builtin_bswap64(sigma_words[18 + i]);
    }
    wchain_v2_compress_loaded(h, h_prev1, b0, b1);
    for (int i = 0; i < 16; ++i) store64_be(digest + 8 * i, h[i]);
}

void wchain_v2_hash_c(const uint8_t *msg, size_t len, uint8_t digest[128]) {
    wchain_v2_hash_bits_c(msg, (unsigned long long)len * 8ull, digest);
}

int wchain_crypt_hash_c(int digest_len_bits, const unsigned char *msg,
                        unsigned long long msg_len_bits, unsigned char *digest) {
    if (digest == NULL || (msg == NULL && msg_len_bits != 0ull)) return -1;
    if (digest_len_bits == 512) {
        wchain_v1_hash_bits_c(msg, msg_len_bits, digest);
        return 0;
    }
    if (digest_len_bits == 1024) {
        wchain_v2_hash_bits_c(msg, msg_len_bits, digest);
        return 0;
    }
    return -1;
}

#if !defined(WCHAIN_DISABLE_SIMD)

#if defined(__GNUC__) && (defined(__x86_64__) || defined(_M_X64))
WCHAIN_UNUSED static int cpu_has_avx2(void) {
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx2") != 0;
}

WCHAIN_UNUSED static int cpu_has_avx512f(void) {
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx512f") != 0;
}
#else
WCHAIN_UNUSED static int cpu_has_avx2(void) {
    return 0;
}

WCHAIN_UNUSED static int cpu_has_avx512f(void) {
    return 0;
}
#endif

__attribute__((target("avx2")))
static inline __m256i rotl256(__m256i x, int n) {
    n &= 63;
    if (n == 0) return x;
    __m128i ln = _mm_cvtsi64_si128(n);
    __m128i rn = _mm_cvtsi64_si128(64 - n);
    return _mm256_or_si256(_mm256_sll_epi64(x, ln), _mm256_srl_epi64(x, rn));
}

#define ROTL256I(x, n) ((n) == 0 ? (x) : _mm256_or_si256(_mm256_slli_epi64((x), (n)), _mm256_srli_epi64((x), 64 - (n))))
#define XOR3_256(a, b, c) _mm256_xor_si256(_mm256_xor_si256((a), (b)), (c))
#define XOR5_256(a, b, c, d, e) _mm256_xor_si256(_mm256_xor_si256(_mm256_xor_si256((a), (b)), _mm256_xor_si256((c), (d))), (e))

__attribute__((target("avx2")))
static void sbox9_256(__m256i x[9]) {
    const __m256i all = _mm256_set1_epi64x(-1);
    const __m256i a0 = x[0], a1 = x[1], a2 = x[2], a3 = x[3], a4 = x[4];
    const __m256i a5 = x[5], a6 = x[6], a7 = x[7], a8 = x[8];
    x[0] = _mm256_xor_si256(a0, _mm256_or_si256(a1, _mm256_xor_si256(a2, all)));
    x[1] = _mm256_xor_si256(a1, _mm256_or_si256(a2, _mm256_xor_si256(a3, all)));
    x[2] = _mm256_xor_si256(a2, _mm256_or_si256(a3, _mm256_xor_si256(a4, all)));
    x[3] = _mm256_xor_si256(a3, _mm256_or_si256(a4, _mm256_xor_si256(a5, all)));
    x[4] = _mm256_xor_si256(a4, _mm256_or_si256(a5, _mm256_xor_si256(a6, all)));
    x[5] = _mm256_xor_si256(a5, _mm256_or_si256(a6, _mm256_xor_si256(a7, all)));
    x[6] = _mm256_xor_si256(a6, _mm256_or_si256(a7, _mm256_xor_si256(a8, all)));
    x[7] = _mm256_xor_si256(a7, _mm256_or_si256(a8, _mm256_xor_si256(a0, all)));
    x[8] = _mm256_xor_si256(a8, _mm256_or_si256(a0, _mm256_xor_si256(a1, all)));
}

__attribute__((target("avx2")))
static void pi_mix_v1_256(__m256i x[9]) {
    const __m256i a0 = x[0], a1 = x[1], a2 = x[2], a3 = x[3], a4 = x[4];
    const __m256i a5 = x[5], a6 = x[6], a7 = x[7], a8 = x[8];
    x[0] = XOR5_256(ROTL256I(a0, 5),  ROTL256I(a4, 22), ROTL256I(a8, 3),  ROTL256I(a3, 28), ROTL256I(a1, 27));
    x[1] = XOR5_256(ROTL256I(a4, 39), ROTL256I(a8, 59), ROTL256I(a3, 45), ROTL256I(a7, 3),  ROTL256I(a5, 9));
    x[2] = XOR5_256(ROTL256I(a8, 54), ROTL256I(a3, 6),  ROTL256I(a7, 20), ROTL256I(a2, 8),  ROTL256I(a0, 48));
    x[3] = XOR5_256(a3,              ROTL256I(a7, 57), ROTL256I(a2, 55), ROTL256I(a6, 46), ROTL256I(a4, 28));
    x[4] = XOR5_256(ROTL256I(a7, 61), ROTL256I(a2, 38), a6,               ROTL256I(a1, 44), ROTL256I(a8, 52));
    x[5] = XOR5_256(ROTL256I(a2, 29), ROTL256I(a6, 10), ROTL256I(a1, 37), ROTL256I(a5, 19), ROTL256I(a3, 7));
    x[6] = XOR5_256(ROTL256I(a6, 56), ROTL256I(a1, 5),  ROTL256I(a5, 50), ROTL256I(a0, 27), ROTL256I(a7, 13));
    x[7] = XOR5_256(ROTL256I(a1, 40), ROTL256I(a5, 10), ROTL256I(a0, 11), ROTL256I(a4, 54), ROTL256I(a2, 14));
    x[8] = XOR5_256(ROTL256I(a5, 3),  ROTL256I(a0, 38), ROTL256I(a4, 36), ROTL256I(a8, 17), ROTL256I(a6, 13));
}

__attribute__((target("avx2")))
static void pi_cross_mix_v2_256(__m256i x[18]) {
    const __m256i l0 = x[0], l1 = x[1], l2 = x[2], l3 = x[3], l4 = x[4];
    const __m256i l5 = x[5], l6 = x[6], l7 = x[7], l8 = x[8];
    const __m256i r0 = x[9], r1 = x[10], r2 = x[11], r3 = x[12], r4 = x[13];
    const __m256i r5 = x[14], r6 = x[15], r7 = x[16], r8 = x[17];

    const __m256i cl0 = r0, cl1 = r3, cl2 = l8, cl3 = r4, cl4 = r7;
    const __m256i cl5 = l2, cl6 = r6, cl7 = l1, cl8 = l5;
    const __m256i cr0 = l0, cr1 = l3, cr2 = r8, cr3 = l4, cr4 = l7;
    const __m256i cr5 = r2, cr6 = l6, cr7 = r1, cr8 = r5;

    x[0]  = XOR5_256(ROTL256I(cl0, 5),  ROTL256I(cl1, 22), ROTL256I(cl2, 3),  ROTL256I(cl3, 28), ROTL256I(cl7, 27));
    x[1]  = XOR5_256(ROTL256I(cl1, 39), ROTL256I(cl2, 59), ROTL256I(cl3, 45), ROTL256I(cl4, 3),  ROTL256I(cl8, 9));
    x[2]  = XOR5_256(ROTL256I(cl2, 54), ROTL256I(cl3, 6),  ROTL256I(cl4, 20), ROTL256I(cl5, 8),  ROTL256I(cl0, 48));
    x[3]  = XOR5_256(cl3,               ROTL256I(cl4, 57), ROTL256I(cl5, 55), ROTL256I(cl6, 46), ROTL256I(cl1, 28));
    x[4]  = XOR5_256(ROTL256I(cl4, 61), ROTL256I(cl5, 38), cl6,               ROTL256I(cl7, 44), ROTL256I(cl2, 52));
    x[5]  = XOR5_256(ROTL256I(cl5, 29), ROTL256I(cl6, 10), ROTL256I(cl7, 37), ROTL256I(cl8, 19), ROTL256I(cl3, 7));
    x[6]  = XOR5_256(ROTL256I(cl6, 56), ROTL256I(cl7, 5),  ROTL256I(cl8, 50), ROTL256I(cl0, 27), ROTL256I(cl4, 13));
    x[7]  = XOR5_256(ROTL256I(cl7, 40), ROTL256I(cl8, 10), ROTL256I(cl0, 11), ROTL256I(cl1, 54), ROTL256I(cl5, 14));
    x[8]  = XOR5_256(ROTL256I(cl8, 3),  ROTL256I(cl0, 38), ROTL256I(cl1, 36), ROTL256I(cl2, 17), ROTL256I(cl6, 13));
    x[9]  = XOR5_256(ROTL256I(cr0, 16), ROTL256I(cr1, 33), ROTL256I(cr2, 14), ROTL256I(cr3, 39), ROTL256I(cr7, 38));
    x[10] = XOR5_256(ROTL256I(cr1, 60), ROTL256I(cr2, 16), ROTL256I(cr3, 2),  ROTL256I(cr4, 24), ROTL256I(cr8, 30));
    x[11] = XOR5_256(ROTL256I(cr2, 9),  ROTL256I(cr3, 25), ROTL256I(cr4, 39), ROTL256I(cr5, 27), ROTL256I(cr0, 3));
    x[12] = XOR5_256(ROTL256I(cr3, 21), ROTL256I(cr4, 14), ROTL256I(cr5, 12), ROTL256I(cr6, 3),  ROTL256I(cr1, 49));
    x[13] = XOR5_256(ROTL256I(cr4, 40), ROTL256I(cr5, 17), ROTL256I(cr6, 43), ROTL256I(cr7, 23), ROTL256I(cr2, 31));
    x[14] = XOR5_256(ROTL256I(cr5, 34), ROTL256I(cr6, 15), ROTL256I(cr7, 42), ROTL256I(cr8, 24), ROTL256I(cr3, 12));
    x[15] = XOR5_256(ROTL256I(cr6, 59), ROTL256I(cr7, 8),  ROTL256I(cr8, 53), ROTL256I(cr0, 30), ROTL256I(cr4, 16));
    x[16] = XOR5_256(ROTL256I(cr7, 50), ROTL256I(cr8, 20), ROTL256I(cr0, 21), cr1,               ROTL256I(cr5, 24));
    x[17] = XOR5_256(ROTL256I(cr8, 8),  ROTL256I(cr0, 43), ROTL256I(cr1, 41), ROTL256I(cr2, 22), ROTL256I(cr6, 18));
}

__attribute__((target("avx2")))
WCHAIN_UNUSED static void pi9_256(__m256i x[9]) {
    __m256i a[9];
    memcpy(a, x, sizeof(a));
    for (int i = 0; i < 9; ++i) x[P9[i]] = a[i];
}

__attribute__((target("avx2")))
WCHAIN_UNUSED static void sbox9_rot_256(__m256i x[9]) {
    const __m256i all = _mm256_set1_epi64x(-1);
    __m256i a[9];
    for (int i = 0; i < 9; ++i) a[i] = rotl256(x[i], ME_ROT[i]);
    for (int i = 0; i < 9; ++i) {
        __m256i nb = _mm256_xor_si256(a[(i + 2) % 9], all);
        x[i] = _mm256_xor_si256(a[i], _mm256_or_si256(a[(i + 1) % 9], nb));
    }
}

__attribute__((target("avx2")))
WCHAIN_UNUSED static void sbox9_iota_256(__m256i y[9], const __m256i x[9], int r, int rot_base) {
    const __m256i all = _mm256_set1_epi64x(-1);
    __m256i a[9];
    for (int i = 0; i < 9; ++i) {
        a[i] = _mm256_xor_si256(x[i], _mm256_set1_epi64x((int64_t)rotl64(PI16[r], rot_base + i)));
    }
    for (int i = 0; i < 9; ++i) {
        __m256i nb = _mm256_xor_si256(a[(i + 2) % 9], all);
        y[i] = _mm256_xor_si256(a[i], _mm256_or_si256(a[(i + 1) % 9], nb));
    }
}

__attribute__((target("avx2")))
WCHAIN_UNUSED static void mix_256(__m256i y[9], const __m256i x[9], const int rot[9][5]) {
    for (int out = 0; out < 9; ++out) {
        __m256i v = _mm256_setzero_si256();
        for (int k = 0; k < 5; ++k) v = _mm256_xor_si256(v, rotl256(x[L_IDX[out][k]], rot[out][k]));
        y[out] = v;
    }
}

__attribute__((target("avx2")))
WCHAIN_UNUSED static void round_v1_256(__m256i x[9], int r) {
    const uint64_t *rc = PI16_ROT[r];
    x[0] = _mm256_xor_si256(x[0], _mm256_set1_epi64x((int64_t)rc[0]));
    x[1] = _mm256_xor_si256(x[1], _mm256_set1_epi64x((int64_t)rc[1]));
    x[2] = _mm256_xor_si256(x[2], _mm256_set1_epi64x((int64_t)rc[2]));
    x[3] = _mm256_xor_si256(x[3], _mm256_set1_epi64x((int64_t)rc[3]));
    x[4] = _mm256_xor_si256(x[4], _mm256_set1_epi64x((int64_t)rc[4]));
    x[5] = _mm256_xor_si256(x[5], _mm256_set1_epi64x((int64_t)rc[5]));
    x[6] = _mm256_xor_si256(x[6], _mm256_set1_epi64x((int64_t)rc[6]));
    x[7] = _mm256_xor_si256(x[7], _mm256_set1_epi64x((int64_t)rc[7]));
    x[8] = _mm256_xor_si256(x[8], _mm256_set1_epi64x((int64_t)rc[8]));
    sbox9_256(x);
    pi_mix_v1_256(x);
}

__attribute__((target("avx2")))
WCHAIN_UNUSED static void round_v1_inject_256(__m256i x[9], const __m256i inject[9], int r) {
    const uint64_t *rc = PI16_ROT[r];
    x[0] = XOR3_256(x[0], inject[0], _mm256_set1_epi64x((int64_t)rc[0]));
    x[1] = XOR3_256(x[1], inject[1], _mm256_set1_epi64x((int64_t)rc[1]));
    x[2] = XOR3_256(x[2], inject[2], _mm256_set1_epi64x((int64_t)rc[2]));
    x[3] = XOR3_256(x[3], inject[3], _mm256_set1_epi64x((int64_t)rc[3]));
    x[4] = XOR3_256(x[4], inject[4], _mm256_set1_epi64x((int64_t)rc[4]));
    x[5] = XOR3_256(x[5], inject[5], _mm256_set1_epi64x((int64_t)rc[5]));
    x[6] = XOR3_256(x[6], inject[6], _mm256_set1_epi64x((int64_t)rc[6]));
    x[7] = XOR3_256(x[7], inject[7], _mm256_set1_epi64x((int64_t)rc[7]));
    x[8] = XOR3_256(x[8], inject[8], _mm256_set1_epi64x((int64_t)rc[8]));
    sbox9_256(x);
    pi_mix_v1_256(x);
}

__attribute__((target("avx2")))
static void round_v2_256(__m256i x[18], int r) {
    const uint64_t c = PI16[r];
    for (int i = 0; i < 18; ++i) x[i] = _mm256_xor_si256(x[i], _mm256_set1_epi64x((int64_t)rotl64(c, i)));
    sbox9_256(x);
    sbox9_256(x + 9);
    pi_cross_mix_v2_256(x);
}

__attribute__((target("avx2")))
static void mef_half_256(__m256i x[9]) {
    x[1] = ROTL256I(x[1], 1);
    x[2] = ROTL256I(x[2], 10);
    x[3] = ROTL256I(x[3], 20);
    x[4] = ROTL256I(x[4], 25);
    x[5] = ROTL256I(x[5], 32);
    x[6] = ROTL256I(x[6], 41);
    x[7] = ROTL256I(x[7], 46);
    x[8] = ROTL256I(x[8], 50);
    sbox9_256(x);
}

__attribute__((target("avx2")))
WCHAIN_UNUSED static void mef_v1_256(__m256i x[9]) {
    mef_half_256(x);
}

__attribute__((target("avx2")))
WCHAIN_UNUSED static inline void mef_v1_xor_256(__m256i out[9], const __m256i prev1[9], const __m256i prev2[9]) {
    const __m256i all = _mm256_set1_epi64x(-1);
    const __m256i a0 = prev1[0];
    const __m256i a1 = ROTL256I(prev1[1], 1);
    const __m256i a2 = ROTL256I(prev1[2], 10);
    const __m256i a3 = ROTL256I(prev1[3], 20);
    const __m256i a4 = ROTL256I(prev1[4], 25);
    const __m256i a5 = ROTL256I(prev1[5], 32);
    const __m256i a6 = ROTL256I(prev1[6], 41);
    const __m256i a7 = ROTL256I(prev1[7], 46);
    const __m256i a8 = ROTL256I(prev1[8], 50);
    out[0] = XOR3_256(a0, _mm256_or_si256(a1, _mm256_xor_si256(a2, all)), prev2[0]);
    out[1] = XOR3_256(a1, _mm256_or_si256(a2, _mm256_xor_si256(a3, all)), prev2[1]);
    out[2] = XOR3_256(a2, _mm256_or_si256(a3, _mm256_xor_si256(a4, all)), prev2[2]);
    out[3] = XOR3_256(a3, _mm256_or_si256(a4, _mm256_xor_si256(a5, all)), prev2[3]);
    out[4] = XOR3_256(a4, _mm256_or_si256(a5, _mm256_xor_si256(a6, all)), prev2[4]);
    out[5] = XOR3_256(a5, _mm256_or_si256(a6, _mm256_xor_si256(a7, all)), prev2[5]);
    out[6] = XOR3_256(a6, _mm256_or_si256(a7, _mm256_xor_si256(a8, all)), prev2[6]);
    out[7] = XOR3_256(a7, _mm256_or_si256(a8, _mm256_xor_si256(a0, all)), prev2[7]);
    out[8] = XOR3_256(a8, _mm256_or_si256(a0, _mm256_xor_si256(a1, all)), prev2[8]);
}

__attribute__((target("avx2")))
static void mef_v2_256(__m256i x[18]) {
    mef_half_256(x);
    mef_half_256(x + 9);
}

__attribute__((target("avx2")))
void wchain_v1_compress4_avx2(uint64_t out[4][9], uint64_t v[4][9], uint8_t blocks[4][144]) {
    __m256i b[WCHAIN_V1_ROUNDS + 1][9], x[9], vv[9];
    for (int i = 0; i < 9; ++i) {
        vv[i] = _mm256_set_epi64x((int64_t)v[3][i], (int64_t)v[2][i], (int64_t)v[1][i], (int64_t)v[0][i]);
        b[0][i] = _mm256_set_epi64x((int64_t)load64_be(blocks[3] + 8 * i), (int64_t)load64_be(blocks[2] + 8 * i), (int64_t)load64_be(blocks[1] + 8 * i), (int64_t)load64_be(blocks[0] + 8 * i));
        b[1][i] = _mm256_set_epi64x((int64_t)load64_be(blocks[3] + 72 + 8 * i), (int64_t)load64_be(blocks[2] + 72 + 8 * i), (int64_t)load64_be(blocks[1] + 72 + 8 * i), (int64_t)load64_be(blocks[0] + 72 + 8 * i));
    }
    for (int j = 2; j <= WCHAIN_V1_ROUNDS; ++j) {
        memcpy(b[j], b[j - 1], sizeof(b[j]));
        mef_v1_256(b[j]);
        for (int i = 0; i < 9; ++i) b[j][i] = _mm256_xor_si256(b[j][i], b[j - 2][i]);
    }
    memcpy(x, vv, sizeof(x));
    for (int j = 1; j <= WCHAIN_V1_ROUNDS; ++j) {
        for (int i = 0; i < 9; ++i) x[i] = _mm256_xor_si256(x[i], b[j - 1][i]);
        round_v1_256(x, j - 1);
    }

    for (int i = 0; i < 9; ++i) {
        uint64_t tmp[4];
        __m256i h = _mm256_xor_si256(_mm256_xor_si256(x[i], b[WCHAIN_V1_ROUNDS][i]), vv[i]);
        _mm256_storeu_si256((__m256i *)tmp, h);
        for (int lane = 0; lane < 4; ++lane) out[lane][i] = tmp[lane];
    }
}

__attribute__((target("avx2")))
void wchain_v2_compress4_avx2(uint64_t out[4][18], uint64_t v[4][18], uint8_t blocks[4][288]) {
    __m256i b[WCHAIN_V2_ROUNDS + 1][18], x[18], vv[18];
    for (int i = 0; i < 18; ++i) {
        vv[i] = _mm256_set_epi64x((int64_t)v[3][i], (int64_t)v[2][i], (int64_t)v[1][i], (int64_t)v[0][i]);
        b[0][i] = _mm256_set_epi64x((int64_t)load64_be(blocks[3] + 8 * i), (int64_t)load64_be(blocks[2] + 8 * i), (int64_t)load64_be(blocks[1] + 8 * i), (int64_t)load64_be(blocks[0] + 8 * i));
        b[1][i] = _mm256_set_epi64x((int64_t)load64_be(blocks[3] + 144 + 8 * i), (int64_t)load64_be(blocks[2] + 144 + 8 * i), (int64_t)load64_be(blocks[1] + 144 + 8 * i), (int64_t)load64_be(blocks[0] + 144 + 8 * i));
    }
    for (int j = 2; j <= WCHAIN_V2_ROUNDS; ++j) {
        memcpy(b[j], b[j - 1], sizeof(b[j]));
        mef_v2_256(b[j]);
        for (int i = 0; i < 18; ++i) b[j][i] = _mm256_xor_si256(b[j][i], b[j - 2][i]);
    }
    memcpy(x, vv, sizeof(x));
    for (int j = 1; j <= WCHAIN_V2_ROUNDS; ++j) {
        for (int i = 0; i < 18; ++i) x[i] = _mm256_xor_si256(x[i], b[j - 1][i]);
        round_v2_256(x, j - 1);
    }
    for (int i = 0; i < 18; ++i) {
        uint64_t tmp[4];
        __m256i h = _mm256_xor_si256(_mm256_xor_si256(x[i], b[WCHAIN_V2_ROUNDS][i]), vv[i]);
        _mm256_storeu_si256((__m256i *)tmp, h);
        for (int lane = 0; lane < 4; ++lane) out[lane][i] = tmp[lane];
    }
}

__attribute__((target("avx512f")))
static inline __m512i rotl512(__m512i x, int n) {
    n &= 63;
    if (n == 0) return x;
    __m128i ln = _mm_cvtsi64_si128(n);
    __m128i rn = _mm_cvtsi64_si128(64 - n);
    return _mm512_or_si512(_mm512_sll_epi64(x, ln), _mm512_srl_epi64(x, rn));
}

__attribute__((target("avx512f")))
static void sbox9_512(__m512i x[9]) {
    const __m512i all = _mm512_set1_epi64(-1);
    __m512i a[9];
    memcpy(a, x, sizeof(a));
    for (int i = 0; i < 9; ++i) {
        __m512i nb = _mm512_xor_si512(a[(i + 2) % 9], all);
        x[i] = _mm512_xor_si512(a[i], _mm512_or_si512(a[(i + 1) % 9], nb));
    }
}

__attribute__((target("avx512f")))
static void pi9_512(__m512i x[9]) {
    __m512i a[9];
    memcpy(a, x, sizeof(a));
    for (int i = 0; i < 9; ++i) x[P9[i]] = a[i];
}

__attribute__((target("avx512f")))
static void mix_512(__m512i y[9], const __m512i x[9], const int rot[9][5]) {
    for (int out = 0; out < 9; ++out) {
        __m512i v = _mm512_setzero_si512();
        for (int k = 0; k < 5; ++k) v = _mm512_xor_si512(v, rotl512(x[L_IDX[out][k]], rot[out][k]));
        y[out] = v;
    }
}

__attribute__((target("avx512f")))
static void round_v1_512(__m512i x[9], int r) {
    __m512i y[9];
    for (int i = 0; i < 9; ++i) x[i] = _mm512_xor_si512(x[i], _mm512_set1_epi64((int64_t)rotl64(PI16[r], i)));
    sbox9_512(x);
    pi9_512(x);
    mix_512(y, x, L_ROT);
    memcpy(x, y, sizeof(y));
}

__attribute__((target("avx512f")))
static void round_v2_512(__m512i x[18], int r) {
    __m512i y[9], t;
    for (int i = 0; i < 9; ++i) {
        x[i] = _mm512_xor_si512(x[i], _mm512_set1_epi64((int64_t)rotl64(PI16[r], i)));
        x[9 + i] = _mm512_xor_si512(x[9 + i], _mm512_set1_epi64((int64_t)rotl64(PI16[r], i + 9)));
    }
    sbox9_512(x);
    sbox9_512(x + 9);
    pi9_512(x);
    pi9_512(x + 9);
    t = x[0]; x[0] = x[9]; x[9] = t;
    t = x[1]; x[1] = x[12]; x[12] = t;
    t = x[3]; x[3] = x[10]; x[10] = t;
    t = x[4]; x[4] = x[13]; x[13] = t;
    t = x[6]; x[6] = x[15]; x[15] = t;
    mix_512(y, x, L_ROT);
    memcpy(x, y, sizeof(y));
    mix_512(y, x + 9, R_ROT);
    memcpy(x + 9, y, sizeof(y));
}

__attribute__((target("avx512f")))
static void mef_v1_512(__m512i x[9]) {
    for (int i = 0; i < 9; ++i) x[i] = rotl512(x[i], ME_ROT[i]);
    sbox9_512(x);
}

__attribute__((target("avx512f")))
static void mef_v2_512(__m512i x[18]) {
    for (int half = 0; half < 2; ++half) {
        __m512i *base = x + half * 9;
        for (int i = 0; i < 9; ++i) base[i] = rotl512(base[i], ME_ROT[i]);
        sbox9_512(base);
    }
}

__attribute__((target("avx512f")))
void wchain_v1_compress8_avx512(uint64_t out[8][9], uint64_t v[8][9], uint8_t blocks[8][144]) {
    __m512i b[WCHAIN_V1_ROUNDS + 1][9], x[9], vv[9];
    for (int i = 0; i < 9; ++i) {
        vv[i] = _mm512_set_epi64((int64_t)v[7][i], (int64_t)v[6][i], (int64_t)v[5][i], (int64_t)v[4][i], (int64_t)v[3][i], (int64_t)v[2][i], (int64_t)v[1][i], (int64_t)v[0][i]);
        b[0][i] = _mm512_set_epi64((int64_t)load64_be(blocks[7] + 8 * i), (int64_t)load64_be(blocks[6] + 8 * i), (int64_t)load64_be(blocks[5] + 8 * i), (int64_t)load64_be(blocks[4] + 8 * i), (int64_t)load64_be(blocks[3] + 8 * i), (int64_t)load64_be(blocks[2] + 8 * i), (int64_t)load64_be(blocks[1] + 8 * i), (int64_t)load64_be(blocks[0] + 8 * i));
        b[1][i] = _mm512_set_epi64((int64_t)load64_be(blocks[7] + 72 + 8 * i), (int64_t)load64_be(blocks[6] + 72 + 8 * i), (int64_t)load64_be(blocks[5] + 72 + 8 * i), (int64_t)load64_be(blocks[4] + 72 + 8 * i), (int64_t)load64_be(blocks[3] + 72 + 8 * i), (int64_t)load64_be(blocks[2] + 72 + 8 * i), (int64_t)load64_be(blocks[1] + 72 + 8 * i), (int64_t)load64_be(blocks[0] + 72 + 8 * i));
    }
    for (int j = 2; j <= WCHAIN_V1_ROUNDS; ++j) {
        memcpy(b[j], b[j - 1], sizeof(b[j]));
        mef_v1_512(b[j]);
        for (int i = 0; i < 9; ++i) b[j][i] = _mm512_xor_si512(b[j][i], b[j - 2][i]);
    }
    memcpy(x, vv, sizeof(x));
    for (int j = 1; j <= WCHAIN_V1_ROUNDS; ++j) {
        for (int i = 0; i < 9; ++i) x[i] = _mm512_xor_si512(x[i], b[j - 1][i]);
        round_v1_512(x, j - 1);
    }
    for (int i = 0; i < 9; ++i) {
        uint64_t tmp[8];
        __m512i h = _mm512_xor_si512(_mm512_xor_si512(x[i], b[WCHAIN_V1_ROUNDS][i]), vv[i]);
        _mm512_storeu_si512((void *)tmp, h);
        for (int lane = 0; lane < 8; ++lane) out[lane][i] = tmp[lane];
    }
}

__attribute__((target("avx512f")))
void wchain_v2_compress8_avx512(uint64_t out[8][18], uint64_t v[8][18], uint8_t blocks[8][288]) {
    __m512i b[WCHAIN_V2_ROUNDS + 1][18], x[18], vv[18];
    for (int i = 0; i < 18; ++i) {
        vv[i] = _mm512_set_epi64((int64_t)v[7][i], (int64_t)v[6][i], (int64_t)v[5][i], (int64_t)v[4][i], (int64_t)v[3][i], (int64_t)v[2][i], (int64_t)v[1][i], (int64_t)v[0][i]);
        b[0][i] = _mm512_set_epi64((int64_t)load64_be(blocks[7] + 8 * i), (int64_t)load64_be(blocks[6] + 8 * i), (int64_t)load64_be(blocks[5] + 8 * i), (int64_t)load64_be(blocks[4] + 8 * i), (int64_t)load64_be(blocks[3] + 8 * i), (int64_t)load64_be(blocks[2] + 8 * i), (int64_t)load64_be(blocks[1] + 8 * i), (int64_t)load64_be(blocks[0] + 8 * i));
        b[1][i] = _mm512_set_epi64((int64_t)load64_be(blocks[7] + 144 + 8 * i), (int64_t)load64_be(blocks[6] + 144 + 8 * i), (int64_t)load64_be(blocks[5] + 144 + 8 * i), (int64_t)load64_be(blocks[4] + 144 + 8 * i), (int64_t)load64_be(blocks[3] + 144 + 8 * i), (int64_t)load64_be(blocks[2] + 144 + 8 * i), (int64_t)load64_be(blocks[1] + 144 + 8 * i), (int64_t)load64_be(blocks[0] + 144 + 8 * i));
    }
    for (int j = 2; j <= WCHAIN_V2_ROUNDS; ++j) {
        memcpy(b[j], b[j - 1], sizeof(b[j]));
        mef_v2_512(b[j]);
        for (int i = 0; i < 18; ++i) b[j][i] = _mm512_xor_si512(b[j][i], b[j - 2][i]);
    }
    memcpy(x, vv, sizeof(x));
    for (int j = 1; j <= WCHAIN_V2_ROUNDS; ++j) {
        for (int i = 0; i < 18; ++i) x[i] = _mm512_xor_si512(x[i], b[j - 1][i]);
        round_v2_512(x, j - 1);
    }
    for (int i = 0; i < 18; ++i) {
        uint64_t tmp[8];
        __m512i h = _mm512_xor_si512(_mm512_xor_si512(x[i], b[WCHAIN_V2_ROUNDS][i]), vv[i]);
        _mm512_storeu_si512((void *)tmp, h);
        for (int lane = 0; lane < 8; ++lane) out[lane][i] = tmp[lane];
    }
}

#endif

#if !defined(WCHAIN_NO_MAIN)

static void print_hex(const uint8_t *p, size_t n) {
    for (size_t i = 0; i < n; ++i) printf("%02x", p[i]);
}

static double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000000000.0 + (double)ts.tv_nsec;
}

static volatile uint64_t sink64;

static void fill_test_data(uint64_t v1[8][9], uint64_t v2[8][18], uint8_t b1[8][144], uint8_t b2[8][288]) {
    for (int lane = 0; lane < 8; ++lane) {
        for (int i = 0; i < 9; ++i) v1[lane][i] = 0x9e3779b97f4a7c15ULL * (uint64_t)(lane + 1) ^ (uint64_t)(i * 0x1010101u);
        for (int i = 0; i < 18; ++i) v2[lane][i] = 0x517cc1b727220a95ULL * (uint64_t)(lane + 1) ^ (uint64_t)(i * 0x0101010101010101ULL);
        for (int i = 0; i < 144; ++i) b1[lane][i] = (uint8_t)(17 + lane * 29 + i * 131);
        for (int i = 0; i < 288; ++i) b2[lane][i] = (uint8_t)(23 + lane * 31 + i * 97);
    }
}

#if !defined(WCHAIN_DISABLE_SIMD)
static int selftest_simd(void) {
    uint64_t v1[8][9], v2[8][18], ref1[8][9], ref2[8][18], out1_4[4][9], out2_4[4][18], out1_8[8][9], out2_8[8][18];
    uint8_t b1[8][144], b2[8][288];
    fill_test_data(v1, v2, b1, b2);
    for (int lane = 0; lane < 8; ++lane) {
        wchain_v1_compress_c(ref1[lane], v1[lane], b1[lane]);
        wchain_v2_compress_c(ref2[lane], v2[lane], b2[lane]);
    }
    if (cpu_has_avx2()) {
        wchain_v1_compress4_avx2(out1_4, v1, b1);
        wchain_v2_compress4_avx2(out2_4, v2, b2);
        if (memcmp(out1_4, ref1, sizeof(out1_4)) != 0 || memcmp(out2_4, ref2, sizeof(out2_4)) != 0) return 0;
    }
    if (cpu_has_avx512f()) {
        wchain_v1_compress8_avx512(out1_8, v1, b1);
        wchain_v2_compress8_avx512(out2_8, v2, b2);
        if (memcmp(out1_8, ref1, sizeof(out1_8)) != 0 || memcmp(out2_8, ref2, sizeof(out2_8)) != 0) return 0;
    }
    return 1;
}
#else
static int selftest_simd(void) {
    return 1;
}
#endif

static void run_kat(void) {
    static const char *msgs[] = {"", "abc", "The quick brown fox jumps over the lazy dog"};
    uint8_t d1[64], d2[128];
    for (int i = 0; i < 3; ++i) {
        const uint8_t *m = (const uint8_t *)msgs[i];
        size_t n = strlen(msgs[i]);
        wchain_v1_hash_c(m, n, d1);
        wchain_v2_hash_c(m, n, d2);
        printf("WChain-V1(\"%s\") = ", msgs[i]); print_hex(d1, 64); printf("\n");
        printf("WChain-V2(\"%s\") = ", msgs[i]); print_hex(d2, 128); printf("\n");
    }
}

static void run_bench(void) {
    uint64_t v1[8][9], v2[8][18], out1[8][9], out2[8][18];
    uint8_t b1[8][144], b2[8][288];
    fill_test_data(v1, v2, b1, b2);

    const int n_scalar = 300000;
    double t0, t1, ns;

    t0 = now_ns();
    for (int it = 0; it < n_scalar; ++it) {
        v1[0][0] ^= (uint64_t)it;
        wchain_v1_compress_c(out1[0], v1[0], b1[0]);
        sink64 ^= out1[0][0] ^ (uint64_t)it;
    }
    t1 = now_ns();
    ns = (t1 - t0) / n_scalar;
    printf("C scalar V1 compress: %.2f ns, %.2f MB/s\n", ns, 144000.0 / ns);

    t0 = now_ns();
    for (int it = 0; it < n_scalar; ++it) {
        v2[0][0] ^= (uint64_t)it;
        wchain_v2_compress_c(out2[0], v2[0], b2[0]);
        sink64 ^= out2[0][0] ^ (uint64_t)it;
    }
    t1 = now_ns();
    ns = (t1 - t0) / n_scalar;
    printf("C scalar V2 compress: %.2f ns, %.2f MB/s\n", ns, 288000.0 / ns);

#if !defined(WCHAIN_DISABLE_SIMD)
    const int n_simd = 300000;

    if (cpu_has_avx2()) {
        t0 = now_ns();
        for (int it = 0; it < n_simd; ++it) {
            v1[it & 3][0] ^= (uint64_t)it;
            wchain_v1_compress4_avx2(out1, v1, b1);
            sink64 ^= out1[it & 3][0];
        }
        t1 = now_ns();
        ns = (t1 - t0) / n_simd;
        printf("AVX2x4 V1 compress batch: %.2f ns/batch, %.2f MB/s\n", ns, (4.0 * 144000.0) / ns);

        t0 = now_ns();
        for (int it = 0; it < n_simd; ++it) {
            v2[it & 3][0] ^= (uint64_t)it;
            wchain_v2_compress4_avx2(out2, v2, b2);
            sink64 ^= out2[it & 3][0];
        }
        t1 = now_ns();
        ns = (t1 - t0) / n_simd;
        printf("AVX2x4 V2 compress batch: %.2f ns/batch, %.2f MB/s\n", ns, (4.0 * 288000.0) / ns);
    } else {
        printf("AVX2 unsupported on this CPU/OS, skipped.\n");
    }

    if (cpu_has_avx512f()) {
        t0 = now_ns();
        for (int it = 0; it < n_simd; ++it) {
            v1[it & 7][0] ^= (uint64_t)it;
            wchain_v1_compress8_avx512(out1, v1, b1);
            sink64 ^= out1[it & 7][0];
        }
        t1 = now_ns();
        ns = (t1 - t0) / n_simd;
        printf("AVX512x8 V1 compress batch: %.2f ns/batch, %.2f MB/s\n", ns, (8.0 * 144000.0) / ns);

        t0 = now_ns();
        for (int it = 0; it < n_simd; ++it) {
            v2[it & 7][0] ^= (uint64_t)it;
            wchain_v2_compress8_avx512(out2, v2, b2);
            sink64 ^= out2[it & 7][0];
        }
        t1 = now_ns();
        ns = (t1 - t0) / n_simd;
        printf("AVX512x8 V2 compress batch: %.2f ns/batch, %.2f MB/s\n", ns, (8.0 * 288000.0) / ns);
    } else {
        printf("AVX-512F unsupported on this CPU/OS, skipped.\n");
    }
#else
    printf("SIMD disabled at compile time, skipped.\n");
#endif
    printf("sink: %llu\n", (unsigned long long)sink64);
}

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--kat") == 0) {
        run_kat();
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "--hash-v1") == 0) {
        uint8_t d[64];
        wchain_v1_hash_c((const uint8_t *)argv[2], strlen(argv[2]), d);
        print_hex(d, 64);
        printf("\n");
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "--hash-v2") == 0) {
        uint8_t d[128];
        wchain_v2_hash_c((const uint8_t *)argv[2], strlen(argv[2]), d);
        print_hex(d, 128);
        printf("\n");
        return 0;
    }
    run_kat();
    printf("SIMD selftest: %s\n", selftest_simd() ? "ok" : "FAILED");
    run_bench();
    return 0;
}

#endif
