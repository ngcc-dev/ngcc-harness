#include <stdint.h>
#include <string.h>
#include <immintrin.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"
#include "ntt_avx.h"
#include "basemul_avx.h"
#include "consts.h"
#include "reduce.h"
#include "symmetric.h"

/*************************************************
* Name:        poly_freeze
*
* Description: Applies Barrett reduction to all coefficients of a polynomial
*              for details of the Barrett reduction see comments in reduce.c
*
* Arguments:   - poly *r: pointer to input/output polynomial
**************************************************/
void poly_freeze(poly *r)
{
    const __m256i q = _mm256_load_si256((const __m256i *)(qdata + _16XQ));
    const __m256i v = _mm256_load_si256((const __m256i *)(qdata + _16XV));
    for (unsigned int i = 0; i < N; i += 16) {
        __m256i a = _mm256_load_si256((const __m256i *)(r->coeffs + i));
        __m256i t = _mm256_mulhi_epi16(v, a);
        t = _mm256_srai_epi16(t, 8);
        t = _mm256_mullo_epi16(q, t);
        a = _mm256_sub_epi16(a, t);
        _mm256_store_si256((__m256i *)(r->coeffs + i), a);
    }
}

/*************************************************
* Name:        poly_freeze_centered
*
* Description: Applies centered reduction to all coefficients of a polynomial
*           for details of the centered reduction see comments in reduce.c
* 
* Arguments:   - poly *r: pointer to input/output polynomial
**************************************************/
void poly_freeze_centered(poly *r)
{
    const __m256i q  = _mm256_set1_epi16(Q);
    const __m256i v  = _mm256_set1_epi16(((1U << FREEZE_BITSHIFT) + Q/2) / Q);
    const __m256i hq = _mm256_set1_epi16(HALF_Q);
    const __m256i z  = _mm256_setzero_si256();
    for (unsigned int i = 0; i < N; i += 16) {
        __m256i a = _mm256_load_si256((const __m256i *)(r->coeffs + i));
        /* Barrett reduction to [0, Q) */
        __m256i t = _mm256_mulhi_epi16(v, a);
        t = _mm256_srai_epi16(t, 8);
        t = _mm256_mullo_epi16(q, t);
        a = _mm256_sub_epi16(a, t);
        /* Center: if a > (Q-1)/2, subtract Q */
        __m256i b = _mm256_sub_epi16(a, hq);
        __m256i m = _mm256_cmpgt_epi16(b, z);
        a = _mm256_sub_epi16(a, _mm256_and_si256(m, q));
        _mm256_store_si256((__m256i *)(r->coeffs + i), a);
    }
}

/*************************************************
* Name:        poly_ntt
*
* Description: Computes negacyclic number-theoretic transform (NTT) of
*              a polynomial in place;
*              inputs assumed to be in normal order, output in bitreversed order
*
* Arguments:   - uint16_t *r: pointer to in/output polynomial
**************************************************/
void poly_ntt(poly *r)
{
#if KEM_MODE == 128 || KEM_MODE == 256
    poly_freeze(r);
#endif
    ntt_avx(r->coeffs, qdata);
}

/*************************************************
* Name:        poly_invntt_tomont
*
* Description: Computes inverse of negacyclic number-theoretic transform (NTT)
*              of a polynomial in place;
*              inputs assumed to be in bitreversed order, output in normal order
*
* Arguments:   - uint16_t *r: pointer to in/output polynomial
**************************************************/
void poly_invntt_tomont(poly *r)
{
    invntt_avx(r->coeffs, qdata);
}

/*************************************************
* Name:        poly_basemul_montgomery
*
* Description: Multiplication of two polynomials in NTT domain
*
* Arguments:   - poly *r:       pointer to output polynomial
*              - const poly *a: pointer to first input polynomial
*              - const poly *b: pointer to second input polynomial
**************************************************/
void poly_basemul_montgomery(poly *r, const poly *a, const poly *b)
{
    poly_basemul_montgomery_avx(r->coeffs, a->coeffs, b->coeffs, qdata);
}

/*************************************************
* Name:        poly_tomont
*
* Description: Inplace conversion of all coefficients of a polynomial
*              from normal domain to Montgomery domain
*
* Arguments:   - poly *r: pointer to input/output polynomial
**************************************************/
void poly_tomont(poly *r)
{
    const __m256i montsq = _mm256_set1_epi16(MONTSQ);
    const __m256i q      = _mm256_set1_epi16(Q);
    const __m256i qinv   = _mm256_set1_epi16(QINV);
    for (unsigned int i = 0; i < N; i += 16) {
        __m256i a = _mm256_load_si256((const __m256i *)(r->coeffs + i));
        __m256i lo = _mm256_mullo_epi16(montsq, a);
        __m256i hi = _mm256_mulhi_epi16(montsq, a);
        __m256i u  = _mm256_mullo_epi16(lo, qinv);
        __m256i uq = _mm256_mulhi_epi16(q, u);
        a = _mm256_sub_epi16(hi, uq);
        _mm256_store_si256((__m256i *)(r->coeffs + i), a);
    }
}

/*************************************************
* Name:        poly_baseinv
*
* Description: Inversion of polynomial used for inversion
*              of element in Rq in NTT domain
*  
* Arguments:   - poly *b: pointer to the output polynomial
*              - const poly *a: pointer to the input polynomial
***************************************************/
int poly_baseinv(poly *b, const poly *a)
{
    return poly_baseinv_avx(b->coeffs, a->coeffs, qdata);
}

