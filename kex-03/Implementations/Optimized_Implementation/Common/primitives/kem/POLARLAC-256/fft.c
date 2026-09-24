/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
File Description: Implements integer fixed-point AVX2 FFT-based spectral rejection.
*/

#include "fft.h"

#include <string.h>

#ifndef __AVX2__
#error "This AVX2 implementation must be compiled with -mavx2."
#endif

#include <immintrin.h>
#define FFT_I16_FACTOR  256LL    /* 2^6? kept as in reference */
#define FFT_I16_SHIFT   8

/**
 * @brief Precomputed twiddle factors (int16) used by the small FFT
 *        implementation. This table contains RL_KEM_N/1 entries and is
 *        arranged to match the indexing strategy used by `ntt_fwd_i16`.
 */
static const ci16_t g_twiddle_int16_static[512] POLARLAC_FFT_ALIGN64 = {
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
    {-256, 3},
    {256, 2},
    {-2, 256},
    {180, 182},
    {-182, 180},
    {236, 99},
    {-99, 236},
    {97, 237},
    {-237, 97},
    {251, 51},
    {-51, 251},
    {141, 214},
    {-214, 141},
    {212, 144},
    {-144, 212},
    {48, 251},
    {-251, 48},
    {255, 27},
    {-27, 255},
    {161, 199},
    {-199, 161},
    {225, 122},
    {-122, 225},
    {73, 245},
    {-245, 73},
    {245, 76},
    {-76, 245},
    {119, 227},
    {-227, 119},
    {197, 164},
    {-164, 197},
    {24, 255},
    {-255, 24},
    {256, 14},
    {-14, 256},
    {171, 191},
    {-191, 171},
    {231, 111},
    {-111, 231},
    {85, 242},
    {-242, 85},
    {248, 64},
    {-64, 248},
    {130, 220},
    {-220, 130},
    {205, 154},
    {-154, 205},
    {36, 253},
    {-253, 36},
    {253, 39},
    {-39, 253},
    {151, 207},
    {-207, 151},
    {219, 133},
    {-133, 219},
    {61, 249},
    {-249, 61},
    {241, 88},
    {-88, 241},
    {108, 232},
    {-232, 108},
    {189, 173},
    {-173, 189},
    {11, 256},
    {-256, 11},
    {256, 8},
    {-8, 256},
    {175, 186},
    {-186, 175},
    {233, 105},
    {-105, 233},
    {91, 239},
    {-239, 91},
    {249, 58},
    {-58, 249},
    {136, 217},
    {-217, 136},
    {208, 149},
    {-149, 208},
    {42, 252},
    {-252, 42},
    {254, 33},
    {-33, 254},
    {156, 203},
    {-203, 156},
    {222, 128},
    {-128, 222},
    {67, 247},
    {-247, 67},
    {243, 82},
    {-82, 243},
    {114, 229},
    {-229, 114},
    {193, 168},
    {-168, 193},
    {17, 255},
    {-255, 17},
    {255, 20},
    {-20, 255},
    {166, 195},
    {-195, 166},
    {228, 117},
    {-117, 228},
    {79, 244},
    {-244, 79},
    {246, 70},
    {-70, 246},
    {125, 224},
    {-224, 125},
    {201, 159},
    {-159, 201},
    {30, 254},
    {-254, 30},
    {252, 45},
    {-45, 252},
    {146, 210},
    {-210, 146},
    {215, 138},
    {-138, 215},
    {55, 250},
    {-250, 55},
    {238, 94},
    {-94, 238},
    {102, 235},
    {-235, 102},
    {184, 178},
    {-178, 184},
    {5, 256},
    {-256, 5},
    {256, 5},
    {-5, 256},
    {178, 184},
    {-184, 178},
    {235, 102},
    {-102, 235},
    {94, 238},
    {-238, 94},
    {250, 55},
    {-55, 250},
    {138, 215},
    {-215, 138},
    {210, 146},
    {-146, 210},
    {45, 252},
    {-252, 45},
    {254, 30},
    {-30, 254},
    {159, 201},
    {-201, 159},
    {224, 125},
    {-125, 224},
    {70, 246},
    {-246, 70},
    {244, 79},
    {-79, 244},
    {117, 228},
    {-228, 117},
    {195, 166},
    {-166, 195},
    {20, 255},
    {-255, 20},
    {255, 17},
    {-17, 255},
    {168, 193},
    {-193, 168},
    {229, 114},
    {-114, 229},
    {82, 243},
    {-243, 82},
    {247, 67},
    {-67, 247},
    {128, 222},
    {-222, 128},
    {203, 156},
    {-156, 203},
    {33, 254},
    {-254, 33},
    {252, 42},
    {-42, 252},
    {149, 208},
    {-208, 149},
    {217, 136},
    {-136, 217},
    {58, 249},
    {-249, 58},
    {239, 91},
    {-91, 239},
    {105, 233},
    {-233, 105},
    {186, 175},
    {-175, 186},
    {8, 256},
    {-256, 8},
    {256, 11},
    {-11, 256},
    {173, 189},
    {-189, 173},
    {232, 108},
    {-108, 232},
    {88, 241},
    {-241, 88},
    {249, 61},
    {-61, 249},
    {133, 219},
    {-219, 133},
    {207, 151},
    {-151, 207},
    {39, 253},
    {-253, 39},
    {253, 36},
    {-36, 253},
    {154, 205},
    {-205, 154},
    {220, 130},
    {-130, 220},
    {64, 248},
    {-248, 64},
    {242, 85},
    {-85, 242},
    {111, 231},
    {-231, 111},
    {191, 171},
    {-171, 191},
    {14, 256},
    {-256, 14},
    {255, 24},
    {-24, 255},
    {164, 197},
    {-197, 164},
    {227, 119},
    {-119, 227},
    {76, 245},
    {-245, 76},
    {245, 73},
    {-73, 245},
    {122, 225},
    {-225, 122},
    {199, 161},
    {-161, 199},
    {27, 255},
    {-255, 27},
    {251, 48},
    {-48, 251},
    {144, 212},
    {-212, 144},
    {214, 141},
    {-141, 214},
    {51, 251},
    {-251, 51},
    {237, 97},
    {-97, 237},
    {99, 236},
    {-236, 99},
    {182, 180},
    {-180, 182},
    {2, 256},
    {-256, 2}
};


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
    return (ci16_t){ (int16_t)rsh32(re, FFT_I16_SHIFT), (int16_t)rsh32(im, FFT_I16_SHIFT) };
}

