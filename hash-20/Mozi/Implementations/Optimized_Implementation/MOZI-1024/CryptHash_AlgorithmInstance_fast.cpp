/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

MOZI-1024 feedforward-sponge implementation with named-register fast absorb loop.
Build selection:
  - AVX-512F + AVX-512VL: named-register AVX512VL S-box using VPTERNLOGQ.
  - AVX2 or SSE2: named-register SSE2 implementation.
The state layout is always the original 4x4x128-bit cell layout.
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

static constexpr unsigned char RCON[24] = {
    0x24, 0x3f, 0x6a, 0x88, 0x85, 0xa3, 0x08, 0xd3,
    0x13, 0x19, 0x8a, 0x2e, 0x03, 0x70, 0x73, 0x44,
    0xa4, 0x09, 0x38, 0x22, 0x29, 0x9f, 0x31, 0xd0
};

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

static inline __m128i swap64_128(__m128i x) {
    return _mm_shuffle_epi32(x, _MM_SHUFFLE(1, 0, 3, 2));
}

template<unsigned R>
static inline __m128i rotr128_const(__m128i x) {
    constexpr unsigned r = R & 127U;
    if constexpr (r == 0) {
        return x;
    } else if constexpr (r == 64) {
        return swap64_128(x);
    } else if constexpr (r < 64) {
        __m128i shifted = _mm_srli_epi64(x, r);
        __m128i shift_left = _mm_slli_epi64(x, 64 - r);
        __m128i swapped = swap64_128(shift_left);
        return _mm_or_si128(shifted, swapped);
    } else {
        constexpr unsigned k = r - 64U;
        __m128i y = swap64_128(x);
        __m128i shifted = _mm_srli_epi64(y, k);
        __m128i shift_left = _mm_slli_epi64(y, 64 - k);
        __m128i swapped = swap64_128(shift_left);
        return _mm_or_si128(shifted, swapped);
    }
}

static inline void sbox128_sse(__m128i& in0, __m128i& in1, __m128i& in2, __m128i& in3) {
    __m128i out3 = _mm_xor_si128(_mm_and_si128(in3, in2), in1);
    __m128i out1 = _mm_xor_si128(_mm_or_si128(in2, in1), in0);
    __m128i out0 = _mm_xor_si128(_mm_and_si128(out3, in0), in3);
    __m128i out2 = _mm_xor_si128(_mm_and_si128(out1, in3), in2);

    in0 = out0;
    in1 = out1;
    in2 = out2;
    in3 = out3;
}

#if defined(__AVX512F__) && defined(__AVX512VL__)
static inline void sbox128_avx512vl(__m128i& in0, __m128i& in1, __m128i& in2, __m128i& in3) {
    __m128i out3 = _mm_ternarylogic_epi64(in3, in2, in1, 0x6a);
    __m128i out1 = _mm_ternarylogic_epi64(in2, in1, in0, 0x56);
    __m128i out0 = _mm_ternarylogic_epi64(out3, in0, in3, 0x6a);
    __m128i out2 = _mm_ternarylogic_epi64(out1, in3, in2, 0x6a);

    in0 = out0;
    in1 = out1;
    in2 = out2;
    in3 = out3;
}
#endif

static inline void mul_row128_regs(__m128i& x0, __m128i& x1, __m128i& x2, __m128i& x3) {
    __m128i tmp = x3;
    x3 = x2;
    x2 = x1;
    x1 = _mm_xor_si128(tmp, x0);
    x0 = tmp;
}

struct Regs128 {
    __m128i x00, x01, x02, x03;
    __m128i x10, x11, x12, x13;
    __m128i x20, x21, x22, x23;
    __m128i x30, x31, x32, x33;
};

static inline void load_regs128(const State2048& state, Regs128& x) {
    x.x00 = load_cell128(state, 0, 0);
    x.x01 = load_cell128(state, 0, 1);
    x.x02 = load_cell128(state, 0, 2);
    x.x03 = load_cell128(state, 0, 3);
    x.x10 = load_cell128(state, 1, 0);
    x.x11 = load_cell128(state, 1, 1);
    x.x12 = load_cell128(state, 1, 2);
    x.x13 = load_cell128(state, 1, 3);
    x.x20 = load_cell128(state, 2, 0);
    x.x21 = load_cell128(state, 2, 1);
    x.x22 = load_cell128(state, 2, 2);
    x.x23 = load_cell128(state, 2, 3);
    x.x30 = load_cell128(state, 3, 0);
    x.x31 = load_cell128(state, 3, 1);
    x.x32 = load_cell128(state, 3, 2);
    x.x33 = load_cell128(state, 3, 3);
}