/*************************************************
* Name:        poly_add
*
* Description: Add two polynomials
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_add(poly *r, const poly *a, const poly *b)
{
    for (unsigned int i = 0; i < N; i += 16) {
        __m256i va = _mm256_load_si256((const __m256i *)(a->coeffs + i));
        __m256i vb = _mm256_load_si256((const __m256i *)(b->coeffs + i));
        __m256i vr = _mm256_add_epi16(va, vb);
        _mm256_store_si256((__m256i *)(r->coeffs + i), vr);
    }
}

/*************************************************
* Name:        poly_sub
*
* Description: Subtract two polynomials
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void poly_sub(poly *r, const poly *a, const poly *b)
{
    for (unsigned int i = 0; i < N; i += 16) {
        __m256i va = _mm256_load_si256((const __m256i *)(a->coeffs + i));
        __m256i vb = _mm256_load_si256((const __m256i *)(b->coeffs + i));
        __m256i vr = _mm256_sub_epi16(va, vb);
        _mm256_store_si256((__m256i *)(r->coeffs + i), vr);
    }
}

/*************************************************
* Name:        poly_sub_halfq
*
* Description: Subtract half of q from each coefficient of a polynomial
*
* Arguments: - poly *r:       pointer to output polynomial
**************************************************/
void poly_sub_halfq(poly *r)
{
    const __m256i halfq = _mm256_set1_epi16(HALF_Q);
    for (unsigned int i = 0; i < N; i += 16) {
        __m256i v = _mm256_load_si256((const __m256i *)(r->coeffs + i));
        v = _mm256_sub_epi16(v, halfq);
        _mm256_store_si256((__m256i *)(r->coeffs + i), v);
    }
    poly_freeze_centered(r);
}

/*************************************************
* Name:        poly_compress_and_pack
*
* Description: Compression and subsequent serialization of a polynomial
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (of length KEM_POLYCOMPRESSEDBYTES)
*              - poly *a:    pointer to input polynomial
**************************************************/
void poly_compress_and_pack(uint8_t r[KEM_POLYCOMPRESSEDBYTES], poly *a)
{
    unsigned int i;
    
    poly_freeze(a);

#if D == 7
    {
        const __m256i v32   = _mm256_set1_epi32(5585134);
        const __m256i half32 = _mm256_set1_epi32(Q / 2);
        uint16_t t[16] __attribute__((aligned(32)));

        for(i = 0; i < N/8; i++) {
            __m128i c16 = _mm_load_si128((const __m128i *)(a->coeffs + 8*i));
            __m256i c32 = _mm256_cvtepi16_epi32(c16);
            c32 = _mm256_add_epi32(_mm256_slli_epi32(c32, D), half32);

            __m256i c_odd = _mm256_srli_epi64(c32, 32);
            __m256i q_even = _mm256_srli_epi64(_mm256_mul_epu32(c32, v32), 32);
            __m256i q_odd  = _mm256_srli_epi64(_mm256_mul_epu32(c_odd, v32), 32);

            /* packus is lane-interleaved; permute fixes to sequential */
            __m256i packed = _mm256_packus_epi32(q_even, q_odd);
            packed = _mm256_permute4x64_epi64(packed, _MM_SHUFFLE(3, 1, 2, 0));
            _mm256_store_si256((__m256i *)t, packed);

            uint16_t v0 = t[0], v1 = t[8], v2 = t[2], v3 = t[10];
            uint16_t v4 = t[4], v5 = t[12], v6 = t[6], v7 = t[14];

            r[7*i+0] =  v0 | (v1 << 7);
            r[7*i+1] = (v1 >> 1) | (v2 << 6);
            r[7*i+2] = (v2 >> 2) | (v3 << 5);
            r[7*i+3] = (v3 >> 3) | (v4 << 4);
            r[7*i+4] = (v4 >> 4) | (v5 << 3);
            r[7*i+5] = (v5 >> 5) | (v6 << 2);
            r[7*i+6] = (v6 >> 6) | (v7 << 1);
        }
    }
#elif D == 8
    {
        const __m256i v32   = _mm256_set1_epi32(5585134);
        const __m256i half32 = _mm256_set1_epi32(Q / 2);
        /* Compact mask: extract low byte of uint16 at positions 0,1,2,3
         * (bytes 0,4,8,12 of each 128-bit lane) into consecutive output bytes */
        const __m256i compact_mask = _mm256_setr_epi8(
            0, 4, 8, 12, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80,
            0, 4, 8, 12, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80);

        for (i = 0; i < N; i += 16) {
            __m256i c = _mm256_load_si256((const __m256i *)(a->coeffs + i));
            __m256i c_lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(c));
            __m256i c_hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(c, 1));
            c_lo = _mm256_add_epi32(_mm256_slli_epi32(c_lo, D), half32);
            c_hi = _mm256_add_epi32(_mm256_slli_epi32(c_hi, D), half32);

            __m256i c_lo_odd = _mm256_srli_epi64(c_lo, 32);
            __m256i c_hi_odd = _mm256_srli_epi64(c_hi, 32);
            __m256i q_lo_even = _mm256_srli_epi64(_mm256_mul_epu32(c_lo, v32), 32);
            __m256i q_lo_odd  = _mm256_srli_epi64(_mm256_mul_epu32(c_lo_odd, v32), 32);
            __m256i q_hi_even = _mm256_srli_epi64(_mm256_mul_epu32(c_hi, v32), 32);
            __m256i q_hi_odd  = _mm256_srli_epi64(_mm256_mul_epu32(c_hi_odd, v32), 32);

            /* packus: [q0,0,q2,0,q4,0,q6,0, q1,0,q3,0,q5,0,q7,0] */
            __m256i packed_lo = _mm256_packus_epi32(q_lo_even, q_lo_odd);
            packed_lo = _mm256_permute4x64_epi64(packed_lo, _MM_SHUFFLE(3, 1, 2, 0));
            __m256i packed_hi = _mm256_packus_epi32(q_hi_even, q_hi_odd);
            packed_hi = _mm256_permute4x64_epi64(packed_hi, _MM_SHUFFLE(3, 1, 2, 0));

            /* Compact 4 low bytes per lane, then interleave with unpacklo_epi8 */
            __m256i compact_lo = _mm256_shuffle_epi8(packed_lo, compact_mask);
            __m256i compact_hi = _mm256_shuffle_epi8(packed_hi, compact_mask);

            uint32_t evens = (uint32_t)_mm_cvtsi128_si32(_mm256_castsi256_si128(compact_lo));
            uint32_t odds  = (uint32_t)_mm_cvtsi128_si32(_mm256_extracti128_si256(compact_lo, 1));
            __m128i ev = _mm_cvtsi32_si128(evens);
            __m128i od = _mm_cvtsi32_si128(odds);
            __m128i res = _mm_unpacklo_epi8(ev, od);
            _mm_storel_epi64((__m128i *)(r + i), res);

            evens = (uint32_t)_mm_cvtsi128_si32(_mm256_castsi256_si128(compact_hi));
            odds  = (uint32_t)_mm_cvtsi128_si32(_mm256_extracti128_si256(compact_hi, 1));
            ev = _mm_cvtsi32_si128(evens);
            od = _mm_cvtsi32_si128(odds);
            res = _mm_unpacklo_epi8(ev, od);
            _mm_storel_epi64((__m128i *)(r + i + 8), res);
        }
    }
