/**
 * @file matrix_neon.c
 * @brief AArch64 NEON matrix backend for AES/SHAKE/SM3 Scloud+ instances.
 *
 * This backend keeps the optimized family-specific kernels in one source file
 * so reviewers do not need to chase per-family files.  The build defines
 * exactly one matrix-family macro, so the compiler keeps only the matching
 * block.
 */

/* NEON public-matrix generation and products. */
#include "matrix_neon_internal.h"


#if defined(SCLOUDPLUS_NEON_FAMILY_AES)
#include "aes.h"
#endif
#if defined(SCLOUDPLUS_NEON_FAMILY_SHAKE) || defined(SCLOUDPLUS_NEON_FAMILY_SM3)
#include "hash.h"
#endif

/*
 * Public-key encryption needs C1 = S' * A + E1 for every NEON family and
 * public-matrix encoding.  The A byte stream is family/mode-specific, but once
 * eight A rows have been materialized as uint16_t coefficients, the multiply is
 * identical.  Keep that multiply in one implementation and restrict compile-time
 * selection to this A-row generator.
 */
typedef struct {
#if defined(SCLOUDPLUS_NEON_FAMILY_AES)
    uint8_t schedule[16 * 11];
#else
    uint8_t seed[scloudplus_seedA_bytes];
#endif
} neon_a_state;

static inline void neon_a_state_init(neon_a_state *st, const uint8_t *seedA)
{
#if defined(SCLOUDPLUS_NEON_FAMILY_AES)
    AES128_load_schedule(seedA, st->schedule);
#else
    memcpy(st->seed, seedA, scloudplus_seedA_bytes);
#endif
}

static inline void neon_a_state_clear(neon_a_state *st)
{
#if defined(SCLOUDPLUS_NEON_FAMILY_AES)
    AES128_free_schedule(st->schedule);
#else
    memset(st, 0, sizeof(*st));
#endif
}

static inline void neon_generate_a_packed_rows8(neon_a_state *st, size_t row_start,
                                                uint8_t *packed_rows)
{
#if defined(SCLOUDPLUS_NEON_FAMILY_AES)
    const uint32_t counter_start =
        (uint32_t)(row_start * (size_t)scloudplus_a_blocks_per_row);
    AES128_CTR_zero_sch(counter_start,
                        8U * (size_t)scloudplus_a_blocks_per_row,
                        st->schedule, packed_rows);
    memset(packed_rows + 8U * (size_t)scloudplus_a_padded_bytes_per_row,
           0, 16U);
#elif defined(SCLOUDPLUS_NEON_FAMILY_SHAKE)
    uint8_t in[A_INPUT_BYTES];
    memcpy(in, st->seed, scloudplus_seedA_bytes);
    for (size_t row = 0U; row < 8U; row++) {
        set_row_input_counter(in, (uint32_t)(row_start + row));
        shake128(packed_rows + row * (size_t)scloudplus_a_padded_bytes_per_row,
                 (size_t)scloudplus_a_bytes_per_row, in, sizeof(in));
    }
#elif defined(SCLOUDPLUS_NEON_FAMILY_SM3)
    uint8_t *row_ptrs[8];
    for (size_t row = 0U; row < 8U; row++)
        row_ptrs[row] = packed_rows +
                        row * (size_t)scloudplus_a_padded_bytes_per_row;
    sm3_bounded_xof8_a_rows(row_ptrs, (size_t)scloudplus_a_bytes_per_row,
                            st->seed, (uint32_t)row_start);
#else
#error "Unsupported NEON A-row generator selection"
#endif
}

static inline void neon_unpack_a_rows8(const uint8_t *packed_rows,
                                       uint16_t *out_rows)
{
    for (size_t row = 0U; row < 8U; row++) {
        const uint8_t *src = packed_rows + row * (size_t)scloudplus_a_padded_bytes_per_row;
        uint16_t *dst = out_rows + row * (size_t)scloudplus_n;
        for (size_t in_idx = 0U, out_idx = 0U; out_idx < (size_t)scloudplus_n;
             in_idx += 20U, out_idx += 16U)
            unpack10_x16(src + in_idx, dst + out_idx);
    }
}

static inline void neon_generate_a_rows8(neon_a_state *st, size_t row_start,
                                         uint8_t *packed_rows,
                                         uint16_t *out_rows)
{
    neon_generate_a_packed_rows8(st, row_start, packed_rows);
    neon_unpack_a_rows8(packed_rows, out_rows);
}