static inline void store_regs128(State2048& state, const Regs128& x) {
    store_cell128(state, 0, 0, x.x00);
    store_cell128(state, 0, 1, x.x01);
    store_cell128(state, 0, 2, x.x02);
    store_cell128(state, 0, 3, x.x03);
    store_cell128(state, 1, 0, x.x10);
    store_cell128(state, 1, 1, x.x11);
    store_cell128(state, 1, 2, x.x12);
    store_cell128(state, 1, 3, x.x13);
    store_cell128(state, 2, 0, x.x20);
    store_cell128(state, 2, 1, x.x21);
    store_cell128(state, 2, 2, x.x22);
    store_cell128(state, 2, 3, x.x23);
    store_cell128(state, 3, 0, x.x30);
    store_cell128(state, 3, 1, x.x31);
    store_cell128(state, 3, 2, x.x32);
    store_cell128(state, 3, 3, x.x33);
}

#if defined(__AVX512F__) && defined(__AVX512VL__)
static inline void subbytes_regs128_avx512vl(Regs128& x) {
    sbox128_avx512vl(x.x00, x.x01, x.x02, x.x03);
    sbox128_avx512vl(x.x10, x.x11, x.x12, x.x13);
    sbox128_avx512vl(x.x20, x.x21, x.x22, x.x23);
    sbox128_avx512vl(x.x30, x.x31, x.x32, x.x33);
}
#endif

static inline void subbytes_regs128_sse(Regs128& x) {
    sbox128_sse(x.x00, x.x01, x.x02, x.x03);
    sbox128_sse(x.x10, x.x11, x.x12, x.x13);
    sbox128_sse(x.x20, x.x21, x.x22, x.x23);
    sbox128_sse(x.x30, x.x31, x.x32, x.x33);
}

static inline void mixcolumns_regs128(Regs128& x) {
    x.x20 = _mm_xor_si128(x.x20, x.x30);
    x.x21 = _mm_xor_si128(x.x21, x.x31);
    x.x22 = _mm_xor_si128(x.x22, x.x32);
    x.x23 = _mm_xor_si128(x.x23, x.x33);
    x.x00 = _mm_xor_si128(x.x00, x.x10);
    x.x01 = _mm_xor_si128(x.x01, x.x11);
    x.x02 = _mm_xor_si128(x.x02, x.x12);
    x.x03 = _mm_xor_si128(x.x03, x.x13);
    mul_row128_regs(x.x10, x.x11, x.x12, x.x13);
    mul_row128_regs(x.x30, x.x31, x.x32, x.x33);
    x.x10 = _mm_xor_si128(x.x10, x.x20);
    x.x11 = _mm_xor_si128(x.x11, x.x21);
    x.x12 = _mm_xor_si128(x.x12, x.x22);
    x.x13 = _mm_xor_si128(x.x13, x.x23);
    x.x30 = _mm_xor_si128(x.x30, x.x00);
    x.x31 = _mm_xor_si128(x.x31, x.x01);
    x.x32 = _mm_xor_si128(x.x32, x.x02);
    x.x33 = _mm_xor_si128(x.x33, x.x03);
    mul_row128_regs(x.x00, x.x01, x.x02, x.x03);
    mul_row128_regs(x.x00, x.x01, x.x02, x.x03);
    mul_row128_regs(x.x20, x.x21, x.x22, x.x23);
    mul_row128_regs(x.x20, x.x21, x.x22, x.x23);
    x.x20 = _mm_xor_si128(x.x20, x.x30);
    x.x21 = _mm_xor_si128(x.x21, x.x31);
    x.x22 = _mm_xor_si128(x.x22, x.x32);
    x.x23 = _mm_xor_si128(x.x23, x.x33);
    x.x00 = _mm_xor_si128(x.x00, x.x10);
    x.x01 = _mm_xor_si128(x.x01, x.x11);
    x.x02 = _mm_xor_si128(x.x02, x.x12);
    x.x03 = _mm_xor_si128(x.x03, x.x13);
    x.x10 = _mm_xor_si128(x.x10, x.x20);
    x.x11 = _mm_xor_si128(x.x11, x.x21);
    x.x12 = _mm_xor_si128(x.x12, x.x22);
    x.x13 = _mm_xor_si128(x.x13, x.x23);
    x.x30 = _mm_xor_si128(x.x30, x.x00);
    x.x31 = _mm_xor_si128(x.x31, x.x01);
    x.x32 = _mm_xor_si128(x.x32, x.x02);
    x.x33 = _mm_xor_si128(x.x33, x.x03);
}