#elif D == 9
    {
        const __m256i v32   = _mm256_set1_epi32(1290168);  // ceil(2^32/Q)
        const __m256i half32 = _mm256_set1_epi32(Q / 2);
        uint16_t t[16] __attribute__((aligned(32)));

        for(i = 0; i < N/8; i++) {
            __m128i c16 = _mm_load_si128((const __m128i *)(a->coeffs + 8*i));
            __m256i c32 = _mm256_cvtepi16_epi32(c16);
            c32 = _mm256_add_epi32(_mm256_slli_epi32(c32, D), half32);

            __m256i c_odd = _mm256_srli_epi64(c32, 32);
            __m256i q_even = _mm256_srli_epi64(_mm256_mul_epu32(c32, v32), 32);
            __m256i q_odd  = _mm256_srli_epi64(_mm256_mul_epu32(c_odd, v32), 32);

            __m256i packed = _mm256_packus_epi32(q_even, q_odd);
            packed = _mm256_permute4x64_epi64(packed, _MM_SHUFFLE(3, 1, 2, 0));
            _mm256_store_si256((__m256i *)t, packed);

            uint16_t v0 = t[0], v1 = t[8], v2 = t[2], v3 = t[10];
            uint16_t v4 = t[4], v5 = t[12], v6 = t[6], v7 = t[14];

            r[9*i+0] = (uint8_t)(v0);
            r[9*i+1] = (uint8_t)((v0 >> 8) | (v1 << 1));
            r[9*i+2] = (uint8_t)((v1 >> 7) | (v2 << 2));
            r[9*i+3] = (uint8_t)((v2 >> 6) | (v3 << 3));
            r[9*i+4] = (uint8_t)((v3 >> 5) | (v4 << 4));
            r[9*i+5] = (uint8_t)((v4 >> 4) | (v5 << 5));
            r[9*i+6] = (uint8_t)((v5 >> 3) | (v6 << 6));
            r[9*i+7] = (uint8_t)((v6 >> 2) | (v7 << 7));
            r[9*i+8] = (uint8_t)(v7 >> 1);
        }
    }
#endif
}

/*************************************************
* Name:        poly_unpack_and_decompress
*
* Description: De-serialization and subsequent decompression of a polynomial;
*              approximate inverse of poly_compress_and_pack
*
* Arguments:   - poly *r:          pointer to output polynomial
*              - const uint8_t *a: pointer to input byte array
*                                  (of length KEM_POLYCOMPRESSEDBYTES bytes)
**************************************************/
void poly_unpack_and_decompress(poly *r, const uint8_t a[KEM_POLYCOMPRESSEDBYTES])
{
    unsigned int i;
#if D == 7
    {
        const __m128i q32 = _mm_set1_epi32(Q);
        const __m128i half32 = _mm_set1_epi32(1 << (D-1));
        for (i = 0; i < N/8; i++) {
            uint16_t t[8] __attribute__((aligned(16)));
            t[0] = (a[7*i+0] & 0x7F);
            t[1] = ((a[7*i+0] >> 7) | ((a[7*i+1] & 0x3F) << 1));
            t[2] = ((a[7*i+1] >> 6) | ((a[7*i+2] & 0x1F) << 2));
            t[3] = ((a[7*i+2] >> 5) | ((a[7*i+3] & 0x0F) << 3));
            t[4] = ((a[7*i+3] >> 4) | ((a[7*i+4] & 0x07) << 4));
            t[5] = ((a[7*i+4] >> 3) | ((a[7*i+5] & 0x03) << 5));
            t[6] = ((a[7*i+5] >> 2) | ((a[7*i+6] & 0x01) << 6));
            t[7] = (a[7*i+6] >> 1);
            __m128i tv = _mm_load_si128((const __m128i *)t);
            __m128i t32_lo = _mm_cvtepi16_epi32(tv);
            __m128i t32_hi = _mm_cvtepi16_epi32(_mm_unpackhi_epi64(tv, tv));
            t32_lo = _mm_srli_epi32(_mm_add_epi32(_mm_mullo_epi32(t32_lo, q32), half32), D);
            t32_hi = _mm_srli_epi32(_mm_add_epi32(_mm_mullo_epi32(t32_hi, q32), half32), D);
            __m128i result = _mm_packus_epi32(t32_lo, t32_hi);
            _mm_store_si128((__m128i *)&r->coeffs[8*i], result);
        }
    }
#elif D == 8
    {
        const __m256i q32 = _mm256_set1_epi32(Q);
        const __m256i half32 = _mm256_set1_epi32(1 << (D-1));
        for (i = 0; i < N; i += 16) {
            __m128i a8 = _mm_load_si128((const __m128i *)&a[i]);
            __m256i a16 = _mm256_cvtepu8_epi16(a8);
            __m256i a32_lo = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(a16));
            __m256i a32_hi = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(a16, 1));
            a32_lo = _mm256_srli_epi32(_mm256_add_epi32(_mm256_mullo_epi32(a32_lo, q32), half32), D);
            a32_hi = _mm256_srli_epi32(_mm256_add_epi32(_mm256_mullo_epi32(a32_hi, q32), half32), D);
            __m256i result = _mm256_packus_epi32(a32_lo, a32_hi);
            result = _mm256_permute4x64_epi64(result, _MM_SHUFFLE(3, 1, 2, 0));
            _mm256_store_si256((__m256i *)&r->coeffs[i], result);
        }
    }
