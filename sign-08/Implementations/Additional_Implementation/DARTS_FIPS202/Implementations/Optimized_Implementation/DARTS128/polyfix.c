#include "polyfix.h"
#include "math.h"
#include "ntt.h"
#include "params.h"
#include "reduce_avx.h"
#include "symmetric.h"
#include "consts.h"
#include <stdint.h>
#include <immintrin.h>

/*************************************************
 * Name:        polyfix_add
 *
 * Description: Add double polynomial and integer polynomial.
 *              No modular reduction is performed.
 *
 * Arguments:   - polyfix *c: pointer to output double polynomial
 *              - const polyfix *a: pointer to first summand
 *              - const poly *b: pointer to second summand
 **************************************************/
void polyfix_add(polyfix *c, const polyfix *a, const poly *b) {
    unsigned int i;

    const __m256i v_lnbits = _mm256_load_si256((const __m256i *)&qdata[_8XLNBITS]);

    for (i = 0; i < N; i += 8) {
        __m256i vb = _mm256_loadu_si256((__m256i *)&b->coeffs[i]);
        // vb 左移 14 位，完美等价于乘以 16384
        __m256i vbln = _mm256_sllv_epi32(vb, v_lnbits);
        __m256i va = _mm256_loadu_si256((__m256i *)&a->coeffs[i]);
        __m256i vc = _mm256_add_epi32(va, vbln);
        _mm256_storeu_si256((__m256i *)&c->coeffs[i], vc);
    }
}
/*************************************************
 * Name:        polyfixfix_sub
 *
 * Description: Subtract fixed polynomial and fixed polynomial.
 *              No modular reduction is performed.
 *
 * Arguments:   - polyfix *c: pointer to output fixed polynomial
 *              - const polyfix *a: pointer to first summand
 *              - const polyfix *b: pointer to second summand
 **************************************************/
void polyfixfix_sub(polyfix *c, const polyfix *a, const polyfix *b) {
    unsigned int i;

    for (i = 0; i < N; i += 8) {

        __m256i va = _mm256_loadu_si256((__m256i *)&a->coeffs[i]);
        __m256i vb = _mm256_loadu_si256((__m256i *)&b->coeffs[i]);
        __m256i vc = _mm256_sub_epi32(va, vb);

        _mm256_storeu_si256((__m256i *)&c->coeffs[i], vc);
    }
}

/*************************************************
 * Name:        polyfix_round
 *
 * Description: rounds a fixed polynomial to integer polynomial
 *
 * Arguments:   - poly *a: output integer polynomial
 *              - poly *b: input fixed polynomial
 **************************************************/
void polyfix_round(poly *a, const polyfix *b) {
    unsigned int i;


    const __m256i v_lnhalf = _mm256_load_si256((const __m256i *)&qdata[_8XLNHALF]);
    const __m256i v_lnbits = _mm256_load_si256((const __m256i *)&qdata[_8XLNBITS]);

    for (i = 0; i < N; i += 8) {

        __m256i vb = _mm256_loadu_si256((__m256i *)&b->coeffs[i]);
        __m256i v_add = _mm256_add_epi32(vb, v_lnhalf);
        __m256i v_res = _mm256_srav_epi32(v_add, v_lnbits);

        _mm256_storeu_si256((__m256i *)&a->coeffs[i], v_res);
    }
}

/**************************************************************/
/********* Double Vectors of polynomials of length K **********/
/**************************************************************/

/*************************************************
 * Name:        polyfixveck_add
 *
 * Description: Add vector to a vector of double polynomials of length K.
 *              No modular reduction is performed.
 *
 * Arguments:   - polyveck *w: pointer to output vector
 *              - const polyveck *u: pointer to first summand
 *              - const polyveck *v: pointer to second summand
 **************************************************/
void polyfixveck_add(polyfixveck *w, const polyfixveck *u, const polyveck *v) {
    unsigned int i;

    for (i = 0; i < K; ++i)
        polyfix_add(&w->vec[i], &u->vec[i], &v->vec[i]);
}

/*************************************************
 * Name:        polyfixfixveck_sub
 *
 * Description: subtract vector to a vector of fixed polynomials of length k.
 *              No modular reduction is performed.
 *
 * Arguments:   - polyveck *w: pointer to output vector
 *              - const polyfixveck *u: pointer to first summand
 *              - const polyfixveck *v: pointer to second summand
 **************************************************/
void polyfixfixveck_sub(polyfixveck *w, const polyfixveck *u,
                        const polyfixveck *v) {
    unsigned int i;

    for (i = 0; i < K; ++i)
        polyfixfix_sub(&w->vec[i], &u->vec[i], &v->vec[i]);
}

