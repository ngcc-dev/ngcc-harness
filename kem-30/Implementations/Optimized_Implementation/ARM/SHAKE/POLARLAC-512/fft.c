/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements FFT-based spectral helper routines for the optimized POLARLAC-512 instance.
*/



#include "fft.h"
#define POLARLAC_ARM_MLWE_FFT_HELPERS 1
#include "arm_mlwe_sampler_fft.h"

/**
 * @file fft.c
 * @brief Reference FFT implementation used by RL-KEM demo code.
 *
 * The implementation below provides a fixed-size forward FFT for 16-bit
 * integer input. Compared with the original recursive version, the base-case
 * test `if (n == 2)` is removed from the recursive path.  Instead, each
 * transform length is generated as a dedicated function at compile time:
 *
 *     n512 -> n256 -> n128 -> ... -> n2
 *
 * This keeps the original decomposition structure but avoids the runtime
 * branch at every recursive call.
 */
static inline int32_t rsh32(int32_t x, int s)
{
    /* Symmetric fixed-point rounding, expressed without signed shifts. */
    const int64_t neg = (x < 0);
    const int64_t sign = INT64_C(1) - INT64_C(2) * neg;
    const int64_t mag = (int64_t)x * sign;
    const int64_t half = INT64_C(1) << (s - 1);
    const int64_t rounded = (mag + half) / (INT64_C(1) << s);
    return (int32_t)(rounded * sign);
}

static inline int16_t fft_i16_wrap_add(int16_t a, int16_t b)
{
    return (int16_t)((uint16_t)a + (uint16_t)b);
}

static inline int16_t fft_i16_wrap_sub(int16_t a, int16_t b)
{
    return (int16_t)((uint16_t)a - (uint16_t)b);
}

static inline uint32_t fft_reject_u64(uint64_t comp, uint32_t bound)
{
    return (uint32_t)((((uint64_t)bound - 1u) - comp) >> 63);
}

/**
 * @brief Multiply a complex int16 value by a twiddle factor and scale by
 *        FFT_I16_SHIFT.
 */
static inline ci16_t ci16_mul_tw(ci16_t data, ci16_t tw)
{
    int32_t t0 = (int32_t)tw.re * (int32_t)data.re;
    int32_t t2 = (int32_t)tw.im * (int32_t)data.im;
    int32_t t1 = ((int32_t)tw.re + (int32_t)tw.im) *
                 ((int32_t)data.re + (int32_t)data.im);

    int32_t re = t0 - t2;
    int32_t im = t1 - t0 - t2;

    // return (ci16_t){
    //     (int16_t)(re >> FFT_I16_SHIFT),
    //     (int16_t)(im >> FFT_I16_SHIFT)
    // };
        return (ci16_t){
        (int16_t)rsh32(re, FFT_I16_SHIFT),
        (int16_t)rsh32(im, FFT_I16_SHIFT)
    };
}


static inline void fft_combine_dispatch(
    ci16_t *out,
    const ci16_t *even_unique,
    const ci16_t *odd_unique,
    const ci16_t *twiddle,
    int count)
{
    const size_t scount = (count > 0) ? (size_t)count : 0u;
    size_t k = 0;
#if POLARLAC_COMPONENT_B_SELECTED && defined(__aarch64__)
    k = polarlac_fft_combine_neon(out, even_unique, odd_unique,
                                  twiddle, scount);
#endif
    for (; k < scount; ++k) {
        ci16_t t = ci16_mul_tw(odd_unique[k], twiddle[k]);
        const size_t out_idx = k << 1;
        out[out_idx].re = fft_i16_wrap_add(even_unique[k].re, t.re);
        out[out_idx].im = fft_i16_wrap_add(even_unique[k].im, t.im);
        out[out_idx + 1u].re = fft_i16_wrap_sub(even_unique[k].re, t.re);
        out[out_idx + 1u].im = fft_i16_wrap_sub(even_unique[k].im, t.im);
    }
}



/* ------------------------------------------------------------------------- */
/* Branch-free recursive path by compile-time fixed-size specialization.       */
/* ------------------------------------------------------------------------- */

#define CAT2_(a, b) a##b
#define CAT2(a, b)  CAT2_(a, b)
#define REAL_FFT_FN(N) CAT2(real_fft_unique_raw_i16_n, N)

static inline void real_fft_unique_raw_i16_n2(
    const int16_t *a,
    int stride,
    ci16_t *out
)
{
    ci16_t u = { (int16_t)((int32_t)a[0] * (INT32_C(1) << FFT_I16_SHIFT)), 0 };
    ci16_t v = ci16_mul_tw(
        (ci16_t){ (int16_t)((int32_t)a[stride] * (INT32_C(1) << FFT_I16_SHIFT)), 0 },
        g_twiddle_int16_static[1]
    );

    out[0].re = u.re + v.re;
    out[0].im = u.im + v.im;
}

