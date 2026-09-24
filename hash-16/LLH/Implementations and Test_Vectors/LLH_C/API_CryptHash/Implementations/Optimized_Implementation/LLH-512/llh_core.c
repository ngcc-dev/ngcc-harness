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

typedef uint64_t word_t;

static inline word_t llh_rotl(word_t x, unsigned int n)
{
    return (word_t)((x << n) | (x >> (64u - n)));
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
    2,  25, 20, 51, 15, 40, 37, 6,
    36, 63, 62, 33, 1,  30, 31, 4,
    38, 5,  8,  47, 19, 52, 57, 34
};

#if LLH_X86_INTRINSICS
typedef struct {
    uint64_t lanes[4];
} rc256_const_t;
#endif

#define LLH_W64_RC_LIST(X)                 \
    X(B7E151628AED2A6A)                    \
    X(BF7158809CF4F3C7)                    \
    X(62E7160F38B4DA56)                    \
    X(A784D9045190CFEF)                    \
    X(324E7738926CFBE5)                    \
    X(F4BF8D8D8C31D763)                    \
    X(DA06C80ABB1185EB)                    \
    X(4F7C7B5757F59584)                    \
    X(90CFD47D7C19BB42)                    \
    X(158D9554F7B46BCE)                    \
    X(D55C4D79FD5F24D6)                    \
    X(613C31C3839A2DDF)                    \
    X(8A9A276BCFBFA1C8)                    \
    X(77C56284DAB79CD4)                    \
    X(C2B3293D20E9E5EA)                    \
    X(F02AC60ACC93ED87)                    \
    X(4422A52ECB238FEE)                    \
    X(E5AB6ADD835FD1A0)                    \
    X(753D0A8F78E537D2)                    \
    X(B95BB79D8DCAEC64)                    \
    X(2C1E9F23B829B5C2)                    \
    X(780BF38737DF8BB3)                    \
    X(00D01334A0D0BD86)                    \
    X(45CBFA73A6160FFE)

#define LLH_W64_RC_ENTRY_WORD(v) 0x##v##ull,

static const word_t RC[24] = {
    LLH_W64_RC_LIST(LLH_W64_RC_ENTRY_WORD)
};

#if LLH_X86_INTRINSICS
#define LLH_W64_RC_ENTRY_AVX512(v) { { 0ull, 0ull, 0ull, 0x##v##ull } },

static const ALIGN32 rc256_const_t RC_AVX512_W64[24] = {
    LLH_W64_RC_LIST(LLH_W64_RC_ENTRY_AVX512)
};

#undef LLH_W64_RC_ENTRY_AVX512
#endif

#undef LLH_W64_RC_ENTRY_WORD
#undef LLH_W64_RC_LIST


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

#define load_word(p) \
    ((word_t)LLH_LOAD_U32_LE(p) | ((word_t)LLH_LOAD_U32_LE((p) + 4) << 32))

#define store_word(p, w)                 \
    do {                                 \
        LLH_STORE_U32_LE((p), (uint32_t)(w)); \
        LLH_STORE_U32_LE((p) + 4, (uint32_t)((w) >> 32)); \
    } while (0)

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
typedef struct {
    uint64_t counts[4];
} rotl64x4_consts_t;

#define ROTL64X4_CONSTS(n0, n1, n2, n3) {                                \
    {                                                                    \
        (uint64_t)(n0), (uint64_t)(n1), (uint64_t)(n2), (uint64_t)(n3)   \
    }                                                                    \
}


static const ALIGN32 rotl64x4_consts_t SHIFTROWS_AVX2_W64[6] = {
    ROTL64X4_CONSTS(2,  25, 20, 51),
    ROTL64X4_CONSTS(15, 40, 37, 6),
    ROTL64X4_CONSTS(36, 63, 62, 33),
    ROTL64X4_CONSTS(1,  30, 31, 4),
    ROTL64X4_CONSTS(38, 5,  8,  47),
    ROTL64X4_CONSTS(19, 52, 57, 34)
};

#define LLH_AVX2_W64_BLEND13_EPI32 0xCC
#define LLH_AVX2_W64_BLEND23_EPI32 0xF0

static LLH_ALWAYS_INLINE __m256i rotl64x4_avx2(
    __m256i v,
    __m256i counts,
    __m256i shift_64)
{
    return _mm256_or_si256(
        _mm256_sllv_epi64(v, counts),
        _mm256_srlv_epi64(v, _mm256_sub_epi64(shift_64, counts)));
}