/*************************************************
 * Name:        polyfixveck_double
 *
 * Description: Double vector of polynomials of length K.
 *
 * Arguments:   - polyveck *b: pointer to output vector
 *              - polyveck *a: pointer to input vector
 **************************************************/
void polyfixveck_double(polyfixveck *b, const polyfixveck *a) {
    unsigned int i, j;

    for (i = 0; i < K; ++i) {
        for (j = 0; j < N; j += 8) {

            __m256i va = _mm256_loadu_si256((__m256i *)&a->vec[i].coeffs[j]);
            __m256i vb = _mm256_add_epi32(va, va);

            _mm256_storeu_si256((__m256i *)&b->vec[i].coeffs[j], vb);
        }
    }
}

/*************************************************
 * Name:        polyfixveck_round
 *
 * Description: rounds a fixed polynomial vector of length K
 *
 * Arguments:   - polyveck *a: output integer polynomial vector
 *              - polyfixveck *b: input fixed polynomial vector
 **************************************************/
void polyfixveck_round(polyveck *a, const polyfixveck *b) {
    unsigned i;

    for (i = 0; i < K; ++i)
        polyfix_round(&a->vec[i], &b->vec[i]);
}

/**************************************************************/
/********* Double Vectors of polynomials of length L **********/
/**************************************************************/

/*************************************************
 * Name:        polyfixvecl_add
 *
 * Description: Add vector to a vector of double polynomials of length L.
 *              No modular reduction is performed.
 *
 * Arguments:   - polyvecl *w: pointer to output vector
 *              - const polyfixvecl *u: pointer to first summand
 *              - const polyvecl *v: pointer to second summand
 **************************************************/
void polyfixvecl_add(polyfixvecl *w, const polyfixvecl *u, const polyvecl *v) {
    unsigned int i;

    for (i = 0; i < L; ++i)
        polyfix_add(&w->vec[i], &u->vec[i], &v->vec[i]);
}

/*************************************************
 * Name:        polyfixfixvecl_sub
 *
 * Description: subtract vector to a vector of fixed polynomials of length l.
 *              No modular reduction is performed.
 *
 * Arguments:   - polyvecl *w: pointer to output vector
 *              - const polyfixvecl *u: pointer to first summand
 *              - const polyfixvecl *v: pointer to second summand
 **************************************************/
void polyfixfixvecl_sub(polyfixvecl *w, const polyfixvecl *u,
                        const polyfixvecl *v) {
    unsigned int i;

    for (i = 0; i < L; ++i)
        polyfixfix_sub(&w->vec[i], &u->vec[i], &v->vec[i]);
}

/*************************************************
 * Name:        polyfixvecl_double
 *
 * Description: Double vector of polynomials of length L.
 *
 * Arguments:   - polyveck *b: pointer to output vector
 *              - polyveck *a: pointer to input vector
 **************************************************/
void polyfixvecl_double(polyfixvecl *b, const polyfixvecl *a) {
    unsigned int i, j;

    for (i = 0; i < L; ++i) {
        for (j = 0; j < N; j += 8) {

            __m256i va = _mm256_loadu_si256((__m256i *)&a->vec[i].coeffs[j]);
            __m256i vb = _mm256_add_epi32(va, va);

            _mm256_storeu_si256((__m256i *)&b->vec[i].coeffs[j], vb);
        }
    }
}

/*************************************************
 * Name:        polyfixvecl_round
 *
 * Description: rounds a fixed polynomial vector of length L
 *
 * Arguments:   - polyvecl *a: output integer polynomial vector
 *              - polyfixvecl *b: input fixed polynomial vector
 **************************************************/
void polyfixvecl_round(polyvecl *a, const polyfixvecl *b) {
    unsigned i;

    for (i = 0; i < L; ++i)
        polyfix_round(&a->vec[i], &b->vec[i]);
}

/*************************************************
 * Name:        polyfixveclk_norm2
 *
 * Description: Calculates L2 norm of a fixed point polynomial vector with
 *length L + K The result is L2 norm * LN similar to the way polynomial is
 *usually stored
 *
 * Arguments:   - polyfixvecl *a: polynomial vector with length L to calculate
 *                norm
 *              - polyfixveck *a: polynomial vector with length K to calculate
 *                norm
 **************************************************/
