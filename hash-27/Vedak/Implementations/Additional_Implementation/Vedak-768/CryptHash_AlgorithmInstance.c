/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "CryptHash_AlgorithmInstance.h"


#define _CRT_SECURE_NO_WARNINGS
#include <immintrin.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(_MSC_VER)
#include <intrin.h>
#include <windows.h>
#define FINL __forceinline
#define RESTRICT __restrict
#define ALIGN64 __declspec(align(64))
#else
#include <x86intrin.h>
#define FINL __attribute__((always_inline)) inline
#define RESTRICT __restrict__
#define ALIGN64 __attribute__((aligned(64)))
#endif

#define CRYPTHASH_SUCCESS 0
#define CRYPTHASH_ERR_NULLPTR 1
#define CRYPTHASH_ERR_BAD_DIGEST_LEN 2
static const ALIGN64 uint8_t S512_BYTES[64] = {
    0x1,0x0,0x5,0x6,0xC,0x9,0x2,0x8,0xA,0x7,0x3,0xF,0xE,0xB,0x4,0xD,
    0x1,0x0,0x5,0x6,0xC,0x9,0x2,0x8,0xA,0x7,0x3,0xF,0xE,0xB,0x4,0xD,
    0x1,0x0,0x5,0x6,0xC,0x9,0x2,0x8,0xA,0x7,0x3,0xF,0xE,0xB,0x4,0xD,
    0x1,0x0,0x5,0x6,0xC,0x9,0x2,0x8,0xA,0x7,0x3,0xF,0xE,0xB,0x4,0xD
};

//static const ALIGN64 uint8_t S512_BYTES[64] = {
//    0x3,0x0,0x6,0x2,0x5,0x4,0xF,0xE,0xA,0x8,0x7,0x9,0x1,0xC,0xD,0xB,
//    0x3,0x0,0x6,0x2,0x5,0x4,0xF,0xE,0xA,0x8,0x7,0x9,0x1,0xC,0xD,0xB,
//    0x3,0x0,0x6,0x2,0x5,0x4,0xF,0xE,0xA,0x8,0x7,0x9,0x1,0xC,0xD,0xB,
//    0x3,0x0,0x6,0x2,0x5,0x4,0xF,0xE,0xA,0x8,0x7,0x9,0x1,0xC,0xD,0xB
//};

static const ALIGN64 uint8_t ROW1_BYTES[64] = {
    7,0,1,2,3,4,5,6,15,8,9,10,11,12,13,14, 7,0,1,2,3,4,5,6,15,8,9,10,11,12,13,14,
    7,0,1,2,3,4,5,6,15,8,9,10,11,12,13,14, 7,0,1,2,3,4,5,6,15,8,9,10,11,12,13,14
};
static const ALIGN64 uint8_t ROW2_BYTES[64] = {
    5,6,7,0,1,2,3,4,13,14,15,8,9,10,11,12, 5,6,7,0,1,2,3,4,13,14,15,8,9,10,11,12,
    5,6,7,0,1,2,3,4,13,14,15,8,9,10,11,12, 5,6,7,0,1,2,3,4,13,14,15,8,9,10,11,12
};
static const ALIGN64 uint8_t ROW4_BYTES[64] = {
    9,10,11,12,13,14,15,8,1,2,3,4,5,6,7,0, 9,10,11,12,13,14,15,8,1,2,3,4,5,6,7,0,
    9,10,11,12,13,14,15,8,1,2,3,4,5,6,7,0, 9,10,11,12,13,14,15,8,1,2,3,4,5,6,7,0
};
static const ALIGN64 uint8_t ROW6_BYTES[64] = {
    3,4,5,6,7,0,1,2,11,12,13,14,15,8,9,10, 11,12,13,14,15,8,9,10,3,4,5,6,7,0,1,2,
    3,4,5,6,7,0,1,2,11,12,13,14,15,8,9,10, 11,12,13,14,15,8,9,10,3,4,5,6,7,0,1,2
};
static const ALIGN64 uint8_t ROW7_BYTES[64] = {
    5,6,7,0,1,2,3,4,13,14,15,8,9,10,11,12, 13,14,15,8,9,10,11,12,5,6,7,0,1,2,3,4,
    5,6,7,0,1,2,3,4,13,14,15,8,9,10,11,12, 13,14,15,8,9,10,11,12,5,6,7,0,1,2,3,4
};
static const ALIGN64 uint8_t ROW8_BYTES[64] = {
    11,12,13,14,15,8,9,10,3,4,5,6,7,0,1,2, 3,4,5,6,7,0,1,2,11,12,13,14,15,8,9,10,
    11,12,13,14,15,8,9,10,3,4,5,6,7,0,1,2, 3,4,5,6,7,0,1,2,11,12,13,14,15,8,9,10
};
static const ALIGN64 uint8_t ROW9_BYTES[64] = {
    15,8,9,10,11,12,13,14,7,0,1,2,3,4,5,6, 7,0,1,2,3,4,5,6,15,8,9,10,11,12,13,14,
    15,8,9,10,11,12,13,14,7,0,1,2,3,4,5,6, 7,0,1,2,3,4,5,6,15,8,9,10,11,12,13,14
};

