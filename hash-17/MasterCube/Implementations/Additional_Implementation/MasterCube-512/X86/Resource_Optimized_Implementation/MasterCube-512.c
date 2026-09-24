#include <stdalign.h>
#include <stdint.h>
#include <string.h>
#include <immintrin.h>

#include "mastercube.h"
#if !defined(_mm_loadu_si64) && defined(__AVX2__)
static inline __m128i mc_loadu_si64_compat(const void *p)
{
    return _mm_loadl_epi64((const __m128i *)p);
}
#define _mm_loadu_si64(p) mc_loadu_si64_compat((p))
#endif

#if defined(__GNUC__) && !defined(__clang__) && defined(__AVX2__) && !defined(MC_GCC53_COMPAT_H)
static inline __m256i mc_zextsi128_si256_compat(__m128i x)
{
    return _mm256_inserti128_si256(_mm256_setzero_si256(), x, 0);
}
#ifndef _mm256_zextsi128_si256
#define _mm256_zextsi128_si256(x) mc_zextsi128_si256_compat((x))
#endif
#endif

/****************************************************************************
 * AVX2 Implementation
 ****************************************************************************/
#if defined(__AVX2__)

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
    const __m128i c = _mm_set_epi64x(
        0x243F6A8885A308D3ULL,
        0x13198A2E03707344ULL
    );
    const __m128i r = _mm_set1_epi16((short)rn);
    state[0] = _mm256_xor_si256(state[0], _mm256_zextsi128_si256(_mm_xor_si128(c, r)));

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

static inline void zip_prf_avx2_regs(__m256i state[6])
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

    __m256i a0 = state[0], a1 = state[1], a2 = state[2];
    __m256i a3 = state[3], a4 = state[4], a5 = state[5];
    __m256i b0 = a0, b1 = a1, b2 = a2, b3 = a3, b4 = a4, b5 = a5;