template<unsigned R>
static inline void rotr_row_regs128(__m128i& x0, __m128i& x1, __m128i& x2, __m128i& x3) {
    x0 = rotr128_const<R>(x0);
    x1 = rotr128_const<R>(x1);
    x2 = rotr128_const<R>(x2);
    x3 = rotr128_const<R>(x3);
}

static inline void shiftrows_regs128(Regs128& x, unsigned subround) {
    switch (subround & 3U) {
    case 0:
        rotr_row_regs128<14>(x.x10, x.x11, x.x12, x.x13);
        rotr_row_regs128<20>(x.x20, x.x21, x.x22, x.x23);
        rotr_row_regs128<22>(x.x30, x.x31, x.x32, x.x33);
        break;
    case 1:
        rotr_row_regs128<13>(x.x10, x.x11, x.x12, x.x13);
        rotr_row_regs128<68>(x.x20, x.x21, x.x22, x.x23);
        rotr_row_regs128<91>(x.x30, x.x31, x.x32, x.x33);
        break;
    case 2:
        rotr_row_regs128<27>(x.x10, x.x11, x.x12, x.x13);
        rotr_row_regs128<42>(x.x20, x.x21, x.x22, x.x23);
        rotr_row_regs128<106>(x.x30, x.x31, x.x32, x.x33);
        break;
    default:
        rotr_row_regs128<32>(x.x10, x.x11, x.x12, x.x13);
        rotr_row_regs128<48>(x.x20, x.x21, x.x22, x.x23);
        rotr_row_regs128<80>(x.x30, x.x31, x.x32, x.x33);
        break;
    }
}

static inline void add_constant_regs128(Regs128& x, unsigned round) {
    __m128i rc = _mm_cvtsi64_si128(static_cast<long long>(RCON[round]));
    x.x33 = _mm_xor_si128(x.x33, rc);
}

#if defined(__AVX512F__) && defined(__AVX512VL__)
static inline void permute2048_regs128_avx512vl(Regs128& x, unsigned rounds) {
    for (unsigned r = 0; r < rounds; r++) {
        subbytes_regs128_avx512vl(x);
        mixcolumns_regs128(x);
        shiftrows_regs128(x, r & 3U);
        add_constant_regs128(x, r);
    }
}
#endif

static inline void permute2048_regs128_sse(Regs128& x, unsigned rounds) {
    for (unsigned r = 0; r < rounds; r++) {
        subbytes_regs128_sse(x);
        mixcolumns_regs128(x);
        shiftrows_regs128(x, r & 3U);
        add_constant_regs128(x, r);
    }
}

static void permute2048_dispatch(State2048& s, unsigned rounds) {
    Regs128 x;
    load_regs128(s, x);
#if defined(__AVX512F__) && defined(__AVX512VL__)
    permute2048_regs128_avx512vl(x, rounds);
#elif defined(__AVX2__)
    permute2048_regs128_sse(x, rounds);
#elif defined(__SSE2__)
    permute2048_regs128_sse(x, rounds);
#else
#error "CryptHash_AlgorithmInstance_fast.cpp requires at least SSE2"
#endif
    store_regs128(s, x);
}


static inline __m128i high64_mask128() {
    return _mm_set_epi64x(static_cast<long long>(0xffffffffffffffffULL), 0LL);
}

static inline __m128i domain_sep_high64_mask128() {
    return _mm_set_epi64x(0x80LL, 0LL);
}

static inline void absorb_block120_regs128(Regs128& x, const unsigned char *block) {
    x.x00 = _mm_xor_si128(x.x00, _mm_loadu_si128(reinterpret_cast<const __m128i *>(block +   0)));
    x.x10 = _mm_xor_si128(x.x10, _mm_loadu_si128(reinterpret_cast<const __m128i *>(block +  16)));
    x.x20 = _mm_xor_si128(x.x20, _mm_loadu_si128(reinterpret_cast<const __m128i *>(block +  32)));
    x.x30 = _mm_xor_si128(x.x30, _mm_loadu_si128(reinterpret_cast<const __m128i *>(block +  48)));
    x.x01 = _mm_xor_si128(x.x01, _mm_loadu_si128(reinterpret_cast<const __m128i *>(block +  64)));
    x.x11 = _mm_xor_si128(x.x11, _mm_loadu_si128(reinterpret_cast<const __m128i *>(block +  80)));
    x.x21 = _mm_xor_si128(x.x21, _mm_loadu_si128(reinterpret_cast<const __m128i *>(block +  96)));
    x.x31 = _mm_xor_si128(x.x31, _mm_loadl_epi64(reinterpret_cast<const __m128i *>(block + 112)));
}

