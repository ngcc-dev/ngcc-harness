/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

This file combines the fast MEGASCON-2048 permutation implementation from
megascon_f.cpp with the standard NGCC hash interface implementation.
*/

extern "C" {
#include "CryptHash_AlgorithmInstance.h"
}

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define ULL unsigned long long


struct State2048 {
    alignas(64) uint64_t w[8][4]{};
};

static constexpr unsigned BIT_PERM[8] = {1, 7, 5, 2, 4, 6, 0, 3};

static constexpr unsigned A[8] = { 1,  2,  4,  7, 26, 49, 55, 58 };
static constexpr unsigned B[8] = { 3,  5, 15,  9, 46,  6, 19, 47 };
static constexpr unsigned C[8] = {14, 23, 17, 12, 13, 50, 56, 61 };
static constexpr unsigned D[8] = {36, 60, 24, 40, 37, 25, 57, 28 };

static inline uint64_t rotl64(uint64_t x, unsigned r) {
    r &= 63;
    return r ? ((x << r) | (x >> (64 - r))) : x;
}

static inline uint64_t round_constant(unsigned round) {
    uint64_t x = 0x9e3779b97f4a7c15ULL ^ uint64_t(round);
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

// -----------------------------------------------------------------------------
// Scalar
// -----------------------------------------------------------------------------

static void chichi_scalar(State2048& s) {
    uint64_t x[8][4];
    std::memcpy(x, s.w, sizeof(x));

    for (int j = 0; j < 4; j++) {
        s.w[0][j] = x[0][j] ^ (~x[1][j] & x[2][j]);
        s.w[1][j] = x[4][j] ^ (~x[2][j] & x[0][j]);
        s.w[2][j] = x[3][j] ^ (~x[0][j] & x[1][j]);
        s.w[3][j] = ~x[1][j] ^ (~x[4][j] & ~x[5][j]);
        s.w[4][j] = x[2][j] ^ (~x[5][j] & x[6][j]);
        s.w[5][j] = x[5][j] ^ (~x[6][j] & x[7][j]);
        s.w[6][j] = x[6][j] ^ (~x[7][j] & x[3][j]);
        s.w[7][j] = x[7][j] ^ (~x[3][j] & x[4][j]);
    }
}

static void bitperm_scalar(State2048& s) {
    uint64_t t[8][4];

    for (int old = 0; old < 8; old++) {
        int nw = BIT_PERM[old];
        for (int j = 0; j < 4; j++) {
            t[nw][j] = s.w[old][j];
        }
    }

    std::memcpy(s.w, t, sizeof(t));
}

template<unsigned a, unsigned b, unsigned c, unsigned d>
static void row_mix_one_scalar(uint64_t x[4]) {
    uint64_t t[4] = {x[0], x[1], x[2], x[3]};

    x[0] = t[0] ^ rotl64(t[1], a) ^ rotl64(t[2], b) ^ rotl64(t[3], c) ^ rotl64(t[0], d);
    x[1] = t[1] ^ rotl64(t[2], a) ^ rotl64(t[3], b) ^ rotl64(t[0], c) ^ rotl64(t[1], d);
    x[2] = t[2] ^ rotl64(t[3], a) ^ rotl64(t[0], b) ^ rotl64(t[1], c) ^ rotl64(t[2], d);
    x[3] = t[3] ^ rotl64(t[0], a) ^ rotl64(t[1], b) ^ rotl64(t[2], c) ^ rotl64(t[3], d);
}

static void row_mix_scalar(State2048& s) {
    row_mix_one_scalar<A[0], B[0], C[0], D[0]>(s.w[0]);
    row_mix_one_scalar<A[1], B[1], C[1], D[1]>(s.w[1]);
    row_mix_one_scalar<A[2], B[2], C[2], D[2]>(s.w[2]);
    row_mix_one_scalar<A[3], B[3], C[3], D[3]>(s.w[3]);
    row_mix_one_scalar<A[4], B[4], C[4], D[4]>(s.w[4]);
    row_mix_one_scalar<A[5], B[5], C[5], D[5]>(s.w[5]);
    row_mix_one_scalar<A[6], B[6], C[6], D[6]>(s.w[6]);
    row_mix_one_scalar<A[7], B[7], C[7], D[7]>(s.w[7]);
}

static void add_constant_scalar(State2048& s, unsigned round) {
    s.w[0][0] ^= round_constant(round);
}

static void permute2048_scalar(State2048& s, unsigned rounds) {
    for (unsigned r = 0; r < rounds; r++) {
        chichi_scalar(s);
        bitperm_scalar(s);
        row_mix_scalar(s);
        add_constant_scalar(s, r);
    }
}

// -----------------------------------------------------------------------------
// SSSE3
// -----------------------------------------------------------------------------

#if defined(__SSSE3__)
#include <immintrin.h>

struct Vec256_SSE {
    __m128i lo;
    __m128i hi;
};

static inline __m128i allones128() {
    return _mm_cmpeq_epi32(_mm_setzero_si128(), _mm_setzero_si128());
}

static inline Vec256_SSE load_row_sse(const State2048& s, int r) {
    return {
        _mm_load_si128(reinterpret_cast<const __m128i*>(&s.w[r][0])),
        _mm_load_si128(reinterpret_cast<const __m128i*>(&s.w[r][2]))
    };
}

static inline void store_row_sse(State2048& s, int r, Vec256_SSE x) {
    _mm_store_si128(reinterpret_cast<__m128i*>(&s.w[r][0]), x.lo);
    _mm_store_si128(reinterpret_cast<__m128i*>(&s.w[r][2]), x.hi);
}

static inline Vec256_SSE xor256(Vec256_SSE a, Vec256_SSE b) {
    return {_mm_xor_si128(a.lo, b.lo), _mm_xor_si128(a.hi, b.hi)};
}

static inline Vec256_SSE andnot256(Vec256_SSE a, Vec256_SSE b) {
    return {_mm_andnot_si128(a.lo, b.lo), _mm_andnot_si128(a.hi, b.hi)};
}

static inline Vec256_SSE not256(Vec256_SSE x) {
    __m128i m = allones128();
    return {_mm_xor_si128(x.lo, m), _mm_xor_si128(x.hi, m)};
}

template<unsigned r>
static inline __m128i rotl64_128(__m128i x) {
    if constexpr ((r & 63) == 0) {
        return x;
    } else {
        return _mm_or_si128(
            _mm_slli_epi64(x, r & 63),
            _mm_srli_epi64(x, 64 - (r & 63))
        );
    }
}

template<unsigned r>
static inline Vec256_SSE rotl64_256_sse(Vec256_SSE x) {
    return {rotl64_128<r>(x.lo), rotl64_128<r>(x.hi)};
}

static inline Vec256_SSE word_rot1_sse(Vec256_SSE x) {
#if defined(__SSSE3__)
    return {
        _mm_alignr_epi8(x.hi, x.lo, 8),
        _mm_alignr_epi8(x.lo, x.hi, 8)
    };
#else
    return {
        _mm_unpackhi_epi64(x.lo, x.hi),
        _mm_unpackhi_epi64(x.hi, x.lo)
    };
#endif
}

static inline Vec256_SSE word_rot2_sse(Vec256_SSE x) {
    return {x.hi, x.lo};
}

static inline Vec256_SSE word_rot3_sse(Vec256_SSE x) {
#if defined(__SSSE3__)
    return {
        _mm_alignr_epi8(x.lo, x.hi, 8),
        _mm_alignr_epi8(x.hi, x.lo, 8)
    };
#else
    return {
        _mm_unpackhi_epi64(x.hi, x.lo),
        _mm_unpackhi_epi64(x.lo, x.hi)
    };
#endif
}

template<unsigned a, unsigned b, unsigned c, unsigned d>
static inline Vec256_SSE row_mix_one_sse(Vec256_SSE x) {
    Vec256_SSE x1 = word_rot1_sse(x);
    Vec256_SSE x2 = word_rot2_sse(x);
    Vec256_SSE x3 = word_rot3_sse(x);
    Vec256_SSE xd = rotl64_256_sse<d>(x);

    return xor256(
        xor256(x, xd),
        xor256(
            rotl64_256_sse<a>(x1),
            xor256(rotl64_256_sse<b>(x2), rotl64_256_sse<c>(x3))
        )
    );
}

static inline void chichi_sse_regs(Vec256_SSE row[8]) {
    Vec256_SSE x0 = row[0], x1 = row[1], x2 = row[2], x3 = row[3];
    Vec256_SSE x4 = row[4], x5 = row[5], x6 = row[6], x7 = row[7];

    row[BIT_PERM[0]] = xor256(x0, andnot256(x1, x2));
    row[BIT_PERM[1]] = xor256(x4, andnot256(x2, x0));
    row[BIT_PERM[2]] = xor256(x3, andnot256(x0, x1));
    row[BIT_PERM[3]] = xor256(not256(x1), andnot256(x4, not256(x5)));
    row[BIT_PERM[4]] = xor256(x2, andnot256(x5, x6));
    row[BIT_PERM[5]] = xor256(x5, andnot256(x6, x7));
    row[BIT_PERM[6]] = xor256(x6, andnot256(x7, x3));
    row[BIT_PERM[7]] = xor256(x7, andnot256(x3, x4));
}


static inline void row_mix_sse_regs(Vec256_SSE row[8]) {
    row[0] = row_mix_one_sse<A[0], B[0], C[0], D[0]>(row[0]);
    row[1] = row_mix_one_sse<A[1], B[1], C[1], D[1]>(row[1]);
    row[2] = row_mix_one_sse<A[2], B[2], C[2], D[2]>(row[2]);
    row[3] = row_mix_one_sse<A[3], B[3], C[3], D[3]>(row[3]);
    row[4] = row_mix_one_sse<A[4], B[4], C[4], D[4]>(row[4]);
    row[5] = row_mix_one_sse<A[5], B[5], C[5], D[5]>(row[5]);
    row[6] = row_mix_one_sse<A[6], B[6], C[6], D[6]>(row[6]);
    row[7] = row_mix_one_sse<A[7], B[7], C[7], D[7]>(row[7]);
}

static inline void add_constant_sse_regs(Vec256_SSE row[8], unsigned round) {
    row[0].lo = _mm_xor_si128(row[0].lo, _mm_set_epi64x(0, round_constant(round)));
}

static inline void permute2048_sse_regs(Vec256_SSE row[8], unsigned rounds) {
    for (unsigned r = 0; r < rounds; r++) {
        chichi_sse_regs(row);
        row_mix_sse_regs(row);
        add_constant_sse_regs(row, r);
    }
}

static void permute2048_sse(State2048& s, unsigned rounds) {
    Vec256_SSE row[8];
    for (int r = 0; r < 8; r++) row[r] = load_row_sse(s, r);
    permute2048_sse_regs(row, rounds);
    for (int r = 0; r < 8; r++) store_row_sse(s, r, row[r]);
}
#endif

// -----------------------------------------------------------------------------
// AVX2
// -----------------------------------------------------------------------------

#if defined(__AVX2__)
#include <immintrin.h>

template<unsigned r>
static inline __m256i rotl64_256_avx2(__m256i x) {
    if constexpr ((r & 63) == 0) {
        return x;
    } else {
        return _mm256_or_si256(
            _mm256_slli_epi64(x, r & 63),
            _mm256_srli_epi64(x, 64 - (r & 63))
        );
    }
}

template<int imm>
static inline __m256i perm64_avx2(__m256i x) {
    return _mm256_permute4x64_epi64(x, imm);
}

template<unsigned a, unsigned b, unsigned c, unsigned d>
static inline __m256i row_mix_one_avx2(__m256i x) {
    __m256i x1 = perm64_avx2<0x39>(x);
    __m256i x2 = perm64_avx2<0x4e>(x);
    __m256i x3 = perm64_avx2<0x93>(x);
    __m256i xd = rotl64_256_avx2<d>(x);

    return _mm256_xor_si256(
        _mm256_xor_si256(x, xd),
        _mm256_xor_si256(
            rotl64_256_avx2<a>(x1),
            _mm256_xor_si256(rotl64_256_avx2<b>(x2), rotl64_256_avx2<c>(x3))
        )
    );
}

static inline void chichi_avx2_regs(__m256i row[8]) {
    __m256i x0 = row[0], x1 = row[1], x2 = row[2], x3 = row[3];
    __m256i x4 = row[4], x5 = row[5], x6 = row[6], x7 = row[7];

    const __m256i all1 =
        _mm256_cmpeq_epi32(_mm256_setzero_si256(), _mm256_setzero_si256());

    row[BIT_PERM[0]] = _mm256_xor_si256(x0, _mm256_andnot_si256(x1, x2));
    row[BIT_PERM[1]] = _mm256_xor_si256(x4, _mm256_andnot_si256(x2, x0));
    row[BIT_PERM[2]] = _mm256_xor_si256(x3, _mm256_andnot_si256(x0, x1));

    __m256i nx1 = _mm256_xor_si256(x1, all1);
    __m256i nx5 = _mm256_xor_si256(x5, all1);
    row[BIT_PERM[3]] = _mm256_xor_si256(nx1, _mm256_andnot_si256(x4, nx5));

    row[BIT_PERM[4]] = _mm256_xor_si256(x2, _mm256_andnot_si256(x5, x6));
    row[BIT_PERM[5]] = _mm256_xor_si256(x5, _mm256_andnot_si256(x6, x7));
    row[BIT_PERM[6]] = _mm256_xor_si256(x6, _mm256_andnot_si256(x7, x3));
    row[BIT_PERM[7]] = _mm256_xor_si256(x7, _mm256_andnot_si256(x3, x4));
}


static inline void row_mix_avx2_regs(__m256i row[8]) {
    row[0] = row_mix_one_avx2<A[0], B[0], C[0], D[0]>(row[0]);
    row[1] = row_mix_one_avx2<A[1], B[1], C[1], D[1]>(row[1]);
    row[2] = row_mix_one_avx2<A[2], B[2], C[2], D[2]>(row[2]);
    row[3] = row_mix_one_avx2<A[3], B[3], C[3], D[3]>(row[3]);
    row[4] = row_mix_one_avx2<A[4], B[4], C[4], D[4]>(row[4]);
    row[5] = row_mix_one_avx2<A[5], B[5], C[5], D[5]>(row[5]);
    row[6] = row_mix_one_avx2<A[6], B[6], C[6], D[6]>(row[6]);
    row[7] = row_mix_one_avx2<A[7], B[7], C[7], D[7]>(row[7]);
}

static inline void add_constant_avx2_regs(__m256i row[8], unsigned round) {
    row[0] = _mm256_xor_si256(row[0], _mm256_set_epi64x(0, 0, 0, round_constant(round)));
}

static inline void permute2048_avx2_regs(__m256i row[8], unsigned rounds) {
    for (unsigned r = 0; r < rounds; r++) {
        chichi_avx2_regs(row);
        row_mix_avx2_regs(row);
        add_constant_avx2_regs(row, r);
    }
}

static void permute2048_avx2(State2048& s, unsigned rounds) {
    __m256i row[8];

    for (int r = 0; r < 8; r++) {
        row[r] = _mm256_load_si256(reinterpret_cast<const __m256i*>(s.w[r]));
    }

    permute2048_avx2_regs(row, rounds);

    for (int r = 0; r < 8; r++) {
        _mm256_store_si256(reinterpret_cast<__m256i*>(s.w[r]), row[r]);
    }
}
#endif

// -----------------------------------------------------------------------------
// AVX-512VL
// -----------------------------------------------------------------------------

#if defined(__AVX512F__) && defined(__AVX512VL__)
#include <immintrin.h>

static inline __m256i chi3_avx512(__m256i a, __m256i b, __m256i c) {
    // c ^ (~a & b)
    // return _mm256_ternarylogic_epi64(a, b, c, 0xb4);
    return _mm256_ternarylogic_epi64(a, b, c, 0xa6);
}

static inline __m256i chi3_special_avx512(__m256i a, __m256i b, __m256i c) {
    // ~c ^ (~a & ~b)
    // return _mm256_ternarylogic_epi64(a, b, c, 0x1e);
    return _mm256_ternarylogic_epi64(a, b, c, 0x56);
}

template<unsigned r>
static inline __m256i rotl64_256_avx512(__m256i x) {
    if constexpr ((r & 63) == 0) {
        return x;
    } else {
        return _mm256_rol_epi64(x, r & 63);
    }
}

template<int imm>
static inline __m256i perm64_avx512(__m256i x) {
    return _mm256_permutex_epi64(x, imm);
}

template<unsigned a, unsigned b, unsigned c, unsigned d>
static inline __m256i row_mix_one_avx512(__m256i x) {
    __m256i x1 = perm64_avx512<0x39>(x);
    __m256i x2 = perm64_avx512<0x4e>(x);
    __m256i x3 = perm64_avx512<0x93>(x);
    __m256i xd = rotl64_256_avx512<d>(x);

    return _mm256_xor_si256(
        _mm256_xor_si256(x, xd),
        _mm256_xor_si256(
            rotl64_256_avx512<a>(x1),
            _mm256_xor_si256(rotl64_256_avx512<b>(x2), rotl64_256_avx512<c>(x3))
        )
    );
}

static inline void chichi_avx512_regs(__m256i row[8]) {
    __m256i x0 = row[0], x1 = row[1], x2 = row[2], x3 = row[3];
    __m256i x4 = row[4], x5 = row[5], x6 = row[6], x7 = row[7];

    row[BIT_PERM[0]] = chi3_avx512(x1, x2, x0);
    row[BIT_PERM[1]] = chi3_avx512(x2, x0, x4);
    row[BIT_PERM[2]] = chi3_avx512(x0, x1, x3);
    row[BIT_PERM[3]] = chi3_special_avx512(x4, x5, x1);
    row[BIT_PERM[4]] = chi3_avx512(x5, x6, x2);
    row[BIT_PERM[5]] = chi3_avx512(x6, x7, x5);
    row[BIT_PERM[6]] = chi3_avx512(x7, x3, x6);
    row[BIT_PERM[7]] = chi3_avx512(x3, x4, x7);
}


static inline void row_mix_avx512_regs(__m256i row[8]) {
    row[0] = row_mix_one_avx512<A[0], B[0], C[0], D[0]>(row[0]);
    row[1] = row_mix_one_avx512<A[1], B[1], C[1], D[1]>(row[1]);
    row[2] = row_mix_one_avx512<A[2], B[2], C[2], D[2]>(row[2]);
    row[3] = row_mix_one_avx512<A[3], B[3], C[3], D[3]>(row[3]);
    row[4] = row_mix_one_avx512<A[4], B[4], C[4], D[4]>(row[4]);
    row[5] = row_mix_one_avx512<A[5], B[5], C[5], D[5]>(row[5]);
    row[6] = row_mix_one_avx512<A[6], B[6], C[6], D[6]>(row[6]);
    row[7] = row_mix_one_avx512<A[7], B[7], C[7], D[7]>(row[7]);
}

static inline void add_constant_avx512_regs(__m256i row[8], unsigned round) {
    row[0] = _mm256_xor_si256(row[0], _mm256_set_epi64x(0, 0, 0, round_constant(round)));
}

static inline void permute2048_avx512_regs(__m256i row[8], unsigned rounds) {
    for (unsigned r = 0; r < rounds; r++) {
        chichi_avx512_regs(row);
        row_mix_avx512_regs(row);
        add_constant_avx512_regs(row, r);
    }
}

static void permute2048_avx512(State2048& s, unsigned rounds) {
    __m256i row[8];

    for (int r = 0; r < 8; r++) {
        row[r] = _mm256_load_si256(reinterpret_cast<const __m256i*>(s.w[r]));
    }

    permute2048_avx512_regs(row, rounds);

    for (int r = 0; r < 8; r++) {
        _mm256_store_si256(reinterpret_cast<__m256i*>(s.w[r]), row[r]);
    }
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
#elif defined(__SSSE3__)
    permute2048_sse(s, rounds);
#else
    permute2048_scalar(s, rounds);
#endif
}




// -----------------------------------------------------------------------------
// Fast F-sponge absorb loop for MEGASCON-768, rate = 152 bytes
// -----------------------------------------------------------------------------

static inline uint64_t load_u64_unaligned(const unsigned char *p) {
    uint64_t x;
    std::memcpy(&x, p, sizeof(x));
    return x;
}

static inline void absorb_block152_scalar(State2048& s, const unsigned char *block) {
    s.w[0][0] ^= load_u64_unaligned(block +   0);
    s.w[0][1] ^= load_u64_unaligned(block +   8);
    s.w[0][2] ^= load_u64_unaligned(block +  16);
    s.w[0][3] ^= load_u64_unaligned(block +  24);
    s.w[1][0] ^= load_u64_unaligned(block +  32);
    s.w[1][1] ^= load_u64_unaligned(block +  40);
    s.w[1][2] ^= load_u64_unaligned(block +  48);
    s.w[1][3] ^= load_u64_unaligned(block +  56);
    s.w[2][0] ^= load_u64_unaligned(block +  64);
    s.w[2][1] ^= load_u64_unaligned(block +  72);
    s.w[2][2] ^= load_u64_unaligned(block +  80);
    s.w[2][3] ^= load_u64_unaligned(block +  88);
    s.w[3][0] ^= load_u64_unaligned(block +  96);
    s.w[3][1] ^= load_u64_unaligned(block + 104);
    s.w[3][2] ^= load_u64_unaligned(block + 112);
    s.w[3][3] ^= load_u64_unaligned(block + 120);
    s.w[4][0] ^= load_u64_unaligned(block + 128);
    s.w[4][1] ^= load_u64_unaligned(block + 136);
    s.w[4][2] ^= load_u64_unaligned(block + 144);
}

static inline void fast_absorb_one152_scalar(State2048& state,
                                             const unsigned char *block,
                                             unsigned rounds,
                                             bool final_block) {
    absorb_block152_scalar(state, block);

    if (final_block) {
        state.w[4][3] ^= 0x80ULL;
    }

    uint64_t f43 = state.w[4][3];
    uint64_t f5[4], f6[4], f7[4];
    std::memcpy(f5, state.w[5], sizeof(f5));
    std::memcpy(f6, state.w[6], sizeof(f6));
    std::memcpy(f7, state.w[7], sizeof(f7));

    permute2048_scalar(state, rounds);

    state.w[4][3] ^= f43;
    for (int i = 0; i < 4; i++) {
        state.w[5][i] ^= f5[i];
        state.w[6][i] ^= f6[i];
        state.w[7][i] ^= f7[i];
    }
}

static inline void fast_absorb_blocks152_scalar(State2048& state,
                                                const unsigned char *blocks,
                                                ULL block_count,
                                                unsigned rounds,
                                                bool last_block_is_final) {
    for (ULL i = 0; i < block_count; i++) {
        bool final_block = last_block_is_final && (i + 1 == block_count);
        fast_absorb_one152_scalar(state, blocks + i * 152ULL, rounds, final_block);
    }
}

#if defined(__SSSE3__)

static inline __m128i mask_high64_sse(__m128i x) {
    return _mm_and_si128(x, _mm_set_epi64x(-1LL, 0LL));
}

static inline void absorb_block152_sse_regs(Vec256_SSE row[8], const unsigned char *block) {
    row[0].lo = _mm_xor_si128(row[0].lo, _mm_loadu_si128(reinterpret_cast<const __m128i *>(block +   0)));
    row[0].hi = _mm_xor_si128(row[0].hi, _mm_loadu_si128(reinterpret_cast<const __m128i *>(block +  16)));
    row[1].lo = _mm_xor_si128(row[1].lo, _mm_loadu_si128(reinterpret_cast<const __m128i *>(block +  32)));
    row[1].hi = _mm_xor_si128(row[1].hi, _mm_loadu_si128(reinterpret_cast<const __m128i *>(block +  48)));
    row[2].lo = _mm_xor_si128(row[2].lo, _mm_loadu_si128(reinterpret_cast<const __m128i *>(block +  64)));
    row[2].hi = _mm_xor_si128(row[2].hi, _mm_loadu_si128(reinterpret_cast<const __m128i *>(block +  80)));
    row[3].lo = _mm_xor_si128(row[3].lo, _mm_loadu_si128(reinterpret_cast<const __m128i *>(block +  96)));
    row[3].hi = _mm_xor_si128(row[3].hi, _mm_loadu_si128(reinterpret_cast<const __m128i *>(block + 112)));
    row[4].lo = _mm_xor_si128(row[4].lo, _mm_loadu_si128(reinterpret_cast<const __m128i *>(block + 128)));
    row[4].hi = _mm_xor_si128(row[4].hi, _mm_loadl_epi64(reinterpret_cast<const __m128i *>(block + 144)));
}

static inline void domain_sep152_sse_regs(Vec256_SSE row[8]) {
    row[4].hi = _mm_xor_si128(row[4].hi, _mm_set_epi64x(0x80LL, 0LL));
}

static inline void feedforward152_sse_regs(Vec256_SSE row[8], unsigned rounds) {
    __m128i f4hi = mask_high64_sse(row[4].hi);
    Vec256_SSE f5 = row[5];
    Vec256_SSE f6 = row[6];
    Vec256_SSE f7 = row[7];

    permute2048_sse_regs(row, rounds);

    row[4].hi = _mm_xor_si128(row[4].hi, f4hi);
    row[5] = xor256(row[5], f5);
    row[6] = xor256(row[6], f6);
    row[7] = xor256(row[7], f7);
}

static inline void fast_absorb_blocks152_sse(State2048& state,
                                             const unsigned char *blocks,
                                             ULL block_count,
                                             unsigned rounds,
                                             bool last_block_is_final) {
    Vec256_SSE row[8];
    for (int r = 0; r < 8; r++) row[r] = load_row_sse(state, r);

    for (ULL i = 0; i < block_count; i++) {
        absorb_block152_sse_regs(row, blocks + i * 152ULL);
        if (last_block_is_final && (i + 1 == block_count)) {
            domain_sep152_sse_regs(row);
        }
        feedforward152_sse_regs(row, rounds);
    }

    for (int r = 0; r < 8; r++) store_row_sse(state, r, row[r]);
}

#endif

#if defined(__AVX2__)

static inline __m256i load_absorb_row4_tail152_avx2(const unsigned char *block) {
    __m128i lo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(block + 128));
    __m128i hi = _mm_cvtsi64_si128(static_cast<long long>(load_u64_unaligned(block + 144)));
    __m256i v = _mm256_castsi128_si256(lo);
    return _mm256_inserti128_si256(v, hi, 1);
}