#elif D == 9
    {
        const __m128i q32 = _mm_set1_epi32(Q);
        const __m128i half32 = _mm_set1_epi32(1 << (D-1));
        for (i = 0; i < N/8; i++) {
            uint16_t t[8] __attribute__((aligned(16)));
            t[0] = (uint16_t)(a[9*i+0]) | ((uint16_t)(a[9*i+1] & 0x01) << 8);
            t[1] = ((uint16_t)(a[9*i+1] >> 1) & 0x7F) | ((uint16_t)(a[9*i+2] & 0x03) << 7);
            t[2] = ((uint16_t)(a[9*i+2] >> 2) & 0x3F) | ((uint16_t)(a[9*i+3] & 0x07) << 6);
            t[3] = ((uint16_t)(a[9*i+3] >> 3) & 0x1F) | ((uint16_t)(a[9*i+4] & 0x0F) << 5);
            t[4] = ((uint16_t)(a[9*i+4] >> 4) & 0x0F) | ((uint16_t)(a[9*i+5] & 0x1F) << 4);
            t[5] = ((uint16_t)(a[9*i+5] >> 5) & 0x07) | ((uint16_t)(a[9*i+6] & 0x3F) << 3);
            t[6] = ((uint16_t)(a[9*i+6] >> 6) & 0x03) | ((uint16_t)(a[9*i+7] & 0x7F) << 2);
            t[7] = ((uint16_t)(a[9*i+7] >> 7) & 0x01) | ((uint16_t)(a[9*i+8]) << 1);
            __m128i tv = _mm_load_si128((const __m128i *)t);
            __m128i t32_lo = _mm_cvtepi16_epi32(tv);
            __m128i t32_hi = _mm_cvtepi16_epi32(_mm_unpackhi_epi64(tv, tv));
            t32_lo = _mm_srli_epi32(_mm_add_epi32(_mm_mullo_epi32(t32_lo, q32), half32), D);
            t32_hi = _mm_srli_epi32(_mm_add_epi32(_mm_mullo_epi32(t32_hi, q32), half32), D);
            __m128i result = _mm_packus_epi32(t32_lo, t32_hi);
            _mm_store_si128((__m128i *)&r->coeffs[8*i], result);
        }
    }
#endif
}

