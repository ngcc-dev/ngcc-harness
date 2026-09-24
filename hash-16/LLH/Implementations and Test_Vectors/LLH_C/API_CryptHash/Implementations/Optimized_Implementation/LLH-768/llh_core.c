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

typedef struct {
    uint32_t hi;
    uint64_t lo;
} word_t;

static inline word_t word_not(word_t a)
{
    return (word_t){~a.hi, ~a.lo};
}

static inline word_t word_xor(word_t a, word_t b)
{
    return (word_t){a.hi ^ b.hi, a.lo ^ b.lo};
}

static inline word_t word_and(word_t a, word_t b)
{
    return (word_t){a.hi & b.hi, a.lo & b.lo};
}

static inline word_t llh_rotl_1_31(word_t x, unsigned int n)
{
    uint32_t hi = (uint32_t)((x.hi << n) | (uint32_t)(x.lo >> (64u - n)));
    uint64_t lo = (x.lo << n) | ((uint64_t)x.hi >> (32u - n));
    return (word_t){hi, lo};
}

static inline word_t llh_rotl_32(word_t x)
{
    uint32_t hi = (uint32_t)(x.lo >> 32u);
    uint64_t lo = (x.lo << 32u) | (uint64_t)x.hi;
    return (word_t){hi, lo};
}

static inline word_t llh_rotl_33_63(word_t x, unsigned int n)
{
    unsigned int k = n - 32u;
    uint32_t hi = (uint32_t)(x.lo >> (32u - k));
    uint64_t lo = (x.lo << n) |
                  ((uint64_t)x.hi << k) |
                  (x.lo >> (64u - k));
    return (word_t){hi, lo};
}

static inline word_t llh_rotl_64_95(word_t x, unsigned int n)
{
    unsigned int k = n - 64u;
    uint32_t hi = (uint32_t)((x.lo << k) |
                             ((uint64_t)x.hi >> (32u - k)));
    uint64_t lo = ((uint64_t)x.hi << (32u + k)) |
                  (x.lo >> (32u - k));
    return (word_t){hi, lo};
}

#define WORD_NOT(a) word_not(a)
#define WORD_XOR(a, b) word_xor((a), (b))
#define WORD_AND(a, b) word_and((a), (b))


#define RATE_BITS (RATE_ROWS * W)
#define RATE_BYTES (RATE_BITS / 8)
#define OUTPUT_BYTES RATE_BYTES
#define NR_DEFAULT (8 + (W / 32) * 8)

typedef struct {
    word_t row[ROWS];
} State;

static const int SIGMA[ROWS] = {
    2,  11, 28, 53, 19, 34, 57, 88,
    52, 73, 6,  43, 5,  32, 67, 14,
    70, 7,  48, 1,  55, 94, 45, 4
};

#if LLH_X86_INTRINSICS
typedef struct {
    uint32_t c0[4];
    uint32_t c1[4];
    uint32_t c2[4];
} rc384_const_t;
#endif

static const word_t RC[32] = {
    {0xB7E15162u, 0x8AED2A6ABF715880ull},
    {0x9CF4F3C7u, 0x62E7160F38B4DA56ull},
    {0xA784D904u, 0x5190CFEF324E7738ull},
    {0x926CFBE5u, 0xF4BF8D8D8C31D763ull},
    {0xDA06C80Au, 0xBB1185EB4F7C7B57ull},
    {0x57F59584u, 0x90CFD47D7C19BB42ull},
    {0x158D9554u, 0xF7B46BCED55C4D79ull},
    {0xFD5F24D6u, 0x613C31C3839A2DDFull},
    {0x8A9A276Bu, 0xCFBFA1C877C56284ull},
    {0xDAB79CD4u, 0xC2B3293D20E9E5EAull},
    {0xF02AC60Au, 0xCC93ED874422A52Eull},
    {0xCB238FEEu, 0xE5AB6ADD835FD1A0ull},
    {0x753D0A8Fu, 0x78E537D2B95BB79Dull},
    {0x8DCAEC64u, 0x2C1E9F23B829B5C2ull},
    {0x780BF387u, 0x37DF8BB300D01334ull},
    {0xA0D0BD86u, 0x45CBFA73A6160FFEull},
    {0x393C48CBu, 0xBBCA060F0FF8EC6Dull},
    {0x31BEB5CCu, 0xEED7F2F0BB088017ull},
    {0x163BC60Du, 0xF45A0ECB1BCD289Bull},
    {0x06CBBFEAu, 0x21AD08E1847F3F73ull},
    {0x78D56CEDu, 0x94640D6EF0D3D37Bull},
    {0xE67008E1u, 0x86D1BF275B9B241Dull},
    {0xEB64749Au, 0x47DFDFB96632C3EBull},
    {0x061B6472u, 0xBBF84C26144E49C2ull},
    {0xD04C324Eu, 0xF10DE513D3F5114Bull},
    {0x8B5D374Du, 0x93CB8879C7D52FFDull},
    {0x72BA0AAEu, 0x7277DA7BA1B4AF14ull},
    {0x88D8E836u, 0xAF14865E6C37AB68ull},
    {0x76FE690Bu, 0x571121382AF341AFull},
    {0xE94F77BCu, 0xF06C83B8FF5675F0ull},
    {0x979074ADu, 0x9A787BC5B9BD4B0Cull},
    {0x5937D3EDu, 0xE4C3A79396215EDAull}
};

#if LLH_X86_INTRINSICS
static const ALIGN32 rc384_const_t RC_AVX512_W96[32] = {
    { { 0u, 0u, 0u, 0xB7E15162u }, { 0u, 0u, 0u, 0x8AED2A6Au }, { 0u, 0u, 0u, 0xBF715880u } },
    { { 0u, 0u, 0u, 0x9CF4F3C7u }, { 0u, 0u, 0u, 0x62E7160Fu }, { 0u, 0u, 0u, 0x38B4DA56u } },
    { { 0u, 0u, 0u, 0xA784D904u }, { 0u, 0u, 0u, 0x5190CFEFu }, { 0u, 0u, 0u, 0x324E7738u } },
    { { 0u, 0u, 0u, 0x926CFBE5u }, { 0u, 0u, 0u, 0xF4BF8D8Du }, { 0u, 0u, 0u, 0x8C31D763u } },
    { { 0u, 0u, 0u, 0xDA06C80Au }, { 0u, 0u, 0u, 0xBB1185EBu }, { 0u, 0u, 0u, 0x4F7C7B57u } },
    { { 0u, 0u, 0u, 0x57F59584u }, { 0u, 0u, 0u, 0x90CFD47Du }, { 0u, 0u, 0u, 0x7C19BB42u } },
    { { 0u, 0u, 0u, 0x158D9554u }, { 0u, 0u, 0u, 0xF7B46BCEu }, { 0u, 0u, 0u, 0xD55C4D79u } },
    { { 0u, 0u, 0u, 0xFD5F24D6u }, { 0u, 0u, 0u, 0x613C31C3u }, { 0u, 0u, 0u, 0x839A2DDFu } },
    { { 0u, 0u, 0u, 0x8A9A276Bu }, { 0u, 0u, 0u, 0xCFBFA1C8u }, { 0u, 0u, 0u, 0x77C56284u } },
    { { 0u, 0u, 0u, 0xDAB79CD4u }, { 0u, 0u, 0u, 0xC2B3293Du }, { 0u, 0u, 0u, 0x20E9E5EAu } },
    { { 0u, 0u, 0u, 0xF02AC60Au }, { 0u, 0u, 0u, 0xCC93ED87u }, { 0u, 0u, 0u, 0x4422A52Eu } },
    { { 0u, 0u, 0u, 0xCB238FEEu }, { 0u, 0u, 0u, 0xE5AB6ADDu }, { 0u, 0u, 0u, 0x835FD1A0u } },
    { { 0u, 0u, 0u, 0x753D0A8Fu }, { 0u, 0u, 0u, 0x78E537D2u }, { 0u, 0u, 0u, 0xB95BB79Du } },
    { { 0u, 0u, 0u, 0x8DCAEC64u }, { 0u, 0u, 0u, 0x2C1E9F23u }, { 0u, 0u, 0u, 0xB829B5C2u } },
    { { 0u, 0u, 0u, 0x780BF387u }, { 0u, 0u, 0u, 0x37DF8BB3u }, { 0u, 0u, 0u, 0x00D01334u } },
    { { 0u, 0u, 0u, 0xA0D0BD86u }, { 0u, 0u, 0u, 0x45CBFA73u }, { 0u, 0u, 0u, 0xA6160FFEu } },
    { { 0u, 0u, 0u, 0x393C48CBu }, { 0u, 0u, 0u, 0xBBCA060Fu }, { 0u, 0u, 0u, 0x0FF8EC6Du } },
    { { 0u, 0u, 0u, 0x31BEB5CCu }, { 0u, 0u, 0u, 0xEED7F2F0u }, { 0u, 0u, 0u, 0xBB088017u } },
    { { 0u, 0u, 0u, 0x163BC60Du }, { 0u, 0u, 0u, 0xF45A0ECBu }, { 0u, 0u, 0u, 0x1BCD289Bu } },
    { { 0u, 0u, 0u, 0x06CBBFEAu }, { 0u, 0u, 0u, 0x21AD08E1u }, { 0u, 0u, 0u, 0x847F3F73u } },
    { { 0u, 0u, 0u, 0x78D56CEDu }, { 0u, 0u, 0u, 0x94640D6Eu }, { 0u, 0u, 0u, 0xF0D3D37Bu } },
    { { 0u, 0u, 0u, 0xE67008E1u }, { 0u, 0u, 0u, 0x86D1BF27u }, { 0u, 0u, 0u, 0x5B9B241Du } },
    { { 0u, 0u, 0u, 0xEB64749Au }, { 0u, 0u, 0u, 0x47DFDFB9u }, { 0u, 0u, 0u, 0x6632C3EBu } },
    { { 0u, 0u, 0u, 0x061B6472u }, { 0u, 0u, 0u, 0xBBF84C26u }, { 0u, 0u, 0u, 0x144E49C2u } },
    { { 0u, 0u, 0u, 0xD04C324Eu }, { 0u, 0u, 0u, 0xF10DE513u }, { 0u, 0u, 0u, 0xD3F5114Bu } },
    { { 0u, 0u, 0u, 0x8B5D374Du }, { 0u, 0u, 0u, 0x93CB8879u }, { 0u, 0u, 0u, 0xC7D52FFDu } },
    { { 0u, 0u, 0u, 0x72BA0AAEu }, { 0u, 0u, 0u, 0x7277DA7Bu }, { 0u, 0u, 0u, 0xA1B4AF14u } },
    { { 0u, 0u, 0u, 0x88D8E836u }, { 0u, 0u, 0u, 0xAF14865Eu }, { 0u, 0u, 0u, 0x6C37AB68u } },
    { { 0u, 0u, 0u, 0x76FE690Bu }, { 0u, 0u, 0u, 0x57112138u }, { 0u, 0u, 0u, 0x2AF341AFu } },
    { { 0u, 0u, 0u, 0xE94F77BCu }, { 0u, 0u, 0u, 0xF06C83B8u }, { 0u, 0u, 0u, 0xFF5675F0u } },
    { { 0u, 0u, 0u, 0x979074ADu }, { 0u, 0u, 0u, 0x9A787BC5u }, { 0u, 0u, 0u, 0xB9BD4B0Cu } },
    { { 0u, 0u, 0u, 0x5937D3EDu }, { 0u, 0u, 0u, 0xE4C3A793u }, { 0u, 0u, 0u, 0x96215EDAu } }
};
#endif


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

#define load_word(p)                                                          \
    ((word_t){                                                                \
        LLH_LOAD_U32_LE((p) + 8),                                             \
        (uint64_t)LLH_LOAD_U32_LE(p) | ((uint64_t)LLH_LOAD_U32_LE((p) + 4) << 32) \
    })