static inline void absorb_block152_avx2_regs(__m256i row[8], const unsigned char *block) {
    row[0] = _mm256_xor_si256(row[0], _mm256_loadu_si256(reinterpret_cast<const __m256i *>(block +   0)));
    row[1] = _mm256_xor_si256(row[1], _mm256_loadu_si256(reinterpret_cast<const __m256i *>(block +  32)));
    row[2] = _mm256_xor_si256(row[2], _mm256_loadu_si256(reinterpret_cast<const __m256i *>(block +  64)));
    row[3] = _mm256_xor_si256(row[3], _mm256_loadu_si256(reinterpret_cast<const __m256i *>(block +  96)));
    row[4] = _mm256_xor_si256(row[4], load_absorb_row4_tail152_avx2(block));
}

static inline void domain_sep152_avx2_regs(__m256i row[8]) {
    row[4] = _mm256_xor_si256(row[4], _mm256_set_epi64x(0x80LL, 0LL, 0LL, 0LL));
}

static inline void feedforward152_avx2_regs(__m256i row[8], unsigned rounds) {
    const __m256i cap4_mask = _mm256_set_epi64x(-1LL, 0LL, 0LL, 0LL);
    __m256i f4 = _mm256_and_si256(row[4], cap4_mask);
    __m256i f5 = row[5];
    __m256i f6 = row[6];
    __m256i f7 = row[7];

    permute2048_avx2_regs(row, rounds);

    row[4] = _mm256_xor_si256(row[4], f4);
    row[5] = _mm256_xor_si256(row[5], f5);
    row[6] = _mm256_xor_si256(row[6], f6);
    row[7] = _mm256_xor_si256(row[7], f7);
}