uint64_t polyfixveclk_sqnorm2(const polyfixvecl *a, const polyfixveck *b) {
    unsigned int i, j;
    
    __m256i v_acc = _mm256_setzero_si256();

    for (i = 0; i < L; ++i) {
        for (j = 0; j < N; j += 8) {

            __m256i va = _mm256_loadu_si256((__m256i *)&a->vec[i].coeffs[j]);
            __m256i v_even_sq = _mm256_mul_epi32(va, va);
            __m256i va_odd = _mm256_shuffle_epi32(va, 0xF5); 
            __m256i v_odd_sq = _mm256_mul_epi32(va_odd, va_odd);

            v_acc = _mm256_add_epi64(v_acc, v_even_sq);
            v_acc = _mm256_add_epi64(v_acc, v_odd_sq);
        }
    }

    for (i = 0; i < K; ++i) {
        for (j = 0; j < N; j += 8) {

            __m256i vb = _mm256_loadu_si256((__m256i *)&b->vec[i].coeffs[j]);
            __m256i v_even_sq = _mm256_mul_epi32(vb, vb);
            __m256i vb_odd = _mm256_shuffle_epi32(vb, 0xF5); 
            __m256i v_odd_sq = _mm256_mul_epi32(vb_odd, vb_odd);

            v_acc = _mm256_add_epi64(v_acc, v_even_sq);
            v_acc = _mm256_add_epi64(v_acc, v_odd_sq);
        }
    }

    __m128i acc_high = _mm256_extracti128_si256(v_acc, 1);
    __m128i acc_low  = _mm256_castsi256_si128(v_acc);
    __m128i acc_128  = _mm_add_epi64(acc_low, acc_high);
    
    uint64_t sum0 = _mm_extract_epi64(acc_128, 0);
    uint64_t sum1 = _mm_extract_epi64(acc_128, 1);

    return sum0 + sum1;
}


static const union {
  __m256i vec[4];
  int64_t arr[4*4];
} mulrnd_avx = {.arr = {
  (1ULL<<29), (1ULL<<29), (1ULL<<29), (1ULL<<29),
  0,0,0,0,
  2,2,2,2,
  (1UL<<27), (1UL<<27), (1UL<<27), (1UL<<27)
}};

static void __mr14(__m256i tmp[6], __m256i smp[2], const __m256i isv[3], const uint64_t *samples)
{
  smp[0] = _mm256_loadu_si256((__m256i const *) samples);
  smp[1] = _mm256_srli_epi64(smp[0], 32);

  tmp[0] = _mm256_mul_epu32(smp[0], isv[0]); // sl * il  ( 16)
  tmp[1] = _mm256_mul_epu32(smp[1], isv[0]); // sh * il  ( 48)
  tmp[2] = _mm256_mul_epu32(smp[0], isv[1]); // sl * im  ( 46)
  tmp[3] = _mm256_mul_epu32(smp[1], isv[1]); // sh * im  ( 78)
  tmp[4] = _mm256_mul_epu32(smp[0], isv[2]); // sl * ih  ( 76)
  tmp[5] = _mm256_mul_epu32(smp[1], isv[2]); // sh * ih  (108)

  tmp[0] = _mm256_srli_epi64(tmp[0], 46);
  tmp[0] = _mm256_add_epi64(tmp[0], tmp[2]);
  tmp[0] = _mm256_add_epi64(tmp[0], mulrnd_avx.vec[2]); // rounding
  tmp[0] = _mm256_srli_epi64(tmp[0], 2);
  tmp[0] = _mm256_add_epi64(tmp[0], tmp[1]);
  tmp[0] = _mm256_add_epi64(tmp[0], mulrnd_avx.vec[3]); // rounding
  tmp[0] = _mm256_srli_epi64(tmp[0], 28);
  tmp[0] = _mm256_add_epi64(tmp[0], tmp[4]);
  tmp[0] = _mm256_srli_epi64(tmp[0], 2);
  tmp[0] = _mm256_add_epi64(tmp[0], tmp[3]);
  tmp[0] = _mm256_srli_epi64(tmp[0], 30);
  tmp[0] = _mm256_add_epi64(tmp[0], tmp[5]);
  tmp[0] = _mm256_add_epi64(tmp[0], mulrnd_avx.vec[0]); // rounding
  tmp[0] = _mm256_srli_epi64(tmp[0], 30);
}