/*************************************************
* Name:        poly_to_bytes
*
* Description: Serialization of a polynomial
*
* Arguments:   - uint8_t *r: pointer to output byte array
*                            (needs space for KEM_POLYBYTES bytes)
*              - poly *a:    pointer to input polynomial
**************************************************/
void poly_to_bytes(uint8_t r[KEM_POLYBYTES], const poly *a)
{
    unsigned int i;
#if KEM_MODE == 128
    {
        const int16_t *coeffs = a->coeffs;

        for (i = 0; i < 102; i += 2) {
            /* group A: coeffs[0..4], group B: coeffs[5..9] */
            uint16_t ca0 = (uint16_t)coeffs[5*i + 0];
            uint16_t ca1 = (uint16_t)coeffs[5*i + 1];
            uint16_t ca2 = (uint16_t)coeffs[5*i + 2];
            uint16_t ca3 = (uint16_t)coeffs[5*i + 3];
            uint16_t ca4 = (uint16_t)coeffs[5*i + 4];
            uint16_t cb0 = (uint16_t)coeffs[5*i + 5];
            uint16_t cb1 = (uint16_t)coeffs[5*i + 6];
            uint16_t cb2 = (uint16_t)coeffs[5*i + 7];
            uint16_t cb3 = (uint16_t)coeffs[5*i + 8];
            uint16_t cb4 = (uint16_t)coeffs[5*i + 9];

            /* L: interleaved compute */
            uint32_t L_a = (ca0 & 7) | ((ca1 & 7) << 3) | ((ca2 & 7) << 6) |
                           ((ca3 & 7) << 9) | ((ca4 & 7) << 12);
            uint32_t L_b = (cb0 & 7) | ((cb1 & 7) << 3) | ((cb2 & 7) << 6) |
                           ((cb3 & 7) << 9) | ((cb4 & 7) << 12);

            /* H: interleaved multiply-accumulate to hide latency */
            uint64_t ha0 = ca0 >> 3;
            uint64_t hb0 = cb0 >> 3;
            uint64_t ha1 = (ca1 >> 3) * 97ULL;
            uint64_t hb1 = (cb1 >> 3) * 97ULL;
            uint64_t H_a = ha0 + ha1;
            uint64_t H_b = hb0 + hb1;

            ha1 = (ca2 >> 3) * 9409ULL;
            hb1 = (cb2 >> 3) * 9409ULL;
            H_a += ha1; H_b += hb1;

            ha1 = (ca3 >> 3) * 912673ULL;
            hb1 = (cb3 >> 3) * 912673ULL;
            H_a += ha1; H_b += hb1;

            ha1 = (ca4 >> 3) * 88529281ULL;
            hb1 = (cb4 >> 3) * 88529281ULL;
            H_a += ha1; H_b += hb1;

            uint64_t code_a = (H_a << 15) | L_a;
            uint64_t code_b = (H_b << 15) | L_b;

            memcpy(r + 6*i,      &code_a, 6);
            memcpy(r + 6*i + 6,  &code_b, 6);
        }

        /* tail: 2 coefficients → 20 bits */
        {
            uint32_t t;
            t  = (uint32_t)(uint16_t)a->coeffs[510];
            t |= (uint32_t)(uint16_t)a->coeffs[511] << 10;
            memcpy(r + 612, &t, 3);
        }
    }
#elif KEM_MODE == 256
    {
        const int16_t *coeffs = a->coeffs;

        for (i = 0; i < 204; i += 2) {
            uint16_t ca0 = (uint16_t)coeffs[5*i + 0];
            uint16_t ca1 = (uint16_t)coeffs[5*i + 1];
            uint16_t ca2 = (uint16_t)coeffs[5*i + 2];
            uint16_t ca3 = (uint16_t)coeffs[5*i + 3];
            uint16_t ca4 = (uint16_t)coeffs[5*i + 4];
            uint16_t cb0 = (uint16_t)coeffs[5*i + 5];
            uint16_t cb1 = (uint16_t)coeffs[5*i + 6];
            uint16_t cb2 = (uint16_t)coeffs[5*i + 7];
            uint16_t cb3 = (uint16_t)coeffs[5*i + 8];
            uint16_t cb4 = (uint16_t)coeffs[5*i + 9];

            uint32_t L_a = (ca0 & 7) | ((ca1 & 7) << 3) | ((ca2 & 7) << 6) |
                           ((ca3 & 7) << 9) | ((ca4 & 7) << 12);
            uint32_t L_b = (cb0 & 7) | ((cb1 & 7) << 3) | ((cb2 & 7) << 6) |
                           ((cb3 & 7) << 9) | ((cb4 & 7) << 12);

            uint64_t ha0 = ca0 >> 3;
            uint64_t hb0 = cb0 >> 3;
            uint64_t ha1 = (ca1 >> 3) * 97ULL;
            uint64_t hb1 = (cb1 >> 3) * 97ULL;
            uint64_t H_a = ha0 + ha1;
            uint64_t H_b = hb0 + hb1;

            ha1 = (ca2 >> 3) * 9409ULL;
            hb1 = (cb2 >> 3) * 9409ULL;
            H_a += ha1; H_b += hb1;

            ha1 = (ca3 >> 3) * 912673ULL;
            hb1 = (cb3 >> 3) * 912673ULL;
            H_a += ha1; H_b += hb1;

            ha1 = (ca4 >> 3) * 88529281ULL;
            hb1 = (cb4 >> 3) * 88529281ULL;
            H_a += ha1; H_b += hb1;

            uint64_t code_a = (H_a << 15) | L_a;
            uint64_t code_b = (H_b << 15) | L_b;

            memcpy(r + 6*i,      &code_a, 6);
            memcpy(r + 6*i + 6,  &code_b, 6);
        }

        /* tail: 4 coefficients → 40 bits */
        {
            uint64_t t;
            t  = (uint64_t)(uint16_t)a->coeffs[1020];
            t |= (uint64_t)(uint16_t)a->coeffs[1021] << 10;
            t |= (uint64_t)(uint16_t)a->coeffs[1022] << 20;
            t |= (uint64_t)(uint16_t)a->coeffs[1023] << 30;
            memcpy(r + 1224, &t, 5);
        }
    }

#elif KEM_MODE == 512
    {
        uint16_t c0, c1;

        poly_freeze((poly *)a);

        for (i = 0; i < N / 2; i++) {
            c0 = (uint16_t)a->coeffs[2*i + 0];
            c1 = (uint16_t)a->coeffs[2*i + 1];

            r[3*i + 0] = (uint8_t)(c0);
            r[3*i + 1] = (uint8_t)((c0 >> 8) | (c1 << 4));
            r[3*i + 2] = (uint8_t)(c1 >> 4);
        }
    }
#else
    #error "Unsupported KEM_MODE: must be 128, 256, or 512"
#endif
}

