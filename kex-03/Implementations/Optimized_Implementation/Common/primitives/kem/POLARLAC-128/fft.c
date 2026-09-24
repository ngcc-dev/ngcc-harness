/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
File Description: Implements integer fixed-point AVX2 FFT-based spectral rejection for POLARLAC-128.
*/

#include "fft.h"

#include <string.h>

#ifndef __AVX2__
#error "This AVX2 implementation must be compiled with -mavx2."
#endif

#include <immintrin.h>
#define FFT_I16_FACTOR  256LL    /* 2^8 */
#define FFT_I16_SHIFT   8

/**
 * @brief Precomputed twiddle factors (int16) used by the small FFT
 *        implementation. This table contains RL_KEM_N/1 entries and is
 *        arranged to match the indexing strategy used by `ntt_fwd_i16`.
 */
static const ci16_t g_twiddle_int16_static[256] POLARLAC_FFT_ALIGN64 = {
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
 * @brief Integer fixed-point FFT implementation used by the POLARLAC-128
 *        rejection sampler.
 *
 * The implementation is specialized for RL_KEM_N=256.  It avoids floating
 * point arithmetic completely: inputs, twiddle factors and working buffers are
 * int16_t, while complex products and squared magnitudes use int32_t.
 *
 * Compared with the previous macro-recursive half-spectrum FFT, this version
 * uses an iterative folded-packing structure:
 *
 *   1. pack a[0..127] and a[128..255] into 128 fixed-point complex values;
 *   2. place the packed values in 7-bit bit-reversed order;
 *   3. apply seven out-of-place radix-2 stages over aligned buffers;
 *   4. perform the bound check over the unique half-spectrum.
 *
 * The stage recurrence is identical to the former recursive implementation:
 *
 *   out[2k]   = even[k] + tw[k] * odd[k]
 *   out[2k+1] = even[k] - tw[k] * odd[k]
 *
 * but it is now scheduled as regular stage loops, making the large stages
 * friendlier to aligned AVX2 loads/stores and vpmaddwd-based complex products.
 */

#if RL_KEM_N != 256
#error "This integer AVX2 FFT kernel is specialized for POLARLAC-128 with RL_KEM_N=256."
#endif

static inline int32_t rsh32(int32_t x, int s)
{
    int32_t half = 1 << (s - 1);
    int32_t sign = (int32_t)((uint32_t)x >> 31);

    return (x + half - sign) >> s;
}

static inline ci16_t ci16_mul_tw(ci16_t data, ci16_t tw)
{
    int32_t t0 = (int32_t)tw.re * (int32_t)data.re;
    int32_t t2 = (int32_t)tw.im * (int32_t)data.im;
    int32_t t1 = ((int32_t)tw.re + (int32_t)tw.im) *
                 ((int32_t)data.re + (int32_t)data.im);

    int32_t re = t0 - t2;
    int32_t im = t1 - t0 - t2;

    return (ci16_t){
        (int16_t)rsh32(re, FFT_I16_SHIFT),
        (int16_t)rsh32(im, FFT_I16_SHIFT)
    };
}

static const uint8_t g_bitrev7[RL_KEM_N_Half] POLARLAC_FFT_ALIGN64 = {
      0,  64,  32,  96,  16,  80,  48, 112,   8,  72,  40, 104,  24,  88,  56, 120,
      4,  68,  36, 100,  20,  84,  52, 116,  12,  76,  44, 108,  28,  92,  60, 124,
      2,  66,  34,  98,  18,  82,  50, 114,  10,  74,  42, 106,  26,  90,  58, 122,
      6,  70,  38, 102,  22,  86,  54, 118,  14,  78,  46, 110,  30,  94,  62, 126,
      1,  65,  33,  97,  17,  81,  49, 113,   9,  73,  41, 105,  25,  89,  57, 121,
      5,  69,  37, 101,  21,  85,  53, 117,  13,  77,  45, 109,  29,  93,  61, 125,
      3,  67,  35,  99,  19,  83,  51, 115,  11,  75,  43, 107,  27,  91,  59, 123,
      7,  71,  39, 103,  23,  87,  55, 119,  15,  79,  47, 111,  31,  95,  63, 127
};

static inline void fft_pack_halves_bitrev_i16(
    const int16_t *a,
    int stride,
    ci16_t *dst
)
{
    for (int i = 0; i < RL_KEM_N_Half; ++i) {
        int j = g_bitrev7[i];
        dst[j].re = (int16_t)(a[i * stride] << FFT_I16_SHIFT);
        dst[j].im = (int16_t)(a[(i + RL_KEM_N_Half) * stride] << FFT_I16_SHIFT);
    }
}

static inline void fft_combine_scalar(
    ci16_t *out,
    const ci16_t *even,
    const ci16_t *odd,
    const ci16_t *twiddle,
    int count
)
{
    for (int k = 0; k < count; ++k) {
        ci16_t t = ci16_mul_tw(odd[k], twiddle[k]);

        out[k << 1].re = (int16_t)((int32_t)even[k].re + (int32_t)t.re);
        out[k << 1].im = (int16_t)((int32_t)even[k].im + (int32_t)t.im);
        out[(k << 1) + 1].re = (int16_t)((int32_t)even[k].re - (int32_t)t.re);
        out[(k << 1) + 1].im = (int16_t)((int32_t)even[k].im - (int32_t)t.im);
    }
}

static inline __m256i avx2_rsh_epi32(__m256i x, int s)
{
    const __m256i half = _mm256_set1_epi32(1 << (s - 1));
    __m256i sign = _mm256_srli_epi32(x, 31);

    x = _mm256_add_epi32(x, half);
    x = _mm256_sub_epi32(x, sign);
    return _mm256_srai_epi32(x, s);
}

static inline __m256i avx2_pair_re_epi32(__m256i pair)
{
    return _mm256_srai_epi32(_mm256_slli_epi32(pair, 16), 16);
}

static inline __m256i avx2_pair_im_epi32(__m256i pair)
{
    return _mm256_srai_epi32(pair, 16);
}

static inline __m256i avx2_pack_complex_i32(__m256i re, __m256i im)
{
    const __m256i mask16 = _mm256_set1_epi32(0x0000ffff);
    __m256i re16 = _mm256_and_si256(re, mask16);
    __m256i im16 = _mm256_slli_epi32(im, 16);

    return _mm256_or_si256(re16, im16);
}

static inline __m256i avx2_swap_complex_i16(__m256i x)
{
    const __m256i mask = _mm256_setr_epi8(
         2,  3,  0,  1,  6,  7,  4,  5,
        10, 11,  8,  9, 14, 15, 12, 13,
         2,  3,  0,  1,  6,  7,  4,  5,
        10, 11,  8,  9, 14, 15, 12, 13
    );
    return _mm256_shuffle_epi8(x, mask);
}

static inline __m256i avx2_negate_im_lanes_i16(__m256i x)
{
    const __m256i sign = _mm256_setr_epi16(
         1, -1,  1, -1,  1, -1,  1, -1,
         1, -1,  1, -1,  1, -1,  1, -1
    );
    return _mm256_sign_epi16(x, sign);
}

static inline void avx2_store_interleaved_complex8(ci16_t *out, __m256i sum_pair, __m256i diff_pair)
{
    __m256i lo = _mm256_unpacklo_epi32(sum_pair, diff_pair);
    __m256i hi = _mm256_unpackhi_epi32(sum_pair, diff_pair);

    __m128i lo_0 = _mm256_castsi256_si128(lo);
    __m128i hi_0 = _mm256_castsi256_si128(hi);
    __m128i lo_1 = _mm256_extracti128_si256(lo, 1);
    __m128i hi_1 = _mm256_extracti128_si256(hi, 1);

    _mm_store_si128((__m128i *)(void *)(out + 0),  lo_0); /* sum0,diff0,sum1,diff1 */
    _mm_store_si128((__m128i *)(void *)(out + 4),  hi_0); /* sum2,diff2,sum3,diff3 */
    _mm_store_si128((__m128i *)(void *)(out + 8),  lo_1); /* sum4,diff4,sum5,diff5 */
    _mm_store_si128((__m128i *)(void *)(out + 12), hi_1); /* sum6,diff6,sum7,diff7 */
}

static inline void fft_combine_avx2(
    ci16_t *out,
    const ci16_t *even,
    const ci16_t *odd,
    const ci16_t *twiddle,
    int count
)
{
    int k = 0;

    for (; k + 8 <= count; k += 8) {
        __m256i even_pair = _mm256_load_si256((const __m256i *)(const void *)(even + k));
        __m256i odd_pair  = _mm256_load_si256((const __m256i *)(const void *)(odd + k));
        __m256i tw_pair   = _mm256_load_si256((const __m256i *)(const void *)(twiddle + k));

        __m256i t_re = avx2_rsh_epi32(
            _mm256_madd_epi16(odd_pair, avx2_negate_im_lanes_i16(tw_pair)),
            FFT_I16_SHIFT
        );
        __m256i t_im = avx2_rsh_epi32(
            _mm256_madd_epi16(odd_pair, avx2_swap_complex_i16(tw_pair)),
            FFT_I16_SHIFT
        );
        __m256i t_pair = avx2_pack_complex_i32(t_re, t_im);

        __m256i sum_pair = _mm256_add_epi16(even_pair, t_pair);
        __m256i diff_pair = _mm256_sub_epi16(even_pair, t_pair);

        avx2_store_interleaved_complex8(out + (k << 1), sum_pair, diff_pair);
    }

    if (k < count) {
        fft_combine_scalar(out + (k << 1), even + k, odd + k, twiddle + k, count - k);
    }
}

static inline int fft_bound_check_unique_avx2(const ci16_t *uniq, int32_t bound)
{
    const __m256i bound_m1 = _mm256_set1_epi32(bound - 1);
    __m256i reject = _mm256_setzero_si256();
    int i = 0;

    for (; i + 8 <= RL_KEM_N_Half; i += 8) {
        __m256i pair = _mm256_load_si256((const __m256i *)(const void *)(uniq + i));
        __m256i re = avx2_rsh_epi32(avx2_pair_re_epi32(pair), FFT_I16_SHIFT);
        __m256i im = avx2_rsh_epi32(avx2_pair_im_epi32(pair), FFT_I16_SHIFT);
        __m256i comp = _mm256_add_epi32(_mm256_mullo_epi32(re, re), _mm256_mullo_epi32(im, im));

        reject = _mm256_or_si256(reject, _mm256_cmpgt_epi32(comp, bound_m1));
    }

    if (!_mm256_testz_si256(reject, reject)) {
        return 0;
    }

    for (; i < RL_KEM_N_Half; ++i) {
        int32_t re = rsh32((int32_t)uniq[i].re, FFT_I16_SHIFT);
        int32_t im = rsh32((int32_t)uniq[i].im, FFT_I16_SHIFT);
        int32_t comp = re * re + im * im;

        if (comp >= bound) {
            return 0;
        }
    }

    return 1;
}

static inline __m256i avx2_reject_mask_from_complex_pair(__m256i pair, __m256i bound_m1)
{
    __m256i re = avx2_rsh_epi32(avx2_pair_re_epi32(pair), FFT_I16_SHIFT);
    __m256i im = avx2_rsh_epi32(avx2_pair_im_epi32(pair), FFT_I16_SHIFT);
    __m256i comp = _mm256_add_epi32(_mm256_mullo_epi32(re, re), _mm256_mullo_epi32(im, im));

    return _mm256_cmpgt_epi32(comp, bound_m1);
}

static inline int fft_final_stage_bound_check_avx2(const ci16_t *src, int32_t bound)
{
    const __m256i bound_m1 = _mm256_set1_epi32(bound - 1);
    const ci16_t *even = src;
    const ci16_t *odd = src + 64;
    const ci16_t *twiddle = g_twiddle_int16_static + 128;
    __m256i reject = _mm256_setzero_si256();

    for (int k = 0; k < 64; k += 8) {
        __m256i even_pair = _mm256_load_si256((const __m256i *)(const void *)(even + k));
        __m256i odd_pair  = _mm256_load_si256((const __m256i *)(const void *)(odd + k));
        __m256i tw_pair   = _mm256_load_si256((const __m256i *)(const void *)(twiddle + k));

        __m256i t_re = avx2_rsh_epi32(
            _mm256_madd_epi16(odd_pair, avx2_negate_im_lanes_i16(tw_pair)),
            FFT_I16_SHIFT
        );
        __m256i t_im = avx2_rsh_epi32(
            _mm256_madd_epi16(odd_pair, avx2_swap_complex_i16(tw_pair)),
            FFT_I16_SHIFT
        );
        __m256i t_pair = avx2_pack_complex_i32(t_re, t_im);

        __m256i sum_pair = _mm256_add_epi16(even_pair, t_pair);
        __m256i diff_pair = _mm256_sub_epi16(even_pair, t_pair);

        reject = _mm256_or_si256(reject, avx2_reject_mask_from_complex_pair(sum_pair, bound_m1));
        reject = _mm256_or_si256(reject, avx2_reject_mask_from_complex_pair(diff_pair, bound_m1));
    }

    return _mm256_testz_si256(reject, reject) ? 1 : 0;
}

static inline void fft_stage_iterative_i16(
    const ci16_t *src,
    ci16_t *dst,
    int group_size
)
{
    int half = group_size >> 1;

    for (int base = 0; base < RL_KEM_N_Half; base += group_size) {
        const ci16_t *even = src + base;
        const ci16_t *odd = src + base + half;
        ci16_t *out = dst + base;
        const ci16_t *twiddle = g_twiddle_int16_static + group_size;

        fft_combine_avx2(out, even, odd, twiddle, half);
    }
}

static inline void real_fft_unique_raw_i16_root_aligned(
    const int16_t *a,
    int stride,
    ci16_t *out
)
{
    ci16_t buf0[RL_KEM_N_Half] POLARLAC_FFT_ALIGN64;
    ci16_t buf1[RL_KEM_N_Half] POLARLAC_FFT_ALIGN64;

    fft_pack_halves_bitrev_i16(a, stride, buf0);

    /*
     * Seven fixed stages are used for RL_KEM_N_Half=128.  The last stage
     * writes directly to the caller-provided aligned output buffer, avoiding
     * the extra full-spectrum copy that would otherwise be needed after a
     * generic ping-pong loop.
     */
    fft_stage_iterative_i16(buf0, buf1,   2);
    fft_stage_iterative_i16(buf1, buf0,   4);
    fft_stage_iterative_i16(buf0, buf1,   8);
    fft_stage_iterative_i16(buf1, buf0,  16);
    fft_stage_iterative_i16(buf0, buf1,  32);
    fft_stage_iterative_i16(buf1, buf0,  64);
    fft_stage_iterative_i16(buf0, out,  128);
}

void real_fft_unique_raw_i16(const int16_t *a, int stride, int n, ci16_t *out)
{
    ci16_t tmp[RL_KEM_N_Half] POLARLAC_FFT_ALIGN64;

    (void)n;
    real_fft_unique_raw_i16_root_aligned(a, stride, tmp);
    memcpy(out, tmp, sizeof(tmp));
}

void fft_forward_int16(const int16_t *a, int16_t *out_re, int16_t *out_im)
{
    ci16_t uniq[RL_KEM_N_Half] POLARLAC_FFT_ALIGN64;

    real_fft_unique_raw_i16_root_aligned(a, 1, uniq);

    for (int i = 0; i < RL_KEM_N_Half; ++i) {
        int16_t re = (int16_t)rsh32((int32_t)uniq[i].re, FFT_I16_SHIFT);
        int16_t im = (int16_t)rsh32((int32_t)uniq[i].im, FFT_I16_SHIFT);
        int mirror = RL_KEM_N - 1 - i;

        out_re[i] = re;
        out_im[i] = im;
        out_re[mirror] = re;
        out_im[mirror] = (int16_t)(-(int32_t)im);
    }
}

int fft_within_bound_int16_avx2(const int16_t *a, int32_t bound)
{
    ci16_t buf0[RL_KEM_N_Half] POLARLAC_FFT_ALIGN64;
    ci16_t buf1[RL_KEM_N_Half] POLARLAC_FFT_ALIGN64;

    fft_pack_halves_bitrev_i16(a, 1, buf0);

    /*
     * The rejection path fuses the final 128-point stage with the spectral
     * bound check.  This avoids materializing the full unique spectrum when
     * the caller only needs the accept/reject bit.
     */
    fft_stage_iterative_i16(buf0, buf1,  2);
    fft_stage_iterative_i16(buf1, buf0,  4);
    fft_stage_iterative_i16(buf0, buf1,  8);
    fft_stage_iterative_i16(buf1, buf0, 16);
    fft_stage_iterative_i16(buf0, buf1, 32);
    fft_stage_iterative_i16(buf1, buf0, 64);

    return fft_final_stage_bound_check_avx2(buf0, bound);
}

int fft_within_bound_int16(const int16_t *a, int32_t bound)
{
    return fft_within_bound_int16_avx2(a, bound);
}

/**
 * End of fft.c
 */
