/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares FFT-based spectral helper routines and constants for the optimized POLARLAC-Light instance.
*/

#ifndef FFT_H
#define FFT_H

#include <stdint.h>
#include "params.h"

/**
 * @file fft.h
 * @brief Fixed-point real-input FFT interface used by screened sampling.
 *
 * The transform preserves the reference algorithm's recursive even/odd
 * decomposition and computes the N/2 unique bins implied by conjugate
 * symmetry.  Component-B candidates may vectorise independent butterfly
 * combines and the final bound reduction, but do not alter twiddle order,
 * rounding, threshold semantics, or first-accept sampling.
 */

/**
 * @brief 16-bit complex integer type (real, imag)
 */
typedef struct { int16_t re, im; } ci16_t;

/**
 * @brief Scaling factor used during FFT staging to avoid overflow. The
 *        static twiddle table is scaled accordingly.
 */
#define FFT_I16_FACTOR  256LL    /* 2^8 */
#define FFT_I16_SHIFT   8


/**
 * @brief Precomputed int16 twiddle factors in the exact order consumed by
 *        the fixed-size real FFT specialisations.
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
 * @brief Compute forward FFT of int16 input and produce int16 real/imag
 *        outputs.
 * @param[in]  a       Input array of length RL_KEM_N (int16_t).
 * @param[out] out_re  Output real parts (length RL_KEM_N).
 * @param[out] out_im  Output imaginary parts (length RL_KEM_N).
 *
 * The transform length is fixed by `RL_KEM_N`; the public `n` argument is
 * retained for compatibility with the existing API.
 */
void fft_forward_int16(const int16_t *a, int16_t *out_re, int16_t *out_im);

/**
 * @brief Check whether every squared spectral component stays below `bound`.
 * @param[in] a Input array of length RL_KEM_N (int16_t).
 * @param[in] bound Exclusive upper bound for re^2 + im^2.
 * @return 1 if every spectral component is < bound, otherwise 0.
 */
int fft_within_bound_int16(const int16_t *a, int32_t bound);


/**
 * @brief Compute the unique half-spectrum of a real int16 input polynomial.
 * @param[in] a Input array. Elements are read with the provided stride.
 * @param[in] stride Distance between consecutive input samples.
 * @param[in] n Requested transform length; current implementations use
 *        `RL_KEM_N` and keep this parameter for compatibility.
 * @param[out] out Output array of length `RL_KEM_N_Half`.
 */
void real_fft_unique_raw_i16(const int16_t *a, int stride, int n, ci16_t *out);
#ifdef POLARLAC_B_TEST_HOOKS
int polarlac_b_test_bound_from_unique(const ci16_t *uniq, int count,
                                      int32_t bound);
#endif

#endif // FFT_H