#define DEFINE_REAL_FFT_UNIQUE_RAW_I16(N, CHILD_N)                         \
static inline void REAL_FFT_FN(N)(                                          \
    const int16_t *a,                                                       \
    int stride,                                                             \
    ci16_t *out                                                             \
)                                                                           \
{                                                                           \
    enum { HALF = (N) >> 1 };                                               \
    enum { UNIQ = (N) >> 2 };                                               \
                                                                            \
    ci16_t even_unique[UNIQ];                                               \
    ci16_t odd_unique[UNIQ];                                                \
                                                                            \
    REAL_FFT_FN(CHILD_N)(a, stride << 1, even_unique);                      \
    REAL_FFT_FN(CHILD_N)(a + stride, stride << 1, odd_unique);              \
                                                                            \
    fft_combine_dispatch(out, even_unique, odd_unique,                    \
                         &g_twiddle_int16_static[HALF], UNIQ);               \
}

DEFINE_REAL_FFT_UNIQUE_RAW_I16(4,   2)
DEFINE_REAL_FFT_UNIQUE_RAW_I16(8,   4)
DEFINE_REAL_FFT_UNIQUE_RAW_I16(16,  8)
DEFINE_REAL_FFT_UNIQUE_RAW_I16(32,  16)
DEFINE_REAL_FFT_UNIQUE_RAW_I16(64,  32)
DEFINE_REAL_FFT_UNIQUE_RAW_I16(128, 64)
DEFINE_REAL_FFT_UNIQUE_RAW_I16(256, 128)
DEFINE_REAL_FFT_UNIQUE_RAW_I16(512, 256)

#if RL_KEM_N >= 1024
DEFINE_REAL_FFT_UNIQUE_RAW_I16(1024, 512)
#endif

#if RL_KEM_N >= 2048
DEFINE_REAL_FFT_UNIQUE_RAW_I16(2048, 1024)
#endif

static inline void real_fft_unique_raw_i16_root(
    const int16_t *a,
    int stride,
    ci16_t *out
)
{
#if RL_KEM_N == 512
    real_fft_unique_raw_i16_n512(a, stride, out);
#elif RL_KEM_N == 1024
    real_fft_unique_raw_i16_n1024(a, stride, out);
#elif RL_KEM_N == 2048
    real_fft_unique_raw_i16_n2048(a, stride, out);
#else
#error "Unsupported RL_KEM_N for real_fft_unique_raw_i16_root"
#endif
}

/**
 * @brief Compatibility wrapper for the public prototype in fft.h.
 *
 * The original implementation accepted `n`, but current RL-KEM parameters use
 * a fixed transform length.  The recursive computation itself no longer checks
 * `if (n == 2)` at every call; it dispatches once to the compile-time selected
 * fixed-size transform.
 */
void real_fft_unique_raw_i16(const int16_t *a, int stride, int n, ci16_t *out)
{
    (void)n;
    real_fft_unique_raw_i16_root(a, stride, out);
}

/* ------------------------------------------------------------------------- */
/* Public APIs.                                                               */
/* ------------------------------------------------------------------------- */

void fft_forward_int16(const int16_t *a, int16_t *out_re, int16_t *out_im)
{
    ci16_t uniq[RL_KEM_N_Half];

    real_fft_unique_raw_i16_root(a, 1, uniq);

    for (int i = 0; i < RL_KEM_N_Half; ++i) {
        int16_t re = (int16_t)rsh32((int32_t)uniq[i].re, FFT_I16_SHIFT);
        int16_t im = (int16_t)rsh32((int32_t)uniq[i].im, FFT_I16_SHIFT);
        // int16_t re = (int16_t)(uniq[i].re >> FFT_I16_SHIFT);
        // int16_t im = (int16_t)(uniq[i].im >> FFT_I16_SHIFT);
        int mirror = RL_KEM_N - 1 - i;

        out_re[i] = re;
        out_im[i] = im;
        out_re[mirror] = re;
        out_im[mirror] = (int16_t)(-(int32_t)im);
    }
}



static int fft_unique_within_bound_impl(const ci16_t *uniq, int count,
                                        int32_t bound)
{
    const size_t scount = (count > 0) ? (size_t)count : 0u;
    uint32_t reject_mask = 0;
    size_t i = 0;
#if POLARLAC_COMPONENT_B_SELECTED && defined(__aarch64__)
    i = polarlac_fft_bound_neon(uniq, scount, bound, &reject_mask);
#endif
    for (; i < scount; ++i) {
        int32_t re = rsh32((int32_t)uniq[i].re, FFT_I16_SHIFT);
        int32_t im = rsh32((int32_t)uniq[i].im, FFT_I16_SHIFT);
        uint64_t comp = (uint64_t)((int64_t)re * (int64_t)re) +
                        (uint64_t)((int64_t)im * (int64_t)im);
        reject_mask |= fft_reject_u64(comp, (uint32_t)bound);
    }
    return (int)(reject_mask ^ 1u);
}

int fft_within_bound_int16(const int16_t *a, int32_t bound)
{
    ci16_t uniq[RL_KEM_N_Half];
    real_fft_unique_raw_i16_root(a, 1, uniq);
    return fft_unique_within_bound_impl(uniq, RL_KEM_N_Half, bound);
}

#ifdef POLARLAC_B_TEST_HOOKS
int polarlac_b_test_bound_from_unique(const ci16_t *uniq, int count,
                                      int32_t bound)
{
    return fft_unique_within_bound_impl(uniq, count, bound);
}
#endif

/**
 * End of fft.c
 */