void mul_sa_e(const uint8_t *seedA, const uint16_t *S, uint16_t *E, uint16_t *C)
{
    ALIGN_HEADER(32)
    uint16_t a_rows[8U * (size_t)scloudplus_n] ALIGN_FOOTER(32);
    ALIGN_HEADER(32)
    uint8_t packed_rows[8U * (size_t)scloudplus_a_padded_bytes_per_row + 16U] ALIGN_FOOTER(32);
    neon_a_state st;

    if (C != E)
        memcpy(C, E, 2U * (size_t)scloudplus_mbar * (size_t)scloudplus_n);
    neon_a_state_init(&st, seedA);
    for (int i = 0; i < scloudplus_m; i += 8) {
        neon_generate_a_rows8(&st, (size_t)i, packed_rows, a_rows);
        int p = 0;
        for (; p + 1 < scloudplus_mbar; p += 2) {
            uint16_t coeffs0[8], coeffs1[8];
            uint16_t *crow0 = C + (size_t)(p + 0) * (size_t)scloudplus_n;
            uint16_t *crow1 = C + (size_t)(p + 1) * (size_t)scloudplus_n;
            for (int row = 0; row < 8; row++) {
                coeffs0[row] = S[(size_t)(p + 0) * (size_t)scloudplus_m + i + row];
                coeffs1[row] = S[(size_t)(p + 1) * (size_t)scloudplus_m + i + row];
            }
            neon_mul_add_rows8_2x_u16(crow0, crow1, a_rows, coeffs0, coeffs1);
        }
        for (; p < scloudplus_mbar; p++) {
            uint16_t coeffs[8];
            uint16_t *crow = C + (size_t)p * (size_t)scloudplus_n;
            for (int row = 0; row < 8; row++)
                coeffs[row] = S[(size_t)p * (size_t)scloudplus_m + i + row];
            neon_mul_add_rows8_u16(crow, a_rows, coeffs);
        }
    }
    neon_a_state_clear(&st);
}

void mul_as_e(const uint8_t *seedA, const uint16_t *S,
              const uint16_t *E, uint16_t *B)
{
    ALIGN_HEADER(32)
    uint16_t a_rows[8U * (size_t)scloudplus_n] ALIGN_FOOTER(32);
    ALIGN_HEADER(32)
    uint8_t packed_rows[8U * (size_t)scloudplus_a_padded_bytes_per_row + 16U] ALIGN_FOOTER(32);
    neon_a_state st;

    neon_a_state_init(&st, seedA);

    ALIGN_HEADER(32)
    uint16_t st_t[(size_t)scloudplus_n * (size_t)scloudplus_nbar] ALIGN_FOOTER(32);
    for (int j = 0; j < scloudplus_n; j++) {
        uint16_t *sj = st_t + (size_t)j * (size_t)scloudplus_nbar;
        for (int k = 0; k < scloudplus_nbar; k++)
            sj[k] = S[(size_t)k * (size_t)scloudplus_n + (size_t)j];
    }

    if (B != E)
        memcpy(B, E, 2U * (size_t)scloudplus_m * (size_t)scloudplus_nbar);
    for (int i = 0; i < scloudplus_m; i += 8) {
        neon_generate_a_rows8(&st, (size_t)i, packed_rows, a_rows);

        int col = 0;
        for (; col + 8 <= scloudplus_nbar; col += 8) {
            uint16x8_t acc[8];
            for (int row = 0; row < 8; row++)
                acc[row] = vld1q_u16(B + (size_t)(i + row) *
                                           (size_t)scloudplus_nbar +
                                       (size_t)col);
            for (int j = 0; j < scloudplus_n; j++) {
                const uint16x8_t s_vec = vld1q_u16(
                    st_t + (size_t)j * (size_t)scloudplus_nbar + (size_t)col);
                for (int row = 0; row < 8; row++) {
                    const uint16x8_t a = vdupq_n_u16(
                        a_rows[(size_t)row * (size_t)scloudplus_n + (size_t)j]);
                    acc[row] = vaddq_u16(acc[row], vmulq_u16(a, s_vec));
                }
            }
            for (int row = 0; row < 8; row++)
                vst1q_u16(B + (size_t)(i + row) * (size_t)scloudplus_nbar +
                              (size_t)col,
                          acc[row]);
        }

        for (; col < scloudplus_nbar; col++) {
            const uint16_t *sp = S + (size_t)col * (size_t)scloudplus_n;
            uint16_t sum[8] = {0};
            for (int j = 0; j < scloudplus_n; j++) {
                const uint16_t s = sp[j];
                for (int row = 0; row < 8; row++)
                    sum[row] = (uint16_t)(sum[row] + mul16(a_rows[(size_t)row * (size_t)scloudplus_n + (size_t)j], s));
            }
            for (int row = 0; row < 8; row++)
                B[(i + row) * scloudplus_nbar + col] =
                    (uint16_t)(B[(i + row) * scloudplus_nbar + col] + sum[row]);
        }
    }

    neon_a_state_clear(&st);
}

