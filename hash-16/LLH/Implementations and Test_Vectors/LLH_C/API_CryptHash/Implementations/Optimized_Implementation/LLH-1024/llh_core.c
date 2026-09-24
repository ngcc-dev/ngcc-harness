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

typedef __uint128_t word_t;

static inline word_t llh_rotl(word_t x, unsigned int n)
{
    return (word_t)((x << n) | (x >> (128u - n)));
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
    1,  16, 55,  118, 10, 29,  72, 11,
    35, 58, 105, 48,  76, 103, 26, 101,
    5,  36, 91,  42,  78, 113, 44, 127
};

typedef struct {
    uint64_t lanes[8];
} rc512_const_t;

#define LLH_W128_RC_LIST(X)                                     \
    X(B7E151628AED2A6A, BF7158809CF4F3C7)                        \
    X(62E7160F38B4DA56, A784D9045190CFEF)                        \
    X(324E7738926CFBE5, F4BF8D8D8C31D763)                        \
    X(DA06C80ABB1185EB, 4F7C7B5757F59584)                        \
    X(90CFD47D7C19BB42, 158D9554F7B46BCE)                        \
    X(D55C4D79FD5F24D6, 613C31C3839A2DDF)                        \
    X(8A9A276BCFBFA1C8, 77C56284DAB79CD4)                        \
    X(C2B3293D20E9E5EA, F02AC60ACC93ED87)                        \
    X(4422A52ECB238FEE, E5AB6ADD835FD1A0)                        \
    X(753D0A8F78E537D2, B95BB79D8DCAEC64)                        \
    X(2C1E9F23B829B5C2, 780BF38737DF8BB3)                        \
    X(00D01334A0D0BD86, 45CBFA73A6160FFE)                        \
    X(393C48CBBBCA060F, 0FF8EC6D31BEB5CC)                        \
    X(EED7F2F0BB088017, 163BC60DF45A0ECB)                        \
    X(1BCD289B06CBBFEA, 21AD08E1847F3F73)                        \
    X(78D56CED94640D6E, F0D3D37BE67008E1)                        \
    X(86D1BF275B9B241D, EB64749A47DFDFB9)                        \
    X(6632C3EB061B6472, BBF84C26144E49C2)                        \
    X(D04C324EF10DE513, D3F5114B8B5D374D)                        \
    X(93CB8879C7D52FFD, 72BA0AAE7277DA7B)                        \
    X(A1B4AF1488D8E836, AF14865E6C37AB68)                        \
    X(76FE690B57112138, 2AF341AFE94F77BC)                        \
    X(F06C83B8FF5675F0, 979074AD9A787BC5)                        \
    X(B9BD4B0C5937D3ED, E4C3A79396215EDA)                        \
    X(B1F57D0B5A7DB461, DD8F3C75540D0012)                        \
    X(1FD56E95F8C731E9, C4D7221BBED0C62B)                        \
    X(B5A87804B679A0CA, A41D802A4604C311)                        \
    X(B71DE3E5C6B400E0, 24A6668CCF2E2DE8)                        \
    X(6876E4F5C50000F0, A93B3AA7E6342B30)                        \
    X(2A0A47373B25F73E, 3B26D569FE2291AD)                        \
    X(36D6A147D1060B87, 1A2801F978376408)                        \
    X(2FF592D9140DB1E9, 399DF4B0E14CA8E8)                        \
    X(8EE9110B2BD4FA98, EED150CA6DD89322)                        \
    X(45EF7592C703F532, CE3A30CD31C070EB)                        \
    X(36B4195FF33FB1C6, 6C7D70F93918107C)                        \
    X(E2051FED33F6D1DE, 9491C7DEA6A5A442)                        \
    X(E154C8BB6D8D0362, 803BC248D414478C)                        \
    X(2AFB07FFE78E89B9, FECA7E3060C08F0D)                        \
    X(61F8E36801DF66D1, D8F9392E52CAEF06)                        \
    X(53199479DF2BE64B, BAAB008CA8A06FDA)