/*************************************************
* Name:        poly_from_bytes
*
* Description: De-serialization of a polynomial;
*              inverse of poly_to_bytes
*
* Arguments:   - poly *r:          pointer to output polynomial
*              - const uint8_t *a: pointer to input byte array
*                                  (of KEM_POLYBYTES bytes)
**************************************************/
void poly_from_bytes(poly *r, const uint8_t a[KEM_POLYBYTES])
{
    unsigned int i;

#if KEM_MODE == 128
    {
        int16_t *coeffs = r->coeffs;

        for (i = 0; i < 102; i += 2) {
            uint64_t code0, code1;
            memcpy(&code0, a + 6*i, 8);
            memcpy(&code1, a + 6*i + 6, 8);
            code0 &= 0xFFFFFFFFFFFFULL;
            code1 &= 0xFFFFFFFFFFFFULL;

            uint32_t L0 = (uint32_t)(code0 & 0x7FFF);
            uint64_t H0 = code0 >> 15;
            uint32_t L1 = (uint32_t)(code1 & 0x7FFF);
            uint64_t H1 = code1 >> 15;

            /* j=0: 64-bit division for both groups */
            uint16_t l00 = L0 & 7;
            uint16_t h00 = (uint16_t)(H0 % 97);
            H0 /= 97;
            uint16_t l10 = L1 & 7;
            uint16_t h10 = (uint16_t)(H1 % 97);
            H1 /= 97;
            coeffs[5*i + 0] = (int16_t)((h00 << 3) | l00);
            coeffs[5*i + 5] = (int16_t)((h10 << 3) | l10);

            /* j=1..4: H < 2^27, use 32-bit division */
            uint32_t H0_32 = (uint32_t)H0;
            uint32_t H1_32 = (uint32_t)H1;

            /* j=1 */
            uint16_t l01 = (L0 >> 3) & 7;
            uint16_t h01 = (uint16_t)(H0_32 % 97);
            H0_32 /= 97;
            uint16_t l11 = (L1 >> 3) & 7;
            uint16_t h11 = (uint16_t)(H1_32 % 97);
            H1_32 /= 97;
            coeffs[5*i + 1] = (int16_t)((h01 << 3) | l01);
            coeffs[5*i + 6] = (int16_t)((h11 << 3) | l11);

            /* j=2 */
            uint16_t l02 = (L0 >> 6) & 7;
            uint16_t h02 = (uint16_t)(H0_32 % 97);
            H0_32 /= 97;
            uint16_t l12 = (L1 >> 6) & 7;
            uint16_t h12 = (uint16_t)(H1_32 % 97);
            H1_32 /= 97;
            coeffs[5*i + 2] = (int16_t)((h02 << 3) | l02);
            coeffs[5*i + 7] = (int16_t)((h12 << 3) | l12);

            /* j=3 */
            uint16_t l03 = (L0 >> 9) & 7;
            uint16_t h03 = (uint16_t)(H0_32 % 97);
            H0_32 /= 97;
            uint16_t l13 = (L1 >> 9) & 7;
            uint16_t h13 = (uint16_t)(H1_32 % 97);
            H1_32 /= 97;
            coeffs[5*i + 3] = (int16_t)((h03 << 3) | l03);
            coeffs[5*i + 8] = (int16_t)((h13 << 3) | l13);

            /* j=4 */
            uint16_t l04 = (L0 >> 12) & 7;
            uint16_t h04 = (uint16_t)(H0_32 % 97);
            uint16_t l14 = (L1 >> 12) & 7;
            uint16_t h14 = (uint16_t)(H1_32 % 97);
            coeffs[5*i + 4] = (int16_t)((h04 << 3) | l04);
            coeffs[5*i + 9] = (int16_t)((h14 << 3) | l14);
        }

        {
            uint32_t t;
            t  = (uint32_t)a[612];
            t |= (uint32_t)a[613] <<  8;
            t |= (uint32_t)a[614] << 16;
            r->coeffs[510] = (int16_t)( t        & 0x3FF);
            r->coeffs[511] = (int16_t)((t >> 10) & 0x3FF);
        }
    }
#elif KEM_MODE == 256
    {
        int16_t *coeffs = r->coeffs;

        for (i = 0; i < 204; i += 2) {
            uint64_t code0, code1;
            memcpy(&code0, a + 6*i, 8);
            memcpy(&code1, a + 6*i + 6, 8);
            code0 &= 0xFFFFFFFFFFFFULL;
            code1 &= 0xFFFFFFFFFFFFULL;

            uint32_t L0 = (uint32_t)(code0 & 0x7FFF);
            uint64_t H0 = code0 >> 15;
            uint32_t L1 = (uint32_t)(code1 & 0x7FFF);
            uint64_t H1 = code1 >> 15;

            /* j=0: 64-bit division for both groups */
            uint16_t l00 = L0 & 7;
            uint16_t h00 = (uint16_t)(H0 % 97);
            H0 /= 97;
            uint16_t l10 = L1 & 7;
            uint16_t h10 = (uint16_t)(H1 % 97);
            H1 /= 97;
            coeffs[5*i + 0] = (int16_t)((h00 << 3) | l00);
            coeffs[5*i + 5] = (int16_t)((h10 << 3) | l10);

            /* j=1..4: H < 2^27, 32-bit division */
            uint32_t H0_32 = (uint32_t)H0;
            uint32_t H1_32 = (uint32_t)H1;

            /* j=1 */
            uint16_t l01 = (L0 >> 3) & 7;
            uint16_t h01 = (uint16_t)(H0_32 % 97);
            H0_32 /= 97;
            uint16_t l11 = (L1 >> 3) & 7;
            uint16_t h11 = (uint16_t)(H1_32 % 97);
            H1_32 /= 97;
            coeffs[5*i + 1] = (int16_t)((h01 << 3) | l01);
            coeffs[5*i + 6] = (int16_t)((h11 << 3) | l11);

            /* j=2 */
            uint16_t l02 = (L0 >> 6) & 7;
            uint16_t h02 = (uint16_t)(H0_32 % 97);
            H0_32 /= 97;
            uint16_t l12 = (L1 >> 6) & 7;
            uint16_t h12 = (uint16_t)(H1_32 % 97);
            H1_32 /= 97;
            coeffs[5*i + 2] = (int16_t)((h02 << 3) | l02);
            coeffs[5*i + 7] = (int16_t)((h12 << 3) | l12);

            /* j=3 */
            uint16_t l03 = (L0 >> 9) & 7;
            uint16_t h03 = (uint16_t)(H0_32 % 97);
            H0_32 /= 97;
            uint16_t l13 = (L1 >> 9) & 7;
            uint16_t h13 = (uint16_t)(H1_32 % 97);
            H1_32 /= 97;
            coeffs[5*i + 3] = (int16_t)((h03 << 3) | l03);
            coeffs[5*i + 8] = (int16_t)((h13 << 3) | l13);

            /* j=4 */
            uint16_t l04 = (L0 >> 12) & 7;
            uint16_t h04 = (uint16_t)(H0_32 % 97);
            uint16_t l14 = (L1 >> 12) & 7;
            uint16_t h14 = (uint16_t)(H1_32 % 97);
            coeffs[5*i + 4] = (int16_t)((h04 << 3) | l04);
            coeffs[5*i + 9] = (int16_t)((h14 << 3) | l14);
        }

        {
            uint64_t t;
            t  = (uint64_t)a[1224];
            t |= (uint64_t)a[1225] <<  8;
            t |= (uint64_t)a[1226] << 16;
            t |= (uint64_t)a[1227] << 24;
            t |= (uint64_t)a[1228] << 32;
            r->coeffs[1020] = (int16_t)( t        & 0x3FF);
            r->coeffs[1021] = (int16_t)((t >> 10) & 0x3FF);
            r->coeffs[1022] = (int16_t)((t >> 20) & 0x3FF);
            r->coeffs[1023] = (int16_t)((t >> 30) & 0x3FF);
        }
    }
#elif KEM_MODE == 512
    for (i = 0; i < N / 2; i++) {
        r->coeffs[2*i + 0] = (int16_t)( (uint16_t) a[3*i + 0]             |
                                        ((uint16_t)(a[3*i + 1] & 0x0F) << 8));
        r->coeffs[2*i + 1] = (int16_t)(((uint16_t) a[3*i + 1] >> 4)       |
                                        ((uint16_t) a[3*i + 2]        << 4));
    }

#else
    #error "Unsupported KEM_MODE"
#endif
}

