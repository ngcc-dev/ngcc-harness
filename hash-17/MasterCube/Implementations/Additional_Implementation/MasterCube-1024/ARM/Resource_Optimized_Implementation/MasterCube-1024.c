#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#include <stdalign.h>
#include <stdint.h>
#include <string.h>
#if (defined(__aarch64__) || defined(__arm64__)) && !defined(MCUBE_USE_SSE2NEON) && !defined(MCUBE_RESOURCE_KEEP_PLAIN)
#define MCUBE_USE_SSE2NEON 1
#endif
#if defined(MCUBE_USE_SSE2NEON)
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include "sse2neon.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#elif defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
#include <immintrin.h>
#endif

#include "mastercube.h"
#if !defined(_mm_loadu_si64) && (defined(__SSE2__) || defined(__SSE4_2__) || defined(__AVX2__) || defined(__AVX512F__))
static inline __m128i mc_loadu_si64_compat(const void *p)
{
    return _mm_loadl_epi64((const __m128i *)p);
}
#define _mm_loadu_si64(p) mc_loadu_si64_compat((p))
#endif

#if defined(MCUBE_USE_SSE2NEON) && (defined(__GNUC__) || defined(__clang__))
#define MC_SSE_HOT_ATTR
#define MC_SSE_TRANSFORM_DECL static inline __attribute__((always_inline)) MC_SSE_HOT_ATTR
#define MC_SSE_ABSORB_DECL static inline __attribute__((always_inline)) MC_SSE_HOT_ATTR
#define MC_SSE_ENTRY_ATTR __attribute__((flatten)) MC_SSE_HOT_ATTR
#else
#define MC_SSE_HOT_ATTR
#define MC_SSE_TRANSFORM_DECL static
#define MC_SSE_ABSORB_DECL static inline
#define MC_SSE_ENTRY_ATTR
#endif

#if defined(__GNUC__) && !defined(__clang__) && (defined(__AVX2__) || defined(__AVX512F__)) && !defined(MC_GCC53_COMPAT_H)
static inline __m256i mc_zextsi128_si256_compat(__m128i x)
{
    return _mm256_inserti128_si256(_mm256_setzero_si256(), x, 0);
}
#ifndef _mm256_zextsi128_si256
#define _mm256_zextsi128_si256(x) mc_zextsi128_si256_compat((x))
#endif
#endif

/****************************************************************************
 * Plain scalar implementation
 ****************************************************************************/
#if !defined(MCUBE_USE_SSE2NEON) && (!defined (__AVX512F__) || !defined (__AVX512BW__) || !defined (__AVX512VL__)) && !defined(__AVX2__) && !defined(__SSE4_2__)


static const int N_ROUNDS = 18;

static const uint16_t MC_ADD_CONSTANT[8] = {
    0x7344, 0x0370, 0x8A2E, 0x1319,
    0x08D3, 0x85A3, 0x6A88, 0x243F
};

/*
 * row-major matrices
 */
union xy_slice_view {
    alignas(16) uint16_t slice[6][8];
    uint16_t row[6][8];
    uint64_t row64[6][2];
    uint16_t raw_data[48];
};

struct xy_slice {
    union xy_slice_view u;
};

struct mc_state_halves {
    struct xy_slice left;
    struct xy_slice right;
};

union mc_state_view {
    struct mc_state_halves halves;
    uint16_t row[12][8];
    uint64_t row64[12][2];
    uint32_t row32[12][4];
    uint16_t raw_data[96];
    uint32_t pair32[6][8];
    uint64_t pair64[6][4];
};

struct mc_state {
    union mc_state_view u;
};

#define slice u.slice
#define row u.row
#define row64 u.row64
#define raw_data u.raw_data
#define left u.halves.left
#define right u.halves.right
#define row32 u.row32
#define pair32 u.pair32
#define pair64 u.pair64

static inline uint16_t ror16(const uint16_t x, const int n_ror)
{
    if (n_ror == 0) return x;
    return (x >> n_ror) | (x << (16 - n_ror));
}

static inline void rotl_row16_1(uint64_t x[2])
{
    const uint64_t a = x[0];
    const uint64_t b = x[1];
    x[0] = (a >> 16) | (b << 48);
    x[1] = (b >> 16) | (a << 48);
}

static inline void rotl_row16_2(uint64_t x[2])
{
    const uint64_t a = x[0];
    const uint64_t b = x[1];
    x[0] = (a >> 32) | (b << 32);
    x[1] = (b >> 32) | (a << 32);
}

static inline void rotl_row16_3(uint64_t x[2])
{
    const uint64_t a = x[0];
    const uint64_t b = x[1];
    x[0] = (a >> 48) | (b << 16);
    x[1] = (b >> 48) | (a << 16);
}

static inline void rotl_row16_4(uint64_t x[2])
{
    const uint64_t a = x[0];
    x[0] = x[1];
    x[1] = a;
}

static inline void rotl_row16_5(uint64_t x[2])
{
    const uint64_t a = x[0];
    const uint64_t b = x[1];
    x[0] = (b >> 16) | (a << 48);
    x[1] = (a >> 16) | (b << 48);
}

static inline void rotr_row16_1(uint64_t x[2])
{
    const uint64_t a = x[0];
    const uint64_t b = x[1];
    x[0] = (a << 16) | (b >> 48);
    x[1] = (b << 16) | (a >> 48);
}

static inline void rotr_row16_2(uint64_t x[2])
{
    const uint64_t a = x[0];
    const uint64_t b = x[1];
    x[0] = (a << 32) | (b >> 32);
    x[1] = (b << 32) | (a >> 32);
}

static inline void rotr_row16_3(uint64_t x[2])
{
    rotl_row16_5(x);
}

static inline void rotr_row16_4(uint64_t x[2])
{
    rotl_row16_4(x);
}

static inline void rotr_row16_5(uint64_t x[2])
{
    rotl_row16_3(x);
}

static inline void MAndRXs(struct xy_slice *state_l, struct xy_slice *state_r, const int alpha, const int beta, const int gamma)
{
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 8; ++j) {
            const uint16_t x = state_l->slice[i][j];
            const uint16_t y = state_r->slice[i][j];
            const uint16_t a = ror16(x, alpha);
            const uint16_t b = ror16(x, beta);
            const uint16_t c = ror16(x, gamma);
            state_r->slice[i][j] = (a & b) ^ c ^ y;
        }

    return;
}

static inline void ShiftRows(struct xy_slice *state_l, struct xy_slice *state_r)
{
    rotl_row16_1(state_l->row64[1]);
    rotl_row16_2(state_l->row64[2]);
    rotl_row16_3(state_l->row64[3]);
    rotl_row16_4(state_l->row64[4]);
    rotl_row16_5(state_l->row64[5]);

    rotr_row16_1(state_r->row64[1]);
    rotr_row16_2(state_r->row64[2]);
    rotr_row16_3(state_r->row64[3]);
    rotr_row16_4(state_r->row64[4]);
    rotr_row16_5(state_r->row64[5]);
}

static inline void mix_row_64x2(uint64_t x[2])
{
    const uint64_t a = x[0];
    const uint64_t b = x[1];
    x[0] = a ^ ((a >> 32) | (b << 32)) ^ ((a << 32) | (b >> 32));
    x[1] = b ^ ((b >> 32) | (a << 32)) ^ ((b << 32) | (a >> 32));
}

static inline void MixRows(struct mc_state *state)
{
    for (int i = 0; i < 12; ++i)
        mix_row_64x2(state->row64[i]);
}

static inline void swap_row64x2(uint64_t a[2], uint64_t b[2])
{
    const uint64_t t0 = a[0];
    const uint64_t t1 = a[1];
    a[0] = b[0];
    a[1] = b[1];
    b[0] = t0;
    b[1] = t1;
}

static inline void SwapRows(struct mc_state *state)
{
    swap_row64x2(state->right.row64[0], state->right.row64[1]);
    swap_row64x2(state->right.row64[2], state->right.row64[3]);
    swap_row64x2(state->right.row64[4], state->right.row64[5]);
}