static LLH_ALWAYS_INLINE __m256i avx2_shiftrows_family(
    __m256i v,
    size_t table_idx,
    __m256i shift_64)
{
    const __m256i counts =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX2_W64[table_idx].counts);
    return rotl64x4_avx2(v, counts, shift_64);
}

#define avx2_load_rc(round_idx) \
    _mm256_load_si256((const __m256i *)(const void *)&RC_AVX512_W64[(round_idx)])

static LLH_ALWAYS_INLINE __m256i avx2_total_xor_broadcast(__m256i v)
{
    const __m256i pair_xor =
        _mm256_xor_si256(v, _mm256_permute4x64_epi64(v, _MM_SHUFFLE(1, 0, 3, 2)));

    return _mm256_xor_si256(pair_xor, _mm256_permute4x64_epi64(pair_xor, _MM_SHUFFLE(2, 3, 0, 1)));
}

static LLH_ALWAYS_INLINE __m256i avx2_linear_family(__m256i v)
{
    return _mm256_xor_si256(v, avx2_total_xor_broadcast(v));
}

#define avx2_xor3_epi64(a, b, c) \
    _mm256_xor_si256(_mm256_xor_si256((a), (b)), (c))

#define avx2_xnor3_epi64(a, b, c, all_ones) \
    _mm256_xor_si256(avx2_xor3_epi64((a), (b), (c)), (all_ones))

#define avx2_build_family(lane0_src, lane1_src, lane2_src, lane3_src)           \
    _mm256_blend_epi32(                                                          \
        _mm256_blend_epi32((lane0_src), (lane1_src), LLH_AVX2_W64_BLEND13_EPI32), \
        _mm256_blend_epi32((lane2_src), (lane3_src), LLH_AVX2_W64_BLEND13_EPI32), \
        LLH_AVX2_W64_BLEND23_EPI32)

static LLH_ALWAYS_INLINE void avx2_init_state_families_i2(
    __m256i *g0,
    __m256i *g1,
    __m256i *g2,
    __m256i *g3,
    __m256i *g4,
    __m256i *g5,
    uint64_t msg_len_bits)
{
    const __m256i zero = _mm256_setzero_si256();

    *g0 = zero;
    *g1 = zero;
    *g2 = zero;
    *g3 = zero;
    *g4 = zero;
    *g5 = _mm256_set_epi64x((long long)msg_len_bits, 0LL, 0LL, 0LL);
}

static LLH_ALWAYS_INLINE void avx2_load_padded_rate_block_w64(
    const uint8_t *src,
    size_t valid_bytes,
    __m256i *block0,
    __m256i *block1)
{
    ALIGN32 uint8_t padded[RATE_BYTES];

    memset(padded, 0xFF, sizeof(padded));
    if (valid_bytes != 0) {
        memcpy(padded, src, valid_bytes);
    }

    *block0 = _mm256_load_si256((const __m256i *)(const void *)padded);
    *block1 = _mm256_load_si256((const __m256i *)(const void *)(padded + 32));
}

#define avx2_xor_rate_vectors(g0, g1, block0, block1) \
    do {                                               \
        *(g0) = _mm256_xor_si256(*(g0), (block0));     \
        *(g1) = _mm256_xor_si256(*(g1), (block1));     \
    } while (0)

static LLH_ALWAYS_INLINE void avx2_xor_rate_families(
    __m256i *g0,
    __m256i *g1,
    const uint8_t *block)
{
    const __m256i block0 =
        _mm256_loadu_si256((const __m256i *)(const void *)(block + 0));
    const __m256i block1 =
        _mm256_loadu_si256((const __m256i *)(const void *)(block + 32));

    avx2_xor_rate_vectors(g0, g1, block0, block1);
}

static LLH_ALWAYS_INLINE void avx2_load_state_families(
    const word_t *state,
    __m256i *g0,
    __m256i *g1,
    __m256i *g2,
    __m256i *g3,
    __m256i *g4,
    __m256i *g5)
{
    *g0 = _mm256_loadu_si256((const __m256i *)(const void *)&state[0]);
    *g1 = _mm256_loadu_si256((const __m256i *)(const void *)&state[4]);
    *g2 = _mm256_loadu_si256((const __m256i *)(const void *)&state[8]);
    *g3 = _mm256_loadu_si256((const __m256i *)(const void *)&state[12]);
    *g4 = _mm256_loadu_si256((const __m256i *)(const void *)&state[16]);
    *g5 = _mm256_loadu_si256((const __m256i *)(const void *)&state[20]);
}

