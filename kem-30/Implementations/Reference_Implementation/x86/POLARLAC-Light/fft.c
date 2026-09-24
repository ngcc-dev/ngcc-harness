/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense, Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements FFT-based spectral helper routines for the reference POLARLAC-Light instance.
*/



#include "fft.h"

/**
 * @brief Scaling factor used during FFT staging to avoid overflow. The
 *        static twiddle table is scaled accordingly.
 */
#define FFT_I16_FACTOR  256LL    /* 2^8 */
#define FFT_I16_SHIFT   8

/**
 * @brief Precomputed twiddle factors (int16) used by the small FFT
 *        implementation. This table contains RL_KEM_N/1 entries and is
 *        arranged to match the indexing strategy used by `ntt_fwd_i16`.
 */
static const ci16_t g_twiddle_int16_static[256] = {
    {0, 0},
    {0, 256},
    {181, 181},
    {-181, 181},
    {237, 98},
    {-98, 237},
    {98, 237},
    {-237, 98},
    {251, 50},
    {-50, 251},
    {142, 213},
    {-213, 142},
    {213, 142},
    {-142, 213},
    {50, 251},
    {-251, 50},
    {255, 25},
    {-25, 255},
    {162, 198},
    {-198, 162},
    {226, 121},
    {-121, 226},
    {74, 245},
    {-245, 74},
    {245, 74},
    {-74, 245},
    {121, 226},
    {-226, 121},
    {198, 162},
    {-162, 198},
    {25, 255},
    {-255, 25},
    {256, 13},
    {-13, 256},
    {172, 190},
    {-190, 172},
    {231, 109},
    {-109, 231},
    {86, 241},
    {-241, 86},
    {248, 62},
    {-62, 248},
    {132, 220},
    {-220, 132},
    {206, 152},
    {-152, 206},
    {38, 253},
    {-253, 38},
    {253, 38},
    {-38, 253},
    {152, 206},
    {-206, 152},
    {220, 132},
    {-132, 220},
    {62, 248},
    {-248, 62},
    {241, 86},
    {-86, 241},
    {109, 231},
    {-231, 109},
    {190, 172},
    {-172, 190},
    {13, 256},
    {-256, 13},
    {256, 6},
    {-6, 256},
    {177, 185},
    {-185, 177},
    {234, 104},
    {-104, 234},
    {92, 239},
    {-239, 92},
    {250, 56},
    {-56, 250},
    {137, 216},
    {-216, 137},
    {209, 147},
    {-147, 209},
    {44, 252},
    {-252, 44},
    {254, 31},
    {-31, 254},
    {157, 202},
    {-202, 157},
    {223, 126},
    {-126, 223},
    {68, 247},
    {-247, 68},
    {243, 80},
    {-80, 243},
    {115, 229},
    {-229, 115},
    {194, 167},
    {-167, 194},
    {19, 255},
    {-255, 19},
    {255, 19},
    {-19, 255},
    {167, 194},
    {-194, 167},
    {229, 115},
    {-115, 229},
    {80, 243},
    {-243, 80},
    {247, 68},
    {-68, 247},
    {126, 223},
    {-223, 126},
    {202, 157},
    {-157, 202},
    {31, 254},
    {-254, 31},
    {252, 44},
    {-44, 252},
    {147, 209},
    {-209, 147},
    {216, 137},
    {-137, 216},
    {56, 250},
    {-250, 56},
    {239, 92},
    {-92, 239},
    {104, 234},
    {-234, 104},
    {185, 177},
    {-177, 185},
    {6, 256},
    {-256, 6},
    {256, 3},
    {-3, 256},
    {179, 183},
    {-183, 179},
    {235, 101},
    {-101, 235},
    {95, 238},
    {-238, 95},
    {250, 53},
    {-53, 250},
    {140, 215},
    {-215, 140},
    {211, 145},
    {-145, 211},
    {47, 252},
    {-252, 47},
    {254, 28},
    {-28, 254},
    {160, 200},
    {-200, 160},
    {224, 123},
    {-123, 224},
    {71, 246},
    {-246, 71},
    {244, 77},
    {-77, 244},
    {118, 227},
    {-227, 118},
    {196, 165},
    {-165, 196},
    {22, 255},
    {-255, 22},
    {256, 16},
    {-16, 256},
    {170, 192},
    {-192, 170},
    {230, 112},
    {-112, 230},
    {83, 242},
    {-242, 83},
    {248, 65},
    {-65, 248},
    {129, 221},
    {-221, 129},
    {204, 155},
    {-155, 204},
    {34, 254},
    {-254, 34},
    {253, 41},
    {-41, 253},
    {150, 207},
    {-207, 150},
    {218, 134},
    {-134, 218},
    {59, 249},
    {-249, 59},
    {240, 89},
    {-89, 240},
    {107, 233},
    {-233, 107},
    {188, 174},
    {-174, 188},
    {9, 256},
    {-256, 9},
    {256, 9},
    {-9, 256},
    {174, 188},
    {-188, 174},
    {233, 107},
    {-107, 233},
    {89, 240},
    {-240, 89},
    {249, 59},
    {-59, 249},
    {134, 218},
    {-218, 134},
    {207, 150},
    {-150, 207},
    {41, 253},
    {-253, 41},
    {254, 34},
    {-34, 254},
    {155, 204},
    {-204, 155},
    {221, 129},
    {-129, 221},
    {65, 248},
    {-248, 65},
    {242, 83},
    {-83, 242},
    {112, 230},
    {-230, 112},
    {192, 170},
    {-170, 192},
    {16, 256},
    {-256, 16},
    {255, 22},
    {-22, 255},
    {165, 196},
    {-196, 165},
    {227, 118},
    {-118, 227},
    {77, 244},
    {-244, 77},
    {246, 71},
    {-71, 246},
    {123, 224},
    {-224, 123},
    {200, 160},
    {-160, 200},
    {28, 254},
    {-254, 28},
    {252, 47},
    {-47, 252},
    {145, 211},
    {-211, 145},
    {215, 140},
    {-140, 215},
    {53, 250},
    {-250, 53},
    {238, 95},
    {-95, 238},
    {101, 235},
    {-235, 101},
    {183, 179},
    {-179, 183},
    {3, 256},
    {-256, 3}
};


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
    int32_t half = 1 << (s - 1);

    /*
     * sign = 0, if x >= 0
     * sign = 1, if x <  0
     */
    int32_t sign = (int32_t)((uint32_t)x >> 31);

    return (x + half - sign) >> s;
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
    ci16_t u = { (int16_t)(a[0] << FFT_I16_SHIFT), 0 };
    ci16_t v = ci16_mul_tw(
        (ci16_t){ (int16_t)(a[stride] << FFT_I16_SHIFT), 0 },
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
    for (int k = 0; k < UNIQ; ++k) {                                        \
        ci16_t t = ci16_mul_tw(                                             \
            odd_unique[k],                                                  \
            g_twiddle_int16_static[HALF + k]                                \
        );                                                                  \
                                                                            \
        out[k << 1].re = even_unique[k].re + t.re;                          \
        out[k << 1].im = even_unique[k].im + t.im;                          \
        out[(k << 1) + 1].re = even_unique[k].re - t.re;                    \
        out[(k << 1) + 1].im = even_unique[k].im - t.im;                    \
    }                                                                       \
}

