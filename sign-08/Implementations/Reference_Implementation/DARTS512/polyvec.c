#include <stdint.h>

#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "math.h"
#include "fft.h"

/**************************************************************/
/************ Vectors of polynomials of length K **************/
/**************************************************************/

// 定义 q+1 / 2 beta 多项式------------------------------------------------------------------------------

/*************************************************
 * Name:        polyveck_halfq_beta
 *
 * Description: Set r to the polynomial vector with
 *              r.vec[0].coeffs[0] = (Q+1)/2 and all other coefficients 0.
 *
 * Arguments:   - polyveck *r: pointer to output vector
 **************************************************/
void polyveck_halfq_beta(polyveck *r) {
    unsigned int i, j;
    
    for (i = 0; i < K; ++i)
        for (j = 0; j < N; ++j)
            r->vec[i].coeffs[j] = 0;

    r->vec[0].coeffs[0] = HALF_Q; 
}

// 定义多项式向量的模约化---------------------------------------------------------------------------------

/*************************************************
 * Name:        polyveck_freeze
 *
 * Description: For all coefficients of polynomials in vector of length K
 *              compute standard representative r = a mod^+ Q.
 *
 * Arguments:   - polyveck *v: pointer to input/output vector
 **************************************************/
void polyveck_freeze(polyveck *a) {
    unsigned int i;

    for (i = 0; i < K; ++i)
        poly_freeze(&a->vec[i]);
}

// 定义多项式向量的压缩与解压缩---------------------------------------------------------------------------------

/*************************************************
 * Name:        polyveck_compress
 *
 * Description: Compression of a vector of polynomials of length K
 *
 * Arguments:   - polyveck *a_high: pointer to output compressed vector
 *              - const polyveck *a: pointer to input vector
 **************************************************/
void polyveck_compress(polyveck *a_high, const polyveck *a) {
    unsigned int i;

    for (i = 0; i < K; ++i)
        poly_compress(&a_high->vec[i], &a->vec[i]);
}

/*************************************************
 * Name:        polyveck_decompress
 *
 * Description: Decompression of a vector of polynomials of length K
 *
 * Arguments:   - polyveck *a_mid: pointer to output decompressed vector
 *              - const polyveck *a_high: pointer to input compressed vector
 **************************************************/
void polyveck_decompress(polyveck *a_mid, const polyveck *a_high) {
    unsigned int i;

    for (i = 0; i < K; ++i)
        poly_decompress(&a_mid->vec[i], &a_high->vec[i]);
}

/*************************************************
 * Name:        polyveck_lowbits
 *
 * Description: Compute the low bits of a vector of polynomials of length K
 *
 * Arguments:   - polyveck *a_low:   pointer to output vector (low bits)
 *              - const polyveck *a: pointer to input vector
 **************************************************/
void polyveck_lowbits(polyveck *a_low, const polyveck *a) {
    unsigned int i;

    for (i = 0; i < K; ++i)
        poly_lowbits(&a_low->vec[i], &a->vec[i]);
}

// 定义的多项式向量之间的运算---------------------------------------------------------------------------------

/*************************************************
 * Name:        polyveck_add
 *
 * Description: Add vectors of polynomials of length K.
 *              No modular reduction is performed.
 *
 * Arguments:   - polyveck *w: pointer to output vector
 *              - const polyveck *u: pointer to first summand
 *              - const polyveck *v: pointer to second summand
 **************************************************/
void polyveck_add(polyveck *r, const polyveck *a, const polyveck *b) {
    unsigned int i;

    for (i = 0; i < K; i++)
        poly_add(&r->vec[i], &a->vec[i], &b->vec[i]);
}

/*************************************************
 * Name:        polyveck_sub
 *
 * Description: Subtract vectors of polynomials of length K.
 *              No modular reduction is performed.
 *
 * Arguments:   - polyveck *w: pointer to output vector
 *              - const polyveck *u: pointer to first input vector
 *              - const polyveck *v: pointer to second input vector to be
 *                                   subtracted from first input vector
 **************************************************/
