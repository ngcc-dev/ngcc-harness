#include <stdint.h>
#include "params.h"
#include "reduce.h"
#include <immintrin.h>
/*************************************************
* Name:        montgomery_reduce
*
* Description: Montgomery reduction; given a 32-bit integer a, computes
*              16-bit integer congruent to a * R^-1 mod q, where R=2^16
*
* Arguments:   - int32_t a: input integer to be reduced;
*                           has to be in {-q2^15,...,q2^15-1}
*
* Returns:     integer in {-q+1,...,q-1} congruent to a * R^-1 modulo q.
**************************************************/
int16_t montgomery_reduce(int32_t a)
{
  int16_t t;

  t = (int16_t)a*QINV;
  t = (a - (int32_t)t*COMPASS_KEM_Q) >> 16;
  return t;
}

/*************************************************
* Name:        barrett_reduce
*
* Description: Barrett reduction; given a 16-bit integer a, computes
*              centered representative congruent to a mod q in {-(q-1)/2,...,(q-1)/2}
*
* Arguments:   - int16_t a: input integer to be reduced
*
* Returns:     integer in {-(q-1)/2,...,(q-1)/2} congruent to a modulo q.
**************************************************/
int16_t barrett_reduce(int16_t a) {
  int16_t t;
  const int16_t v = ((1<<26) + (COMPASS_KEM_Q>>1))/COMPASS_KEM_Q;

  t  = ((int32_t)v*a + (1<<25)) >> 26;
  t *= COMPASS_KEM_Q;
  return a - t;
}

#if (COMPASS_KEM_Q == 7681)
#include "zetas_7681_avx.h"
__m256i fqmul_avx(__m256i a, __m256i b, __m256i q_vec, __m256i qinv_vec) {
    // c = a * b
    __m256i c_lo = _mm256_mullo_epi16(a, b);
    __m256i c_hi = _mm256_mulhi_epi16(a, b);
    
    // t = (c_lo * QINV) & 0xFFFF
    __m256i t = _mm256_mullo_epi16(c_lo, qinv_vec);
    
    // t_hi = (t * Q) >> 16
    __m256i t_hi = _mm256_mulhi_epi16(t, q_vec);
    
    // Return c_hi - t_hi
    return _mm256_sub_epi16(c_hi, t_hi);
}

// 2. Vectorized Barrett reduction (reduces coefficients to near [-q/2, q/2])
__m256i barrett_reduce_avx(__m256i a, __m256i v_vec, __m256i q_vec) {
    // t = (a * V) >> 16
    __m256i t = _mm256_mulhi_epi16(a, v_vec);
    // Continue right shift by 10 bits, completing the total >> 26 operation
    t = _mm256_srai_epi16(t, 10);
    
    // t_q = t * Q
    __m256i t_q = _mm256_mullo_epi16(t, q_vec);
    
    // Return a - t_q
    return _mm256_sub_epi16(a, t_q);
}
#endif