static inline void CrossMixs(struct mc_state *state)
{
#if defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_7M__)
    for (int j = 0; j < 8; ++j) {
        const uint32_t x0 = state->pair32[0][j];
        const uint32_t x1 = state->pair32[1][j];
        const uint32_t x2 = state->pair32[2][j];
        const uint32_t x3 = state->pair32[3][j];
        const uint32_t x4 = state->pair32[4][j];
        const uint32_t x5 = state->pair32[5][j];

        state->pair32[0][j] = x0 ^ x1 ^ x4;
        state->pair32[1][j] = x1 ^ x2 ^ x5;
        state->pair32[2][j] = x2 ^ x3 ^ x0;
        state->pair32[3][j] = x3 ^ x4 ^ x1;
        state->pair32[4][j] = x4 ^ x5 ^ x2;
        state->pair32[5][j] = x5 ^ x0 ^ x3;
    }
#else
    const uint64_t x00 = state->pair64[0][0], x01 = state->pair64[1][0], x02 = state->pair64[2][0], x03 = state->pair64[3][0], x04 = state->pair64[4][0], x05 = state->pair64[5][0];
    const uint64_t x10 = state->pair64[0][1], x11 = state->pair64[1][1], x12 = state->pair64[2][1], x13 = state->pair64[3][1], x14 = state->pair64[4][1], x15 = state->pair64[5][1];
    const uint64_t x20 = state->pair64[0][2], x21 = state->pair64[1][2], x22 = state->pair64[2][2], x23 = state->pair64[3][2], x24 = state->pair64[4][2], x25 = state->pair64[5][2];
    const uint64_t x30 = state->pair64[0][3], x31 = state->pair64[1][3], x32 = state->pair64[2][3], x33 = state->pair64[3][3], x34 = state->pair64[4][3], x35 = state->pair64[5][3];

    state->pair64[0][0] = x00 ^ x01 ^ x04;
    state->pair64[1][0] = x01 ^ x02 ^ x05;
    state->pair64[2][0] = x02 ^ x03 ^ x00;
    state->pair64[3][0] = x03 ^ x04 ^ x01;
    state->pair64[4][0] = x04 ^ x05 ^ x02;
    state->pair64[5][0] = x05 ^ x00 ^ x03;

    state->pair64[0][1] = x10 ^ x11 ^ x14;
    state->pair64[1][1] = x11 ^ x12 ^ x15;
    state->pair64[2][1] = x12 ^ x13 ^ x10;
    state->pair64[3][1] = x13 ^ x14 ^ x11;
    state->pair64[4][1] = x14 ^ x15 ^ x12;
    state->pair64[5][1] = x15 ^ x10 ^ x13;

    state->pair64[0][2] = x20 ^ x21 ^ x24;
    state->pair64[1][2] = x21 ^ x22 ^ x25;
    state->pair64[2][2] = x22 ^ x23 ^ x20;
    state->pair64[3][2] = x23 ^ x24 ^ x21;
    state->pair64[4][2] = x24 ^ x25 ^ x22;
    state->pair64[5][2] = x25 ^ x20 ^ x23;

    state->pair64[0][3] = x30 ^ x31 ^ x34;
    state->pair64[1][3] = x31 ^ x32 ^ x35;
    state->pair64[2][3] = x32 ^ x33 ^ x30;
    state->pair64[3][3] = x33 ^ x34 ^ x31;
    state->pair64[4][3] = x34 ^ x35 ^ x32;
    state->pair64[5][3] = x35 ^ x30 ^ x33;
#endif


    return;
}

static inline void MixColumns(struct mc_state *state)
{
    for (int j = 0; j < 2; ++j) {
        const uint64_t x0 = state->row64[0][j];
        const uint64_t x2 = state->row64[2][j];
        const uint64_t x4 = state->row64[4][j];
        state->row64[0][j] = x2 ^ x4;
        state->row64[2][j] = x0 ^ x4;
        state->row64[4][j] = x0 ^ x2 ^ x4;

        const uint64_t x1 = state->row64[1][j];
        const uint64_t x3 = state->row64[3][j];
        const uint64_t x5 = state->row64[5][j];
        state->row64[1][j] = x3 ^ x5;
        state->row64[3][j] = x1 ^ x5;
        state->row64[5][j] = x1 ^ x3 ^ x5;

        const uint64_t x6 = state->row64[6][j];
        const uint64_t x8 = state->row64[8][j];
        const uint64_t x10 = state->row64[10][j];
        state->row64[6][j] = x6 ^ x10;
        state->row64[8][j] = x8 ^ x10;
        state->row64[10][j] = x6 ^ x8 ^ x10;

        const uint64_t x7 = state->row64[7][j];
        const uint64_t x9 = state->row64[9][j];
        const uint64_t x11 = state->row64[11][j];
        state->row64[7][j] = x7 ^ x11;
        state->row64[9][j] = x9 ^ x11;
        state->row64[11][j] = x7 ^ x9 ^ x11;
    }
}

static inline void AddConstant(struct mc_state *state, const int rn)
{
    const uint16_t r = (uint16_t)rn;
    state->row[0][0] ^= MC_ADD_CONSTANT[0] ^ r;
    state->row[0][1] ^= MC_ADD_CONSTANT[1] ^ r;
    state->row[0][2] ^= MC_ADD_CONSTANT[2] ^ r;
    state->row[0][3] ^= MC_ADD_CONSTANT[3] ^ r;
    state->row[0][4] ^= MC_ADD_CONSTANT[4] ^ r;
    state->row[0][5] ^= MC_ADD_CONSTANT[5] ^ r;
    state->row[0][6] ^= MC_ADD_CONSTANT[6] ^ r;
    state->row[0][7] ^= MC_ADD_CONSTANT[7] ^ r;
}

static inline void _core(struct mc_state *state, const int n_rounds)
{
    //#pragma GCC unroll 20
    for (int i = 0; i < n_rounds; ++i) {
        CrossMixs(state);
        SwapRows(state);

        MAndRXs(&state->left, &state->right,  0, 1, 8);
        MAndRXs(&state->right, &state->left, 14, 5, 0);
        MAndRXs(&state->left, &state->right, 9, 12, 8);
        MAndRXs(&state->right, &state->left, 14, 5, 0);
        MAndRXs(&state->left, &state->right, 0, 1, 8);
        MAndRXs(&state->right, &state->left, 8, 9, 0);

        CrossMixs(state);
        SwapRows(state);

        MixColumns(state);
        ShiftRows(&state->left, &state->right);
        MixRows(state);

        AddConstant(state, i);
    }

    return;
}

static inline void _core_inverse(struct mc_state *state, const int n_rounds)
{

    for (int i = n_rounds - 1; i >= 0; --i) {
        AddConstant(state, i + 9); // for 18 rounds mcube

        MixRows(state);

        ShiftRows(&state->right, &state->left);

        MixColumns(state);

        SwapRows(state);
        CrossMixs(state);

        MAndRXs(&state->right, &state->left, 8, 9, 0);
        MAndRXs(&state->left, &state->right, 0, 1, 8);
        MAndRXs(&state->right, &state->left, 14, 5, 0);
        MAndRXs(&state->left, &state->right, 9, 12, 8);
        MAndRXs(&state->right, &state->left, 14, 5, 0);
        MAndRXs(&state->left, &state->right,  0, 1, 8);

        for (int j = 0; j < 6; ++j)
            swap_row64x2(state->left.row64[j], state->right.row64[j]);

        SwapRows(state);
        CrossMixs(state);
    }

    return;
}

static inline void zip_prf(struct mc_state *state)
{
    struct mc_state state_copy;
    memcpy(state_copy.raw_data, state->raw_data, sizeof(struct mc_state));

    _core(state, N_ROUNDS / 2);

    _core_inverse(&state_copy, N_ROUNDS / 2);

    for (int i = 0; i < 12; ++i) {
        state->row64[i][0] ^= state_copy.row64[i][0];
        state->row64[i][1] ^= state_copy.row64[i][1];
    }
}

static void mc_transform(struct mc_state *state)
{
    zip_prf(state);
    return;
}


static inline void _sponge_absorb_helper(void (*transform)(struct mc_state *), struct mc_state *state, const int block_bits, const unsigned char *message, const uint64_t message_bits)
{
    memset(state, 0, sizeof(*state));

    const int block_bytes = block_bits / 8;
    const int n_m128_per_block = block_bits / 128; // _m128i reg
    const uint64_t message_bytes = (message_bits + 7) / 8;
    const uint64_t n_block = message_bits / block_bits; // number of full blocks
    uint8_t last_block[136] = {0x00}; // the last and unaligned block, allocate the largest potential needed size, and pad with 0x00

    // copy and turn the last unaligned block to a padded full block
    const uint64_t last_index = n_block * block_bytes;
    memcpy(last_block, message + last_index, message_bytes - last_index);
    pad10star1(last_block, message_bits % block_bits, block_bits);

    for (uint64_t i = 0; i < n_block; ++i) {
        for (int j = 0; j < n_m128_per_block; ++j) {
            for (int k = 0; k < 8; ++k)
                state->row[j][k] ^= ((uint16_t *)(message + block_bytes * i + 16 * j))[k];
        }
        for (int k = 0; k < 4; ++k)
            state->row[n_m128_per_block][k] ^= ((uint16_t *)(message + block_bytes * i + 16 * n_m128_per_block))[k];

        transform(state);
    }
    {
        for (int j = 0; j < n_m128_per_block; ++j) {
            for (int k = 0; k < 8; ++k)
                state->row[j][k] ^= ((uint16_t *)(last_block + 16 * j))[k];
        }
        for (int k = 0; k < 4; ++k)
            state->row[n_m128_per_block][k] ^= ((uint16_t *)(last_block + 16 * n_m128_per_block))[k];

        transform(state);
    }

    return;
}

