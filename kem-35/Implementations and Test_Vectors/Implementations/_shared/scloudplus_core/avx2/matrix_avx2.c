/**
 * @file matrix_avx2.c
 * @brief AVX2 matrix backend for AES/SHAKE/SM3 Scloud+ instances.
 *
 * This backend keeps the optimized family-specific kernels in one source file
 * so reviewers do not need to chase per-family files.  The build defines
 * exactly one matrix-family macro, so the compiler keeps only the matching
 * block.
 */


/* Shared AVX2 public-matrix multiply entry points.  The byte stream used to
 * generate A remains selected at compile time by family and A encoding, while
 * the matrix products themselves are common. */
#if defined(SCLOUDPLUS_AVX2_FAMILY_AES) || \
    defined(SCLOUDPLUS_AVX2_FAMILY_SHAKE) || \
    defined(SCLOUDPLUS_AVX2_FAMILY_SM3)

#include "matrix.h"
#include "modarith.h"
#include "scloudplus_param_common.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "avx2_util.h"

#if defined(SCLOUDPLUS_AVX2_FAMILY_AES)
#define SCLOUDPLUS_AVX2_FAMILY_AES 1
#include "aes.h"
#endif
#if defined(SCLOUDPLUS_AVX2_FAMILY_SHAKE)
#define SCLOUDPLUS_AVX2_FAMILY_SHAKE 1
#include "hash.h"
#include "fips202x4.h"
#endif
#if defined(SCLOUDPLUS_AVX2_FAMILY_SM3)
#define SCLOUDPLUS_AVX2_FAMILY_SM3 1
#include "hash.h"
int sm3_bounded_xof8_a_rows(uint8_t *out[8], size_t outlen,
                            const uint8_t *seed, uint32_t row_start);
#endif
#if defined(_WIN32) || defined(_WIN64)
#define ALIGN_HEADER(N) __declspec(aligned(N))
#define ALIGN_FOOTER(N)
#else
#define ALIGN_HEADER(N)
#define ALIGN_FOOTER(N) __attribute__((aligned(N)))
#endif

static inline uint16_t scloudplus_avx2_common_hsum_i32_mod_2p16(__m256i value)
{
    const __m128i lo = _mm256_castsi256_si128(value);
    const __m128i hi = _mm256_extracti128_si256(value, 1);
    __m128i sum = _mm_add_epi32(lo, hi);

    sum = _mm_hadd_epi32(sum, sum);
    sum = _mm_hadd_epi32(sum, sum);
    return (uint16_t)_mm_cvtsi128_si32(sum);
}

