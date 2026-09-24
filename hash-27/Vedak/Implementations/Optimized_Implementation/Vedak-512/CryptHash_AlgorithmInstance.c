/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/
#define _CRT_SECURE_NO_WARNINGS
#include "CryptHash_AlgorithmInstance.h"

#include <immintrin.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#if !defined(_WIN32)
#include <sys/time.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(_MSC_VER)
#include <intrin.h>
#define FINL __forceinline
#define RESTRICT __restrict
#define ALIGN32 __declspec(align(32))
#else
#include <x86intrin.h>
#define FINL __attribute__((always_inline)) inline
#define RESTRICT __restrict__
#define ALIGN32 __attribute__((aligned(32)))
#endif

#define CRYPTHASH_SUCCESS 0
#define CRYPTHASH_ERR_NULLPTR 1
#define CRYPTHASH_ERR_BAD_DIGEST_LEN 2

typedef union {
    uint8_t b[32];
    __m256i v;
} u256;

typedef union {
    uint8_t b[16];
    __m128i v;
} u128;

#define U256_INIT(...) {{ __VA_ARGS__ }}
#define U128_INIT(...) {{ __VA_ARGS__ }}
//static const ALIGN64 uint8_t S512_BYTES[64] = {
//    0x1,0x0,0x5,0x6,0xC,0x9,0x2,0x8,0xA,0x7,0x3,0xF,0xE,0xB,0x4,0xD,
//    0x1,0x0,0x5,0x6,0xC,0x9,0x2,0x8,0xA,0x7,0x3,0xF,0xE,0xB,0x4,0xD,
//    0x1,0x0,0x5,0x6,0xC,0x9,0x2,0x8,0xA,0x7,0x3,0xF,0xE,0xB,0x4,0xD,
//    0x1,0x0,0x5,0x6,0xC,0x9,0x2,0x8,0xA,0x7,0x3,0xF,0xE,0xB,0x4,0xD
//};


//static const u256 S256 = U256_INIT(
//    0x3, 0x0, 0x6, 0x2, 0x5, 0x4, 0xF, 0xE,
//    0xA, 0x8, 0x7, 0x9, 0x1, 0xC, 0xD, 0xB,
//    0x3, 0x0, 0x6, 0x2, 0x5, 0x4, 0xF, 0xE,
//    0xA, 0x8, 0x7, 0x9, 0x1, 0xC, 0xD, 0xB);

static const u256 S256 = U256_INIT(
        0x1,0x0,0x5,0x6,0xC,0x9,0x2,0x8,
        0xA,0x7,0x3,0xF,0xE,0xB,0x4,0xD,
        0x1,0x0,0x5,0x6,0xC,0x9,0x2,0x8,
        0xA,0x7,0x3,0xF,0xE,0xB,0x4,0xD);

static const u256 CON0F = U256_INIT(
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f);

static const u256 ROW1_SHUF_LO = U256_INIT(7, 0, 1, 2, 3, 4, 5, 6, 15, 8, 9, 10, 11, 12, 13, 14, 7, 0, 1, 2, 3, 4, 5, 6, 15, 8, 9, 10, 11, 12, 13, 14);
static const u256 ROW1_SHUF_HI = U256_INIT(7, 0, 1, 2, 3, 4, 5, 6, 15, 8, 9, 10, 11, 12, 13, 14, 7, 0, 1, 2, 3, 4, 5, 6, 15, 8, 9, 10, 11, 12, 13, 14);
static const u256 ROW2_SHUF_LO = U256_INIT(5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12, 5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12);
static const u256 ROW2_SHUF_HI = U256_INIT(5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12, 5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12);
static const u256 ROW4_SHUF_LO = U256_INIT(9, 10, 11, 12, 13, 14, 15, 8, 1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8, 1, 2, 3, 4, 5, 6, 7, 0);
static const u256 ROW4_SHUF_HI = U256_INIT(9, 10, 11, 12, 13, 14, 15, 8, 1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8, 1, 2, 3, 4, 5, 6, 7, 0);
static const u256 ROW6_SHUF_LO = U256_INIT(3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10, 11, 12, 13, 14, 15, 8, 9, 10, 3, 4, 5, 6, 7, 0, 1, 2);
static const u256 ROW6_SHUF_HI = U256_INIT(3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10, 11, 12, 13, 14, 15, 8, 9, 10, 3, 4, 5, 6, 7, 0, 1, 2);
static const u256 ROW7_SHUF_LO = U256_INIT(5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12, 13, 14, 15, 8, 9, 10, 11, 12, 5, 6, 7, 0, 1, 2, 3, 4);
static const u256 ROW7_SHUF_HI = U256_INIT(5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12, 13, 14, 15, 8, 9, 10, 11, 12, 5, 6, 7, 0, 1, 2, 3, 4);
static const u256 ROW8_SHUF_LO = U256_INIT(11, 12, 13, 14, 15, 8, 9, 10, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10);
static const u256 ROW8_SHUF_HI = U256_INIT(11, 12, 13, 14, 15, 8, 9, 10, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10);
static const u256 ROW9_SHUF_LO = U256_INIT(15, 8, 9, 10, 11, 12, 13, 14, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 15, 8, 9, 10, 11, 12, 13, 14);
static const u256 ROW9_SHUF_HI = U256_INIT(15, 8, 9, 10, 11, 12, 13, 14, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 15, 8, 9, 10, 11, 12, 13, 14);
static const u128 NIB_EVEN = U128_INIT(0, 2, 4, 6, 8, 10, 12, 14, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);
static const u128 NIB_ODD = U128_INIT(1, 3, 5, 7, 9, 11, 13, 15, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80);

