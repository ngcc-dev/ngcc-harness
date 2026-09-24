#ifndef SIGN_REDUCE_AVX2_H
#define SIGN_REDUCE_AVX2_H

#include <immintrin.h>
#include "consts.h"   // 引入 _8XQ 等索引宏和 qdata 数组声明

#if DARTS_MODE == 128 || DARTS_MODE == 256

#define MONT 114369      // 2^32 mod q
#define MONTSQ 7148      // 2^64 mod q
#define QINV 4244570369  // q^(-1) mod 2^32
#define QREC 32831       // 2^32 // Q for Barrett
#define DQREC 16415      // 2^32 // DQ for Barrett

#elif DARTS_MODE == 512

#define MONT  130976     // 2^32 mod q
#define MONTSQ 125151    // 2^64 mod q
#define QINV 2820670977  // q^(-1) mod 2^32
#define QREC 16480       // 2^32 // Q for Barrett
#define DQREC 8240       // 2^32 // DQ for Barrett

#endif  

/*************************************************
* Name:        _mm256_mulhi_epi32_custom
*
* Description: Computes the product of eight 32-bit signed integers in two 
*              256-bit vectors and returns the high 32 bits of the 64-bit results.
*
* Arguments:   - __m256i a: 256-bit vector containing the first operands
*              - __m256i b: 256-bit vector containing the second operands
*
* Returns:     - __m256i:   256-bit vector containing the high 32 bits of 
*              the 8 independent products
**************************************************/
static inline __attribute__((always_inline))
__m256i _mm256_mulhi_epi32_custom(__m256i a, __m256i b) {

    __m256i even_prod = _mm256_mul_epi32(a, b);
    __m256i odd_prod  = _mm256_mul_epi32(_mm256_srli_epi64(a, 32), _mm256_srli_epi64(b, 32));

    __m256i even_hi = _mm256_srli_epi64(even_prod, 32);

    return _mm256_blend_epi32(even_hi, odd_prod, 0xAA);
}

/*************************************************
* Name:        fqmul_avx
*
* Description: Performs AVX2-optimized Montgomery multiplication of two 
*              vectors of coefficients. Computes (a * b * R^-1) mod q, 
*              where R = 2^32.
*
* Arguments:   - __m256i a: 256-bit vector containing the first factors
*              - __m256i b: 256-bit vector containing the second factors
*
* Returns:     - __m256i:   256-bit vector containing the Montgomery 
*              multiplication results
**************************************************/
static inline __attribute__((always_inline))
__m256i fqmul_avx(__m256i a, __m256i b) {
    __m256i v_qinv = _mm256_load_si256((const __m256i *)&qdata[_8XQINV]);
    __m256i v_q    = _mm256_load_si256((const __m256i *)&qdata[_8XQ]);

    __m256i ab_lo = _mm256_mullo_epi32(a, b);
    __m256i t = _mm256_mullo_epi32(ab_lo, v_qinv);

    __m256i ab_hi = _mm256_mulhi_epi32_custom(a, b);
    __m256i tq_hi = _mm256_mulhi_epi32_custom(t, v_q);
    
    return _mm256_sub_epi32(ab_hi, tq_hi);
}

/*************************************************
* Name:        montgomery_reduce_avx
*
* Description: Performs AVX2-optimized Montgomery reduction. 
*              Transforms the input elements into (a * R^-1) mod q.
*
* Arguments:   - __m256i a: 256-bit vector containing the elements to be reduced
*
* Returns:     - __m256i:   256-bit vector containing the Montgomery reduced 
*              coefficients
**************************************************/
static inline __attribute__((always_inline))
__m256i montgomery_reduce_avx(__m256i a) {
    return fqmul_avx(a, _mm256_set1_epi32(1));
}

/*************************************************
 * Name:        freeze_avx2
 *
 * Description: AVX2-optimized computation of the standard 
 *              representative r = a mod^+ Q for 8 elements.
 *
 * Arguments:   - __m256i a: vector of 8 finite field elements
 *
 * Returns vector of 8 standard representatives r.
 **************************************************/