static inline void scloudplus_avx2_common_dot_rows8(const uint16_t *rows,
                                                    const uint16_t *sp,
                                                    uint16_t sums[8])
{
    __m256i acc0 = _mm256_setzero_si256();
    __m256i acc1 = _mm256_setzero_si256();
    __m256i acc2 = _mm256_setzero_si256();
    __m256i acc3 = _mm256_setzero_si256();
    __m256i acc4 = _mm256_setzero_si256();
    __m256i acc5 = _mm256_setzero_si256();
    __m256i acc6 = _mm256_setzero_si256();
    __m256i acc7 = _mm256_setzero_si256();

    for (size_t j = 0; j < (size_t)scloudplus_n; j += SCLOUDPLUS_AVX2_LANES_U16) {
        const __m256i s = _mm256_loadu_si256((const __m256i *)(sp + j));
        acc0 = _mm256_add_epi32(acc0, _mm256_madd_epi16(_mm256_load_si256((const __m256i *)(rows + 0U * (size_t)scloudplus_n + j)), s));
        acc1 = _mm256_add_epi32(acc1, _mm256_madd_epi16(_mm256_load_si256((const __m256i *)(rows + 1U * (size_t)scloudplus_n + j)), s));
        acc2 = _mm256_add_epi32(acc2, _mm256_madd_epi16(_mm256_load_si256((const __m256i *)(rows + 2U * (size_t)scloudplus_n + j)), s));
        acc3 = _mm256_add_epi32(acc3, _mm256_madd_epi16(_mm256_load_si256((const __m256i *)(rows + 3U * (size_t)scloudplus_n + j)), s));
        acc4 = _mm256_add_epi32(acc4, _mm256_madd_epi16(_mm256_load_si256((const __m256i *)(rows + 4U * (size_t)scloudplus_n + j)), s));
        acc5 = _mm256_add_epi32(acc5, _mm256_madd_epi16(_mm256_load_si256((const __m256i *)(rows + 5U * (size_t)scloudplus_n + j)), s));
        acc6 = _mm256_add_epi32(acc6, _mm256_madd_epi16(_mm256_load_si256((const __m256i *)(rows + 6U * (size_t)scloudplus_n + j)), s));
        acc7 = _mm256_add_epi32(acc7, _mm256_madd_epi16(_mm256_load_si256((const __m256i *)(rows + 7U * (size_t)scloudplus_n + j)), s));
    }

    sums[0] = scloudplus_avx2_common_hsum_i32_mod_2p16(acc0);
    sums[1] = scloudplus_avx2_common_hsum_i32_mod_2p16(acc1);
    sums[2] = scloudplus_avx2_common_hsum_i32_mod_2p16(acc2);
    sums[3] = scloudplus_avx2_common_hsum_i32_mod_2p16(acc3);
    sums[4] = scloudplus_avx2_common_hsum_i32_mod_2p16(acc4);
    sums[5] = scloudplus_avx2_common_hsum_i32_mod_2p16(acc5);
    sums[6] = scloudplus_avx2_common_hsum_i32_mod_2p16(acc6);
    sums[7] = scloudplus_avx2_common_hsum_i32_mod_2p16(acc7);
}

/*
 * Compute four A-row dot products against two secret columns.  This halves the
 * number of times the generated A rows are read in mul_as_e compared with
 * calling the single-column dot kernel twice.  The helper is parameter-generic:
 * callers pass the base row pointer, so it is used for both row halves of the
 * 8-row A generation batch and for all security levels.
 */
static inline void scloudplus_avx2_common_dot_rows4_cols2(const uint16_t *rows,
                                                          const uint16_t *sp0,
                                                          const uint16_t *sp1,
                                                          uint16_t sums0[4],
                                                          uint16_t sums1[4])
{
    __m256i acc00 = _mm256_setzero_si256();
    __m256i acc01 = _mm256_setzero_si256();
    __m256i acc10 = _mm256_setzero_si256();
    __m256i acc11 = _mm256_setzero_si256();
    __m256i acc20 = _mm256_setzero_si256();
    __m256i acc21 = _mm256_setzero_si256();
    __m256i acc30 = _mm256_setzero_si256();
    __m256i acc31 = _mm256_setzero_si256();

    for (size_t j = 0; j < (size_t)scloudplus_n; j += SCLOUDPLUS_AVX2_LANES_U16) {
        const __m256i s0 = _mm256_loadu_si256((const __m256i *)(sp0 + j));
        const __m256i s1 = _mm256_loadu_si256((const __m256i *)(sp1 + j));
        const __m256i a0 = _mm256_load_si256((const __m256i *)(rows + 0U * (size_t)scloudplus_n + j));
        const __m256i a1 = _mm256_load_si256((const __m256i *)(rows + 1U * (size_t)scloudplus_n + j));
        const __m256i a2 = _mm256_load_si256((const __m256i *)(rows + 2U * (size_t)scloudplus_n + j));
        const __m256i a3 = _mm256_load_si256((const __m256i *)(rows + 3U * (size_t)scloudplus_n + j));

        acc00 = _mm256_add_epi32(acc00, _mm256_madd_epi16(a0, s0));
        acc01 = _mm256_add_epi32(acc01, _mm256_madd_epi16(a0, s1));
        acc10 = _mm256_add_epi32(acc10, _mm256_madd_epi16(a1, s0));
        acc11 = _mm256_add_epi32(acc11, _mm256_madd_epi16(a1, s1));
        acc20 = _mm256_add_epi32(acc20, _mm256_madd_epi16(a2, s0));
        acc21 = _mm256_add_epi32(acc21, _mm256_madd_epi16(a2, s1));
        acc30 = _mm256_add_epi32(acc30, _mm256_madd_epi16(a3, s0));
        acc31 = _mm256_add_epi32(acc31, _mm256_madd_epi16(a3, s1));
    }

    sums0[0] = scloudplus_avx2_common_hsum_i32_mod_2p16(acc00);
    sums1[0] = scloudplus_avx2_common_hsum_i32_mod_2p16(acc01);
    sums0[1] = scloudplus_avx2_common_hsum_i32_mod_2p16(acc10);
    sums1[1] = scloudplus_avx2_common_hsum_i32_mod_2p16(acc11);
    sums0[2] = scloudplus_avx2_common_hsum_i32_mod_2p16(acc20);
    sums1[2] = scloudplus_avx2_common_hsum_i32_mod_2p16(acc21);
    sums0[3] = scloudplus_avx2_common_hsum_i32_mod_2p16(acc30);
    sums1[3] = scloudplus_avx2_common_hsum_i32_mod_2p16(acc31);
}