#define RCROW(a,b,c,d,e,f,g,h) U256_INIT(a,b,c,d,e,f,g,h,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0)
static const u256 RC256[40] = {
    RCROW(0x2,0xC,0x3,0x8,0x7,0xD,0x6,0x9), RCROW(0x9,0x8,0x8,0xC,0xC,0x9,0xD,0xD),
    RCROW(0xF,0x0,0xE,0x4,0xA,0x1,0xB,0x5), RCROW(0x2,0x1,0x3,0x5,0x7,0x0,0x6,0x4),
    RCROW(0x8,0x3,0x9,0x7,0xD,0x2,0xC,0x6), RCROW(0xC,0x7,0xD,0x3,0x9,0x6,0x8,0x2),
    RCROW(0x4,0xF,0x5,0xB,0x1,0xE,0x0,0xA), RCROW(0x5,0xE,0x4,0xA,0x0,0xF,0x1,0xB),
    RCROW(0x7,0xC,0x6,0x8,0x2,0xD,0x3,0x9), RCROW(0x3,0x9,0x2,0xD,0x6,0x8,0x7,0xC),
    RCROW(0xB,0x3,0xA,0x7,0xE,0x2,0xF,0x6), RCROW(0xA,0x7,0xB,0x3,0xF,0x6,0xE,0x2),
    RCROW(0x8,0xE,0x9,0xA,0xD,0xF,0xC,0xB), RCROW(0xD,0xC,0xC,0x8,0x8,0xD,0x9,0x9),
    RCROW(0x7,0x8,0x6,0xC,0x2,0x9,0x3,0xD), RCROW(0x3,0x0,0x2,0x4,0x6,0x1,0x7,0x5),
    RCROW(0xA,0x1,0xB,0x5,0xF,0x0,0xE,0x4), RCROW(0x8,0x2,0x9,0x6,0xD,0x3,0xC,0x7),
    RCROW(0xC,0x5,0xD,0x1,0x9,0x4,0x8,0x0), RCROW(0x4,0xA,0x5,0xE,0x1,0xB,0x0,0xF),
    RCROW(0x5,0x5,0x4,0x1,0x0,0x4,0x1,0x0), RCROW(0x6,0xB,0x7,0xF,0x3,0xA,0x2,0xE),
    RCROW(0x1,0x7,0x0,0x3,0x4,0x6,0x5,0x2), RCROW(0xE,0xF,0xF,0xB,0xB,0xE,0xA,0xA),
    RCROW(0x1,0xF,0x0,0xB,0x4,0xE,0x5,0xA), RCROW(0xF,0xF,0xE,0xB,0xA,0xE,0xB,0xA),
    RCROW(0x3,0xF,0x2,0xB,0x6,0xE,0x7,0xA), RCROW(0xB,0xF,0xA,0xB,0xE,0xE,0xF,0xA),
    RCROW(0xB,0xE,0xA,0xA,0xE,0xF,0xF,0xB), RCROW(0xB,0xC,0xA,0x8,0xE,0xD,0xF,0x9),
    RCROW(0xB,0x9,0xA,0xD,0xE,0x8,0xF,0xC), RCROW(0xB,0x2,0xA,0x6,0xE,0x3,0xF,0x7),
    RCROW(0xA,0x5,0xB,0x1,0xF,0x4,0xE,0x0), RCROW(0x8,0xB,0x9,0xF,0xD,0xA,0xC,0xE),
    RCROW(0xD,0x7,0xC,0x3,0x8,0x6,0x9,0x2), RCROW(0x6,0xF,0x7,0xB,0x3,0xE,0x2,0xA),
    RCROW(0x1,0xE,0x0,0xA,0x4,0xF,0x5,0xB), RCROW(0xF,0xD,0xE,0x9,0xA,0xC,0xB,0x8),
    RCROW(0x3,0xA,0x2,0xE,0x6,0xB,0x7,0xF), RCROW(0xB,0x4,0xA,0x0,0xE,0x5,0xF,0x1)
};
#undef RCROW