#define store_word(p, w)                         \
    do {                                         \
        LLH_STORE_U32_LE((p), (uint32_t)((w).lo)); \
        LLH_STORE_U32_LE((p) + 4, (uint32_t)((w).lo >> 32)); \
        LLH_STORE_U32_LE((p) + 8, (w).hi);       \
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

    state[0] = llh_rotl_1_31(state[0], SIGMA[0]);
    state[1] = llh_rotl_1_31(state[1], SIGMA[1]);
    state[2] = llh_rotl_1_31(state[2], SIGMA[2]);
    state[3] = llh_rotl_33_63(state[3], SIGMA[3]);
    state[4] = llh_rotl_1_31(state[4], SIGMA[4]);
    state[5] = llh_rotl_33_63(state[5], SIGMA[5]);
    state[6] = llh_rotl_33_63(state[6], SIGMA[6]);
    state[7] = llh_rotl_64_95(state[7], SIGMA[7]);
    state[8] = llh_rotl_33_63(state[8], SIGMA[8]);
    state[9] = llh_rotl_64_95(state[9], SIGMA[9]);
    state[10] = llh_rotl_1_31(state[10], SIGMA[10]);
    state[11] = llh_rotl_33_63(state[11], SIGMA[11]);
    state[12] = llh_rotl_1_31(state[12], SIGMA[12]);
    state[13] = llh_rotl_32(state[13]);
    state[14] = llh_rotl_64_95(state[14], SIGMA[14]);
    state[15] = llh_rotl_1_31(state[15], SIGMA[15]);
    state[16] = llh_rotl_64_95(state[16], SIGMA[16]);
    state[17] = llh_rotl_1_31(state[17], SIGMA[17]);
    state[18] = llh_rotl_33_63(state[18], SIGMA[18]);
    state[19] = llh_rotl_1_31(state[19], SIGMA[19]);
    state[20] = llh_rotl_33_63(state[20], SIGMA[20]);
    state[21] = llh_rotl_64_95(state[21], SIGMA[21]);
    state[22] = llh_rotl_33_63(state[22], SIGMA[22]);
    state[23] = llh_rotl_1_31(state[23], SIGMA[23]);
    state[23] = WORD_XOR(state[23], rc);
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
    __m128i c0;
    __m128i c1;
    __m128i c2;
} family96x4_t;

#define LLH_AVX512_W96_TERNLOG_XOR3 0x96

typedef struct {
    uint32_t lanes[8];
} u32x8_consts_t;

typedef struct {
    __m256i hi;
    __m256i mid;
    __m256i lo;
} column96x8_t;

typedef struct {
    uint32_t counts[8];
    uint32_t inv_counts[8];
    unsigned char q1_mask;
    unsigned char q2_mask;
} rotl96x8_consts_t;

#define LLH_U32X8_CONSTS(n0, n1, n2, n3, n4, n5, n6, n7) {                           \
    {                                                                                \
        (uint32_t)(n0), (uint32_t)(n1), (uint32_t)(n2), (uint32_t)(n3),             \
        (uint32_t)(n4), (uint32_t)(n5), (uint32_t)(n6), (uint32_t)(n7)              \
    }                                                                                \
}

static const ALIGN32 u32x8_consts_t BUILD_PERM_AVX512_W96_COL[4] = {
    LLH_U32X8_CONSTS(1u, 2u, 3u, 5u, 0u, 4u, 1u, 2u),
    LLH_U32X8_CONSTS(2u, 0u, 4u, 1u, 5u, 3u, 2u, 0u),
    LLH_U32X8_CONSTS(4u, 5u, 1u, 2u, 3u, 0u, 4u, 5u),
    LLH_U32X8_CONSTS(5u, 3u, 0u, 4u, 2u, 1u, 5u, 3u)
};

static const ALIGN32 u32x8_consts_t COMPLEMENT_MASK_AVX512_W96_COL =
    LLH_U32X8_CONSTS(0xFFFFFFFFu, 0xFFFFFFFFu, 0u, 0xFFFFFFFFu, 0xFFFFFFFFu, 0u, 0xFFFFFFFFu, 0xFFFFFFFFu);

static const ALIGN32 u32x8_consts_t CAPACITY_MASK_AVX512_W96_COL =
    LLH_U32X8_CONSTS(0u, 0u, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0u, 0u);

static const ALIGN32 u32x8_consts_t DUP_RATE_AVX512_W96_COL =
    LLH_U32X8_CONSTS(0u, 1u, 2u, 3u, 4u, 5u, 0u, 1u);

static const ALIGN32 u32x8_consts_t ROT1_PERM_AVX512_W96_COL =
    LLH_U32X8_CONSTS(1u, 2u, 3u, 4u, 5u, 6u, 7u, 0u);

static const ALIGN32 u32x8_consts_t ROT2_PERM_AVX512_W96_COL =
    LLH_U32X8_CONSTS(2u, 3u, 4u, 5u, 6u, 7u, 0u, 1u);

static const ALIGN32 u32x8_consts_t ROT3_PERM_AVX512_W96_COL =
    LLH_U32X8_CONSTS(3u, 4u, 5u, 0u, 1u, 2u, 6u, 7u);

static const ALIGN32 rotl96x8_consts_t SHIFTROWS_AVX512_W96_COL[4] = {
    { { 2u, 19u, 20u, 5u, 6u, 23u, 2u, 19u }, { 30u, 13u, 12u, 27u, 26u, 9u, 30u, 13u }, 0x24, 0x10 },
    { { 11u, 2u, 9u, 0u, 7u, 30u, 11u, 2u }, { 21u, 30u, 23u, 32u, 25u, 2u, 21u, 30u }, 0x8A, 0x24 },
    { { 28u, 25u, 6u, 3u, 16u, 13u, 28u, 25u }, { 4u, 7u, 26u, 29u, 16u, 19u, 4u, 7u }, 0xB2, 0x08 },
    { { 21u, 24u, 11u, 14u, 1u, 4u, 21u, 24u }, { 11u, 8u, 21u, 18u, 31u, 28u, 11u, 8u }, 0x45, 0x82 }
};

#undef LLH_U32X8_CONSTS

#define LLH_AVX512_W96_SHIFT0_Q1 ((__mmask8)0x24)
#define LLH_AVX512_W96_SHIFT0_Q2 ((__mmask8)0x10)
#define LLH_AVX512_W96_SHIFT1_Q1 ((__mmask8)0x8A)
#define LLH_AVX512_W96_SHIFT1_Q2 ((__mmask8)0x24)
#define LLH_AVX512_W96_SHIFT2_Q1 ((__mmask8)0xB2)
#define LLH_AVX512_W96_SHIFT2_Q2 ((__mmask8)0x08)
#define LLH_AVX512_W96_SHIFT3_Q1 ((__mmask8)0x45)
#define LLH_AVX512_W96_SHIFT3_Q2 ((__mmask8)0x82)

#define LLH_AVX512_W96_TERNLOG_XOR_AND 0x6C

#define llh_load_u32x8_w96(table) \
    _mm256_load_si256((const __m256i *)(const void *)((table)->lanes))

static LLH_ALWAYS_INLINE __m256i avx2_shldv_epi32_w96(
    __m256i hi,
    __m256i lo,
    __m256i counts,
    __m256i inv_counts)
{
    return _mm256_or_si256(
        _mm256_sllv_epi32(hi, counts),
        _mm256_srlv_epi32(lo, inv_counts));
}

static LLH_ALWAYS_INLINE __m256i avx2_mask_bits8_epi32(unsigned char bits)
{
    return _mm256_setr_epi32(
        (bits & 0x01u) ? -1 : 0,
        (bits & 0x02u) ? -1 : 0,
        (bits & 0x04u) ? -1 : 0,
        (bits & 0x08u) ? -1 : 0,
        (bits & 0x10u) ? -1 : 0,
        (bits & 0x20u) ? -1 : 0,
        (bits & 0x40u) ? -1 : 0,
        (bits & 0x80u) ? -1 : 0);
}

#define avx2_xor3_epi32_w96(a, b, c) \
    _mm256_xor_si256(_mm256_xor_si256((a), (b)), (c))

#define avx2_xorand_epi32_w96(a, b, c) \
    _mm256_xor_si256((b), _mm256_and_si256((a), (c)))

static LLH_ALWAYS_INLINE __m256i avx2_boolean_column_chunk96(
    __m256i column,
    __m256i rot1_perm,
    __m256i rot2_perm,
    __m256i rot3_perm,
    __m256i complement_mask)
{
    const __m256i rot1 = _mm256_permutevar8x32_epi32(column, rot1_perm);
    const __m256i rot2 = _mm256_permutevar8x32_epi32(column, rot2_perm);
    const __m256i rot3 = _mm256_permutevar8x32_epi32(column, rot3_perm);
    const __m256i mixed = avx2_xorand_epi32_w96(rot1, rot2, rot3);

    return avx2_xor3_epi32_w96(column, mixed, complement_mask);
}

static LLH_ALWAYS_INLINE void avx2_round_columns_chunk96(
    __m256i s0,
    __m256i s1,
    __m256i s2,
    __m256i s3,
    __m256i build0,
    __m256i build1,
    __m256i build2,
    __m256i build3,
    __m256i rot1_perm,
    __m256i rot2_perm,
    __m256i rot3_perm,
    __m256i complement_mask,
    __m256i *out0,
    __m256i *out1,
    __m256i *out2,
    __m256i *out3)
{
    const __m256i b0 = avx2_boolean_column_chunk96(s0, rot1_perm, rot2_perm, rot3_perm, complement_mask);
    const __m256i b1 = avx2_boolean_column_chunk96(s1, rot1_perm, rot2_perm, rot3_perm, complement_mask);
    const __m256i b2 = avx2_boolean_column_chunk96(s2, rot1_perm, rot2_perm, rot3_perm, complement_mask);
    const __m256i b3 = avx2_boolean_column_chunk96(s3, rot1_perm, rot2_perm, rot3_perm, complement_mask);
    const __m256i f0 = _mm256_permutevar8x32_epi32(b0, build0);
    const __m256i f1 = _mm256_permutevar8x32_epi32(b1, build1);
    const __m256i f2 = _mm256_permutevar8x32_epi32(b2, build2);
    const __m256i f3 = _mm256_permutevar8x32_epi32(b3, build3);

    *out0 = avx2_xor3_epi32_w96(f1, f2, f3);
    *out1 = avx2_xor3_epi32_w96(f0, f2, f3);
    *out2 = avx2_xor3_epi32_w96(f0, f1, f3);
    *out3 = avx2_xor3_epi32_w96(f0, f1, f2);
}

static LLH_ALWAYS_INLINE column96x8_t avx2_xor_column96(column96x8_t a, column96x8_t b)
{
    column96x8_t out;

    out.hi = _mm256_xor_si256(a.hi, b.hi);
    out.mid = _mm256_xor_si256(a.mid, b.mid);
    out.lo = _mm256_xor_si256(a.lo, b.lo);
    return out;
}

static LLH_ALWAYS_INLINE void avx2_shiftrows_column96(
    column96x8_t *v,
    __m256i counts,
    __m256i inv_counts,
    unsigned char q1_mask_bits,
    unsigned char q2_mask_bits)
{
    const __m256i hi_mid = avx2_shldv_epi32_w96(v->hi, v->mid, counts, inv_counts);
    const __m256i mid_lo = avx2_shldv_epi32_w96(v->mid, v->lo, counts, inv_counts);
    const __m256i lo_hi = avx2_shldv_epi32_w96(v->lo, v->hi, counts, inv_counts);
    const __m256i q1_mask = avx2_mask_bits8_epi32(q1_mask_bits);
    const __m256i q2_mask = avx2_mask_bits8_epi32(q2_mask_bits);
    __m256i out_hi = hi_mid;
    __m256i out_mid = mid_lo;
    __m256i out_lo = lo_hi;

    out_hi = _mm256_blendv_epi8(out_hi, mid_lo, q1_mask);
    out_mid = _mm256_blendv_epi8(out_mid, lo_hi, q1_mask);
    out_lo = _mm256_blendv_epi8(out_lo, hi_mid, q1_mask);

    out_hi = _mm256_blendv_epi8(out_hi, lo_hi, q2_mask);
    out_mid = _mm256_blendv_epi8(out_mid, hi_mid, q2_mask);
    out_lo = _mm256_blendv_epi8(out_lo, mid_lo, q2_mask);

    v->hi = out_hi;
    v->mid = out_mid;
    v->lo = out_lo;
}