static inline void scloudplus_avx2_common_mul_add_rows8(uint16_t *dst,
                                                        const uint16_t *rows,
                                                        const uint16_t coeffs[8])
{
    for (size_t q = 0; q < (size_t)scloudplus_n; q += SCLOUDPLUS_AVX2_LANES_U16) {
        __m256i sum = _mm256_loadu_si256((const __m256i *)(dst + q));
        for (size_t row = 0U; row < 8U; row++) {
            const __m256i a = _mm256_load_si256((const __m256i *)(rows + row * (size_t)scloudplus_n + q));
            const __m256i c = _mm256_set1_epi16((short)coeffs[row]);
            sum = _mm256_add_epi16(sum, _mm256_mullo_epi16(a, c));
        }
        _mm256_storeu_si256((__m256i *)(dst + q), sum);
    }
}

/*
 * Add the same 8 generated A rows to two output rows with two independent
 * coefficient vectors.  This keeps the row-major A batch hot for mul_sa_e and
 * avoids loading the 8 rows once per mbar output row.
 */
static inline void scloudplus_avx2_common_mul_add_rows8_dst2(uint16_t *dst0,
                                                             uint16_t *dst1,
                                                             const uint16_t *rows,
                                                             const uint16_t coeffs0[8],
                                                             const uint16_t coeffs1[8])
{
    for (size_t q = 0; q < (size_t)scloudplus_n; q += SCLOUDPLUS_AVX2_LANES_U16) {
        __m256i sum0 = _mm256_loadu_si256((const __m256i *)(dst0 + q));
        __m256i sum1 = _mm256_loadu_si256((const __m256i *)(dst1 + q));
        for (size_t row = 0U; row < 8U; row++) {
            const __m256i a = _mm256_load_si256((const __m256i *)(rows + row * (size_t)scloudplus_n + q));
            const __m256i c0 = _mm256_set1_epi16((short)coeffs0[row]);
            const __m256i c1 = _mm256_set1_epi16((short)coeffs1[row]);
            sum0 = _mm256_add_epi16(sum0, _mm256_mullo_epi16(a, c0));
            sum1 = _mm256_add_epi16(sum1, _mm256_mullo_epi16(a, c1));
        }
        _mm256_storeu_si256((__m256i *)(dst0 + q), sum0);
        _mm256_storeu_si256((__m256i *)(dst1 + q), sum1);
    }
}

static inline __m128i scloudplus_avx2_common_load10_u8(const uint8_t *in)
{
    uint64_t lo;
    uint16_t hi;
    memcpy(&lo, in, sizeof(lo));
    memcpy(&hi, in + 8U, sizeof(hi));
    return _mm_insert_epi16(_mm_cvtsi64_si128((long long)lo), (int)hi, 4);
}

static inline __m256i scloudplus_avx2_common_unpack10_x8_u32(const uint8_t *in)
{
    const __m128i bytes = scloudplus_avx2_common_load10_u8(in);
    const __m128i gather = _mm_setr_epi8(0, 1, 1, 2, 2, 3, 3, 4, 5, 6, 6, 7, 7, 8, 8, 9);
    const __m256i shifts = _mm256_setr_epi32(0, 2, 4, 6, 0, 2, 4, 6);
    const __m256i mask = _mm256_set1_epi32(0x3FF);
    const __m128i windows16 = _mm_shuffle_epi8(bytes, gather);
    const __m256i windows32 = _mm256_cvtepu16_epi32(windows16);

    return _mm256_and_si256(_mm256_srlv_epi32(windows32, shifts), mask);
}