static LLH_ALWAYS_INLINE void avx2_store_state_families(
    word_t *state,
    __m256i g0,
    __m256i g1,
    __m256i g2,
    __m256i g3,
    __m256i g4,
    __m256i g5)
{
    _mm256_storeu_si256((__m256i *)(void *)&state[0], g0);
    _mm256_storeu_si256((__m256i *)(void *)&state[4], g1);
    _mm256_storeu_si256((__m256i *)(void *)&state[8], g2);
    _mm256_storeu_si256((__m256i *)(void *)&state[12], g3);
    _mm256_storeu_si256((__m256i *)(void *)&state[16], g4);
    _mm256_storeu_si256((__m256i *)(void *)&state[20], g5);
}

static LLH_ALWAYS_INLINE void avx2_round_i2(
    __m256i *g0,
    __m256i *g1,
    __m256i *g2,
    __m256i *g3,
    __m256i *g4,
    __m256i *g5,
    __m256i rc_vec,
    __m256i all_ones,
    __m256i shift_64)
{
    const __m256i g1g3 = _mm256_and_si256(*g1, *g3);
    const __m256i g2g4 = _mm256_and_si256(*g2, *g4);
    const __m256i g3g5 = _mm256_and_si256(*g3, *g5);
    const __m256i g4g0 = _mm256_and_si256(*g4, *g0);
    const __m256i g5g1 = _mm256_and_si256(*g5, *g1);
    const __m256i g0g2 = _mm256_and_si256(*g0, *g2);
    const __m256i n0 = avx2_xnor3_epi64(*g0, *g2, g1g3, all_ones);
    const __m256i n1 = avx2_xnor3_epi64(*g1, *g3, g2g4, all_ones);
    const __m256i u2 = avx2_xor3_epi64(*g2, *g4, g3g5);
    const __m256i n3 = avx2_xnor3_epi64(*g3, *g5, g4g0, all_ones);
    const __m256i n4 = avx2_xnor3_epi64(*g4, *g0, g5g1, all_ones);
    const __m256i u5 = avx2_xor3_epi64(*g5, *g1, g0g2);

    *g0 = avx2_shiftrows_family(avx2_linear_family(avx2_build_family(n1, u2, n4, u5)), 0, shift_64);
    *g1 = avx2_shiftrows_family(avx2_linear_family(avx2_build_family(u2, n0, u5, n3)), 1, shift_64);
    *g2 = avx2_shiftrows_family(avx2_linear_family(avx2_build_family(n3, n4, n1, n0)), 2, shift_64);
    *g3 = avx2_shiftrows_family(avx2_linear_family(avx2_build_family(u5, n1, u2, n4)), 3, shift_64);
    *g4 = avx2_shiftrows_family(avx2_linear_family(avx2_build_family(n0, u5, n3, u2)), 4, shift_64);
    *g5 = avx2_shiftrows_family(avx2_linear_family(avx2_build_family(n4, n3, n0, n1)), 5, shift_64);

    *g5 = _mm256_xor_si256(*g5, rc_vec);
}

static LLH_ALWAYS_INLINE void avx2_permute_state_families(
    __m256i *g0,
    __m256i *g1,
    __m256i *g2,
    __m256i *g3,
    __m256i *g4,
    __m256i *g5,
    __m256i all_ones,
    __m256i shift_64)
{
    for (int round = 0; round < NR_DEFAULT; round++) {
        avx2_round_i2(g0, g1, g2, g3, g4, g5, avx2_load_rc((size_t)round), all_ones, shift_64);
    }
}

static LLH_ALWAYS_INLINE void avx2_process_prepared_block_w64(
    __m256i *g0,
    __m256i *g1,
    __m256i *g2,
    __m256i *g3,
    __m256i *g4,
    __m256i *g5,
    __m256i block0,
    __m256i block1,
    __m256i all_ones,
    __m256i shift_64)
{
    const __m256i prev2 = *g2;
    const __m256i prev3 = *g3;
    const __m256i prev4 = *g4;
    const __m256i prev5 = *g5;

    avx2_permute_state_families(g0, g1, g2, g3, g4, g5, all_ones, shift_64);

    *g2 = _mm256_xor_si256(*g2, prev2);
    *g3 = _mm256_xor_si256(*g3, prev3);
    *g4 = _mm256_xor_si256(*g4, prev4);
    *g5 = _mm256_xor_si256(*g5, prev5);

    avx2_xor_rate_vectors(g0, g1, block0, block1);
}