/*************************************************
* Name:        poly_from_msg
*
* Description: Convert message to polynomial
*
* Arguments:   - poly *r:            pointer to output polynomial
*              - const uint8_t *msg: pointer to input message
**************************************************/
void poly_from_msg(poly *r, const uint8_t msg[KEM_CPAPKE_MSGBYTES])
{
    unsigned int i;
    const __m256i hq = _mm256_set1_epi16(HALF_Q);
    const __m256i z  = _mm256_setzero_si256();

#if REPETITIONS == 2
    for (i = 0; i < KEM_CPAPKE_MSGBYTES; i += 2) {
        uint64_t lo = _pdep_u64(msg[i],     0x0101010101010101ULL);
        uint64_t hi = _pdep_u64(msg[i + 1], 0x0101010101010101ULL);
        __m128i b8  = _mm_set_epi64x(hi, lo);
        __m256i b16 = _mm256_cvtepu8_epi16(b8);
        __m256i val = _mm256_and_si256(_mm256_cmpgt_epi16(b16, z), hq);
        _mm256_store_si256((__m256i *)&r->coeffs[8*i], val);
        _mm256_store_si256((__m256i *)&r->coeffs[8*i + N/2], val);
    }
#elif REPETITIONS == 4
    for (i = 0; i < KEM_CPAPKE_MSGBYTES; i += 2) {
        uint64_t lo = _pdep_u64(msg[i],     0x0101010101010101ULL);
        uint64_t hi = _pdep_u64(msg[i + 1], 0x0101010101010101ULL);
        __m128i b8  = _mm_set_epi64x(hi, lo);
        __m256i b16 = _mm256_cvtepu8_epi16(b8);
        __m256i val = _mm256_and_si256(_mm256_cmpgt_epi16(b16, z), hq);
        _mm256_store_si256((__m256i *)&r->coeffs[8*i], val);
        _mm256_store_si256((__m256i *)&r->coeffs[8*i + N/4], val);
        _mm256_store_si256((__m256i *)&r->coeffs[8*i + N/2], val);
        _mm256_store_si256((__m256i *)&r->coeffs[8*i + 3*N/4], val);
    }
#endif
}