void polyveck_sub(polyveck *r, const polyveck *a, const polyveck *b) {
    unsigned int i;

    for (i = 0; i < K; i++)
        poly_sub(&r->vec[i], &a->vec[i], &b->vec[i]);
}

/*************************************************
 * Name:        polyveck_double
 *
 * Description: Double vector of polynomials of length K.
 *              No modular reduction is performed.
 *
 * Arguments:   - polyveck *w: pointer to output vector
 **************************************************/
void polyveck_double(polyveck *r) {
    unsigned int i, j;

    for (i = 0; i < K; ++i)
        for (j = 0; j < N; ++j)
            r->vec[i].coeffs[j] *= 2;
}

/*************************************************
 * Name:        polyveck_neg
 *
 * Description: Negate vector of polynomials of length K
 *
 * Arguments:   - polyveck *r: pointer to output vector of polynomials of
 *                              length K
 **************************************************/
void polyveck_neg(polyveck *r) {
    unsigned int i, j;

    for (i = 0; i < K; i++) {
        for (j = 0; j < N; j++) {
            r->vec[i].coeffs[j] *= -1;
        }
    }
}

/*************************************************
 * Name:        polyveck_cneg
 *
 * Description: Conditionnally negate vector of polynomials of length K
 *              if b == 1
 *
 * Arguments:   - polyveck *v: pointer to output vector of polynomials of
 *                              length K
 *              - const uint8_t b: condition bit
 **************************************************/
void polyveck_cneg(polyveck *v, const uint8_t b) {
        unsigned int i, j;
    for (i = 0; i < K; i++) {
        for (j = 0; j < N; j++) {
            v->vec[i].coeffs[j] *= 1 - 2 * b;
        }
    }
}

/*************************************************
 * Name:        polyveck_frommont
 *
 * Description: multiply each coefficient with MONTSQ
 *
 * Arguments:   - polyveck *v: pointer to output vector of polynomials of
 *                              length K
 **************************************************/
void polyveck_frommont(polyveck *r) {
    unsigned int i, j;

    for (i = 0; i < K; ++i)
        for (j = 0; j < N; j++)
            r->vec[i].coeffs[j] =
                montgomery_reduce((int64_t)r->vec[i].coeffs[j] * MONTSQ);
}

/*************************************************
 * Name:        polyveck_ntt
 *
 * Description: Forward NTT of all polynomials in vector of length K
 *
 * Arguments:   - polyveck *v: pointer to input/output vector
 **************************************************/
void polyveck_ntt(polyveck *r) {
    unsigned int i;

    for (i = 0; i < K; ++i)
        poly_ntt(&r->vec[i]);
}

/*************************************************
 * Name:        polyveck_invntt_tomont
 *
 * Description: Inverse NTT and multiplication by Montgomery factor
 *              of all polynomials in vector of length K
 *
 * Arguments:   - polyveck *v: pointer to input/output vector
 **************************************************/
void polyveck_invntt_tomont(polyveck *r) {
    unsigned int i;

    for (i = 0; i < K; ++i)
        poly_invntt_tomont(&r->vec[i]);
}

/*************************************************
 * Name:        polyveck_basemul_montgomery
 *
 * Description: Pointwise multiplication of vectors of polynomials of length K
 *              in NTT domain.
 *
 * Arguments:   - polyveck *r: pointer to output vector
 *              - const polyveck *a: pointer to first input vector
 *              - const polyveck *b: pointer to second input vector
 **************************************************/
void polyveck_basemul_montgomery(polyveck *r, const polyveck *a, const polyveck *b) {
    unsigned int i;

    for (i = 0; i < K; ++i)
        poly_basemul_montgomery(&r->vec[i], &a->vec[i], &b->vec[i]);
}

/*************************************************
 * Name:        polyveck_caddq
 *
 * Description: For all coefficients of polynomials in vector of length K
 *              conditionally add Q if coefficient is negative
 *
 * Arguments:   - polyveck *r: pointer to input/output vector
 **************************************************/
void polyveck_caddq(polyveck *r) {
    unsigned int i;

    for (i = 0; i < K; ++i)
        poly_caddq(&r->vec[i]);
}