#define MC_MAND(L, R, A, B, G) do { \
    const __m256i mx = (L); \
    (R) = _mm256_xor_si256((R), _mm256_xor_si256(_mm256_and_si256(ror16_optimized_avx2(mx, (A)), ror16_optimized_avx2(mx, (B))), ror16_optimized_avx2(mx, (G)))); \
} while (0)
#define MC_CROSS(S0, S1, S2, S3, S4, S5) do { \
    const __m256i cs0 = (S0), cs1 = (S1), cs2 = (S2), cs3 = (S3), cs4 = (S4), cs5 = (S5); \
    const __m256i x03 = _mm256_xor_si256(cs0, cs3); \
    const __m256i x14 = _mm256_xor_si256(cs1, cs4); \
    const __m256i x25 = _mm256_xor_si256(cs2, cs5); \
    (S0) = _mm256_xor_si256(x14, cs0); \
    (S1) = _mm256_xor_si256(x25, cs1); \
    (S2) = _mm256_xor_si256(x03, cs2); \
    (S3) = _mm256_xor_si256(x14, cs3); \
    (S4) = _mm256_xor_si256(x25, cs4); \
    (S5) = _mm256_xor_si256(x03, cs5); \
} while (0)
#define MC_SWAPROWS(S3, S4, S5) do { \
    (S3) = _mm256_permute2x128_si256((S3), (S3), 0x01); \
    (S4) = _mm256_permute2x128_si256((S4), (S4), 0x01); \
    (S5) = _mm256_permute2x128_si256((S5), (S5), 0x01); \
} while (0)
#define MC_MIXCOL(S0, S1, S2, S3, S4, S5) do { \
    const __m256i m0 = _mm256_xor_si256((S1), (S2)); \
    const __m256i m1 = _mm256_xor_si256((S0), (S2)); \
    const __m256i m2 = _mm256_xor_si256((S0), m0); \
    const __m256i m3 = _mm256_xor_si256((S3), (S5)); \
    const __m256i m4 = _mm256_xor_si256((S4), (S5)); \
    const __m256i m5 = _mm256_xor_si256((S4), m3); \
    (S0) = m0; (S1) = m1; (S2) = m2; (S3) = m3; (S4) = m4; (S5) = m5; \
} while (0)
#define MC_MIXROWS(S0, S1, S2, S3, S4, S5) do { \
    (S0) = mix_row_32x4_avx2((S0)); (S1) = mix_row_32x4_avx2((S1)); (S2) = mix_row_32x4_avx2((S2)); \
    (S3) = mix_row_32x4_avx2((S3)); (S4) = mix_row_32x4_avx2((S4)); (S5) = mix_row_32x4_avx2((S5)); \
} while (0)
#define MC_SHIFT(L0, L1, L2, R0, R1, R2) do { \
    (L0) = _mm256_shuffle_epi8((L0), _mm256_setr_epi8(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,1)); \
    (L1) = _mm256_shuffle_epi8((L1), _mm256_setr_epi8(4,5,6,7,8,9,10,11,12,13,14,15,0,1,2,3,6,7,8,9,10,11,12,13,14,15,0,1,2,3,4,5)); \
    (L2) = _mm256_shuffle_epi8((L2), _mm256_setr_epi8(8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,10,11,12,13,14,15,0,1,2,3,4,5,6,7,8,9)); \
    (R0) = _mm256_shuffle_epi8((R0), _mm256_setr_epi8(0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,14,15,0,1,2,3,4,5,6,7,8,9,10,11,12,13)); \
    (R1) = _mm256_shuffle_epi8((R1), _mm256_setr_epi8(12,13,14,15,0,1,2,3,4,5,6,7,8,9,10,11,10,11,12,13,14,15,0,1,2,3,4,5,6,7,8,9)); \
    (R2) = _mm256_shuffle_epi8((R2), _mm256_setr_epi8(8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7,6,7,8,9,10,11,12,13,14,15,0,1,2,3,4,5)); \
} while (0)
#define MC_ADD(S0, RN) do { \
    const __m128i ac = _mm_load_si128((const __m128i *)round_constants[(RN)]); \
    (S0) = _mm256_xor_si256((S0), _mm256_zextsi128_si256(ac)); \
} while (0)
#define MC_FORWARD(S0, S1, S2, S3, S4, S5, RN) do { \
    MC_CROSS(S0, S1, S2, S3, S4, S5); MC_SWAPROWS(S3, S4, S5); \
    MC_MAND(S0, S3, 0, 1, 8); MC_MAND(S1, S4, 0, 1, 8); MC_MAND(S2, S5, 0, 1, 8); \
    MC_MAND(S3, S0, 14, 5, 0); MC_MAND(S4, S1, 14, 5, 0); MC_MAND(S5, S2, 14, 5, 0); \
    MC_MAND(S0, S3, 9, 12, 8); MC_MAND(S1, S4, 9, 12, 8); MC_MAND(S2, S5, 9, 12, 8); \
    MC_MAND(S3, S0, 14, 5, 0); MC_MAND(S4, S1, 14, 5, 0); MC_MAND(S5, S2, 14, 5, 0); \
    MC_MAND(S0, S3, 0, 1, 8); MC_MAND(S1, S4, 0, 1, 8); MC_MAND(S2, S5, 0, 1, 8); \
    MC_MAND(S3, S0, 8, 9, 0); MC_MAND(S4, S1, 8, 9, 0); MC_MAND(S5, S2, 8, 9, 0); \
    MC_CROSS(S0, S1, S2, S3, S4, S5); MC_SWAPROWS(S3, S4, S5); \
    MC_MIXCOL(S0, S1, S2, S3, S4, S5); MC_SHIFT(S0, S1, S2, S3, S4, S5); MC_MIXROWS(S0, S1, S2, S3, S4, S5); MC_ADD(S0, RN); \
} while (0)
#define MC_INVERSE(S0, S1, S2, S3, S4, S5, RN) do { \
    MC_ADD(S0, (RN) + 9); MC_MIXROWS(S0, S1, S2, S3, S4, S5); MC_SHIFT(S3, S4, S5, S0, S1, S2); MC_MIXCOL(S0, S1, S2, S3, S4, S5); \
    MC_SWAPROWS(S3, S4, S5); MC_CROSS(S0, S1, S2, S3, S4, S5); \
    MC_MAND(S3, S0, 8, 9, 0); MC_MAND(S4, S1, 8, 9, 0); MC_MAND(S5, S2, 8, 9, 0); \
    MC_MAND(S0, S3, 0, 1, 8); MC_MAND(S1, S4, 0, 1, 8); MC_MAND(S2, S5, 0, 1, 8); \
    MC_MAND(S3, S0, 14, 5, 0); MC_MAND(S4, S1, 14, 5, 0); MC_MAND(S5, S2, 14, 5, 0); \
    MC_MAND(S0, S3, 9, 12, 8); MC_MAND(S1, S4, 9, 12, 8); MC_MAND(S2, S5, 9, 12, 8); \
    MC_MAND(S3, S0, 14, 5, 0); MC_MAND(S4, S1, 14, 5, 0); MC_MAND(S5, S2, 14, 5, 0); \
    MC_MAND(S0, S3, 0, 1, 8); MC_MAND(S1, S4, 0, 1, 8); MC_MAND(S2, S5, 0, 1, 8); \
    { const __m256i t0 = (S0), t1 = (S1), t2 = (S2); (S0) = (S3); (S1) = (S4); (S2) = (S5); (S3) = t0; (S4) = t1; (S5) = t2; } \
    MC_SWAPROWS(S3, S4, S5); MC_CROSS(S0, S1, S2, S3, S4, S5); \
} while (0)

    for (int i = 0; i < 9; ++i)
        MC_FORWARD(a0, a1, a2, a3, a4, a5, i);
    for (int i = 8; i >= 0; --i)
        MC_INVERSE(b0, b1, b2, b3, b4, b5, i);

    state[0] = _mm256_xor_si256(a0, b0);
    state[1] = _mm256_xor_si256(a1, b1);
    state[2] = _mm256_xor_si256(a2, b2);
    state[3] = _mm256_xor_si256(a3, b3);
    state[4] = _mm256_xor_si256(a4, b4);
    state[5] = _mm256_xor_si256(a5, b5);