static const uint16_t g_bitrev8[RL_KEM_N_Half] POLARLAC_FFT_ALIGN64 = {
      0, 128,  64, 192,  32, 160,  96, 224,  16, 144,  80, 208,  48, 176, 112, 240,
      8, 136,  72, 200,  40, 168, 104, 232,  24, 152,  88, 216,  56, 184, 120, 248,
      4, 132,  68, 196,  36, 164, 100, 228,  20, 148,  84, 212,  52, 180, 116, 244,
     12, 140,  76, 204,  44, 172, 108, 236,  28, 156,  92, 220,  60, 188, 124, 252,
      2, 130,  66, 194,  34, 162,  98, 226,  18, 146,  82, 210,  50, 178, 114, 242,
     10, 138,  74, 202,  42, 170, 106, 234,  26, 154,  90, 218,  58, 186, 122, 250,
      6, 134,  70, 198,  38, 166, 102, 230,  22, 150,  86, 214,  54, 182, 118, 246,
     14, 142,  78, 206,  46, 174, 110, 238,  30, 158,  94, 222,  62, 190, 126, 254,
      1, 129,  65, 193,  33, 161,  97, 225,  17, 145,  81, 209,  49, 177, 113, 241,
      9, 137,  73, 201,  41, 169, 105, 233,  25, 153,  89, 217,  57, 185, 121, 249,
      5, 133,  69, 197,  37, 165, 101, 229,  21, 149,  85, 213,  53, 181, 117, 245,
     13, 141,  77, 205,  45, 173, 109, 237,  29, 157,  93, 221,  61, 189, 125, 253,
      3, 131,  67, 195,  35, 163,  99, 227,  19, 147,  83, 211,  51, 179, 115, 243,
     11, 139,  75, 203,  43, 171, 107, 235,  27, 155,  91, 219,  59, 187, 123, 251,
      7, 135,  71, 199,  39, 167, 103, 231,  23, 151,  87, 215,  55, 183, 119, 247,
     15, 143,  79, 207,  47, 175, 111, 239,  31, 159,  95, 223,  63, 191, 127, 255
};

static inline void fft_pack_halves_bitrev_i16(const int16_t *a, int stride, ci16_t *dst)
{
    for (int i = 0; i < RL_KEM_N_Half; ++i) {
        unsigned j = g_bitrev8[i];
        dst[j].re = (int16_t)(a[i * stride] << FFT_I16_SHIFT);
        dst[j].im = (int16_t)(a[(i + RL_KEM_N_Half) * stride] << FFT_I16_SHIFT);
    }
}

static inline void fft_combine_scalar(ci16_t *out, const ci16_t *even, const ci16_t *odd, const ci16_t *twiddle, int count)
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
    return _mm256_or_si256(_mm256_and_si256(re, mask16), _mm256_slli_epi32(im, 16));
}

static inline __m256i avx2_swap_complex_i16(__m256i x)
{
    const __m256i mask = _mm256_setr_epi8(
         2,  3,  0,  1,  6,  7,  4,  5,
        10, 11,  8,  9, 14, 15, 12, 13,
         2,  3,  0,  1,  6,  7,  4,  5,
        10, 11,  8,  9, 14, 15, 12, 13);
    return _mm256_shuffle_epi8(x, mask);
}

