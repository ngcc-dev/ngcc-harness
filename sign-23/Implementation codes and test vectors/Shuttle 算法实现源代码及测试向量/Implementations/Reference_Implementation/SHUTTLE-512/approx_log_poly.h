#ifndef SHUTTLE_APPROX_LOG_POLY_H
#define SHUTTLE_APPROX_LOG_POLY_H

#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define SHUTTLE_ALWAYS_INLINE static inline __attribute__((always_inline))
#else
#define SHUTTLE_ALWAYS_INLINE static inline
#endif

/* Segmented base-2 log: log2(b), b in [1,2), to absolute error < 2^-57.
   g = 2 segment-index bits, degree 13, Q62 coefficients.
   measured max abs error = 2^-59.453
   table = 4 x 14 int64 = 448 bytes. */
#define SHUTTLE_LOG_POLY_G 2
#define SHUTTLE_LOG_POLY_SEGMENTS 4
#define SHUTTLE_LOG_POLY_DEGREE 13
#define SHUTTLE_LOG_POLY_QBITS 62

/* kShuttleLogPoly[j][k] : Q62 coefficient of x^k on segment j,
   with x = (low mantissa bits) normalised to [0,1) as a Q64 value. */
static const int64_t kShuttleLogPoly[4][14] = {
    {INT64_C(0), INT64_C(1663314137230540005), INT64_C(-207914267153782834), INT64_C(34652377857794422), INT64_C(-6497320829889967), INT64_C(1299464001399016), INT64_C(-270720747985221), INT64_C(58008183734025), INT64_C(-12680634058615), INT64_C(2802591328231), INT64_C(-611697204761), INT64_C(123141961189), INT64_C(-19530076391), INT64_C(1717255301)},
    {INT64_C(1484631294131014398), INT64_C(1330651309784432187), INT64_C(-133065130978439107), INT64_C(17742017463685572), INT64_C(-2661302618120493), INT64_C(425808407352253), INT64_C(-70968007646487), INT64_C(12165731010280), INT64_C(-2128477788302), INT64_C(377485364884), INT64_C(-66839897685), INT64_C(11228353229), INT64_C(-1554229086), INT64_C(124985134)},
    {INT64_C(2697663385880076776), INT64_C(1108876091487026868), INT64_C(-92406340957251847), INT64_C(10267371217462318), INT64_C(-1283421402046772), INT64_C(171122852511038), INT64_C(-23767057140815), INT64_C(3395273701197), INT64_C(-495094418506), INT64_C(73261264270), INT64_C(-10884612082), INT64_C(1561954161), INT64_C(-191070343), INT64_C(14130118)},
    {INT64_C(3723267405961586381), INT64_C(950465221274594463), INT64_C(-67890372948185266), INT64_C(6465749804587722), INT64_C(-692758907616243), INT64_C(79172446438655), INT64_C(-9425290482070), INT64_C(1154114515161), INT64_C(-144257705268), INT64_C(18306992888), INT64_C(-2339854304), INT64_C(292286931), INT64_C(-31958889), INT64_C(2187745)},
};

/* High half of a signed 128-bit product, rounded to nearest:
   ((a*x) + 2^63) >> 64  (arithmetic shift).  Rounding is unbiased and
   halves the per-step error vs truncation. */
SHUTTLE_ALWAYS_INLINE int64_t shuttle_log_mulhi(__int128 a, uint64_t x)
{
    return (int64_t)(((a * (__int128)(__uint128_t)x) + ((__int128)1 << 63)) >> 64);
}

/* Branch-free constant-time equality mask: all-ones iff a==b, else 0.
   Data-independent: no branch and no cmov.  (At -O2/-O3 the compiler
   may re-fold this into a cmp+sbb/sete, all single-cycle, flag consumed
   arithmetically -- never by a conditional jump.) */
SHUTTLE_ALWAYS_INLINE uint64_t shuttle_log_eqmask(uint32_t a, uint32_t b)
{
    uint64_t z = (uint64_t)(a ^ b);          /* 0 iff a==b */
    uint64_t nz = (z | (~z + 1)) >> 63;       /* 1 iff z!=0, else 0 */
    return nz - 1;                            /* 0xFF..FF iff a==b, else 0 */
}