/*************************************************
* Name:        ternary_p
*
* Description: Sample a polynomial with coefficients in {-1,0,1} according to probabilities
*              P(-1) = P(1) = P, P(0) = 1 - 2*P
*
* Arguments:   - poly *r:            pointer to output polynomial
*              - uint8_t *buf:        pointer to input buffer for randomness
*                                      (needs to have enough bytes to sample N coefficients)
**************************************************/
static void ternary_15625(poly *r, const uint8_t *buf)  // 5/32
{
    unsigned int i, j, k;

    for (i = 0; i < N/8; i++) {
        uint64_t w = 0;

        for (j = 0; j < 5; j++)
        {
            w |= (uint64_t)buf[5*i + j] << (8*j);
        }

        for (k = 0; k < 8; k++) {
            uint16_t v = (w >> (5*k)) & 0x1F;
            uint16_t lt5 = (v - 5) >> 15;
            uint16_t lt10 = (v - 10) >> 15;
            r->coeffs[8*i + k] = (int16_t)lt5 - (int16_t)((lt5 ^ 1) & lt10);
        }
    }
}
static void ternary_1875(poly *r, const uint8_t *buf) // 3/16
{
    unsigned int i;
    for (i = 0; i < N/2; i++) {
        uint8_t w = buf[i];
        uint16_t lo = w & 0x0F;
        uint16_t hi = w >> 4;
        uint16_t lt3_lo = (uint16_t)(lo - 3) >> 15;
        uint16_t lt6_lo = (uint16_t)(lo - 6) >> 15;
        uint16_t lt3_hi = (uint16_t)(hi - 3) >> 15;
        uint16_t lt6_hi = (uint16_t)(hi - 6) >> 15;

        r->coeffs[2*i]   = (int16_t)((2*lt3_lo - 1) * lt6_lo);
        r->coeffs[2*i+1] = (int16_t)((2*lt3_hi - 1) * lt6_hi);
    }
}
static void ternary_3125(poly *r, const uint8_t *buf) // 5/16
{
    unsigned int i;
    for (i = 0; i < N/2; i++) {
        uint8_t w = buf[i];

        uint16_t lo = w & 0x0F;
        uint16_t hi = w >> 4;
        uint16_t lt5_lo = (uint16_t)(lo - 5) >> 15;
        uint16_t lt10_lo = (uint16_t)(lo - 10) >> 15;
        uint16_t lt5_hi = (uint16_t)(hi - 5) >> 15;
        uint16_t lt10_hi = (uint16_t)(hi - 10) >> 15;

        r->coeffs[2*i]   = (int16_t)lt5_lo - (int16_t)((lt5_lo ^ 1) & lt10_lo);
        r->coeffs[2*i+1] = (int16_t)lt5_hi - (int16_t)((lt5_hi ^ 1) & lt10_hi);
    }
}
static void ternary_4375(poly *r, const uint8_t *buf) // 7/16
{
    unsigned int i;
    for (i = 0; i < N/2; i++) {
        uint8_t w = buf[i];

        uint16_t lo = w & 0x0F;
        uint16_t hi = w >> 4;
        uint16_t lt7_lo  = (uint16_t)(lo - 7) >> 15;
        uint16_t lt14_lo = (uint16_t)(lo - 14) >> 15;
        uint16_t lt7_hi  = (uint16_t)(hi - 7) >> 15;
        uint16_t lt14_hi = (uint16_t)(hi - 14) >> 15;

        r->coeffs[2*i] =  (int16_t)lt7_lo - (int16_t)((lt7_lo ^ 1) & lt14_lo);
        r->coeffs[2*i+1] = (int16_t)lt7_hi - (int16_t)((lt7_hi ^ 1) & lt14_hi);
    }
}
static void ternary_25(poly *r, const uint8_t *buf) // 1/4
{
    unsigned int i;
    for(i = 0; i < N/4; i++) {
        uint8_t w = buf[i];

        r->coeffs[4*i+0] = (int16_t)((w >> 0) & 1) - (int16_t)((w >> 1) & 1);
        r->coeffs[4*i+1] = (int16_t)((w >> 2) & 1) - (int16_t)((w >> 3) & 1);
        r->coeffs[4*i+2] = (int16_t)((w >> 4) & 1) - (int16_t)((w >> 5) & 1);
        r->coeffs[4*i+3] = (int16_t)((w >> 6) & 1) - (int16_t)((w >> 7) & 1);
    }
}
static void ternary_125(poly *r, const uint8_t *buf) // 1/8
{
    unsigned int i,j;

    for (i = 0; i < N/8; i++) {
        uint32_t w = (uint32_t)buf[3*i] | ((uint32_t)buf[3*i+1] << 8) | ((uint32_t)buf[3*i+2] << 16);

        for (j = 0; j < 8; j++) {
            uint16_t v = (w >> (3*j)) & 0x07;
            uint16_t lt3 = (uint16_t)(v - 1) >> 15;
            uint16_t lt6 = (uint16_t)(v - 2) >> 15;
            r->coeffs[8*i + j] = (int16_t)((2*lt3 - 1) * lt6);
        }
    }
}
static void ternary_375(poly *r, const uint8_t *buf) // 3/8
{
    unsigned int i, j;

    for (i = 0; i < N/8; i++) {
        uint32_t w = (uint32_t)buf[3*i] | ((uint32_t)buf[3*i+1] << 8) | ((uint32_t)buf[3*i+2] << 16);

        for (j = 0; j < 8; j++) {
            uint16_t v = (w >> (3*j)) & 0x07;
            uint16_t lt3 = (uint16_t)(v - 3) >> 15;
            uint16_t lt6 = (uint16_t)(v - 6) >> 15;
            r->coeffs[8*i + j] = (int16_t)((2*lt3 - 1) * lt6);
        }
    }
}

/*************************************************
* Name:        poly_ternary_p
*
* Description: Dispatch ternary polynomial sampling based on probability.
*              Wrappers pass compile-time constants (PF/PG/PR/PE), so the
*              switch is constant-folded by the optimizer.
*
* Arguments:   - poly *r:            pointer to output polynomial
*              - const uint8_t *seed: pointer to input seed
*              - uint8_t nonce:       domain-separation nonce
*              - int prob:            one of {125, 3125, 25, 15625, 4375}
**************************************************/
static void poly_ternary_p(poly *r, const uint8_t seed[SEEDBYTES],
                            uint8_t nonce, int prob)
{
    switch (prob) {
    case 125: {   // 1/8
        uint8_t buf[N * 3/8];
        prf(buf, sizeof(buf), seed, nonce);
        ternary_125(r, buf);
        break;
    }
    case 3125: {  // 5/16
        uint8_t buf[N * 4/8];
        prf(buf, sizeof(buf), seed, nonce);
        ternary_3125(r, buf);
        break;
    }
    case 25: {    // 1/4
        uint8_t buf[N * 2/8];
        prf(buf, sizeof(buf), seed, nonce);
        ternary_25(r, buf);
        break;
    }
    case 15625: { // 5/32
        uint8_t buf[N * 5/8];
        prf(buf, sizeof(buf), seed, nonce);
        ternary_15625(r, buf);
        break;
    }
    case 4375: {  // 7/16
        uint8_t buf[N * 4/8];
        prf(buf, sizeof(buf), seed, nonce);
        ternary_4375(r, buf);
        break;
    }
    }
}

void poly_f_ternary_p(poly *r, const uint8_t seed[SEEDBYTES], uint8_t nonce) {
    poly_ternary_p(r, seed, nonce, PF);
}
void poly_g_ternary_p(poly *r, const uint8_t seed[SEEDBYTES], uint8_t nonce) {
    poly_ternary_p(r, seed, nonce, PG);
}
void poly_r_ternary_p(poly *r, const uint8_t seed[SEEDBYTES], uint8_t nonce) {
    poly_ternary_p(r, seed, nonce, PR);
}
void poly_e_ternary_p(poly *r, const uint8_t seed[SEEDBYTES], uint8_t nonce) {
    poly_ternary_p(r, seed, nonce, PE);
}