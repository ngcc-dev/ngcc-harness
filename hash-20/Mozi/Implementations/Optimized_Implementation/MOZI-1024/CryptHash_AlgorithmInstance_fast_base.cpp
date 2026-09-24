/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

This file keeps the MOZI-384 NGCC hash interface and provides a dispatching
2048-bit permutation implementation for SSE2, AVX2, and AVX-512.
*/

#if defined(__has_include)
#if __has_include("CryptHash_AlgorithmInstance.h")
extern "C" {
#include "CryptHash_AlgorithmInstance.h"
}
#endif
#endif

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(__AVX512F__) && defined(__AVX512VL__)
#include <immintrin.h>
#elif defined(__AVX2__)
#include <immintrin.h>
#elif defined(__SSE2__)
#include <emmintrin.h>
#else
#error "CryptHash_AlgorithmInstance_fast.cpp requires at least SSE2"
#endif

#define ULL unsigned long long

struct State2048 {
    alignas(64) unsigned char b[256]{};
};

static constexpr int rho[4][4] = {
    {0, 14, 20, 22},
    {0, 13, 68, 91},
    {0, 27, 42, 106},
    {0, 32, 48, 80}
};

static constexpr unsigned char RCON[24] = {
    0x24, 0x3f, 0x6a, 0x88, 0x85, 0xa3, 0x08, 0xd3,
    0x13, 0x19, 0x8a, 0x2e, 0x03, 0x70, 0x73, 0x44,
    0xa4, 0x09, 0x38, 0x22, 0x29, 0x9f, 0x31, 0xd0
};

#if defined(__SSE2__) || defined(__AVX2__) || (defined(__AVX512F__) && defined(__AVX512VL__))

static inline unsigned char *cell_ptr(State2048& s, int row, int col) {
    return s.b + (col * 4 + row) * 16;
}

static inline const unsigned char *cell_ptr(const State2048& s, int row, int col) {
    return s.b + (col * 4 + row) * 16;
}

static inline __m128i load_cell128(const State2048& s, int row, int col) {
    return _mm_loadu_si128(reinterpret_cast<const __m128i *>(cell_ptr(s, row, col)));
}

static inline void store_cell128(State2048& s, int row, int col, __m128i x) {
    _mm_storeu_si128(reinterpret_cast<__m128i *>(cell_ptr(s, row, col)), x);
}

static inline __m128i rotr128_sse(__m128i x, int n) {
    n &= 127;
    if (n == 0) {
        return x;
    }
    if (n >= 64) {
        x = _mm_shuffle_epi32(x, _MM_SHUFFLE(1, 0, 3, 2));
        n -= 64;
        if (n == 0) {
            return x;
        }
    }

    __m128i shifted = _mm_srli_epi64(x, n);
    __m128i shift_left = _mm_slli_epi64(x, 64 - n);
    __m128i swapped = _mm_shuffle_epi32(shift_left, _MM_SHUFFLE(1, 0, 3, 2));
    return _mm_or_si128(shifted, swapped);
}

static inline void sbox128(__m128i *in0, __m128i *in1, __m128i *in2, __m128i *in3) {
    __m128i out3 = _mm_xor_si128(_mm_and_si128(*in3, *in2), *in1);
    __m128i out1 = _mm_xor_si128(_mm_or_si128(*in2, *in1), *in0);
    __m128i out0 = _mm_xor_si128(_mm_and_si128(out3, *in0), *in3);
    __m128i out2 = _mm_xor_si128(_mm_and_si128(out1, *in3), *in2);

    *in0 = out0;
    *in1 = out1;
    *in2 = out2;
    *in3 = out3;
}

static inline void mul_row128(__m128i row[4]) {
    __m128i tmp = row[3];
    row[3] = row[2];
    row[2] = row[1];
    row[1] = _mm_xor_si128(tmp, row[0]);
    row[0] = tmp;
}

#endif

// -----------------------------------------------------------------------------
// SSE2
// -----------------------------------------------------------------------------

#if defined(__SSE2__)