/* Constant-time fetch of segment row `sel` into c[0..DEGREE]:
   touches EVERY table entry, selects by arithmetic mask (no branch,
   no data-dependent address). */
SHUTTLE_ALWAYS_INLINE void shuttle_log_fetch_row(uint32_t sel, int64_t c[SHUTTLE_LOG_POLY_DEGREE + 1])
{
    for (int k = 0; k <= SHUTTLE_LOG_POLY_DEGREE; k++) c[k] = 0;
    for (uint32_t j = 0; j < SHUTTLE_LOG_POLY_SEGMENTS; j++) {
        uint64_t m = shuttle_log_eqmask(j, sel); /* all-ones iff j==sel */
        for (int k = 0; k <= SHUTTLE_LOG_POLY_DEGREE; k++)
            c[k] |= (int64_t)(m & (uint64_t)kShuttleLogPoly[j][k]);
    }
}

/* Approximate log2(b) in Q62 for b = 1 + m/2^kappa_b in [1,2).
   `j` = top g bits of m (segment), `x_q64` = remaining low mantissa
   bits left-justified into a Q64 fraction in [0,1).  Branch-free,
   division-free, constant-time. */
SHUTTLE_ALWAYS_INLINE int64_t shuttle_log2_frac_q62(uint32_t j, uint64_t x_q64)
{
    int64_t c[SHUTTLE_LOG_POLY_DEGREE + 1];
    shuttle_log_fetch_row(j, c);
    __int128 acc = c[SHUTTLE_LOG_POLY_DEGREE];
    for (int k = SHUTTLE_LOG_POLY_DEGREE - 1; k >= 0; k--)
        acc = (__int128)c[k] + (__int128)shuttle_log_mulhi(acc, x_q64);
    return (int64_t)acc;
}

/* PERFORMANCE-OPTIMAL variant (see ApproxLog.tex, Table tab:batch):
   2-way batched evaluator, ~69 cyc/output vs ~78 for the scalar above.
   Computes two INDEPENDENT log2 fractions at once -- each kShuttleLogPoly
   entry is loaded once and shared by both lanes, and the two Horner chains
   interleave to hide the multiply latency.  Register-lean fused form (only
   the 2*SEGMENTS masks + two accumulators are live).  Same constant-time
   guarantees as the scalar.  Use this when the caller can supply two
   independent inputs; otherwise use shuttle_log2_frac_q62. */
SHUTTLE_ALWAYS_INLINE void shuttle_log2_frac_q62_x2(const uint32_t sel[2], const uint64_t x_q64[2], int64_t out[2])
{
    uint64_t M0[SHUTTLE_LOG_POLY_SEGMENTS], M1[SHUTTLE_LOG_POLY_SEGMENTS];
    for (uint32_t j = 0; j < SHUTTLE_LOG_POLY_SEGMENTS; j++) {
        M0[j] = shuttle_log_eqmask(j, sel[0]);
        M1[j] = shuttle_log_eqmask(j, sel[1]);
    }
    int64_t h0 = 0, h1 = 0;
    for (uint32_t j = 0; j < SHUTTLE_LOG_POLY_SEGMENTS; j++) {
        int64_t v = kShuttleLogPoly[j][SHUTTLE_LOG_POLY_DEGREE];
        h0 |= (int64_t)(M0[j] & (uint64_t)v);
        h1 |= (int64_t)(M1[j] & (uint64_t)v);
    }
    __int128 a0 = h0, a1 = h1;
    for (int k = SHUTTLE_LOG_POLY_DEGREE - 1; k >= 0; k--) {
        int64_t c0 = 0, c1 = 0;
        for (uint32_t j = 0; j < SHUTTLE_LOG_POLY_SEGMENTS; j++) {
            int64_t v = kShuttleLogPoly[j][k];
            c0 |= (int64_t)(M0[j] & (uint64_t)v);
            c1 |= (int64_t)(M1[j] & (uint64_t)v);
        }
        a0 = (__int128)c0 + (__int128)shuttle_log_mulhi(a0, x_q64[0]);
        a1 = (__int128)c1 + (__int128)shuttle_log_mulhi(a1, x_q64[1]);
    }
    out[0] = (int64_t)a0;
    out[1] = (int64_t)a1;
}

#undef SHUTTLE_ALWAYS_INLINE
#endif