static FINL __m256i swap128(__m256i x)
{
    return _mm256_permute2x128_si256(x, x, 0x01);
}

static FINL void expand32_to_nibbles_2ymm(const uint8_t* RESTRICT src, __m256i* out_lo, __m256i* out_hi)
{
    __m256i x = _mm256_loadu_si256((const __m256i*)src);
    __m256i lo = _mm256_and_si256(x, CON0F.v);
    __m256i hi = _mm256_and_si256(_mm256_srli_epi16(x, 4), CON0F.v);
    __m256i t0 = _mm256_unpacklo_epi8(hi, lo);
    __m256i t1 = _mm256_unpackhi_epi8(hi, lo);
    *out_lo = _mm256_permute2x128_si256(t0, t1, 0x20);
    *out_hi = _mm256_permute2x128_si256(t0, t1, 0x31);
}

static FINL void nib32_to_bytes16(uint8_t* RESTRICT dst16, __m256i v32nib)
{
    __m128i a = _mm256_castsi256_si128(v32nib);
    __m128i b = _mm256_extracti128_si256(v32nib, 1);

    __m128i ae = _mm_shuffle_epi8(a, NIB_EVEN.v);
    __m128i ao = _mm_shuffle_epi8(a, NIB_ODD.v);
    ae = _mm_slli_epi16(ae, 4);
    __m128i ap = _mm_or_si128(ae, ao);

    __m128i be = _mm_shuffle_epi8(b, NIB_EVEN.v);
    __m128i bo = _mm_shuffle_epi8(b, NIB_ODD.v);
    be = _mm_slli_epi16(be, 4);
    __m128i bp = _mm_or_si128(be, bo);

    _mm_storel_epi64((__m128i*)(dst16 + 0), ap);
    _mm_storel_epi64((__m128i*)(dst16 + 8), bp);
}

static FINL void dump_vec32bytes(uint8_t* RESTRICT dst32, __m256i lo, __m256i hi)
{
    nib32_to_bytes16(dst32 + 0, lo);
    nib32_to_bytes16(dst32 + 16, hi);
}

#define XO2T(a,b) do { \
    t[(a)*2+0] = _mm256_xor_si256(t[(a)*2+0], t[(b)*2+0]); \
    t[(a)*2+1] = _mm256_xor_si256(t[(a)*2+1], t[(b)*2+1]); \
} while (0)