static void _mul_rnd14(polyfix *y, const uint64_t samples[N], const uint8_t signs[N/8], const uint64_t invsqrt[2])
{
/*
for (j = 0; j < N; j++)
    y1->vec[i].coeffs[j] = fixpoint_mul_rnd(
        samples[(i * N + j)], &sqsum,
        (signs[(i * N + j) / 8] >> ((i * N + j) % 8)) & 1);

int32_t fixpoint_mul_rnd(const uint64_t x, const fp96_76 *y,
                         const uint8_t sign) {
    int64_t res;
    fp96_76 tmp, xx;
    xx.limb48[1] = x >> 32;
    xx.limb48[0] = (x & ((1ULL << 32) - 1)) << 16;
    fixpoint_mul(&tmp, &xx, y);
    res = (tmp.limb48[1] + (1UL << (13)) >> 14; // rounding
    return (1 - 2 * (int32_t)sign) * res;
} 
static void fixpoint_mul(fp96_76 *xy, const fp96_76 *x, const fp96_76 *y) {
    uint64_t tmp[2];
    mul48(&xy->limb48[0], x->limb48[0], y->limb48[0]);

    // shift right by 48, rounding
    xy->limb48[0] = xy->limb48[1] + (((xy->limb48[0] >> 47) + 1) >> 1);

    mul48(tmp, x->limb48[0], y->limb48[1]);
    xy->limb48[0] += tmp[0];
    xy->limb48[1] = tmp[1];
    mulacc48(&xy->limb48[0], x->limb48[1], y->limb48[0]);

    // shift right by 28, rounding
    xy->limb48[0] += 1UL << 27;
    xy->limb48[0] >>= 28;
    xy->limb48[0] += (xy->limb48[1] << 20) & ((1ULL << 48) - 1);
    xy->limb48[1] >>= 28;

    mul64(tmp, x->limb48[1], y->limb48[1]);
    xy->limb48[0] += (tmp[0] << 20) & ((1ULL << 48) - 1);
    xy->limb48[1] += (tmp[0] >> 28) + (tmp[1] << 36);

    renormalize(xy);
}      
*/
  size_t i;
  uint32_t is[3] = {invsqrt[0]&((1ULL<<30)-1),  ((invsqrt[0]>>30) | (invsqrt[1]<<(48-30)))&((1ULL<<30)-1),  (invsqrt[1]>>(60-48))&((1ULL<<30)-1)};
  __m256i isv[3], tmp[7], smp[2];
  isv[0] = _mm256_set1_epi64x(is[0]);
  isv[1] = _mm256_set1_epi64x(is[1]);
  isv[2] = _mm256_set1_epi64x(is[2]);

  for (i = 0; i < N/8; i++)
  {
    const union {
      __m256 vec;
      int32_t arr[8];
    } signs_avx = {.arr = {
      -(int32_t)(signs[i]&1), -(int32_t)((signs[i]>>1)&1),  -(int32_t)((signs[i]>>2)&1),  -(int32_t)((signs[i]>>3)&1), 
        -(int32_t)((signs[i]>>4)&1),  -(int32_t)((signs[i]>>5)&1),  -(int32_t)((signs[i]>>6)&1),  -(int32_t)((signs[i]>>7)&1)
    }};
    __mr14(tmp, smp, isv, &samples[i*8 + 0]);
    tmp[6] = tmp[0];
    __mr14(tmp, smp, isv, &samples[i*8 + 4]);

    // write to polynomial
    // we have tmp[6] 0d0c0b0a, tmp[0] 0h0g0f0e, but we want hgfedcba
    tmp[1] = _mm256_permute4x64_epi64(tmp[0], 0x44); // now tmp[0] holds 0h0g0f0e and tmp[1] has 0f0e0f0e
    tmp[2] = _mm256_permute4x64_epi64(tmp[6], 0xee); // now tmp[6] holds 0d0c0b0a and tmp[2] has 0d0c0d0c
    tmp[0] = _mm256_shuffle_epi32(tmp[0], 0x85); // now tmp[0] holds hg00fe00
    tmp[1] = _mm256_shuffle_epi32(tmp[1], 0x58); // now tmp[1] holds 00fe00fe
    tmp[2] = _mm256_shuffle_epi32(tmp[2], 0x85); // now tmp[2] holds dc00dc00
    tmp[6] = _mm256_shuffle_epi32(tmp[6], 0x58); // now tmp[6] holds 00dc00ba
    tmp[3] = _mm256_blend_epi32(tmp[6], tmp[2], 0xcc); // tmp[0] hols dcdcdcba
    tmp[1] = _mm256_blend_epi32(tmp[1], tmp[0], 0xcc); // tmp[1] hols hgfefefe
    tmp[3] = _mm256_blend_epi32(tmp[3], tmp[1], 0xf0); // hgfedcba
    tmp[4] = _mm256_sub_epi32(mulrnd_avx.vec[1], tmp[3]); // negate
    y->vec[i] = _mm256_castps_si256(_mm256_blendv_ps(_mm256_castsi256_ps(tmp[3]), _mm256_castsi256_ps(tmp[4]), signs_avx.vec));
  }
}