static inline void load_sse(const State2048& state, __m128i s[4][4]) {
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            s[row][col] = load_cell128(state, row, col);
        }
    }
}

static inline void store_sse(State2048& state, __m128i s[4][4]) {
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            store_cell128(state, row, col, s[row][col]);
        }
    }
}

static inline void subbytes_sse(__m128i s[4][4]) {
    sbox128(&s[0][0], &s[0][1], &s[0][2], &s[0][3]);
    sbox128(&s[1][0], &s[1][1], &s[1][2], &s[1][3]);
    sbox128(&s[2][0], &s[2][1], &s[2][2], &s[2][3]);
    sbox128(&s[3][0], &s[3][1], &s[3][2], &s[3][3]);
}

static inline void xor_row_sse(__m128i row1[4], const __m128i row2[4]) {
    row1[0] = _mm_xor_si128(row1[0], row2[0]);
    row1[1] = _mm_xor_si128(row1[1], row2[1]);
    row1[2] = _mm_xor_si128(row1[2], row2[2]);
    row1[3] = _mm_xor_si128(row1[3], row2[3]);
}

static inline void mixcolumns_sse(__m128i s[4][4]) {
    xor_row_sse(s[2], s[3]);
    xor_row_sse(s[0], s[1]);
    mul_row128(s[1]);
    mul_row128(s[3]);
    xor_row_sse(s[1], s[2]);
    xor_row_sse(s[3], s[0]);
    mul_row128(s[0]);
    mul_row128(s[0]);
    mul_row128(s[2]);
    mul_row128(s[2]);
    xor_row_sse(s[2], s[3]);
    xor_row_sse(s[0], s[1]);
    xor_row_sse(s[1], s[2]);
    xor_row_sse(s[3], s[0]);
}

static inline void shiftrows_sse(__m128i s[4][4], unsigned subround) {
    for (int row = 1; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            s[row][col] = rotr128_sse(s[row][col], rho[subround][row]);
        }
    }
}

static void permute2048_sse(State2048& state, unsigned rounds) {
    __m128i s[4][4];
    load_sse(state, s);

    for (unsigned r = 0; r < rounds; r++) {
        subbytes_sse(s);
        mixcolumns_sse(s);
        shiftrows_sse(s, r & 3U);
        __m128i rc = _mm_cvtsi64_si128(static_cast<long long>(RCON[r]));
        s[3][3] = _mm_xor_si128(s[3][3], rc);
    }

    store_sse(state, s);
}

#endif

// -----------------------------------------------------------------------------
// AVX2
// -----------------------------------------------------------------------------

#if defined(__AVX2__)

static void permute2048_avx2(State2048& state, unsigned rounds) {
    __m128i s[4][4];
    load_sse(state, s);

    for (unsigned r = 0; r < rounds; r++) {
        subbytes_sse(s);
        mixcolumns_sse(s);
        shiftrows_sse(s, r & 3U);
        __m128i rc = _mm_cvtsi64_si128(static_cast<long long>(RCON[r]));
        s[3][3] = _mm_xor_si128(s[3][3], rc);
    }

    store_sse(state, s);
}

#endif

// -----------------------------------------------------------------------------
// AVX-512VL
// -----------------------------------------------------------------------------

#if defined(__AVX512F__) && defined(__AVX512VL__)

static inline void sbox128_avx512(__m128i *in0, __m128i *in1, __m128i *in2, __m128i *in3) {
    __m128i out3 = _mm_ternarylogic_epi64(*in3, *in2, *in1, 0x6a); // in1 ^ (in3 & in2)
    __m128i out1 = _mm_ternarylogic_epi64(*in2, *in1, *in0, 0x56); // in0 ^ (in2 | in1)
    __m128i out0 = _mm_ternarylogic_epi64(out3, *in0, *in3, 0x6a); // in3 ^ (out3 & in0)
    __m128i out2 = _mm_ternarylogic_epi64(out1, *in3, *in2, 0x6a); // in2 ^ (out1 & in3)

    *in0 = out0;
    *in1 = out1;
    *in2 = out2;
    *in3 = out3;
}