static const ALIGN64 uint64_t IDX_2301_QWORDS[8] = { 4,5,6,7,0,1,2,3 };
static const ALIGN64 uint64_t IDX_1032_QWORDS[8] = { 2,3,0,1,6,7,4,5 };
static const ALIGN64 uint64_t IDX_3210_QWORDS[8] = { 6,7,4,5,2,3,0,1 };

#define RCROW(a,b,c,d,e,f,g,h) {a,b,c,d,e,f,g,h}
static const ALIGN64 uint8_t RC8[40][8] = {
    RCROW(0x2,0xC,0x3,0x8,0x7,0xD,0x6,0x9),
    RCROW(0x9,0x8,0x8,0xC,0xC,0x9,0xD,0xD),
    RCROW(0xF,0x0,0xE,0x4,0xA,0x1,0xB,0x5),
    RCROW(0x2,0x1,0x3,0x5,0x7,0x0,0x6,0x4),
    RCROW(0x8,0x3,0x9,0x7,0xD,0x2,0xC,0x6),
    RCROW(0xC,0x7,0xD,0x3,0x9,0x6,0x8,0x2),
    RCROW(0x4,0xF,0x5,0xB,0x1,0xE,0x0,0xA),
    RCROW(0x5,0xE,0x4,0xA,0x0,0xF,0x1,0xB),
    RCROW(0x7,0xC,0x6,0x8,0x2,0xD,0x3,0x9),
    RCROW(0x3,0x9,0x2,0xD,0x6,0x8,0x7,0xC),
    RCROW(0xB,0x3,0xA,0x7,0xE,0x2,0xF,0x6),
    RCROW(0xA,0x7,0xB,0x3,0xF,0x6,0xE,0x2),
    RCROW(0x8,0xE,0x9,0xA,0xD,0xF,0xC,0xB),
    RCROW(0xD,0xC,0xC,0x8,0x8,0xD,0x9,0x9),
    RCROW(0x7,0x8,0x6,0xC,0x2,0x9,0x3,0xD),
    RCROW(0x3,0x0,0x2,0x4,0x6,0x1,0x7,0x5),
    RCROW(0xA,0x1,0xB,0x5,0xF,0x0,0xE,0x4),
    RCROW(0x8,0x2,0x9,0x6,0xD,0x3,0xC,0x7),
    RCROW(0xC,0x5,0xD,0x1,0x9,0x4,0x8,0x0),
    RCROW(0x4,0xA,0x5,0xE,0x1,0xB,0x0,0xF),
    RCROW(0x5,0x5,0x4,0x1,0x0,0x4,0x1,0x0),
    RCROW(0x6,0xB,0x7,0xF,0x3,0xA,0x2,0xE),
    RCROW(0x1,0x7,0x0,0x3,0x4,0x6,0x5,0x2),
    RCROW(0xE,0xF,0xF,0xB,0xB,0xE,0xA,0xA),
    RCROW(0x1,0xF,0x0,0xB,0x4,0xE,0x5,0xA),
    RCROW(0xF,0xF,0xE,0xB,0xA,0xE,0xB,0xA),
    RCROW(0x3,0xF,0x2,0xB,0x6,0xE,0x7,0xA),
    RCROW(0xB,0xF,0xA,0xB,0xE,0xE,0xF,0xA),
    RCROW(0xB,0xE,0xA,0xA,0xE,0xF,0xF,0xB),
    RCROW(0xB,0xC,0xA,0x8,0xE,0xD,0xF,0x9),
    RCROW(0xB,0x9,0xA,0xD,0xE,0x8,0xF,0xC),
    RCROW(0xB,0x2,0xA,0x6,0xE,0x3,0xF,0x7),
    RCROW(0xA,0x5,0xB,0x1,0xF,0x4,0xE,0x0),
    RCROW(0x8,0xB,0x9,0xF,0xD,0xA,0xC,0xE),
    RCROW(0xD,0x7,0xC,0x3,0x8,0x6,0x9,0x2),
    RCROW(0x6,0xF,0x7,0xB,0x3,0xE,0x2,0xA),
    RCROW(0x1,0xE,0x0,0xA,0x4,0xF,0x5,0xB),
    RCROW(0xF,0xD,0xE,0x9,0xA,0xC,0xB,0x8),
    RCROW(0x3,0xA,0x2,0xE,0x6,0xB,0x7,0xF),
    RCROW(0xB,0x4,0xA,0x0,0xE,0x5,0xF,0x1)
};
#undef RCROW

