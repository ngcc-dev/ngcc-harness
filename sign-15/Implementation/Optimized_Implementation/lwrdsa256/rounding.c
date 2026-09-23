#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <immintrin.h>
#include "params.h"

/*************************************************
* Name:        decompose
*
* Description: For element a, compute high and low bits a0, a1 such
*              that a = a1*ALPHA + a0 with -ALPHA/2 < a0 <= ALPHA/2.
*              Assumes a to be standard representative.
*
* Arguments:   - uint32_t a: input element
*              - uint32_t *a0: pointer to output element a0
*
* Returns a1
**************************************************/
__m256i decompose_keygen_avx2(__m256i a, __m256i *a0) {
  __m256i a1 = _mm256_srli_epi32(_mm256_add_epi32(_mm256_sub_epi32(a, _mm256_set1_epi32(1)), _mm256_set1_epi32(1 << (D - 1))), D);
  _mm256_storeu_si256(a0, _mm256_add_epi32(a, _mm256_sub_epi32(_mm256_set1_epi32(Q), _mm256_slli_epi32(a1, D))));
  return a1;
}


/*************************************************
* Name:        decompose
*
* Description: For element a, compute high and low bits a0, a1 such
*              that a = a1*ALPHA + a0 with -ALPHA/2 < a0 <= ALPHA/2.
*              Assumes a to be standard representative.
*
* Arguments:   - uint32_t a: input element
*              - uint32_t *a0: pointer to output element a0
*
* Returns a1
**************************************************/
__m256i decompose_sign_avx2(__m256i a, __m256i *a0) {
  __m256i a1 = _mm256_srli_epi32(_mm256_add_epi32(_mm256_sub_epi32(a, _mm256_set1_epi32(1)), _mm256_set1_epi32(GAMMA2_BAR)), GAMMA1_BAR_BITS);
  _mm256_storeu_si256(a0, _mm256_add_epi32(a, _mm256_sub_epi32(_mm256_set1_epi32(P), _mm256_slli_epi32(a1, GAMMA1_BAR_BITS))));
  return _mm256_and_si256(a1, _mm256_set1_epi32((P >> GAMMA1_BAR_BITS) - 1));
}



/*************************************************
* Name:        make_hint
*
* Description: Compute hint bit indicating whether high bits of two elements
*              differ or not
*
* Arguments:   - uint32_t a: first input element
*              - uint32_t b: second input element
*
* Returns 1 if high bits of a and b differ and 0 otherwise
**************************************************/
__m256i make_hint_avx2(const __m256i a, const __m256i b)
{
    __m256i t1, t2;

    const __m256i maskP = _mm256_set1_epi32(P - 1);

    __m256i temp =
        _mm256_and_si256(
            _mm256_add_epi32(a, b),
            maskP);

    __m256i a1 = decompose_sign_avx2(a,    &t1);
    __m256i a2 = decompose_sign_avx2(temp, &t2);

    /* Returns 1 when a1 != a2, otherwise 0 */
    return _mm256_andnot_si256(
               _mm256_cmpeq_epi32(a1, a2),
               _mm256_set1_epi32(1));
}

/*************************************************
* Name:        use_hint
*
* Description: Correct high bits according to hint
*
* Arguments:   - uint32_t a: input element
*              - unsigned int hint: hint bit
*
* Returns corrected high bits
**************************************************/
__m256i use_hint_avx2(__m256i a, __m256i hint) {
  __m256i a0, a1;
  __m256i val        = _mm256_set1_epi32(1);
  __m256i hint_mask  = _mm256_cmpeq_epi32(hint, _mm256_set1_epi32(0));

  a  = _mm256_and_si256(a, _mm256_set1_epi32(P - 1));   // Bug 1 fixed: P-1
  a1 = decompose_sign_avx2(a, &a0);

  __m256i p_vec        = _mm256_set1_epi32(P);
  __m256i a0_gt        = _mm256_cmpgt_epi32(a0, p_vec);
  __m256i a0_le        = _mm256_xor_si256(a0_gt, _mm256_set1_epi32(-1));

  // Zero out adjustments for hint==0 lanes
  a0_gt = _mm256_andnot_si256(hint_mask, a0_gt);
  a0_le = _mm256_andnot_si256(hint_mask, a0_le);

  __m256i ret_val = _mm256_add_epi32(a1, _mm256_and_si256(a0_gt, val));
  ret_val         = _mm256_sub_epi32(ret_val, _mm256_and_si256(a0_le, val));

  // Bug 2 fixed: only mask hint!=0 lanes, return a1 directly for hint==0
  __m256i mod_mask = _mm256_set1_epi32((P >> GAMMA1_BAR_BITS) - 1);
  __m256i masked   = _mm256_and_si256(ret_val, mod_mask);
  return _mm256_blendv_epi8(masked, a1, hint_mask);
}
