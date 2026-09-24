#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if !defined(W)
#error "W must be defined before including llh_v2_core.c"
#endif

#if defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)
#define LLH_X86_INTRINSICS 1
#include <immintrin.h>
#else
#define LLH_X86_INTRINSICS 0
#endif

#if defined(_MSC_VER)
#define ALIGN32 __declspec(align(32))
#define ALIGN64 __declspec(align(64))
#define LLH_UNUSED
#else
#define ALIGN32 __attribute__((aligned(32)))
#define ALIGN64 __attribute__((aligned(64)))
#define LLH_UNUSED __attribute__((unused))
#endif

#if LLH_X86_INTRINSICS && (defined(__GNUC__) || defined(__clang__))
#define LLH_ENABLE_AVX512_I2 1
#define LLH_TARGET_AVX512 __attribute__((target("avx512f,avx512dq,avx512bw,avx512vl")))
#else
#define LLH_ENABLE_AVX512_I2 0
#define LLH_TARGET_AVX512
#endif

#if defined(__clang__)
#define LLH_PRAGMA_NO_UNROLL _Pragma("clang loop unroll(disable)")
#define LLH_PRAGMA_UNROLL_4 _Pragma("clang loop unroll_count(4)")
#elif defined(__GNUC__)
#define LLH_PRAGMA_NO_UNROLL _Pragma("GCC unroll 1")
#define LLH_PRAGMA_UNROLL_4 _Pragma("GCC unroll 4")
#else
#define LLH_PRAGMA_NO_UNROLL
#define LLH_PRAGMA_UNROLL_4
#endif

#if defined(__GNUC__) || defined(__clang__)
#define LLH_ALWAYS_INLINE __attribute__((always_inline)) inline
#else
#define LLH_ALWAYS_INLINE inline
#endif

#ifndef LLH_FORCE_AVX2
#define LLH_FORCE_AVX2 0
#endif

static int LLH_UNUSED llh_runtime_has_avx512_i2(void)
{
#if LLH_FORCE_AVX2
    return 0;
#elif LLH_ENABLE_AVX512_I2
    static int cached = -1;

    if (cached >= 0) {
        return cached;
    }

    __builtin_cpu_init();
    cached = __builtin_cpu_supports("avx512f") &&
             __builtin_cpu_supports("avx512dq") &&
             __builtin_cpu_supports("avx512bw") &&
             __builtin_cpu_supports("avx512vl");
    return cached;
#else
    return 0;
#endif
}

#define ROWS 24
#define RATE_ROWS 8
#define CAP_ROWS 16

typedef uint32_t word_t;

static inline word_t llh_rotl(word_t x, unsigned int n)
{
    return (word_t)((x << n) | (x >> (32u - n)));
}

#define WORD_NOT(a) (~(a))
#define WORD_XOR(a, b) ((a) ^ (b))
#define WORD_AND(a, b) ((a) & (b))


#define RATE_BITS (RATE_ROWS * W)
#define RATE_BYTES (RATE_BITS / 8)
#define OUTPUT_BYTES RATE_BYTES
#define NR_DEFAULT (8 + (W / 32) * 8)

typedef struct {
    word_t row[ROWS];
} State;

static const int SIGMA[ROWS] = {
    3, 16, 13, 26, 8, 29, 2, 23,
    21, 18, 31, 28, 10, 15, 4, 9,
    7, 20, 17, 30, 12, 1, 6, 27
};

static const word_t RC[16] = {
    0xB7E15162u, 0x8AED2A6Au, 0xBF715880u, 0x9CF4F3C7u,
    0x62E7160Fu, 0x38B4DA56u, 0xA784D904u, 0x5190CFEFu,
    0x324E7738u, 0x926CFBE5u, 0xF4BF8D8Du, 0x8C31D763u,
    0xDA06C80Au, 0xBB1185EBu, 0x4F7C7B57u, 0x57F59584u
};


#define LLH_LOAD_U32_LE(src)                                                  \
    ((uint32_t)((src)[0]) | ((uint32_t)((src)[1]) << 8) |                    \
     ((uint32_t)((src)[2]) << 16) | ((uint32_t)((src)[3]) << 24))

#define LLH_STORE_U32_LE(dst, value)                                          \
    do {                                                                      \
        uint8_t *llh_dst_ = (dst);                                            \
        uint32_t llh_value_ = (uint32_t)(value);                              \
        llh_dst_[0] = (uint8_t)llh_value_;                                    \
        llh_dst_[1] = (uint8_t)(llh_value_ >> 8);                             \
        llh_dst_[2] = (uint8_t)(llh_value_ >> 16);                            \
        llh_dst_[3] = (uint8_t)(llh_value_ >> 24);                            \
    } while (0)

#define load_word(p) LLH_LOAD_U32_LE(p)
#define store_word(p, w) LLH_STORE_U32_LE((p), (w))

typedef struct {
    State state;
    uint8_t buf[RATE_BYTES];
    size_t buf_len;
} HashCtx;

#define LLH_MIX_FAMILY(dst, a, b, c, d)                                 \
    do {                                                                \
        const word_t llh_ab_ = WORD_XOR(T[(a)], T[(b)]);               \
        const word_t llh_cd_ = WORD_XOR(T[(c)], T[(d)]);               \
        state[(dst) + 0] = WORD_XOR(T[(b)], llh_cd_);                  \
        state[(dst) + 1] = WORD_XOR(T[(a)], llh_cd_);                  \
        state[(dst) + 2] = WORD_XOR(llh_ab_, T[(d)]);                  \
        state[(dst) + 3] = WORD_XOR(llh_ab_, T[(c)]);                  \
    } while (0)