static FINL void vedak_round(__m256i st[20], int r)
{
    ALIGN32 __m256i t[20];

    t[0] = _mm256_shuffle_epi8(S256.v, st[0]);
    t[1] = _mm256_shuffle_epi8(S256.v, st[1]);
    t[2] = _mm256_shuffle_epi8(S256.v, st[2]);
    t[3] = _mm256_shuffle_epi8(S256.v, st[3]);
    t[4] = _mm256_shuffle_epi8(S256.v, st[4]);
    t[5] = _mm256_shuffle_epi8(S256.v, st[5]);
    t[6] = _mm256_shuffle_epi8(S256.v, st[6]);
    t[7] = _mm256_shuffle_epi8(S256.v, st[7]);
    t[8] = _mm256_shuffle_epi8(S256.v, st[8]);
    t[9] = _mm256_shuffle_epi8(S256.v, st[9]);
    t[10] = _mm256_shuffle_epi8(S256.v, st[10]);
    t[11] = _mm256_shuffle_epi8(S256.v, st[11]);
    t[12] = _mm256_shuffle_epi8(S256.v, st[12]);
    t[13] = _mm256_shuffle_epi8(S256.v, st[13]);
    t[14] = _mm256_shuffle_epi8(S256.v, st[14]);
    t[15] = _mm256_shuffle_epi8(S256.v, st[15]);
    t[16] = _mm256_shuffle_epi8(S256.v, st[16]);
    t[17] = _mm256_shuffle_epi8(S256.v, st[17]);
    t[18] = _mm256_shuffle_epi8(S256.v, st[18]);
    t[19] = _mm256_shuffle_epi8(S256.v, st[19]);

    XO2T(5, 0); XO2T(6, 1); XO2T(7, 2); XO2T(8, 3); XO2T(9, 4);
    XO2T(0, 6); XO2T(1, 7); XO2T(2, 8); XO2T(3, 9); XO2T(4, 5);
    XO2T(5, 2); XO2T(6, 3); XO2T(7, 4); XO2T(8, 0); XO2T(9, 1);
    XO2T(0, 5); XO2T(1, 6); XO2T(2, 7); XO2T(3, 8); XO2T(4, 9);
    XO2T(5, 4); XO2T(6, 0); XO2T(7, 1); XO2T(8, 2); XO2T(9, 3);

    st[0] = _mm256_shuffle_epi32(t[19], 0x1B);
    st[1] = _mm256_shuffle_epi32(t[18], 0x1B);

    st[2] = _mm256_shuffle_epi8(swap128(t[12]), ROW1_SHUF_LO.v);
    st[3] = _mm256_shuffle_epi8(swap128(t[13]), ROW1_SHUF_HI.v);

    st[4] = _mm256_shuffle_epi8(swap128(t[15]), ROW2_SHUF_LO.v);
    st[5] = _mm256_shuffle_epi8(swap128(t[14]), ROW2_SHUF_HI.v);

    st[6] = _mm256_shuffle_epi32(swap128(t[17]), 0x4E);
    st[7] = _mm256_shuffle_epi32(swap128(t[16]), 0x4E);

    st[8] = _mm256_shuffle_epi8(swap128(t[10]), ROW4_SHUF_LO.v);
    st[9] = _mm256_shuffle_epi8(swap128(t[11]), ROW4_SHUF_HI.v);

    st[10] = t[8];
    st[11] = t[9];

    st[12] = _mm256_shuffle_epi8(t[4], ROW6_SHUF_LO.v);
    st[13] = _mm256_shuffle_epi8(t[5], ROW6_SHUF_HI.v);

    st[14] = _mm256_shuffle_epi8(t[3], ROW7_SHUF_LO.v);
    st[15] = _mm256_shuffle_epi8(t[2], ROW7_SHUF_HI.v);

    st[16] = _mm256_shuffle_epi8(t[1], ROW8_SHUF_LO.v);
    st[17] = _mm256_shuffle_epi8(t[0], ROW8_SHUF_HI.v);

    st[18] = _mm256_shuffle_epi8(t[6], ROW9_SHUF_LO.v);
    st[19] = _mm256_shuffle_epi8(t[7], ROW9_SHUF_HI.v);

    st[0] = _mm256_xor_si256(st[0], RC256[r].v);
}
#undef XO2T

static FINL void vedak_p40(__m256i st[20])
{
    int r;
    for (r = 0; r < 40; ++r) {
        vedak_round(st, r);
    }
}

static FINL void absorb_r1536(__m256i st[20], const uint8_t* RESTRICT blk192)
{
    __m256i a, b;
    expand32_to_nibbles_2ymm(blk192 + 0, &a, &b); st[0] = _mm256_xor_si256(st[0], a); st[1] = _mm256_xor_si256(st[1], b);
    expand32_to_nibbles_2ymm(blk192 + 32, &a, &b); st[2] = _mm256_xor_si256(st[2], a); st[3] = _mm256_xor_si256(st[3], b);
    expand32_to_nibbles_2ymm(blk192 + 64, &a, &b); st[4] = _mm256_xor_si256(st[4], a); st[5] = _mm256_xor_si256(st[5], b);
    expand32_to_nibbles_2ymm(blk192 + 96, &a, &b); st[6] = _mm256_xor_si256(st[6], a); st[7] = _mm256_xor_si256(st[7], b);
    expand32_to_nibbles_2ymm(blk192 + 128, &a, &b); st[8] = _mm256_xor_si256(st[8], a); st[9] = _mm256_xor_si256(st[9], b);
    expand32_to_nibbles_2ymm(blk192 + 160, &a, &b); st[10] = _mm256_xor_si256(st[10], a); st[11] = _mm256_xor_si256(st[11], b);
}