static FINL __m512i load_u8x64(const uint8_t* p) { return _mm512_load_si512((const void*)p); }
static FINL __m512i load_u64x8(const uint64_t* p) { return _mm512_load_si512((const void*)p); }

static FINL __m512i perm2301(__m512i x) {
    return _mm512_permutexvar_epi64(load_u64x8(IDX_2301_QWORDS), x);
}
static FINL __m512i perm1032(__m512i x) {
    return _mm512_permutexvar_epi64(load_u64x8(IDX_1032_QWORDS), x);
}
static FINL __m512i perm3210(__m512i x) {
    return _mm512_permutexvar_epi64(load_u64x8(IDX_3210_QWORDS), x);
}

static FINL __m512i expand32_to_nibbles_zmm(const uint8_t* RESTRICT src)
{
    const __m256i x8 = _mm256_loadu_si256((const __m256i*)src);
    const __m512i x16 = _mm512_cvtepu8_epi16(x8);
    const __m512i mask0f = _mm512_set1_epi16(0x000F);
    const __m512i hi = _mm512_and_si512(_mm512_srli_epi16(x16, 4), mask0f);
    const __m512i lo = _mm512_and_si512(x16, mask0f);
    return _mm512_or_si512(hi, _mm512_slli_epi16(lo, 8));
}

static FINL void dump_vec32bytes(uint8_t* RESTRICT dst32, __m512i v)
{
    ALIGN64 uint16_t words[32];
    int i;
    _mm512_store_si512((void*)words, v);
    for (i = 0; i < 32; ++i) {
        uint8_t hi = (uint8_t)(words[i] & 0x00FFu);
        uint8_t lo = (uint8_t)((words[i] >> 8) & 0x00FFu);
        dst32[i] = (uint8_t)((hi << 4) | lo);
    }
}

#define XO2T(a,b) do { t[(a)] = _mm512_xor_si512(t[(a)], t[(b)]); } while(0)

