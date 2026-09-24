#include <stdlib.h>
#include <string.h>
#ifdef FROST_USE_E8_CODE
#include "frost_e8.h"
#endif
#include "frost_u32_core.h"
#include "frost_macrify.h"
#include "../../common/sha3/fips202.h"
#if defined(USE_AVX2_U32)
#include "../../common/sha3/fips202x4.h"
#endif
#include <stdio.h>
#if defined(USE_AVX2_U32)
#include <immintrin.h>
#endif


const char *frost_message_codec_name(void)
{
#ifdef FROST_USE_E8_CODE
    return "e8";
#else
    return "scalar";
#endif
}

#ifdef FROST_CODEC_TRACE
volatile unsigned long frost_e8_encode_call_count = 0;
volatile unsigned long frost_e8_decode_call_count = 0;

void frost_codec_trace_reset(void)
{
    frost_e8_encode_call_count = 0;
    frost_e8_decode_call_count = 0;
}

unsigned long frost_codec_trace_e8_encode_calls(void)
{
    return frost_e8_encode_call_count;
}

unsigned long frost_codec_trace_e8_decode_calls(void)
{
    return frost_e8_decode_call_count;
}
#endif

static inline uint32_t qmask_mul_u32(void) { return (1u << PARAMS_LOGQ) - 1u; }
static int u32_profile_enabled_mul(void)
{
    const char *p = getenv("PROFILE_U32");
    const char *q = getenv("PROFILE_U32_AVX2");
    return ((p != NULL && strcmp(p, "1") == 0) || (q != NULL && strcmp(q, "1") == 0));
}
#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
static inline unsigned long long mul_now_cycles(void) { return __rdtsc(); }
#else
#include <time.h>
static inline unsigned long long mul_now_cycles(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec * 1000000000ull + (unsigned long long)ts.tv_nsec;
}
#endif

#ifndef FROST_U32_A_WORD_BYTES
#define FROST_U32_A_WORD_BYTES 3
#endif
#if (FROST_U32_A_WORD_BYTES != 3) && (FROST_U32_A_WORD_BYTES != 4)
#error "FROST_U32_A_WORD_BYTES must be 3 or 4"
#endif
#ifndef FROST_U32_A_PARSE_STYLE
#define FROST_U32_A_PARSE_STYLE 0
#endif
#ifndef FROST_U32_MUL_BS_PREPACK
#define FROST_U32_MUL_BS_PREPACK 0
#endif
#ifndef FROST_U32_J_BLOCK
#define FROST_U32_J_BLOCK 2
#endif
#define A_ROW_BYTES ((size_t)PARAMS_N * FROST_U32_A_WORD_BYTES)
static frost_u32_fast_stats_t g_u32_fast_stats = {0};
static size_t g_active_batch_rows = 1;

void frost_u32_fast_stats_reset(void)
{
    memset(&g_u32_fast_stats, 0, sizeof(g_u32_fast_stats));
}

frost_u32_fast_stats_t frost_u32_fast_stats_get(void)
{
    return g_u32_fast_stats;
}

static void expand_a_row_from_bytes(uint32_t *row, const uint8_t *row_bytes)
{
#if FROST_U32_A_WORD_BYTES == 3
#if FROST_U32_A_PARSE_STYLE == 1
    size_t j = 0;
    size_t p = 0;
    for (; j + 3 < (size_t)PARAMS_N; j += 4, p += 12) {
        uint32_t v0 = (uint32_t)row_bytes[p] | ((uint32_t)row_bytes[p + 1] << 8) | ((uint32_t)row_bytes[p + 2] << 16);
        uint32_t v1 = (uint32_t)row_bytes[p + 3] | ((uint32_t)row_bytes[p + 4] << 8) | ((uint32_t)row_bytes[p + 5] << 16);
        uint32_t v2 = (uint32_t)row_bytes[p + 6] | ((uint32_t)row_bytes[p + 7] << 8) | ((uint32_t)row_bytes[p + 8] << 16);
        uint32_t v3 = (uint32_t)row_bytes[p + 9] | ((uint32_t)row_bytes[p + 10] << 8) | ((uint32_t)row_bytes[p + 11] << 16);
        row[j] = v0 & qmask_mul_u32();
        row[j + 1] = v1 & qmask_mul_u32();
        row[j + 2] = v2 & qmask_mul_u32();
        row[j + 3] = v3 & qmask_mul_u32();
    }
    for (; j < (size_t)PARAMS_N; j++, p += 3) {
        uint32_t v = (uint32_t)row_bytes[p] | ((uint32_t)row_bytes[p + 1] << 8) | ((uint32_t)row_bytes[p + 2] << 16);
        row[j] = v & qmask_mul_u32();
    }
#else
    for (size_t j = 0; j < PARAMS_N; j++) {
        const size_t p = 3 * j;
        uint32_t v = (uint32_t)row_bytes[p] | ((uint32_t)row_bytes[p + 1] << 8) | ((uint32_t)row_bytes[p + 2] << 16);
        row[j] = v & qmask_mul_u32();
    }
#endif
#else
    for (size_t j = 0; j < PARAMS_N; j++) {
        uint32_t v = (uint32_t)row_bytes[4*j] | ((uint32_t)row_bytes[4*j+1] << 8) | ((uint32_t)row_bytes[4*j+2] << 16) | ((uint32_t)row_bytes[4*j+3] << 24);
        row[j] = v & qmask_mul_u32();
    }
#endif
}