static FINL void absorb_r1024(__m256i st[20], const uint8_t* RESTRICT blk128)
{
    __m256i a, b;
    expand32_to_nibbles_2ymm(blk128 + 0, &a, &b); st[0] = _mm256_xor_si256(st[0], a); st[1] = _mm256_xor_si256(st[1], b);
    expand32_to_nibbles_2ymm(blk128 + 32, &a, &b); st[2] = _mm256_xor_si256(st[2], a); st[3] = _mm256_xor_si256(st[3], b);
    expand32_to_nibbles_2ymm(blk128 + 64, &a, &b); st[4] = _mm256_xor_si256(st[4], a); st[5] = _mm256_xor_si256(st[5], b);
    expand32_to_nibbles_2ymm(blk128 + 96, &a, &b); st[6] = _mm256_xor_si256(st[6], a); st[7] = _mm256_xor_si256(st[7], b);
}

static FINL void absorb_r512(__m256i st[20], const uint8_t* RESTRICT blk64)
{
    __m256i a, b;
    expand32_to_nibbles_2ymm(blk64 + 0, &a, &b); st[0] = _mm256_xor_si256(st[0], a); st[1] = _mm256_xor_si256(st[1], b);
    expand32_to_nibbles_2ymm(blk64 + 32, &a, &b); st[2] = _mm256_xor_si256(st[2], a); st[3] = _mm256_xor_si256(st[3], b);
}

static FINL void pad10x1_byte_aligned(uint8_t* RESTRICT blk, size_t rate_bytes, size_t tail_bytes)
{
    blk[tail_bytes] ^= 0x80u;
    blk[rate_bytes - 1] ^= 0x01u;
}

static void pad10x1_bit_level(uint8_t* blk, size_t rate_bytes, const uint8_t* tail, size_t tail_bits)
{
    size_t full = tail_bits >> 3;
    size_t rem = tail_bits & 7u;

    memset(blk, 0, rate_bytes);
    if (full != 0u) {
        memcpy(blk, tail, full);
    }

    if (rem != 0u) {
        uint8_t mask = (uint8_t)(0xFFu << (8u - rem));
        blk[full] = (uint8_t)(tail[full] & mask);
    }

    blk[full] |= (uint8_t)(1u << (7u - rem));
    blk[rate_bytes - 1] |= 0x01u;
}

static FINL void zero_state20(__m256i st[20])
{
    int i;
    __m256i z = _mm256_setzero_si256();
    for (i = 0; i < 20; ++i) {
        st[i] = z;
    }
}

static void vedak512_hash(const uint8_t* RESTRICT msg, size_t msg_bits, uint8_t* RESTRICT out64)
{
    ALIGN32 __m256i st[20];
    const size_t rate_bytes = 192u;
    const size_t msg_bytes_full = msg_bits >> 3;
    const size_t full_blocks = msg_bytes_full / rate_bytes;
    const size_t full_bytes = full_blocks * rate_bytes;
    const uint8_t* p = msg;
    ALIGN32 uint8_t last[192];
    size_t b;
    size_t tail_bits;

    zero_state20(st);
    for (b = 0; b < full_blocks; ++b) {
        absorb_r1536(st, p);
        vedak_p40(st);
        p += rate_bytes;
    }

    tail_bits = msg_bits - (full_bytes << 3);
    if ((tail_bits & 7u) == 0u) {
        size_t tail_bytes = tail_bits >> 3;
        memset(last, 0, sizeof(last));
        if (tail_bytes != 0u) {
            memcpy(last, p, tail_bytes);
        }
        pad10x1_byte_aligned(last, rate_bytes, tail_bytes);
    }
    else if (tail_bits == (rate_bytes << 3) - 1u) {
        pad10x1_bit_level(last, rate_bytes, p, tail_bits);
        absorb_r1536(st, last);
        vedak_p40(st);
        memset(last, 0, sizeof(last));
        last[rate_bytes - 1] = 0x01u;
    }
    else {
        pad10x1_bit_level(last, rate_bytes, p, tail_bits);
    }

    absorb_r1536(st, last);
    vedak_p40(st);
    dump_vec32bytes(out64 + 0, st[0], st[1]);
    dump_vec32bytes(out64 + 32, st[2], st[3]);
}