static LLH_ALWAYS_INLINE family96x4_t avx2_load_block_family96(const uint8_t *src)
{
    const __m128i a =
        _mm_loadu_si128((const __m128i *)(const void *)src);
    const __m128i b =
        _mm_loadu_si128((const __m128i *)(const void *)(src + 16));
    const __m128i c =
        _mm_loadu_si128((const __m128i *)(const void *)(src + 32));
    family96x4_t out;
    const __m128i llh_bc2 = _mm_alignr_epi8(c, b, 8);
    const __m128i llh_ba1 = _mm_alignr_epi8(b, a, 4);
    const __m128i llh_ab2 = _mm_alignr_epi8(b, a, 8);
    const __m128i llh_bc3 = _mm_alignr_epi8(c, b, 12);

    out.c0 = _mm_castps_si128(
        _mm_shuffle_ps(_mm_castsi128_ps(llh_ab2), _mm_castsi128_ps(c), _MM_SHUFFLE(3, 0, 3, 0)));
    out.c1 = _mm_castps_si128(
        _mm_shuffle_ps(_mm_castsi128_ps(llh_ba1), _mm_castsi128_ps(llh_bc3), _MM_SHUFFLE(3, 0, 3, 0)));
    out.c2 = _mm_castps_si128(
        _mm_shuffle_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(llh_bc2), _MM_SHUFFLE(3, 0, 3, 0)));
    return out;
}

static LLH_ALWAYS_INLINE void avx2_load_state_columns_w96(
    const word_t *state,
    column96x8_t *c0,
    column96x8_t *c1,
    column96x8_t *c2,
    column96x8_t *c3)
{
#define LLH_AVX2_W96_LOAD_COLUMN(dst, col)                                                  \
    do {                                                                                     \
        (dst)->hi = _mm256_setr_epi32(                                                       \
            (int)state[(col) + 0].hi,                                                        \
            (int)state[(col) + 4].hi,                                                        \
            (int)state[(col) + 8].hi,                                                        \
            (int)state[(col) + 12].hi,                                                       \
            (int)state[(col) + 16].hi,                                                       \
            (int)state[(col) + 20].hi,                                                       \
            (int)state[(col) + 0].hi,                                                        \
            (int)state[(col) + 4].hi);                                                       \
        (dst)->mid = _mm256_setr_epi32(                                                      \
            (int)(uint32_t)(state[(col) + 0].lo >> 32),                                      \
            (int)(uint32_t)(state[(col) + 4].lo >> 32),                                      \
            (int)(uint32_t)(state[(col) + 8].lo >> 32),                                      \
            (int)(uint32_t)(state[(col) + 12].lo >> 32),                                     \
            (int)(uint32_t)(state[(col) + 16].lo >> 32),                                     \
            (int)(uint32_t)(state[(col) + 20].lo >> 32),                                     \
            (int)(uint32_t)(state[(col) + 0].lo >> 32),                                      \
            (int)(uint32_t)(state[(col) + 4].lo >> 32));                                     \
        (dst)->lo = _mm256_setr_epi32(                                                       \
            (int)(uint32_t)state[(col) + 0].lo,                                              \
            (int)(uint32_t)state[(col) + 4].lo,                                              \
            (int)(uint32_t)state[(col) + 8].lo,                                              \
            (int)(uint32_t)state[(col) + 12].lo,                                             \
            (int)(uint32_t)state[(col) + 16].lo,                                             \
            (int)(uint32_t)state[(col) + 20].lo,                                             \
            (int)(uint32_t)state[(col) + 0].lo,                                              \
            (int)(uint32_t)state[(col) + 4].lo);                                             \
    } while (0)

    LLH_AVX2_W96_LOAD_COLUMN(c0, 0);
    LLH_AVX2_W96_LOAD_COLUMN(c1, 1);
    LLH_AVX2_W96_LOAD_COLUMN(c2, 2);
    LLH_AVX2_W96_LOAD_COLUMN(c3, 3);

#undef LLH_AVX2_W96_LOAD_COLUMN
}

static LLH_ALWAYS_INLINE void avx2_store_state_column96(word_t *state, size_t col, column96x8_t c)
{
    ALIGN32 uint32_t hi[8];
    ALIGN32 uint32_t mid[8];
    ALIGN32 uint32_t lo[8];

    _mm256_store_si256((__m256i *)(void *)hi, c.hi);
    _mm256_store_si256((__m256i *)(void *)mid, c.mid);
    _mm256_store_si256((__m256i *)(void *)lo, c.lo);

    state[col + 0] = (word_t){hi[0], ((uint64_t)mid[0] << 32) | (uint64_t)lo[0]};
    state[col + 4] = (word_t){hi[1], ((uint64_t)mid[1] << 32) | (uint64_t)lo[1]};
    state[col + 8] = (word_t){hi[2], ((uint64_t)mid[2] << 32) | (uint64_t)lo[2]};
    state[col + 12] = (word_t){hi[3], ((uint64_t)mid[3] << 32) | (uint64_t)lo[3]};
    state[col + 16] = (word_t){hi[4], ((uint64_t)mid[4] << 32) | (uint64_t)lo[4]};
    state[col + 20] = (word_t){hi[5], ((uint64_t)mid[5] << 32) | (uint64_t)lo[5]};
}

static LLH_ALWAYS_INLINE void avx2_store_state_columns_w96(
    word_t *state,
    column96x8_t c0,
    column96x8_t c1,
    column96x8_t c2,
    column96x8_t c3)
{
    avx2_store_state_column96(state, 0, c0);
    avx2_store_state_column96(state, 1, c1);
    avx2_store_state_column96(state, 2, c2);
    avx2_store_state_column96(state, 3, c3);
}

static LLH_ALWAYS_INLINE column96x8_t avx2_load_rc_column96(size_t round_idx)
{
    column96x8_t out;

    out.hi = _mm256_setr_epi32(0, 0, 0, 0, 0, (int)RC[round_idx].hi, 0, 0);
    out.mid = _mm256_setr_epi32(0, 0, 0, 0, 0, (int)(uint32_t)(RC[round_idx].lo >> 32), 0, 0);
    out.lo = _mm256_setr_epi32(0, 0, 0, 0, 0, (int)(uint32_t)RC[round_idx].lo, 0, 0);
    return out;
}

static LLH_ALWAYS_INLINE void avx2_xor_rate_columns_from_families_w96(
    column96x8_t *c0,
    column96x8_t *c1,
    column96x8_t *c2,
    column96x8_t *c3,
    family96x4_t block0,
    family96x4_t block1)
{
    const __m128i zero = _mm_setzero_si128();
    const __m256i dup_rate_perm = llh_load_u32x8_w96(&DUP_RATE_AVX512_W96_COL);

#define LLH_AVX2_W96_XOR_CHUNK_FROM_FAMILIES(dst0, dst1, dst2, dst3, chunk0, chunk1)           \
    do {                                                                                         \
        const __m128i llh_pair01 = _mm_unpacklo_epi32((chunk0), (chunk1));                      \
        const __m128i llh_pair23 = _mm_unpackhi_epi32((chunk0), (chunk1));                      \
                                                                                                 \
        (dst0) = _mm256_xor_si256(                                                               \
            (dst0),                                                                              \
            _mm256_permutevar8x32_epi32(                                                         \
                _mm256_zextsi128_si256(_mm_unpacklo_epi64(llh_pair01, zero)),                    \
                dup_rate_perm));                                                                  \
        (dst1) = _mm256_xor_si256(                                                               \
            (dst1),                                                                              \
            _mm256_permutevar8x32_epi32(                                                         \
                _mm256_zextsi128_si256(_mm_unpackhi_epi64(llh_pair01, zero)),                    \
                dup_rate_perm));                                                                  \
        (dst2) = _mm256_xor_si256(                                                               \
            (dst2),                                                                              \
            _mm256_permutevar8x32_epi32(                                                         \
                _mm256_zextsi128_si256(_mm_unpacklo_epi64(llh_pair23, zero)),                    \
                dup_rate_perm));                                                                  \
        (dst3) = _mm256_xor_si256(                                                               \
            (dst3),                                                                              \
            _mm256_permutevar8x32_epi32(                                                         \
                _mm256_zextsi128_si256(_mm_unpackhi_epi64(llh_pair23, zero)),                    \
                dup_rate_perm));                                                                  \
    } while (0)

    LLH_AVX2_W96_XOR_CHUNK_FROM_FAMILIES(c0->hi, c1->hi, c2->hi, c3->hi, block0.c0, block1.c0);
    LLH_AVX2_W96_XOR_CHUNK_FROM_FAMILIES(c0->mid, c1->mid, c2->mid, c3->mid, block0.c1, block1.c1);
    LLH_AVX2_W96_XOR_CHUNK_FROM_FAMILIES(c0->lo, c1->lo, c2->lo, c3->lo, block0.c2, block1.c2);

#undef LLH_AVX2_W96_XOR_CHUNK_FROM_FAMILIES
}

static LLH_ALWAYS_INLINE void avx2_permute_columns_i2_w96(
    column96x8_t *c0,
    column96x8_t *c1,
    column96x8_t *c2,
    column96x8_t *c3)
{
    column96x8_t llh_c0 = *c0;
    column96x8_t llh_c1 = *c1;
    column96x8_t llh_c2 = *c2;
    column96x8_t llh_c3 = *c3;
    const __m256i build0 = llh_load_u32x8_w96(&BUILD_PERM_AVX512_W96_COL[0]);
    const __m256i build1 = llh_load_u32x8_w96(&BUILD_PERM_AVX512_W96_COL[1]);
    const __m256i build2 = llh_load_u32x8_w96(&BUILD_PERM_AVX512_W96_COL[2]);
    const __m256i build3 = llh_load_u32x8_w96(&BUILD_PERM_AVX512_W96_COL[3]);
    const __m256i rot1_perm = llh_load_u32x8_w96(&ROT1_PERM_AVX512_W96_COL);
    const __m256i rot2_perm = llh_load_u32x8_w96(&ROT2_PERM_AVX512_W96_COL);
    const __m256i rot3_perm = llh_load_u32x8_w96(&ROT3_PERM_AVX512_W96_COL);
    const __m256i shift0 =
        _mm256_loadu_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[0].counts);
    const __m256i shift0_inv =
        _mm256_loadu_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[0].inv_counts);
    const __m256i shift1 =
        _mm256_loadu_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[1].counts);
    const __m256i shift1_inv =
        _mm256_loadu_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[1].inv_counts);
    const __m256i shift2 =
        _mm256_loadu_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[2].counts);
    const __m256i shift2_inv =
        _mm256_loadu_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[2].inv_counts);
    const __m256i shift3 =
        _mm256_loadu_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[3].counts);
    const __m256i shift3_inv =
        _mm256_loadu_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[3].inv_counts);
    const __m256i complement_mask = llh_load_u32x8_w96(&COMPLEMENT_MASK_AVX512_W96_COL);

    LLH_PRAGMA_NO_UNROLL
    for (size_t round_idx = 0; round_idx < NR_DEFAULT; round_idx++) {
        const column96x8_t llh_rc_vec = avx2_load_rc_column96(round_idx);
        column96x8_t llh_out0;
        column96x8_t llh_out1;
        column96x8_t llh_out2;
        column96x8_t llh_out3;

        avx2_round_columns_chunk96(
            llh_c0.hi, llh_c1.hi, llh_c2.hi, llh_c3.hi,
            build0, build1, build2, build3,
            rot1_perm, rot2_perm, rot3_perm, complement_mask,
            &llh_out0.hi, &llh_out1.hi, &llh_out2.hi, &llh_out3.hi);
        avx2_round_columns_chunk96(
            llh_c0.mid, llh_c1.mid, llh_c2.mid, llh_c3.mid,
            build0, build1, build2, build3,
            rot1_perm, rot2_perm, rot3_perm, complement_mask,
            &llh_out0.mid, &llh_out1.mid, &llh_out2.mid, &llh_out3.mid);
        avx2_round_columns_chunk96(
            llh_c0.lo, llh_c1.lo, llh_c2.lo, llh_c3.lo,
            build0, build1, build2, build3,
            rot1_perm, rot2_perm, rot3_perm, complement_mask,
            &llh_out0.lo, &llh_out1.lo, &llh_out2.lo, &llh_out3.lo);

        avx2_shiftrows_column96(&llh_out0, shift0, shift0_inv, 0x24, 0x10);
        avx2_shiftrows_column96(&llh_out1, shift1, shift1_inv, 0x8A, 0x24);
        avx2_shiftrows_column96(&llh_out2, shift2, shift2_inv, 0xB2, 0x08);
        avx2_shiftrows_column96(&llh_out3, shift3, shift3_inv, 0x45, 0x82);

        llh_out3 = avx2_xor_column96(llh_out3, llh_rc_vec);

        llh_c0 = llh_out0;
        llh_c1 = llh_out1;
        llh_c2 = llh_out2;
        llh_c3 = llh_out3;
    }

    *c0 = llh_c0;
    *c1 = llh_c1;
    *c2 = llh_c2;
    *c3 = llh_c3;
}

