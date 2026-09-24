/*
 * Copyright (c) 2026 Hang Zhang.
 * State Key Laboratory of Cyberspace Security Defense,
 * Institute of Information Engineering, CAS
 * School of Cyber Security, University of Chinese Academy of Sciences
 */
/* Shim: the canonical NTT entry points (ntt_avx_forward / ntt_avx_inverse /
   ntt_basemul_raw / ntt_basemul_acc_raw) are provided directly by the
   hand-written assembly (ntt.S / intt.S / pointwise.S) as zero-overhead
   aliases — the inverse NTT's signed reduction is fused into intt.S.
   Only montgomery_lift is kept here as intrinsics (not part of the asm scope),
   matching the avx2-128 layout. */
#include <immintrin.h>
#include <stdint.h>
#include "params.h"
#include "ntt_avx.h"

/* INVERSE_Q = q^{-1} mod 2^32 (positive inverse, bit pattern 2286038529),
 * so Montgomery reduction uses subtraction:  (p - m*q) / 2^32. */
#define MONT_R2     54377
#define INVERSE_Q   -2008928767u

void ntt_montgomery_lift(int32_t *r) {
    __m256i *vr    = (__m256i *)r;
    const __m256i v_r2   = _mm256_set1_epi32(MONT_R2);
    const __m256i v_qinv = _mm256_set1_epi32((int32_t)INVERSE_Q);
    const __m256i v_q    = _mm256_set1_epi32(BIT_Q);

    for (int i = 0; i < BIT_N / 8; i++) {
        __m256i a     = _mm256_load_si256(&vr[i]);
        __m256i a_odd = _mm256_shuffle_epi32(a, 0xF5);   /* odd lanes -> even dword pos */

        /* p = a * R^2  — signed 32x32->64 (input in [0,q), signed==unsigned) */
        __m256i p_lo = _mm256_mul_epi32(a,     v_r2);
        __m256i p_hi = _mm256_mul_epi32(a_odd, v_r2);

        /* m = lo32(p) * qinv */
        __m256i m_lo = _mm256_mul_epu32(p_lo, v_qinv);
        __m256i m_hi = _mm256_mul_epu32(p_hi, v_qinv);

        /* tq = m * q  — m may exceed 2^31, signed 32x32->64 to match ref (int64_t)m*Q */
        __m256i tq_lo = _mm256_mul_epi32(m_lo, v_q);
        __m256i tq_hi = _mm256_mul_epi32(m_hi, v_q);

        /* d = p - tq  (subtraction — INVERSE_Q is the positive inverse) */
        __m256i d_lo = _mm256_sub_epi64(p_lo, tq_lo);
        __m256i d_hi = _mm256_sub_epi64(p_hi, tq_hi);

        /* result = d >> 32  (low 32 bits == 0, high 32 = result in (-q, q)) */
        __m256i result = _mm256_blend_epi32(
            _mm256_srli_epi64(d_lo, 32), d_hi, 0xAA);

        /* (-q, q) -> [0, q) */
        result = _mm256_add_epi32(result,
            _mm256_and_si256(_mm256_srai_epi32(result, 31), v_q));

        _mm256_store_si256(&vr[i], result);
    }
}