/*************************************************
 * Name:        polyveck_sqnorm2
 *
 * Description: Compute squared l2-norm of vector of polynomials of length K
 *
 * Arguments:   - const polyveck *r: pointer to input vector
 *
 * Returns:     - squared l2-norm
 **************************************************/
uint64_t polyveck_sqnorm2(const polyveck *r) {
    unsigned int i, j;
    uint64_t sqnorm = 0;

    for (i = 0; i < K; ++i)
        for (j = 0; j < N; ++j)
            sqnorm += (uint64_t)r->vec[i].coeffs[j] * r->vec[i].coeffs[j];

    return sqnorm;
}

/*************************************************
 * Name:        polyveck_cadd_2_d
 *
 * Description: For all coefficients of polynomials in vector of length K
 *              conditionally add 2^d if coefficient is negative
 *
 * Arguments:   - polyveck *r: pointer to input/output vector
 **************************************************/
void polyveck_cadd_2_d(polyveck *r) {
    unsigned int i, j;

    for (i = 0; i < K; ++i)
        for (j = 0; j < N; ++j)
            r->vec[i].coeffs[j] += ((r->vec[i].coeffs[j] >> 31) & (1 << D));
}

// 定义多项式的封装------------------------------------------------------------------------------------

/*************************************************
 * Name:        polyveck_q_pack
 *
 * Description: Pack vector of polynomials of length K with
 *              coefficients in [0, Q-1].
 *
 * Arguments:   - uint8_t *r: pointer to output byte array with at least
 *                            K * N bytes
 *              - const polyveck *a: pointer to input vector
 **************************************************/
void polyveck_q_pack(uint8_t *r, const polyveck *a) {
    unsigned int i;

    for (i = 0; i < K; ++i)
        poly_q_pack(r + i * POLY_Q_PACKEDBYTES, &a->vec[i]);
}

/*************************************************
 * Name:        polyveck_pack_highbits
 *
 * Description: Pack the high bits of a vector of polynomials of length K
 *
 * Arguments:   - uint8_t *r: pointer to output byte array with at least
 *                            K * N * D / 8 bytes
 *              - const polyveck *a: pointer to input vector
 **************************************************/
void polyveck_pack_highbits(uint8_t *r, const polyveck *a) {
    unsigned int i;

    for (i = 0; i < K; ++i)
        poly_pack_highbits(r + i * POLY_HIGHBITS_PACKEDBYTES, &a->vec[i]);
}

/*************************************************
 * Name:        polyveck_pack_compressed
 *
 * Description: Pack already-compressed D-bit values of a vector of 
 *              polynomials of length K (no compression performed)
 *
 * Arguments:   - uint8_t *r: pointer to output byte array with at least
 *                            K * N * D / 8 bytes
 *              - const polyveck *a: pointer to input vector with coefficients
 *                                   already in [0, 2^D - 1] range
 **************************************************/
void polyveck_pack_compressed(uint8_t *r, const polyveck *a) {
    unsigned int i;

    for (i = 0; i < K; ++i)
        poly_pack_compressed(r + i * POLY_HIGHBITS_PACKEDBYTES, &a->vec[i]);
}


// 定义多项式向量的采样---------------------------------------------------------------------------------

/*************************************************
 * Name:        polyveck_expand
 *
 * Description: Sample a vector of polynomials with uniformly random
 *              coefficients in Zq by rejection sampling on the
 *              output stream from SHAKE128(seed|nonce)
 *
 * Arguments:   - polyveck *v: pointer to output a vector of polynomials of
 *                             length K
 *              - const uint8_t seed[]: byte array with seed of length SEEDBYTES
 **************************************************/
void polyveck_expand(polyveck *r, const uint8_t seed[SEEDBYTES]) {
    unsigned int i, nonce = (K << 8) + L;
    for (i = 0; i < K; ++i)
        poly_uniform(&r->vec[i], seed, nonce++);
}


/**************************************************************/
/************ Vectors of polynomials of length L **************/
/**************************************************************/