static inline void scloudplus_avx2_common_unpack10_x16(const uint8_t *in,
                                                       uint16_t *out)
{
    const __m256i lo = scloudplus_avx2_common_unpack10_x8_u32(in);
    const __m256i hi = scloudplus_avx2_common_unpack10_x8_u32(in + 10U);
    const __m128i lo16 = _mm_packus_epi32(_mm256_castsi256_si128(lo),
                                          _mm256_extracti128_si256(lo, 1));
    const __m128i hi16 = _mm_packus_epi32(_mm256_castsi256_si128(hi),
                                          _mm256_extracti128_si256(hi, 1));

    _mm_storeu_si128((__m128i *)out, lo16);
    _mm_storeu_si128((__m128i *)(out + 8U), hi16);
}

#if defined(SCLOUDPLUS_AVX2_FAMILY_SHAKE)
#if scloudplus_seedA_bytes != 16
#error "SHAKE AVX2 direct A absorber assumes a 16-byte seedA"
#endif
extern void KeccakP1600times4_PermuteAll_24rounds(__m256i *s);
extern void KeccakP1600times4_ExtractLanesAll(const void *states,
                                              unsigned char *data,
                                              unsigned int laneCount,
                                              unsigned int laneOffset);

typedef struct { uint64_t lane0; uint64_t lane1; } scloudplus_avx2_a_seed_lanes;

static inline uint64_t scloudplus_avx2_load_seed_lane0(const uint8_t *seed)
{
    return (uint64_t)seed[0] | ((uint64_t)seed[1] << 8U) |
           ((uint64_t)seed[2] << 16U) | ((uint64_t)seed[3] << 24U) |
           ((uint64_t)seed[4] << 32U) | ((uint64_t)seed[5] << 40U) |
           ((uint64_t)seed[6] << 48U) | ((uint64_t)seed[7] << 56U);
}

static inline uint64_t scloudplus_avx2_load_seed_lane1(const uint8_t *seed)
{
    return (uint64_t)seed[8] | ((uint64_t)seed[9] << 8U) |
           ((uint64_t)seed[10] << 16U) | ((uint64_t)seed[11] << 24U) |
           ((uint64_t)seed[12] << 32U) | ((uint64_t)seed[13] << 40U) |
           ((uint64_t)seed[14] << 48U) | ((uint64_t)seed[15] << 56U);
}

static inline void scloudplus_avx2_init_shake_state4(__m256i state[25],
                                                     const scloudplus_avx2_a_seed_lanes *seed,
                                                     uint32_t row_start)
{
    const __m256i zero = _mm256_setzero_si256();
    for (size_t i = 0; i < 25U; i++)
        state[i] = zero;
    state[0] = _mm256_set1_epi64x((long long)seed->lane0);
    state[1] = _mm256_set1_epi64x((long long)seed->lane1);
    state[2] = _mm256_set_epi64x((long long)((uint64_t)(row_start + 3U) | ((uint64_t)0x1FU << 32U)),
                                 (long long)((uint64_t)(row_start + 2U) | ((uint64_t)0x1FU << 32U)),
                                 (long long)((uint64_t)(row_start + 1U) | ((uint64_t)0x1FU << 32U)),
                                 (long long)((uint64_t)(row_start + 0U) | ((uint64_t)0x1FU << 32U)));
    state[20] = _mm256_set1_epi64x((long long)UINT64_C(0x8000000000000000));
}
#endif

typedef struct {
#if defined(SCLOUDPLUS_AVX2_FAMILY_AES)
    uint8_t schedule[16 * 11];
#elif defined(SCLOUDPLUS_AVX2_FAMILY_SHAKE)
    scloudplus_avx2_a_seed_lanes shake_seed;
#else
    uint8_t seed[scloudplus_seedA_bytes];
#endif
} scloudplus_avx2_a_state;