static inline void u32_stats_set_modes(void)
{
    g_u32_fast_stats.coeff_parse_mode = (uint64_t)FROST_U32_A_WORD_BYTES;
    g_u32_fast_stats.a_rows_per_shake_batch = (uint64_t)g_active_batch_rows;
}

static inline void expandA_row_u32_fast(uint32_t *row, uint8_t *row_bytes, uint16_t row_idx, const uint8_t *seed_A)
{
    uint8_t in[2 + BYTES_SEED_A];
    memcpy(&in[2], seed_A, BYTES_SEED_A);
    in[0] = (uint8_t)(row_idx & 0xFF);
    in[1] = (uint8_t)((row_idx >> 8) & 0xFF);
    shake128(row_bytes, (unsigned long long)A_ROW_BYTES, in, sizeof(in));
    expand_a_row_from_bytes(row, row_bytes);
    g_u32_fast_stats.expand_row_calls += 1;
    g_u32_fast_stats.shake_init_calls += 1;
    g_u32_fast_stats.shake_squeeze_calls += 1;
    g_u32_fast_stats.bytes_squeezed_for_a += A_ROW_BYTES;
}

static inline void expandA_rows_u32_fast(uint32_t *rows, uint8_t *row_bytes, uint16_t row_idx, size_t count, const uint8_t *seed_A)
{
#if defined(USE_AVX2_U32)
    while (count >= 4) {
        uint8_t in0[2 + BYTES_SEED_A], in1[2 + BYTES_SEED_A], in2[2 + BYTES_SEED_A], in3[2 + BYTES_SEED_A];
        memcpy(&in0[2], seed_A, BYTES_SEED_A);
        memcpy(&in1[2], seed_A, BYTES_SEED_A);
        memcpy(&in2[2], seed_A, BYTES_SEED_A);
        memcpy(&in3[2], seed_A, BYTES_SEED_A);
        in0[0] = (uint8_t)(row_idx & 0xFF);         in0[1] = (uint8_t)((row_idx >> 8) & 0xFF);
        in1[0] = (uint8_t)((row_idx + 1) & 0xFF);   in1[1] = (uint8_t)(((row_idx + 1) >> 8) & 0xFF);
        in2[0] = (uint8_t)((row_idx + 2) & 0xFF);   in2[1] = (uint8_t)(((row_idx + 2) >> 8) & 0xFF);
        in3[0] = (uint8_t)((row_idx + 3) & 0xFF);   in3[1] = (uint8_t)(((row_idx + 3) >> 8) & 0xFF);
        shake128_4x(row_bytes,
                    row_bytes + A_ROW_BYTES,
                    row_bytes + 2 * A_ROW_BYTES,
                    row_bytes + 3 * A_ROW_BYTES,
                    (unsigned long long)A_ROW_BYTES,
                    in0, in1, in2, in3, sizeof(in0));
        for (size_t r = 0; r < 4; r++) {
            expand_a_row_from_bytes(rows + r * (size_t)PARAMS_N, row_bytes + r * A_ROW_BYTES);
        }
        g_u32_fast_stats.expand_row_calls += 4;
        g_u32_fast_stats.shake_init_calls += 1;
        g_u32_fast_stats.shake_squeeze_calls += 1;
        g_u32_fast_stats.bytes_squeezed_for_a += 4 * A_ROW_BYTES;
        g_u32_fast_stats.shake4x_used = 1;
        rows += 4 * (size_t)PARAMS_N;
        row_bytes += 4 * A_ROW_BYTES;
        row_idx = (uint16_t)(row_idx + 4);
        count -= 4;
    }
#endif
    for (size_t r = 0; r < count; r++) {
        expandA_row_u32_fast(rows + r * (size_t)PARAMS_N, row_bytes + r * A_ROW_BYTES, (uint16_t)(row_idx + r), seed_A);
    }
}