static inline void _sponge_squeeze_helper(void (*transform)(struct mc_state *), struct mc_state *state, const int block_bits, unsigned char *digest, const int digest_bits)
{
    const int digest_bytes = digest_bits / 8;
    const int block_bytes = block_bits / 8;
    const int n_block = (digest_bytes + block_bytes - 1) / block_bytes;

    const int n_m128_per_block  = (block_bits  + 128 - 1) / 128; // _m128i reg
    const int n_m128_per_digest = (digest_bits + 128 - 1) / 128; // _m128i reg
    const int n_m128_per_iter   = n_m128_per_block < n_m128_per_digest ? n_m128_per_block : n_m128_per_digest;

    uint8_t buffer[3][768 / 8];

    for (int i = 0; i < n_block; ++i) { // max: 3 (the 1024 case)
        for (int j = 0; j < n_m128_per_iter; ++j) // max: 6 (the 768 case)
            memcpy(buffer[i] + 16 * j, state->row[j], 16);

        transform(state);
    }

    if (digest_bytes <= block_bytes) {
        memcpy(digest, buffer[0], digest_bytes);
    } else if (digest_bytes <= 2 * block_bytes) {
        memcpy(digest, buffer[0], block_bytes);
        memcpy(digest + block_bytes, buffer[1], digest_bytes - block_bytes);
    } else if (digest_bytes <= 3 * block_bytes) {
        memcpy(digest, buffer[0], block_bytes);
        memcpy(digest + block_bytes, buffer[1], block_bytes);
        memcpy(digest + 2 * block_bytes, buffer[2], digest_bytes - 2 * block_bytes);
    }

    return;
}

static inline void _sponge_squeeze_1024_direct(void (*transform)(struct mc_state *), struct mc_state *state, unsigned char *digest)
{
    memcpy(digest, state->raw_data, 56);
    transform(state);
    memcpy(digest + 56, state->raw_data, 56);
    transform(state);
    memcpy(digest + 112, state->raw_data, 16);
}


int MasterCube1024_plain(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest)
{
    struct mc_state state;

    enum {
        digest_bits = 1024, // bits
        block_bits = 448 // bits
    };

    _sponge_absorb_helper(mc_transform, &state, block_bits, message, message_bit_len);
    _sponge_squeeze_1024_direct(mc_transform, &state, digest);

    return 0;
}

#if defined(__aarch64__) || defined(__arm64__)
int MasterCube1024_arm64_plain(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest)
{
    return MasterCube1024_plain(message, message_bit_len, digest);
}
#endif


#undef pair64
#undef pair32
#undef row32
#undef right
#undef left
#undef raw_data
#undef row64
#undef row
#undef slice

#endif /* Plain scalar */

/****************************************************************************
 * SSE4.2 Implementation
 ****************************************************************************/
#if defined(MCUBE_USE_SSE2NEON) || ((!defined (__AVX512F__) || !defined (__AVX512BW__) || !defined (__AVX512VL__)) && !defined (__AVX2__) && defined(__SSE4_2__))

static const int N_ROUNDS = 18;

// regard the 128-bit reg as a 16-byte reg and rotate bytes not bits.
//static inline __m128i ror128(const __m128i x, const int n_ror) { return _mm_or_si128(_mm_srli_si128(x, n_ror), _mm_slli_si128(x, 16 - n_ror)); }
static inline __m128i neon_rotl_bytes(__m128i x, const int n)
{
    const uint8x16_t v = vreinterpretq_u8_m128i(x);
    switch (n) {
        case 0:  return x;
        case 2:  return vreinterpretq_m128i_u8(vextq_u8(v, v, 2));
        case 4:  return vreinterpretq_m128i_u8(vextq_u8(v, v, 4));
        case 6:  return vreinterpretq_m128i_u8(vextq_u8(v, v, 6));
        case 8:  return vreinterpretq_m128i_u8(vextq_u8(v, v, 8));
        case 10: return vreinterpretq_m128i_u8(vextq_u8(v, v, 10));
        case 12: return vreinterpretq_m128i_u8(vextq_u8(v, v, 12));
        case 14: return vreinterpretq_m128i_u8(vextq_u8(v, v, 14));
        default: return x;
    }
}

static inline __m128i ror128_optimized(const __m128i x, const int n_ror) {
    switch (n_ror) {
        case 0:
        case 16:
            return x;
            break;

        case 2:
            return neon_rotl_bytes(x, 2);
            break;

        case 4:
            return neon_rotl_bytes(x, 4);
            break;

        case 6:
            return neon_rotl_bytes(x, 6);
            break;

        case 8:
            return neon_rotl_bytes(x, 8);
            break;

        case 10:
            return neon_rotl_bytes(x, 10);
            break;

        case 12:
            return neon_rotl_bytes(x, 12);
            break;

        case 14:
            return neon_rotl_bytes(x, 14);
            break;

        default:
            return x;
            break;
    }
}
//static inline __m128i ror64(const __m128i x, const int n_ror) { return _mm_or_si128(_mm_srli_epi64(x, n_ror), _mm_slli_epi64(x, 64 - n_ror)); }
static inline __m128i ror64_optimized(const __m128i x, const int n_ror) {
    switch (n_ror) {
        case 0:
        case 64:
            return x;
            break;

        case 8:
            return _mm_shuffle_epi8(x, _mm_setr_epi8(1, 2, 3, 4, 5, 6, 7, 0, 9, 10, 11, 12, 13, 14, 15, 8));
            break;

        case 16:
            return _mm_shuffle_epi8(x, _mm_setr_epi8(2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9));
            break;

        case 24:
            return _mm_shuffle_epi8(x, _mm_setr_epi8(3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10));
            break;

        case 32:
            return _mm_shuffle_epi8(x, _mm_setr_epi8(4, 5, 6, 7, 0, 1, 2, 3, 12, 13, 14, 15, 8, 9, 10, 11));
            break;

        case 40:
            return _mm_shuffle_epi8(x, _mm_setr_epi8(5, 6, 7, 0, 1, 2, 3, 4, 13, 14, 15, 8, 9, 10, 11, 12));
            break;

        case 48:
            return _mm_shuffle_epi8(x, _mm_setr_epi8(6, 7, 0, 1, 2, 3, 4, 5, 14, 15, 8, 9, 10, 11, 12, 13));
            break;

        case 56:
            return _mm_shuffle_epi8(x, _mm_setr_epi8(7, 0, 1, 2, 3, 4, 5, 6, 15, 8, 9, 10, 11, 12, 13, 14));
            break;

        default:
            return _mm_or_si128(_mm_srli_epi64(x, n_ror), _mm_slli_epi64(x, 64 - n_ror));
    }
}
//static inline __m128i ror32(const __m128i x, const int n_ror) { return _mm_or_si128(_mm_srli_epi32(x, n_ror), _mm_slli_epi32(x, 32 - n_ror)); }
//static inline __m128i ror16(const __m128i x, const int n_ror) { return _mm_or_si128(_mm_srli_epi16(x, n_ror), _mm_slli_epi16(x, 16 - n_ror)); }
static inline __m128i ror16_optimized(const __m128i x, const int n_ror) {
    switch (n_ror) {
        case 0:
        case 16:
            return x;
            break;

        case 8:
            return vreinterpretq_m128i_u8(vrev16q_u8(vreinterpretq_u8_m128i(x)));
            break;

        default:
        {
            const uint16x8_t v = vreinterpretq_u16_m128i(x);
            switch (n_ror) {
                case 1:  return vreinterpretq_m128i_u16(vsriq_n_u16(vshlq_n_u16(v, 15), v, 1));
                case 5:  return vreinterpretq_m128i_u16(vsriq_n_u16(vshlq_n_u16(v, 11), v, 5));
                case 9:  return vreinterpretq_m128i_u16(vsriq_n_u16(vshlq_n_u16(v, 7), v, 9));
                case 12: return vreinterpretq_m128i_u16(vsriq_n_u16(vshlq_n_u16(v, 4), v, 12));
                case 14: return vreinterpretq_m128i_u16(vsriq_n_u16(vshlq_n_u16(v, 2), v, 14));
                default: return _mm_or_si128(_mm_srli_epi16(x, n_ror), _mm_slli_epi16(x, 16 - n_ror));
            }
        }
    }
}