void mul_cs(uint16_t *C, uint16_t *S, uint16_t *out)
{
    memset(out, 0, (size_t)scloudplus_mbar * (size_t)scloudplus_nbar * sizeof(uint16_t));

    ALIGN_HEADER(32)
    uint16_t st[(size_t)scloudplus_n * (size_t)scloudplus_nbar] ALIGN_FOOTER(32);
    for (int k = 0; k < scloudplus_n; k++)
        for (int j = 0; j < scloudplus_nbar; j++)
            st[(size_t)k * (size_t)scloudplus_nbar + (size_t)j] =
                S[(size_t)j * (size_t)scloudplus_n + (size_t)k];

    int i = 0;
    for (; i + 1 < scloudplus_mbar; i += 2) {
        int col = 0;
        for (; col + 8 <= scloudplus_nbar; col += 8) {
            uint16x8_t acc0 = vdupq_n_u16(0);
            uint16x8_t acc1 = vdupq_n_u16(0);
            for (int k = 0; k < scloudplus_n; k++) {
                const uint16x8_t svec =
                    vld1q_u16(st + (size_t)k * (size_t)scloudplus_nbar +
                              (size_t)col);
                const uint16x8_t c0 = vdupq_n_u16(
                    C[(size_t)(i + 0) * (size_t)scloudplus_n + (size_t)k]);
                const uint16x8_t c1 = vdupq_n_u16(
                    C[(size_t)(i + 1) * (size_t)scloudplus_n + (size_t)k]);
                acc0 = vaddq_u16(acc0, vmulq_u16(c0, svec));
                acc1 = vaddq_u16(acc1, vmulq_u16(c1, svec));
            }
            vst1q_u16(out + (size_t)(i + 0) * (size_t)scloudplus_nbar +
                          (size_t)col,
                      acc0);
            vst1q_u16(out + (size_t)(i + 1) * (size_t)scloudplus_nbar +
                          (size_t)col,
                      acc1);
        }
        for (; col < scloudplus_nbar; col++) {
            uint16_t sum0 = 0;
            uint16_t sum1 = 0;
            for (int k = 0; k < scloudplus_n; k++) {
                const uint16_t s =
                    st[(size_t)k * (size_t)scloudplus_nbar + (size_t)col];
                sum0 = (uint16_t)(sum0 +
                    C[(size_t)(i + 0) * (size_t)scloudplus_n + (size_t)k] * s);
                sum1 = (uint16_t)(sum1 +
                    C[(size_t)(i + 1) * (size_t)scloudplus_n + (size_t)k] * s);
            }
            out[(size_t)(i + 0) * (size_t)scloudplus_nbar + (size_t)col] = sum0;
            out[(size_t)(i + 1) * (size_t)scloudplus_nbar + (size_t)col] = sum1;
        }
    }
    for (; i < scloudplus_mbar; i++) {
        int col = 0;
        for (; col + 8 <= scloudplus_nbar; col += 8) {
            uint16x8_t acc = vdupq_n_u16(0);
            for (int k = 0; k < scloudplus_n; k++) {
                const uint16x8_t svec =
                    vld1q_u16(st + (size_t)k * (size_t)scloudplus_nbar +
                              (size_t)col);
                const uint16x8_t c = vdupq_n_u16(
                    C[(size_t)i * (size_t)scloudplus_n + (size_t)k]);
                acc = vaddq_u16(acc, vmulq_u16(c, svec));
            }
            vst1q_u16(out + (size_t)i * (size_t)scloudplus_nbar +
                          (size_t)col,
                      acc);
        }
        for (; col < scloudplus_nbar; col++) {
            uint16_t sum = 0;
            for (int k = 0; k < scloudplus_n; k++)
                sum = (uint16_t)(sum +
                    C[(size_t)i * (size_t)scloudplus_n + (size_t)k] *
                    st[(size_t)k * (size_t)scloudplus_nbar + (size_t)col]);
            out[(size_t)i * (size_t)scloudplus_nbar + (size_t)col] = sum;
        }
    }
}

void mul_sb_e(const uint16_t *S, const uint16_t *B, const uint16_t *E, uint16_t *out)
{
    if (out != E)
        memcpy(out, E, (size_t)scloudplus_mbar * (size_t)scloudplus_nbar * sizeof(uint16_t));

    for (int i = 0; i < scloudplus_mbar; i++) {
        const uint16_t *S_row = S + (size_t)i * (size_t)scloudplus_m;
        uint16_t *out_row = out + (size_t)i * (size_t)scloudplus_nbar;
        int col = 0;
        for (; col + 8 <= scloudplus_nbar; col += 8) {
            uint16x8_t acc =
                vld1q_u16(out_row + (size_t)col);
            for (int k = 0; k < scloudplus_m; k++) {
                const uint16_t *B_row =
                    B + (size_t)k * (size_t)scloudplus_nbar;
                const uint16x8_t s = vdupq_n_u16(S_row[k]);
                acc = vaddq_u16(
                    acc, vmulq_u16(s, vld1q_u16(B_row + (size_t)col)));
            }
            vst1q_u16(out_row + (size_t)col, acc);
        }
        for (; col < scloudplus_nbar; col++) {
            uint16_t acc = out_row[col];
            for (int k = 0; k < scloudplus_m; k++)
                acc = (uint16_t)(
                    acc + mul16(S_row[k],
                                B[(size_t)k * (size_t)scloudplus_nbar +
                                  (size_t)col]));
            out_row[col] = acc;
        }
    }
}