static LLH_ALWAYS_INLINE void avx2_process_block_w64(
    __m256i *g0,
    __m256i *g1,
    __m256i *g2,
    __m256i *g3,
    __m256i *g4,
    __m256i *g5,
    const uint8_t *block,
    __m256i all_ones,
    __m256i shift_64)
{
    const __m256i block0 =
        _mm256_loadu_si256((const __m256i *)(const void *)(block + 0));
    const __m256i block1 =
        _mm256_loadu_si256((const __m256i *)(const void *)(block + 32));

    avx2_process_prepared_block_w64(g0, g1, g2, g3, g4, g5, block0, block1, all_ones, shift_64);
}

static LLH_ALWAYS_INLINE void avx2_squeeze_rate_families(
    __m256i g0,
    __m256i g1,
    uint8_t *out)
{
    _mm256_storeu_si256((__m256i *)(void *)out, g0);
    _mm256_storeu_si256((__m256i *)(void *)(out + 32), g1);
}

static void avx2_process_full_blocks_i2(HashCtx *ctx, const uint8_t *data, size_t full_blocks)
{
    __m256i g0, g1, g2, g3, g4, g5;
    const __m256i all_ones = _mm256_set1_epi64x(-1);
    const __m256i shift_64 = _mm256_set1_epi64x(64);

    avx2_load_state_families(ctx->state.row, &g0, &g1, &g2, &g3, &g4, &g5);

    for (; full_blocks != 0; full_blocks--, data += RATE_BYTES) {
        avx2_process_block_w64(&g0, &g1, &g2, &g3, &g4, &g5, data, all_ones, shift_64);
    }

    avx2_store_state_families(ctx->state.row, g0, g1, g2, g3, g4, g5);
}

static void permutation_i2_avx2(State *s)
{
    __m256i g0, g1, g2, g3, g4, g5;
    const __m256i all_ones = _mm256_set1_epi64x(-1);
    const __m256i shift_64 = _mm256_set1_epi64x(64);

    avx2_load_state_families(s->row, &g0, &g1, &g2, &g3, &g4, &g5);
    avx2_permute_state_families(&g0, &g1, &g2, &g3, &g4, &g5, all_ones, shift_64);
    avx2_store_state_families(s->row, g0, g1, g2, g3, g4, g5);
}

static void avx2_hash_i2_fastpath(
    const uint8_t *msg,
    size_t msg_len,
    uint64_t msg_len_bits,
    uint8_t *out)
{
    size_t full_blocks = msg_len / RATE_BYTES;
    const size_t tail_len = msg_len - full_blocks * RATE_BYTES;
    __m256i g0, g1, g2, g3, g4, g5;
    const __m256i all_ones = _mm256_set1_epi64x(-1);
    const __m256i shift_64 = _mm256_set1_epi64x(64);

    avx2_init_state_families_i2(&g0, &g1, &g2, &g3, &g4, &g5, msg_len_bits);

    if (msg_len < (RATE_BYTES * 2)) {
        if (msg_len == 0) {
            avx2_permute_state_families(&g0, &g1, &g2, &g3, &g4, &g5, all_ones, shift_64);
            avx2_squeeze_rate_families(g0, g1, out);
            return;
        }

        if (msg_len < RATE_BYTES) {
            __m256i tail0, tail1;

            avx2_load_padded_rate_block_w64(msg, msg_len, &tail0, &tail1);
            avx2_process_prepared_block_w64(&g0, &g1, &g2, &g3, &g4, &g5, tail0, tail1, all_ones, shift_64);
            avx2_permute_state_families(&g0, &g1, &g2, &g3, &g4, &g5, all_ones, shift_64);
            avx2_squeeze_rate_families(g0, g1, out);
            return;
        }

        avx2_process_block_w64(&g0, &g1, &g2, &g3, &g4, &g5, msg, all_ones, shift_64);

        if (msg_len > RATE_BYTES) {
            if (msg_len == (RATE_BYTES * 2)) {
                avx2_process_block_w64(&g0, &g1, &g2, &g3, &g4, &g5, msg + RATE_BYTES, all_ones, shift_64);
            } else {
                __m256i tail0, tail1;

                avx2_load_padded_rate_block_w64(msg + RATE_BYTES, msg_len - RATE_BYTES, &tail0, &tail1);
                avx2_process_prepared_block_w64(&g0, &g1, &g2, &g3, &g4, &g5, tail0, tail1, all_ones, shift_64);
            }
        }

        avx2_permute_state_families(&g0, &g1, &g2, &g3, &g4, &g5, all_ones, shift_64);
        avx2_squeeze_rate_families(g0, g1, out);
        return;
    }

    for (; full_blocks != 0; full_blocks--, msg += RATE_BYTES) {
        avx2_process_block_w64(&g0, &g1, &g2, &g3, &g4, &g5, msg, all_ones, shift_64);
    }

    if (tail_len != 0) {
        __m256i tail0, tail1;

        avx2_load_padded_rate_block_w64(msg, tail_len, &tail0, &tail1);
        avx2_process_prepared_block_w64(&g0, &g1, &g2, &g3, &g4, &g5, tail0, tail1, all_ones, shift_64);
    }

    avx2_permute_state_families(&g0, &g1, &g2, &g3, &g4, &g5, all_ones, shift_64);
    avx2_squeeze_rate_families(g0, g1, out);
}