#define LLH_W128_RC_ENTRY_WORD(hi, lo) \
    (((__uint128_t)0x##hi##ull << 64) | 0x##lo##ull),

static const word_t RC[40] = {
    LLH_W128_RC_LIST(LLH_W128_RC_ENTRY_WORD)
};

#define LLH_W128_RC_ENTRY_AVX512(hi, lo) \
    { { 0ull, 0ull, 0ull, 0ull, 0ull, 0ull, 0x##lo##ull, 0x##hi##ull } },

static const ALIGN64 rc512_const_t RC_AVX512[40] = {
    LLH_W128_RC_LIST(LLH_W128_RC_ENTRY_AVX512)
};

#undef LLH_W128_RC_ENTRY_AVX512
#undef LLH_W128_RC_ENTRY_WORD
#undef LLH_W128_RC_LIST

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

#define load_word(p)                                                                  \
    ((__uint128_t)LLH_LOAD_U32_LE(p) |                                               \
     ((__uint128_t)LLH_LOAD_U32_LE((p) + 4) << 32) |                                \
     ((__uint128_t)LLH_LOAD_U32_LE((p) + 8) << 64) |                                \
     ((__uint128_t)LLH_LOAD_U32_LE((p) + 12) << 96))

#define store_word(p, w)                             \
    do {                                             \
        LLH_STORE_U32_LE((p), (uint32_t)(w));        \
        LLH_STORE_U32_LE((p) + 4, (uint32_t)((w) >> 32));  \
        LLH_STORE_U32_LE((p) + 8, (uint32_t)((w) >> 64));  \
        LLH_STORE_U32_LE((p) + 12, (uint32_t)((w) >> 96)); \
    } while (0)

typedef struct {
    State state;
    uint8_t buf[RATE_BYTES];
    size_t buf_len;
} HashCtx;

void hash_init(HashCtx *ctx, uint64_t msg_len_bits);
static void hash_final_i2(HashCtx *ctx, uint8_t *out);

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
} rotl128x2_consts_t;

#define ROTL128X2_CONSTS(n0, n1) {                                      \
    { (uint64_t)(n0), (uint64_t)(n0), (uint64_t)(n1), (uint64_t)(n1) } \
}

#define LLH_AVX2_RATE_PAIR_LIST(X) \
    X(0, 0)                        \
    X(1, 32)                       \
    X(2, 64)                       \
    X(3, 96)

#define LLH_AVX2_CAPACITY_PAIR_LIST(X) \
    X(4)                               \
    X(5)                               \
    X(6)                               \
    X(7)                               \
    X(8)                               \
    X(9)                               \
    X(10)                              \
    X(11)


static const ALIGN32 rotl128x2_consts_t SHIFTROWS_AVX2[12] = {
    ROTL128X2_CONSTS(1,  16),
    ROTL128X2_CONSTS(55, 118),
    ROTL128X2_CONSTS(10, 29),
    ROTL128X2_CONSTS(72, 11),
    ROTL128X2_CONSTS(35, 58),
    ROTL128X2_CONSTS(105, 48),
    ROTL128X2_CONSTS(76, 103),
    ROTL128X2_CONSTS(26, 101),
    ROTL128X2_CONSTS(5, 36),
    ROTL128X2_CONSTS(91, 42),
    ROTL128X2_CONSTS(78, 113),
    ROTL128X2_CONSTS(44, 127)
};

static inline __m256i rotl128x2_avx2(
    __m256i v,
    __m256i counts,
    __m256i shift_63,
    __m256i shift_64,
    __m256i shift_128)
{
    const __m256i swapped = _mm256_permute4x64_epi64(v, _MM_SHUFFLE(2, 3, 0, 1));
    const __m256i low_rot = _mm256_or_si256(
        _mm256_sllv_epi64(v, counts),
        _mm256_srlv_epi64(swapped, _mm256_sub_epi64(shift_64, counts)));
    const __m256i high_rot = _mm256_or_si256(
        _mm256_sllv_epi64(swapped, _mm256_sub_epi64(counts, shift_64)),
        _mm256_srlv_epi64(v, _mm256_sub_epi64(shift_128, counts)));
    const __m256i select_high = _mm256_cmpgt_epi64(counts, shift_63);

    return _mm256_blendv_epi8(low_rot, high_rot, select_high);
}

static inline __m256i avx2_pair_swap128(__m256i v)
{
    return _mm256_permute2x128_si256(v, v, 0x01);
}

static inline __m256i avx2_pair_lohi128(__m256i lo_src, __m256i hi_src)
{
    return _mm256_permute2x128_si256(lo_src, hi_src, 0x30);
}

#define avx2_pair_not(v, all_ones) _mm256_xor_si256((v), (all_ones))

#define avx2_shiftrows_pair(v, counts, shift_63, shift_64, shift_128) \
    rotl128x2_avx2((v), (counts), (shift_63), (shift_64), (shift_128))

#define avx2_state23_rc(rc_ptr)                                                      \
    _mm256_inserti128_si256(                                                         \
        _mm256_setzero_si256(),                                                      \
        _mm_loadu_si128((const __m128i *)(const void *)(rc_ptr)),                    \
        1)

static inline void avx2_round_i2(
    __m256i state_pairs[12],
    const word_t *rc_ptr,
    __m256i all_ones,
    __m256i shift_63,
    __m256i shift_64,
    __m256i shift_128);

static inline void avx2_permute_state_pairs(
    __m256i state_pairs[12],
    __m256i all_ones,
    __m256i shift_63,
    __m256i shift_64,
    __m256i shift_128)
{
    for (int round = 0; round < NR_DEFAULT; round++) {
        avx2_round_i2(
            state_pairs,
            &RC[round],
            all_ones,
            shift_63,
            shift_64,
            shift_128);
    }
}

static inline void avx2_load_state_pairs(__m256i pairs[12], const word_t *state)
{
    for (int i = 0; i < 12; i++) {
        pairs[i] = _mm256_loadu_si256((const __m256i *)(const void *)&state[2 * i]);
    }
}

static inline void avx2_store_state_pairs(word_t *state, const __m256i pairs[12])
{
    for (int i = 0; i < 12; i++) {
        _mm256_storeu_si256((__m256i *)(void *)&state[2 * i], pairs[i]);
    }
}

static inline void avx2_linear_family(
    __m256i x,
    __m256i y,
    __m256i *out_lo,
    __m256i *out_hi)
{
    const __m256i x_swapped = avx2_pair_swap128(x);
    const __m256i y_swapped = avx2_pair_swap128(y);
    const __m256i p = _mm256_xor_si256(x, x_swapped);
    const __m256i q = _mm256_xor_si256(y, y_swapped);

    *out_lo = _mm256_xor_si256(q, x_swapped);
    *out_hi = _mm256_xor_si256(p, y_swapped);
}

static inline void avx2_round_i2(
    __m256i state_pairs[12],
    const word_t *rc_ptr,
    __m256i all_ones,
    __m256i shift_63,
    __m256i shift_64,
    __m256i shift_128)
{
    __m256i t01[6];
    __m256i t23[6];
    __m256i x;
    __m256i y;
    __m256i out_lo;
    __m256i out_hi;

    t01[0] = avx2_pair_not(
        _mm256_xor_si256(
            _mm256_xor_si256(state_pairs[0], state_pairs[4]),
            _mm256_and_si256(state_pairs[2], state_pairs[6])),
        all_ones);
    t01[1] = avx2_pair_not(
        _mm256_xor_si256(
            _mm256_xor_si256(state_pairs[2], state_pairs[6]),
            _mm256_and_si256(state_pairs[4], state_pairs[8])),
        all_ones);
    t01[2] = _mm256_xor_si256(
        _mm256_xor_si256(state_pairs[4], state_pairs[8]),
        _mm256_and_si256(state_pairs[6], state_pairs[10]));
    t01[3] = avx2_pair_not(
        _mm256_xor_si256(
            _mm256_xor_si256(state_pairs[6], state_pairs[10]),
            _mm256_and_si256(state_pairs[8], state_pairs[0])),
        all_ones);
    t01[4] = avx2_pair_not(
        _mm256_xor_si256(
            _mm256_xor_si256(state_pairs[8], state_pairs[0]),
            _mm256_and_si256(state_pairs[10], state_pairs[2])),
        all_ones);
    t01[5] = _mm256_xor_si256(
        _mm256_xor_si256(state_pairs[10], state_pairs[2]),
        _mm256_and_si256(state_pairs[0], state_pairs[4]));

    t23[0] = avx2_pair_not(
        _mm256_xor_si256(
            _mm256_xor_si256(state_pairs[1], state_pairs[5]),
            _mm256_and_si256(state_pairs[3], state_pairs[7])),
        all_ones);
    t23[1] = avx2_pair_not(
        _mm256_xor_si256(
            _mm256_xor_si256(state_pairs[3], state_pairs[7]),
            _mm256_and_si256(state_pairs[5], state_pairs[9])),
        all_ones);
    t23[2] = _mm256_xor_si256(
        _mm256_xor_si256(state_pairs[5], state_pairs[9]),
        _mm256_and_si256(state_pairs[7], state_pairs[11]));
    t23[3] = avx2_pair_not(
        _mm256_xor_si256(
            _mm256_xor_si256(state_pairs[7], state_pairs[11]),
            _mm256_and_si256(state_pairs[9], state_pairs[1])),
        all_ones);
    t23[4] = avx2_pair_not(
        _mm256_xor_si256(
            _mm256_xor_si256(state_pairs[9], state_pairs[1]),
            _mm256_and_si256(state_pairs[11], state_pairs[3])),
        all_ones);
    t23[5] = _mm256_xor_si256(
        _mm256_xor_si256(state_pairs[11], state_pairs[3]),
        _mm256_and_si256(state_pairs[1], state_pairs[5]));

    x = avx2_pair_lohi128(t01[1], t01[2]);
    y = avx2_pair_lohi128(t23[4], t23[5]);
    avx2_linear_family(x, y, &out_lo, &out_hi);
    state_pairs[0] = avx2_shiftrows_pair(
        out_lo,
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX2[0].counts),
        shift_63,
        shift_64,
        shift_128);
    state_pairs[1] = avx2_shiftrows_pair(
        out_hi,
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX2[1].counts),
        shift_63,
        shift_64,
        shift_128);

    x = avx2_pair_lohi128(t01[2], t01[0]);
    y = avx2_pair_lohi128(t23[5], t23[3]);
    avx2_linear_family(x, y, &out_lo, &out_hi);
    state_pairs[2] = avx2_shiftrows_pair(
        out_lo,
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX2[2].counts),
        shift_63,
        shift_64,
        shift_128);
    state_pairs[3] = avx2_shiftrows_pair(
        out_hi,
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX2[3].counts),
        shift_63,
        shift_64,
        shift_128);

    x = avx2_pair_lohi128(t01[3], t01[4]);
    y = avx2_pair_lohi128(t23[1], t23[0]);
    avx2_linear_family(x, y, &out_lo, &out_hi);
    state_pairs[4] = avx2_shiftrows_pair(
        out_lo,
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX2[4].counts),
        shift_63,
        shift_64,
        shift_128);
    state_pairs[5] = avx2_shiftrows_pair(
        out_hi,
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX2[5].counts),
        shift_63,
        shift_64,
        shift_128);

    x = avx2_pair_lohi128(t01[5], t01[1]);
    y = avx2_pair_lohi128(t23[2], t23[4]);
    avx2_linear_family(x, y, &out_lo, &out_hi);
    state_pairs[6] = avx2_shiftrows_pair(
        out_lo,
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX2[6].counts),
        shift_63,
        shift_64,
        shift_128);
    state_pairs[7] = avx2_shiftrows_pair(
        out_hi,
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX2[7].counts),
        shift_63,
        shift_64,
        shift_128);

    x = avx2_pair_lohi128(t01[0], t01[5]);
    y = avx2_pair_lohi128(t23[3], t23[2]);
    avx2_linear_family(x, y, &out_lo, &out_hi);
    state_pairs[8] = avx2_shiftrows_pair(
        out_lo,
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX2[8].counts),
        shift_63,
        shift_64,
        shift_128);
    state_pairs[9] = avx2_shiftrows_pair(
        out_hi,
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX2[9].counts),
        shift_63,
        shift_64,
        shift_128);

    x = avx2_pair_lohi128(t01[4], t01[3]);
    y = avx2_pair_lohi128(t23[0], t23[1]);
    avx2_linear_family(x, y, &out_lo, &out_hi);
    state_pairs[10] = avx2_shiftrows_pair(
        out_lo,
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX2[10].counts),
        shift_63,
        shift_64,
        shift_128);
    state_pairs[11] = avx2_shiftrows_pair(
        out_hi,
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX2[11].counts),
        shift_63,
        shift_64,
        shift_128);

    state_pairs[11] = _mm256_xor_si256(state_pairs[11], avx2_state23_rc(rc_ptr));
}

