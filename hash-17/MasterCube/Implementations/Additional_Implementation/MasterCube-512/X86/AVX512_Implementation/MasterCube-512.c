#include <stdalign.h>
#include <stdint.h>
#include <string.h>
#include <immintrin.h>

#include "mastercube.h"
#if defined(__GNUC__) && !defined(__clang__) && defined(__AVX512F__) && !defined(MC_GCC53_COMPAT_H)
static inline __m256i mc_zextsi128_si256_compat(__m128i x)
{
    return _mm256_inserti128_si256(_mm256_setzero_si256(), x, 0);
}
#ifndef _mm256_zextsi128_si256
#define _mm256_zextsi128_si256(x) mc_zextsi128_si256_compat((x))
#endif
#endif

#if defined(__AVX512F__) && defined(__AVX512BW__) && defined(__AVX512VL__)

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
    const __m256i s0 = state[0];
    const __m256i s1 = state[1];
    const __m256i s2 = state[2];
    const __m256i s3 = state[3];
    const __m256i s4 = state[4];
    const __m256i s5 = state[5];

    state[0] = _mm256_ternarylogic_epi32(s0, s1, s4, 0x96);
    state[1] = _mm256_ternarylogic_epi32(s1, s2, s5, 0x96);
    state[2] = _mm256_ternarylogic_epi32(s2, s3, s0, 0x96);
    state[3] = _mm256_ternarylogic_epi32(s3, s4, s1, 0x96);
    state[4] = _mm256_ternarylogic_epi32(s4, s5, s2, 0x96);
    state[5] = _mm256_ternarylogic_epi32(s5, s0, s3, 0x96);

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

static inline void AddConstant_a512(__m256i state[6], const int rn)
{
    const __m128i c = _mm_set_epi64x(
        0x243F6A8885A308D3ULL,  // high 64 bits
        0x13198A2E03707344ULL   // low 64 bits
    );
    const __m256i c256 = _mm256_zextsi128_si256(c);

    const __m128i x = _mm_set1_epi16((short)rn);
    const __m256i x256 = _mm256_zextsi128_si256(x);

    state[0] = _mm256_ternarylogic_epi32(state[0], c256, x256, 0x96);

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

int MasterCube512_avx512_256(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest)
{
    __m256i state[6] = {0};

    enum {
        digest_bits = 512, // bits
        block_bits = 960 // bits
    };

    _sponge_absorb_helper_a512(mc_transform_a512, state, block_bits, message, message_bit_len);
    _sponge_squeeze_helper_a512(mc_transform_a512, state, block_bits, digest, digest_bits);

    return 0;
}

#else
#error "This x86-64 AVX512 implementation requires -mavx512f -mavx512bw -mavx512vl."
#endif /* AVX-512 */

