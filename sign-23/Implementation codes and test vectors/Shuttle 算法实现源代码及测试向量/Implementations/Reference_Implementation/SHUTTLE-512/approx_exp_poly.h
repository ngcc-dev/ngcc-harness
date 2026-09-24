#ifndef SHUTTLE_APPROX_EXP_POLY_H
#define SHUTTLE_APPROX_EXP_POLY_H

#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define SHUTTLE_ALWAYS_INLINE static inline __attribute__((always_inline))
#else
#define SHUTTLE_ALWAYS_INLINE static inline
#endif

#define SHUTTLE_EXP_POLY_DEGREE 8
#define SHUTTLE_EXP_POLY_SQUARINGS 7
#define SHUTTLE_EXP_POLY_SPLIT 128
#define SHUTTLE_EXP_POLY_X_MAX 36
#define SHUTTLE_EXP_POLY_Y_MAX 255

static const int64_t kShuttleExpPolyCoeff[8] = {
    INT64_C(-888099775658129789),
    INT64_C(21378331275493551),
    INT64_C(-343079355609610),
    INT64_C(4129301863954),
    INT64_C(-39760209654),
    INT64_C(319035825),
    INT64_C(-2194237),
    INT64_C(13205),
};

SHUTTLE_ALWAYS_INLINE int64_t shuttle_high64_s128(__int128 a, int64_t b)
{
    return (int64_t)((a * (__int128)b) >> 64);
}

SHUTTLE_ALWAYS_INLINE uint64_t shuttle_high64_u64(uint64_t a, uint64_t b)
{
    return (uint64_t)(((__uint128_t)a * (__uint128_t)b) >> 64);
}

SHUTTLE_ALWAYS_INLINE uint64_t shuttle_exp_accept_poly_q64(int x, int y)
{
    uint64_t n = (uint64_t)y * (uint64_t)(y + 512 * x);
    int64_t s_q63 = (int64_t)(n << 40);
    __int128 acc = kShuttleExpPolyCoeff[7];
    acc = (__int128)kShuttleExpPolyCoeff[6] +
          ((__int128)shuttle_high64_s128(acc, s_q63) * 2);
    acc = (__int128)kShuttleExpPolyCoeff[5] +
          ((__int128)shuttle_high64_s128(acc, s_q63) * 2);
    acc = (__int128)kShuttleExpPolyCoeff[4] +
          ((__int128)shuttle_high64_s128(acc, s_q63) * 2);
    acc = (__int128)kShuttleExpPolyCoeff[3] +
          ((__int128)shuttle_high64_s128(acc, s_q63) * 2);
    acc = (__int128)kShuttleExpPolyCoeff[2] +
          ((__int128)shuttle_high64_s128(acc, s_q63) * 2);
    acc = (__int128)kShuttleExpPolyCoeff[1] +
          ((__int128)shuttle_high64_s128(acc, s_q63) * 2);
    acc = (__int128)kShuttleExpPolyCoeff[0] +
          ((__int128)shuttle_high64_s128(acc, s_q63) * 2);
    acc = ((__int128)UINT64_MAX) +
          ((__int128)shuttle_high64_s128(acc, s_q63) * 2);
    uint64_t v = (uint64_t)acc;
    v = shuttle_high64_u64(v, v);
    v = shuttle_high64_u64(v, v);
    v = shuttle_high64_u64(v, v);
    v = shuttle_high64_u64(v, v);
    v = shuttle_high64_u64(v, v);
    v = shuttle_high64_u64(v, v);
    v = shuttle_high64_u64(v, v);
    return v;
}

/* PERFORMANCE-OPTIMAL variant (see ApproxExp.tex, Table tab:exp-batch):
   4-way batched, ~42 cyc/output vs ~56 for the scalar above.  Four
   INDEPENDENT exp evaluations share the coefficient table (one load per
   Horner step) and interleave four Horner + squaring chains to hide the
   multiply latency; only four accumulators are live.  Same constant-time
   guarantees as the scalar.  Use when the caller can supply four
   independent (x,y); otherwise use shuttle_exp_accept_poly_q64. */
SHUTTLE_ALWAYS_INLINE void shuttle_exp_accept_poly_q64_x4(const int x[4], const int y[4], uint64_t out[4])
{
    int64_t s0 = (int64_t)(((uint64_t)y[0] * (uint64_t)(y[0] + 512 * x[0])) << 40);
    int64_t s1 = (int64_t)(((uint64_t)y[1] * (uint64_t)(y[1] + 512 * x[1])) << 40);
    int64_t s2 = (int64_t)(((uint64_t)y[2] * (uint64_t)(y[2] + 512 * x[2])) << 40);
    int64_t s3 = (int64_t)(((uint64_t)y[3] * (uint64_t)(y[3] + 512 * x[3])) << 40);
    __int128 a0 = kShuttleExpPolyCoeff[7]; __int128 a1 = kShuttleExpPolyCoeff[7]; __int128 a2 = kShuttleExpPolyCoeff[7]; __int128 a3 = kShuttleExpPolyCoeff[7];
    for (int k = 6; k >= 0; k--) {
        int64_t c = kShuttleExpPolyCoeff[k];
        a0 = (__int128)c + ((__int128)shuttle_high64_s128(a0, s0) * 2);
        a1 = (__int128)c + ((__int128)shuttle_high64_s128(a1, s1) * 2);
        a2 = (__int128)c + ((__int128)shuttle_high64_s128(a2, s2) * 2);
        a3 = (__int128)c + ((__int128)shuttle_high64_s128(a3, s3) * 2);
    }
    a0 = ((__int128)UINT64_MAX) + ((__int128)shuttle_high64_s128(a0, s0) * 2);
    a1 = ((__int128)UINT64_MAX) + ((__int128)shuttle_high64_s128(a1, s1) * 2);
    a2 = ((__int128)UINT64_MAX) + ((__int128)shuttle_high64_s128(a2, s2) * 2);
    a3 = ((__int128)UINT64_MAX) + ((__int128)shuttle_high64_s128(a3, s3) * 2);
    uint64_t v0 = (uint64_t)a0;
    uint64_t v1 = (uint64_t)a1;
    uint64_t v2 = (uint64_t)a2;
    uint64_t v3 = (uint64_t)a3;
    for (int q = 0; q < 7; q++) {
        v0 = shuttle_high64_u64(v0, v0);
        v1 = shuttle_high64_u64(v1, v1);
        v2 = shuttle_high64_u64(v2, v2);
        v3 = shuttle_high64_u64(v3, v3);
    }
    out[0] = v0;
    out[1] = v1;
    out[2] = v2;
    out[3] = v3;
}

#undef SHUTTLE_ALWAYS_INLINE

#endif