static inline void scloudplus_avx2_a_state_init(scloudplus_avx2_a_state *st,
                                                const uint8_t *seedA)
{
#if defined(SCLOUDPLUS_AVX2_FAMILY_AES)
    AES128_load_schedule(seedA, st->schedule);
#elif defined(SCLOUDPLUS_AVX2_FAMILY_SHAKE)
    st->shake_seed.lane0 = scloudplus_avx2_load_seed_lane0(seedA);
    st->shake_seed.lane1 = scloudplus_avx2_load_seed_lane1(seedA);
#else
    memcpy(st->seed, seedA, scloudplus_seedA_bytes);
#endif
}

static inline void scloudplus_avx2_a_state_clear(scloudplus_avx2_a_state *st)
{
#if defined(SCLOUDPLUS_AVX2_FAMILY_AES)
    AES128_free_schedule(st->schedule);
#else
    memset(st, 0, sizeof(*st));
#endif
}

static void scloudplus_avx2_generate_a_rows8(scloudplus_avx2_a_state *st,
                                             size_t row_start,
                                             uint8_t *packed_rows,
                                             uint16_t *out_rows)
{
#if defined(SCLOUDPLUS_AVX2_FAMILY_AES)
    AES128_CTR_zero_sch((uint32_t)(row_start * (size_t)scloudplus_a_blocks_per_row),
                        8U * (size_t)scloudplus_a_blocks_per_row,
                        st->schedule, packed_rows);
    memset(packed_rows + 8U * (size_t)scloudplus_a_padded_bytes_per_row, 0, 16U);
    for (size_t row = 0U; row < 8U; row++) {
        const uint8_t *src = packed_rows + row * (size_t)scloudplus_a_padded_bytes_per_row;
        uint16_t *dst = out_rows + row * (size_t)scloudplus_n;
        for (size_t in_idx = 0U, out_idx = 0U; out_idx < (size_t)scloudplus_n; in_idx += 20U, out_idx += 16U)
            scloudplus_avx2_common_unpack10_x16(src + in_idx, dst + out_idx);
    }
#elif defined(SCLOUDPLUS_AVX2_FAMILY_SHAKE)
    enum {
        AVX2_A_ROW_BYTES = scloudplus_a_bytes_per_row,
        AVX2_A_FULL_BLOCKS = AVX2_A_ROW_BYTES / SHAKE128_RATE,
        AVX2_A_REM_BYTES = AVX2_A_ROW_BYTES - AVX2_A_FULL_BLOCKS * SHAKE128_RATE,
        AVX2_A_REM_CEIL_LANES = (AVX2_A_REM_BYTES + 7U) / 8U
    };
    uint8_t *bytes = packed_rows;
    for (size_t group = 0U; group < 2U; group++) {
        __m256i state[25];
        uint8_t block_out[4U * SHAKE128_RATE];
        uint8_t *out = bytes + group * 4U * (size_t)AVX2_A_ROW_BYTES;
        scloudplus_avx2_init_shake_state4(state, &st->shake_seed,
                                          (uint32_t)row_start + (uint32_t)(4U * group));
        for (size_t block = 0U; block < (size_t)AVX2_A_FULL_BLOCKS; block++) {
            KeccakP1600times4_PermuteAll_24rounds(state);
            KeccakP1600times4_ExtractLanesAll(state, block_out,
                                              SHAKE128_RATE / 8U,
                                              SHAKE128_RATE / 8U);
            for (size_t row = 0U; row < 4U; row++)
                memcpy(out + row * (size_t)AVX2_A_ROW_BYTES + block * (size_t)SHAKE128_RATE,
                       block_out + row * (size_t)SHAKE128_RATE,
                       SHAKE128_RATE);
        }
        KeccakP1600times4_PermuteAll_24rounds(state);
        KeccakP1600times4_ExtractLanesAll(state, block_out,
                                          AVX2_A_REM_CEIL_LANES,
                                          AVX2_A_REM_CEIL_LANES);
        for (size_t row = 0U; row < 4U; row++)
            memcpy(out + row * (size_t)AVX2_A_ROW_BYTES +
                       (size_t)AVX2_A_FULL_BLOCKS * (size_t)SHAKE128_RATE,
                   block_out + row * (size_t)AVX2_A_REM_CEIL_LANES * 8U,
                   AVX2_A_REM_BYTES);
    }
    for (size_t row = 0U; row < 8U; row++) {
        const uint8_t *src = packed_rows + row * (size_t)AVX2_A_ROW_BYTES;
        uint16_t *dst = out_rows + row * (size_t)scloudplus_n;
        for (size_t in_idx = 0U, out_idx = 0U; out_idx < (size_t)scloudplus_n; in_idx += 20U, out_idx += 16U)
            scloudplus_avx2_common_unpack10_x16(src + in_idx, dst + out_idx);
    }
#elif defined(SCLOUDPLUS_AVX2_FAMILY_SM3)
    uint8_t *row_ptrs[8];
    for (size_t row = 0U; row < 8U; row++)
        row_ptrs[row] = packed_rows + row * (size_t)scloudplus_a_padded_bytes_per_row;
    sm3_bounded_xof8_a_rows(row_ptrs, (size_t)scloudplus_a_bytes_per_row, st->seed,
                    (uint32_t)row_start);
    for (size_t row = 0U; row < 8U; row++) {
        const uint8_t *src = packed_rows + row * (size_t)scloudplus_a_padded_bytes_per_row;
        uint16_t *dst = out_rows + row * (size_t)scloudplus_n;
        for (size_t in_idx = 0U, out_idx = 0U; out_idx < (size_t)scloudplus_n; in_idx += 20U, out_idx += 16U)
            scloudplus_avx2_common_unpack10_x16(src + in_idx, dst + out_idx);
    }
#else
#error "Unsupported AVX2 A-row generator selection"
#endif
}