static inline void fast_absorb_blocks152_avx2(State2048& state,
                                              const unsigned char *blocks,
                                              ULL block_count,
                                              unsigned rounds,
                                              bool last_block_is_final) {
    __m256i row[8];
    for (int r = 0; r < 8; r++) {
        row[r] = _mm256_load_si256(reinterpret_cast<const __m256i *>(state.w[r]));
    }

    for (ULL i = 0; i < block_count; i++) {
        absorb_block152_avx2_regs(row, blocks + i * 152ULL);
        if (last_block_is_final && (i + 1 == block_count)) {
            domain_sep152_avx2_regs(row);
        }
        feedforward152_avx2_regs(row, rounds);
    }

    for (int r = 0; r < 8; r++) {
        _mm256_store_si256(reinterpret_cast<__m256i *>(state.w[r]), row[r]);
    }
}

#endif

#if defined(__AVX512F__) && defined(__AVX512VL__)

static inline __m256i load_absorb_row4_tail152_avx512(const unsigned char *block) {
    __m128i lo = _mm_loadu_si128(reinterpret_cast<const __m128i *>(block + 128));
    __m128i hi = _mm_cvtsi64_si128(static_cast<long long>(load_u64_unaligned(block + 144)));
    __m256i v = _mm256_castsi128_si256(lo);
    return _mm256_inserti128_si256(v, hi, 1);
}