static inline void expandA_rows_u32_block_transposed(uint32_t *ablock, uint8_t *row_bytes, uint16_t row_idx, size_t count, const uint8_t *seed_A)
{
#if defined(USE_AVX2_U32)
    size_t gen = 0;
    while (gen + 4 <= count) {
        uint8_t in0[2 + BYTES_SEED_A], in1[2 + BYTES_SEED_A], in2[2 + BYTES_SEED_A], in3[2 + BYTES_SEED_A];
        memcpy(&in0[2], seed_A, BYTES_SEED_A);
        memcpy(&in1[2], seed_A, BYTES_SEED_A);
        memcpy(&in2[2], seed_A, BYTES_SEED_A);
        memcpy(&in3[2], seed_A, BYTES_SEED_A);
        in0[0] = (uint8_t)((row_idx + gen + 0) & 0xFF); in0[1] = (uint8_t)(((row_idx + gen + 0) >> 8) & 0xFF);
        in1[0] = (uint8_t)((row_idx + gen + 1) & 0xFF); in1[1] = (uint8_t)(((row_idx + gen + 1) >> 8) & 0xFF);
        in2[0] = (uint8_t)((row_idx + gen + 2) & 0xFF); in2[1] = (uint8_t)(((row_idx + gen + 2) >> 8) & 0xFF);
        in3[0] = (uint8_t)((row_idx + gen + 3) & 0xFF); in3[1] = (uint8_t)(((row_idx + gen + 3) >> 8) & 0xFF);
        shake128_4x(row_bytes + (gen + 0) * A_ROW_BYTES,
                    row_bytes + (gen + 1) * A_ROW_BYTES,
                    row_bytes + (gen + 2) * A_ROW_BYTES,
                    row_bytes + (gen + 3) * A_ROW_BYTES,
                    (unsigned long long)A_ROW_BYTES,
                    in0, in1, in2, in3, sizeof(in0));
        g_u32_fast_stats.expand_row_calls += 4;
        g_u32_fast_stats.shake_init_calls += 1;
        g_u32_fast_stats.shake_squeeze_calls += 1;
        g_u32_fast_stats.bytes_squeezed_for_a += 4 * A_ROW_BYTES;
        g_u32_fast_stats.shake4x_used = 1;
        gen += 4;
    }
    for (; gen < count; gen++) {
        uint8_t in[2 + BYTES_SEED_A];
        memcpy(&in[2], seed_A, BYTES_SEED_A);
        in[0] = (uint8_t)((row_idx + gen) & 0xFF);
        in[1] = (uint8_t)(((row_idx + gen) >> 8) & 0xFF);
        shake128(row_bytes + gen * A_ROW_BYTES, (unsigned long long)A_ROW_BYTES, in, sizeof(in));
        g_u32_fast_stats.expand_row_calls += 1;
        g_u32_fast_stats.shake_init_calls += 1;
        g_u32_fast_stats.shake_squeeze_calls += 1;
        g_u32_fast_stats.bytes_squeezed_for_a += A_ROW_BYTES;
    }
#else
    for (size_t gen = 0; gen < count; gen++) {
        uint8_t in[2 + BYTES_SEED_A];
        memcpy(&in[2], seed_A, BYTES_SEED_A);
        in[0] = (uint8_t)((row_idx + gen) & 0xFF);
        in[1] = (uint8_t)(((row_idx + gen) >> 8) & 0xFF);
        shake128(row_bytes + gen * A_ROW_BYTES, (unsigned long long)A_ROW_BYTES, in, sizeof(in));
        g_u32_fast_stats.expand_row_calls += 1;
        g_u32_fast_stats.shake_init_calls += 1;
        g_u32_fast_stats.shake_squeeze_calls += 1;
        g_u32_fast_stats.bytes_squeezed_for_a += A_ROW_BYTES;
    }
#endif

    for (size_t j = 0; j < (size_t)PARAMS_N; j++) {
        for (size_t b = 0; b < count; b++) {
#if FROST_U32_A_WORD_BYTES == 3
            const uint8_t *p = row_bytes + b * A_ROW_BYTES + 3 * j;
            uint32_t v = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
#else
            const uint8_t *p = row_bytes + b * A_ROW_BYTES + 4 * j;
            uint32_t v = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
#endif
            ablock[j * count + b] = v & qmask_mul_u32();
        }
    }
}

static inline size_t j_block_cols_u32(void)
{
    const char *p = getenv("FROST_U32_J_BLOCK");
    if (p == NULL || *p == '\0') return FROST_U32_J_BLOCK;
    long v = strtol(p, NULL, 10);
    if (v == 1 || v == 2 || v == 4) return (size_t)v;
    return FROST_U32_J_BLOCK;
}