void mul_as_e(const uint8_t *seedA, const uint16_t *S,
              const uint16_t *E, uint16_t *B)
{
    ALIGN_HEADER(32)
    uint16_t a_rows[8U * (size_t)scloudplus_n] ALIGN_FOOTER(32);
    ALIGN_HEADER(32)
    uint8_t packed_rows[8U * (size_t)scloudplus_a_padded_bytes_per_row + 16U] ALIGN_FOOTER(32);
    scloudplus_avx2_a_state st;

    if (B != E)
        memcpy(B, E, 2U * (size_t)scloudplus_m * (size_t)scloudplus_nbar);
    scloudplus_avx2_a_state_init(&st, seedA);
    for (int i = 0; i < scloudplus_m; i += 8) {
        scloudplus_avx2_generate_a_rows8(&st, (size_t)i, packed_rows, a_rows);
        int k = 0;
        for (; k + 1 < scloudplus_nbar; k += 2) {
            const uint16_t *sp0 = S + (size_t)k * (size_t)scloudplus_n;
            const uint16_t *sp1 = sp0 + (size_t)scloudplus_n;
            uint16_t sums0_lo[4];
            uint16_t sums1_lo[4];
            uint16_t sums0_hi[4];
            uint16_t sums1_hi[4];

            scloudplus_avx2_common_dot_rows4_cols2(a_rows, sp0, sp1,
                                                   sums0_lo, sums1_lo);
            scloudplus_avx2_common_dot_rows4_cols2(a_rows + 4U * (size_t)scloudplus_n,
                                                   sp0, sp1, sums0_hi, sums1_hi);
            for (int row = 0; row < 4; row++) {
                const size_t base = (size_t)(i + row) * (size_t)scloudplus_nbar + (size_t)k;
                B[base] = (uint16_t)(B[base] + sums0_lo[row]);
                B[base + 1U] = (uint16_t)(B[base + 1U] + sums1_lo[row]);
            }
            for (int row = 0; row < 4; row++) {
                const size_t base = (size_t)(i + row + 4) * (size_t)scloudplus_nbar + (size_t)k;
                B[base] = (uint16_t)(B[base] + sums0_hi[row]);
                B[base + 1U] = (uint16_t)(B[base + 1U] + sums1_hi[row]);
            }
        }
        if (k < scloudplus_nbar) {
            const uint16_t *sp = S + (size_t)k * (size_t)scloudplus_n;
            uint16_t sums[8];
            scloudplus_avx2_common_dot_rows8(a_rows, sp, sums);
            for (int row = 0; row < 8; row++) {
                const size_t base = (size_t)(i + row) * (size_t)scloudplus_nbar + (size_t)k;
                B[base] = (uint16_t)(B[base] + sums[row]);
            }
        }
    }
    scloudplus_avx2_a_state_clear(&st);
}