static void llh_round(word_t *state, word_t rc)
{
    word_t T[ROWS];

    T[0] = WORD_NOT(WORD_XOR(state[0], WORD_XOR(state[8], WORD_AND(state[4], state[12]))));
    T[4] = WORD_NOT(WORD_XOR(state[4], WORD_XOR(state[12], WORD_AND(state[8], state[16]))));
    T[8] = WORD_XOR(state[8], WORD_XOR(state[16], WORD_AND(state[12], state[20])));
    T[12] = WORD_NOT(WORD_XOR(state[12], WORD_XOR(state[20], WORD_AND(state[16], state[0]))));
    T[16] = WORD_NOT(WORD_XOR(state[16], WORD_XOR(state[0], WORD_AND(state[20], state[4]))));
    T[20] = WORD_XOR(state[20], WORD_XOR(state[4], WORD_AND(state[0], state[8])));

    T[1] = WORD_NOT(WORD_XOR(state[1], WORD_XOR(state[9], WORD_AND(state[5], state[13]))));
    T[5] = WORD_NOT(WORD_XOR(state[5], WORD_XOR(state[13], WORD_AND(state[9], state[17]))));
    T[9] = WORD_XOR(state[9], WORD_XOR(state[17], WORD_AND(state[13], state[21])));
    T[13] = WORD_NOT(WORD_XOR(state[13], WORD_XOR(state[21], WORD_AND(state[17], state[1]))));
    T[17] = WORD_NOT(WORD_XOR(state[17], WORD_XOR(state[1], WORD_AND(state[21], state[5]))));
    T[21] = WORD_XOR(state[21], WORD_XOR(state[5], WORD_AND(state[1], state[9])));

    T[2] = WORD_NOT(WORD_XOR(state[2], WORD_XOR(state[10], WORD_AND(state[6], state[14]))));
    T[6] = WORD_NOT(WORD_XOR(state[6], WORD_XOR(state[14], WORD_AND(state[10], state[18]))));
    T[10] = WORD_XOR(state[10], WORD_XOR(state[18], WORD_AND(state[14], state[22])));
    T[14] = WORD_NOT(WORD_XOR(state[14], WORD_XOR(state[22], WORD_AND(state[18], state[2]))));
    T[18] = WORD_NOT(WORD_XOR(state[18], WORD_XOR(state[2], WORD_AND(state[22], state[6]))));
    T[22] = WORD_XOR(state[22], WORD_XOR(state[6], WORD_AND(state[2], state[10])));

    T[3] = WORD_NOT(WORD_XOR(state[3], WORD_XOR(state[11], WORD_AND(state[7], state[15]))));
    T[7] = WORD_NOT(WORD_XOR(state[7], WORD_XOR(state[15], WORD_AND(state[11], state[19]))));
    T[11] = WORD_XOR(state[11], WORD_XOR(state[19], WORD_AND(state[15], state[23])));
    T[15] = WORD_NOT(WORD_XOR(state[15], WORD_XOR(state[23], WORD_AND(state[19], state[3]))));
    T[19] = WORD_NOT(WORD_XOR(state[19], WORD_XOR(state[3], WORD_AND(state[23], state[7]))));
    T[23] = WORD_XOR(state[23], WORD_XOR(state[7], WORD_AND(state[3], state[11])));

    LLH_MIX_FAMILY(0, 4, 9, 18, 23);
    LLH_MIX_FAMILY(4, 8, 1, 22, 15);
    LLH_MIX_FAMILY(8, 12, 17, 6, 3);
    LLH_MIX_FAMILY(12, 20, 5, 10, 19);
    LLH_MIX_FAMILY(16, 0, 21, 14, 11);
    LLH_MIX_FAMILY(20, 16, 13, 2, 7);

    for (int i = 0; i < ROWS; i++) {
        state[i] = llh_rotl(state[i], (unsigned int)SIGMA[i]);
    }
    state[23] ^= rc;
}

static void permutation(State *s)
{
    for (int round = 0; round < NR_DEFAULT; round++) {
        llh_round(s->row, RC[round]);
    }
}

#if LLH_X86_INTRINSICS
#if LLH_ENABLE_AVX512_I2
typedef struct {
    uint32_t lanes[8];
} u32x8_consts_t;

#define U32X8_CONSTS(n0, n1, n2, n3, n4, n5, n6, n7) {                           \
    {                                                                            \
        (uint32_t)(n0), (uint32_t)(n1), (uint32_t)(n2), (uint32_t)(n3),         \
        (uint32_t)(n4), (uint32_t)(n5), (uint32_t)(n6), (uint32_t)(n7)          \
    }                                                                            \
}

#define LLH_AVX512_W32_TERNLOG_XOR3 0x96
#define LLH_AVX512_W32_TERNLOG_XOR_AND 0x6C


static const ALIGN32 u32x8_consts_t SHIFTROWS_AVX512_W32[4] = {
    U32X8_CONSTS(3,     8,  21, 10, 7,  12, 0, 0),
    U32X8_CONSTS(16,    29, 18, 15, 20, 1,  0, 0),
    U32X8_CONSTS(13,    2,  31, 4,  17, 6,  0, 0),
    U32X8_CONSTS(26,    23, 28, 9,  30, 27, 0, 0)
};

static const ALIGN32 u32x8_consts_t BUILD_PERM_AVX512_W32[4] = {
    U32X8_CONSTS(1, 2, 3, 5, 0, 4, 6, 7),
    U32X8_CONSTS(2, 0, 4, 1, 5, 3, 6, 7),
    U32X8_CONSTS(4, 5, 1, 2, 3, 0, 6, 7),
    U32X8_CONSTS(5, 3, 0, 4, 2, 1, 6, 7)
};

static const ALIGN32 u32x8_consts_t FAMILY_ROT_AVX512_W32[3] = {
    U32X8_CONSTS(1, 2, 3, 4, 5, 0, 6, 7),
    U32X8_CONSTS(2, 3, 4, 5, 0, 1, 6, 7),
    U32X8_CONSTS(3, 4, 5, 0, 1, 2, 6, 7)
};

static const ALIGN32 u32x8_consts_t COMPLEMENT_MASK_AVX512_W32 =
    U32X8_CONSTS(0xFFFFFFFFu, 0xFFFFFFFFu, 0u, 0xFFFFFFFFu, 0xFFFFFFFFu, 0u, 0u, 0u);

static const ALIGN32 u32x8_consts_t CAPACITY_MASK_AVX512_W32 =
    U32X8_CONSTS(0u, 0u, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0u, 0u);

#define LLH_W32_RC_LIST(X) \
    X(B7E15162)            \
    X(8AED2A6A)            \
    X(BF715880)            \
    X(9CF4F3C7)            \
    X(62E7160F)            \
    X(38B4DA56)            \
    X(A784D904)            \
    X(5190CFEF)            \
    X(324E7738)            \
    X(926CFBE5)            \
    X(F4BF8D8D)            \
    X(8C31D763)            \
    X(DA06C80A)            \
    X(BB1185EB)            \
    X(4F7C7B57)            \
    X(57F59584)

#define LLH_W32_RC_ENTRY_AVX512(v) U32X8_CONSTS(0u, 0u, 0u, 0u, 0u, 0x##v##u, 0u, 0u),

static const ALIGN32 u32x8_consts_t RC_AVX512_W32[16] = {
    LLH_W32_RC_LIST(LLH_W32_RC_ENTRY_AVX512)
};

#undef LLH_W32_RC_ENTRY_AVX512
#undef LLH_W32_RC_LIST

#define avx2_load_u32x8(table) \
    _mm256_load_si256((const __m256i *)(const void *)((table)->lanes))

static LLH_ALWAYS_INLINE __m256i rotl32x8_avx2(
    __m256i v,
    __m256i counts,
    __m256i shift_32)
{
    return _mm256_or_si256(
        _mm256_sllv_epi32(v, counts),
        _mm256_srlv_epi32(v, _mm256_sub_epi32(shift_32, counts)));
}

#define avx2_xor3_epi32(a, b, c) \
    _mm256_xor_si256(_mm256_xor_si256((a), (b)), (c))

#define avx2_xorand_epi32(a, b, c) \
    _mm256_xor_si256((b), _mm256_and_si256((a), (c)))

static LLH_ALWAYS_INLINE __m256i avx2_boolean_column(
    __m256i column,
    __m256i rot1_perm,
    __m256i rot2_perm,
    __m256i rot3_perm,
    __m256i complement_mask)
{
    const __m256i rot1 = _mm256_permutevar8x32_epi32(column, rot1_perm);
    const __m256i rot2 = _mm256_permutevar8x32_epi32(column, rot2_perm);
    const __m256i rot3 = _mm256_permutevar8x32_epi32(column, rot3_perm);
    const __m256i mixed = avx2_xorand_epi32(rot1, rot2, rot3);

    return avx2_xor3_epi32(column, mixed, complement_mask);
}