#define avx2_xor_rate_pairs(state_pairs, block)                                      \
    do {                                                                             \
        (state_pairs)[0] = _mm256_xor_si256(                                         \
            (state_pairs)[0],                                                        \
            _mm256_loadu_si256((const __m256i *)(const void *)((block) + 0)));       \
        (state_pairs)[1] = _mm256_xor_si256(                                         \
            (state_pairs)[1],                                                        \
            _mm256_loadu_si256((const __m256i *)(const void *)((block) + 32)));      \
        (state_pairs)[2] = _mm256_xor_si256(                                         \
            (state_pairs)[2],                                                        \
            _mm256_loadu_si256((const __m256i *)(const void *)((block) + 64)));      \
        (state_pairs)[3] = _mm256_xor_si256(                                         \
            (state_pairs)[3],                                                        \
            _mm256_loadu_si256((const __m256i *)(const void *)((block) + 96)));      \
    } while (0)

static inline void avx2_load_padded_rate_block_w128(
    const uint8_t *src,
    size_t valid_bytes,
    uint8_t padded[RATE_BYTES])
{
    memset(padded, 0xFF, RATE_BYTES);
    if (valid_bytes != 0) {
        memcpy(padded, src, valid_bytes);
    }
}

static inline void avx2_process_prepared_block_w128(
    __m256i state_pairs[12],
    const uint8_t *block,
    __m256i all_ones,
    __m256i shift_63,
    __m256i shift_64,
    __m256i shift_128)
{
#define LLH_AVX2_SAVE_CAPACITY_PAIR_LOCAL(idx) const __m256i prev_##idx = state_pairs[idx];
    LLH_AVX2_CAPACITY_PAIR_LIST(LLH_AVX2_SAVE_CAPACITY_PAIR_LOCAL)
#undef LLH_AVX2_SAVE_CAPACITY_PAIR_LOCAL

    avx2_permute_state_pairs(
        state_pairs,
        all_ones,
        shift_63,
        shift_64,
        shift_128);

#define LLH_AVX2_FEEDFORWARD_CAPACITY_PAIR_LOCAL(idx) \
    state_pairs[idx] = _mm256_xor_si256(state_pairs[idx], prev_##idx);
    LLH_AVX2_CAPACITY_PAIR_LIST(LLH_AVX2_FEEDFORWARD_CAPACITY_PAIR_LOCAL)
#undef LLH_AVX2_FEEDFORWARD_CAPACITY_PAIR_LOCAL

    avx2_xor_rate_pairs(state_pairs, block);
}

static void avx2_process_full_blocks_i2(HashCtx *ctx, const uint8_t *data, size_t full_blocks)
{
    __m256i state_pairs[12];
    const __m256i all_ones = _mm256_set1_epi64x(-1);
    const __m256i shift_63 = _mm256_set1_epi64x(63);
    const __m256i shift_64 = _mm256_set1_epi64x(64);
    const __m256i shift_128 = _mm256_set1_epi64x(128);

    avx2_load_state_pairs(state_pairs, ctx->state.row);

    for (; full_blocks != 0; full_blocks--, data += RATE_BYTES) {
#define LLH_AVX2_SAVE_CAPACITY_PAIR(idx) const __m256i prev_##idx = state_pairs[idx];
        LLH_AVX2_CAPACITY_PAIR_LIST(LLH_AVX2_SAVE_CAPACITY_PAIR)
#undef LLH_AVX2_SAVE_CAPACITY_PAIR

        avx2_permute_state_pairs(
            state_pairs,
            all_ones,
            shift_63,
            shift_64,
            shift_128);

#define LLH_AVX2_FEEDFORWARD_CAPACITY_PAIR(idx) \
    state_pairs[idx] = _mm256_xor_si256(state_pairs[idx], prev_##idx);
        LLH_AVX2_CAPACITY_PAIR_LIST(LLH_AVX2_FEEDFORWARD_CAPACITY_PAIR)
#undef LLH_AVX2_FEEDFORWARD_CAPACITY_PAIR

        avx2_xor_rate_pairs(state_pairs, data);
    }

    avx2_store_state_pairs(ctx->state.row, state_pairs);
}

static void permutation_i2_avx2(State *s)
{
    __m256i state_pairs[12];
    const __m256i all_ones = _mm256_set1_epi64x(-1);
    const __m256i shift_63 = _mm256_set1_epi64x(63);
    const __m256i shift_64 = _mm256_set1_epi64x(64);
    const __m256i shift_128 = _mm256_set1_epi64x(128);

    avx2_load_state_pairs(state_pairs, s->row);
    avx2_permute_state_pairs(state_pairs, all_ones, shift_63, shift_64, shift_128);
    avx2_store_state_pairs(s->row, state_pairs);
}

static void avx2_hash_i2_fastpath(
    const uint8_t *msg,
    size_t msg_len,
    uint64_t msg_len_bits,
    uint8_t *out)
{
    HashCtx ctx;
    size_t full_blocks = msg_len / RATE_BYTES;
    const size_t tail_len = msg_len - full_blocks * RATE_BYTES;
    __m256i state_pairs[12];
    const __m256i all_ones = _mm256_set1_epi64x(-1);
    const __m256i shift_63 = _mm256_set1_epi64x(63);
    const __m256i shift_64 = _mm256_set1_epi64x(64);
    const __m256i shift_128 = _mm256_set1_epi64x(128);

    hash_init(&ctx, msg_len_bits);
    avx2_load_state_pairs(state_pairs, ctx.state.row);

    for (; full_blocks != 0; full_blocks--, msg += RATE_BYTES) {
        avx2_process_prepared_block_w128(
            state_pairs,
            msg,
            all_ones,
            shift_63,
            shift_64,
            shift_128);
    }

    if (tail_len != 0) {
        ALIGN32 uint8_t padded[RATE_BYTES];

        avx2_load_padded_rate_block_w128(msg, tail_len, padded);
        avx2_process_prepared_block_w128(
            state_pairs,
            padded,
            all_ones,
            shift_63,
            shift_64,
            shift_128);
    }

    avx2_store_state_pairs(ctx.state.row, state_pairs);
    hash_final_i2(&ctx, out);
}

#if LLH_ENABLE_AVX512_I2
typedef struct {
    uint64_t counts[8];
    __mmask8 select_high;
} rotl128x4_consts_t;

#define ROTL128X4_REPEAT2(v) (uint64_t)(v), (uint64_t)(v)

#define ROTL128X4_CONSTS(n0, n1, n2, n3) {                                                          \
    {                                                                                                \
        ROTL128X4_REPEAT2(n0),                                                                       \
        ROTL128X4_REPEAT2(n1),                                                                       \
        ROTL128X4_REPEAT2(n2),                                                                       \
        ROTL128X4_REPEAT2(n3)                                                                        \
    },                                                                                               \
    (__mmask8)((((n0) >= 64) ? 0x03 : 0x00) | (((n1) >= 64) ? 0x0C : 0x00) |                       \
               (((n2) >= 64) ? 0x30 : 0x00) | (((n3) >= 64) ? 0xC0 : 0x00))                        \
}


static const ALIGN64 rotl128x4_consts_t SHIFTROWS_AVX512[6] = {
    ROTL128X4_CONSTS(1,  16, 55,  118),
    ROTL128X4_CONSTS(10, 29,  72, 11),
    ROTL128X4_CONSTS(35, 58, 105, 48),
    ROTL128X4_CONSTS(76, 103, 26, 101),
    ROTL128X4_CONSTS(5,  36, 91,  42),
    ROTL128X4_CONSTS(78, 113, 44, 127)
};

static const ALIGN64 uint64_t SHIFTROWS_COUNTS_AVX512_W128[6][8] = {
    { 1ULL, 1ULL, 16ULL, 16ULL, 55ULL, 55ULL, 118ULL, 118ULL },
    { 10ULL, 10ULL, 29ULL, 29ULL, 72ULL, 72ULL, 11ULL, 11ULL },
    { 35ULL, 35ULL, 58ULL, 58ULL, 105ULL, 105ULL, 48ULL, 48ULL },
    { 76ULL, 76ULL, 103ULL, 103ULL, 26ULL, 26ULL, 101ULL, 101ULL },
    { 5ULL, 5ULL, 36ULL, 36ULL, 91ULL, 91ULL, 42ULL, 42ULL },
    { 78ULL, 78ULL, 113ULL, 113ULL, 44ULL, 44ULL, 127ULL, 127ULL }
};

static const ALIGN64 uint64_t BUILD_PERM_LO_AVX512_W128[8] = {
    0ULL, 1ULL, 10ULL, 11ULL, 0ULL, 1ULL, 10ULL, 11ULL
};

static const ALIGN64 uint64_t BUILD_PERM_HI_AVX512_W128[8] = {
    4ULL, 5ULL, 14ULL, 15ULL, 4ULL, 5ULL, 14ULL, 15ULL
};

#define LANE1_MASK ((__mmask8)0x0C)
#define LANE2_MASK ((__mmask8)0x30)
#define LANE3_MASK ((__mmask8)0xC0)
#define LANE23_MASK ((__mmask8)0xF0)
#define LLH_AVX512_W128_TERNLOG_XOR3 0x96
#define LLH_AVX512_W128_TERNLOG_XNOR3 0x69

#define LLH_AVX512_RATE_FAMILY_LIST(X) \
    X(0, 0)                            \
    X(1, 64)

#define LLH_AVX512_CAPACITY_FAMILY_LIST(X) \
    X(2)                                   \
    X(3)                                   \
    X(4)                                   \
    X(5)

#define LLH_AVX512_ROUND_LIST(X) \
    X(0)                         \
    X(1)                         \
    X(2)                         \
    X(3)                         \
    X(4)                         \
    X(5)                         \
    X(6)                         \
    X(7)                         \
    X(8)                         \
    X(9)                         \
    X(10)                        \
    X(11)                        \
    X(12)                        \
    X(13)                        \
    X(14)                        \
    X(15)                        \
    X(16)                        \
    X(17)                        \
    X(18)                        \
    X(19)                        \
    X(20)                        \
    X(21)                        \
    X(22)                        \
    X(23)                        \
    X(24)                        \
    X(25)                        \
    X(26)                        \
    X(27)                        \
    X(28)                        \
    X(29)                        \
    X(30)                        \
    X(31)                        \
    X(32)                        \
    X(33)                        \
    X(34)                        \
    X(35)                        \
    X(36)                        \
    X(37)                        \
    X(38)                        \
    X(39)

static const ALIGN64 unsigned char BUILD_MASKS_AVX512_W128[3] = {
    (unsigned char)LANE1_MASK,
    (unsigned char)LANE3_MASK,
    (unsigned char)LANE23_MASK
};

static const ALIGN64 unsigned char SHIFT_MASKS_AVX512_W128[6] = {
    SHIFTROWS_AVX512[0].select_high,
    SHIFTROWS_AVX512[1].select_high,
    SHIFTROWS_AVX512[2].select_high,
    SHIFTROWS_AVX512[3].select_high,
    SHIFTROWS_AVX512[4].select_high,
    SHIFTROWS_AVX512[5].select_high
};

static const ALIGN64 uint64_t SHIFT64_AVX512_ALL[8] = {
    64ULL, 64ULL, 64ULL, 64ULL, 64ULL, 64ULL, 64ULL, 64ULL
};

static const ALIGN64 uint64_t SHIFT128_AVX512_ALL[8] = {
    128ULL, 128ULL, 128ULL, 128ULL, 128ULL, 128ULL, 128ULL, 128ULL
};

LLH_TARGET_AVX512
static inline __m512i avx512_swap64_in128(__m512i v)
{
    return _mm512_castpd_si512(
        _mm512_shuffle_pd(_mm512_castsi512_pd(v), _mm512_castsi512_pd(v), 0x55));
}

LLH_TARGET_AVX512
static inline __m512i rotl128x4_avx512(
    __m512i v,
    const rotl128x4_consts_t *rot,
    __m512i shift_64,
    __m512i shift_128)
{
    const __m512i swapped = avx512_swap64_in128(v);
    const __m512i counts =
        _mm512_load_si512((const void *)rot->counts);
    const __m512i low_rot = _mm512_or_si512(
        _mm512_sllv_epi64(v, counts),
        _mm512_srlv_epi64(swapped, _mm512_sub_epi64(shift_64, counts)));
    const __m512i high_rot = _mm512_or_si512(
        _mm512_sllv_epi64(swapped, _mm512_sub_epi64(counts, shift_64)),
        _mm512_srlv_epi64(v, _mm512_sub_epi64(shift_128, counts)));

    return _mm512_mask_blend_epi64(rot->select_high, low_rot, high_rot);
}

LLH_TARGET_AVX512
static inline __m512i avx512_shiftrows_family(
    __m512i v,
    size_t table_idx,
    __m512i shift_64,
    __m512i shift_128)
{
    return rotl128x4_avx512(v, &SHIFTROWS_AVX512[table_idx], shift_64, shift_128);
}

LLH_TARGET_AVX512
#define avx512_load_rc(round_idx) _mm512_load_si512((const void *)&RC_AVX512[(round_idx)])

LLH_TARGET_AVX512
static inline __m512i avx512_total_xor_broadcast(
    __m512i v,
    LLH_UNUSED __m512i swap256,
    LLH_UNUSED __m512i swap128)
{
    const __m512i pair_xor =
        _mm512_xor_si512(v, _mm512_shuffle_i64x2(v, v, 0x4E));

    return _mm512_xor_si512(pair_xor, _mm512_shuffle_i64x2(pair_xor, pair_xor, 0xB1));
}

LLH_TARGET_AVX512
static inline __m512i avx512_linear_family(
    __m512i v,
    __m512i swap256,
    __m512i swap128)
{
    return _mm512_xor_si512(v, avx512_total_xor_broadcast(v, swap256, swap128));
}

LLH_TARGET_AVX512
#define avx512_xor3_epi64(a, b, c) \
    _mm512_ternarylogic_epi64((a), (b), (c), LLH_AVX512_W128_TERNLOG_XOR3)

LLH_TARGET_AVX512
#define avx512_xnor3_epi64(a, b, c) \
    _mm512_ternarylogic_epi64((a), (b), (c), LLH_AVX512_W128_TERNLOG_XNOR3)

LLH_TARGET_AVX512
#define avx512_build_family(lane0_src, lane1_src, lane2_src, lane3_src)      \
    _mm512_mask_blend_epi64(                                                  \
        LANE23_MASK,                                                          \
        _mm512_mask_blend_epi64(LANE1_MASK, (lane0_src), (lane1_src)),        \
        _mm512_mask_blend_epi64(LANE3_MASK, (lane2_src), (lane3_src)))

#define LLH_AVX512_W128_BUILD_LINEAR_SHIFT_ASM(dst_reg, src0_reg, src1_reg, src2_reg, src3_reg, count_reg, shift_k) \
    "vmovdqa64 " src0_reg ", " dst_reg "\n\t"                                                                       \
    "vpermt2q " src1_reg ", %%zmm29, " dst_reg "\n\t"                                                               \
    "vmovdqa64 " src2_reg ", %%zmm16\n\t"                                                                           \
    "vpermt2q " src3_reg ", %%zmm30, %%zmm16\n\t"                                                                   \
    "vshufi64x2 $68, %%zmm16, " dst_reg ", " dst_reg "\n\t"                                                         \
    "vshufi64x2 $78, " dst_reg ", " dst_reg ", %%zmm17\n\t"                                                        \
    "vpxorq " dst_reg ", %%zmm17, %%zmm17\n\t"                                                                      \
    "vshufi64x2 $177, %%zmm17, %%zmm17, %%zmm18\n\t"                                                                \
    "vpxorq %%zmm18, %%zmm17, %%zmm17\n\t"                                                                          \
    "vpxorq %%zmm17, " dst_reg ", " dst_reg "\n\t"                                                                  \
    "vshufpd $85, " dst_reg ", " dst_reg ", %%zmm17\n\t"                                                            \
    "vpsllvq " count_reg ", " dst_reg ", %%zmm18\n\t"                                                               \
    "vpsubq " count_reg ", %%zmm26, %%zmm16\n\t"                                                                    \
    "vpsrlvq %%zmm16, %%zmm17, %%zmm16\n\t"                                                                         \
    "vporq %%zmm16, %%zmm18, %%zmm18\n\t"                                                                           \
    "vpsubq %%zmm26, " count_reg ", %%zmm16\n\t"                                                                    \
    "vpsllvq %%zmm16, %%zmm17, %%zmm17\n\t"                                                                         \
    "vpsubq " count_reg ", %%zmm27, %%zmm16\n\t"                                                                    \
    "vpsrlvq %%zmm16, " dst_reg ", %%zmm16\n\t"                                                                     \
    "vporq %%zmm16, %%zmm17, %%zmm17\n\t"                                                                           \
    "vmovdqa64 %%zmm18, " dst_reg "\n\t"                                                                            \
    "vmovdqa64 %%zmm17, " dst_reg "%{" shift_k "%}\n\t"

#define LLH_AVX512_W128_DO_ROUND_ASM(round_idx)                                                                      \
    "vmovdqa64 %%zmm0, %%zmm6\n\t"                                                                                   \
    "vpandq %%zmm3, %%zmm1, %%zmm16\n\t"                                                                             \
    "vpternlogq $105, %%zmm16, %%zmm2, %%zmm6\n\t"                                                                   \
    "vmovdqa64 %%zmm1, %%zmm7\n\t"                                                                                   \
    "vpandq %%zmm4, %%zmm2, %%zmm16\n\t"                                                                             \
    "vpternlogq $105, %%zmm16, %%zmm3, %%zmm7\n\t"                                                                   \
    "vmovdqa64 %%zmm2, %%zmm8\n\t"                                                                                   \
    "vpandq %%zmm5, %%zmm3, %%zmm16\n\t"                                                                             \
    "vpternlogq $150, %%zmm16, %%zmm4, %%zmm8\n\t"                                                                   \
    "vmovdqa64 %%zmm3, %%zmm9\n\t"                                                                                   \
    "vpandq %%zmm0, %%zmm4, %%zmm16\n\t"                                                                             \
    "vpternlogq $105, %%zmm16, %%zmm5, %%zmm9\n\t"                                                                   \
    "vmovdqa64 %%zmm4, %%zmm10\n\t"                                                                                  \
    "vpandq %%zmm1, %%zmm5, %%zmm16\n\t"                                                                             \
    "vpternlogq $105, %%zmm16, %%zmm0, %%zmm10\n\t"                                                                  \
    "vmovdqa64 %%zmm5, %%zmm11\n\t"                                                                                  \
    "vpandq %%zmm2, %%zmm0, %%zmm16\n\t"                                                                             \
    "vpternlogq $150, %%zmm16, %%zmm1, %%zmm11\n\t"                                                                  \
    LLH_AVX512_W128_BUILD_LINEAR_SHIFT_ASM("%%zmm0", "%%zmm7", "%%zmm8", "%%zmm10", "%%zmm11", "%%zmm20", "%%k1") \
    LLH_AVX512_W128_BUILD_LINEAR_SHIFT_ASM("%%zmm1", "%%zmm8", "%%zmm6", "%%zmm11", "%%zmm9", "%%zmm21", "%%k2")  \
    LLH_AVX512_W128_BUILD_LINEAR_SHIFT_ASM("%%zmm2", "%%zmm9", "%%zmm10", "%%zmm7", "%%zmm6", "%%zmm22", "%%k3")  \
    LLH_AVX512_W128_BUILD_LINEAR_SHIFT_ASM("%%zmm3", "%%zmm11", "%%zmm7", "%%zmm8", "%%zmm10", "%%zmm23", "%%k4") \
    LLH_AVX512_W128_BUILD_LINEAR_SHIFT_ASM("%%zmm4", "%%zmm6", "%%zmm11", "%%zmm9", "%%zmm8", "%%zmm24", "%%k5")  \
    LLH_AVX512_W128_BUILD_LINEAR_SHIFT_ASM("%%zmm5", "%%zmm10", "%%zmm9", "%%zmm6", "%%zmm7", "%%zmm25", "%%k6")  \
    "vpxorq (%%rax), %%zmm5, %%zmm5\n\t"                                                                             \
    "addq $64, %%rax\n\t"

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE void avx512_process_blocks_i2_w128_asm(
    __m512i *g0,
    __m512i *g1,
    __m512i *g2,
    __m512i *g3,
    __m512i *g4,
    __m512i *g5,
    const uint8_t *data,
    size_t full_blocks)
{
    const uint8_t *block_ptr = data;
    size_t remaining_blocks = full_blocks;
    const void *const rc_base = RC_AVX512;
    const uint64_t *const shift_base = &SHIFTROWS_COUNTS_AVX512_W128[0][0];
    const unsigned char *const shift_masks_ptr = SHIFT_MASKS_AVX512_W128;

    if (remaining_blocks == 0) {
        return;
    }

    __asm__ volatile(
        "vmovdqu64 %[g0_mem], %%zmm0\n\t"
        "vmovdqu64 %[g1_mem], %%zmm1\n\t"
        "vmovdqu64 %[g2_mem], %%zmm2\n\t"
        "vmovdqu64 %[g3_mem], %%zmm3\n\t"
        "vmovdqu64 %[g4_mem], %%zmm4\n\t"
        "vmovdqu64 %[g5_mem], %%zmm5\n\t"
        "vmovdqa64 0(%[shift_base]), %%zmm20\n\t"
        "vmovdqa64 64(%[shift_base]), %%zmm21\n\t"
        "vmovdqa64 128(%[shift_base]), %%zmm22\n\t"
        "vmovdqa64 192(%[shift_base]), %%zmm23\n\t"
        "vmovdqa64 256(%[shift_base]), %%zmm24\n\t"
        "vmovdqa64 320(%[shift_base]), %%zmm25\n\t"
        "vmovdqa64 %[shift64_mem], %%zmm26\n\t"
        "vmovdqa64 %[shift128_mem], %%zmm27\n\t"
        "vmovdqa64 %[build_idx_lo_mem], %%zmm29\n\t"
        "vmovdqa64 %[build_idx_hi_mem], %%zmm30\n\t"
        "kmovb 0(%[shift_masks_ptr]), %%k1\n\t"
        "kmovb 1(%[shift_masks_ptr]), %%k2\n\t"
        "kmovb 2(%[shift_masks_ptr]), %%k3\n\t"
        "kmovb 3(%[shift_masks_ptr]), %%k4\n\t"
        "kmovb 4(%[shift_masks_ptr]), %%k5\n\t"
        "kmovb 5(%[shift_masks_ptr]), %%k6\n\t"
        "1:\n\t"
        "vmovdqa64 %%zmm2, %%zmm12\n\t"
        "vmovdqa64 %%zmm3, %%zmm13\n\t"
        "vmovdqa64 %%zmm4, %%zmm14\n\t"
        "vmovdqa64 %%zmm5, %%zmm15\n\t"
        "movq %[rc_base], %%rax\n\t"
        LLH_AVX512_ROUND_LIST(LLH_AVX512_W128_DO_ROUND_ASM)
        "vpxorq %%zmm12, %%zmm2, %%zmm2\n\t"
        "vpxorq %%zmm13, %%zmm3, %%zmm3\n\t"
        "vpxorq %%zmm14, %%zmm4, %%zmm4\n\t"
        "vpxorq %%zmm15, %%zmm5, %%zmm5\n\t"
        "vmovdqu64 (%[data_ptr]), %%zmm16\n\t"
        "vmovdqu64 64(%[data_ptr]), %%zmm17\n\t"
        "vpxorq %%zmm16, %%zmm0, %%zmm0\n\t"
        "vpxorq %%zmm17, %%zmm1, %%zmm1\n\t"
        "addq $128, %[data_ptr]\n\t"
        "dec %[blocks]\n\t"
        "jnz 1b\n\t"
        "vmovdqu64 %%zmm0, %[g0_mem]\n\t"
        "vmovdqu64 %%zmm1, %[g1_mem]\n\t"
        "vmovdqu64 %%zmm2, %[g2_mem]\n\t"
        "vmovdqu64 %%zmm3, %[g3_mem]\n\t"
        "vmovdqu64 %%zmm4, %[g4_mem]\n\t"
        "vmovdqu64 %%zmm5, %[g5_mem]\n\t"
        : [data_ptr] "+r"(block_ptr),
          [blocks] "+r"(remaining_blocks),
          [g0_mem] "+m"(*g0),
          [g1_mem] "+m"(*g1),
          [g2_mem] "+m"(*g2),
          [g3_mem] "+m"(*g3),
          [g4_mem] "+m"(*g4),
          [g5_mem] "+m"(*g5)
        : [rc_base] "r"(rc_base),
          [shift_base] "r"(shift_base),
          [shift_masks_ptr] "r"(shift_masks_ptr),
          [build_idx_lo_mem] "m"(BUILD_PERM_LO_AVX512_W128),
          [build_idx_hi_mem] "m"(BUILD_PERM_HI_AVX512_W128),
          [shift64_mem] "m"(SHIFT64_AVX512_ALL),
          [shift128_mem] "m"(SHIFT128_AVX512_ALL)
        : "cc",
          "memory",
          "rax",
          "zmm0", "zmm1", "zmm2", "zmm3", "zmm4", "zmm5",
          "zmm6", "zmm7", "zmm8", "zmm9", "zmm10", "zmm11",
          "zmm12", "zmm13", "zmm14", "zmm15", "zmm16", "zmm17",
          "zmm18", "zmm20", "zmm21", "zmm22", "zmm23", "zmm24",
          "zmm25", "zmm26", "zmm27", "zmm28", "zmm29", "zmm30",
          "k1", "k2", "k3", "k4", "k5", "k6");
}

LLH_TARGET_AVX512
static inline void avx512_init_state_families_i2(
    __m512i *g0,
    __m512i *g1,
    __m512i *g2,
    __m512i *g3,
    __m512i *g4,
    __m512i *g5,
    uint64_t msg_len_bits)
{
    const __m512i zero = _mm512_setzero_si512();

    *g0 = zero;
    *g1 = zero;
    *g2 = zero;
    *g3 = zero;
    *g4 = zero;
    *g5 = _mm512_set_epi64(
        0LL,
        (long long)msg_len_bits,
        0LL,
        0LL,
        0LL,
        0LL,
        0LL,
        0LL);
}

LLH_TARGET_AVX512
static inline __mmask64 avx512_prefix_mask64(size_t count)
{
    if (count == 0) {
        return (__mmask64)0;
    }
    if (count >= 64) {
        return ~(__mmask64)0;
    }
    return ((__mmask64)1ULL << count) - 1ULL;
}

LLH_TARGET_AVX512
static inline __m512i avx512_load_padded_chunk64(const uint8_t *src, size_t valid_bytes)
{
    const __m512i fill = _mm512_set1_epi8((char)0xFF);
    const __mmask64 valid_mask = avx512_prefix_mask64(valid_bytes);

    if (valid_bytes == 0) {
        return fill;
    }

    {
        const __m512i loaded = _mm512_maskz_loadu_epi8(valid_mask, src);
        return _mm512_mask_blend_epi8(valid_mask, fill, loaded);
    }
}

LLH_TARGET_AVX512
static inline void avx512_load_padded_rate_block_w128(
    const uint8_t *src,
    size_t valid_bytes,
    __m512i *block0,
    __m512i *block1)
{
    const size_t low_bytes = (valid_bytes < 64) ? valid_bytes : 64;
    const size_t high_bytes = (valid_bytes > 64) ? (valid_bytes - 64) : 0;
    const uint8_t *src_hi = (high_bytes != 0) ? (src + 64) : src;

    *block0 = avx512_load_padded_chunk64(src, low_bytes);
    *block1 = avx512_load_padded_chunk64(src_hi, high_bytes);
}

LLH_TARGET_AVX512
#define avx512_xor_rate_vectors(g0, g1, block0, block1) \
    do {                                                 \
        *(g0) = _mm512_xor_si512(*(g0), (block0));       \
        *(g1) = _mm512_xor_si512(*(g1), (block1));       \
    } while (0)

LLH_TARGET_AVX512
static inline void avx512_load_state_families(
    const word_t *state,
    __m512i *g0,
    __m512i *g1,
    __m512i *g2,
    __m512i *g3,
    __m512i *g4,
    __m512i *g5)
{
    *g0 = _mm512_loadu_si512((const void *)&state[0]);
    *g1 = _mm512_loadu_si512((const void *)&state[4]);
    *g2 = _mm512_loadu_si512((const void *)&state[8]);
    *g3 = _mm512_loadu_si512((const void *)&state[12]);
    *g4 = _mm512_loadu_si512((const void *)&state[16]);
    *g5 = _mm512_loadu_si512((const void *)&state[20]);
}

LLH_TARGET_AVX512
static inline void avx512_store_state_families(
    word_t *state,
    __m512i g0,
    __m512i g1,
    __m512i g2,
    __m512i g3,
    __m512i g4,
    __m512i g5)
{
    _mm512_storeu_si512((void *)&state[0], g0);
    _mm512_storeu_si512((void *)&state[4], g1);
    _mm512_storeu_si512((void *)&state[8], g2);
    _mm512_storeu_si512((void *)&state[12], g3);
    _mm512_storeu_si512((void *)&state[16], g4);
    _mm512_storeu_si512((void *)&state[20], g5);
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE void avx512_round_i2(
    __m512i *g0,
    __m512i *g1,
    __m512i *g2,
    __m512i *g3,
    __m512i *g4,
    __m512i *g5,
    __m512i rc_vec,
    __m512i swap256,
    __m512i swap128,
    __m512i shift_64,
    __m512i shift_128)
{
    const __m512i g1g3 = _mm512_and_si512(*g1, *g3);
    const __m512i g2g4 = _mm512_and_si512(*g2, *g4);
    const __m512i g3g5 = _mm512_and_si512(*g3, *g5);
    const __m512i g4g0 = _mm512_and_si512(*g4, *g0);
    const __m512i g5g1 = _mm512_and_si512(*g5, *g1);
    const __m512i g0g2 = _mm512_and_si512(*g0, *g2);
    const __m512i n0 = avx512_xnor3_epi64(*g0, *g2, g1g3);
    const __m512i n1 = avx512_xnor3_epi64(*g1, *g3, g2g4);
    const __m512i u2 = avx512_xor3_epi64(*g2, *g4, g3g5);
    const __m512i n3 = avx512_xnor3_epi64(*g3, *g5, g4g0);
    const __m512i n4 = avx512_xnor3_epi64(*g4, *g0, g5g1);
    const __m512i u5 = avx512_xor3_epi64(*g5, *g1, g0g2);

    *g0 = avx512_shiftrows_family(
        avx512_linear_family(avx512_build_family(n1, u2, n4, u5), swap256, swap128),
        0,
        shift_64,
        shift_128);

    *g1 = avx512_shiftrows_family(
        avx512_linear_family(avx512_build_family(u2, n0, u5, n3), swap256, swap128),
        1,
        shift_64,
        shift_128);

    *g2 = avx512_shiftrows_family(
        avx512_linear_family(avx512_build_family(n3, n4, n1, n0), swap256, swap128),
        2,
        shift_64,
        shift_128);

    *g3 = avx512_shiftrows_family(
        avx512_linear_family(avx512_build_family(u5, n1, u2, n4), swap256, swap128),
        3,
        shift_64,
        shift_128);

    *g4 = avx512_shiftrows_family(
        avx512_linear_family(avx512_build_family(n0, u5, n3, u2), swap256, swap128),
        4,
        shift_64,
        shift_128);

    *g5 = avx512_shiftrows_family(
        avx512_linear_family(avx512_build_family(n4, n3, n0, n1), swap256, swap128),
        5,
        shift_64,
        shift_128);

    *g5 = _mm512_xor_si512(*g5, rc_vec);
}

/*
 * Keep the W=128 hot path fully local to avoid out-of-line round calls and
 * the accompanying stack spills that GCC emits for the helper-based skeleton.
 */
#define LLH_AVX512_W128_DO_ROUND_LOCAL(round_idx)                                                                                  \
    {                                                                                                                                   \
        const __m512i llh_rc_vec = avx512_load_rc(round_idx);                                                                           \
        const __m512i llh_g1g3 = _mm512_and_si512(g1, g3);                                                                              \
        const __m512i llh_g2g4 = _mm512_and_si512(g2, g4);                                                                              \
        const __m512i llh_g3g5 = _mm512_and_si512(g3, g5);                                                                              \
        const __m512i llh_g4g0 = _mm512_and_si512(g4, g0);                                                                              \
        const __m512i llh_g5g1 = _mm512_and_si512(g5, g1);                                                                              \
        const __m512i llh_g0g2 = _mm512_and_si512(g0, g2);                                                                              \
        const __m512i llh_n0 = avx512_xnor3_epi64(g0, g2, llh_g1g3);                                                                    \
        const __m512i llh_n1 = avx512_xnor3_epi64(g1, g3, llh_g2g4);                                                                    \
        const __m512i llh_u2 = avx512_xor3_epi64(g2, g4, llh_g3g5);                                                                     \
        const __m512i llh_n3 = avx512_xnor3_epi64(g3, g5, llh_g4g0);                                                                    \
        const __m512i llh_n4 = avx512_xnor3_epi64(g4, g0, llh_g5g1);                                                                    \
        const __m512i llh_u5 = avx512_xor3_epi64(g5, g1, llh_g0g2);                                                                     \
                                                                                                                                         \
        g0 = avx512_shiftrows_family(                                                                                                   \
            avx512_linear_family(avx512_build_family(llh_n1, llh_u2, llh_n4, llh_u5), llh_swap256, llh_swap128),                     \
            0,                                                                                                                           \
            llh_shift_64,                                                                                                                \
            llh_shift_128);                                                                                                               \
        g1 = avx512_shiftrows_family(                                                                                                   \
            avx512_linear_family(avx512_build_family(llh_u2, llh_n0, llh_u5, llh_n3), llh_swap256, llh_swap128),                     \
            1,                                                                                                                           \
            llh_shift_64,                                                                                                                \
            llh_shift_128);                                                                                                               \
        g2 = avx512_shiftrows_family(                                                                                                   \
            avx512_linear_family(avx512_build_family(llh_n3, llh_n4, llh_n1, llh_n0), llh_swap256, llh_swap128),                     \
            2,                                                                                                                           \
            llh_shift_64,                                                                                                                \
            llh_shift_128);                                                                                                               \
        g3 = avx512_shiftrows_family(                                                                                                   \
            avx512_linear_family(avx512_build_family(llh_u5, llh_n1, llh_u2, llh_n4), llh_swap256, llh_swap128),                     \
            3,                                                                                                                           \
            llh_shift_64,                                                                                                                \
            llh_shift_128);                                                                                                               \
        g4 = avx512_shiftrows_family(                                                                                                   \
            avx512_linear_family(avx512_build_family(llh_n0, llh_u5, llh_n3, llh_u2), llh_swap256, llh_swap128),                     \
            4,                                                                                                                           \
            llh_shift_64,                                                                                                                \
            llh_shift_128);                                                                                                               \
        g5 = avx512_shiftrows_family(                                                                                                   \
            avx512_linear_family(avx512_build_family(llh_n4, llh_n3, llh_n0, llh_n1), llh_swap256, llh_swap128),                     \
            5,                                                                                                                           \
            llh_shift_64,                                                                                                                \
            llh_shift_128);                                                                                                               \
        g5 = _mm512_xor_si512(g5, llh_rc_vec);                                                                                          \
    }

#define LLH_AVX512_W128_PERMUTE_LOCAL()                                                  \
    do {                                                                                 \
        const __m512i llh_swap256 = _mm512_setr_epi64(4, 5, 6, 7, 0, 1, 2, 3);          \
        const __m512i llh_swap128 = _mm512_setr_epi64(2, 3, 0, 1, 6, 7, 4, 5);          \
        const __m512i llh_shift_64 = _mm512_set1_epi64(64);                              \
        const __m512i llh_shift_128 = _mm512_set1_epi64(128);                            \
        LLH_AVX512_ROUND_LIST(LLH_AVX512_W128_DO_ROUND_LOCAL)                            \
    } while (0)

LLH_TARGET_AVX512
static inline void avx512_xor_rate_families(
    __m512i *g0,
    __m512i *g1,
    const uint8_t *block)
{
    const __m512i block0 =
        _mm512_loadu_si512((const void *)(block + 0));
    const __m512i block1 =
        _mm512_loadu_si512((const void *)(block + 64));

    avx512_xor_rate_vectors(g0, g1, block0, block1);
}

#define LLH_AVX512_W128_PROCESS_PREPARED_BLOCK_LOCAL(block0_vec, block1_vec)    \
    do {                                                                                    \
        const __m512i llh_prev2 = g2;                                                       \
        const __m512i llh_prev3 = g3;                                                       \
        const __m512i llh_prev4 = g4;                                                       \
        const __m512i llh_prev5 = g5;                                                       \
                                                                                            \
        LLH_AVX512_W128_PERMUTE_LOCAL();                                                    \
                                                                                            \
        g2 = _mm512_xor_si512(g2, llh_prev2);                                               \
        g3 = _mm512_xor_si512(g3, llh_prev3);                                               \
        g4 = _mm512_xor_si512(g4, llh_prev4);                                               \
        g5 = _mm512_xor_si512(g5, llh_prev5);                                               \
                                                                                            \
        avx512_xor_rate_vectors(&g0, &g1, (block0_vec), (block1_vec));                      \
    } while (0)

#define LLH_AVX512_W128_PROCESS_BLOCK_LOCAL(block_ptr)                                 \
    do {                                                                              \
        const __m512i llh_block0 =                                                   \
            _mm512_loadu_si512((const void *)((block_ptr) + 0));                      \
        const __m512i llh_block1 =                                                   \
            _mm512_loadu_si512((const void *)((block_ptr) + 64));                     \
                                                                                        \
        LLH_AVX512_W128_PROCESS_PREPARED_BLOCK_LOCAL(                                 \
            llh_block0,                                                               \
            llh_block1);                                                              \
    } while (0)

LLH_TARGET_AVX512
static void avx512_process_full_blocks_i2(HashCtx *ctx, const uint8_t *data, size_t full_blocks)
{
    __m512i g0, g1, g2, g3, g4, g5;

    avx512_load_state_families(ctx->state.row, &g0, &g1, &g2, &g3, &g4, &g5);

    avx512_process_blocks_i2_w128_asm(&g0, &g1, &g2, &g3, &g4, &g5, data, full_blocks);

    avx512_store_state_families(ctx->state.row, g0, g1, g2, g3, g4, g5);
}

LLH_TARGET_AVX512
#define avx512_squeeze_rate_families(g0, g1, out) \
    do {                                          \
        _mm512_storeu_si512((void *)(out), (g0)); \
        _mm512_storeu_si512((void *)((out) + 64), (g1)); \
    } while (0)

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE void avx512_hash_i2_shortpath_w128(
    const uint8_t *msg,
    size_t msg_len,
    uint64_t msg_len_bits,
    uint8_t *out)
{
    __m512i g0, g1, g2, g3, g4, g5;
    __m512i tail0, tail1;

    avx512_init_state_families_i2(&g0, &g1, &g2, &g3, &g4, &g5, msg_len_bits);

    if (msg_len == 0) {
        LLH_AVX512_W128_PERMUTE_LOCAL();
        avx512_squeeze_rate_families(g0, g1, out);
        return;
    }

    if (msg_len < RATE_BYTES) {
        avx512_load_padded_rate_block_w128(msg, msg_len, &tail0, &tail1);
        LLH_AVX512_W128_PROCESS_PREPARED_BLOCK_LOCAL(tail0, tail1);
        LLH_AVX512_W128_PERMUTE_LOCAL();
        avx512_squeeze_rate_families(g0, g1, out);
        return;
    }

    LLH_AVX512_W128_PROCESS_BLOCK_LOCAL(msg);

    if (msg_len > RATE_BYTES) {
        avx512_load_padded_rate_block_w128(
            msg + RATE_BYTES,
            msg_len - RATE_BYTES,
            &tail0,
            &tail1);
        LLH_AVX512_W128_PROCESS_PREPARED_BLOCK_LOCAL(tail0, tail1);
    }

    LLH_AVX512_W128_PERMUTE_LOCAL();
    avx512_squeeze_rate_families(g0, g1, out);
}

LLH_TARGET_AVX512
static void permutation_i2_avx512(State *s)
{
    __m512i g0, g1, g2, g3, g4, g5;

    avx512_load_state_families(s->row, &g0, &g1, &g2, &g3, &g4, &g5);
    LLH_AVX512_W128_PERMUTE_LOCAL();
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

    ctx->state.row[23] = (__uint128_t)msg_len_bits;
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

#if LLH_X86_INTRINSICS && (0 || 1) && LLH_ENABLE_AVX512_I2
LLH_TARGET_AVX512
static void avx512_hash_i2_fastpath(
    const uint8_t *msg,
    size_t msg_len,
    uint64_t msg_len_bits,
    uint8_t *out)
{
    size_t full_blocks = msg_len / RATE_BYTES;
    const size_t tail_len = msg_len - full_blocks * RATE_BYTES;

    if (msg_len < (RATE_BYTES * 2)) {
        avx512_hash_i2_shortpath_w128(msg, msg_len, msg_len_bits, out);
        return;
    }

    __m512i g0, g1, g2, g3, g4, g5;
    avx512_init_state_families_i2(&g0, &g1, &g2, &g3, &g4, &g5, msg_len_bits);
    for (; full_blocks != 0; full_blocks--, msg += RATE_BYTES) {
        LLH_AVX512_W128_PROCESS_BLOCK_LOCAL(msg);
    }

    if (tail_len != 0) {
        __m512i tail0, tail1;

        avx512_load_padded_rate_block_w128(msg, tail_len, &tail0, &tail1);
        LLH_AVX512_W128_PROCESS_PREPARED_BLOCK_LOCAL(tail0, tail1);
    }

    LLH_AVX512_W128_PERMUTE_LOCAL();
    avx512_squeeze_rate_families(g0, g1, out);
}
#endif


#if LLH_X86_INTRINSICS && LLH_ENABLE_AVX512_I2
#undef LLH_AVX512_W128_PROCESS_BLOCK_LOCAL
#undef LLH_AVX512_W128_PROCESS_PREPARED_BLOCK_LOCAL
#undef LLH_AVX512_W128_PERMUTE_LOCAL
#undef LLH_AVX512_W128_DO_ROUND_LOCAL
#endif

static void hash_i2_fallback(
    const uint8_t *msg,
    size_t msg_len,
    uint64_t msg_len_bits,
    uint8_t *out)
{
    HashCtx ctx;

    hash_init(&ctx, msg_len_bits);

#if LLH_X86_INTRINSICS
    {
        size_t full_blocks = msg_len / RATE_BYTES;

        if (full_blocks != 0) {
            avx2_process_full_blocks_i2(&ctx, msg, full_blocks);
            msg += full_blocks * RATE_BYTES;
            msg_len -= full_blocks * RATE_BYTES;
        }

        if (msg_len != 0) {
            uint8_t final_block[RATE_BYTES];

            memcpy(final_block, msg, msg_len);
            memset(final_block + msg_len, 0xFF, RATE_BYTES - msg_len);
            hash_process_block_i2(&ctx, final_block);
        }

        permutation_i2(&ctx.state);
        squeeze_rate(&ctx.state, out);
        return;
    }
#else
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
#endif
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