/*************************************************
 * Name:        polyvecl_freeze
 *
 * Description: For all coefficients of polynomials in vector of length L
 *              compute standard representative r = a mod^+ Q.
 *
 * Arguments:   - polyvecl *v: pointer to input/output vector
 **************************************************/
void polyvecl_freeze(polyvecl *r) {
    unsigned int i;

    for (i = 0; i < L; ++i)
        poly_freeze(&r->vec[i]);
}

/*************************************************
 * Name:        polyvecl_pack_lowbits
 *
 * Description: Pack the low bits of a vector of polynomials of length L
 *
 * Arguments:   - uint8_t *r: pointer to output byte array with at least
 *                            L * N bytes
 *              - const polyvecl *a: pointer to input vector
 **************************************************/
void polyvecl_pack_lowbits(uint8_t *r, const polyvecl *a) {
    unsigned int i;

    for (i = 0; i < L; ++i)
        poly_pack_lowbits(r + i * POLY_LOWBITS_PACKEDBYTES, &a->vec[i]);
}

/*************************************************
 * Name:        polyvecl_unpack_lowbits
 *
 * Description: Unpack the low bits of a vector of polynomials of length L
 *
 * Arguments:   - polyvecl *r: pointer to output vector
 *              - const uint8_t *a: pointer to input byte array
 **************************************************/
void polyvecl_unpack_lowbits(polyvecl *r, const uint8_t *a) {
    unsigned int i;

    for (i = 0; i < L; ++i)
        poly_unpack_lowbits(&r->vec[i], a + i * POLY_LOWBITS_PACKEDBYTES);
}

/*************************************************
 * Name:        polyvecl_compress
 *
 * Description: Compression of a vector of polynomials of length L
 *
 * Arguments:   - polyvecl *a_high: pointer to output compressed vector
 *              - const polyvecl *a: pointer to input vector
 **************************************************/
void polyvecl_compress(polyvecl *a_high, const polyvecl *a) {
    unsigned int i;

    for (i = 0; i < L; ++i)
        poly_compress(&a_high->vec[i], &a->vec[i]);
}

/*************************************************
 * Name:        polyvecl_decompress
 *
 * Description: Decompression of a vector of polynomials of length L
 *
 * Arguments:   - polyvecl *a_mid: pointer to output decompressed vector
 *              - const polyvecl *a_high: pointer to input compressed vector
 **************************************************/
void polyvecl_decompress(polyvecl *a_mid, const polyvecl *a_high) {
    unsigned int i;

    for (i = 0; i < L; ++i)
        poly_decompress(&a_mid->vec[i], &a_high->vec[i]);
}

/*************************************************
 * Name:        polyvecl_lowbits
 *
 * Description: Compute the low bits of a vector of polynomials of length L
 *
 * Arguments:   - polyvecl *a_low:   pointer to output vector (low bits)
 *              - const polyvecl *a: pointer to input vector
 **************************************************/
void polyvecl_lowbits(polyvecl *a_low, const polyvecl *a_high) {
    unsigned int i;

    for (i = 0; i < L; ++i)
        poly_lowbits(&a_low->vec[i], &a_high->vec[i]);
}

/*************************************************
 * Name:        polyvecl_compose
 * 
 * Description: Compose a vector of polynomials from its low bits and high bits
 * 
 * Arguments:   - polyvecl *r: pointer to output vector
 *              - const polyvecl *a_low: pointer to low bits vector
 *              - const polyvecl *a_high: pointer to high bits vector
 **************************************************/
void polyvecl_compose(polyvecl *r, const polyvecl *a_low, const polyvecl *a_high) {
    unsigned int i;

    for (i = 0; i < L; i++)
        poly_compose(&r->vec[i], &a_low->vec[i], &a_high->vec[i]);
}

/*************************************************
 * Name:        polyvecl_ntt
 *
 * Description: Forward NTT of all polynomials in vector of length L
 *
 * Arguments:   - polyvecl *v: pointer to input/output vector
 **************************************************/
void polyvecl_ntt(polyvecl *r) {
    unsigned int i;

    for (i = 0; i < L; ++i)
        poly_ntt(&r->vec[i]);
}

