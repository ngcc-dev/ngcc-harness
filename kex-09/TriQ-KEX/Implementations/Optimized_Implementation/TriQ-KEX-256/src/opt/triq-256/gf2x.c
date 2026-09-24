/**
 * @file gf2x.c
 * @brief Native-size AVX2 implementation of multiplication of two polynomials.
 */

#define a1_times_a2 base_a1_times_a2
#define reduce base_reduce
#define karat_mult_small base_karat_mult_small
#define karat_mul_rec_25 base_karat_mul_rec_25
#define karat_mul_rec_26 base_karat_mul_rec_26
#define karat_mul_rec_31 base_karat_mul_rec_31
#define karat_mul_rec_51 base_karat_mul_rec_51
#define karat_mul_vec base_karat_mul_vec
#define karat_mult_5 base_karat_mult_5
#define toom_3_mult base_toom_3_mult
#define vect_mul base_vect_mul
#include "gf2x_base.c"
#undef a1_times_a2
#undef reduce
#undef karat_mult_small
#undef karat_mul_rec_25
#undef karat_mul_rec_26
#undef karat_mul_rec_31
#undef karat_mul_rec_51
#undef karat_mul_vec
#undef karat_mult_5
#undef toom_3_mult
#undef vect_mul

#undef T_TM3R_3W
#undef T_TM3R
#undef tTM3R
#undef T_TM3R_3W_256
#undef T_TM3R_3W_64
#undef T_5W
#undef T_5W_256
#undef T2_5W_256
#undef t5

#include <stddef.h>
#include <stdint.h>

#define VEC_N_MULT_256        CEIL_DIVIDE(PARAM_N_MULT, 256)
#define VEC_N_256_SIZE_BYTES  (VEC_N_256_SIZE_64 * 8)
#define OUT_PAD_BYTES         (VEC_N_256_SIZE_BYTES - VEC_N_SIZE_BYTES)
#define TOOM3_INPUT_BYTES     (TOOM3_INPUT_256 * sizeof(__m256i))
#define TOOM3_SLICE_256       CEIL_DIVIDE(CEIL_DIVIDE(PARAM_N_MULT, 3), 256)
#define TOOM3_INPUT_256       (3 * TOOM3_SLICE_256)
#define TOOM3_PRODUCT_256     (2 * TOOM3_INPUT_256)
#define TOOM3_EXT_256         (TOOM3_SLICE_256 + 2)
#define TOOM3_EXT_PRODUCT_256 (2 * TOOM3_EXT_256)
#define TOOM5_SLICE_256       CEIL_DIVIDE(TOOM3_EXT_256, 5)
#define TOOM5_INPUT_256       (5 * TOOM5_SLICE_256)
#define TOOM5_PRODUCT_256     (2 * TOOM5_INPUT_256)

_Static_assert(TOOM3_INPUT_256 >= VEC_N_MULT_256, "native input padding is too small");
_Static_assert(TOOM5_INPUT_256 >= TOOM3_EXT_256, "Toom-5 input padding is too small");
_Static_assert(TOOM5_PRODUCT_256 >= TOOM3_EXT_PRODUCT_256, "Toom-5 product padding is too small");
_Static_assert(OUT_PAD_BYTES >= 0, "output padding size is negative");
_Static_assert(TOOM3_INPUT_BYTES >= VEC_N_SIZE_BYTES, "native input byte padding is too small");

static __m256i native_a1_times_a2[TOOM3_PRODUCT_256];
static __m256i native_a_pad[TOOM3_INPUT_256];
static __m256i native_b_pad[TOOM3_INPUT_256];

static inline void copy_and_clear_input_padding(__m256i *dst, const __m256i *src) {
    uint8_t *dst_bytes = (uint8_t *)dst;

    memcpy(dst_bytes, src, VEC_N_SIZE_BYTES);
    dst_bytes[VEC_N_SIZE_BYTES - 1] &= (uint8_t)BITMASK(PARAM_N, 8);
    memset(dst_bytes + VEC_N_SIZE_BYTES, 0, TOOM3_INPUT_BYTES - VEC_N_SIZE_BYTES);
}