void mul_sa_e(const uint8_t *seedA, const uint16_t *S,
              uint16_t *E, uint16_t *C)
{
    ALIGN_HEADER(32)
    uint16_t a_rows[8U * (size_t)scloudplus_n] ALIGN_FOOTER(32);
    ALIGN_HEADER(32)
    uint8_t packed_rows[8U * (size_t)scloudplus_a_padded_bytes_per_row + 16U] ALIGN_FOOTER(32);
    scloudplus_avx2_a_state st;

    if (C != E)
        memcpy(C, E, 2U * (size_t)scloudplus_mbar * (size_t)scloudplus_n);
    scloudplus_avx2_a_state_init(&st, seedA);
    for (int i = 0; i < scloudplus_m; i += 8) {
        scloudplus_avx2_generate_a_rows8(&st, (size_t)i, packed_rows, a_rows);
        int p = 0;
        for (; p + 1 < scloudplus_mbar; p += 2) {
            uint16_t coeffs0[8];
            uint16_t coeffs1[8];
            uint16_t *crow0 = C + (size_t)p * (size_t)scloudplus_n;
            uint16_t *crow1 = crow0 + (size_t)scloudplus_n;

            for (int row = 0; row < 8; row++) {
                coeffs0[row] = S[(size_t)p * (size_t)scloudplus_m + (size_t)i + (size_t)row];
                coeffs1[row] = S[(size_t)(p + 1) * (size_t)scloudplus_m + (size_t)i + (size_t)row];
            }
            scloudplus_avx2_common_mul_add_rows8_dst2(crow0, crow1, a_rows,
                                                      coeffs0, coeffs1);
        }
        if (p < scloudplus_mbar) {
            uint16_t coeffs[8];
            uint16_t *crow = C + (size_t)p * (size_t)scloudplus_n;
            for (int row = 0; row < 8; row++)
                coeffs[row] = S[(size_t)p * (size_t)scloudplus_m + (size_t)i + (size_t)row];
            scloudplus_avx2_common_mul_add_rows8(crow, a_rows, coeffs);
        }
    }
    scloudplus_avx2_a_state_clear(&st);
}

void mat_add(uint16_t *in0, uint16_t *in1, int len, uint16_t *out)
{
    int i = 0;
    for (; i + (int)SCLOUDPLUS_AVX2_LANES_U16 <= len;
         i += (int)SCLOUDPLUS_AVX2_LANES_U16) {
        const __m256i lhs = _mm256_loadu_si256((const __m256i *)(in0 + i));
        const __m256i rhs = _mm256_loadu_si256((const __m256i *)(in1 + i));
        _mm256_storeu_si256((__m256i *)(out + i),
                            scloudplus_avx2_mask_q(_mm256_add_epi16(lhs, rhs)));
    }
    for (; i < len; i++)
        out[i] = scloudplus_mod_q((uint32_t)in0[i] + (uint32_t)in1[i]);
}

void mat_sub(uint16_t *in0, uint16_t *in1, int len, uint16_t *out)
{
    int i = 0;
    for (; i + (int)SCLOUDPLUS_AVX2_LANES_U16 <= len;
         i += (int)SCLOUDPLUS_AVX2_LANES_U16) {
        const __m256i lhs = _mm256_loadu_si256((const __m256i *)(in0 + i));
        const __m256i rhs = _mm256_loadu_si256((const __m256i *)(in1 + i));
        _mm256_storeu_si256((__m256i *)(out + i),
                            scloudplus_avx2_mask_q(_mm256_sub_epi16(lhs, rhs)));
    }
    for (; i < len; i++)
        out[i] = scloudplus_mod_q((uint32_t)in0[i] - (uint32_t)in1[i]);
}