static inline __m256i avx2_negate_im_lanes_i16(__m256i x)
{
    const __m256i sign = _mm256_setr_epi16(1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1,1,-1);
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
    _mm_store_si128((__m128i *)(void *)(out + 0),  lo_0);
    _mm_store_si128((__m128i *)(void *)(out + 4),  hi_0);
    _mm_store_si128((__m128i *)(void *)(out + 8),  lo_1);
    _mm_store_si128((__m128i *)(void *)(out + 12), hi_1);
}

static inline void fft_combine_avx2(ci16_t *out, const ci16_t *even, const ci16_t *odd, const ci16_t *twiddle, int count)
{
    int k = 0;
    for (; k + 8 <= count; k += 8) {
        __m256i even_pair = _mm256_load_si256((const __m256i *)(const void *)(even + k));
        __m256i odd_pair  = _mm256_load_si256((const __m256i *)(const void *)(odd + k));
        __m256i tw_pair   = _mm256_load_si256((const __m256i *)(const void *)(twiddle + k));
        __m256i t_re = avx2_rsh_epi32(_mm256_madd_epi16(odd_pair, avx2_negate_im_lanes_i16(tw_pair)), FFT_I16_SHIFT);
        __m256i t_im = avx2_rsh_epi32(_mm256_madd_epi16(odd_pair, avx2_swap_complex_i16(tw_pair)), FFT_I16_SHIFT);
        __m256i t_pair = avx2_pack_complex_i32(t_re, t_im);
        __m256i sum_pair = _mm256_add_epi16(even_pair, t_pair);
        __m256i diff_pair = _mm256_sub_epi16(even_pair, t_pair);
        avx2_store_interleaved_complex8(out + (k << 1), sum_pair, diff_pair);
    }
    if (k < count) {
        fft_combine_scalar(out + (k << 1), even + k, odd + k, twiddle + k, count - k);
    }
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
    const int group_size = RL_KEM_N_Half;
    const int half = group_size >> 1;
    const ci16_t *even = src;
    const ci16_t *odd = src + half;
    const ci16_t *twiddle = g_twiddle_int16_static + group_size;
    const __m256i bound_m1 = _mm256_set1_epi32(bound - 1);
    __m256i reject = _mm256_setzero_si256();
    for (int k = 0; k < half; k += 8) {
        __m256i even_pair = _mm256_load_si256((const __m256i *)(const void *)(even + k));
        __m256i odd_pair  = _mm256_load_si256((const __m256i *)(const void *)(odd + k));
        __m256i tw_pair   = _mm256_load_si256((const __m256i *)(const void *)(twiddle + k));
        __m256i t_re = avx2_rsh_epi32(_mm256_madd_epi16(odd_pair, avx2_negate_im_lanes_i16(tw_pair)), FFT_I16_SHIFT);
        __m256i t_im = avx2_rsh_epi32(_mm256_madd_epi16(odd_pair, avx2_swap_complex_i16(tw_pair)), FFT_I16_SHIFT);
        __m256i t_pair = avx2_pack_complex_i32(t_re, t_im);
        __m256i sum_pair = _mm256_add_epi16(even_pair, t_pair);
        __m256i diff_pair = _mm256_sub_epi16(even_pair, t_pair);
        reject = _mm256_or_si256(reject, avx2_reject_mask_from_complex_pair(sum_pair, bound_m1));
        reject = _mm256_or_si256(reject, avx2_reject_mask_from_complex_pair(diff_pair, bound_m1));
    }
    return _mm256_testz_si256(reject, reject) ? 1 : 0;
}

static inline void fft_stage_iterative_i16(const ci16_t *src, ci16_t *dst, int group_size)
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

static inline void real_fft_unique_raw_i16_root_aligned(const int16_t *a, int stride, ci16_t *out)
{
    ci16_t buf0[RL_KEM_N_Half] POLARLAC_FFT_ALIGN64;
    ci16_t buf1[RL_KEM_N_Half] POLARLAC_FFT_ALIGN64;
    fft_pack_halves_bitrev_i16(a, stride, buf0);
    const ci16_t *src = buf0;
    ci16_t *dst = buf1;
    for (int group = 2; group <= RL_KEM_N_Half; group <<= 1) {
        fft_stage_iterative_i16(src, dst, group);
        const ci16_t *tmp = src;
        src = dst;
        dst = (ci16_t *)tmp;
    }
    if (src != out) {
        memcpy(out, src, RL_KEM_N_Half * sizeof(ci16_t));
    }
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

    fft_stage_iterative_i16(buf0, buf1,   2);
    fft_stage_iterative_i16(buf1, buf0,   4);
    fft_stage_iterative_i16(buf0, buf1,   8);
    fft_stage_iterative_i16(buf1, buf0,  16);
    fft_stage_iterative_i16(buf0, buf1,  32);
    fft_stage_iterative_i16(buf1, buf0,  64);
    fft_stage_iterative_i16(buf0, buf1, 128);

    return fft_final_stage_bound_check_avx2(buf1, bound);
}

int fft_within_bound_int16(const int16_t *a, int32_t bound)
{
    return fft_within_bound_int16_avx2(a, bound);
}