static inline void MAndRXs(__m128i state_l[6], __m128i state_r[6], const int alpha, const int beta, const int gamma)
{
    for (int i = 0; i < 6; ++i) {
        const __m128i x = state_l[i];
        const __m128i y = state_r[i];
        const __m128i a = ror16_optimized(x, alpha);
        const __m128i b = ror16_optimized(x, beta);
        const __m128i c = ror16_optimized(x, gamma);
        const __m128i r = _mm_xor_si128(_mm_and_si128(a, b), c);
        state_r[i] = _mm_xor_si128(r, y);
    }

    return;
}

static inline void ShiftRows(__m128i state_l[6], __m128i state_r[6])
{
    //#pragma GCC unroll 6
    for (int row = 0; row < 6; ++row) {
        const __m128i x = state_l[row];
        state_l[row] = ror128_optimized(x, 2 * row);
    }

    //#pragma GCC unroll 6
    for (int row = 0; row < 6; ++row) {
        const __m128i x = state_r[row];
        state_r[row] = ror128_optimized(x, 16 - 2 * row);
    }

    return;
}

static inline __m128i mix_row_32x4(__m128i x)
{
    const __m128i pre  = ror128_optimized(x, 32 / 8);
    const __m128i post = ror128_optimized(x, 3 * 32 / 8);

    return _mm_xor_si128(pre, _mm_xor_si128(x, post));
}

static inline void MixRows(__m128i state[12])
{
    state[ 0] = mix_row_32x4(state[ 0]);
    state[ 1] = mix_row_32x4(state[ 1]);

    state[ 2] = mix_row_32x4(state[ 2]);
    state[ 3] = mix_row_32x4(state[ 3]);

    state[ 4] = mix_row_32x4(state[ 4]);
    state[ 5] = mix_row_32x4(state[ 5]);

    state[ 6] = mix_row_32x4(state[ 6]);
    state[ 7] = mix_row_32x4(state[ 7]);

    state[ 8] = mix_row_32x4(state[ 8]);
    state[ 9] = mix_row_32x4(state[ 9]);

    state[10] = mix_row_32x4(state[10]);
    state[11] = mix_row_32x4(state[11]);

    return;
}

static inline void SwapRows(__m128i state[12])
{
    __m128i tmp0 = state[6];
    state[6] = state[7];
    state[7] = tmp0;

    __m128i tmp1 = state[8];
    state[8] = state[9];
    state[9] = tmp1;

    __m128i tmp2 = state[10];
    state[10] = state[11];
    state[11] = tmp2;

    return;
}

static inline void CrossMixs(__m128i state[12])
{
    static const int arrange[6] = {0, 2, 4, 6, 8, 10};
    __m128i tmp[12];
    memcpy(tmp, state, sizeof(tmp));

    //#pragma GCC unroll 6
    for (int j = 0; j < 6; ++j) {
        __m128i a = tmp[arrange[j]];
        __m128i b = tmp[arrange[(j + 1) % 6]];
        __m128i c = tmp[arrange[(j + 4) % 6]];
        state[arrange[j]] = _mm_xor_si128(_mm_xor_si128(a, b), c);
    }

    //#pragma GCC unroll 6
    for (int j = 0; j < 6; ++j) {
        __m128i a = tmp[arrange[j] + 1];
        __m128i b = tmp[arrange[(j + 1) % 6] + 1];
        __m128i c = tmp[arrange[(j + 4) % 6] + 1];
        state[arrange[j] + 1] = _mm_xor_si128(_mm_xor_si128(a, b), c);
    }

    return;
}

static inline void MixColumns(__m128i state[12])
{
    for (int i = 0; i < 2; ++i) {
        __m128i tmp0 = _mm_xor_si128(state[2 + i], state[4 + i]);
        __m128i tmp1 = _mm_xor_si128(state[0 + i], state[4 + i]);
        __m128i tmp2 = _mm_xor_si128(state[0 + i], tmp0);
        state[0 + i] = tmp0;
        state[2 + i] = tmp1;
        state[4 + i] = tmp2;
    }

    for (int i = 0; i < 2; ++i) {
        __m128i tmp0 = _mm_xor_si128(state[6 + i], state[10 + i]);
        __m128i tmp1 = _mm_xor_si128(state[8 + i], state[10 + i]);
        __m128i tmp2 = _mm_xor_si128(state[8 + i], tmp0);
        state[6 + i] = tmp0;
        state[8 + i] = tmp1;
        state[10 + i] = tmp2;
    }

    return;
}

static inline void AddConstant(__m128i state[12], const int rn)
{
    __m128i c = _mm_set_epi64x(
        0x243F6A8885A308D3ULL,  // high 64 bits
        0x13198A2E03707344ULL   // low 64 bits
    );
    __m128i x = _mm_set1_epi16((short)rn);
    state[0] = _mm_xor_si128(_mm_xor_si128(state[0], c), x);

    return;
}

static inline void _core(__m128i state[12], const int n_rounds)
{
    //#pragma GCC unroll 20
    for (int i = 0; i < n_rounds; ++i) {
        CrossMixs(state);
        SwapRows(state);

        MAndRXs(state, state + 6,  0, 1, 8);
        MAndRXs(state + 6, state, 14, 5, 0);
        MAndRXs(state, state + 6, 9, 12, 8);
        MAndRXs(state + 6, state, 14, 5, 0);
        MAndRXs(state, state + 6, 0, 1, 8);
        MAndRXs(state + 6, state, 8, 9, 0);

        CrossMixs(state);
        SwapRows(state);

        MixColumns(state);
        ShiftRows(state, state + 6);
        MixRows(state);

        AddConstant(state, i);
    }

    return;
}

static inline void _core_inverse(__m128i state[12], const int n_rounds)
{
    for (int i = n_rounds - 1; i >= 0; --i) {
        AddConstant(state, i + 9);

        MixRows(state);
        ShiftRows(state + 6, state);
        MixColumns(state);

        SwapRows(state);
        CrossMixs(state);

        MAndRXs(state + 6, state, 8, 9, 0);
        MAndRXs(state, state + 6,  0, 1, 8);
        MAndRXs(state + 6, state, 14, 5, 0);
        MAndRXs(state, state + 6, 9, 12, 8);
        MAndRXs(state + 6, state, 14, 5, 0);
        MAndRXs(state, state + 6, 0, 1, 8);

        __m128i tmp0 = state[0];
        __m128i tmp1 = state[1];
        __m128i tmp2 = state[2];
        __m128i tmp3 = state[3];
        __m128i tmp4 = state[4];
        __m128i tmp5 = state[5];
        state[0] = state[6];
        state[1] = state[7];
        state[2] = state[8];
        state[3] = state[9];
        state[4] = state[10];
        state[5] = state[11];
        state[6] = tmp0;
        state[7] = tmp1;
        state[8] = tmp2;
        state[9] = tmp3;
        state[10] = tmp4;
        state[11] = tmp5;

        SwapRows(state);
        CrossMixs(state);
    }

    return;
}

static inline void zip_prf(__m128i state[12])
{
    __m128i state_copy[12];
    memcpy(state_copy, state, 12 * sizeof(__m128i));

    _core(state, N_ROUNDS / 2);

    _core_inverse(state_copy, N_ROUNDS / 2);

    for (int i = 0; i < 12; ++i)
        state[i] = _mm_xor_si128(state[i], state_copy[i]);

    return;
}

static void mc_transform(__m128i state[12])
{
    zip_prf(state);
    return;
}