#if LLH_ENABLE_AVX512_I2
#define LLH_AVX512_W64_TERNLOG_XOR3 0x96
#define LLH_AVX512_W64_TERNLOG_XNOR3 0x69
#define LLH_AVX512_W64_BLEND13_EPI32 0xCC
#define LLH_AVX512_W64_BLEND23_EPI32 0xF0

#define LLH_AVX512_W64_ROUND_LIST(X) \
    X(0)                             \
    X(1)                             \
    X(2)                             \
    X(3)                             \
    X(4)                             \
    X(5)                             \
    X(6)                             \
    X(7)                             \
    X(8)                             \
    X(9)                             \
    X(10)                            \
    X(11)                            \
    X(12)                            \
    X(13)                            \
    X(14)                            \
    X(15)                            \
    X(16)                            \
    X(17)                            \
    X(18)                            \
    X(19)                            \
    X(20)                            \
    X(21)                            \
    X(22)                            \
    X(23)


static const ALIGN32 rotl64x4_consts_t SHIFTROWS_AVX512_W64[6] = {
    ROTL64X4_CONSTS(2,  25, 20, 51),
    ROTL64X4_CONSTS(15, 40, 37, 6),
    ROTL64X4_CONSTS(36, 63, 62, 33),
    ROTL64X4_CONSTS(1,  30, 31, 4),
    ROTL64X4_CONSTS(38, 5,  8,  47),
    ROTL64X4_CONSTS(19, 52, 57, 34)
};

LLH_TARGET_AVX512
static inline __m256i rotl64x4_avx512(
    __m256i v,
    __m256i counts)
{
    return _mm256_rolv_epi64(v, counts);
}

LLH_TARGET_AVX512
static inline __m256i avx512_shiftrows_family(
    __m256i v,
    size_t table_idx)
{
    const __m256i counts =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W64[table_idx].counts);
    return rotl64x4_avx512(v, counts);
}

LLH_TARGET_AVX512
#define avx512_load_rc(round_idx) \
    _mm256_load_si256((const __m256i *)(const void *)&RC_AVX512_W64[(round_idx)])

LLH_TARGET_AVX512
static inline __m256i avx512_total_xor_broadcast(__m256i v)
{
    const __m256i pair_xor =
        _mm256_xor_si256(v, _mm256_permute4x64_epi64(v, _MM_SHUFFLE(1, 0, 3, 2)));

    return _mm256_xor_si256(pair_xor, _mm256_permute4x64_epi64(pair_xor, _MM_SHUFFLE(2, 3, 0, 1)));
}

LLH_TARGET_AVX512
static inline __m256i avx512_linear_family(__m256i v)
{
    return _mm256_xor_si256(v, avx512_total_xor_broadcast(v));
}

LLH_TARGET_AVX512
#define avx512_xor3_epi64(a, b, c) \
    _mm256_ternarylogic_epi64((a), (b), (c), LLH_AVX512_W64_TERNLOG_XOR3)

LLH_TARGET_AVX512
#define avx512_xnor3_epi64(a, b, c) \
    _mm256_ternarylogic_epi64((a), (b), (c), LLH_AVX512_W64_TERNLOG_XNOR3)

LLH_TARGET_AVX512
#define avx512_build_family(lane0_src, lane1_src, lane2_src, lane3_src)         \
    _mm256_blend_epi32(                                                          \
        _mm256_blend_epi32((lane0_src), (lane1_src), LLH_AVX512_W64_BLEND13_EPI32), \
        _mm256_blend_epi32((lane2_src), (lane3_src), LLH_AVX512_W64_BLEND13_EPI32), \
        LLH_AVX512_W64_BLEND23_EPI32)