static FINL void vedak_round(__m512i st[10], int r)
{
    ALIGN64 __m512i t[10];
    const __m512i S = load_u8x64(S512_BYTES);
    const __m512i row1 = load_u8x64(ROW1_BYTES);
    const __m512i row2 = load_u8x64(ROW2_BYTES);
    const __m512i row4 = load_u8x64(ROW4_BYTES);
    const __m512i row6 = load_u8x64(ROW6_BYTES);
    const __m512i row7 = load_u8x64(ROW7_BYTES);
    const __m512i row8 = load_u8x64(ROW8_BYTES);
    const __m512i row9 = load_u8x64(ROW9_BYTES);
    int i;

    for (i = 0; i < 10; ++i) t[i] = _mm512_shuffle_epi8(S, st[i]);

    XO2T(5, 0); XO2T(6, 1); XO2T(7, 2); XO2T(8, 3); XO2T(9, 4);
    XO2T(0, 6); XO2T(1, 7); XO2T(2, 8); XO2T(3, 9); XO2T(4, 5);
    XO2T(5, 2); XO2T(6, 3); XO2T(7, 4); XO2T(8, 0); XO2T(9, 1);
    XO2T(0, 5); XO2T(1, 6); XO2T(2, 7); XO2T(3, 8); XO2T(4, 9);
    XO2T(5, 4); XO2T(6, 0); XO2T(7, 1); XO2T(8, 2); XO2T(9, 3);

    st[0] = _mm512_shuffle_epi32(perm2301(t[9]), (_MM_PERM_ENUM)0x1B);
    st[1] = _mm512_shuffle_epi8(perm1032(t[6]), row1);
    st[2] = _mm512_shuffle_epi8(perm3210(t[7]), row2);
    st[3] = _mm512_shuffle_epi32(perm3210(t[8]), (_MM_PERM_ENUM)0x4E);
    st[4] = _mm512_shuffle_epi8(perm1032(t[5]), row4);
    st[5] = t[4];
    st[6] = _mm512_shuffle_epi8(t[2], row6);
    st[7] = _mm512_shuffle_epi8(perm2301(t[1]), row7);
    st[8] = _mm512_shuffle_epi8(perm2301(t[0]), row8);
    st[9] = _mm512_shuffle_epi8(t[3], row9);

    st[0] = _mm512_xor_si512(st[0], _mm512_maskz_loadu_epi8((__mmask64)0x00000000000000FFULL, RC8[r]));
}
#undef XO2T

static FINL void vedak_p40(__m512i st[10])
{
    int r;
    for (r = 0; r < 40; ++r) vedak_round(st, r);
}

static FINL void absorb_r1536(__m512i st[10], const uint8_t* RESTRICT blk192)
{
    st[0] = _mm512_xor_si512(st[0], expand32_to_nibbles_zmm(blk192 + 0));
    st[1] = _mm512_xor_si512(st[1], expand32_to_nibbles_zmm(blk192 + 32));
    st[2] = _mm512_xor_si512(st[2], expand32_to_nibbles_zmm(blk192 + 64));
    st[3] = _mm512_xor_si512(st[3], expand32_to_nibbles_zmm(blk192 + 96));
    st[4] = _mm512_xor_si512(st[4], expand32_to_nibbles_zmm(blk192 + 128));
    st[5] = _mm512_xor_si512(st[5], expand32_to_nibbles_zmm(blk192 + 160));
}

static FINL void absorb_r1024(__m512i st[10], const uint8_t* RESTRICT blk128)
{
    st[0] = _mm512_xor_si512(st[0], expand32_to_nibbles_zmm(blk128 + 0));
    st[1] = _mm512_xor_si512(st[1], expand32_to_nibbles_zmm(blk128 + 32));
    st[2] = _mm512_xor_si512(st[2], expand32_to_nibbles_zmm(blk128 + 64));
    st[3] = _mm512_xor_si512(st[3], expand32_to_nibbles_zmm(blk128 + 96));
}

static FINL void absorb_r512(__m512i st[10], const uint8_t* RESTRICT blk64)
{
    st[0] = _mm512_xor_si512(st[0], expand32_to_nibbles_zmm(blk64 + 0));
    st[1] = _mm512_xor_si512(st[1], expand32_to_nibbles_zmm(blk64 + 32));
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
    if (full) memcpy(blk, tail, full);
    if (rem) {
        uint8_t mask = (uint8_t)(0xFFu << (8u - rem));
        blk[full] = (uint8_t)(tail[full] & mask);
    }
    blk[full] |= (uint8_t)(1u << (7u - rem));
    blk[rate_bytes - 1] |= 0x01u;
}