static void avx2_process_full_blocks_i2(HashCtx *ctx, const uint8_t *data, size_t full_blocks)
{
    column96x8_t c0;
    column96x8_t c1;
    column96x8_t c2;
    column96x8_t c3;
    const __m256i capacity_mask = llh_load_u32x8_w96(&CAPACITY_MASK_AVX512_W96_COL);

    avx2_load_state_columns_w96(ctx->state.row, &c0, &c1, &c2, &c3);

    for (; full_blocks != 0; full_blocks--, data += RATE_BYTES) {
        const __m256i prev0_hi = _mm256_and_si256(c0.hi, capacity_mask);
        const __m256i prev0_mid = _mm256_and_si256(c0.mid, capacity_mask);
        const __m256i prev0_lo = _mm256_and_si256(c0.lo, capacity_mask);
        const __m256i prev1_hi = _mm256_and_si256(c1.hi, capacity_mask);
        const __m256i prev1_mid = _mm256_and_si256(c1.mid, capacity_mask);
        const __m256i prev1_lo = _mm256_and_si256(c1.lo, capacity_mask);
        const __m256i prev2_hi = _mm256_and_si256(c2.hi, capacity_mask);
        const __m256i prev2_mid = _mm256_and_si256(c2.mid, capacity_mask);
        const __m256i prev2_lo = _mm256_and_si256(c2.lo, capacity_mask);
        const __m256i prev3_hi = _mm256_and_si256(c3.hi, capacity_mask);
        const __m256i prev3_mid = _mm256_and_si256(c3.mid, capacity_mask);
        const __m256i prev3_lo = _mm256_and_si256(c3.lo, capacity_mask);
        const family96x4_t block0 = avx2_load_block_family96(data);
        const family96x4_t block1 = avx2_load_block_family96(data + 48);

        avx2_permute_columns_i2_w96(&c0, &c1, &c2, &c3);

        c0.hi = _mm256_xor_si256(c0.hi, prev0_hi);
        c0.mid = _mm256_xor_si256(c0.mid, prev0_mid);
        c0.lo = _mm256_xor_si256(c0.lo, prev0_lo);
        c1.hi = _mm256_xor_si256(c1.hi, prev1_hi);
        c1.mid = _mm256_xor_si256(c1.mid, prev1_mid);
        c1.lo = _mm256_xor_si256(c1.lo, prev1_lo);
        c2.hi = _mm256_xor_si256(c2.hi, prev2_hi);
        c2.mid = _mm256_xor_si256(c2.mid, prev2_mid);
        c2.lo = _mm256_xor_si256(c2.lo, prev2_lo);
        c3.hi = _mm256_xor_si256(c3.hi, prev3_hi);
        c3.mid = _mm256_xor_si256(c3.mid, prev3_mid);
        c3.lo = _mm256_xor_si256(c3.lo, prev3_lo);

        avx2_xor_rate_columns_from_families_w96(&c0, &c1, &c2, &c3, block0, block1);
    }

    avx2_store_state_columns_w96(ctx->state.row, c0, c1, c2, c3);
}

static LLH_ALWAYS_INLINE void avx2_init_state_columns_i2_w96(
    column96x8_t *c0,
    column96x8_t *c1,
    column96x8_t *c2,
    column96x8_t *c3,
    uint64_t msg_len_bits)
{
    const __m256i zero = _mm256_setzero_si256();

    c0->hi = zero;
    c0->mid = zero;
    c0->lo = zero;
    c1->hi = zero;
    c1->mid = zero;
    c1->lo = zero;
    c2->hi = zero;
    c2->mid = zero;
    c2->lo = zero;
    c3->hi = zero;
    c3->mid = _mm256_setr_epi32(0, 0, 0, 0, 0, (int)(uint32_t)(msg_len_bits >> 32), 0, 0);
    c3->lo = _mm256_setr_epi32(0, 0, 0, 0, 0, (int)(uint32_t)msg_len_bits, 0, 0);
}

static LLH_ALWAYS_INLINE void avx2_load_padded_rate_block_w96(
    const uint8_t *src,
    size_t valid_bytes,
    family96x4_t *block0,
    family96x4_t *block1)
{
    ALIGN32 uint8_t padded[RATE_BYTES];

    memset(padded, 0xFF, sizeof(padded));
    if (valid_bytes != 0) {
        memcpy(padded, src, valid_bytes);
    }

    *block0 = avx2_load_block_family96(padded);
    *block1 = avx2_load_block_family96(padded + 48);
}

static LLH_ALWAYS_INLINE void avx2_process_prepared_block_w96(
    column96x8_t *c0,
    column96x8_t *c1,
    column96x8_t *c2,
    column96x8_t *c3,
    family96x4_t block0,
    family96x4_t block1,
    __m256i capacity_mask)
{
    const __m256i prev0_hi = _mm256_and_si256(c0->hi, capacity_mask);
    const __m256i prev0_mid = _mm256_and_si256(c0->mid, capacity_mask);
    const __m256i prev0_lo = _mm256_and_si256(c0->lo, capacity_mask);
    const __m256i prev1_hi = _mm256_and_si256(c1->hi, capacity_mask);
    const __m256i prev1_mid = _mm256_and_si256(c1->mid, capacity_mask);
    const __m256i prev1_lo = _mm256_and_si256(c1->lo, capacity_mask);
    const __m256i prev2_hi = _mm256_and_si256(c2->hi, capacity_mask);
    const __m256i prev2_mid = _mm256_and_si256(c2->mid, capacity_mask);
    const __m256i prev2_lo = _mm256_and_si256(c2->lo, capacity_mask);
    const __m256i prev3_hi = _mm256_and_si256(c3->hi, capacity_mask);
    const __m256i prev3_mid = _mm256_and_si256(c3->mid, capacity_mask);
    const __m256i prev3_lo = _mm256_and_si256(c3->lo, capacity_mask);

    avx2_permute_columns_i2_w96(c0, c1, c2, c3);

    c0->hi = _mm256_xor_si256(c0->hi, prev0_hi);
    c0->mid = _mm256_xor_si256(c0->mid, prev0_mid);
    c0->lo = _mm256_xor_si256(c0->lo, prev0_lo);
    c1->hi = _mm256_xor_si256(c1->hi, prev1_hi);
    c1->mid = _mm256_xor_si256(c1->mid, prev1_mid);
    c1->lo = _mm256_xor_si256(c1->lo, prev1_lo);
    c2->hi = _mm256_xor_si256(c2->hi, prev2_hi);
    c2->mid = _mm256_xor_si256(c2->mid, prev2_mid);
    c2->lo = _mm256_xor_si256(c2->lo, prev2_lo);
    c3->hi = _mm256_xor_si256(c3->hi, prev3_hi);
    c3->mid = _mm256_xor_si256(c3->mid, prev3_mid);
    c3->lo = _mm256_xor_si256(c3->lo, prev3_lo);

    avx2_xor_rate_columns_from_families_w96(c0, c1, c2, c3, block0, block1);
}

static LLH_ALWAYS_INLINE void avx2_squeeze_rate_columns_w96(
    column96x8_t c0,
    column96x8_t c1,
    column96x8_t c2,
    column96x8_t c3,
    uint8_t *out)
{
    State tmp = { 0 };

    avx2_store_state_columns_w96(tmp.row, c0, c1, c2, c3);
    for (size_t i = 0; i < RATE_ROWS; i++) {
        store_word(out + i * 12u, tmp.row[i]);
    }
}

static void avx2_hash_i2_fastpath(
    const uint8_t *msg,
    size_t msg_len,
    uint64_t msg_len_bits,
    uint8_t *out)
{
    size_t full_blocks = msg_len / RATE_BYTES;
    const size_t tail_len = msg_len - full_blocks * RATE_BYTES;
    column96x8_t c0, c1, c2, c3;
    const __m256i capacity_mask = llh_load_u32x8_w96(&CAPACITY_MASK_AVX512_W96_COL);

    avx2_init_state_columns_i2_w96(&c0, &c1, &c2, &c3, msg_len_bits);

    for (; full_blocks != 0; full_blocks--, msg += RATE_BYTES) {
        const family96x4_t block0 = avx2_load_block_family96(msg);
        const family96x4_t block1 = avx2_load_block_family96(msg + 48);
        avx2_process_prepared_block_w96(&c0, &c1, &c2, &c3, block0, block1, capacity_mask);
    }

    if (tail_len != 0) {
        family96x4_t block0;
        family96x4_t block1;

        avx2_load_padded_rate_block_w96(msg, tail_len, &block0, &block1);
        avx2_process_prepared_block_w96(&c0, &c1, &c2, &c3, block0, block1, capacity_mask);
    }

    avx2_permute_columns_i2_w96(&c0, &c1, &c2, &c3);
    avx2_squeeze_rate_columns_w96(c0, c1, c2, c3, out);
}

static void permutation_i2_avx2(State *s)
{
    column96x8_t c0;
    column96x8_t c1;
    column96x8_t c2;
    column96x8_t c3;

    avx2_load_state_columns_w96(s->row, &c0, &c1, &c2, &c3);
    avx2_permute_columns_i2_w96(&c0, &c1, &c2, &c3);
    avx2_store_state_columns_w96(s->row, c0, c1, c2, c3);
}

#define LLH_AVX512_W96_PACK_FAMILY_FROM_BLOCKS(dst, a, b, c)                                  \
    do {                                                                                       \
        const __m128i llh_bc2 = _mm_alignr_epi8((c), (b), 8);                                 \
        const __m128i llh_ba1 = _mm_alignr_epi8((b), (a), 4);                                 \
        const __m128i llh_ab2 = _mm_alignr_epi8((b), (a), 8);                                 \
        const __m128i llh_bc3 = _mm_alignr_epi8((c), (b), 12);                                \
                                                                                               \
        (dst).c0 = _mm_castps_si128(                                                           \
            _mm_shuffle_ps(_mm_castsi128_ps(llh_ab2), _mm_castsi128_ps((c)), _MM_SHUFFLE(3, 0, 3, 0))); \
        (dst).c1 = _mm_castps_si128(                                                           \
            _mm_shuffle_ps(_mm_castsi128_ps(llh_ba1), _mm_castsi128_ps(llh_bc3), _MM_SHUFFLE(3, 0, 3, 0))); \
        (dst).c2 = _mm_castps_si128(                                                           \
            _mm_shuffle_ps(_mm_castsi128_ps((a)), _mm_castsi128_ps(llh_bc2), _MM_SHUFFLE(3, 0, 3, 0))); \
    } while (0)

LLH_TARGET_AVX512
#define avx512_load_u32x8_w96(table) \
    _mm256_load_si256((const __m256i *)(const void *)((table)->lanes))

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE __m256i avx512_shldv_epi32_w96(
    __m256i hi,
    __m256i lo,
    __m256i counts,
    __m256i inv_counts)
{
    return _mm256_or_si256(
        _mm256_sllv_epi32(hi, counts),
        _mm256_srlv_epi32(lo, inv_counts));
}

LLH_TARGET_AVX512
#define avx512_xor3_epi32_w96(a, b, c) \
    _mm256_ternarylogic_epi32((a), (b), (c), LLH_AVX512_W96_TERNLOG_XOR3)