static inline void reduce(__m256i *o, const __m256i *a256) {
    __m256i r256, carry256;
    const uint64_t *a = (const uint64_t *)a256;
    uint64_t *out64 = (uint64_t *)o;
    const int32_t dec64 = PARAM_N & 0x3f;
    const int32_t d0 = WORD - dec64;
    int32_t i, i2;

    for (i = LAST64; i < (PARAM_N >> 5) - 4; i += 4) {
        r256 = _mm256_lddqu_si256((__m256i const *)(&a[i]));
        r256 = _mm256_srli_epi64(r256, dec64);
        carry256 = _mm256_lddqu_si256((__m256i const *)(&a[i + 1]));
        carry256 = _mm256_slli_epi64(carry256, d0);
        r256 ^= carry256;
        i2 = (i - LAST64) >> 2;
        _mm256_storeu_si256((__m256i *)(&out64[i2 << 2]), a256[i2] ^ r256);
    }

    i = i - LAST64;
    for (; i < LAST64 + 1; i++) {
        const uint64_t r = (a[i + LAST64] >> dec64) ^ (a[i + LAST64 + 1] << d0);
        out64[i] = a[i] ^ r;
    }

    out64[LAST64] &= BITMASK(PARAM_N, 64);
#if OUT_PAD_BYTES > 0
    memset(((uint8_t *)o) + VEC_N_SIZE_BYTES, 0, OUT_PAD_BYTES);
#endif
}

static inline void karat_mult_small(__m256i *C, const __m256i *A, const __m256i *B, size_t n) {
    if (n <= 8) {
        __m256i A8[8] = {0};
        __m256i B8[8] = {0};
        __m256i R8[16];

        for (size_t i = 0; i < n; i++) {
            A8[i] = A[i];
            B8[i] = B[i];
        }

        karat_mult_8(R8, A8, B8);
        memcpy(C, R8, 2 * n * sizeof(__m256i));
        return;
    }

    __m256i A16[16] = {0};
    __m256i B16[16] = {0};
    __m256i R16[32];

    for (size_t i = 0; i < n; i++) {
        A16[i] = A[i];
        B16[i] = B[i];
    }

    karat_mult_16(R16, A16, B16);
    memcpy(C, R16, 2 * n * sizeof(__m256i));
}

static inline void karat_mul_vec(__m256i *C, const __m256i *A, const __m256i *B, size_t n) {
    if (n <= 16) {
        karat_mult_small(C, A, B, n);
        return;
    }

    const size_t n0 = n >> 1;
    const size_t n1 = n - n0;
    const size_t m = n0;
    const __m256i zero = _mm256_setzero_si256();
    __m256i z0[2 * n0];
    __m256i z2[2 * n1];
    __m256i zmid[2 * n1];
    __m256i ta[n1];
    __m256i tb[n1];

    karat_mul_vec(z0, A, B, n0);
    karat_mul_vec(z2, A + m, B + m, n1);

    for (size_t i = 0; i < n1; i++) {
        const __m256i alo = (i < n0) ? A[i] : zero;
        const __m256i blo = (i < n0) ? B[i] : zero;
        ta[i] = alo ^ A[m + i];
        tb[i] = blo ^ B[m + i];
    }

    karat_mul_vec(zmid, ta, tb, n1);

    memset(C, 0, 2 * n * sizeof(__m256i));
    for (size_t i = 0; i < 2 * n0; i++) {
        C[i] ^= z0[i];
    }
    for (size_t i = 0; i < 2 * n1; i++) {
        C[2 * m + i] ^= z2[i];
    }
    for (size_t i = 0; i < 2 * n1; i++) {
        const __m256i z0i = (i < 2 * n0) ? z0[i] : zero;
        C[m + i] ^= zmid[i] ^ z0i ^ z2[i];
    }
}