#define avx2_build_column(v, build_perm) \
    _mm256_permutevar8x32_epi32((v), (build_perm))

#define avx2_shiftrows_column(v, counts, shift_32) \
    rotl32x8_avx2((v), (counts), (shift_32))

#define avx2_load_rc(round_idx) avx2_load_u32x8(&RC_AVX512_W32[(round_idx)])

static LLH_ALWAYS_INLINE void avx2_init_state_columns_i2(
    __m256i *c0,
    __m256i *c1,
    __m256i *c2,
    __m256i *c3,
    uint64_t msg_len_bits)
{
    const __m256i zero = _mm256_setzero_si256();
    const uint32_t msg_len_hi = (uint32_t)(msg_len_bits >> 32);
    const uint32_t msg_len_lo = (uint32_t)msg_len_bits;

    *c0 = zero;
    *c1 = zero;
    *c2 = _mm256_setr_epi32(0, 0, 0, 0, 0, (int)msg_len_lo, 0, 0);
    *c3 = _mm256_setr_epi32(0, 0, 0, 0, 0, (int)msg_len_hi, 0, 0);
}

static LLH_ALWAYS_INLINE void avx2_load_padded_rate_block_w32(
    const uint8_t *src,
    size_t valid_bytes,
    __m256i *block_words)
{
    ALIGN32 uint8_t padded[RATE_BYTES];

    memset(padded, 0xFF, sizeof(padded));
    if (valid_bytes != 0) {
        memcpy(padded, src, valid_bytes);
    }

    *block_words = _mm256_load_si256((const __m256i *)(const void *)padded);
}

static LLH_ALWAYS_INLINE void avx2_load_state_columns(
    const word_t *state,
    __m256i *c0,
    __m256i *c1,
    __m256i *c2,
    __m256i *c3)
{
    *c0 = _mm256_setr_epi32(
        (int)state[0],
        (int)state[4],
        (int)state[8],
        (int)state[12],
        (int)state[16],
        (int)state[20],
        0,
        0);
    *c1 = _mm256_setr_epi32(
        (int)state[1],
        (int)state[5],
        (int)state[9],
        (int)state[13],
        (int)state[17],
        (int)state[21],
        0,
        0);
    *c2 = _mm256_setr_epi32(
        (int)state[2],
        (int)state[6],
        (int)state[10],
        (int)state[14],
        (int)state[18],
        (int)state[22],
        0,
        0);
    *c3 = _mm256_setr_epi32(
        (int)state[3],
        (int)state[7],
        (int)state[11],
        (int)state[15],
        (int)state[19],
        (int)state[23],
        0,
        0);
}

static LLH_ALWAYS_INLINE void avx2_store_state_columns(
    word_t *state,
    __m256i c0,
    __m256i c1,
    __m256i c2,
    __m256i c3)
{
    ALIGN32 uint32_t tmp0[8];
    ALIGN32 uint32_t tmp1[8];
    ALIGN32 uint32_t tmp2[8];
    ALIGN32 uint32_t tmp3[8];

    _mm256_store_si256((__m256i *)(void *)tmp0, c0);
    _mm256_store_si256((__m256i *)(void *)tmp1, c1);
    _mm256_store_si256((__m256i *)(void *)tmp2, c2);
    _mm256_store_si256((__m256i *)(void *)tmp3, c3);

    state[0] = tmp0[0];
    state[4] = tmp0[1];
    state[8] = tmp0[2];
    state[12] = tmp0[3];
    state[16] = tmp0[4];
    state[20] = tmp0[5];

    state[1] = tmp1[0];
    state[5] = tmp1[1];
    state[9] = tmp1[2];
    state[13] = tmp1[3];
    state[17] = tmp1[4];
    state[21] = tmp1[5];

    state[2] = tmp2[0];
    state[6] = tmp2[1];
    state[10] = tmp2[2];
    state[14] = tmp2[3];
    state[18] = tmp2[4];
    state[22] = tmp2[5];

    state[3] = tmp3[0];
    state[7] = tmp3[1];
    state[11] = tmp3[2];
    state[15] = tmp3[3];
    state[19] = tmp3[4];
    state[23] = tmp3[5];
}

static LLH_ALWAYS_INLINE void avx2_round_i2(
    __m256i *c0,
    __m256i *c1,
    __m256i *c2,
    __m256i *c3,
    __m256i rc_vec,
    __m256i build0,
    __m256i build1,
    __m256i build2,
    __m256i build3,
    __m256i rot1_perm,
    __m256i rot2_perm,
    __m256i rot3_perm,
    __m256i shift0,
    __m256i shift1,
    __m256i shift2,
    __m256i shift3,
    __m256i complement_mask,
    __m256i shift_32)
{
    const __m256i b0 = avx2_boolean_column(*c0, rot1_perm, rot2_perm, rot3_perm, complement_mask);
    const __m256i b1 = avx2_boolean_column(*c1, rot1_perm, rot2_perm, rot3_perm, complement_mask);
    const __m256i b2 = avx2_boolean_column(*c2, rot1_perm, rot2_perm, rot3_perm, complement_mask);
    const __m256i b3 = avx2_boolean_column(*c3, rot1_perm, rot2_perm, rot3_perm, complement_mask);
    const __m256i f0 = avx2_build_column(b0, build0);
    const __m256i f1 = avx2_build_column(b1, build1);
    const __m256i f2 = avx2_build_column(b2, build2);
    const __m256i f3 = avx2_build_column(b3, build3);

    *c0 = avx2_shiftrows_column(avx2_xor3_epi32(f1, f2, f3), shift0, shift_32);
    *c1 = avx2_shiftrows_column(avx2_xor3_epi32(f0, f2, f3), shift1, shift_32);
    *c2 = avx2_shiftrows_column(avx2_xor3_epi32(f0, f1, f3), shift2, shift_32);
    *c3 = _mm256_xor_si256(
        avx2_shiftrows_column(avx2_xor3_epi32(f0, f1, f2), shift3, shift_32),
        rc_vec);
}

static LLH_ALWAYS_INLINE void avx2_permute_columns_i2(
    __m256i *c0,
    __m256i *c1,
    __m256i *c2,
    __m256i *c3)
{
    __m256i x0 = *c0;
    __m256i x1 = *c1;
    __m256i x2 = *c2;
    __m256i x3 = *c3;
    const __m256i build0 = avx2_load_u32x8(&BUILD_PERM_AVX512_W32[0]);
    const __m256i build1 = avx2_load_u32x8(&BUILD_PERM_AVX512_W32[1]);
    const __m256i build2 = avx2_load_u32x8(&BUILD_PERM_AVX512_W32[2]);
    const __m256i build3 = avx2_load_u32x8(&BUILD_PERM_AVX512_W32[3]);
    const __m256i rot1_perm = avx2_load_u32x8(&FAMILY_ROT_AVX512_W32[0]);
    const __m256i rot2_perm = avx2_load_u32x8(&FAMILY_ROT_AVX512_W32[1]);
    const __m256i rot3_perm = avx2_load_u32x8(&FAMILY_ROT_AVX512_W32[2]);
    const __m256i shift0 = avx2_load_u32x8(&SHIFTROWS_AVX512_W32[0]);
    const __m256i shift1 = avx2_load_u32x8(&SHIFTROWS_AVX512_W32[1]);
    const __m256i shift2 = avx2_load_u32x8(&SHIFTROWS_AVX512_W32[2]);
    const __m256i shift3 = avx2_load_u32x8(&SHIFTROWS_AVX512_W32[3]);
    const __m256i complement_mask = avx2_load_u32x8(&COMPLEMENT_MASK_AVX512_W32);
    const __m256i shift_32 = _mm256_set1_epi32(32);

    for (size_t round_idx = 0; round_idx < NR_DEFAULT; round_idx++) {
        avx2_round_i2(
            &x0,
            &x1,
            &x2,
            &x3,
            avx2_load_rc(round_idx),
            build0,
            build1,
            build2,
            build3,
            rot1_perm,
            rot2_perm,
            rot3_perm,
            shift0,
            shift1,
            shift2,
            shift3,
            complement_mask,
            shift_32);
    }

    *c0 = x0;
    *c1 = x1;
    *c2 = x2;
    *c3 = x3;
}