static inline void _sponge_absorb_helper(void (*transform)(__m128i *), __m128i state[12], const int block_bits, const unsigned char *message, const uint64_t message_bits)
{
    for (int row = 0; row < 12; ++row)
        state[row] = _mm_setzero_si128();

    const int block_bytes = block_bits / 8;
    const int n_m128_per_block = block_bits / 128; // _m128i reg
    const uint64_t message_bytes = (message_bits + 7) / 8;
    const uint64_t n_block = message_bits / block_bits; // number of full blocks
    uint8_t last_block[136] = {0x00}; // the last and unaligned block, allocate the largest potential needed size, and pad with 0x00

    // copy and turn the last unaligned block to a padded full block
    const uint64_t last_index = n_block * block_bytes;
    memcpy(last_block, message + last_index, message_bytes - last_index);
    pad10star1(last_block, message_bits % block_bits, block_bits);

    for (uint64_t i = 0; i < n_block; ++i) {
        for (int j = 0; j < n_m128_per_block; ++j) {
            __m128i chunk = _mm_loadu_si128((const __m128i*)(message + block_bytes * i + 16 * j));
            state[j] = _mm_xor_si128(state[j], chunk);
        }
        __m128i chunk64 = _mm_loadu_si64(message + block_bytes * i + 16 * n_m128_per_block);
        state[n_m128_per_block] = _mm_xor_si128(state[n_m128_per_block], chunk64);

        transform(state);
    }
    {
        for (int j = 0; j < n_m128_per_block; ++j) {
            __m128i chunk = _mm_loadu_si128((const __m128i*)(last_block + 16 * j));
            state[j] = _mm_xor_si128(state[j], chunk);
        }
        __m128i chunk64 = _mm_loadu_si64(last_block + 16 * n_m128_per_block);
        state[n_m128_per_block] = _mm_xor_si128(state[n_m128_per_block], chunk64);

        transform(state);
    }

    return;
}

static inline void _sponge_squeeze_helper(void (*transform)(__m128i *), __m128i state[12], const int block_bits, unsigned char *digest, const int digest_bits)
{
    const int digest_bytes = digest_bits / 8;
    const int block_bytes = block_bits / 8;
    const int n_block = (digest_bytes + block_bytes - 1) / block_bytes;

    const int n_m128_per_block  = (block_bits  + 128 - 1) / 128; // _m128i reg
    const int n_m128_per_digest = (digest_bits + 128 - 1) / 128; // _m128i reg
    const int n_m128_per_iter   = n_m128_per_block < n_m128_per_digest ? n_m128_per_block : n_m128_per_digest;

    uint8_t buffer[3][768 / 8];

    for (int i = 0; i < n_block; ++i) { // max: 3 (the 1024 case)
        for (int j = 0; j < n_m128_per_iter; ++j) // max: 6 (the 768 case)
            _mm_storeu_si128((__m128i *)(buffer[i] + 16 * j), state[j]);

        if (i + 1 < n_block)
            transform(state);
    }

    if (digest_bytes <= block_bytes) {
        memcpy(digest, buffer[0], digest_bytes);
    } else if (digest_bytes <= 2 * block_bytes) {
        memcpy(digest, buffer[0], block_bytes);
        memcpy(digest + block_bytes, buffer[1], digest_bytes - block_bytes);
    } else if (digest_bytes <= 3 * block_bytes) {
        memcpy(digest, buffer[0], block_bytes);
        memcpy(digest + block_bytes, buffer[1], block_bytes);
        memcpy(digest + 2 * block_bytes, buffer[2], digest_bytes - 2 * block_bytes);
    }

    return;
}

int MasterCube1024_sse(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest)
{
    __m128i state[12] = {0};

    enum {
        digest_bits = 1024, // bits
        block_bits = 448 // bits
    };

    _sponge_absorb_helper(mc_transform, state, block_bits, message, message_bit_len);
    _sponge_squeeze_helper(mc_transform, state, block_bits, digest, digest_bits);

    return 0;
}

#if defined(MCUBE_USE_SSE2NEON)
int MasterCube1024_arm64_neon(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest)
{
    return MasterCube1024_sse(message, message_bit_len, digest);
}
#endif

#endif /* SSE4.2 */


/****************************************************************************
 * AVX2 Implementation
 ****************************************************************************/
#if (!defined (__AVX512F__) || !defined (__AVX512BW__) || !defined (__AVX512VL__)) && defined (__AVX2__)

static const int N_ROUNDS_avx2 = 18;

static inline __m256i ror16_optimized_avx2(const __m256i x, const int n_ror) {
    switch (n_ror) {
        case 0:
        case 16:
            return x;
            break;

        case 8:
            return _mm256_shuffle_epi8(
                x,
                _mm256_setr_epi8(
                    1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14,
                    1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14
                )
            );
            break;

        default:
            return _mm256_or_si256(
                _mm256_srli_epi16(x, n_ror),
                _mm256_slli_epi16(x, 16 - n_ror)
            );
    }
}

static inline void MAndRXs_avx2(__m256i state_l[3], __m256i state_r[3], const int alpha, const int beta, const int gamma)
{
    for (int i = 0; i < 3; ++i) {
        const __m256i x = state_l[i];
        const __m256i y = state_r[i];
        const __m256i a = ror16_optimized_avx2(x, alpha);
        const __m256i b = ror16_optimized_avx2(x, beta);
        const __m256i c = ror16_optimized_avx2(x, gamma);
        const __m256i r = _mm256_xor_si256(_mm256_and_si256(a, b), c);
        state_r[i] = _mm256_xor_si256(r, y);
    }

    return;
}

static inline void ShiftRows_avx2(__m256i state_l[3], __m256i state_r[3])
{
    state_l[0] = _mm256_shuffle_epi8(
        state_l[0],
        _mm256_setr_epi8(
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
            2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1
        )
    );
    state_l[1] = _mm256_shuffle_epi8(
        state_l[1],
        _mm256_setr_epi8(
            4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3,
            6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5
        )
    );
    state_l[2] = _mm256_shuffle_epi8(
        state_l[2],
        _mm256_setr_epi8(
            8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7,
            10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
        )
    );

    state_r[0] = _mm256_shuffle_epi8(
        state_r[0],
        _mm256_setr_epi8(
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
            14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13
        )
    );
    state_r[1] = _mm256_shuffle_epi8(
        state_r[1],
        _mm256_setr_epi8(
            12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
            10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
        )
    );
    state_r[2] = _mm256_shuffle_epi8(
        state_r[2],
        _mm256_setr_epi8(
            8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7,
            6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5
        )
    );

    return;
}

static inline __m256i mix_row_32x4_avx2(__m256i x)
{
    // Rotate left by 32 bits (1 word) in each 128-bit lane
    __m256i pre = _mm256_shuffle_epi32(x, _MM_SHUFFLE_R(1, 2, 3, 0));
    // Rotate left by 96 bits (3 words) in each 128-bit lane
    __m256i post = _mm256_shuffle_epi32(x, _MM_SHUFFLE_R(3, 0, 1, 2));

    return _mm256_xor_si256(pre, _mm256_xor_si256(x, post));
}

static inline void MixRows_avx2(__m256i state[6])
{
    state[0] = mix_row_32x4_avx2(state[0]);
    state[1] = mix_row_32x4_avx2(state[1]);
    state[2] = mix_row_32x4_avx2(state[2]);
    state[3] = mix_row_32x4_avx2(state[3]);
    state[4] = mix_row_32x4_avx2(state[4]);
    state[5] = mix_row_32x4_avx2(state[5]);

    return;
}

static inline void SwapRows_avx2(__m256i state[6])
{
    //#pragma GCC unroll 3
    for (int i = 0; i < 3; ++i) {
        __m256i x = state[i + 3];
        state[i + 3] = _mm256_permute2x128_si256(x, x, 0x01);
    }
    return;
}

static inline void CrossMixs_avx2(__m256i state[6])
{
    const __m256i s0 = state[0];
    const __m256i s1 = state[1];
    const __m256i s2 = state[2];
    const __m256i s3 = state[3];
    const __m256i s4 = state[4];
    const __m256i s5 = state[5];

    const __m256i x03 = _mm256_xor_si256(s0, s3);
    const __m256i x14 = _mm256_xor_si256(s1, s4);
    const __m256i x25 = _mm256_xor_si256(s2, s5);

    state[0] = _mm256_xor_si256(x14, s0);
    state[1] = _mm256_xor_si256(x25, s1);
    state[2] = _mm256_xor_si256(x03, s2);
    state[3] = _mm256_xor_si256(x14, s3);
    state[4] = _mm256_xor_si256(x25, s4);
    state[5] = _mm256_xor_si256(x03, s5);

    return;
}

static inline void MixColumns_avx2(__m256i state[6])
{
    __m256i tmp0 = _mm256_xor_si256(state[1], state[2]);
    __m256i tmp1 = _mm256_xor_si256(state[0], state[2]);
    __m256i tmp2 = _mm256_xor_si256(state[0], tmp0);
    state[0] = tmp0;
    state[1] = tmp1;
    state[2] = tmp2;

    __m256i tmp3 = _mm256_xor_si256(state[3], state[5]);
    __m256i tmp4 = _mm256_xor_si256(state[4], state[5]);
    __m256i tmp5 = _mm256_xor_si256(state[4], tmp3);
    state[3] = tmp3;
    state[4] = tmp4;
    state[5] = tmp5;

    return;
}