#if defined(USE_AVX2_U32)
static inline int64_t dot_u32_i32_avx2(const uint32_t *a, const int32_t *b)
{
    __m256i acc = _mm256_setzero_si256();
    size_t j = 0;
    for (; j + 8 <= (size_t)PARAMS_N; j += 8) {
        __m256i va = _mm256_loadu_si256((const __m256i *)(a + j));
        __m256i vb = _mm256_loadu_si256((const __m256i *)(b + j));
        __m256i prod_even = _mm256_mul_epi32(va, vb);
        __m256i va_odd = _mm256_srli_epi64(va, 32);
        __m256i vb_odd = _mm256_srli_epi64(vb, 32);
        __m256i prod_odd = _mm256_mul_epi32(va_odd, vb_odd);
        acc = _mm256_add_epi64(acc, prod_even);
        acc = _mm256_add_epi64(acc, prod_odd);
    }
    __m128i lo = _mm256_castsi256_si128(acc);
    __m128i hi = _mm256_extracti128_si256(acc, 1);
    __m128i s = _mm_add_epi64(lo, hi);
    s = _mm_add_epi64(s, _mm_unpackhi_epi64(s, s));
    int64_t sum = (int64_t)_mm_cvtsi128_si64(s);
    for (; j < (size_t)PARAMS_N; j++) {
        sum += (int64_t)a[j] * (int64_t)b[j];
    }
    return sum;
}
#endif

static int mul_A_times_S_u32_fast(uint32_t *out, const int32_t *s, const uint32_t *e, const uint8_t *seed_A, frost_u32_workspace_t *ws)
{
    unsigned long long c0 = mul_now_cycles(), c_expand = 0, c_mac = 0, c1;
    size_t batch_rows = 1;
#if defined(USE_AVX2_U32)
    batch_rows = (ws && ws->row_count > 0) ? ws->row_count : 4;
#endif
    if (batch_rows > 16) batch_rows = 16;
    g_active_batch_rows = batch_rows;
    u32_stats_set_modes();
    uint32_t *rows = (ws && ws->arow) ? ws->arow : (uint32_t *)malloc((size_t)PARAMS_N * batch_rows * sizeof(uint32_t));
    uint8_t *row_bytes = (ws && ws->arow_bytes) ? ws->arow_bytes : (uint8_t *)malloc(batch_rows * A_ROW_BYTES);
    int own = !(ws && ws->arow && ws->arow_bytes);
    if (!rows || !row_bytes) { if (own) { free(rows); free(row_bytes); } return 0; }

    for (size_t i = 0; i < (size_t)PARAMS_N; i++) {
        if ((i % batch_rows) == 0) {
            size_t cnt = ((size_t)PARAMS_N - i >= batch_rows) ? batch_rows : ((size_t)PARAMS_N - i);
            c1 = mul_now_cycles();
            expandA_rows_u32_fast(rows, row_bytes, (uint16_t)i, cnt, seed_A);
            c_expand += mul_now_cycles() - c1;
        }
        const uint32_t *row = rows + (i % batch_rows) * (size_t)PARAMS_N;
        c1 = mul_now_cycles();
        const int32_t *s0 = s + 0*(size_t)PARAMS_N;
        const int32_t *s1 = s + 1*(size_t)PARAMS_N;
        const int32_t *s2 = s + 2*(size_t)PARAMS_N;
        const int32_t *s3 = s + 3*(size_t)PARAMS_N;
        const int32_t *s4 = s + 4*(size_t)PARAMS_N;
        const int32_t *s5 = s + 5*(size_t)PARAMS_N;
        const int32_t *s6 = s + 6*(size_t)PARAMS_N;
        const int32_t *s7 = s + 7*(size_t)PARAMS_N;
        int64_t acc0 = e[i*(size_t)PARAMS_NBAR + 0];
        int64_t acc1 = e[i*(size_t)PARAMS_NBAR + 1];
        int64_t acc2 = e[i*(size_t)PARAMS_NBAR + 2];
        int64_t acc3 = e[i*(size_t)PARAMS_NBAR + 3];
        int64_t acc4 = e[i*(size_t)PARAMS_NBAR + 4];
        int64_t acc5 = e[i*(size_t)PARAMS_NBAR + 5];
        int64_t acc6 = e[i*(size_t)PARAMS_NBAR + 6];
        int64_t acc7 = e[i*(size_t)PARAMS_NBAR + 7];
#if defined(USE_AVX2_U32)
        acc0 += dot_u32_i32_avx2(row, s0);
        acc1 += dot_u32_i32_avx2(row, s1);
        acc2 += dot_u32_i32_avx2(row, s2);
        acc3 += dot_u32_i32_avx2(row, s3);
        acc4 += dot_u32_i32_avx2(row, s4);
        acc5 += dot_u32_i32_avx2(row, s5);
        acc6 += dot_u32_i32_avx2(row, s6);
        acc7 += dot_u32_i32_avx2(row, s7);
#else
        for (size_t j = 0; j < (size_t)PARAMS_N; j++) {
            int64_t a = (int64_t)row[j];
            acc0 += a * (int64_t)s0[j];
            acc1 += a * (int64_t)s1[j];
            acc2 += a * (int64_t)s2[j];
            acc3 += a * (int64_t)s3[j];
            acc4 += a * (int64_t)s4[j];
            acc5 += a * (int64_t)s5[j];
            acc6 += a * (int64_t)s6[j];
            acc7 += a * (int64_t)s7[j];
        }
#endif
        out[i*(size_t)PARAMS_NBAR + 0] = (uint32_t)acc0 & qmask_mul_u32();
        out[i*(size_t)PARAMS_NBAR + 1] = (uint32_t)acc1 & qmask_mul_u32();
        out[i*(size_t)PARAMS_NBAR + 2] = (uint32_t)acc2 & qmask_mul_u32();
        out[i*(size_t)PARAMS_NBAR + 3] = (uint32_t)acc3 & qmask_mul_u32();
        out[i*(size_t)PARAMS_NBAR + 4] = (uint32_t)acc4 & qmask_mul_u32();
        out[i*(size_t)PARAMS_NBAR + 5] = (uint32_t)acc5 & qmask_mul_u32();
        out[i*(size_t)PARAMS_NBAR + 6] = (uint32_t)acc6 & qmask_mul_u32();
        out[i*(size_t)PARAMS_NBAR + 7] = (uint32_t)acc7 & qmask_mul_u32();
        c_mac += mul_now_cycles() - c1;
    }
    g_u32_fast_stats.mac_ops += (uint64_t)PARAMS_N * (uint64_t)PARAMS_N * (uint64_t)PARAMS_NBAR;
    g_u32_fast_stats.matrix_products += 1;
    if (own) { free(rows); free(row_bytes); }
    if (u32_profile_enabled_mul()) {
        unsigned long long total = mul_now_cycles() - c0;
        double p_expand = (total == 0) ? 0.0 : (100.0 * (double)c_expand / (double)total);
        double p_mac = (total == 0) ? 0.0 : (100.0 * (double)c_mac / (double)total);
        fprintf(stderr, "[profile-u32] level=%d fn=mul_as details expandA=%llu(%.2f%%) mac=%llu(%.2f%%)\n",
                PARAMS_N, c_expand, p_expand, c_mac, p_mac);
    }
    return 1;
}