#undef MC_INVERSE
#undef MC_FORWARD
#undef MC_ADD
#undef MC_SHIFT
#undef MC_MIXROWS
#undef MC_MIXCOL
#undef MC_SWAPROWS
#undef MC_CROSS
#undef MC_MAND
}
static void mc_transform_avx2(__m256i state[6])
{
    zip_prf_avx2_regs(state);
    return;
}

static inline __m256i loadu_tail24_avx2(const void *p)
{
    const char *q = (const char *)p;
    const __m128i lo = _mm_loadu_si128((const __m128i *)(const void *)q);
    const __m128i hi = _mm_loadl_epi64((const __m128i *)(const void *)(q + 16));
    return _mm256_inserti128_si256(_mm256_castsi128_si256(lo), hi, 1);
}

static inline void _sponge_absorb_512_avx2(__m256i state[6], const unsigned char *message, const uint64_t message_bits)
{
    for (int row = 0; row < 6; ++row)
        state[row] = _mm256_setzero_si256();

    enum { block_bits = 960, block_bytes = 120 };
    const uint64_t message_bytes = (message_bits + 7) / 8;
    const uint64_t n_block = message_bits / block_bits;
    uint8_t last_block[block_bytes] = {0};
    const uint64_t last_index = n_block * block_bytes;
    memcpy(last_block, message + last_index, message_bytes - last_index);
    pad10star1(last_block, message_bits % block_bits, block_bits);

    for (uint64_t i = 0; i < n_block; ++i) {
        const unsigned char *p = message + block_bytes * i;
        state[0] = _mm256_xor_si256(state[0], _mm256_loadu_si256((const __m256i *)(const void *)(p + 0)));
        state[1] = _mm256_xor_si256(state[1], _mm256_loadu_si256((const __m256i *)(const void *)(p + 32)));
        state[2] = _mm256_xor_si256(state[2], _mm256_loadu_si256((const __m256i *)(const void *)(p + 64)));
        state[3] = _mm256_xor_si256(state[3], loadu_tail24_avx2(p + 96));
        mc_transform_avx2(state);
    }

    state[0] = _mm256_xor_si256(state[0], _mm256_loadu_si256((const __m256i *)(const void *)(last_block + 0)));
    state[1] = _mm256_xor_si256(state[1], _mm256_loadu_si256((const __m256i *)(const void *)(last_block + 32)));
    state[2] = _mm256_xor_si256(state[2], _mm256_loadu_si256((const __m256i *)(const void *)(last_block + 64)));
    state[3] = _mm256_xor_si256(state[3], loadu_tail24_avx2(last_block + 96));
    mc_transform_avx2(state);
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

        __m256i chunk = loadu_tail24_avx2(message + block_bytes * i + m256_bytes * n_m256_per_block);
        state[n_m256_per_block] = _mm256_xor_si256(state[n_m256_per_block], chunk);

        transform(state);
    }
    {
        for (int j = 0; j < n_m256_per_block; ++j) {
            __m256i chunk = _mm256_loadu_si256((const __m256i*)(last_block + m256_bytes * j));
            state[j] = _mm256_xor_si256(state[j], chunk);
        }
        __m256i chunk = loadu_tail24_avx2(last_block + m256_bytes * n_m256_per_block);
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

static inline void _sponge_squeeze_512_direct_avx2(__m256i state[6], unsigned char *digest)
{
    _mm256_storeu_si256((__m256i *)(void *)(digest + 0), state[0]);
    _mm256_storeu_si256((__m256i *)(void *)(digest + 32), state[1]);

    return;
}

int MasterCube512_avx2(const unsigned char *message, unsigned long long message_bit_len, unsigned char *digest)
{
    __m256i state[6] = {0};

    enum {
        digest_bits = 512, // bits
        block_bits = 960 // bits
    };

    _sponge_absorb_512_avx2(state, message, message_bit_len);
    _sponge_squeeze_512_direct_avx2(state, digest);

    return 0;
}

#else
#error "This x86-64 implementation requires AVX2. Compile with -mavx2."
#endif /* AVX2 */