static inline void fsponge_permute_feedforward_regs128(Regs128& x, unsigned rounds, bool last_block) {
    const __m128i hi_mask = high64_mask128();

    if (last_block) {
        x.x31 = _mm_xor_si128(x.x31, domain_sep_high64_mask128());
    }

    __m128i f31 = _mm_and_si128(x.x31, hi_mask);
    __m128i f02 = x.x02;
    __m128i f12 = x.x12;
    __m128i f22 = x.x22;
    __m128i f32 = x.x32;
    __m128i f03 = x.x03;
    __m128i f13 = x.x13;
    __m128i f23 = x.x23;
    __m128i f33 = x.x33;

#if defined(__AVX512F__) && defined(__AVX512VL__)
    permute2048_regs128_avx512vl(x, rounds);
#elif defined(__AVX2__)
    permute2048_regs128_sse(x, rounds);
#elif defined(__SSE2__)
    permute2048_regs128_sse(x, rounds);
#else
#error "CryptHash_AlgorithmInstance_fast.cpp requires at least SSE2"
#endif

    x.x31 = _mm_xor_si128(x.x31, f31);
    x.x02 = _mm_xor_si128(x.x02, f02);
    x.x12 = _mm_xor_si128(x.x12, f12);
    x.x22 = _mm_xor_si128(x.x22, f22);
    x.x32 = _mm_xor_si128(x.x32, f32);
    x.x03 = _mm_xor_si128(x.x03, f03);
    x.x13 = _mm_xor_si128(x.x13, f13);
    x.x23 = _mm_xor_si128(x.x23, f23);
    x.x33 = _mm_xor_si128(x.x33, f33);
}

static inline void fast_absorb_fblocks120_dispatch(State2048& state,
                                                   const unsigned char *blocks,
                                                   ULL block_count,
                                                   unsigned rounds) {
    Regs128 x;
    load_regs128(state, x);

    for (ULL i = 0; i < block_count; i++) {
        absorb_block120_regs128(x, blocks + i * 120ULL);
        fsponge_permute_feedforward_regs128(x, rounds, false);
    }

    store_regs128(state, x);
}

static inline void fast_absorb_fblock120_last_dispatch(State2048& state,
                                                       const unsigned char block[120],
                                                       unsigned rounds) {
    Regs128 x;
    load_regs128(state, x);
    absorb_block120_regs128(x, block);
    fsponge_permute_feedforward_regs128(x, rounds, true);
    store_regs128(state, x);
}

extern "C" void permutation(unsigned char *state, unsigned rounds) {
    State2048 s{};
    std::memcpy(s.b, state, sizeof(s.b));
    permute2048_dispatch(s, rounds);
    std::memcpy(state, s.b, sizeof(s.b));
}

extern "C" int hash_Fsponge(int hash,
                             int rate,
                             const unsigned char *input,
                             ULL input_len_bits,
                             int output_len_bits,
                             unsigned char *output) {
    (void)output_len_bits;

    if (rate != 120 || hash != 128) {
        std::printf("Invalid rate or hash length\n");
        return 1;
    }
    if (output == nullptr || (input == nullptr && input_len_bits != 0)) {
        return 1;
    }

    State2048 state{};

    const ULL rate_bits = 120ULL * 8ULL;
    const ULL full_blocks = input_len_bits / rate_bits;
    const ULL rem_bits = input_len_bits % rate_bits;

    if (full_blocks != 0) {
        fast_absorb_fblocks120_dispatch(state, input, full_blocks, 24);
    }

    alignas(64) unsigned char last_block[120];
    std::memset(last_block, 0, sizeof(last_block));

    const unsigned char *tail = (input != nullptr) ? (input + full_blocks * 120ULL) : nullptr;
    const ULL rem_full_bytes = rem_bits / 8ULL;
    const int rem_extra_bits = static_cast<int>(rem_bits & 7ULL);

    if (rem_full_bytes != 0) {
        std::memcpy(last_block, tail, static_cast<size_t>(rem_full_bytes));
    }

    if (rem_extra_bits > 0) {
        unsigned char mask = static_cast<unsigned char>(0xFFU << (8 - rem_extra_bits));
        last_block[rem_full_bytes] = tail[rem_full_bytes] & mask;
        last_block[rem_full_bytes] |= static_cast<unsigned char>(0x80U >> rem_extra_bits);
    } else {
        last_block[rem_full_bytes] = 0x80;
    }

    fast_absorb_fblock120_last_dispatch(state, last_block, 24);

    std::memcpy(output, state.b + (256 - 128), 128);
    return 0;
}

extern "C" int CryptHash(int digest_len_bits,
                          const unsigned char *msg,
                          unsigned long long msg_len_bits,
                          unsigned char *digest) {
    return hash_Fsponge(digest_len_bits / 8, 120, msg, msg_len_bits, digest_len_bits, digest);
}