static int mul_AT_times_R_u32_fast(uint32_t *out, const int32_t *s, const uint32_t *e, const uint8_t *seed_A, frost_u32_workspace_t *ws)
{
    unsigned long long c0 = mul_now_cycles(), c_expand = 0, c_mac = 0, c_out = 0, c1;
    size_t jblk = j_block_cols_u32();
#if !defined(USE_AVX2_U32)
    jblk = 1;
#endif
    size_t batch_rows = 1;
#if defined(USE_AVX2_U32)
    batch_rows = (ws && ws->row_count > 0) ? ws->row_count : 4;
#endif
    if (batch_rows > 16) batch_rows = 16;
    g_active_batch_rows = batch_rows;
    u32_stats_set_modes();
    uint32_t *ablock = (ws && ws->arow) ? ws->arow : (uint32_t *)malloc((size_t)PARAMS_N * batch_rows * sizeof(uint32_t));
    uint8_t *row_bytes = (ws && ws->arow_bytes) ? ws->arow_bytes : (uint8_t *)malloc(batch_rows * A_ROW_BYTES);
    uint32_t *acc = (uint32_t *)malloc((size_t)PARAMS_N * PARAMS_NBAR * sizeof(uint32_t));
    int own = !(ws && ws->arow && ws->arow_bytes);
    if (!ablock || !row_bytes || !acc) { if (own) { free(ablock); free(row_bytes); } free(acc); return 0; }

    for (size_t j = 0; j < PARAMS_N; j++) {
        for (size_t k = 0; k < PARAMS_NBAR; k++) {
            acc[j*(size_t)PARAMS_NBAR + k] = e[k*(size_t)PARAMS_N + j];
        }
    }
    for (size_t i0 = 0; i0 < (size_t)PARAMS_N; i0 += batch_rows) {
        size_t cnt = ((size_t)PARAMS_N - i0 >= batch_rows) ? batch_rows : ((size_t)PARAMS_N - i0);
        c1 = mul_now_cycles();
        expandA_rows_u32_block_transposed(ablock, row_bytes, (uint16_t)i0, cnt, seed_A);
        c_expand += mul_now_cycles() - c1;
        c1 = mul_now_cycles();
#if defined(USE_AVX2_U32)
        __m256i rvec[16];
#endif
        for (size_t b = 0; b < cnt; b++) {
#if defined(USE_AVX2_U32)
            const size_t i = i0 + b;
            rvec[b] = _mm256_set_epi32(
                s[7*(size_t)PARAMS_N + i], s[6*(size_t)PARAMS_N + i], s[5*(size_t)PARAMS_N + i], s[4*(size_t)PARAMS_N + i],
                s[3*(size_t)PARAMS_N + i], s[2*(size_t)PARAMS_N + i], s[1*(size_t)PARAMS_N + i], s[0*(size_t)PARAMS_N + i]
            );
#endif
        }
        for (size_t q = 0; q < (size_t)PARAMS_N; q += jblk) {
#if defined(USE_AVX2_U32)
            size_t jb = ((size_t)PARAMS_N - q >= jblk) ? jblk : ((size_t)PARAMS_N - q);
            __m256i u0 = _mm256_loadu_si256((const __m256i *)(acc + (q + 0)*(size_t)PARAMS_NBAR));
            __m256i u1 = _mm256_setzero_si256(), u2 = _mm256_setzero_si256(), u3 = _mm256_setzero_si256();
            if (jb > 1) u1 = _mm256_loadu_si256((const __m256i *)(acc + (q + 1)*(size_t)PARAMS_NBAR));
            if (jb > 2) u2 = _mm256_loadu_si256((const __m256i *)(acc + (q + 2)*(size_t)PARAMS_NBAR));
            if (jb > 3) u3 = _mm256_loadu_si256((const __m256i *)(acc + (q + 3)*(size_t)PARAMS_NBAR));
            for (size_t b = 0; b < cnt; b++) {
                __m256i r = rvec[b];
                __m256i a0 = _mm256_set1_epi32((int32_t)ablock[(q + 0) * cnt + b]);
                u0 = _mm256_add_epi32(u0, _mm256_mullo_epi32(a0, r));
                if (jb > 1) {
                    __m256i a1 = _mm256_set1_epi32((int32_t)ablock[(q + 1) * cnt + b]);
                    u1 = _mm256_add_epi32(u1, _mm256_mullo_epi32(a1, r));
                }
                if (jb > 2) {
                    __m256i a2 = _mm256_set1_epi32((int32_t)ablock[(q + 2) * cnt + b]);
                    u2 = _mm256_add_epi32(u2, _mm256_mullo_epi32(a2, r));
                }
                if (jb > 3) {
                    __m256i a3 = _mm256_set1_epi32((int32_t)ablock[(q + 3) * cnt + b]);
                    u3 = _mm256_add_epi32(u3, _mm256_mullo_epi32(a3, r));
                }
            }
            _mm256_storeu_si256((__m256i *)(acc + (q + 0)*(size_t)PARAMS_NBAR), u0);
            if (jb > 1) _mm256_storeu_si256((__m256i *)(acc + (q + 1)*(size_t)PARAMS_NBAR), u1);
            if (jb > 2) _mm256_storeu_si256((__m256i *)(acc + (q + 2)*(size_t)PARAMS_NBAR), u2);
            if (jb > 3) _mm256_storeu_si256((__m256i *)(acc + (q + 3)*(size_t)PARAMS_NBAR), u3);
#else
            uint32_t *uq = acc + q*(size_t)PARAMS_NBAR;
            for (size_t b = 0; b < cnt; b++) {
                const size_t i = i0 + b;
                uint32_t a = ablock[q * cnt + b];
                uq[0] += a * (uint32_t)s[0*(size_t)PARAMS_N + i];
                uq[1] += a * (uint32_t)s[1*(size_t)PARAMS_N + i];
                uq[2] += a * (uint32_t)s[2*(size_t)PARAMS_N + i];
                uq[3] += a * (uint32_t)s[3*(size_t)PARAMS_N + i];
                uq[4] += a * (uint32_t)s[4*(size_t)PARAMS_N + i];
                uq[5] += a * (uint32_t)s[5*(size_t)PARAMS_N + i];
                uq[6] += a * (uint32_t)s[6*(size_t)PARAMS_N + i];
                uq[7] += a * (uint32_t)s[7*(size_t)PARAMS_N + i];
            }
#endif
        }
        c_mac += mul_now_cycles() - c1;
    }
    g_u32_fast_stats.mac_ops += (uint64_t)PARAMS_N * (uint64_t)PARAMS_N * (uint64_t)PARAMS_NBAR;
    g_u32_fast_stats.matrix_products += 1;
    c1 = mul_now_cycles();
    for (size_t k = 0; k < PARAMS_NBAR; k++) {
        for (size_t i = 0; i < PARAMS_N; i++) {
            out[k*(size_t)PARAMS_N + i] = acc[i*(size_t)PARAMS_NBAR + k] & qmask_mul_u32();
        }
    }
    c_out += mul_now_cycles() - c1;
    clear_bytes((uint8_t *)acc, (size_t)PARAMS_NBAR * PARAMS_N * sizeof(uint32_t));
    if (own) { free(ablock); free(row_bytes); }
    free(acc);
    if (u32_profile_enabled_mul()) {
        unsigned long long total = mul_now_cycles() - c0;
        double p_expand = (total == 0) ? 0.0 : (100.0 * (double)c_expand / (double)total);
        double p_mac = (total == 0) ? 0.0 : (100.0 * (double)c_mac / (double)total);
        double p_out = (total == 0) ? 0.0 : (100.0 * (double)c_out / (double)total);
        fprintf(stderr, "[profile-u32] level=%d fn=mul_sa details expandA=%llu(%.2f%%) mac=%llu(%.2f%%) finalize=%llu(%.2f%%)\n",
                PARAMS_N, c_expand, p_expand, c_mac, p_mac, c_out, p_out);
    }
    return 1;
}