/*************************************************
 * Name:        polyfixveclk_sample_hyperball
 *
 * Description: Sample a vector (y1, y2) from a hyperball
 *
 * Arguments:   - polyfixvecl *y1: output polynomial vector of length L
 *              - polyfixveck *y2: output polynomial vector of length K
 *              - uint8_t *b: byte array for sampling
 *              - const uint8_t seed[CRHBYTES]: seed for sampling
 *              - const uint16_t nonce: nonce for sampling
 *
 * Returns:     - the number of used bytes from b
 **************************************************/
uint16_t polyfixveclk_sample_hyperball(polyfixvecl *y1, polyfixveck *y2, uint8_t *b,
                                       const uint8_t seed[CRHBYTES],
                                       const uint16_t nonce) {
    
    uint16_t ni = nonce;

    uint64_t samples[N * (L + K)];
    uint8_t signs[N * (L + K) / 8];


    fp96_76 sqsum, invsqrt;
    unsigned int i, j;
    

    do {
        sqsum.limb48[0] = 0;
        sqsum.limb48[1] = 0;

//        sample_gauss_N(&samples[0], &signs[0], &sqsum, seed, ni, N + 1);
        // sample_gauss_N(&samples[N], &signs[N / 8], &sqsum, seed, ni+1, N + 1);

        // for (i = 2; i < L + K; i++){
        //     sample_gauss_N(&samples[N * i], &signs[N / 8 * i], &sqsum, seed,
        //                    ni+i, N);
        // }

#if K == 1 && L == 2
        sample_gauss_N_4x(  &samples[0],   &samples[N], &samples[2*N], NULL,
                              &signs[0],   &signs[N/8], &signs[2*N/8], &signs[0],
                          &sqsum, seed, 
                                     ni,          ni+1,          ni+2,          ni,
                                    N+1,           N+1,             N,             0);


#elif K == 2 && L == 3
        sample_gauss_N_4x(  &samples[0],   &samples[N], &samples[2*N], &samples[3*N],
                              &signs[0],   &signs[N/8], &signs[2*N/8], &signs[3*N/8],
                          &sqsum, seed, 
                                     ni,          ni+1,          ni+2,          ni+3,
                                    N+1,           N+1,             N,             N);
        sample_gauss_N(&samples[4*N], &signs[4*N/8], &sqsum, seed, ni+4, N);
#endif

         ni += K + L;

        // divide sqsum by 2 and approximate inverse square root
        sqsum.limb48[0] += 1; // rounding
        sqsum.limb48[0] >>= 1;
        sqsum.limb48[0] += (sqsum.limb48[1] & 1) << 47;
        sqsum.limb48[1] >>= 1;
        sqsum.limb48[1] += sqsum.limb48[0] >> 48;
        sqsum.limb48[0] &= (1ULL << 48) - 1;
        fixpoint_newton_invsqrt(&invsqrt, &sqsum);
        fixpoint_mul_high(&sqsum, &invsqrt,
                          (uint64_t)(B * LN + SQNM / 2) << (28 - LNBITS));



        // for (i = 0; i < L; i++) {
        //     for (j = 0; j < N; j++)
        //         y1->vec[i].coeffs[j] = fixpoint_mul_rnd(
        //             samples[(i * N + j)], &sqsum,
        //             (signs[(i * N + j) / 8] >> ((i * N + j) % 8)) & 1);
        // }
        // for (i = L; i < K + L; i++) {
        //     for (j = 0; j < N; j++)
        //         y2->vec[i - L].coeffs[j] = fixpoint_mul_rnd(
        //             samples[(i * N + j)], &sqsum,
        //             (signs[(i * N + j) / 8] >> ((i * N + j) % 8)) & 1);
        // }


        for (i = 0; i < L; i++) {
          _mul_rnd14(&y1->vec[i], &samples[i*N], &signs[i*N/8], &sqsum.limb48[0]);
        }
        for (i = L; i < K + L; i++) {
          _mul_rnd14(&y2->vec[i-L], &samples[i*N], &signs[i*N/8], &sqsum.limb48[0]);
        }

        // for(j=0; j<16; j++){
        //     printf("%d, ", y1->vec[0].coeffs[j]);
        // }
        // printf("\n\n");



    } while(polyfixveclk_sqnorm2(y1, y2) > BSQ * LN * LN);

    {
      uint8_t tmp[CRHBYTES + 2];
      for (i = 0; i < CRHBYTES; i++)
      {
        tmp[i] = seed[i];
      }
      tmp[CRHBYTES + 0] = ni >> 0;
      tmp[CRHBYTES + 1] = ni >> 8;
      shake256(b, 1, tmp, CRHBYTES+2);
    }

    return ni;
}