static inline void karat_mult_5(__m256i *Out, const __m256i *A, const __m256i *B) {
    const __m256i *a0 = A;
    const __m256i *a1 = a0 + TOOM5_SLICE_256;
    const __m256i *a2 = a1 + TOOM5_SLICE_256;
    const __m256i *a3 = a2 + TOOM5_SLICE_256;
    const __m256i *a4 = a3 + TOOM5_SLICE_256;
    const __m256i *b0 = B;
    const __m256i *b1 = b0 + TOOM5_SLICE_256;
    const __m256i *b2 = b1 + TOOM5_SLICE_256;
    const __m256i *b3 = b2 + TOOM5_SLICE_256;
    const __m256i *b4 = b3 + TOOM5_SLICE_256;

    static __m256i aa01[TOOM5_SLICE_256], bb01[TOOM5_SLICE_256], aa02[TOOM5_SLICE_256], bb02[TOOM5_SLICE_256];
    static __m256i aa03[TOOM5_SLICE_256], bb03[TOOM5_SLICE_256], aa04[TOOM5_SLICE_256], bb04[TOOM5_SLICE_256];
    static __m256i aa12[TOOM5_SLICE_256], bb12[TOOM5_SLICE_256], aa13[TOOM5_SLICE_256], bb13[TOOM5_SLICE_256];
    static __m256i aa14[TOOM5_SLICE_256], bb14[TOOM5_SLICE_256], aa23[TOOM5_SLICE_256], bb23[TOOM5_SLICE_256];
    static __m256i aa24[TOOM5_SLICE_256], bb24[TOOM5_SLICE_256], aa34[TOOM5_SLICE_256], bb34[TOOM5_SLICE_256];

    static __m256i D0[2 * TOOM5_SLICE_256], D1[2 * TOOM5_SLICE_256], D2[2 * TOOM5_SLICE_256];
    static __m256i D3[2 * TOOM5_SLICE_256], D4[2 * TOOM5_SLICE_256], D01[2 * TOOM5_SLICE_256];
    static __m256i D02[2 * TOOM5_SLICE_256], D03[2 * TOOM5_SLICE_256], D04[2 * TOOM5_SLICE_256];
    static __m256i D12[2 * TOOM5_SLICE_256], D13[2 * TOOM5_SLICE_256], D14[2 * TOOM5_SLICE_256];
    static __m256i D23[2 * TOOM5_SLICE_256], D24[2 * TOOM5_SLICE_256], D34[2 * TOOM5_SLICE_256];
    static __m256i ro256[TOOM5_PRODUCT_256];

    for (int32_t i = 0; i < TOOM5_SLICE_256; i++) {
        aa01[i] = a0[i] ^ a1[i];
        bb01[i] = b0[i] ^ b1[i];
        aa02[i] = a0[i] ^ a2[i];
        bb02[i] = b0[i] ^ b2[i];
        aa03[i] = a0[i] ^ a3[i];
        bb03[i] = b0[i] ^ b3[i];
        aa04[i] = a0[i] ^ a4[i];
        bb04[i] = b0[i] ^ b4[i];
        aa12[i] = a2[i] ^ a1[i];
        bb12[i] = b2[i] ^ b1[i];
        aa13[i] = a3[i] ^ a1[i];
        bb13[i] = b3[i] ^ b1[i];
        aa14[i] = a4[i] ^ a1[i];
        bb14[i] = b4[i] ^ b1[i];
        aa23[i] = a2[i] ^ a3[i];
        bb23[i] = b2[i] ^ b3[i];
        aa24[i] = a2[i] ^ a4[i];
        bb24[i] = b2[i] ^ b4[i];
        aa34[i] = a3[i] ^ a4[i];
        bb34[i] = b3[i] ^ b4[i];
    }

    karat_mul_vec(D0, a0, b0, TOOM5_SLICE_256);
    karat_mul_vec(D1, a1, b1, TOOM5_SLICE_256);
    karat_mul_vec(D2, a2, b2, TOOM5_SLICE_256);
    karat_mul_vec(D3, a3, b3, TOOM5_SLICE_256);
    karat_mul_vec(D4, a4, b4, TOOM5_SLICE_256);
    karat_mul_vec(D01, aa01, bb01, TOOM5_SLICE_256);
    karat_mul_vec(D02, aa02, bb02, TOOM5_SLICE_256);
    karat_mul_vec(D03, aa03, bb03, TOOM5_SLICE_256);
    karat_mul_vec(D04, aa04, bb04, TOOM5_SLICE_256);
    karat_mul_vec(D12, aa12, bb12, TOOM5_SLICE_256);
    karat_mul_vec(D13, aa13, bb13, TOOM5_SLICE_256);
    karat_mul_vec(D14, aa14, bb14, TOOM5_SLICE_256);
    karat_mul_vec(D23, aa23, bb23, TOOM5_SLICE_256);
    karat_mul_vec(D24, aa24, bb24, TOOM5_SLICE_256);
    karat_mul_vec(D34, aa34, bb34, TOOM5_SLICE_256);

    for (int32_t i = 0; i < TOOM5_SLICE_256; i++) {
        ro256[i] = D0[i];
        ro256[i + TOOM5_SLICE_256] = D0[i + TOOM5_SLICE_256] ^ D01[i] ^ D0[i] ^ D1[i];
        ro256[i + 2 * TOOM5_SLICE_256] = D1[i] ^ D02[i] ^ D0[i] ^ D2[i] ^
                                          D01[i + TOOM5_SLICE_256] ^ D0[i + TOOM5_SLICE_256] ^
                                          D1[i + TOOM5_SLICE_256];
        ro256[i + 3 * TOOM5_SLICE_256] = D1[i + TOOM5_SLICE_256] ^ D03[i] ^ D0[i] ^ D3[i] ^
                                          D12[i] ^ D1[i] ^ D2[i] ^ D02[i + TOOM5_SLICE_256] ^
                                          D0[i + TOOM5_SLICE_256] ^ D2[i + TOOM5_SLICE_256];
        ro256[i + 4 * TOOM5_SLICE_256] = D2[i] ^ D04[i] ^ D0[i] ^ D4[i] ^ D13[i] ^ D1[i] ^
                                          D3[i] ^ D03[i + TOOM5_SLICE_256] ^
                                          D0[i + TOOM5_SLICE_256] ^ D3[i + TOOM5_SLICE_256] ^
                                          D12[i + TOOM5_SLICE_256] ^ D1[i + TOOM5_SLICE_256] ^
                                          D2[i + TOOM5_SLICE_256];
        ro256[i + 5 * TOOM5_SLICE_256] = D2[i + TOOM5_SLICE_256] ^ D14[i] ^ D1[i] ^ D4[i] ^
                                          D23[i] ^ D2[i] ^ D3[i] ^ D04[i + TOOM5_SLICE_256] ^
                                          D0[i + TOOM5_SLICE_256] ^ D4[i + TOOM5_SLICE_256] ^
                                          D13[i + TOOM5_SLICE_256] ^ D1[i + TOOM5_SLICE_256] ^
                                          D3[i + TOOM5_SLICE_256];
        ro256[i + 6 * TOOM5_SLICE_256] = D3[i] ^ D24[i] ^ D2[i] ^ D4[i] ^
                                          D14[i + TOOM5_SLICE_256] ^ D1[i + TOOM5_SLICE_256] ^
                                          D4[i + TOOM5_SLICE_256] ^ D23[i + TOOM5_SLICE_256] ^
                                          D2[i + TOOM5_SLICE_256] ^ D3[i + TOOM5_SLICE_256];
        ro256[i + 7 * TOOM5_SLICE_256] = D3[i + TOOM5_SLICE_256] ^ D34[i] ^ D3[i] ^ D4[i] ^
                                          D24[i + TOOM5_SLICE_256] ^ D2[i + TOOM5_SLICE_256] ^
                                          D4[i + TOOM5_SLICE_256];
        ro256[i + 8 * TOOM5_SLICE_256] = D4[i] ^ D34[i + TOOM5_SLICE_256] ^
                                          D3[i + TOOM5_SLICE_256] ^ D4[i + TOOM5_SLICE_256];
        ro256[i + 9 * TOOM5_SLICE_256] = D4[i + TOOM5_SLICE_256];
    }

    for (int32_t i = 0; i < TOOM5_PRODUCT_256; i++) {
        Out[i] = ro256[i];
    }
}