static void vedak768_hash(const uint8_t* RESTRICT msg, size_t msg_bits, uint8_t* RESTRICT out96)
{
    ALIGN32 __m256i st[20];
    const size_t rate_bytes = 128u;
    const size_t msg_bytes_full = msg_bits >> 3;
    const size_t full_blocks = msg_bytes_full / rate_bytes;
    const size_t full_bytes = full_blocks * rate_bytes;
    const uint8_t* p = msg;
    ALIGN32 uint8_t last[128];
    size_t b;
    size_t tail_bits;

    zero_state20(st);
    for (b = 0; b < full_blocks; ++b) {
        absorb_r1024(st, p);
        vedak_p40(st);
        p += rate_bytes;
    }

    tail_bits = msg_bits - (full_bytes << 3);
    if ((tail_bits & 7u) == 0u) {
        size_t tail_bytes = tail_bits >> 3;
        memset(last, 0, sizeof(last));
        if (tail_bytes != 0u) {
            memcpy(last, p, tail_bytes);
        }
        pad10x1_byte_aligned(last, rate_bytes, tail_bytes);
    }
    else if (tail_bits == (rate_bytes << 3) - 1u) {
        pad10x1_bit_level(last, rate_bytes, p, tail_bits);
        absorb_r1024(st, last);
        vedak_p40(st);
        memset(last, 0, sizeof(last));
        last[rate_bytes - 1] = 0x01u;
    }
    else {
        pad10x1_bit_level(last, rate_bytes, p, tail_bits);
    }

    absorb_r1024(st, last);
    vedak_p40(st);
    dump_vec32bytes(out96 + 0, st[0], st[1]);
    dump_vec32bytes(out96 + 32, st[2], st[3]);
    dump_vec32bytes(out96 + 64, st[4], st[5]);
}

static void vedak1024_hash(const uint8_t* RESTRICT msg, size_t msg_bits, uint8_t* RESTRICT out128)
{
    ALIGN32 __m256i st[20];
    const size_t rate_bytes = 64u;
    const size_t msg_bytes_full = msg_bits >> 3;
    const size_t full_blocks = msg_bytes_full / rate_bytes;
    const size_t full_bytes = full_blocks * rate_bytes;
    const uint8_t* p = msg;
    ALIGN32 uint8_t last[64];
    size_t b;
    size_t tail_bits;

    zero_state20(st);
    for (b = 0; b < full_blocks; ++b) {
        absorb_r512(st, p);
        vedak_p40(st);
        p += rate_bytes;
    }

    tail_bits = msg_bits - (full_bytes << 3);
    if ((tail_bits & 7u) == 0u) {
        size_t tail_bytes = tail_bits >> 3;
        memset(last, 0, sizeof(last));
        if (tail_bytes != 0u) {
            memcpy(last, p, tail_bytes);
        }
        pad10x1_byte_aligned(last, rate_bytes, tail_bytes);
    }
    else if (tail_bits == (rate_bytes << 3) - 1u) {
        pad10x1_bit_level(last, rate_bytes, p, tail_bits);
        absorb_r512(st, last);
        vedak_p40(st);
        memset(last, 0, sizeof(last));
        last[rate_bytes - 1] = 0x01u;
    }
    else {
        pad10x1_bit_level(last, rate_bytes, p, tail_bits);
    }

    absorb_r512(st, last);
    vedak_p40(st);
    dump_vec32bytes(out128 + 0, st[0], st[1]);
    dump_vec32bytes(out128 + 32, st[2], st[3]);

    vedak_p40(st);
    dump_vec32bytes(out128 + 64, st[0], st[1]);
    dump_vec32bytes(out128 + 96, st[2], st[3]);
}

int CryptHash(int digest_len_bits, const unsigned char* msg, unsigned long long msg_len_bits, unsigned char* digest)
{
    if (digest == NULL) {
        return CRYPTHASH_ERR_NULLPTR;
    }
    if (msg_len_bits > 0ULL && msg == NULL) {
        return CRYPTHASH_ERR_NULLPTR;
    }

    switch (digest_len_bits) {
    case 512:
        vedak512_hash((const uint8_t*)msg, (size_t)msg_len_bits, (uint8_t*)digest);
        return CRYPTHASH_SUCCESS;
    case 768:
        vedak768_hash((const uint8_t*)msg, (size_t)msg_len_bits, (uint8_t*)digest);
        return CRYPTHASH_SUCCESS;
    case 1024:
        vedak1024_hash((const uint8_t*)msg, (size_t)msg_len_bits, (uint8_t*)digest);
        return CRYPTHASH_SUCCESS;
    default:
        return CRYPTHASH_ERR_BAD_DIGEST_LEN;
    }
}