static inline __attribute__((always_inline))
__m256i freeze_avx2(__m256i a) {
    __m256i v_qrec = _mm256_load_si256((const __m256i *)&qdata[_8XQREC]);
    __m256i v_q    = _mm256_load_si256((const __m256i *)&qdata[_8XQ]);
    __m256i v_dq   = _mm256_load_si256((const __m256i *)&qdata[_8XDQ]);

    __m256i t = _mm256_mulhi_epi32_custom(a, v_qrec);
    __m256i t_Q = _mm256_mullo_epi32(t, v_q);
    t = _mm256_sub_epi32(a, t_Q);

    __m256i sign_t = _mm256_srai_epi32(t, 31);
    t = _mm256_add_epi32(t, _mm256_and_si256(sign_t, v_dq));

    __m256i t_minus_Q = _mm256_sub_epi32(t, v_q);
    __m256i shift = _mm256_srai_epi32(t_minus_Q, 31);
    __m256i not_shift_and_Q = _mm256_andnot_si256(shift, v_q);
    return _mm256_sub_epi32(t, not_shift_and_Q);
}



/*************************************************
 * Name:        fqinv_avx
 *
 * Description: AVX2-optimized inversion for 8 elements.
 *
 * Arguments:   - __m256i a: vector of 8 factors (a = x * R mod q)
 *
 * Returns vector of 8 integers congruent to x^{-1} * R mod q.
 **************************************************/
static inline __attribute__((always_inline))
__m256i fqinv_avx(__m256i a) {
#if DARTS_MODE == 128 || DARTS_MODE == 256
    __m256i t2, t3;
    t2 = fqmul_avx(a, a);    
    t2 = fqmul_avx(t2, a);   
    t3 = fqmul_avx(t2, t2);  
    t3 = fqmul_avx(t3, t3);  
    t2 = fqmul_avx(t3, t2);  
    t3 = fqmul_avx(t2, t2);  
    t3 = fqmul_avx(t3, t3);  
    t3 = fqmul_avx(t3, t3);  
    t3 = fqmul_avx(t3, t3);  
    t2 = fqmul_avx(t3, t2);  
    t3 = fqmul_avx(t2, t2);  
    t3 = fqmul_avx(t3, t3);  
    t3 = fqmul_avx(t3, t3);  
    t3 = fqmul_avx(t3, t3);  
    t3 = fqmul_avx(t3, t3);  
    t3 = fqmul_avx(t3, t3);  
    t3 = fqmul_avx(t3, t3);  
    t3 = fqmul_avx(t3, t3);  
    t3 = fqmul_avx(t3, t3);  
    t2 = fqmul_avx(t3, t2);  
    return t2;
#elif DARTS_MODE == 512
    __m256i t2, t3, t4;
    t2 = fqmul_avx(a, a);      
    t2 = fqmul_avx(t2, a);     
    t3 = fqmul_avx(t2, t2);    
    t3 = fqmul_avx(t3, a);     
    t4 = fqmul_avx(t3, t3);    
    t4 = fqmul_avx(t4, t4);    
    t4 = fqmul_avx(t4, t4);    
    t4 = fqmul_avx(t4, t3);    
    t4 = fqmul_avx(t4, t4);    
    t4 = fqmul_avx(t4, a);     
    t3 = fqmul_avx(t4, t4);    
    t3 = fqmul_avx(t3, t3);    
    t3 = fqmul_avx(t3, t2);    
    t4 = fqmul_avx(t4, t4);    
    t4 = fqmul_avx(t4, t4);    
    t4 = fqmul_avx(t4, t4);    
    t4 = fqmul_avx(t4, t4);    
    t4 = fqmul_avx(t4, t4);    
    t4 = fqmul_avx(t4, t4);    
    t4 = fqmul_avx(t4, t4);    
    t4 = fqmul_avx(t4, t4);    
    t4 = fqmul_avx(t4, t4);    
    t4 = fqmul_avx(t4, t4);    
    t4 = fqmul_avx(t4, t4);    
    t3 = fqmul_avx(t4, t3);    
    return t3;

#endif
}

#endif