static inline void absorb_block152_avx512_regs(__m256i row[8], const unsigned char *block) {
    row[0] = _mm256_xor_si256(row[0], _mm256_loadu_si256(reinterpret_cast<const __m256i *>(block +   0)));
    row[1] = _mm256_xor_si256(row[1], _mm256_loadu_si256(reinterpret_cast<const __m256i *>(block +  32)));
    row[2] = _mm256_xor_si256(row[2], _mm256_loadu_si256(reinterpret_cast<const __m256i *>(block +  64)));
    row[3] = _mm256_xor_si256(row[3], _mm256_loadu_si256(reinterpret_cast<const __m256i *>(block +  96)));
    row[4] = _mm256_xor_si256(row[4], load_absorb_row4_tail152_avx512(block));
}

static inline void domain_sep152_avx512_regs(__m256i row[8]) {
    row[4] = _mm256_xor_si256(row[4], _mm256_set_epi64x(0x80LL, 0LL, 0LL, 0LL));
}

static inline void feedforward152_avx512_regs(__m256i row[8], unsigned rounds) {
    const __m256i cap4_mask = _mm256_set_epi64x(-1LL, 0LL, 0LL, 0LL);
    __m256i f4 = _mm256_and_si256(row[4], cap4_mask);
    __m256i f5 = row[5];
    __m256i f6 = row[6];
    __m256i f7 = row[7];

    permute2048_avx512_regs(row, rounds);

    row[4] = _mm256_xor_si256(row[4], f4);
    row[5] = _mm256_xor_si256(row[5], f5);
    row[6] = _mm256_xor_si256(row[6], f6);
    row[7] = _mm256_xor_si256(row[7], f7);
}