int frost_mul_add_as_plus_e_u32(uint32_t *out, const int32_t *s, const uint32_t *e, const uint8_t *seed_A, frost_u32_workspace_t *ws)
{
    return mul_A_times_S_u32_fast(out, s, e, seed_A, ws);
}

int frost_mul_add_sa_plus_e_u32(uint32_t *out, const int32_t *s, const uint32_t *e, const uint8_t *seed_A, frost_u32_workspace_t *ws)
{
    return mul_AT_times_R_u32_fast(out, s, e, seed_A, ws);
}

void frost_mul_bs_u32(uint32_t *out, const uint32_t *b, const int32_t *s)
{
#if defined(USE_AVX2_U32) && (FROST_U32_MUL_BS_PREPACK == 1)
    int32_t *spack = (int32_t *)malloc((size_t)PARAMS_N * PARAMS_NBAR * sizeof(int32_t));
    if (spack != NULL) {
        for (size_t k = 0; k < (size_t)PARAMS_N; k++) {
            for (size_t j = 0; j < PARAMS_NBAR; j++) {
                spack[k*(size_t)PARAMS_NBAR + j] = s[j*(size_t)PARAMS_N + k];
            }
        }
        for (size_t i = 0; i < PARAMS_NBAR; i++) {
            __m256i acc = _mm256_setzero_si256();
            for (size_t k = 0; k < (size_t)PARAMS_N; k++) {
                __m256i svec = _mm256_loadu_si256((const __m256i *)(spack + k*(size_t)PARAMS_NBAR));
                __m256i bvec = _mm256_set1_epi32((int32_t)b[i*(size_t)PARAMS_N + k]);
                acc = _mm256_add_epi32(acc, _mm256_mullo_epi32(bvec, svec));
            }
            _mm256_storeu_si256((__m256i *)(out + i*(size_t)PARAMS_NBAR), acc);
        }
        clear_bytes((uint8_t *)spack, (size_t)PARAMS_N * PARAMS_NBAR * sizeof(int32_t));
        free(spack);
    } else
#endif
    for (size_t i = 0; i < PARAMS_NBAR; i++) {
#if defined(USE_AVX2_U32)
        __m256i acc = _mm256_setzero_si256();
        for (size_t k = 0; k < (size_t)PARAMS_N; k++) {
            __m256i svec = _mm256_set_epi32(
                s[7*(size_t)PARAMS_N + k], s[6*(size_t)PARAMS_N + k], s[5*(size_t)PARAMS_N + k], s[4*(size_t)PARAMS_N + k],
                s[3*(size_t)PARAMS_N + k], s[2*(size_t)PARAMS_N + k], s[1*(size_t)PARAMS_N + k], s[0*(size_t)PARAMS_N + k]
            );
            __m256i bvec = _mm256_set1_epi32((int32_t)b[i*(size_t)PARAMS_N + k]);
            acc = _mm256_add_epi32(acc, _mm256_mullo_epi32(bvec, svec));
        }
        _mm256_storeu_si256((__m256i *)(out + i*(size_t)PARAMS_NBAR), acc);
#else
        for (size_t j = 0; j < PARAMS_NBAR; j++) {
            uint32_t sum = 0;
            for (size_t k = 0; k < (size_t)PARAMS_N; k++) {
                sum += b[i*(size_t)PARAMS_N + k] * (uint32_t)s[j*(size_t)PARAMS_N + k];
            }
            out[i*(size_t)PARAMS_NBAR + j] = sum;
        }
#endif
    }
    for (size_t i = 0; i < PARAMS_NBAR * PARAMS_NBAR; i++) out[i] &= qmask_mul_u32();
}