/*************************************************
 * Name:        polyvecl_invntt_tomont
 *
 * Description: Inverse NTT and multiplication by Montgomery factor
 *              of all polynomials in vector of length L
 *
 * Arguments:   - polyvecl *v: pointer to input/output vector
 **************************************************/
void polyvecl_invntt_tomont(polyvecl *r) {
    unsigned int i;

    for (i = 0; i < L; ++i)
        poly_invntt_tomont(&r->vec[i]);
}

/*************************************************
 * Name:        polyvecl_basemul_acc_montgomery
 *
 * Description: Pointwise multiplication of vectors of polynomials of length L
 *              in NTT domain and accumulate to a single polynomial.
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const polyvecl *a: pointer to first input vector
 *              - const polyvecl *b: pointer to second input vector
 **************************************************/
void polyvecl_basemul_acc_montgomery(poly *r, const polyvecl *a, const polyvecl *b) {
    unsigned int i;
    poly t;

    poly_basemul_montgomery(r, &a->vec[0], &b->vec[0]);

    for (i = 1; i < L; ++i) {
        poly_basemul_montgomery(&t, &a->vec[i], &b->vec[i]);
        poly_add(r, r, &t);
    }
}

/*************************************************
 * Name:        polyveck_cneg
 *
 * Description: Conditionnally negate vector of polynomials of length L
 *              if b == 1
 *
 * Arguments:   - polyveck *v: pointer to output vector of polynomials of
 *                              length L
 *              - const uint8_t b: condition bit
 **************************************************/
void polyvecl_cneg(polyvecl *v, const uint8_t b) {
    unsigned int i, j;
    
    for (i = 0; i < L; i++) {
        for (j = 0; j < N; j++) {
            v->vec[i].coeffs[j] *= 1 - 2 * b;
        }
    }
}

/*************************************************
 * Name:        polyvecl_sqnorm2
 *
 * Description: Compute squared l2-norm of vector of polynomials of length L
 *
 * Arguments:   - const polyvecl *r: pointer to input vector
 *
 * Returns:     - squared l2-norm
 **************************************************/
uint64_t polyvecl_sqnorm2(const polyvecl *r) {
    unsigned int i, j;
    uint64_t sqnorm = 0;

    for (i = 0; i < L; ++i)
        for (j = 0; j < N; ++j)
            sqnorm += (uint64_t)r->vec[i].coeffs[j] * r->vec[i].coeffs[j];

    return sqnorm;
}

/**************************************************************/
/************ Vectors of polynomials of length L-1 ************/
/**************************************************************/

/*************************************************
 * Name:        polyvecl_1_ntt
 *
 * Description: Forward NTT of all polynomials in vector of length L-1
 *
 * Arguments:   - polyvecl_1 *v: pointer to input/output vector
 **************************************************/
void polyvecl_1_ntt(polyvecl_1 *r) {
    unsigned int i;

    for (i = 0; i < L-1; ++i)
        poly_ntt(&r->vec[i]);
}

/*************************************************
 * Name:        polyvecl_1_basemul_acc_montgomery
 *
 * Description: Pointwise multiplication of vectors of polynomials of length L-1
 *              in NTT domain and accumulate to a single polynomial.
 *
 * Arguments:   - poly *r: pointer to output polynomial
 *              - const polyvecl_1 *a: pointer to first input vector
 *              - const polyvecl_1 *b: pointer to second input vector
 **************************************************/
void polyvecl_1_basemul_acc_montgomery(poly *r, const polyvecl_1 *a, const polyvecl_1 *b) {
    unsigned int i;
    poly t;

    poly_basemul_montgomery(r, &a->vec[0], &b->vec[0]);

    for (i = 1; i < L-1; ++i) {
        poly_basemul_montgomery(&t, &a->vec[i], &b->vec[i]);
        poly_add(r, r, &t);
    }
}