static LLH_ALWAYS_INLINE void avx2_xor_rate_columns(
    __m256i *c0,
    __m256i *c1,
    __m256i *c2,
    __m256i *c3,
    __m256i block_words)
{
    const __m128i zero = _mm_setzero_si128();
    const __m128i lo = _mm256_castsi256_si128(block_words);
    const __m128i hi = _mm256_extracti128_si256(block_words, 1);
    const __m128i pair01 = _mm_unpacklo_epi32(lo, hi);
    const __m128i pair23 = _mm_unpackhi_epi32(lo, hi);
    const __m128i col0_lo = _mm_unpacklo_epi64(pair01, zero);
    const __m128i col1_lo = _mm_unpackhi_epi64(pair01, zero);
    const __m128i col2_lo = _mm_unpacklo_epi64(pair23, zero);
    const __m128i col3_lo = _mm_unpackhi_epi64(pair23, zero);

    *c0 = _mm256_xor_si256(*c0, _mm256_zextsi128_si256(col0_lo));
    *c1 = _mm256_xor_si256(*c1, _mm256_zextsi128_si256(col1_lo));
    *c2 = _mm256_xor_si256(*c2, _mm256_zextsi128_si256(col2_lo));
    *c3 = _mm256_xor_si256(*c3, _mm256_zextsi128_si256(col3_lo));
}

static LLH_ALWAYS_INLINE void avx2_process_prepared_block_w32(
    __m256i *c0,
    __m256i *c1,
    __m256i *c2,
    __m256i *c3,
    __m256i block_words,
    __m256i capacity_mask)
{
    const __m256i prev0 = *c0;
    const __m256i prev1 = *c1;
    const __m256i prev2 = *c2;
    const __m256i prev3 = *c3;

    avx2_permute_columns_i2(c0, c1, c2, c3);

    *c0 = _mm256_xor_si256(*c0, _mm256_and_si256(prev0, capacity_mask));
    *c1 = _mm256_xor_si256(*c1, _mm256_and_si256(prev1, capacity_mask));
    *c2 = _mm256_xor_si256(*c2, _mm256_and_si256(prev2, capacity_mask));
    *c3 = _mm256_xor_si256(*c3, _mm256_and_si256(prev3, capacity_mask));

    avx2_xor_rate_columns(c0, c1, c2, c3, block_words);
}

static LLH_ALWAYS_INLINE void avx2_squeeze_rate_columns(
    __m256i c0,
    __m256i c1,
    __m256i c2,
    __m256i c3,
    uint8_t *out)
{
    const __m128i c0_lo = _mm256_castsi256_si128(c0);
    const __m128i c1_lo = _mm256_castsi256_si128(c1);
    const __m128i c2_lo = _mm256_castsi256_si128(c2);
    const __m128i c3_lo = _mm256_castsi256_si128(c3);
    const __m128i lo01 = _mm_unpacklo_epi32(c0_lo, c1_lo);
    const __m128i lo23 = _mm_unpacklo_epi32(c2_lo, c3_lo);
    const __m128i out_lo = _mm_unpacklo_epi64(lo01, lo23);
    const __m128i out_hi = _mm_unpackhi_epi64(lo01, lo23);
    const __m256i out_words =
        _mm256_inserti128_si256(_mm256_castsi128_si256(out_lo), out_hi, 1);

    _mm256_storeu_si256((__m256i *)(void *)out, out_words);
}

static void avx2_process_full_blocks_i2(HashCtx *ctx, const uint8_t *data, size_t full_blocks)
{
    __m256i c0, c1, c2, c3;
    const __m256i capacity_mask = avx2_load_u32x8(&CAPACITY_MASK_AVX512_W32);

    avx2_load_state_columns(ctx->state.row, &c0, &c1, &c2, &c3);

    for (; full_blocks != 0; full_blocks--, data += RATE_BYTES) {
        const __m256i block_words =
            _mm256_loadu_si256((const __m256i *)(const void *)data);
        avx2_process_prepared_block_w32(&c0, &c1, &c2, &c3, block_words, capacity_mask);
    }

    avx2_store_state_columns(ctx->state.row, c0, c1, c2, c3);
}

static void permutation_i2_avx2(State *s)
{
    __m256i c0, c1, c2, c3;

    avx2_load_state_columns(s->row, &c0, &c1, &c2, &c3);
    avx2_permute_columns_i2(&c0, &c1, &c2, &c3);
    avx2_store_state_columns(s->row, c0, c1, c2, c3);
}

static void avx2_hash_i2_fastpath(
    const uint8_t *msg,
    size_t msg_len,
    uint64_t msg_len_bits,
    uint8_t *out)
{
    size_t full_blocks = msg_len / RATE_BYTES;
    const size_t tail_len = msg_len - full_blocks * RATE_BYTES;
    __m256i c0, c1, c2, c3;
    const __m256i capacity_mask = avx2_load_u32x8(&CAPACITY_MASK_AVX512_W32);

    avx2_init_state_columns_i2(&c0, &c1, &c2, &c3, msg_len_bits);

    for (; full_blocks != 0; full_blocks--, msg += RATE_BYTES) {
        const __m256i block_words =
            _mm256_loadu_si256((const __m256i *)(const void *)msg);
        avx2_process_prepared_block_w32(&c0, &c1, &c2, &c3, block_words, capacity_mask);
    }

    if (tail_len != 0) {
        __m256i tail_words;

        avx2_load_padded_rate_block_w32(msg, tail_len, &tail_words);
        avx2_process_prepared_block_w32(&c0, &c1, &c2, &c3, tail_words, capacity_mask);
    }

    avx2_permute_columns_i2(&c0, &c1, &c2, &c3);
    avx2_squeeze_rate_columns(c0, c1, c2, c3, out);
}

LLH_TARGET_AVX512
#define avx512_load_u32x8(table) \
    _mm256_load_si256((const __m256i *)(const void *)((table)->lanes))

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE __m256i rotl32x8_avx512(
    __m256i v,
    __m256i counts)
{
    return _mm256_rolv_epi32(v, counts);
}

LLH_TARGET_AVX512
#define avx512_xor3_epi32(a, b, c) \
    _mm256_ternarylogic_epi32((a), (b), (c), LLH_AVX512_W32_TERNLOG_XOR3)