void mul_cs(uint16_t *C, uint16_t *S, uint16_t *out)
{
    const int padded_nbar = (scloudplus_nbar + 15) & ~15;
    uint16_t st[(size_t)scloudplus_n * (size_t)padded_nbar];

    for (int k = 0; k < scloudplus_n; k++)
    {
        for (int j = 0; j < scloudplus_nbar; j++)
            st[(size_t)k * (size_t)padded_nbar + (size_t)j] =
                S[(size_t)j * (size_t)scloudplus_n + (size_t)k];
        for (int j = scloudplus_nbar; j < padded_nbar; j++)
            st[(size_t)k * (size_t)padded_nbar + (size_t)j] = 0;
    }

    for (int i = 0; i < scloudplus_mbar; i++) {
        for (int col = 0; col < padded_nbar; col += 16) {
            __m256i acc = _mm256_setzero_si256();
            for (int k = 0; k < scloudplus_n; k++) {
                const __m256i c = _mm256_set1_epi16(
                    (short)C[(size_t)i * (size_t)scloudplus_n + (size_t)k]);
                const __m256i s = _mm256_loadu_si256(
                    (const __m256i *)(st + (size_t)k *
                                            (size_t)padded_nbar +
                                      (size_t)col));
                acc = _mm256_add_epi16(acc, _mm256_mullo_epi16(c, s));
            }
            if (col + 16 <= scloudplus_nbar) {
                _mm256_storeu_si256(
                    (__m256i *)(out + (size_t)i * (size_t)scloudplus_nbar +
                                (size_t)col),
                    acc);
            } else {
                uint16_t tmp[16];
                _mm256_storeu_si256((__m256i *)tmp, acc);
                for (int j = col; j < scloudplus_nbar; j++)
                    out[(size_t)i * (size_t)scloudplus_nbar + (size_t)j] =
                        tmp[j - col];
            }
        }
    }
}

void mul_sb_e(const uint16_t *S, const uint16_t *B,
              const uint16_t *E, uint16_t *out)
{
    if (out != E)
        memcpy(out, E, (size_t)scloudplus_mbar * (size_t)scloudplus_nbar * sizeof(uint16_t));

    for (int i = 0; i < scloudplus_mbar; i++) {
        const uint16_t *S_row = S + (size_t)i * (size_t)scloudplus_m;
        uint16_t *out_row = out + (size_t)i * (size_t)scloudplus_nbar;
        int col = 0;
        for (; col + 16 <= scloudplus_nbar; col += 16) {
            __m256i acc = _mm256_loadu_si256((const __m256i *)(out_row + col));
            for (int k = 0; k < scloudplus_m; k++) {
                const __m256i s = _mm256_set1_epi16((short)S_row[k]);
                const __m256i b = _mm256_loadu_si256(
                    (const __m256i *)(B + (size_t)k *
                                          (size_t)scloudplus_nbar +
                                      (size_t)col));
                acc = _mm256_add_epi16(acc, _mm256_mullo_epi16(s, b));
            }
            _mm256_storeu_si256((__m256i *)(out_row + col), acc);
        }
        for (; col + 8 <= scloudplus_nbar; col += 8) {
            __m128i acc = _mm_loadu_si128((const __m128i *)(out_row + col));
            for (int k = 0; k < scloudplus_m; k++) {
                const __m128i s = _mm_set1_epi16((short)S_row[k]);
                const __m128i b = _mm_loadu_si128(
                    (const __m128i *)(B + (size_t)k *
                                          (size_t)scloudplus_nbar +
                                      (size_t)col));
                acc = _mm_add_epi16(acc, _mm_mullo_epi16(s, b));
            }
            _mm_storeu_si128((__m128i *)(out_row + col), acc);
        }
        for (; col < scloudplus_nbar; col++) {
            uint16_t acc = out_row[col];
            for (int k = 0; k < scloudplus_m; k++)
                acc = (uint16_t)(
                    acc + S_row[k] *
                          B[(size_t)k * (size_t)scloudplus_nbar +
                            (size_t)col]);
            out_row[col] = acc;
        }
    }
}

#endif