static FINL void zero_state10(__m512i st[10])
{
    __m512i z = _mm512_setzero_si512();
    int i;
    for (i = 0; i < 10; ++i) st[i] = z;
}

static void vedak512_hash(const uint8_t* RESTRICT msg, size_t msg_bits, uint8_t* RESTRICT out64)
{
    ALIGN64 __m512i st[10];
    const size_t rate_bytes = 192;
    const size_t msg_bytes_full = msg_bits >> 3;
    const size_t full_blocks = msg_bytes_full / rate_bytes;
    const size_t full_bytes = full_blocks * rate_bytes;
    const uint8_t* p = msg;
    ALIGN64 uint8_t last[192];
    size_t b;

    zero_state10(st);
    for (b = 0; b < full_blocks; ++b) {
        absorb_r1536(st, p);
        vedak_p40(st);
        p += rate_bytes;
    }

    {
        size_t tail_bits = msg_bits - (full_bytes << 3);
        if ((tail_bits & 7u) == 0) {
            size_t tail_bytes = tail_bits >> 3;
            memset(last, 0, sizeof(last));
            if (tail_bytes) memcpy(last, p, tail_bytes);
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
    }

    absorb_r1536(st, last);
    vedak_p40(st);
    dump_vec32bytes(out64 + 0, st[0]);
    dump_vec32bytes(out64 + 32, st[1]);
}

static void vedak768_hash(const uint8_t* RESTRICT msg, size_t msg_bits, uint8_t* RESTRICT out96)
{
    ALIGN64 __m512i st[10];
    const size_t rate_bytes = 128;
    const size_t msg_bytes_full = msg_bits >> 3;
    const size_t full_blocks = msg_bytes_full / rate_bytes;
    const size_t full_bytes = full_blocks * rate_bytes;
    const uint8_t* p = msg;
    ALIGN64 uint8_t last[128];
    size_t b;

    zero_state10(st);
    for (b = 0; b < full_blocks; ++b) {
        absorb_r1024(st, p);
        vedak_p40(st);
        p += rate_bytes;
    }

    {
        size_t tail_bits = msg_bits - (full_bytes << 3);
        if ((tail_bits & 7u) == 0) {
            size_t tail_bytes = tail_bits >> 3;
            memset(last, 0, sizeof(last));
            if (tail_bytes) memcpy(last, p, tail_bytes);
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
    }

    absorb_r1024(st, last);
    vedak_p40(st);
    dump_vec32bytes(out96 + 0, st[0]);
    dump_vec32bytes(out96 + 32, st[1]);
    dump_vec32bytes(out96 + 64, st[2]);
}

static void vedak1024_hash(const uint8_t* RESTRICT msg, size_t msg_bits, uint8_t* RESTRICT out128)
{
    ALIGN64 __m512i st[10];
    const size_t rate_bytes = 64;
    const size_t msg_bytes_full = msg_bits >> 3;
    const size_t full_blocks = msg_bytes_full / rate_bytes;
    const size_t full_bytes = full_blocks * rate_bytes;
    const uint8_t* p = msg;
    ALIGN64 uint8_t last[64];
    size_t b;

    zero_state10(st);
    for (b = 0; b < full_blocks; ++b) {
        absorb_r512(st, p);
        vedak_p40(st);
        p += rate_bytes;
    }

    {
        size_t tail_bits = msg_bits - (full_bytes << 3);
        if ((tail_bits & 7u) == 0) {
            size_t tail_bytes = tail_bits >> 3;
            memset(last, 0, sizeof(last));
            if (tail_bytes) memcpy(last, p, tail_bytes);
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
    }

    absorb_r512(st, last);
    vedak_p40(st);
    dump_vec32bytes(out128 + 0, st[0]);
    dump_vec32bytes(out128 + 32, st[1]);

    vedak_p40(st);
    dump_vec32bytes(out128 + 64, st[0]);
    dump_vec32bytes(out128 + 96, st[1]);
}

int CryptHash(int digest_len_bits, const unsigned char* msg, unsigned long long msg_len_bits, unsigned char* digest)
{
    if (digest == NULL) return CRYPTHASH_ERR_NULLPTR;
    if (msg_len_bits > 0 && msg == NULL) return CRYPTHASH_ERR_NULLPTR;

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
static __forceinline uint64_t read_tsc(void) { return __rdtsc(); }
static __forceinline uint64_t read_tscp(void) { unsigned aux; return __rdtscp(&aux); }
static int cpu_supports_avx512bw(void)
{
    int info[4];
    int osxsave, avx, avx512f, avx512bw;
    unsigned long long xcr0;

    __cpuidex(info, 0, 0);
    if (info[0] < 7) return 0;

    __cpuidex(info, 1, 0);
    osxsave = (info[2] & (1 << 27)) != 0;
    avx = (info[2] & (1 << 28)) != 0;
    if (!osxsave || !avx) return 0;

    xcr0 = _xgetbv(0);
    if ((xcr0 & 0xE6) != 0xE6) return 0;

    __cpuidex(info, 7, 0);
    avx512f = (info[1] & (1 << 16)) != 0;
    avx512bw = (info[1] & (1 << 30)) != 0;
    return avx512f && avx512bw;
}

static double wall_time_seconds(void)
{
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
}
#else
static inline void cpuid_serializing(void)
{
    unsigned a, b, c, d;
    a = 0;
    __asm__ __volatile__("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(a));
}
static inline uint64_t read_tsc(void) { return __rdtsc(); }
static inline uint64_t read_tscp(void) { unsigned aux; return __rdtscp(&aux); }
static int cpu_supports_avx512bw(void)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512bw");
#else
    return 0;
#endif
}

static double wall_time_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}
#endif

static size_t get_repeat_count(size_t msg_len_bytes)
{
    if (msg_len_bytes <= 64) return 300000;
    if (msg_len_bytes <= 4096) return 120000;
    if (msg_len_bytes <= 65536) return 30000;
    return 300;
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
        printf("%02x", (unsigned)digest[i]);
    }
}

static void bench_one(int digest_len_bits, size_t msg_len_bytes)
{
    size_t repeat = get_repeat_count(msg_len_bytes);
    size_t digest_len_bytes = (size_t)(digest_len_bits / 8);
    uint8_t* msg = NULL;
    uint8_t* digest = NULL;
    const unsigned char* msg_ptr;
    unsigned char* digest_ptr;
    uint8_t sink = 0;
    uint64_t c0, c1;
    double t0, t1, sec, total_bytes, total_bits, total_cycles, mbps, cyc_per_byte;
    size_t i;
    int rc;

    if (msg_len_bytes > 0) {
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

    msg_ptr = (msg_len_bytes == 0) ? NULL : (const unsigned char*)msg;
    digest_ptr = (unsigned char*)digest;

    for (i = 0; i < 8; ++i) {
        rc = CryptHash(digest_len_bits, msg_ptr, (unsigned long long)msg_len_bytes * 8ULL, digest_ptr);
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
        rc = CryptHash(digest_len_bits, msg_ptr, (unsigned long long)msg_len_bytes * 8ULL, digest_ptr);
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
    print_digest_head(digest, digest_len_bytes, 16);
    printf("\n");

    free(digest);
    free(msg);
}

int main_test(void)
{
    static const int digests[] = { 512, 768, 1024 };
    static const size_t msg_sizes[] = { 8, 32, 64, 256, 1024, 4096, 1048576 };
    size_t i, j;

    if (!cpu_supports_avx512bw()) {
        fprintf(stderr, "AVX-512F + AVX-512BW not supported.\n");
        return 1;
    }

    printf("%-8s%-12s%-10s%-14s%-14s%s\n",
        "Digest", "Msg(B)", "Repeat", "Mbps", "cyc/B", "DigestHead16");
    printf("------------------------------------------------------------------------\n");

    for (i = 0; i < sizeof(digests) / sizeof(digests[0]); ++i) {
        for (j = 0; j < sizeof(msg_sizes) / sizeof(msg_sizes[0]); ++j) {
            bench_one(digests[i], msg_sizes[j]);
        }
        printf("------------------------------------------------------------------------\n");
    }

    return 0;
}