void frost_mul_add_sb_plus_e_u32(uint32_t *out, const uint32_t *b, const int32_t *s, const uint32_t *e)
{
    for (size_t k = 0; k < PARAMS_NBAR; k++) {
        uint32_t *ok = out + k*(size_t)PARAMS_NBAR;
#if defined(USE_AVX2_U32)
        __m256i acc = _mm256_loadu_si256((const __m256i *)(e + k*(size_t)PARAMS_NBAR));
        for (size_t j = 0; j < (size_t)PARAMS_N; j++) {
            __m256i svec = _mm256_set1_epi32(s[k*(size_t)PARAMS_N + j]);
            __m256i bvec = _mm256_loadu_si256((const __m256i *)(b + j*(size_t)PARAMS_NBAR));
            acc = _mm256_add_epi32(acc, _mm256_mullo_epi32(svec, bvec));
        }
        _mm256_storeu_si256((__m256i *)ok, acc);
#else
        for (size_t i = 0; i < PARAMS_NBAR; i++) {
            uint32_t sum = e[k*(size_t)PARAMS_NBAR + i];
            for (size_t j = 0; j < (size_t)PARAMS_N; j++) {
                sum += (uint32_t)s[k*(size_t)PARAMS_N + j] * b[j*(size_t)PARAMS_NBAR + i];
            }
            ok[i] = sum;
        }
#endif
    }
    for (size_t i = 0; i < PARAMS_NBAR * PARAMS_NBAR; i++) {
        out[i] &= qmask_mul_u32();
    }
}

