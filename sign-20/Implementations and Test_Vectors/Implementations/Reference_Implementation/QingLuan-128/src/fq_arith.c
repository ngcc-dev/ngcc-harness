/*
 * QingLuan Digital Signature Scheme
 * fq_arith.c - Constant-time arithmetic operations over F_q
 *
 * All operations are designed to be constant-time to prevent
 * timing side-channel attacks. Uses Barrett reduction for
 * modular multiplication.
 */

#include "fq_arith.h"
#include <string.h>

/* ============================================================
 * Basic F_q arithmetic (constant-time)
 * ============================================================ */

/* Addition in F_q: (a + b) mod q */
fq_t fq_add(fq_t a, fq_t b)
{
    uint32_t sum = (uint32_t)a + (uint32_t)b;
    uint32_t reduced = sum - PARAM_Q;
    uint32_t mask = (uint32_t)(-(int32_t)(reduced >> 31));
    return (fq_t)((reduced & ~mask) | (sum & mask));
}

/* Subtraction in F_q: (a - b) mod q */
fq_t fq_sub(fq_t a, fq_t b)
{
    int32_t diff = (int32_t)a - (int32_t)b;
    int32_t mask = diff >> 31;
    return (fq_t)(diff + (PARAM_Q & mask));
}

/*
 * Multiplication in F_q: (a * b) mod q
 * Uses Barrett reduction for constant-time modular arithmetic.
 * Barrett constants (BARRETT_M, BARRETT_K) defined in params.h.
 */
fq_t fq_mul(fq_t a, fq_t b)
{
    uint32_t n = (uint32_t)a * (uint32_t)b;
    /* Barrett estimate of quotient */
    uint32_t q_hat = (n * (uint32_t)BARRETT_M) >> BARRETT_K;
    /* Approximate remainder */
    uint32_t r = n - q_hat * (uint32_t)PARAM_Q;
    /* Constant-time conditional subtract (at most once) */
    uint32_t reduced = r - (uint32_t)PARAM_Q;
    uint32_t mask = (uint32_t)(-(int32_t)(reduced >> 31));
    return (fq_t)((reduced & ~mask) | (r & mask));
}

/*
 * Negation in F_q: (-a) mod q
 * Returns 0 if a == 0, otherwise q - a.
 * Constant-time: avoids branch on the value of a.
 */
fq_t fq_neg(fq_t a)
{
    uint32_t v = (uint32_t)a;
    /* Constant-time nonzero check: mask = 0xFFFFFFFF if v != 0, else 0 */
    uint32_t mask = (uint32_t)(-((v | (uint32_t)(-(int32_t)v)) >> 31));
    return (fq_t)(((uint32_t)PARAM_Q - v) & mask);
}

/* ============================================================
 * Vector operations over F_q
 * ============================================================ */

void fq_vec_add(fq_t *out, const fq_t *a, const fq_t *b, size_t len)
{
    for (size_t i = 0; i < len; i++)
        out[i] = fq_add(a[i], b[i]);
}

void fq_vec_sub(fq_t *out, const fq_t *a, const fq_t *b, size_t len)
{
    for (size_t i = 0; i < len; i++)
        out[i] = fq_sub(a[i], b[i]);
}

/*
 * Constant-time Barrett reduction for accumulated sums.
 *
 * Replaces the % operator which may emit variable-time hardware division
 * on some compilers/platforms (e.g. MSVC, GCC -O0).
 *
 * Uses Barrett with K=32: M = floor(2^32 / q).
 * The base field is p = 127 for ALL levels (CROSS-RSDP), so a single constant
 * suffices. Accumulated sums stay far below 2^32 (max real n=512: 512*126^2 =
 * 8,128,512), so the Barrett estimate error is <= 1 and one conditional
 * subtract brings r into [0, q).
 */
#define BARRETT_SUM_M  33818640ULL  /* floor(2^32 / 127), q=127 (all levels) */
#define BARRETT_SUM_K  32

static inline fq_t fq_reduce_sum(uint64_t sum)
{
    uint64_t q_hat = (sum * BARRETT_SUM_M) >> BARRETT_SUM_K;
    uint32_t r = (uint32_t)(sum - q_hat * (uint64_t)PARAM_Q);
    /* Constant-time conditional subtract (at most once) */
    uint32_t reduced = r - (uint32_t)PARAM_Q;
    uint32_t mask = (uint32_t)(-(int32_t)(reduced >> 31));
    return (fq_t)((reduced & ~mask) | (r & mask));
}

/*
 * Inner product: sum(a[i] * b[i]) mod q
 *
 * Accumulates products into uint64_t for performance, then applies
 * explicit constant-time Barrett reduction (not the % operator,
 * which may compile to variable-time division on some platforms).
 */
fq_t fq_vec_inner(const fq_t *a, const fq_t *b, size_t len)
{
    uint64_t sum = 0;
    for (size_t i = 0; i < len; i++)
        sum += (uint64_t)a[i] * (uint64_t)b[i];
    return fq_reduce_sum(sum);
}

/* Matrix-vector multiply: out = M * v, where M is rows x cols */
void fq_mat_vec_mul(fq_t *out, const fq_t *M, const fq_t *v,
                    size_t rows, size_t cols)
{
    for (size_t i = 0; i < rows; i++) {
        uint64_t sum = 0;
        for (size_t j = 0; j < cols; j++)
            sum += (uint64_t)M[i * cols + j] * (uint64_t)v[j];
        out[i] = fq_reduce_sum(sum);
    }
}

void fq_vec_zero(fq_t *v, size_t len)
{
    memset(v, 0, len * sizeof(fq_t));
}

void fq_vec_copy(fq_t *dst, const fq_t *src, size_t len)
{
    memcpy(dst, src, len * sizeof(fq_t));
}