static inline void AddConstant_avx2(__m256i state[6], const int rn)
{
    static const alignas(16) uint64_t round_constants[18][2] = {
        {0x13198A2E03707344ULL, 0x243F6A8885A308D3ULL},
        {0x13188A2F03717345ULL, 0x243E6A8985A208D2ULL},
        {0x131B8A2C03727346ULL, 0x243D6A8A85A108D1ULL},
        {0x131A8A2D03737347ULL, 0x243C6A8B85A008D0ULL},
        {0x131D8A2A03747340ULL, 0x243B6A8C85A708D7ULL},
        {0x131C8A2B03757341ULL, 0x243A6A8D85A608D6ULL},
        {0x131F8A2803767342ULL, 0x24396A8E85A508D5ULL},
        {0x131E8A2903777343ULL, 0x24386A8F85A408D4ULL},
        {0x13118A260378734CULL, 0x24376A8085AB08DBULL},
        {0x13108A270379734DULL, 0x24366A8185AA08DAULL},
        {0x13138A24037A734EULL, 0x24356A8285A908D9ULL},
        {0x13128A25037B734FULL, 0x24346A8385A808D8ULL},
        {0x13158A22037C7348ULL, 0x24336A8485AF08DFULL},
        {0x13148A23037D7349ULL, 0x24326A8585AE08DEULL},
        {0x13178A20037E734AULL, 0x24316A8685AD08DDULL},
        {0x13168A21037F734BULL, 0x24306A8785AC08DCULL},
        {0x13098A3E03607354ULL, 0x242F6A9885B308C3ULL},
        {0x13088A3F03617355ULL, 0x242E6A9985B208C2ULL}
    };

    const __m128i c = _mm_load_si128((const __m128i *)round_constants[rn]);
    state[0] = _mm256_xor_si256(state[0], _mm256_zextsi128_si256(c));

    return;
}

static inline void _core_avx2(__m256i state[6], const int n_rounds)
{
    //#pragma GCC unroll 20
    for (int i = 0; i < n_rounds; ++i) {
        CrossMixs_avx2(state);
        SwapRows_avx2(state);

        MAndRXs_avx2(state, state + 3,  0, 1, 8);
        MAndRXs_avx2(state + 3, state, 14, 5, 0);
        MAndRXs_avx2(state, state + 3, 9, 12, 8);
        MAndRXs_avx2(state + 3, state, 14, 5, 0);
        MAndRXs_avx2(state, state + 3,  0, 1, 8);
        MAndRXs_avx2(state + 3, state,  8, 9, 0);

        CrossMixs_avx2(state);
        SwapRows_avx2(state);

        MixColumns_avx2(state);
        ShiftRows_avx2(state, state + 3);
        MixRows_avx2(state);

        AddConstant_avx2(state, i);
    }

    return;
}

static inline void _core_inverse_avx2(__m256i state[6], const int n_rounds)
{
    //#pragma GCC unroll 20
    for (int i = n_rounds - 1; i >= 0; --i) {
        AddConstant_avx2(state, i + 9);

        MixRows_avx2(state);
        ShiftRows_avx2(state + 3, state);
        MixColumns_avx2(state);

        SwapRows_avx2(state);
        CrossMixs_avx2(state);

        MAndRXs_avx2(state + 3, state,  8, 9, 0);
        MAndRXs_avx2(state, state + 3,  0, 1, 8);
        MAndRXs_avx2(state + 3, state, 14, 5, 0);
        MAndRXs_avx2(state, state + 3, 9, 12, 8);
        MAndRXs_avx2(state + 3, state, 14, 5, 0);
        MAndRXs_avx2(state, state + 3,  0, 1, 8);

        for (int j = 0; j < 3; ++j) {
            __m256i tmp = state[j];
            state[j] = state[j + 3];
            state[j + 3] = tmp;
        }

        SwapRows_avx2(state);
        CrossMixs_avx2(state);
    }

    return;
}

static inline void zip_prf_avx2(__m256i state[6])
{
    __m256i state_copy[6];
    memcpy(state_copy, state, 6 * sizeof(__m256i));

    _core_avx2(state, N_ROUNDS_avx2 / 2);

    _core_inverse_avx2(state_copy, N_ROUNDS_avx2 / 2);

    for (int i = 0; i < 6; ++i)
        state[i] = _mm256_xor_si256(state[i], state_copy[i]);

    return;
}

static void mc_transform_avx2(__m256i state[6])
{
    zip_prf_avx2(state);
    return;
}

static inline __m256i loadu_tail24_avx2(const void *p)
{
    const uint8_t *bytes = (const uint8_t *)p;
    const __m128i lo = _mm_loadu_si128((const __m128i *)(const void *)bytes);
    const __m128i hi = _mm_loadl_epi64((const __m128i *)(const void *)(bytes + 16));
    return _mm256_inserti128_si256(_mm256_castsi128_si256(lo), hi, 1);
}

static inline void _sponge_absorb_1024_avx2(__m256i state[6], const unsigned char *message, const uint64_t message_bits)
{
    state[0] = _mm256_setzero_si256();
    state[1] = _mm256_setzero_si256();
    state[2] = _mm256_setzero_si256();
    state[3] = _mm256_setzero_si256();
    state[4] = _mm256_setzero_si256();
    state[5] = _mm256_setzero_si256();

    enum {
        block_bits = 448,
        block_bytes = 56
    };

    const uint64_t message_bytes = (message_bits + 7) / 8;
    const uint64_t n_block = message_bits / block_bits;
    uint8_t last_block[block_bytes] = {0};

    const uint64_t last_index = n_block * block_bytes;
    memcpy(last_block, message + last_index, message_bytes - last_index);
    pad10star1(last_block, message_bits % block_bits, block_bits);

    for (uint64_t i = 0; i < n_block; ++i) {
        const unsigned char *p = message + block_bytes * i;
        state[0] = _mm256_xor_si256(state[0], _mm256_loadu_si256((const __m256i *)(const void *)p));
        state[1] = _mm256_xor_si256(state[1], loadu_tail24_avx2(p + 32));
        mc_transform_avx2(state);
    }

    state[0] = _mm256_xor_si256(state[0], _mm256_loadu_si256((const __m256i *)(const void *)last_block));
    state[1] = _mm256_xor_si256(state[1], loadu_tail24_avx2(last_block + 32));
    mc_transform_avx2(state);

    return;
}


static inline void _sponge_absorb_helper_avx2(void (*transform)(__m256i *), __m256i state[6], const int block_bits, const unsigned char *message, const uint64_t message_bits)
{
    for (int row = 0; row < 6; ++row)
        state[row] = _mm256_setzero_si256();

    const int block_bytes = block_bits / 8;
    const int n_m256_per_block = block_bits / 256; // _m256i reg
    const int m256_bytes = 256 / 8; // _m256i reg
    const uint64_t message_bytes = (message_bits + 7) / 8;
    const uint64_t n_block = message_bits / block_bits; // number of full blocks
    uint8_t last_block[136] = {0x00}; // the last and unaligned block, allocate the largest potential needed size, and pad with 0x00

    // copy and turn the last unaligned block to a padded full block
    const uint64_t last_index = n_block * block_bytes;
    memcpy(last_block, message + last_index, message_bytes - last_index);
    pad10star1(last_block, message_bits % block_bits, block_bits);

    for (uint64_t i = 0; i < n_block; ++i) {
        for (int j = 0; j < n_m256_per_block; ++j) {
            __m256i chunk = _mm256_loadu_si256((const __m256i*)(message + block_bytes * i + m256_bytes * j));
            state[j] = _mm256_xor_si256(state[j], chunk);
        }

        uint8_t unaligned_piece[32] = {0};
        memcpy(unaligned_piece, message + block_bytes * i + m256_bytes * n_m256_per_block, block_bytes - m256_bytes * n_m256_per_block);
        __m256i chunk = _mm256_loadu_si256((const __m256i*)unaligned_piece);
        state[n_m256_per_block] = _mm256_xor_si256(state[n_m256_per_block], chunk);

        transform(state);
    }
    {
        for (int j = 0; j < n_m256_per_block; ++j) {
            __m256i chunk = _mm256_loadu_si256((const __m256i*)(last_block + m256_bytes * j));
            state[j] = _mm256_xor_si256(state[j], chunk);
        }
        uint8_t unaligned_piece[32] = {0};
        memcpy(unaligned_piece, last_block + m256_bytes * n_m256_per_block, block_bytes - m256_bytes * n_m256_per_block);
        __m256i chunk = _mm256_loadu_si256((const __m256i*)unaligned_piece);
        state[n_m256_per_block] = _mm256_xor_si256(state[n_m256_per_block], chunk);

        transform(state);
    }

    return;
}