void frost_pack_u32(unsigned char *out, size_t outlen, const uint32_t *in, size_t inlen, unsigned bits)
{
    memset(out, 0, outlen);
    uint64_t bitbuf = 0;
    unsigned bitcnt = 0;
    size_t pos = 0;
    uint64_t mask = (bits == 32) ? 0xFFFFFFFFull : ((1ull << bits) - 1ull);
    for (size_t i = 0; i < inlen; i++) {
        bitbuf |= ((uint64_t)in[i] & mask) << bitcnt;
        bitcnt += bits;
        while (bitcnt >= 8) {
            if (pos >= outlen) return;
            out[pos++] = (uint8_t)(bitbuf & 0xFFu);
            bitbuf >>= 8;
            bitcnt -= 8;
        }
    }
    if (bitcnt > 0 && pos < outlen) {
        out[pos] = (uint8_t)(bitbuf & 0xFFu);
    }
}

void frost_unpack_u32(uint32_t *out, size_t outlen, const unsigned char *in, size_t inlen, unsigned bits)
{
    memset(out, 0, outlen * sizeof(uint32_t));
    uint64_t bitbuf = 0;
    unsigned bitcnt = 0;
    size_t pos = 0;
    uint64_t mask = (bits == 32) ? 0xFFFFFFFFull : ((1ull << bits) - 1ull);
    for (size_t i = 0; i < outlen; i++) {
        while (bitcnt < bits) {
            if (pos >= inlen) { out[i] = (uint32_t)(bitbuf & mask); return; }
            bitbuf |= ((uint64_t)in[pos++]) << bitcnt;
            bitcnt += 8;
        }
        out[i] = (uint32_t)(bitbuf & mask);
        bitbuf >>= bits;
        bitcnt -= bits;
    }
}

void frost_key_encode_u32(uint32_t *out, const uint8_t *in)
{
#ifdef FROST_USE_E8_CODE
    frost_e8_encode_u32(out, in);
#else
    const uint32_t step = 1u << (PARAMS_LOGQ - PARAMS_EXTRACTED_BITS);
    size_t bitpos = 0;
    for (size_t i = 0; i < PARAMS_NBAR * PARAMS_NBAR; i++) {
        uint32_t v = 0;
        for (unsigned b = 0; b < PARAMS_EXTRACTED_BITS; b++) {
            v |= ((uint32_t)((in[bitpos >> 3] >> (bitpos & 7)) & 1u)) << b;
            bitpos++;
        }
        out[i] = (v * step) & qmask_u32();
    }
#endif
}

void frost_key_decode_u32(uint8_t *out, const uint32_t *in)
{
#ifdef FROST_USE_E8_CODE
    frost_e8_decode_u32(out, in);
#else
    memset(out, 0, BYTES_MU);
    size_t bitpos = 0;
    for (size_t i = 0; i < PARAMS_NBAR * PARAMS_NBAR; i++) {
        uint32_t x = in[i] & qmask_u32();
        uint32_t v = (x + (1u << (PARAMS_LOGQ - PARAMS_EXTRACTED_BITS - 1))) >> (PARAMS_LOGQ - PARAMS_EXTRACTED_BITS);
        v &= (1u << PARAMS_EXTRACTED_BITS) - 1u;
        for (unsigned b = 0; b < PARAMS_EXTRACTED_BITS; b++) {
            out[bitpos >> 3] |= (uint8_t)(((v >> b) & 1u) << (bitpos & 7));
            bitpos++;
        }
    }
#endif
}