LLH_TARGET_AVX512
#define avx512_xorand_epi32(a, b, c) \
    _mm256_ternarylogic_epi32((a), (b), (c), LLH_AVX512_W32_TERNLOG_XOR_AND)

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE __m256i avx512_boolean_column(
    __m256i column,
    __m256i rot1_perm,
    __m256i rot2_perm,
    __m256i rot3_perm,
    __m256i complement_mask)
{
    const __m256i rot1 = _mm256_permutevar8x32_epi32(column, rot1_perm);
    const __m256i rot2 = _mm256_permutevar8x32_epi32(column, rot2_perm);
    const __m256i rot3 = _mm256_permutevar8x32_epi32(column, rot3_perm);
    const __m256i mixed = avx512_xorand_epi32(rot1, rot2, rot3);

    return avx512_xor3_epi32(column, mixed, complement_mask);
}

LLH_TARGET_AVX512
#define avx512_build_column(v, build_perm) \
    _mm256_permutevar8x32_epi32((v), (build_perm))

LLH_TARGET_AVX512
#define avx512_shiftrows_column(v, counts) rotl32x8_avx512((v), (counts))

LLH_TARGET_AVX512
#define avx512_load_rc(round_idx) avx512_load_u32x8(&RC_AVX512_W32[(round_idx)])