LLH_TARGET_AVX512
static inline void avx512_init_state_families_i2(
    __m256i *g0,
    __m256i *g1,
    __m256i *g2,
    __m256i *g3,
    __m256i *g4,
    __m256i *g5,
    uint64_t msg_len_bits)
{
    const __m256i zero = _mm256_setzero_si256();

    *g0 = zero;
    *g1 = zero;
    *g2 = zero;
    *g3 = zero;
    *g4 = zero;
    *g5 = _mm256_set_epi64x((long long)msg_len_bits, 0LL, 0LL, 0LL);
}

LLH_TARGET_AVX512
static inline __mmask32 avx512_prefix_mask32(size_t count)
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
static inline __m256i avx512_load_padded_chunk32(const uint8_t *src, size_t valid_bytes)
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
static inline void avx512_load_padded_rate_block_w64(
    const uint8_t *src,
    size_t valid_bytes,
    __m256i *block0,
    __m256i *block1)
{
    const size_t low_bytes = (valid_bytes < 32) ? valid_bytes : 32;
    const size_t high_bytes = (valid_bytes > 32) ? (valid_bytes - 32) : 0;
    const uint8_t *src_hi = (high_bytes != 0) ? (src + 32) : src;

    *block0 = avx512_load_padded_chunk32(src, low_bytes);
    *block1 = avx512_load_padded_chunk32(src_hi, high_bytes);
}

LLH_TARGET_AVX512
#define avx512_xor_rate_vectors(g0, g1, block0, block1) \
    do {                                                 \
        *(g0) = _mm256_xor_si256(*(g0), (block0));       \
        *(g1) = _mm256_xor_si256(*(g1), (block1));       \
    } while (0)

LLH_TARGET_AVX512
static inline void avx512_load_state_families(
    const word_t *state,
    __m256i *g0,
    __m256i *g1,
    __m256i *g2,
    __m256i *g3,
    __m256i *g4,
    __m256i *g5)
{
    *g0 = _mm256_loadu_si256((const __m256i *)(const void *)&state[0]);
    *g1 = _mm256_loadu_si256((const __m256i *)(const void *)&state[4]);
    *g2 = _mm256_loadu_si256((const __m256i *)(const void *)&state[8]);
    *g3 = _mm256_loadu_si256((const __m256i *)(const void *)&state[12]);
    *g4 = _mm256_loadu_si256((const __m256i *)(const void *)&state[16]);
    *g5 = _mm256_loadu_si256((const __m256i *)(const void *)&state[20]);
}