static inline void _sponge_squeeze_helper_avx2(void (*transform)(__m256i *), __m256i state[6], const int block_bits, unsigned char *digest, const int digest_bits)
{
    const int digest_bytes = digest_bits / 8;
    const int block_bytes = block_bits / 8;
    const int n_block = (digest_bytes + block_bytes - 1) / block_bytes;

    const int n_m256_per_block  = (block_bits  + 256 - 1) / 256; // _m128i reg
    const int n_m256_per_digest = (digest_bits + 256 - 1) / 256; // _m128i reg
    const int n_m256_per_iter   = n_m256_per_block < n_m256_per_digest ? n_m256_per_block : n_m256_per_digest;

    uint8_t buffer[3][768 / 8];

    for (int i = 0; i < n_block; ++i) { // max: 3 (the 1024 case)
        for (int j = 0; j < n_m256_per_iter; ++j) // max: 3 (the 768 case)
            _mm256_storeu_si256((__m256i *)(buffer[i] + 32 * j), state[j]);

        transform(state);
    }

    if (digest_bytes <= block_bytes) {
        memcpy(digest, buffer[0], digest_bytes);
    } else if (digest_bytes <= 2 * block_bytes) {
        memcpy(digest, buffer[0], block_bytes);
        memcpy(digest + block_bytes, buffer[1], digest_bytes - block_bytes);
    } else if (digest_bytes <= 3 * block_bytes) {
        memcpy(digest, buffer[0], block_bytes);
        memcpy(digest + block_bytes, buffer[1], block_bytes);
        memcpy(digest + 2 * block_bytes, buffer[2], digest_bytes - 2 * block_bytes);
    }

    return;
}

static inline void _sponge_squeeze_1024_direct_avx2(void (*transform)(__m256i *), __m256i state[6], unsigned char *digest)
{
    _mm256_storeu_si256((__m256i *)(void *)(digest + 0), state[0]);
    _mm_storeu_si128((__m128i *)(void *)(digest + 32), _mm256_castsi256_si128(state[1]));
    _mm_storel_epi64((__m128i *)(void *)(digest + 48), _mm256_extracti128_si256(state[1], 1));

    transform(state);

    _mm256_storeu_si256((__m256i *)(void *)(digest + 56), state[0]);
    _mm_storeu_si128((__m128i *)(void *)(digest + 88), _mm256_castsi256_si128(state[1]));
    _mm_storel_epi64((__m128i *)(void *)(digest + 104), _mm256_extracti128_si256(state[1], 1));

    transform(state);

    _mm_storeu_si128((__m128i *)(void *)(digest + 112), _mm256_castsi256_si128(state[0]));

    return;
}

int MasterCube1024_avx2(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest)
{
    __m256i state[6] = {0};

    enum {
        digest_bits = 1024, // bits
        block_bits = 448 // bits
    };

    (void)block_bits;
    (void)digest_bits;

    _sponge_absorb_1024_avx2(state, message, message_bit_len);
    _sponge_squeeze_1024_direct_avx2(mc_transform_avx2, state, digest);

    return 0;
}

#endif /* AVX2 */


/****************************************************************************
 * AVX-512 (256-bit) Implementation
 ****************************************************************************/
#if defined (__AVX512F__) && defined (__AVX512BW__) && defined (__AVX512VL__)

static const int N_ROUNDS_a512 = 18;

static inline __m256i ror16_optimized_a512(const __m256i x, const int n_ror)
{
    switch (n_ror) {
        case 0:
        case 16:
            return x;
            break;

        case 8:
            return _mm256_shuffle_epi8(
                x,
                _mm256_setr_epi8(
                    1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14,
                    1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14
                )
            );
            break;

        default:
            return _mm256_or_si256(
                _mm256_srli_epi16(x, n_ror),
                _mm256_slli_epi16(x, 16 - n_ror)
            );
    }
}

static inline void MAndRXs_a512(__m256i state_l[3], __m256i state_r[3], const int alpha, const int beta, const int gamma)
{
    for (int i = 0; i < 3; ++i) {
        const __m256i x = state_l[i];
        const __m256i y = state_r[i];
        const __m256i a = ror16_optimized_a512(x, alpha);
        const __m256i b = ror16_optimized_a512(x, beta);
        const __m256i c = ror16_optimized_a512(x, gamma);
        state_r[i] = _mm256_ternarylogic_epi32(_mm256_xor_si256(c, y), b, a, 0x78);
    }

    return;
}

static inline void ShiftRows_a512(__m256i state_l[3], __m256i state_r[3])
{
    state_l[0] = _mm256_shuffle_epi8(
        state_l[0],
        _mm256_setr_epi8(
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
            2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1
        )
    );
    state_l[1] = _mm256_shuffle_epi8(
        state_l[1],
        _mm256_setr_epi8(
            4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3,
            6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5
        )
    );
    state_l[2] = _mm256_shuffle_epi8(
        state_l[2],
        _mm256_setr_epi8(
            8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7,
            10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
        )
    );

    state_r[0] = _mm256_shuffle_epi8(
        state_r[0],
        _mm256_setr_epi8(
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
            14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13
        )
    );
    state_r[1] = _mm256_shuffle_epi8(
        state_r[1],
        _mm256_setr_epi8(
            12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
            10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
        )
    );
    state_r[2] = _mm256_shuffle_epi8(
        state_r[2],
        _mm256_setr_epi8(
            8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7,
            6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5
        )
    );

    return;
}

static inline __m256i mix_row_32x4_a512(__m256i x)
{
    // Rotate left by 32 bits (1 word) in each 128-bit lane
    __m256i pre = _mm256_shuffle_epi32(x, _MM_SHUFFLE_R(1, 2, 3, 0));
    // Rotate left by 96 bits (3 words) in each 128-bit lane
    __m256i post = _mm256_shuffle_epi32(x, _MM_SHUFFLE_R(3, 0, 1, 2));

    return _mm256_ternarylogic_epi32(pre, x, post, 0x96);
}

static inline void MixRows_a512(__m256i state[6])
{
    state[0] = mix_row_32x4_a512(state[0]);
    state[1] = mix_row_32x4_a512(state[1]);
    state[2] = mix_row_32x4_a512(state[2]);
    state[3] = mix_row_32x4_a512(state[3]);
    state[4] = mix_row_32x4_a512(state[4]);
    state[5] = mix_row_32x4_a512(state[5]);

    return;
}

static inline void SwapRows_a512(__m256i state[6])
{
    //#pragma GCC unroll 3
    for (int i = 0; i < 3; ++i) {
        __m256i x = state[i + 3];
        state[i + 3] = _mm256_permute2x128_si256(x, x, 0x01);
    }
    return;
}

static inline void CrossMixs_a512(__m256i state[6])
{
    static const int arrange[6] = {0, 1, 2, 3, 4, 5};
    __m256i tmp[6];
    memcpy(tmp, state, sizeof(tmp));

    //#pragma GCC unroll 6
    for (int j = 0; j < 6; ++j) {
        __m256i a = tmp[arrange[j]];
        __m256i b = tmp[arrange[(j + 1) % 6]];
        __m256i c = tmp[arrange[(j + 4) % 6]];
        state[arrange[j]] = _mm256_ternarylogic_epi32(a, b, c, 0x96);
    }

    return;
}

static inline void MixColumns_a512(__m256i state[6])
{
    __m256i tmp0 = _mm256_xor_si256(state[1], state[2]);
    __m256i tmp1 = _mm256_xor_si256(state[0], state[2]);
    __m256i tmp2 = _mm256_ternarylogic_epi32(state[0], state[1], state[2], 0x96);
    state[0] = tmp0;
    state[1] = tmp1;
    state[2] = tmp2;

    __m256i tmp3 = _mm256_xor_si256(state[3], state[5]);
    __m256i tmp4 = _mm256_xor_si256(state[4], state[5]);
    __m256i tmp5 = _mm256_ternarylogic_epi32(state[3], state[4], state[5], 0x96);
    state[3] = tmp3;
    state[4] = tmp4;
    state[5] = tmp5;

    return;
}

#define MC_A512_REP16_64(r) (0x0001000100010001ULL * (uint64_t)(r))
#define MC_A512_RC(r) { \
    0x13198A2E03707344ULL ^ MC_A512_REP16_64(r), \
    0x243F6A8885A308D3ULL ^ MC_A512_REP16_64(r), \
    0ULL, \
    0ULL \
}