static void toom_3_mult(__m256i *Out, const __m256i *A, const __m256i *B) {
    static __m256i U0[TOOM5_INPUT_256], V0[TOOM5_INPUT_256], U1[TOOM5_INPUT_256], V1[TOOM5_INPUT_256];
    static __m256i U2[TOOM5_INPUT_256], V2[TOOM5_INPUT_256];
    static __m256i W0[TOOM5_PRODUCT_256], W1[TOOM5_PRODUCT_256], W2[TOOM5_PRODUCT_256];
    static __m256i W3[TOOM5_PRODUCT_256], W4[TOOM5_PRODUCT_256], tmp[TOOM5_PRODUCT_256 + 3];
    static __m256i ro256[TOOM3_PRODUCT_256];
    static const __m256i zero = {0ul, 0ul, 0ul, 0ul};
    const int32_t T2 = TOOM3_SLICE_256 << 1;

    for (int32_t i = 0; i < TOOM3_SLICE_256; i++) {
        U0[i] = A[i];
        V0[i] = B[i];
        U1[i] = A[i + TOOM3_SLICE_256];
        V1[i] = B[i + TOOM3_SLICE_256];
        U2[i] = A[i + T2];
        V2[i] = B[i + T2];
        W3[i] = U0[i] ^ U1[i] ^ U2[i];
        W2[i] = V0[i] ^ V1[i] ^ V2[i];
    }

    for (int32_t i = TOOM3_SLICE_256; i < TOOM5_INPUT_256; i++) {
        U0[i] = zero;
        V0[i] = zero;
        U1[i] = zero;
        V1[i] = zero;
        U2[i] = zero;
        V2[i] = zero;
        W2[i] = zero;
        W3[i] = zero;
    }

    karat_mult_5(W1, W2, W3);

    W0[0] = zero;
    W4[0] = zero;
    W0[1] = U1[0];
    W4[1] = V1[0];
    for (int32_t i = 1; i < TOOM3_SLICE_256 + 1; i++) {
        W0[i + 1] = U1[i] ^ U2[i - 1];
        W4[i + 1] = V1[i] ^ V2[i - 1];
    }
    W0[TOOM3_SLICE_256 + 1] = U2[TOOM3_SLICE_256 - 1];
    W4[TOOM3_SLICE_256 + 1] = V2[TOOM3_SLICE_256 - 1];
    for (int32_t i = TOOM3_EXT_256; i < TOOM5_INPUT_256; i++) {
        W0[i] = zero;
        W4[i] = zero;
    }

    for (int32_t i = 0; i < TOOM3_EXT_256; i++) {
        W3[i] ^= W0[i];
        W2[i] ^= W4[i];
        W0[i] ^= U0[i];
        W4[i] ^= V0[i];
    }

    karat_mult_5(tmp, W3, W2);
    for (int32_t i = 0; i < TOOM5_PRODUCT_256; i++) {
        W3[i] = tmp[i];
    }
    karat_mult_5(W2, W0, W4);
    karat_mult_5(W4, U2, V2);
    karat_mult_5(W0, U0, V0);

    for (int32_t i = 0; i < TOOM3_EXT_PRODUCT_256; i++) {
        W3[i] ^= W2[i];
    }
    for (int32_t i = 0; i < 2 * TOOM3_SLICE_256; i++) {
        W1[i] ^= W0[i];
    }
    for (int32_t i = 0; i < TOOM3_EXT_PRODUCT_256 - 1; i++) {
        const int32_t i1 = i + 1;
        W2[i] = W2[i1] ^ W0[i1];
    }
    W2[TOOM3_EXT_PRODUCT_256 - 1] = zero;

    for (int32_t i = 0; i < TOOM3_EXT_PRODUCT_256; i++) {
        tmp[i] = W2[i] ^ W3[i] ^ W4[i];
    }
    tmp[TOOM3_EXT_PRODUCT_256] = zero;
    tmp[TOOM3_EXT_PRODUCT_256 + 1] = zero;
    tmp[TOOM3_EXT_PRODUCT_256 + 2] = zero;
    for (int32_t i = 0; i < 2 * TOOM3_SLICE_256; i++) {
        tmp[i + 3] ^= W4[i];
    }
    divide_by_x_plus_one_256(tmp, W2, TOOM3_SLICE_256);

    for (int32_t i = 0; i < TOOM3_EXT_PRODUCT_256 - 1; i++) {
        const int32_t i1 = i + 1;
        tmp[i] = W3[i1] ^ W1[i1];
    }
    tmp[TOOM3_EXT_PRODUCT_256 - 1] = zero;
    divide_by_x_plus_one_256(tmp, W3, TOOM3_SLICE_256);

    for (int32_t i = 0; i < TOOM3_EXT_PRODUCT_256; i++) {
        W1[i] ^= W2[i] ^ W4[i];
        W2[i] ^= W3[i];
    }

    for (int32_t i = 0; i < TOOM3_SLICE_256; i++) {
        ro256[i] = W0[i];
        ro256[i + TOOM3_SLICE_256] = W0[i + TOOM3_SLICE_256] ^ W1[i];
        ro256[i + 2 * TOOM3_SLICE_256] = W1[i + TOOM3_SLICE_256] ^ W2[i];
        ro256[i + 3 * TOOM3_SLICE_256] = W2[i + TOOM3_SLICE_256] ^ W3[i];
        ro256[i + 4 * TOOM3_SLICE_256] = W3[i + TOOM3_SLICE_256] ^ W4[i];
        ro256[i + 5 * TOOM3_SLICE_256] = W4[i + TOOM3_SLICE_256];
    }

    ro256[4 * TOOM3_SLICE_256] ^= W2[2 * TOOM3_SLICE_256];
    ro256[5 * TOOM3_SLICE_256] ^= W3[2 * TOOM3_SLICE_256];
    ro256[1 + 4 * TOOM3_SLICE_256] ^= W2[1 + 2 * TOOM3_SLICE_256];
    ro256[1 + 5 * TOOM3_SLICE_256] ^= W3[1 + 2 * TOOM3_SLICE_256];
    ro256[2 + 4 * TOOM3_SLICE_256] ^= W2[2 + 2 * TOOM3_SLICE_256];
    ro256[2 + 5 * TOOM3_SLICE_256] ^= W3[2 + 2 * TOOM3_SLICE_256];
    ro256[3 + 4 * TOOM3_SLICE_256] ^= W2[3 + 2 * TOOM3_SLICE_256];
    ro256[3 + 5 * TOOM3_SLICE_256] ^= W3[3 + 2 * TOOM3_SLICE_256];

    memcpy(Out, ro256, (VEC_N_256_SIZE_64 << 1) * sizeof(uint64_t));
}

void vect_mul(__m256i *o, const __m256i *a1, const __m256i *a2) {
    copy_and_clear_input_padding(native_a_pad, a1);
    copy_and_clear_input_padding(native_b_pad, a2);
    toom_3_mult(native_a1_times_a2, native_a_pad, native_b_pad);
    reduce(o, native_a1_times_a2);

#ifdef __STDC_LIB_EXT1__
    memset_s(native_a1_times_a2, 0, sizeof(native_a1_times_a2));
#else
    memset(native_a1_times_a2, 0, sizeof(native_a1_times_a2));
#endif
}