LLH_TARGET_AVX512
#define avx512_xorand_epi32_w96(a, b, c) \
    _mm256_ternarylogic_epi32((a), (b), (c), LLH_AVX512_W96_TERNLOG_XOR_AND)

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE __m256i avx512_boolean_column_chunk96(
    __m256i column,
    __m256i rot1_perm,
    __m256i rot2_perm,
    __m256i rot3_perm,
    __m256i complement_mask)
{
    const __m256i rot1 = _mm256_permutevar8x32_epi32(column, rot1_perm);
    const __m256i rot2 = _mm256_permutevar8x32_epi32(column, rot2_perm);
    const __m256i rot3 = _mm256_permutevar8x32_epi32(column, rot3_perm);
    const __m256i mixed = avx512_xorand_epi32_w96(rot1, rot2, rot3);

    return avx512_xor3_epi32_w96(column, mixed, complement_mask);
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE void avx512_round_columns_chunk96(
    __m256i s0,
    __m256i s1,
    __m256i s2,
    __m256i s3,
    __m256i build0,
    __m256i build1,
    __m256i build2,
    __m256i build3,
    __m256i rot1_perm,
    __m256i rot2_perm,
    __m256i rot3_perm,
    __m256i complement_mask,
    __m256i *out0,
    __m256i *out1,
    __m256i *out2,
    __m256i *out3)
{
    const __m256i b0 = avx512_boolean_column_chunk96(s0, rot1_perm, rot2_perm, rot3_perm, complement_mask);
    const __m256i b1 = avx512_boolean_column_chunk96(s1, rot1_perm, rot2_perm, rot3_perm, complement_mask);
    const __m256i b2 = avx512_boolean_column_chunk96(s2, rot1_perm, rot2_perm, rot3_perm, complement_mask);
    const __m256i b3 = avx512_boolean_column_chunk96(s3, rot1_perm, rot2_perm, rot3_perm, complement_mask);
    const __m256i f0 = _mm256_permutevar8x32_epi32(b0, build0);
    const __m256i f1 = _mm256_permutevar8x32_epi32(b1, build1);
    const __m256i f2 = _mm256_permutevar8x32_epi32(b2, build2);
    const __m256i f3 = _mm256_permutevar8x32_epi32(b3, build3);

    *out0 = avx512_xor3_epi32_w96(f1, f2, f3);
    *out1 = avx512_xor3_epi32_w96(f0, f2, f3);
    *out2 = avx512_xor3_epi32_w96(f0, f1, f3);
    *out3 = avx512_xor3_epi32_w96(f0, f1, f2);
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE column96x8_t avx512_xor_column96(column96x8_t a, column96x8_t b)
{
    column96x8_t out;

    out.hi = _mm256_xor_si256(a.hi, b.hi);
    out.mid = _mm256_xor_si256(a.mid, b.mid);
    out.lo = _mm256_xor_si256(a.lo, b.lo);
    return out;
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE void avx512_shiftrows_column96(
    column96x8_t *v,
    __m256i counts,
    __m256i inv_counts,
    __mmask8 q1_mask,
    __mmask8 q2_mask)
{
    const __m256i hi_mid = avx512_shldv_epi32_w96(v->hi, v->mid, counts, inv_counts);
    const __m256i mid_lo = avx512_shldv_epi32_w96(v->mid, v->lo, counts, inv_counts);
    const __m256i lo_hi = avx512_shldv_epi32_w96(v->lo, v->hi, counts, inv_counts);
    __m256i out_hi = hi_mid;
    __m256i out_mid = mid_lo;
    __m256i out_lo = lo_hi;

    out_hi = _mm256_mask_blend_epi32(q1_mask, out_hi, mid_lo);
    out_mid = _mm256_mask_blend_epi32(q1_mask, out_mid, lo_hi);
    out_lo = _mm256_mask_blend_epi32(q1_mask, out_lo, hi_mid);

    out_hi = _mm256_mask_blend_epi32(q2_mask, out_hi, lo_hi);
    out_mid = _mm256_mask_blend_epi32(q2_mask, out_mid, hi_mid);
    out_lo = _mm256_mask_blend_epi32(q2_mask, out_lo, mid_lo);

    v->hi = out_hi;
    v->mid = out_mid;
    v->lo = out_lo;
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE family96x4_t avx512_load_block_family96(const uint8_t *src)
{
    const __m128i a =
        _mm_loadu_si128((const __m128i *)(const void *)src);
    const __m128i b =
        _mm_loadu_si128((const __m128i *)(const void *)(src + 16));
    const __m128i c =
        _mm_loadu_si128((const __m128i *)(const void *)(src + 32));
    family96x4_t out;

    LLH_AVX512_W96_PACK_FAMILY_FROM_BLOCKS(out, a, b, c);
    return out;
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE void avx512_squeeze_rate_families(
    family96x4_t g0,
    family96x4_t g1,
    uint8_t *out)
{
    uint32_t g0_c0[4];
    uint32_t g0_c1[4];
    uint32_t g0_c2[4];
    uint32_t g1_c0[4];
    uint32_t g1_c1[4];
    uint32_t g1_c2[4];

    _mm_storeu_si128((__m128i *)(void *)g0_c0, g0.c0);
    _mm_storeu_si128((__m128i *)(void *)g0_c1, g0.c1);
    _mm_storeu_si128((__m128i *)(void *)g0_c2, g0.c2);
    _mm_storeu_si128((__m128i *)(void *)g1_c0, g1.c0);
    _mm_storeu_si128((__m128i *)(void *)g1_c1, g1.c1);
    _mm_storeu_si128((__m128i *)(void *)g1_c2, g1.c2);

    for (size_t lane = 0; lane < 4; lane++) {
        LLH_STORE_U32_LE(out + lane * 12 + 0, g0_c2[lane]);
        LLH_STORE_U32_LE(out + lane * 12 + 4, g0_c1[lane]);
        LLH_STORE_U32_LE(out + lane * 12 + 8, g0_c0[lane]);
        LLH_STORE_U32_LE(out + 48 + lane * 12 + 0, g1_c2[lane]);
        LLH_STORE_U32_LE(out + 48 + lane * 12 + 4, g1_c1[lane]);
        LLH_STORE_U32_LE(out + 48 + lane * 12 + 8, g1_c0[lane]);
    }
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE void avx512_load_state_columns_w96(
    const word_t *state,
    column96x8_t *c0,
    column96x8_t *c1,
    column96x8_t *c2,
    column96x8_t *c3)
{
#define LLH_AVX512_W96_LOAD_COLUMN(dst, col)                                                \
    do {                                                                                     \
        (dst)->hi = _mm256_setr_epi32(                                                       \
            (int)state[(col) + 0].hi,                                                        \
            (int)state[(col) + 4].hi,                                                        \
            (int)state[(col) + 8].hi,                                                        \
            (int)state[(col) + 12].hi,                                                       \
            (int)state[(col) + 16].hi,                                                       \
            (int)state[(col) + 20].hi,                                                       \
            (int)state[(col) + 0].hi,                                                        \
            (int)state[(col) + 4].hi);                                                       \
        (dst)->mid = _mm256_setr_epi32(                                                      \
            (int)(uint32_t)(state[(col) + 0].lo >> 32),                                      \
            (int)(uint32_t)(state[(col) + 4].lo >> 32),                                      \
            (int)(uint32_t)(state[(col) + 8].lo >> 32),                                      \
            (int)(uint32_t)(state[(col) + 12].lo >> 32),                                     \
            (int)(uint32_t)(state[(col) + 16].lo >> 32),                                     \
            (int)(uint32_t)(state[(col) + 20].lo >> 32),                                     \
            (int)(uint32_t)(state[(col) + 0].lo >> 32),                                      \
            (int)(uint32_t)(state[(col) + 4].lo >> 32));                                     \
        (dst)->lo = _mm256_setr_epi32(                                                       \
            (int)(uint32_t)state[(col) + 0].lo,                                              \
            (int)(uint32_t)state[(col) + 4].lo,                                              \
            (int)(uint32_t)state[(col) + 8].lo,                                              \
            (int)(uint32_t)state[(col) + 12].lo,                                             \
            (int)(uint32_t)state[(col) + 16].lo,                                             \
            (int)(uint32_t)state[(col) + 20].lo,                                             \
            (int)(uint32_t)state[(col) + 0].lo,                                              \
            (int)(uint32_t)state[(col) + 4].lo);                                             \
    } while (0)

    LLH_AVX512_W96_LOAD_COLUMN(c0, 0);
    LLH_AVX512_W96_LOAD_COLUMN(c1, 1);
    LLH_AVX512_W96_LOAD_COLUMN(c2, 2);
    LLH_AVX512_W96_LOAD_COLUMN(c3, 3);

#undef LLH_AVX512_W96_LOAD_COLUMN
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE void avx512_store_state_column96(word_t *state, size_t col, column96x8_t c)
{
    ALIGN32 uint32_t hi[8];
    ALIGN32 uint32_t mid[8];
    ALIGN32 uint32_t lo[8];

    /*
     * Force an actual register-to-memory writeback here. Without this barrier,
     * GCC may re-synthesize lane extraction with instructions outside the
     * intended AVX512VL/DQ/BW subset unless we force a concrete writeback here.
     */
    __asm__ __volatile__("vmovdqa %1, %0" : "=m"(hi) : "x"(c.hi));
    __asm__ __volatile__("vmovdqa %1, %0" : "=m"(mid) : "x"(c.mid));
    __asm__ __volatile__("vmovdqa %1, %0" : "=m"(lo) : "x"(c.lo));

    state[col + 0] = (word_t){hi[0], ((uint64_t)mid[0] << 32) | (uint64_t)lo[0]};
    state[col + 4] = (word_t){hi[1], ((uint64_t)mid[1] << 32) | (uint64_t)lo[1]};
    state[col + 8] = (word_t){hi[2], ((uint64_t)mid[2] << 32) | (uint64_t)lo[2]};
    state[col + 12] = (word_t){hi[3], ((uint64_t)mid[3] << 32) | (uint64_t)lo[3]};
    state[col + 16] = (word_t){hi[4], ((uint64_t)mid[4] << 32) | (uint64_t)lo[4]};
    state[col + 20] = (word_t){hi[5], ((uint64_t)mid[5] << 32) | (uint64_t)lo[5]};
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE void avx512_store_state_columns_w96(
    word_t *state,
    column96x8_t c0,
    column96x8_t c1,
    column96x8_t c2,
    column96x8_t c3)
{
    avx512_store_state_column96(state, 0, c0);
    avx512_store_state_column96(state, 1, c1);
    avx512_store_state_column96(state, 2, c2);
    avx512_store_state_column96(state, 3, c3);
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE column96x8_t avx512_load_rc_column96(size_t round_idx)
{
    const rc384_const_t *const rc = &RC_AVX512_W96[round_idx];
    column96x8_t out;

    out.hi = _mm256_setr_epi32(0, 0, 0, 0, 0, (int)rc->c0[3], 0, 0);
    out.mid = _mm256_setr_epi32(0, 0, 0, 0, 0, (int)rc->c1[3], 0, 0);
    out.lo = _mm256_setr_epi32(0, 0, 0, 0, 0, (int)rc->c2[3], 0, 0);
    return out;
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE __mmask16 avx512_prefix_mask16_w96(size_t count)
{
    if (count == 0) {
        return (__mmask16)0;
    }
    if (count >= 16) {
        return ~(__mmask16)0;
    }
    return ((__mmask16)1u << count) - 1u;
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE __m128i avx512_load_padded16_w96(
    const uint8_t *src,
    size_t valid_bytes)
{
    const __m128i fill = _mm_set1_epi8((char)0xFF);
    const __mmask16 valid_mask = avx512_prefix_mask16_w96(valid_bytes);

    if (valid_bytes == 0) {
        return fill;
    }

    {
        const __m128i loaded = _mm_maskz_loadu_epi8(valid_mask, src);
        return _mm_mask_blend_epi8(valid_mask, fill, loaded);
    }
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE family96x4_t avx512_load_padded_block_family96(
    const uint8_t *src,
    size_t valid_bytes)
{
    const __m128i a =
        avx512_load_padded16_w96(src, valid_bytes < 16 ? valid_bytes : 16);
    const __m128i b =
        avx512_load_padded16_w96(
            src + 16,
            valid_bytes > 16 ? (valid_bytes - 16 < 16 ? valid_bytes - 16 : 16) : 0);
    const __m128i c =
        avx512_load_padded16_w96(
            src + 32,
            valid_bytes > 32 ? (valid_bytes - 32 < 16 ? valid_bytes - 32 : 16) : 0);
    family96x4_t out;

    LLH_AVX512_W96_PACK_FAMILY_FROM_BLOCKS(out, a, b, c);
    return out;
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE void avx512_xor_rate_columns_from_families_w96(
    column96x8_t *c0,
    column96x8_t *c1,
    column96x8_t *c2,
    column96x8_t *c3,
    family96x4_t block0,
    family96x4_t block1)
{
    const __m128i zero = _mm_setzero_si128();
    const __m256i dup_rate_perm = avx512_load_u32x8_w96(&DUP_RATE_AVX512_W96_COL);

#define LLH_AVX512_W96_XOR_CHUNK_FROM_FAMILIES(dst0, dst1, dst2, dst3, chunk0, chunk1)         \
    do {                                                                                         \
        const __m128i llh_pair01 = _mm_unpacklo_epi32((chunk0), (chunk1));                      \
        const __m128i llh_pair23 = _mm_unpackhi_epi32((chunk0), (chunk1));                      \
                                                                                                 \
        (dst0) = _mm256_xor_si256(                                                               \
            (dst0),                                                                              \
            _mm256_permutevar8x32_epi32(                                                         \
                _mm256_zextsi128_si256(_mm_unpacklo_epi64(llh_pair01, zero)),                    \
                dup_rate_perm));                                                                  \
        (dst1) = _mm256_xor_si256(                                                               \
            (dst1),                                                                              \
            _mm256_permutevar8x32_epi32(                                                         \
                _mm256_zextsi128_si256(_mm_unpackhi_epi64(llh_pair01, zero)),                    \
                dup_rate_perm));                                                                  \
        (dst2) = _mm256_xor_si256(                                                               \
            (dst2),                                                                              \
            _mm256_permutevar8x32_epi32(                                                         \
                _mm256_zextsi128_si256(_mm_unpacklo_epi64(llh_pair23, zero)),                    \
                dup_rate_perm));                                                                  \
        (dst3) = _mm256_xor_si256(                                                               \
            (dst3),                                                                              \
            _mm256_permutevar8x32_epi32(                                                         \
                _mm256_zextsi128_si256(_mm_unpackhi_epi64(llh_pair23, zero)),                    \
                dup_rate_perm));                                                                  \
    } while (0)

    LLH_AVX512_W96_XOR_CHUNK_FROM_FAMILIES(c0->hi, c1->hi, c2->hi, c3->hi, block0.c0, block1.c0);
    LLH_AVX512_W96_XOR_CHUNK_FROM_FAMILIES(c0->mid, c1->mid, c2->mid, c3->mid, block0.c1, block1.c1);
    LLH_AVX512_W96_XOR_CHUNK_FROM_FAMILIES(c0->lo, c1->lo, c2->lo, c3->lo, block0.c2, block1.c2);

#undef LLH_AVX512_W96_XOR_CHUNK_FROM_FAMILIES
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE void avx512_rate_columns_to_families_w96(
    column96x8_t c0,
    column96x8_t c1,
    column96x8_t c2,
    column96x8_t c3,
    family96x4_t *g0,
    family96x4_t *g1)
{
    const __m128i c0_hi = _mm256_castsi256_si128(c0.hi);
    const __m128i c0_mid = _mm256_castsi256_si128(c0.mid);
    const __m128i c0_lo = _mm256_castsi256_si128(c0.lo);
    const __m128i c1_hi = _mm256_castsi256_si128(c1.hi);
    const __m128i c1_mid = _mm256_castsi256_si128(c1.mid);
    const __m128i c1_lo = _mm256_castsi256_si128(c1.lo);
    const __m128i c2_hi = _mm256_castsi256_si128(c2.hi);
    const __m128i c2_mid = _mm256_castsi256_si128(c2.mid);
    const __m128i c2_lo = _mm256_castsi256_si128(c2.lo);
    const __m128i c3_hi = _mm256_castsi256_si128(c3.hi);
    const __m128i c3_mid = _mm256_castsi256_si128(c3.mid);
    const __m128i c3_lo = _mm256_castsi256_si128(c3.lo);

#define LLH_AVX512_W96_RATE_COLUMNS_TO_FAMILIES(dst0, dst1, a, b, c, d)                  \
    do {                                                                                  \
        const __m128i llh_lo01 = _mm_unpacklo_epi32((a), (b));                           \
        const __m128i llh_lo23 = _mm_unpacklo_epi32((c), (d));                           \
                                                                                          \
        (dst0) = _mm_unpacklo_epi64(llh_lo01, llh_lo23);                                 \
        (dst1) = _mm_unpackhi_epi64(llh_lo01, llh_lo23);                                 \
    } while (0)

    LLH_AVX512_W96_RATE_COLUMNS_TO_FAMILIES(g0->c0, g1->c0, c0_hi, c1_hi, c2_hi, c3_hi);
    LLH_AVX512_W96_RATE_COLUMNS_TO_FAMILIES(g0->c1, g1->c1, c0_mid, c1_mid, c2_mid, c3_mid);
    LLH_AVX512_W96_RATE_COLUMNS_TO_FAMILIES(g0->c2, g1->c2, c0_lo, c1_lo, c2_lo, c3_lo);

#undef LLH_AVX512_W96_RATE_COLUMNS_TO_FAMILIES
}

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE void avx512_squeeze_rate_columns_w96(
    column96x8_t c0,
    column96x8_t c1,
    column96x8_t c2,
    column96x8_t c3,
    uint8_t *out)
{
    family96x4_t g0;
    family96x4_t g1;

    avx512_rate_columns_to_families_w96(c0, c1, c2, c3, &g0, &g1);
    avx512_squeeze_rate_families(g0, g1, out);
}

#define LLH_AVX512_W96_ROUND_PINNED(round_idx_expr)                                         \
    do {                                                                                     \
        const column96x8_t llh_rc_vec = avx512_load_rc_column96((round_idx_expr));           \
        column96x8_t llh_out0, llh_out1, llh_out2, llh_out3;                                 \
                                                                                             \
        avx512_round_columns_chunk96(                                                        \
            llh_c0_hi,                                                                       \
            llh_c1_hi,                                                                       \
            llh_c2_hi,                                                                       \
            llh_c3_hi,                                                                       \
            build0,                                                                          \
            build1,                                                                          \
            build2,                                                                          \
            build3,                                                                          \
            rot1_perm,                                                                       \
            rot2_perm,                                                                       \
            rot3_perm,                                                                       \
            complement_mask,                                                                 \
            &llh_out0.hi,                                                                    \
            &llh_out1.hi,                                                                    \
            &llh_out2.hi,                                                                    \
            &llh_out3.hi);                                                                   \
        avx512_round_columns_chunk96(                                                        \
            llh_c0_mid,                                                                      \
            llh_c1_mid,                                                                      \
            llh_c2_mid,                                                                      \
            llh_c3_mid,                                                                      \
            build0,                                                                          \
            build1,                                                                          \
            build2,                                                                          \
            build3,                                                                          \
            rot1_perm,                                                                       \
            rot2_perm,                                                                       \
            rot3_perm,                                                                       \
            complement_mask,                                                                 \
            &llh_out0.mid,                                                                   \
            &llh_out1.mid,                                                                   \
            &llh_out2.mid,                                                                   \
            &llh_out3.mid);                                                                  \
        avx512_round_columns_chunk96(                                                        \
            llh_c0_lo,                                                                       \
            llh_c1_lo,                                                                       \
            llh_c2_lo,                                                                       \
            llh_c3_lo,                                                                       \
            build0,                                                                          \
            build1,                                                                          \
            build2,                                                                          \
            build3,                                                                          \
            rot1_perm,                                                                       \
            rot2_perm,                                                                       \
            rot3_perm,                                                                       \
            complement_mask,                                                                 \
            &llh_out0.lo,                                                                    \
            &llh_out1.lo,                                                                    \
            &llh_out2.lo,                                                                    \
            &llh_out3.lo);                                                                   \
                                                                                             \
        avx512_shiftrows_column96(&llh_out0, shift0, shift0_inv, LLH_AVX512_W96_SHIFT0_Q1, LLH_AVX512_W96_SHIFT0_Q2); \
        avx512_shiftrows_column96(&llh_out1, shift1, shift1_inv, LLH_AVX512_W96_SHIFT1_Q1, LLH_AVX512_W96_SHIFT1_Q2); \
        avx512_shiftrows_column96(&llh_out2, shift2, shift2_inv, LLH_AVX512_W96_SHIFT2_Q1, LLH_AVX512_W96_SHIFT2_Q2); \
        avx512_shiftrows_column96(&llh_out3, shift3, shift3_inv, LLH_AVX512_W96_SHIFT3_Q1, LLH_AVX512_W96_SHIFT3_Q2); \
                                                                                             \
        llh_out3 = avx512_xor_column96(llh_out3, llh_rc_vec);                               \
                                                                                             \
        llh_c0_hi = llh_out0.hi;                                                             \
        llh_c0_mid = llh_out0.mid;                                                           \
        llh_c0_lo = llh_out0.lo;                                                             \
        llh_c1_hi = llh_out1.hi;                                                             \
        llh_c1_mid = llh_out1.mid;                                                           \
        llh_c1_lo = llh_out1.lo;                                                             \
        llh_c2_hi = llh_out2.hi;                                                             \
        llh_c2_mid = llh_out2.mid;                                                           \
        llh_c2_lo = llh_out2.lo;                                                             \
        llh_c3_hi = llh_out3.hi;                                                             \
        llh_c3_mid = llh_out3.mid;                                                           \
        llh_c3_lo = llh_out3.lo;                                                             \
    } while (0)

LLH_TARGET_AVX512
static LLH_ALWAYS_INLINE void avx512_permute_columns_i2_w96(
    column96x8_t *c0,
    column96x8_t *c1,
    column96x8_t *c2,
    column96x8_t *c3)
{
    register __m256i llh_c0_hi __asm__("ymm20") = c0->hi;
    register __m256i llh_c0_mid __asm__("ymm21") = c0->mid;
    register __m256i llh_c0_lo __asm__("ymm22") = c0->lo;
    register __m256i llh_c1_hi __asm__("ymm23") = c1->hi;
    register __m256i llh_c1_mid __asm__("ymm24") = c1->mid;
    register __m256i llh_c1_lo __asm__("ymm25") = c1->lo;
    register __m256i llh_c2_hi __asm__("ymm26") = c2->hi;
    register __m256i llh_c2_mid __asm__("ymm27") = c2->mid;
    register __m256i llh_c2_lo __asm__("ymm28") = c2->lo;
    register __m256i llh_c3_hi __asm__("ymm29") = c3->hi;
    register __m256i llh_c3_mid __asm__("ymm30") = c3->mid;
    register __m256i llh_c3_lo __asm__("ymm31") = c3->lo;
    const __m256i build0 = avx512_load_u32x8_w96(&BUILD_PERM_AVX512_W96_COL[0]);
    const __m256i build1 = avx512_load_u32x8_w96(&BUILD_PERM_AVX512_W96_COL[1]);
    const __m256i build2 = avx512_load_u32x8_w96(&BUILD_PERM_AVX512_W96_COL[2]);
    const __m256i build3 = avx512_load_u32x8_w96(&BUILD_PERM_AVX512_W96_COL[3]);
    const __m256i rot1_perm = avx512_load_u32x8_w96(&ROT1_PERM_AVX512_W96_COL);
    const __m256i rot2_perm = avx512_load_u32x8_w96(&ROT2_PERM_AVX512_W96_COL);
    const __m256i rot3_perm = avx512_load_u32x8_w96(&ROT3_PERM_AVX512_W96_COL);
    const __m256i shift0 =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[0].counts);
    const __m256i shift0_inv =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[0].inv_counts);
    const __m256i shift1 =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[1].counts);
    const __m256i shift1_inv =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[1].inv_counts);
    const __m256i shift2 =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[2].counts);
    const __m256i shift2_inv =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[2].inv_counts);
    const __m256i shift3 =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[3].counts);
    const __m256i shift3_inv =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[3].inv_counts);
    const __m256i complement_mask = avx512_load_u32x8_w96(&COMPLEMENT_MASK_AVX512_W96_COL);

    __asm__ __volatile__(
        ""
        : "+v"(llh_c0_hi), "+v"(llh_c0_mid), "+v"(llh_c0_lo),
          "+v"(llh_c1_hi), "+v"(llh_c1_mid), "+v"(llh_c1_lo),
          "+v"(llh_c2_hi), "+v"(llh_c2_mid), "+v"(llh_c2_lo),
          "+v"(llh_c3_hi), "+v"(llh_c3_mid), "+v"(llh_c3_lo));

    LLH_PRAGMA_NO_UNROLL
    for (size_t round_idx = 0; round_idx < 32; round_idx += 2) {
        LLH_AVX512_W96_ROUND_PINNED(round_idx);
        LLH_AVX512_W96_ROUND_PINNED(round_idx + 1);
    }

    __asm__ __volatile__(
        ""
        : "+v"(llh_c0_hi), "+v"(llh_c0_mid), "+v"(llh_c0_lo),
          "+v"(llh_c1_hi), "+v"(llh_c1_mid), "+v"(llh_c1_lo),
          "+v"(llh_c2_hi), "+v"(llh_c2_mid), "+v"(llh_c2_lo),
          "+v"(llh_c3_hi), "+v"(llh_c3_mid), "+v"(llh_c3_lo));

    c0->hi = llh_c0_hi;
    c0->mid = llh_c0_mid;
    c0->lo = llh_c0_lo;
    c1->hi = llh_c1_hi;
    c1->mid = llh_c1_mid;
    c1->lo = llh_c1_lo;
    c2->hi = llh_c2_hi;
    c2->mid = llh_c2_mid;
    c2->lo = llh_c2_lo;
    c3->hi = llh_c3_hi;
    c3->mid = llh_c3_mid;
    c3->lo = llh_c3_lo;
}

#define LLH_AVX512_W96_PERMUTE_PINNED_STATE()                                             \
    do {                                                                                  \
        LLH_PRAGMA_NO_UNROLL                                                              \
        for (size_t round_idx = 0; round_idx < 32; round_idx += 2) {                      \
            LLH_AVX512_W96_ROUND_PINNED(round_idx);                                       \
            LLH_AVX512_W96_ROUND_PINNED(round_idx + 1);                                   \
        }                                                                                 \
    } while (0)

#define LLH_AVX512_W96_XOR_RATE_CHUNK_TO_REGS(dst0, dst1, dst2, dst3, chunk0, chunk1)         \
    do {                                                                                         \
        const __m128i llh_pair01 = _mm_unpacklo_epi32((chunk0), (chunk1));                      \
        const __m128i llh_pair23 = _mm_unpackhi_epi32((chunk0), (chunk1));                      \
                                                                                                 \
        (dst0) = _mm256_xor_si256(                                                               \
            (dst0),                                                                              \
            _mm256_permutevar8x32_epi32(                                                         \
                _mm256_zextsi128_si256(_mm_unpacklo_epi64(llh_pair01, llh_zero128)),            \
                dup_rate_perm));                                                                 \
        (dst1) = _mm256_xor_si256(                                                               \
            (dst1),                                                                              \
            _mm256_permutevar8x32_epi32(                                                         \
                _mm256_zextsi128_si256(_mm_unpackhi_epi64(llh_pair01, llh_zero128)),            \
                dup_rate_perm));                                                                 \
        (dst2) = _mm256_xor_si256(                                                               \
            (dst2),                                                                              \
            _mm256_permutevar8x32_epi32(                                                         \
                _mm256_zextsi128_si256(_mm_unpacklo_epi64(llh_pair23, llh_zero128)),            \
                dup_rate_perm));                                                                 \
        (dst3) = _mm256_xor_si256(                                                               \
            (dst3),                                                                              \
            _mm256_permutevar8x32_epi32(                                                         \
                _mm256_zextsi128_si256(_mm_unpackhi_epi64(llh_pair23, llh_zero128)),            \
                dup_rate_perm));                                                                 \
    } while (0)

#define LLH_AVX512_W96_XOR_RATE_BLOCK_TO_REGS(block_ptr_expr)                                    \
    do {                                                                                         \
        const family96x4_t llh_block0 = avx512_load_block_family96((block_ptr_expr));           \
        const family96x4_t llh_block1 = avx512_load_block_family96((block_ptr_expr) + 48);      \
                                                                                                 \
        LLH_AVX512_W96_XOR_RATE_CHUNK_TO_REGS(                                                   \
            llh_c0_hi, llh_c1_hi, llh_c2_hi, llh_c3_hi, llh_block0.c0, llh_block1.c0);         \
        LLH_AVX512_W96_XOR_RATE_CHUNK_TO_REGS(                                                   \
            llh_c0_mid, llh_c1_mid, llh_c2_mid, llh_c3_mid, llh_block0.c1, llh_block1.c1);     \
        LLH_AVX512_W96_XOR_RATE_CHUNK_TO_REGS(                                                   \
            llh_c0_lo, llh_c1_lo, llh_c2_lo, llh_c3_lo, llh_block0.c2, llh_block1.c2);         \
    } while (0)

#define LLH_AVX512_W96_XOR_PADDED_RATE_BLOCK_TO_REGS(block_ptr_expr, valid_bytes_expr)                \
    do {                                                                                               \
        const size_t llh_valid_bytes = (valid_bytes_expr);                                             \
        const family96x4_t llh_block0 =                                                                \
            avx512_load_padded_block_family96((block_ptr_expr), llh_valid_bytes < 48 ? llh_valid_bytes : 48); \
        const family96x4_t llh_block1 =                                                                \
            avx512_load_padded_block_family96(                                                          \
                (block_ptr_expr) + 48,                                                                 \
                llh_valid_bytes > 48 ? (llh_valid_bytes - 48 < 48 ? llh_valid_bytes - 48 : 48) : 0); \
                                                                                                       \
        LLH_AVX512_W96_XOR_RATE_CHUNK_TO_REGS(                                                         \
            llh_c0_hi, llh_c1_hi, llh_c2_hi, llh_c3_hi, llh_block0.c0, llh_block1.c0);               \
        LLH_AVX512_W96_XOR_RATE_CHUNK_TO_REGS(                                                         \
            llh_c0_mid, llh_c1_mid, llh_c2_mid, llh_c3_mid, llh_block0.c1, llh_block1.c1);           \
        LLH_AVX512_W96_XOR_RATE_CHUNK_TO_REGS(                                                         \
            llh_c0_lo, llh_c1_lo, llh_c2_lo, llh_c3_lo, llh_block0.c2, llh_block1.c2);               \
    } while (0)

LLH_TARGET_AVX512
static void avx512_process_full_blocks_i2(HashCtx *ctx, const uint8_t *data, size_t full_blocks)
{
    column96x8_t c0, c1, c2, c3;
    const __m256i build0 = avx512_load_u32x8_w96(&BUILD_PERM_AVX512_W96_COL[0]);
    const __m256i build1 = avx512_load_u32x8_w96(&BUILD_PERM_AVX512_W96_COL[1]);
    const __m256i build2 = avx512_load_u32x8_w96(&BUILD_PERM_AVX512_W96_COL[2]);
    const __m256i build3 = avx512_load_u32x8_w96(&BUILD_PERM_AVX512_W96_COL[3]);
    const __m256i rot1_perm = avx512_load_u32x8_w96(&ROT1_PERM_AVX512_W96_COL);
    const __m256i rot2_perm = avx512_load_u32x8_w96(&ROT2_PERM_AVX512_W96_COL);
    const __m256i rot3_perm = avx512_load_u32x8_w96(&ROT3_PERM_AVX512_W96_COL);
    const __m256i shift0 =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[0].counts);
    const __m256i shift0_inv =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[0].inv_counts);
    const __m256i shift1 =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[1].counts);
    const __m256i shift1_inv =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[1].inv_counts);
    const __m256i shift2 =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[2].counts);
    const __m256i shift2_inv =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[2].inv_counts);
    const __m256i shift3 =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[3].counts);
    const __m256i shift3_inv =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[3].inv_counts);
    const __m256i complement_mask = avx512_load_u32x8_w96(&COMPLEMENT_MASK_AVX512_W96_COL);
    const __m256i capacity_mask = avx512_load_u32x8_w96(&CAPACITY_MASK_AVX512_W96_COL);
    const __m256i dup_rate_perm = avx512_load_u32x8_w96(&DUP_RATE_AVX512_W96_COL);
    const __m128i llh_zero128 = _mm_setzero_si128();

    avx512_load_state_columns_w96(ctx->state.row, &c0, &c1, &c2, &c3);

    register __m256i llh_c0_hi __asm__("ymm20") = c0.hi;
    register __m256i llh_c0_mid __asm__("ymm21") = c0.mid;
    register __m256i llh_c0_lo __asm__("ymm22") = c0.lo;
    register __m256i llh_c1_hi __asm__("ymm23") = c1.hi;
    register __m256i llh_c1_mid __asm__("ymm24") = c1.mid;
    register __m256i llh_c1_lo __asm__("ymm25") = c1.lo;
    register __m256i llh_c2_hi __asm__("ymm26") = c2.hi;
    register __m256i llh_c2_mid __asm__("ymm27") = c2.mid;
    register __m256i llh_c2_lo __asm__("ymm28") = c2.lo;
    register __m256i llh_c3_hi __asm__("ymm29") = c3.hi;
    register __m256i llh_c3_mid __asm__("ymm30") = c3.mid;
    register __m256i llh_c3_lo __asm__("ymm31") = c3.lo;

    __asm__ __volatile__(
        ""
        : "+v"(llh_c0_hi), "+v"(llh_c0_mid), "+v"(llh_c0_lo),
          "+v"(llh_c1_hi), "+v"(llh_c1_mid), "+v"(llh_c1_lo),
          "+v"(llh_c2_hi), "+v"(llh_c2_mid), "+v"(llh_c2_lo),
          "+v"(llh_c3_hi), "+v"(llh_c3_mid), "+v"(llh_c3_lo));

    for (; full_blocks != 0; full_blocks--, data += RATE_BYTES) {
        const __m256i llh_prev0_hi = _mm256_and_si256(llh_c0_hi, capacity_mask);
        const __m256i llh_prev0_mid = _mm256_and_si256(llh_c0_mid, capacity_mask);
        const __m256i llh_prev0_lo = _mm256_and_si256(llh_c0_lo, capacity_mask);
        const __m256i llh_prev1_hi = _mm256_and_si256(llh_c1_hi, capacity_mask);
        const __m256i llh_prev1_mid = _mm256_and_si256(llh_c1_mid, capacity_mask);
        const __m256i llh_prev1_lo = _mm256_and_si256(llh_c1_lo, capacity_mask);
        const __m256i llh_prev2_hi = _mm256_and_si256(llh_c2_hi, capacity_mask);
        const __m256i llh_prev2_mid = _mm256_and_si256(llh_c2_mid, capacity_mask);
        const __m256i llh_prev2_lo = _mm256_and_si256(llh_c2_lo, capacity_mask);
        const __m256i llh_prev3_hi = _mm256_and_si256(llh_c3_hi, capacity_mask);
        const __m256i llh_prev3_mid = _mm256_and_si256(llh_c3_mid, capacity_mask);
        const __m256i llh_prev3_lo = _mm256_and_si256(llh_c3_lo, capacity_mask);

        LLH_AVX512_W96_PERMUTE_PINNED_STATE();

        llh_c0_hi = _mm256_xor_si256(llh_c0_hi, llh_prev0_hi);
        llh_c0_mid = _mm256_xor_si256(llh_c0_mid, llh_prev0_mid);
        llh_c0_lo = _mm256_xor_si256(llh_c0_lo, llh_prev0_lo);
        llh_c1_hi = _mm256_xor_si256(llh_c1_hi, llh_prev1_hi);
        llh_c1_mid = _mm256_xor_si256(llh_c1_mid, llh_prev1_mid);
        llh_c1_lo = _mm256_xor_si256(llh_c1_lo, llh_prev1_lo);
        llh_c2_hi = _mm256_xor_si256(llh_c2_hi, llh_prev2_hi);
        llh_c2_mid = _mm256_xor_si256(llh_c2_mid, llh_prev2_mid);
        llh_c2_lo = _mm256_xor_si256(llh_c2_lo, llh_prev2_lo);
        llh_c3_hi = _mm256_xor_si256(llh_c3_hi, llh_prev3_hi);
        llh_c3_mid = _mm256_xor_si256(llh_c3_mid, llh_prev3_mid);
        llh_c3_lo = _mm256_xor_si256(llh_c3_lo, llh_prev3_lo);
        LLH_AVX512_W96_XOR_RATE_BLOCK_TO_REGS(data);
    }

    __asm__ __volatile__(
        ""
        : "+v"(llh_c0_hi), "+v"(llh_c0_mid), "+v"(llh_c0_lo),
          "+v"(llh_c1_hi), "+v"(llh_c1_mid), "+v"(llh_c1_lo),
          "+v"(llh_c2_hi), "+v"(llh_c2_mid), "+v"(llh_c2_lo),
          "+v"(llh_c3_hi), "+v"(llh_c3_mid), "+v"(llh_c3_lo));

    c0.hi = llh_c0_hi;
    c0.mid = llh_c0_mid;
    c0.lo = llh_c0_lo;
    c1.hi = llh_c1_hi;
    c1.mid = llh_c1_mid;
    c1.lo = llh_c1_lo;
    c2.hi = llh_c2_hi;
    c2.mid = llh_c2_mid;
    c2.lo = llh_c2_lo;
    c3.hi = llh_c3_hi;
    c3.mid = llh_c3_mid;
    c3.lo = llh_c3_lo;

    avx512_store_state_columns_w96(ctx->state.row, c0, c1, c2, c3);
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
    const __m256i zero = _mm256_setzero_si256();
    const __m256i build0 = avx512_load_u32x8_w96(&BUILD_PERM_AVX512_W96_COL[0]);
    const __m256i build1 = avx512_load_u32x8_w96(&BUILD_PERM_AVX512_W96_COL[1]);
    const __m256i build2 = avx512_load_u32x8_w96(&BUILD_PERM_AVX512_W96_COL[2]);
    const __m256i build3 = avx512_load_u32x8_w96(&BUILD_PERM_AVX512_W96_COL[3]);
    const __m256i rot1_perm = avx512_load_u32x8_w96(&ROT1_PERM_AVX512_W96_COL);
    const __m256i rot2_perm = avx512_load_u32x8_w96(&ROT2_PERM_AVX512_W96_COL);
    const __m256i rot3_perm = avx512_load_u32x8_w96(&ROT3_PERM_AVX512_W96_COL);
    const __m256i shift0 =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[0].counts);
    const __m256i shift0_inv =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[0].inv_counts);
    const __m256i shift1 =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[1].counts);
    const __m256i shift1_inv =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[1].inv_counts);
    const __m256i shift2 =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[2].counts);
    const __m256i shift2_inv =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[2].inv_counts);
    const __m256i shift3 =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[3].counts);
    const __m256i shift3_inv =
        _mm256_load_si256((const __m256i *)(const void *)SHIFTROWS_AVX512_W96_COL[3].inv_counts);
    const __m256i complement_mask = avx512_load_u32x8_w96(&COMPLEMENT_MASK_AVX512_W96_COL);
    const __m256i capacity_mask = avx512_load_u32x8_w96(&CAPACITY_MASK_AVX512_W96_COL);
    const __m256i dup_rate_perm = avx512_load_u32x8_w96(&DUP_RATE_AVX512_W96_COL);
    const __m128i llh_zero128 = _mm_setzero_si128();

    register __m256i llh_c0_hi __asm__("ymm20") = zero;
    register __m256i llh_c0_mid __asm__("ymm21") = zero;
    register __m256i llh_c0_lo __asm__("ymm22") = zero;
    register __m256i llh_c1_hi __asm__("ymm23") = zero;
    register __m256i llh_c1_mid __asm__("ymm24") = zero;
    register __m256i llh_c1_lo __asm__("ymm25") = zero;
    register __m256i llh_c2_hi __asm__("ymm26") = zero;
    register __m256i llh_c2_mid __asm__("ymm27") = zero;
    register __m256i llh_c2_lo __asm__("ymm28") = zero;
    register __m256i llh_c3_hi __asm__("ymm29") = zero;
    register __m256i llh_c3_mid __asm__("ymm30") =
        _mm256_setr_epi32(0, 0, 0, 0, 0, (int)(uint32_t)(msg_len_bits >> 32), 0, 0);
    register __m256i llh_c3_lo __asm__("ymm31") =
        _mm256_setr_epi32(0, 0, 0, 0, 0, (int)(uint32_t)msg_len_bits, 0, 0);

    __asm__ __volatile__(
        ""
        : "+v"(llh_c0_hi), "+v"(llh_c0_mid), "+v"(llh_c0_lo),
          "+v"(llh_c1_hi), "+v"(llh_c1_mid), "+v"(llh_c1_lo),
          "+v"(llh_c2_hi), "+v"(llh_c2_mid), "+v"(llh_c2_lo),
          "+v"(llh_c3_hi), "+v"(llh_c3_mid), "+v"(llh_c3_lo));

    for (; full_blocks != 0; full_blocks--, msg += RATE_BYTES) {
        const __m256i llh_prev0_hi = _mm256_and_si256(llh_c0_hi, capacity_mask);
        const __m256i llh_prev0_mid = _mm256_and_si256(llh_c0_mid, capacity_mask);
        const __m256i llh_prev0_lo = _mm256_and_si256(llh_c0_lo, capacity_mask);
        const __m256i llh_prev1_hi = _mm256_and_si256(llh_c1_hi, capacity_mask);
        const __m256i llh_prev1_mid = _mm256_and_si256(llh_c1_mid, capacity_mask);
        const __m256i llh_prev1_lo = _mm256_and_si256(llh_c1_lo, capacity_mask);
        const __m256i llh_prev2_hi = _mm256_and_si256(llh_c2_hi, capacity_mask);
        const __m256i llh_prev2_mid = _mm256_and_si256(llh_c2_mid, capacity_mask);
        const __m256i llh_prev2_lo = _mm256_and_si256(llh_c2_lo, capacity_mask);
        const __m256i llh_prev3_hi = _mm256_and_si256(llh_c3_hi, capacity_mask);
        const __m256i llh_prev3_mid = _mm256_and_si256(llh_c3_mid, capacity_mask);
        const __m256i llh_prev3_lo = _mm256_and_si256(llh_c3_lo, capacity_mask);

        LLH_AVX512_W96_PERMUTE_PINNED_STATE();

        llh_c0_hi = _mm256_xor_si256(llh_c0_hi, llh_prev0_hi);
        llh_c0_mid = _mm256_xor_si256(llh_c0_mid, llh_prev0_mid);
        llh_c0_lo = _mm256_xor_si256(llh_c0_lo, llh_prev0_lo);
        llh_c1_hi = _mm256_xor_si256(llh_c1_hi, llh_prev1_hi);
        llh_c1_mid = _mm256_xor_si256(llh_c1_mid, llh_prev1_mid);
        llh_c1_lo = _mm256_xor_si256(llh_c1_lo, llh_prev1_lo);
        llh_c2_hi = _mm256_xor_si256(llh_c2_hi, llh_prev2_hi);
        llh_c2_mid = _mm256_xor_si256(llh_c2_mid, llh_prev2_mid);
        llh_c2_lo = _mm256_xor_si256(llh_c2_lo, llh_prev2_lo);
        llh_c3_hi = _mm256_xor_si256(llh_c3_hi, llh_prev3_hi);
        llh_c3_mid = _mm256_xor_si256(llh_c3_mid, llh_prev3_mid);
        llh_c3_lo = _mm256_xor_si256(llh_c3_lo, llh_prev3_lo);
        LLH_AVX512_W96_XOR_RATE_BLOCK_TO_REGS(msg);
    }

    if (tail_len != 0) {
        const __m256i llh_prev0_hi = _mm256_and_si256(llh_c0_hi, capacity_mask);
        const __m256i llh_prev0_mid = _mm256_and_si256(llh_c0_mid, capacity_mask);
        const __m256i llh_prev0_lo = _mm256_and_si256(llh_c0_lo, capacity_mask);
        const __m256i llh_prev1_hi = _mm256_and_si256(llh_c1_hi, capacity_mask);
        const __m256i llh_prev1_mid = _mm256_and_si256(llh_c1_mid, capacity_mask);
        const __m256i llh_prev1_lo = _mm256_and_si256(llh_c1_lo, capacity_mask);
        const __m256i llh_prev2_hi = _mm256_and_si256(llh_c2_hi, capacity_mask);
        const __m256i llh_prev2_mid = _mm256_and_si256(llh_c2_mid, capacity_mask);
        const __m256i llh_prev2_lo = _mm256_and_si256(llh_c2_lo, capacity_mask);
        const __m256i llh_prev3_hi = _mm256_and_si256(llh_c3_hi, capacity_mask);
        const __m256i llh_prev3_mid = _mm256_and_si256(llh_c3_mid, capacity_mask);
        const __m256i llh_prev3_lo = _mm256_and_si256(llh_c3_lo, capacity_mask);

        LLH_AVX512_W96_PERMUTE_PINNED_STATE();

        llh_c0_hi = _mm256_xor_si256(llh_c0_hi, llh_prev0_hi);
        llh_c0_mid = _mm256_xor_si256(llh_c0_mid, llh_prev0_mid);
        llh_c0_lo = _mm256_xor_si256(llh_c0_lo, llh_prev0_lo);
        llh_c1_hi = _mm256_xor_si256(llh_c1_hi, llh_prev1_hi);
        llh_c1_mid = _mm256_xor_si256(llh_c1_mid, llh_prev1_mid);
        llh_c1_lo = _mm256_xor_si256(llh_c1_lo, llh_prev1_lo);
        llh_c2_hi = _mm256_xor_si256(llh_c2_hi, llh_prev2_hi);
        llh_c2_mid = _mm256_xor_si256(llh_c2_mid, llh_prev2_mid);
        llh_c2_lo = _mm256_xor_si256(llh_c2_lo, llh_prev2_lo);
        llh_c3_hi = _mm256_xor_si256(llh_c3_hi, llh_prev3_hi);
        llh_c3_mid = _mm256_xor_si256(llh_c3_mid, llh_prev3_mid);
        llh_c3_lo = _mm256_xor_si256(llh_c3_lo, llh_prev3_lo);
        LLH_AVX512_W96_XOR_PADDED_RATE_BLOCK_TO_REGS(msg, tail_len);
    }

    LLH_AVX512_W96_PERMUTE_PINNED_STATE();

    __asm__ __volatile__(
        ""
        : "+v"(llh_c0_hi), "+v"(llh_c0_mid), "+v"(llh_c0_lo),
          "+v"(llh_c1_hi), "+v"(llh_c1_mid), "+v"(llh_c1_lo),
          "+v"(llh_c2_hi), "+v"(llh_c2_mid), "+v"(llh_c2_lo),
          "+v"(llh_c3_hi), "+v"(llh_c3_mid), "+v"(llh_c3_lo));

    avx512_squeeze_rate_columns_w96(
        (column96x8_t){ llh_c0_hi, llh_c0_mid, llh_c0_lo },
        (column96x8_t){ llh_c1_hi, llh_c1_mid, llh_c1_lo },
        (column96x8_t){ llh_c2_hi, llh_c2_mid, llh_c2_lo },
        (column96x8_t){ llh_c3_hi, llh_c3_mid, llh_c3_lo },
        out);
}

#undef LLH_AVX512_W96_XOR_PADDED_RATE_BLOCK_TO_REGS
#undef LLH_AVX512_W96_XOR_RATE_BLOCK_TO_REGS
#undef LLH_AVX512_W96_XOR_RATE_CHUNK_TO_REGS
#undef LLH_AVX512_W96_PERMUTE_PINNED_STATE

#undef LLH_AVX512_W96_ROUND_PINNED

LLH_TARGET_AVX512
static void permutation_i2_avx512(State *s)
{
    column96x8_t c0, c1, c2, c3;

    avx512_load_state_columns_w96(s->row, &c0, &c1, &c2, &c3);
    avx512_permute_columns_i2_w96(&c0, &c1, &c2, &c3);
    avx512_store_state_columns_w96(s->row, c0, c1, c2, c3);
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
        s->row[i] = WORD_XOR(s->row[i], load_word(block + i * row_bytes));
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
        ctx->state.row[i] = WORD_XOR(ctx->state.row[i], prev.row[i]);
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

    ctx->state.row[23] = (word_t){0u, msg_len_bits};
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