static const uint64_t MC_A512_ROUND_CONSTANTS[18][4] __attribute__((aligned(32))) = {
    MC_A512_RC(0),  MC_A512_RC(1),  MC_A512_RC(2),  MC_A512_RC(3),  MC_A512_RC(4),  MC_A512_RC(5),
    MC_A512_RC(6),  MC_A512_RC(7),  MC_A512_RC(8),  MC_A512_RC(9),  MC_A512_RC(10), MC_A512_RC(11),
    MC_A512_RC(12), MC_A512_RC(13), MC_A512_RC(14), MC_A512_RC(15), MC_A512_RC(16), MC_A512_RC(17)
};

#undef MC_A512_RC
#undef MC_A512_REP16_64

static inline void AddConstant_a512(__m256i state[6], const int rn)
{
    const __m256i cx = _mm256_load_si256((const __m256i *)(const void *)MC_A512_ROUND_CONSTANTS[rn]);
    state[0] = _mm256_xor_si256(state[0], cx);

    return;
}

static inline void _core_a512(__m256i state[6], const int n_rounds)
{
    //#pragma GCC unroll 20
    for (int i = 0; i < n_rounds; ++i) {
        CrossMixs_a512(state);
        SwapRows_a512(state);

        MAndRXs_a512(state, state + 3,  0, 1, 8);
        MAndRXs_a512(state + 3, state, 14, 5, 0);
        MAndRXs_a512(state, state + 3, 9, 12, 8);
        MAndRXs_a512(state + 3, state, 14, 5, 0);
        MAndRXs_a512(state, state + 3,  0, 1, 8);
        MAndRXs_a512(state + 3, state,  8, 9, 0);

        CrossMixs_a512(state);
        SwapRows_a512(state);

        MixColumns_a512(state);
        ShiftRows_a512(state, state + 3);
        MixRows_a512(state);

        AddConstant_a512(state, i);
    }

    return;
}

static inline void _core_inverse_a512(__m256i state[6], const int n_rounds)
{
    //#pragma GCC unroll 20
    for (int i = n_rounds - 1; i >= 0; --i) {
        AddConstant_a512(state, i + 9);

        MixRows_a512(state);
        ShiftRows_a512(state + 3, state);
        MixColumns_a512(state);

        SwapRows_a512(state);
        CrossMixs_a512(state);

        MAndRXs_a512(state + 3, state,  8, 9, 0);
        MAndRXs_a512(state, state + 3,  0, 1, 8);
        MAndRXs_a512(state + 3, state, 14, 5, 0);
        MAndRXs_a512(state, state + 3, 9, 12, 8);
        MAndRXs_a512(state + 3, state, 14, 5, 0);
        MAndRXs_a512(state, state + 3,  0, 1, 8);

        for (int j = 0; j < 3; ++j) {
            __m256i tmp = state[j];
            state[j] = state[j + 3];
            state[j + 3] = tmp;
        }

        SwapRows_a512(state);
        CrossMixs_a512(state);
    }

    return;
}

static inline void zip_prf_a512(__m256i state[6])
{
    __m256i state_copy[6];
    memcpy(state_copy, state, 6 * sizeof(__m256i));

    _core_a512(state, N_ROUNDS_a512 / 2);

    _core_inverse_a512(state_copy, N_ROUNDS_a512 / 2);

    for (int i = 0; i < 6; ++i)
        state[i] = _mm256_xor_si256(state[i], state_copy[i]);

    return;
}

static void mc_transform_a512(__m256i state[6])
{
    zip_prf_a512(state);
    return;
}


static inline void _sponge_absorb_helper_a512(void (*transform)(__m256i *), __m256i state[6], const int block_bits, const unsigned char *message, const uint64_t message_bits)
{
    for (int row = 0; row < 6; ++row)
        state[row] = _mm256_setzero_si256();

    const int block_bytes = block_bits / 8;
    const int n_m256_per_block = block_bits / 256; // _m256i reg
    const int m256_bytes = 256 / 8; // _m256i reg
    const uint64_t message_bytes = (message_bits + 7) / 8;
    const uint64_t n_block = message_bits / block_bits; // number of full blocks
    uint8_t last_block[136] = {0x00}; // the last and unaligned block, allocate the largest potential needed size, and pad with 0x00

    // copy and turn the last unaligned block to a padded full block
    const uint64_t last_index = n_block * block_bytes;
    memcpy(last_block, message + last_index, message_bytes - last_index);
    pad10star1(last_block, message_bits % block_bits, block_bits);

    for (uint64_t i = 0; i < n_block; ++i) {
        for (int j = 0; j < n_m256_per_block; ++j) {
            __m256i chunk = _mm256_loadu_si256((const __m256i*)(message + block_bytes * i + m256_bytes * j));
            state[j] = _mm256_xor_si256(state[j], chunk);
        }

        const int tail_bytes = block_bytes - m256_bytes * n_m256_per_block;
        const __mmask32 k = (__mmask32)((1u << tail_bytes) - 1u);
        __m256i chunk = _mm256_maskz_loadu_epi8(k, message + block_bytes * i + m256_bytes * n_m256_per_block);
        state[n_m256_per_block] = _mm256_xor_si256(state[n_m256_per_block], chunk);

        transform(state);
    }
    {
        for (int j = 0; j < n_m256_per_block; ++j) {
            __m256i chunk = _mm256_loadu_si256((const __m256i*)(last_block + m256_bytes * j));
            state[j] = _mm256_xor_si256(state[j], chunk);
        }

        const int tail_bytes = block_bytes - m256_bytes * n_m256_per_block;
        const __mmask32 k = (__mmask32)((1u << tail_bytes) - 1u);
        __m256i chunk = _mm256_maskz_loadu_epi8(k, last_block + m256_bytes * n_m256_per_block);
        state[n_m256_per_block] = _mm256_xor_si256(state[n_m256_per_block], chunk);

        transform(state);
    }

    return;
}

static inline void _sponge_squeeze_helper_a512(void (*transform)(__m256i *), __m256i state[6], const int block_bits, unsigned char *digest, const int digest_bits)
{
    const int digest_bytes = digest_bits / 8;
    const int block_bytes = block_bits / 8;
    const int n_block = (digest_bytes + block_bytes - 1) / block_bytes;

    const int n_m256_per_block  = (block_bits  + 256 - 1) / 256; // _m128i reg
    const int n_m256_per_digest = (digest_bits + 256 - 1) / 256; // _m128i reg
    const int n_m256_per_iter   = n_m256_per_block < n_m256_per_digest ? n_m256_per_block : n_m256_per_digest;

    uint8_t buffer[3][768 / 8];

    for (int i = 0; i < n_block; ++i) { // max: 3 (the 1024 case)
        for (int j = 0; j < n_m256_per_iter; ++j) // max: 3 (the 768 case)
            _mm256_storeu_si256((__m256i *)(buffer[i] + 32 * j), state[j]);

        transform(state);
    }

    if (digest_bytes <= block_bytes) {
        memcpy(digest, buffer[0], digest_bytes);
    } else if (digest_bytes <= 2 * block_bytes) {
        memcpy(digest, buffer[0], block_bytes);
        memcpy(digest + block_bytes, buffer[1], digest_bytes - block_bytes);
    } else if (digest_bytes <= 3 * block_bytes) {
        memcpy(digest, buffer[0], block_bytes);
        memcpy(digest + block_bytes, buffer[1], block_bytes);
        memcpy(digest + 2 * block_bytes, buffer[2], digest_bytes - 2 * block_bytes);
    }

    return;
}

static inline void _sponge_squeeze_short_a512(void (*transform)(__m256i *), __m256i state[6], unsigned char *digest)
{
    const __mmask32 k24 = (__mmask32)0x00ffffffu;
    const __mmask32 k16 = (__mmask32)0x0000ffffu;

    _mm256_storeu_si256((__m256i *)(digest + 0), state[0]);
    _mm256_mask_storeu_epi8(digest + 32, k24, state[1]);
    transform(state);
    _mm256_storeu_si256((__m256i *)(digest + 56), state[0]);
    _mm256_mask_storeu_epi8(digest + 88, k24, state[1]);
    transform(state);
    _mm256_mask_storeu_epi8(digest + 112, k16, state[0]);

    return;
}

int MasterCube1024_avx512_256(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest)
{
    __m256i state[6] = {0};

    enum {
        digest_bits = 1024, // bits
        block_bits = 448 // bits
    };

    _sponge_absorb_helper_a512(mc_transform_a512, state, block_bits, message, message_bit_len);
    if (message_bit_len <= 8192ULL)
        _sponge_squeeze_short_a512(mc_transform_a512, state, digest);
    else
        _sponge_squeeze_helper_a512(mc_transform_a512, state, block_bits, digest, digest_bits);

    return 0;
}

#endif /* AVX-512 */