LLH_TARGET_AVX512
static inline void avx512_init_state_columns_i2(
    __m256i *c0,
    __m256i *c1,
    __m256i *c2,
    __m256i *c3,
    uint64_t msg_len_bits)
{
    const __m256i zero = _mm256_setzero_si256();
    const uint32_t msg_len_hi = (uint32_t)(msg_len_bits >> 32);
    const uint32_t msg_len_lo = (uint32_t)msg_len_bits;

    *c0 = zero;
    *c1 = zero;
    *c2 = _mm256_setr_epi32(0, 0, 0, 0, 0, (int)msg_len_lo, 0, 0);
    *c3 = _mm256_setr_epi32(0, 0, 0, 0, 0, (int)msg_len_hi, 0, 0);
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE __mmask32 avx512_prefix_mask32(size_t count)
{
    if (count == 0) {
        return (__mmask32)0;
    }
    if (count >= 32) {
        return ~(__mmask32)0;
    }
    return ((__mmask32)1u << count) - 1u;
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE __m256i avx512_load_padded_rate_block_w32(
    const uint8_t *src,
    size_t valid_bytes)
{
    const __m256i fill = _mm256_set1_epi8((char)0xFF);
    const __mmask32 valid_mask = avx512_prefix_mask32(valid_bytes);

    if (valid_bytes == 0) {
        return fill;
    }

    {
        const __m256i loaded = _mm256_maskz_loadu_epi8(valid_mask, src);
        return _mm256_mask_blend_epi8(valid_mask, fill, loaded);
    }
}

LLH_TARGET_AVX512
static inline void avx512_load_state_columns(
    const word_t *state,
    __m256i *c0,
    __m256i *c1,
    __m256i *c2,
    __m256i *c3)
{
    *c0 = _mm256_setr_epi32(
        (int)state[0],
        (int)state[4],
        (int)state[8],
        (int)state[12],
        (int)state[16],
        (int)state[20],
        0,
        0);
    *c1 = _mm256_setr_epi32(
        (int)state[1],
        (int)state[5],
        (int)state[9],
        (int)state[13],
        (int)state[17],
        (int)state[21],
        0,
        0);
    *c2 = _mm256_setr_epi32(
        (int)state[2],
        (int)state[6],
        (int)state[10],
        (int)state[14],
        (int)state[18],
        (int)state[22],
        0,
        0);
    *c3 = _mm256_setr_epi32(
        (int)state[3],
        (int)state[7],
        (int)state[11],
        (int)state[15],
        (int)state[19],
        (int)state[23],
        0,
        0);
}

LLH_TARGET_AVX512
static inline void avx512_store_state_columns(
    word_t *state,
    __m256i c0,
    __m256i c1,
    __m256i c2,
    __m256i c3)
{
    ALIGN32 uint32_t tmp0[8];
    ALIGN32 uint32_t tmp1[8];
    ALIGN32 uint32_t tmp2[8];
    ALIGN32 uint32_t tmp3[8];

    _mm256_store_si256((__m256i *)(void *)tmp0, c0);
    _mm256_store_si256((__m256i *)(void *)tmp1, c1);
    _mm256_store_si256((__m256i *)(void *)tmp2, c2);
    _mm256_store_si256((__m256i *)(void *)tmp3, c3);

    state[0] = tmp0[0];
    state[4] = tmp0[1];
    state[8] = tmp0[2];
    state[12] = tmp0[3];
    state[16] = tmp0[4];
    state[20] = tmp0[5];

    state[1] = tmp1[0];
    state[5] = tmp1[1];
    state[9] = tmp1[2];
    state[13] = tmp1[3];
    state[17] = tmp1[4];
    state[21] = tmp1[5];

    state[2] = tmp2[0];
    state[6] = tmp2[1];
    state[10] = tmp2[2];
    state[14] = tmp2[3];
    state[18] = tmp2[4];
    state[22] = tmp2[5];

    state[3] = tmp3[0];
    state[7] = tmp3[1];
    state[11] = tmp3[2];
    state[15] = tmp3[3];
    state[19] = tmp3[4];
    state[23] = tmp3[5];
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE void avx512_round_i2(
    __m256i *c0,
    __m256i *c1,
    __m256i *c2,
    __m256i *c3,
    __m256i rc_vec,
    __m256i build0,
    __m256i build1,
    __m256i build2,
    __m256i build3,
    __m256i rot1_perm,
    __m256i rot2_perm,
    __m256i rot3_perm,
    __m256i shift0,
    __m256i shift1,
    __m256i shift2,
    __m256i shift3,
    __m256i complement_mask)
{
    const __m256i b0 = avx512_boolean_column(*c0, rot1_perm, rot2_perm, rot3_perm, complement_mask);
    const __m256i b1 = avx512_boolean_column(*c1, rot1_perm, rot2_perm, rot3_perm, complement_mask);
    const __m256i b2 = avx512_boolean_column(*c2, rot1_perm, rot2_perm, rot3_perm, complement_mask);
    const __m256i b3 = avx512_boolean_column(*c3, rot1_perm, rot2_perm, rot3_perm, complement_mask);
    const __m256i f0 = avx512_build_column(b0, build0);
    const __m256i f1 = avx512_build_column(b1, build1);
    const __m256i f2 = avx512_build_column(b2, build2);
    const __m256i f3 = avx512_build_column(b3, build3);

    *c0 = avx512_shiftrows_column(avx512_xor3_epi32(f1, f2, f3), shift0);
    *c1 = avx512_shiftrows_column(avx512_xor3_epi32(f0, f2, f3), shift1);
    *c2 = avx512_shiftrows_column(avx512_xor3_epi32(f0, f1, f3), shift2);
    *c3 = _mm256_xor_si256(
        avx512_shiftrows_column(avx512_xor3_epi32(f0, f1, f2), shift3),
        rc_vec);
}

#define LLH_AVX512_W32_BOOLEAN_BUILD_ASM(src_op, dst_reg, build_reg)                  \
    "vpermd " src_op ", %%ymm20, %%ymm8\n\t"                                          \
    "vpermd " src_op ", %%ymm21, %%ymm9\n\t"                                          \
    "vpermd " src_op ", %%ymm22, %%ymm10\n\t"                                         \
    "vpternlogd $108, %%ymm10, %%ymm9, %%ymm8\n\t"                                    \
    "vmovdqa " src_op ", " dst_reg "\n\t"                                             \
    "vpternlogd $150, %%ymm31, %%ymm8, " dst_reg "\n\t"                               \
    "vpermd " dst_reg ", " build_reg ", " dst_reg "\n\t"

#define LLH_AVX512_W32_MIX_SHIFT_ASM(rc_mem)                                           \
    "vmovdqa %%ymm5, %[c0]\n\t"                                                        \
    "vpternlogd $150, %%ymm7, %%ymm6, %[c0]\n\t"                                       \
    "vprolvd %%ymm27, %[c0], %[c0]\n\t"                                                \
    "vmovdqa %%ymm4, %[c1]\n\t"                                                        \
    "vpternlogd $150, %%ymm7, %%ymm6, %[c1]\n\t"                                       \
    "vprolvd %%ymm28, %[c1], %[c1]\n\t"                                                \
    "vmovdqa %%ymm4, %[c2]\n\t"                                                        \
    "vpternlogd $150, %%ymm7, %%ymm5, %[c2]\n\t"                                       \
    "vprolvd %%ymm29, %[c2], %[c2]\n\t"                                                \
    "vmovdqa %%ymm4, %[c3]\n\t"                                                        \
    "vpternlogd $150, %%ymm6, %%ymm5, %[c3]\n\t"                                       \
    "vprolvd %%ymm30, %[c3], %[c3]\n\t"                                                \
    "vpxor " rc_mem ", %[c3], %[c3]\n\t"

#define LLH_AVX512_W32_ROUND_ASM(rc_mem)                                               \
    LLH_AVX512_W32_BOOLEAN_BUILD_ASM("%[c0]", "%%ymm4", "%%ymm23")                    \
    LLH_AVX512_W32_BOOLEAN_BUILD_ASM("%[c1]", "%%ymm5", "%%ymm24")                    \
    LLH_AVX512_W32_BOOLEAN_BUILD_ASM("%[c2]", "%%ymm6", "%%ymm25")                    \
    LLH_AVX512_W32_BOOLEAN_BUILD_ASM("%[c3]", "%%ymm7", "%%ymm26")                    \
    LLH_AVX512_W32_MIX_SHIFT_ASM(rc_mem)

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE void avx512_permute_columns_i2_asm(
    __m256i *c0,
    __m256i *c1,
    __m256i *c2,
    __m256i *c3)
{
    __m256i x0 = *c0;
    __m256i x1 = *c1;
    __m256i x2 = *c2;
    __m256i x3 = *c3;
    const u32x8_consts_t *rot_ptr = FAMILY_ROT_AVX512_W32;
    const u32x8_consts_t *build_ptr = BUILD_PERM_AVX512_W32;
    const u32x8_consts_t *shift_ptr = SHIFTROWS_AVX512_W32;
    const u32x8_consts_t *rc_ptr = RC_AVX512_W32;
    const u32x8_consts_t *complement_ptr = &COMPLEMENT_MASK_AVX512_W32;

    __asm__ volatile(
        "vmovdqa32 (%[rot_ptr]), %%ymm20\n\t"
        "vmovdqa32 32(%[rot_ptr]), %%ymm21\n\t"
        "vmovdqa32 64(%[rot_ptr]), %%ymm22\n\t"
        "vmovdqa32 (%[build_ptr]), %%ymm23\n\t"
        "vmovdqa32 32(%[build_ptr]), %%ymm24\n\t"
        "vmovdqa32 64(%[build_ptr]), %%ymm25\n\t"
        "vmovdqa32 96(%[build_ptr]), %%ymm26\n\t"
        "vmovdqa32 (%[shift_ptr]), %%ymm27\n\t"
        "vmovdqa32 32(%[shift_ptr]), %%ymm28\n\t"
        "vmovdqa32 64(%[shift_ptr]), %%ymm29\n\t"
        "vmovdqa32 96(%[shift_ptr]), %%ymm30\n\t"
        "vmovdqa32 (%[complement_ptr]), %%ymm31\n\t"
        LLH_AVX512_W32_ROUND_ASM("(%[rc_ptr])")
        LLH_AVX512_W32_ROUND_ASM("32(%[rc_ptr])")
        LLH_AVX512_W32_ROUND_ASM("64(%[rc_ptr])")
        LLH_AVX512_W32_ROUND_ASM("96(%[rc_ptr])")
        LLH_AVX512_W32_ROUND_ASM("128(%[rc_ptr])")
        LLH_AVX512_W32_ROUND_ASM("160(%[rc_ptr])")
        LLH_AVX512_W32_ROUND_ASM("192(%[rc_ptr])")
        LLH_AVX512_W32_ROUND_ASM("224(%[rc_ptr])")
        LLH_AVX512_W32_ROUND_ASM("256(%[rc_ptr])")
        LLH_AVX512_W32_ROUND_ASM("288(%[rc_ptr])")
        LLH_AVX512_W32_ROUND_ASM("320(%[rc_ptr])")
        LLH_AVX512_W32_ROUND_ASM("352(%[rc_ptr])")
        LLH_AVX512_W32_ROUND_ASM("384(%[rc_ptr])")
        LLH_AVX512_W32_ROUND_ASM("416(%[rc_ptr])")
        LLH_AVX512_W32_ROUND_ASM("448(%[rc_ptr])")
        LLH_AVX512_W32_ROUND_ASM("480(%[rc_ptr])")
        : [c0] "+x"(x0),
          [c1] "+x"(x1),
          [c2] "+x"(x2),
          [c3] "+x"(x3)
        : [rot_ptr] "r"(rot_ptr),
          [build_ptr] "r"(build_ptr),
          [shift_ptr] "r"(shift_ptr),
          [rc_ptr] "r"(rc_ptr),
          [complement_ptr] "r"(complement_ptr)
        : "ymm4", "ymm5", "ymm6", "ymm7", "ymm8", "ymm9", "ymm10",
          "ymm20", "ymm21", "ymm22", "ymm23", "ymm24", "ymm25", "ymm26",
          "ymm27", "ymm28", "ymm29", "ymm30", "ymm31");

    *c0 = x0;
    *c1 = x1;
    *c2 = x2;
    *c3 = x3;
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE void avx512_xor_rate_columns(
    __m256i *c0,
    __m256i *c1,
    __m256i *c2,
    __m256i *c3,
    __m256i block_words)
{
    const __m128i zero = _mm_setzero_si128();
    const __m128i lo = _mm256_castsi256_si128(block_words);
    const __m128i hi = _mm256_extracti128_si256(block_words, 1);
    const __m128i pair01 = _mm_unpacklo_epi32(lo, hi);
    const __m128i pair23 = _mm_unpackhi_epi32(lo, hi);
    const __m128i col0_lo = _mm_unpacklo_epi64(pair01, zero);
    const __m128i col1_lo = _mm_unpackhi_epi64(pair01, zero);
    const __m128i col2_lo = _mm_unpacklo_epi64(pair23, zero);
    const __m128i col3_lo = _mm_unpackhi_epi64(pair23, zero);

    *c0 = _mm256_xor_si256(*c0, _mm256_zextsi128_si256(col0_lo));
    *c1 = _mm256_xor_si256(*c1, _mm256_zextsi128_si256(col1_lo));
    *c2 = _mm256_xor_si256(*c2, _mm256_zextsi128_si256(col2_lo));
    *c3 = _mm256_xor_si256(*c3, _mm256_zextsi128_si256(col3_lo));
}

#define LLH_AVX512_W32_DO_ROUND_LOCAL(round_idx) \
    avx512_round_i2(                                             \
        &c0,                                                     \
        &c1,                                                     \
        &c2,                                                     \
        &c3,                                                     \
        avx512_load_rc(round_idx),                               \
        build0,                                                  \
        build1,                                                  \
        build2,                                                  \
        build3,                                                  \
        rot1_perm,                                               \
        rot2_perm,                                               \
        rot3_perm,                                               \
        shift0,                                                  \
        shift1,                                                  \
        shift2,                                                  \
        shift3,                                                  \
        complement_mask);

#define LLH_AVX512_W32_DO_2_ROUNDS_LOCAL(round_idx) \
    do {                                            \
        LLH_AVX512_W32_DO_ROUND_LOCAL(round_idx);   \
        LLH_AVX512_W32_DO_ROUND_LOCAL((round_idx) + 1); \
    } while (0)

#define LLH_AVX512_W32_DO_4_ROUNDS_LOCAL(round_idx) \
    do {                                            \
        LLH_AVX512_W32_DO_2_ROUNDS_LOCAL(round_idx); \
        LLH_AVX512_W32_DO_2_ROUNDS_LOCAL((round_idx) + 2); \
    } while (0)

#define LLH_AVX512_W32_PERMUTE_LOCAL()                \
    do {                                              \
        avx512_permute_columns_i2_asm(&c0, &c1, &c2, &c3); \
    } while (0)

#define LLH_AVX512_W32_SETUP_LOCALS()                                           \
    const __m256i capacity_mask = avx512_load_u32x8(&CAPACITY_MASK_AVX512_W32)

#define LLH_AVX512_W32_PROCESS_PREPARED_BLOCK_LOCAL(block_words_vec)                 \
    do {                                                                             \
        const __m256i llh_prev0 = c0;                                                \
        const __m256i llh_prev1 = c1;                                                \
        const __m256i llh_prev2 = c2;                                                \
        const __m256i llh_prev3 = c3;                                                \
                                                                                     \
        LLH_AVX512_W32_PERMUTE_LOCAL();                                              \
                                                                                     \
        c0 = _mm256_xor_si256(c0, _mm256_and_si256(llh_prev0, capacity_mask));       \
        c1 = _mm256_xor_si256(c1, _mm256_and_si256(llh_prev1, capacity_mask));       \
        c2 = _mm256_xor_si256(c2, _mm256_and_si256(llh_prev2, capacity_mask));       \
        c3 = _mm256_xor_si256(c3, _mm256_and_si256(llh_prev3, capacity_mask));       \
        avx512_xor_rate_columns(&c0, &c1, &c2, &c3, (block_words_vec));              \
    } while (0)

#define LLH_AVX512_W32_PROCESS_BLOCK_LOCAL(block_ptr)                                 \
    do {                                                                              \
        const __m256i llh_block_words =                                               \
            _mm256_loadu_si256((const __m256i *)(const void *)(block_ptr));           \
                                                                                      \
        LLH_AVX512_W32_PROCESS_PREPARED_BLOCK_LOCAL(llh_block_words);                 \
    } while (0)

LLH_TARGET_AVX512
static void avx512_process_full_blocks_i2(HashCtx *ctx, const uint8_t *data, size_t full_blocks)
{
    __m256i c0, c1, c2, c3;

    LLH_AVX512_W32_SETUP_LOCALS();
    avx512_load_state_columns(ctx->state.row, &c0, &c1, &c2, &c3);

#define LLH_AVX512_W32_PROCESS_BLOCK_AT(byte_off) \
    LLH_AVX512_W32_PROCESS_BLOCK_LOCAL((data + (byte_off)))

    for (; full_blocks >= 2; full_blocks -= 2, data += (RATE_BYTES * 2)) {
        LLH_AVX512_W32_PROCESS_BLOCK_AT(0);
        LLH_AVX512_W32_PROCESS_BLOCK_AT(RATE_BYTES);
    }

    if (full_blocks != 0) {
        LLH_AVX512_W32_PROCESS_BLOCK_AT(0);
    }

#undef LLH_AVX512_W32_PROCESS_BLOCK_AT

    avx512_store_state_columns(ctx->state.row, c0, c1, c2, c3);
}

LLH_TARGET_AVX512
static inline void avx512_squeeze_rate_columns(
    __m256i c0,
    __m256i c1,
    __m256i c2,
    __m256i c3,
    uint8_t *out)
{
    const __m128i c0_lo = _mm256_castsi256_si128(c0);
    const __m128i c1_lo = _mm256_castsi256_si128(c1);
    const __m128i c2_lo = _mm256_castsi256_si128(c2);
    const __m128i c3_lo = _mm256_castsi256_si128(c3);
    const __m128i lo01 = _mm_unpacklo_epi32(c0_lo, c1_lo);
    const __m128i lo23 = _mm_unpacklo_epi32(c2_lo, c3_lo);
    const __m128i out_lo = _mm_unpacklo_epi64(lo01, lo23);
    const __m128i out_hi = _mm_unpackhi_epi64(lo01, lo23);
    const __m256i out_words =
        _mm256_inserti128_si256(_mm256_castsi128_si256(out_lo), out_hi, 1);

    _mm256_storeu_si256((__m256i *)(void *)out, out_words);
}

LLH_TARGET_AVX512
static void avx512_hash_i2_fastpath(
    const uint8_t *msg,
    size_t msg_len,
    uint64_t msg_len_bits,
    uint8_t *out)
{
    size_t full_blocks = msg_len / RATE_BYTES;
    const size_t tail_len = msg_len - full_blocks * RATE_BYTES;
    __m256i c0, c1, c2, c3;

    LLH_AVX512_W32_SETUP_LOCALS();
    avx512_init_state_columns_i2(&c0, &c1, &c2, &c3, msg_len_bits);

    for (; full_blocks >= 2; full_blocks -= 2, msg += (RATE_BYTES * 2)) {
        LLH_AVX512_W32_PROCESS_BLOCK_LOCAL(msg);
        LLH_AVX512_W32_PROCESS_BLOCK_LOCAL(msg + RATE_BYTES);
    }

    if (full_blocks != 0) {
        LLH_AVX512_W32_PROCESS_BLOCK_LOCAL(msg);
        msg += RATE_BYTES;
    }

    if (tail_len != 0) {
        const __m256i tail_words =
            avx512_load_padded_rate_block_w32(msg, tail_len);
        LLH_AVX512_W32_PROCESS_PREPARED_BLOCK_LOCAL(tail_words);
    }

    LLH_AVX512_W32_PERMUTE_LOCAL();
    avx512_squeeze_rate_columns(c0, c1, c2, c3, out);
}

LLH_TARGET_AVX512
static void permutation_i2_avx512(State *s)
{
    __m256i c0, c1, c2, c3;

    LLH_AVX512_W32_SETUP_LOCALS();
    (void)capacity_mask;
    avx512_load_state_columns(s->row, &c0, &c1, &c2, &c3);
    LLH_AVX512_W32_PERMUTE_LOCAL();
    avx512_store_state_columns(s->row, c0, c1, c2, c3);
}

#undef LLH_AVX512_W32_SETUP_LOCALS
#undef LLH_AVX512_W32_PROCESS_PREPARED_BLOCK_LOCAL
#undef LLH_AVX512_W32_PROCESS_BLOCK_LOCAL
#undef LLH_AVX512_W32_PERMUTE_LOCAL
#undef LLH_AVX512_W32_DO_ROUND_LOCAL
#undef LLH_AVX512_W32_DO_2_ROUNDS_LOCAL
#undef LLH_AVX512_W32_DO_4_ROUNDS_LOCAL
#endif

static void permutation_i2(State *s)
{
#if LLH_ENABLE_AVX512_I2
    if (llh_runtime_has_avx512_i2()) {
        permutation_i2_avx512(s);
        return;
    }
#endif
    permutation_i2_avx2(s);
}
#else
static void permutation_i2(State *s)
{
    permutation(s);
}
#endif

static void xor_rate(State *s, const uint8_t *block)
{
    const int row_bytes = W / 8;

    for (int i = 0; i < RATE_ROWS; i++) {
        s->row[i] ^= load_word(block + i * row_bytes);
    }
}

static void squeeze_rate(const State *s, uint8_t *out)
{
    const int row_bytes = W / 8;

    for (int i = 0; i < RATE_ROWS; i++) {
        store_word(out + i * row_bytes, s->row[i]);
    }
}

static void hash_process_block(HashCtx *ctx, const uint8_t *block)
{
    State prev = ctx->state;

    permutation(&ctx->state);
    for (int i = RATE_ROWS; i < ROWS; i++) {
        ctx->state.row[i] ^= prev.row[i];
    }
    xor_rate(&ctx->state, block);
}

static void hash_process_block_i2(HashCtx *ctx, const uint8_t *block)
{
    word_t prev_capacity[CAP_ROWS];

    memcpy(prev_capacity, &ctx->state.row[RATE_ROWS], sizeof(prev_capacity));

    permutation_i2(&ctx->state);
    for (int i = 0; i < CAP_ROWS; i++) {
        ctx->state.row[RATE_ROWS + i] =
            WORD_XOR(ctx->state.row[RATE_ROWS + i], prev_capacity[i]);
    }
    xor_rate(&ctx->state, block);
}

void hash_init(HashCtx *ctx, uint64_t msg_len_bits)
{
    memset(ctx, 0, sizeof(*ctx));

    ctx->state.row[22] = (uint32_t)(msg_len_bits & 0xFFFFFFFFu);
    ctx->state.row[23] = (uint32_t)(msg_len_bits >> 32);
}

void hash_absorb(HashCtx *ctx, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        ctx->buf[ctx->buf_len++] = data[i];
        if (ctx->buf_len == RATE_BYTES) {
            hash_process_block(ctx, ctx->buf);
            ctx->buf_len = 0;
        }
    }
}

void hash_final(HashCtx *ctx, uint8_t *out)
{
    uint8_t final_block[RATE_BYTES];

    if (ctx->buf_len != 0) {
        memcpy(final_block, ctx->buf, ctx->buf_len);
        memset(final_block + ctx->buf_len, 0xFF, RATE_BYTES - ctx->buf_len);
        hash_process_block(ctx, final_block);
        ctx->buf_len = 0;
    }

    permutation(&ctx->state);
    squeeze_rate(&ctx->state, out);
}

void hash(const uint8_t *msg, size_t msg_len, uint8_t *out)
{
    HashCtx ctx;

    hash_init(&ctx, (uint64_t)msg_len * 8ULL);
    hash_absorb(&ctx, msg, msg_len);
    hash_final(&ctx, out);
}

static void hash_absorb_i2(HashCtx *ctx, const uint8_t *data, size_t len)
{
    size_t take;

    if (len == 0) {
        return;
    }

    if (ctx->buf_len != 0) {
        take = RATE_BYTES - ctx->buf_len;
        if (take > len) {
            take = len;
        }

        memcpy(ctx->buf + ctx->buf_len, data, take);
        ctx->buf_len += take;
        data += take;
        len -= take;

        if (ctx->buf_len == RATE_BYTES) {
            hash_process_block_i2(ctx, ctx->buf);
            ctx->buf_len = 0;
        }
    }

#if LLH_X86_INTRINSICS
    if (ctx->buf_len == 0 && len >= RATE_BYTES) {
        size_t full_blocks = len / RATE_BYTES;

        if (llh_runtime_has_avx512_i2()) {
            avx512_process_full_blocks_i2(ctx, data, full_blocks);
        } else {
            avx2_process_full_blocks_i2(ctx, data, full_blocks);
        }

        data += full_blocks * RATE_BYTES;
        len -= full_blocks * RATE_BYTES;
    }
#endif

    while (len >= RATE_BYTES) {
        hash_process_block_i2(ctx, data);
        data += RATE_BYTES;
        len -= RATE_BYTES;
    }

    if (len != 0) {
        memcpy(ctx->buf, data, len);
        ctx->buf_len = len;
    }
}

static void hash_final_i2(HashCtx *ctx, uint8_t *out)
{
    uint8_t final_block[RATE_BYTES];

    if (ctx->buf_len != 0) {
        memcpy(final_block, ctx->buf, ctx->buf_len);
        memset(final_block + ctx->buf_len, 0xFF, RATE_BYTES - ctx->buf_len);
        hash_process_block_i2(ctx, final_block);
        ctx->buf_len = 0;
    }

    permutation_i2(&ctx->state);
    squeeze_rate(&ctx->state, out);
}




static void hash_i2_fallback(
    const uint8_t *msg,
    size_t msg_len,
    uint64_t msg_len_bits,
    uint8_t *out)
{
    HashCtx ctx;

    hash_init(&ctx, msg_len_bits);

    if (msg_len <= RATE_BYTES) {
        uint8_t final_block[RATE_BYTES];

        if (msg_len == RATE_BYTES) {
            hash_process_block_i2(&ctx, msg);
        } else if (msg_len != 0) {
            memcpy(final_block, msg, msg_len);
            memset(final_block + msg_len, 0xFF, RATE_BYTES - msg_len);
            hash_process_block_i2(&ctx, final_block);
        }

        permutation_i2(&ctx.state);
        squeeze_rate(&ctx.state, out);
        return;
    }

    hash_absorb_i2(&ctx, msg, msg_len);
    hash_final_i2(&ctx, out);
}

void hash_i2(const uint8_t *msg, size_t msg_len, uint8_t *out)
{
    uint64_t msg_len_bits = (uint64_t)msg_len * 8ULL;

#if LLH_X86_INTRINSICS
    if (llh_runtime_has_avx512_i2()) {
        avx512_hash_i2_fastpath(msg, msg_len, msg_len_bits, out);
        return;
    }

    avx2_hash_i2_fastpath(msg, msg_len, msg_len_bits, out);
    return;
#endif

    hash_i2_fallback(msg, msg_len, msg_len_bits, out);
}