static inline void fast_absorb_blocks152_avx512(State2048& state,
                                                const unsigned char *blocks,
                                                ULL block_count,
                                                unsigned rounds,
                                                bool last_block_is_final) {
    __m256i row[8];
    for (int r = 0; r < 8; r++) {
        row[r] = _mm256_load_si256(reinterpret_cast<const __m256i *>(state.w[r]));
    }

    for (ULL i = 0; i < block_count; i++) {
        absorb_block152_avx512_regs(row, blocks + i * 152ULL);
        if (last_block_is_final && (i + 1 == block_count)) {
            domain_sep152_avx512_regs(row);
        }
        feedforward152_avx512_regs(row, rounds);
    }

    for (int r = 0; r < 8; r++) {
        _mm256_store_si256(reinterpret_cast<__m256i *>(state.w[r]), row[r]);
    }
}

#endif

static inline void fast_absorb_blocks152_dispatch(State2048& state,
                                                  const unsigned char *blocks,
                                                  ULL block_count,
                                                  unsigned rounds,
                                                  bool last_block_is_final) {
#if defined(__AVX512F__) && defined(__AVX512VL__)
    fast_absorb_blocks152_avx512(state, blocks, block_count, rounds, last_block_is_final);
#elif defined(__AVX2__)
    fast_absorb_blocks152_avx2(state, blocks, block_count, rounds, last_block_is_final);
#elif defined(__SSSE3__)
    fast_absorb_blocks152_sse(state, blocks, block_count, rounds, last_block_is_final);
#else
    fast_absorb_blocks152_scalar(state, blocks, block_count, rounds, last_block_is_final);
#endif
}