LLH_TARGET_AVX512
static inline void avx512_store_state_families(
    word_t *state,
    __m256i g0,
    __m256i g1,
    __m256i g2,
    __m256i g3,
    __m256i g4,
    __m256i g5)
{
    _mm256_storeu_si256((__m256i *)(void *)&state[0], g0);
    _mm256_storeu_si256((__m256i *)(void *)&state[4], g1);
    _mm256_storeu_si256((__m256i *)(void *)&state[8], g2);
    _mm256_storeu_si256((__m256i *)(void *)&state[12], g3);
    _mm256_storeu_si256((__m256i *)(void *)&state[16], g4);
    _mm256_storeu_si256((__m256i *)(void *)&state[20], g5);
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE void avx512_round_i2(
    __m256i *g0,
    __m256i *g1,
    __m256i *g2,
    __m256i *g3,
    __m256i *g4,
    __m256i *g5,
    __m256i rc_vec)
{
    const __m256i g1g3 = _mm256_and_si256(*g1, *g3);
    const __m256i g2g4 = _mm256_and_si256(*g2, *g4);
    const __m256i g3g5 = _mm256_and_si256(*g3, *g5);
    const __m256i g4g0 = _mm256_and_si256(*g4, *g0);
    const __m256i g5g1 = _mm256_and_si256(*g5, *g1);
    const __m256i g0g2 = _mm256_and_si256(*g0, *g2);
    const __m256i n0 = avx512_xnor3_epi64(*g0, *g2, g1g3);
    const __m256i n1 = avx512_xnor3_epi64(*g1, *g3, g2g4);
    const __m256i u2 = avx512_xor3_epi64(*g2, *g4, g3g5);
    const __m256i n3 = avx512_xnor3_epi64(*g3, *g5, g4g0);
    const __m256i n4 = avx512_xnor3_epi64(*g4, *g0, g5g1);
    const __m256i u5 = avx512_xor3_epi64(*g5, *g1, g0g2);

    *g0 = avx512_shiftrows_family(avx512_linear_family(avx512_build_family(n1, u2, n4, u5)), 0);
    *g1 = avx512_shiftrows_family(avx512_linear_family(avx512_build_family(u2, n0, u5, n3)), 1);
    *g2 = avx512_shiftrows_family(avx512_linear_family(avx512_build_family(n3, n4, n1, n0)), 2);
    *g3 = avx512_shiftrows_family(avx512_linear_family(avx512_build_family(u5, n1, u2, n4)), 3);
    *g4 = avx512_shiftrows_family(avx512_linear_family(avx512_build_family(n0, u5, n3, u2)), 4);
    *g5 = avx512_shiftrows_family(avx512_linear_family(avx512_build_family(n4, n3, n0, n1)), 5);

    *g5 = _mm256_xor_si256(*g5, rc_vec);
}

#define LLH_AVX512_W64_DO_ROUND_LOCAL(round_idx) \
    avx512_round_i2(&g0, &g1, &g2, &g3, &g4, &g5, avx512_load_rc(round_idx));

#define LLH_AVX512_W64_PERMUTE_LOCAL() \
    do {                               \
        LLH_AVX512_W64_ROUND_LIST(LLH_AVX512_W64_DO_ROUND_LOCAL) \
    } while (0)

#define LLH_AVX512_W64_PROCESS_BLOCK_LOCAL(block_ptr)                         \
    do {                                                                     \
        const __m256i llh_prev2 = g2;                                        \
        const __m256i llh_prev3 = g3;                                        \
        const __m256i llh_prev4 = g4;                                        \
        const __m256i llh_prev5 = g5;                                        \
                                                                             \
        LLH_AVX512_W64_PERMUTE_LOCAL();                                      \
                                                                             \
        g2 = _mm256_xor_si256(g2, llh_prev2);                                \
        g3 = _mm256_xor_si256(g3, llh_prev3);                                \
        g4 = _mm256_xor_si256(g4, llh_prev4);                                \
        g5 = _mm256_xor_si256(g5, llh_prev5);                                \
                                                                             \
        avx512_xor_rate_families(&g0, &g1, (block_ptr));                     \
    } while (0)

#define LLH_AVX512_W64_PROCESS_PREPARED_BLOCK_LOCAL(block0_vec, block1_vec)    \
    do {                                                                                   \
        const __m256i llh_prev2 = g2;                                                      \
        const __m256i llh_prev3 = g3;                                                      \
        const __m256i llh_prev4 = g4;                                                      \
        const __m256i llh_prev5 = g5;                                                      \
                                                                                           \
        LLH_AVX512_W64_PERMUTE_LOCAL();                                                    \
                                                                                           \
        g2 = _mm256_xor_si256(g2, llh_prev2);                                              \
        g3 = _mm256_xor_si256(g3, llh_prev3);                                              \
        g4 = _mm256_xor_si256(g4, llh_prev4);                                              \
        g5 = _mm256_xor_si256(g5, llh_prev5);                                              \
                                                                                           \
        avx512_xor_rate_vectors(&g0, &g1, (block0_vec), (block1_vec));                     \
    } while (0)

LLH_TARGET_AVX512
static inline void avx512_xor_rate_families(
    __m256i *g0,
    __m256i *g1,
    const uint8_t *block)
{
    const __m256i block0 =
        _mm256_loadu_si256((const __m256i *)(const void *)(block + 0));
    const __m256i block1 =
        _mm256_loadu_si256((const __m256i *)(const void *)(block + 32));

    avx512_xor_rate_vectors(g0, g1, block0, block1);
}

LLH_TARGET_AVX512
static void avx512_process_full_blocks_i2(HashCtx *ctx, const uint8_t *data, size_t full_blocks)
{
    __m256i g0, g1, g2, g3, g4, g5;

    avx512_load_state_families(ctx->state.row, &g0, &g1, &g2, &g3, &g4, &g5);

#define LLH_AVX512_W64_PROCESS_BLOCK_AT(byte_off) \
    LLH_AVX512_W64_PROCESS_BLOCK_LOCAL((data + (byte_off)))

    for (; full_blocks >= 2; full_blocks -= 2, data += (RATE_BYTES * 2)) {
        LLH_AVX512_W64_PROCESS_BLOCK_AT(0);
        LLH_AVX512_W64_PROCESS_BLOCK_AT(RATE_BYTES);
    }

    if (full_blocks != 0) {
        LLH_AVX512_W64_PROCESS_BLOCK_AT(0);
    }

#undef LLH_AVX512_W64_PROCESS_BLOCK_AT

    avx512_store_state_families(ctx->state.row, g0, g1, g2, g3, g4, g5);
}

LLH_TARGET_AVX512
#define avx512_squeeze_rate_families(g0, g1, out)             \
    do {                                                       \
        _mm256_storeu_si256((__m256i *)(void *)(out), (g0));   \
        _mm256_storeu_si256((__m256i *)(void *)((out) + 32), (g1)); \
    } while (0)


LLH_TARGET_AVX512
static inline void avx512_hash_i2_shortpath_w64(
    const uint8_t *msg,
    size_t msg_len,
    uint64_t msg_len_bits,
    uint8_t *out)
{
    __m256i g0, g1, g2, g3, g4, g5;
    __m256i tail0, tail1;


    avx512_init_state_families_i2(&g0, &g1, &g2, &g3, &g4, &g5, msg_len_bits);

    if (msg_len == 0) {
        LLH_AVX512_W64_PERMUTE_LOCAL();
        avx512_squeeze_rate_families(g0, g1, out);
        return;
    }

    if (msg_len < RATE_BYTES) {
        avx512_load_padded_rate_block_w64(msg, msg_len, &tail0, &tail1);
        LLH_AVX512_W64_PROCESS_PREPARED_BLOCK_LOCAL(tail0, tail1);
        LLH_AVX512_W64_PERMUTE_LOCAL();
        avx512_squeeze_rate_families(g0, g1, out);
        return;
    }

    LLH_AVX512_W64_PROCESS_BLOCK_LOCAL(msg);

    if (msg_len > RATE_BYTES) {
        if (msg_len == (RATE_BYTES * 2)) {
            LLH_AVX512_W64_PROCESS_BLOCK_LOCAL(msg + RATE_BYTES);
        } else {
            avx512_load_padded_rate_block_w64(
                msg + RATE_BYTES,
                msg_len - RATE_BYTES,
                &tail0,
                &tail1);
            LLH_AVX512_W64_PROCESS_PREPARED_BLOCK_LOCAL(tail0, tail1);
        }
    }

    LLH_AVX512_W64_PERMUTE_LOCAL();
    avx512_squeeze_rate_families(g0, g1, out);
}

LLH_TARGET_AVX512
static void permutation_i2_avx512(State *s)
{
    __m256i g0, g1, g2, g3, g4, g5;

    avx512_load_state_families(s->row, &g0, &g1, &g2, &g3, &g4, &g5);
    LLH_AVX512_W64_PERMUTE_LOCAL();
    avx512_store_state_families(s->row, g0, g1, g2, g3, g4, g5);
}
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

    ctx->state.row[23] = msg_len_bits;
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

#if LLH_X86_INTRINSICS && (1 || 0) && LLH_ENABLE_AVX512_I2
LLH_TARGET_AVX512
static void avx512_hash_i2_fastpath(
    const uint8_t *msg,
    size_t msg_len,
    uint64_t msg_len_bits,
    uint8_t *out)
{
    size_t full_blocks = msg_len / RATE_BYTES;
    const size_t tail_len = msg_len - full_blocks * RATE_BYTES;
    __m256i g0, g1, g2, g3, g4, g5;

    if (msg_len < (RATE_BYTES * 2)) {
        avx512_hash_i2_shortpath_w64(msg, msg_len, msg_len_bits, out);
        return;
    }

    avx512_init_state_families_i2(&g0, &g1, &g2, &g3, &g4, &g5, msg_len_bits);
    for (; full_blocks != 0; full_blocks--, msg += RATE_BYTES) {
        LLH_AVX512_W64_PROCESS_BLOCK_LOCAL(msg);
    }

    if (tail_len != 0) {
        __m256i tail0, tail1;

        avx512_load_padded_rate_block_w64(msg, tail_len, &tail0, &tail1);
        LLH_AVX512_W64_PROCESS_PREPARED_BLOCK_LOCAL(tail0, tail1);
    }

    LLH_AVX512_W64_PERMUTE_LOCAL();
    avx512_squeeze_rate_families(g0, g1, out);
}
#endif

#if LLH_X86_INTRINSICS && LLH_ENABLE_AVX512_I2
#undef LLH_AVX512_W64_PROCESS_BLOCK_LOCAL
#undef LLH_AVX512_W64_PROCESS_PREPARED_BLOCK_LOCAL
#undef LLH_AVX512_W64_PERMUTE_LOCAL
#undef LLH_AVX512_W64_DO_ROUND_LOCAL
#endif


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