static inline void subbytes_avx512(__m128i s[4][4]) {
    sbox128_avx512(&s[0][0], &s[0][1], &s[0][2], &s[0][3]);
    sbox128_avx512(&s[1][0], &s[1][1], &s[1][2], &s[1][3]);
    sbox128_avx512(&s[2][0], &s[2][1], &s[2][2], &s[2][3]);
    sbox128_avx512(&s[3][0], &s[3][1], &s[3][2], &s[3][3]);
}

static void permute2048_avx512(State2048& state, unsigned rounds) {
    __m128i s[4][4];
    load_sse(state, s);

    for (unsigned r = 0; r < rounds; r++) {
        subbytes_avx512(s);
        mixcolumns_sse(s);
        shiftrows_sse(s, r & 3U);
        __m128i rc = _mm_cvtsi64_si128(static_cast<long long>(RCON[r]));
        s[3][3] = _mm_xor_si128(s[3][3], rc);
    }

    store_sse(state, s);
}

#endif

// -----------------------------------------------------------------------------
// Dispatcher
// -----------------------------------------------------------------------------

static void permute2048_dispatch(State2048& s, unsigned rounds) {
#if defined(__AVX512F__) && defined(__AVX512VL__)
    permute2048_avx512(s, rounds);
#elif defined(__AVX2__)
    permute2048_avx2(s, rounds);
#elif defined(__SSE2__)
    permute2048_sse(s, rounds);
#else
#error "CryptHash_AlgorithmInstance_fast.cpp requires at least SSE2"
#endif
}


// -----------------------------------------------------------------------------
// Standard NGCC-style interface
// -----------------------------------------------------------------------------

extern "C" void permutation(unsigned char *state, unsigned rounds) {
    State2048 s{};
    std::memcpy(s.b, state, sizeof(s.b));
    permute2048_dispatch(s, rounds);
    std::memcpy(state, s.b, sizeof(s.b));
}

extern "C" int hash_Fsponge(int hash, int rate, const unsigned char *input, ULL input_len_bits, int output_len_bits, unsigned char *output){
    if (rate != 120 || hash != 128){
        printf("Invalid rate or hash length\n");
        return 1;
    }
    unsigned char state[256];
    unsigned char IV[256];
    memset(IV, 0, sizeof(IV));
    memcpy(state, IV, sizeof(IV));
    // padding
    ULL padded_input_len_bits = ((input_len_bits + rate*8) / (rate*8)) * rate*8;
    ULL padded_input_len = padded_input_len_bits / 8;
    unsigned char *padded_input = (unsigned char *)malloc(padded_input_len);
    memset(padded_input, 0, padded_input_len);
    ULL full_bytes = input_len_bits / 8;
    int remaining_bits = input_len_bits % 8;
    memcpy(padded_input, input, full_bytes);
    if (remaining_bits > 0) {
        unsigned char mask = 0xFF << (8 - remaining_bits);
        padded_input[full_bytes] = input[full_bytes] & mask;
        padded_input[full_bytes] |= 0x80 >> remaining_bits; // padding with 0x80
    } else {
        padded_input[full_bytes] = 0x80; // padding with 0x80
    }
    // absorbing phase
    int blocks = padded_input_len / rate;
    unsigned char forward[256];
    memset(forward, 0, sizeof(forward));
    for (int i = 0; i < blocks; i++) {
        for (int j = 0; j < rate; j++) {
            state[j] ^= padded_input[i * rate + j];
        }
        if (i == blocks - 1) {
            state[rate] ^= 0x80; // domain separation
        }
        memcpy(forward + rate, state + rate, 256 - rate);
        permutation(state, 24);
        // feedforward
        for (int j = rate; j < 256; j++) {
            state[j] ^= forward[j];
        }
    }
    // squeezing phase
    for (int i = 0; i < hash; i ++){
        output[i] = state[256 - hash + i];
    }
    return 0;
}

extern "C" int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
    return hash_Fsponge(digest_len_bits / 8, 120, msg, msg_len_bits, digest_len_bits, digest);
}