/*************************************************
 * Name:        minmax
 *
 * Description: Safely swap two 32-bit integers such that the first becomes
 *              the minimum and the second becomes the maximum.
 *              Branchless implementation to resist timing attacks.
 *
 * Arguments:   - int32_t *x: pointer to the first integer
 *                             (stores the smaller value on return)
 *              - int32_t *y: pointer to the second integer
 *                             (stores the larger value on return)
 **************************************************/
static inline void minmax(int32_t *x, int32_t *y) // djbsort
{
    int32_t a = *x;      
    int32_t b = *y;     
    int32_t ab = b ^ a;   
    int32_t c = b - a;   
    c ^= ab & (c ^ b);  
    c >>= 31;              
    c &= ab;              
    *x = a ^ c;           
    *y = b ^ c;          
}

/*************************************************
 * Name:        polyveckl_sqsing_value
 *
 * Description: Compute N(s)
 *
 * Arguments:   - const poly *s0: pointer to first part of secret key
 *              - const polyvecl_1 *s1: pointer to second part of secret key
 *              - const polyveck *e: pointer to error vector
 **************************************************/
int64_t polyveckl_sqsing_value(const poly *s0, const polyvecl_1 *s1, const polyveck *e) {
    int32_t res = 0;
    complex_fp32_16 input[FFT_N] = {0};
    int32_t sum[N] = {0}, bestm[N / TAU + 1] = {0}, min = 0;

    fft_init_and_bitrev(input, s0);
    fft(input);
    for (size_t j = 0; j < N; ++j) {
        sum[j] = complex_fp_sqabs(input[j]);
    }

    for (size_t i = 0; i < L-1; ++i) {
        fft_init_and_bitrev(input, &s1->vec[i]);
        fft(input);

        for (size_t j = 0; j < N; ++j) {
            sum[j] += complex_fp_sqabs(input[j]);
        }
    }

    for (size_t i = 0; i < K; ++i) {
        fft_init_and_bitrev(input, &e->vec[i]);
        fft(input);

        for (size_t j = 0; j < N; ++j) {
            sum[j] += complex_fp_sqabs(input[j]);
        }
    }

    // 计算前 N/TAU + 1 个最大值
    for (size_t i = 0; i < N / TAU + 1; ++i) {
        bestm[i] = sum[i];
    }
    for (size_t i = N / TAU + 1; i < N; ++i) {
        for (size_t j = 0; j < N / TAU + 1; ++j) {
            minmax(&sum[i], &bestm[j]);
        }
    }

    // 找到 bestm 中的最小值
    min = bestm[0];
    for (size_t i = 1; i < N / TAU + 1; ++i) {
        int32_t temp = bestm[i];
        minmax(&min, &temp);
    }
    
    // 计算最终结果
    for (size_t i = 0; i < N / TAU + 1; ++i) {
        int32_t fac = (min - bestm[i]) >> 31;
        fac = 
            (fac & (TAU)) ^ 
            ((~fac) & (N % TAU));
        
        bestm[i] = (bestm[i] + (1 << 5)) >> 6;

        bestm[i] *= fac;
        res += bestm[i]; 
    }

    return (res + (1 << 9)) >> 10; // 返回 N(s) 的估计值
}

/*************************************************
 * Name:        polyveckl_ternary_p
 *
 * Description: Sample a vector of polynomials with uniformly random
 *              coefficients in [-ETA,ETA] by rejection sampling on the
 *              output stream from SHAKE256(seed|nonce)
 *
 * Arguments:   - polyvecl *s: pointer to output a vector of polynomials of
 *                             length L
 *              - polyveck *e: pointer to output a vector of polynomials of
 *                             length K
 *              - const uint8_t seed[]: byte array with seed of length SEEDBYTES
 *              - uint16_t nonce: 2-byte nonce
 **************************************************/
void polyveckl_ternary_p(poly *s0, polyvecl_1 *s1, polyveck *e, const uint8_t seed[CRHBYTES], uint16_t nonce) {
    unsigned int i, n = nonce;

    poly_ternary_p(s0, seed, n++);

    for (i = 0; i < L-1; i++)
        poly_ternary_p(&s1->vec[i], seed, n++);

    for (i = 0; i < K; ++i)
        poly_ternary_p(&e->vec[i], seed, n++);
}