DEFINE_REAL_FFT_UNIQUE_RAW_I16(4,   2)
DEFINE_REAL_FFT_UNIQUE_RAW_I16(8,   4)
DEFINE_REAL_FFT_UNIQUE_RAW_I16(16,  8)
DEFINE_REAL_FFT_UNIQUE_RAW_I16(32,  16)
DEFINE_REAL_FFT_UNIQUE_RAW_I16(64,  32)
DEFINE_REAL_FFT_UNIQUE_RAW_I16(128, 64)
DEFINE_REAL_FFT_UNIQUE_RAW_I16(256, 128)

#if RL_KEM_N >= 512
DEFINE_REAL_FFT_UNIQUE_RAW_I16(512, 256)
#endif

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
#if RL_KEM_N == 256
    real_fft_unique_raw_i16_n256(a, stride, out);
#elif RL_KEM_N == 512
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

int fft_within_bound_int16(const int16_t *a, int32_t bound)
{
    ci16_t uniq[RL_KEM_N_Half];
    uint32_t reject_mask = 0;

    real_fft_unique_raw_i16_root(a, 1, uniq);

    for (int i = 0; i < RL_KEM_N_Half; ++i) {
        int32_t re = rsh32((int32_t)uniq[i].re, FFT_I16_SHIFT);
        int32_t im = rsh32((int32_t)uniq[i].im, FFT_I16_SHIFT);
        int32_t comp = re * re + im * im;

        reject_mask |= ((((uint32_t)bound - 1u) - (uint32_t)comp) >> 31);
    }

    return (int)(reject_mask ^ 1u);
}

/**
 * End of fft.c
 */