// -----------------------------------------------------------------------------
// Standard NGCC-style interface
// -----------------------------------------------------------------------------

extern "C" void permutation(unsigned char *state, unsigned rounds)
{
    State2048 s{};
    std::memcpy(s.w, state, sizeof(s.w));
    permute2048_dispatch(s, rounds);
    std::memcpy(state, s.w, sizeof(s.w));
}

extern "C" int hash_Fsponge(int hash, int rate,
                             const unsigned char *input,
                             ULL input_len_bits,
                             int output_len_bits,
                             unsigned char *output)
{
    if (rate != 152 || hash != 96 || output_len_bits != hash * 8) {
        std::printf("Invalid rate or hash length\n");
        return 1;
    }
    if (output == nullptr || (input == nullptr && input_len_bits != 0)) {
        return 1;
    }

    State2048 state{};

    const ULL rate_bits = 152ULL * 8ULL;
    const ULL full_blocks = input_len_bits / rate_bits;
    const ULL rem_bits = input_len_bits % rate_bits;

    if (full_blocks > 0) {
        fast_absorb_blocks152_dispatch(state, input, full_blocks, 16, false);
    }

    unsigned char last_block[152];
    std::memset(last_block, 0, sizeof(last_block));

    const unsigned char *tail = input + full_blocks * 152ULL;
    const ULL rem_full_bytes = rem_bits / 8ULL;
    const int rem_extra_bits = static_cast<int>(rem_bits & 7ULL);

    if (rem_full_bytes > 0) {
        std::memcpy(last_block, tail, static_cast<size_t>(rem_full_bytes));
    }

    if (rem_extra_bits > 0) {
        unsigned char mask = static_cast<unsigned char>(0xFFu << (8 - rem_extra_bits));
        last_block[rem_full_bytes] = tail[rem_full_bytes] & mask;
        last_block[rem_full_bytes] |= static_cast<unsigned char>(0x80u >> rem_extra_bits);
    } else {
        last_block[rem_full_bytes] = 0x80;
    }

    fast_absorb_blocks152_dispatch(state, last_block, 1, 16, true);

    std::memcpy(output, reinterpret_cast<const unsigned char *>(state.w) + (256 - 96), 96);
    return 0;
}

extern "C" int CryptHash(int digest_len_bits,
                          const unsigned char *msg,
                          unsigned long long msg_len_bits,
                          unsigned char *digest)
{
    return hash_Fsponge(digest_len_bits / 8, 152,
                        msg, msg_len_bits,
                        digest_len_bits, digest);
}