#if defined(_MSC_VER)
static __forceinline void cpuid_serializing(void)
{
    int info[4];
    __cpuid(info, 0);
}
static __forceinline uint64_t read_tsc(void)
{
    return __rdtsc();
}
static __forceinline uint64_t read_tscp(void)
{
    unsigned int aux;
    return __rdtscp(&aux);
}
#else
static inline void cpuid_serializing(void)
{
    unsigned int a, b, c, d;
    a = 0;
    __asm__ __volatile__("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(a));
}
static inline uint64_t read_tsc(void)
{
    return __rdtsc();
}
static inline uint64_t read_tscp(void)
{
    unsigned int aux;
    return __rdtscp(&aux);
}
#endif

static double wall_time_seconds(void)
{
#if defined(_WIN32)
    static LARGE_INTEGER freq;
    static int initialized = 0;
    LARGE_INTEGER counter;
    if (!initialized) {
        QueryPerformanceFrequency(&freq);
        initialized = 1;
    }
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
#endif
}

static size_t get_repeat_count(size_t msg_len_bytes)
{
    if (msg_len_bytes <= 64u) return 30000u;
    if (msg_len_bytes <= 4096u) return 12000u;
    if (msg_len_bytes <= 65536u) return 3000u;
    return 300u;
}

static void fill_test_data(uint8_t* buf, size_t len)
{
    size_t i;
    for (i = 0; i < len; ++i) {
        buf[i] = (uint8_t)((i * 131u + 17u) & 0xFFu);
    }
}

static void print_digest_head(const uint8_t* digest, size_t digest_bytes, size_t head_bytes)
{
    size_t i;
    size_t n = (digest_bytes < head_bytes) ? digest_bytes : head_bytes;
    for (i = 0; i < n; ++i) {
        printf("%02x", (unsigned int)digest[i]);
    }
}

static void print_hex_multiline_upper(const uint8_t* data, size_t len)
{
    size_t i;
    if (len == 0u) {
        putchar('\n');
        return;
    }
    for (i = 0; i < len; ++i) {
        printf("%02X", (unsigned int)data[i]);
        if (i + 1u == len || ((i + 1u) % 16u) == 0u) {
            putchar('\n');
        }
        else {
            putchar(' ');
        }
    }
}

static void bench_one(int digest_len_bits, size_t msg_len_bytes)
{
    size_t repeat = get_repeat_count(msg_len_bytes);
    size_t digest_len_bytes = (size_t)(digest_len_bits >> 3);
    uint8_t* msg = NULL;
    uint8_t* digest = NULL;
    const unsigned char* msg_ptr;
    unsigned char* digest_ptr;
    size_t i;
    volatile uint8_t sink = 0;
    uint64_t c0, c1;
    double t0, t1;
    double sec, total_bytes, total_bits, total_cycles, mbps, cyc_per_byte;

    if (msg_len_bytes != 0u) {
        msg = (uint8_t*)malloc(msg_len_bytes);
        if (msg == NULL) {
            fprintf(stderr, "malloc failed for msg buffer\n");
            return;
        }
        fill_test_data(msg, msg_len_bytes);
    }

    digest = (uint8_t*)malloc(digest_len_bytes);
    if (digest == NULL) {
        free(msg);
        fprintf(stderr, "malloc failed for digest buffer\n");
        return;
    }

    msg_ptr = (msg_len_bytes == 0u) ? NULL : (const unsigned char*)msg;
    digest_ptr = (unsigned char*)digest;

    for (i = 0; i < 8u; ++i) {
        int rc = CryptHash(digest_len_bits, msg_ptr,
            (unsigned long long)msg_len_bytes * 8ULL,
            digest_ptr);
        if (rc != 0) {
            fprintf(stderr, "CryptHash warm-up failed, rc = %d\n", rc);
            free(digest);
            free(msg);
            return;
        }
    }

    cpuid_serializing();
    c0 = read_tsc();
    t0 = wall_time_seconds();

    for (i = 0; i < repeat; ++i) {
        int rc = CryptHash(digest_len_bits, msg_ptr,
            (unsigned long long)msg_len_bytes * 8ULL,
            digest_ptr);
        if (rc != 0) {
            fprintf(stderr, "CryptHash benchmark failed, rc = %d\n", rc);
            free(digest);
            free(msg);
            return;
        }
        sink ^= digest[0];
    }

    t1 = wall_time_seconds();
    c1 = read_tscp();
    cpuid_serializing();
    (void)sink;

    sec = t1 - t0;
    total_bytes = (double)msg_len_bytes * (double)repeat;
    total_bits = total_bytes * 8.0;
    total_cycles = (double)(c1 - c0);
    mbps = (sec > 0.0) ? (total_bits / 1e6) / sec : 0.0;
    cyc_per_byte = (total_bytes > 0.0) ? (total_cycles / total_bytes) : 0.0;

    printf("%-8d%-12zu%-10zu%-14.2f%-14.3f", digest_len_bits, msg_len_bytes, repeat, mbps, cyc_per_byte);
    print_digest_head(digest, digest_len_bytes, 16u);
    putchar('\n');

    free(digest);
    free(msg);
}

int mainspeed(void)
{
#if defined(__GNUC__) || defined(__clang__)
    if (!__builtin_cpu_supports("avx2")) {
        fprintf(stderr, "AVX2 not supported.\n");
        return 1;
    }
#endif

    {
        const int digests[] = { 512, 768, 1024 };
        const size_t msg_sizes[] = {
            0u, 1u, 8u, 16u, 32u, 64u, 65u, 128u, 256u, 512u,
            1024u, 2048u, 4096u, 4097u, 8192u, 16384u,
            32768u, 65536u, 65537u, 131072u, 1048576u
        };
        size_t i, j;

        printf("%-8s%-12s%-10s%-14s%-14s%s\n", "Digest", "Msg(B)", "Repeat", "Mbps", "cyc/B", "DigestHead16");
        printf("------------------------------------------------------------------------\n");

        for (i = 0; i < sizeof(digests) / sizeof(digests[0]); ++i) {
            for (j = 0; j < sizeof(msg_sizes) / sizeof(msg_sizes[0]); ++j) {
                bench_one(digests[i], msg_sizes[j]);
            }
            printf("------------------------------------------------------------------------\n");
        }
    }
    return 0;
}

int main_test(void)
{
#if defined(__GNUC__) || defined(__clang__)
    if (!__builtin_cpu_supports("avx2")) {
        fprintf(stderr, "AVX2 not supported.\n");
        return 1;
    }
#endif

    {
        static const uint8_t range_00_1f[32] = {
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
            0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
        };
        static const uint8_t bits_1[1] = { 0x80 };
        static const uint8_t bits_13[2] = { 0xCA, 0xF0 };
        static const uint8_t allzero_64b[64] = { 0 };
        static const int digests[] = { 512, 768, 1024 };

        typedef struct TestCase {
            const char* name;
            const uint8_t* msg;
            size_t msg_bytes;
            unsigned long long msg_bits;
        } TestCase;

        static const TestCase cases[] = {
            { "Empty",        NULL,          0u,  0ULL   },
            { "Range-00-1F",  range_00_1f,  32u, 256ULL },
            { "Bits-1",       bits_1,        1u,  1ULL   },
            { "Bits-13",      bits_13,       2u, 13ULL   },
            { "AllZero-64B",  allzero_64b,  64u, 512ULL }
        };

        size_t i, j;
        for (i = 0; i < sizeof(digests) / sizeof(digests[0]); ++i) {
            int d = digests[i];
            size_t digest_bytes = (size_t)(d / 8);
            uint8_t* digest = (uint8_t*)malloc(digest_bytes);
            if (digest == NULL) {
                fprintf(stderr, "malloc failed for digest buffer\n");
                return 1;
            }

            printf("============================================================\n");
            printf("Vedak-%d\n", d);
            printf("============================================================\n\n");

            for (j = 0; j < sizeof(cases) / sizeof(cases[0]); ++j) {
                int rc = CryptHash(d,
                    (const unsigned char*)cases[j].msg,
                    cases[j].msg_bits,
                    digest);
                if (rc != 0) {
                    fprintf(stderr, "CryptHash failed on Vedak-%d, case = %s, rc = %d\n", d, cases[j].name, rc);
                    free(digest);
                    return 1;
                }

                printf("[Case %zu] %s\n", j + 1u, cases[j].name);
                printf("Msg_Len = %llu\n", cases[j].msg_bits);
                printf("Msg =\n");
                if (cases[j].msg_bytes == 0u) {
                    putchar('\n');
                }
                else {
                    print_hex_multiline_upper(cases[j].msg, cases[j].msg_bytes);
                }

                printf("Dst_Len = %d\n", d);
                printf("Dst =\n");
                print_hex_multiline_upper(digest, digest_bytes);
                putchar('\n');
            }

            free(digest);
        }
    }

    return 0;
}
