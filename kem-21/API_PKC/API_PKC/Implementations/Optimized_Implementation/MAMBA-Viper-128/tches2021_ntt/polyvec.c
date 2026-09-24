#include <stdint.h>
#include <assert.h>
#include <immintrin.h>
#include "params.h"
#include "poly.h"
#include "polyvec.h"
#include "consts.h"
#if defined(VIPER_TCHES_NTT_STAGE_PROFILE) && (VIPER_TCHES_NTT_STAGE_PROFILE == 1)
#include <stdio.h>
#include <x86intrin.h>
#define NTT_STAGE_T0(name) uint64_t name = __rdtsc()
#define NTT_STAGE_PRINT(op, stage, start) \
  fprintf(stderr, "NTT_STAGE,%s,%s,%llu\n", (op), (stage), (unsigned long long)(__rdtsc() - (start)))
#else
#define NTT_STAGE_T0(name) (void)0
#define NTT_STAGE_PRINT(op, stage, start) (void)0
#endif

#if KEM_K > 9
static inline __m256i centered_reduce_i32x8(__m256i x, int16_t p) {
  const __m256i vp = _mm256_set1_epi32(p);
  const __m256i vhalf = _mm256_set1_epi32(p / 2);
  const __m256i vneghalf = _mm256_set1_epi32(-(p / 2));

  /*
   * Each poly_basemul_montgomery output coefficient is already centered
   * modulo p.  For Viper's K=7/9 paths the accumulated bound is less than
   * five moduli in magnitude, so five fixed branch-free correction rounds
   * are sufficient and keep this helper independent of secret data.
   */
  for (unsigned int i = 0; i < 5; i++) {
    __m256i gt = _mm256_cmpgt_epi32(x, vhalf);
    __m256i lt = _mm256_cmpgt_epi32(vneghalf, x);
    x = _mm256_sub_epi32(x, _mm256_and_si256(gt, vp));
    x = _mm256_add_epi32(x, _mm256_and_si256(lt, vp));
  }
  return x;
}

static void viper_polyvec_basemul_acc_montgomery_k_gt4(nttpoly *r, const nttpolyvec *a, const nttpolyvec *b, const int16_t *pdata) {
  nttpoly tmp;
  __attribute__((aligned(32))) int32_t acc[NTT_N];
  const int16_t p = pdata[_16XP];

  for (unsigned int l = 0; l < NTT_N; l += 16) {
    _mm256_store_si256((__m256i *)&acc[l + 0], _mm256_setzero_si256());
    _mm256_store_si256((__m256i *)&acc[l + 8], _mm256_setzero_si256());
  }

  for (unsigned int j = 0; j < KEM_K; j++) {
    poly_basemul_montgomery(&tmp, &a->vec[j], &b->vec[j], pdata);
    for (unsigned int l = 0; l < NTT_N; l += 16) {
      __m256i t = _mm256_load_si256((const __m256i *)&tmp.coeffs[l]);
      __m256i acc0 = _mm256_load_si256((const __m256i *)&acc[l + 0]);
      __m256i acc1 = _mm256_load_si256((const __m256i *)&acc[l + 8]);
      acc0 = _mm256_add_epi32(acc0, _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t)));
      acc1 = _mm256_add_epi32(acc1, _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t, 1)));
      _mm256_store_si256((__m256i *)&acc[l + 0], acc0);
      _mm256_store_si256((__m256i *)&acc[l + 8], acc1);
    }
  }

  for (unsigned int l = 0; l < NTT_N; l += 16) {
    __m256i acc0 = _mm256_load_si256((const __m256i *)&acc[l + 0]);
    __m256i acc1 = _mm256_load_si256((const __m256i *)&acc[l + 8]);
    acc0 = centered_reduce_i32x8(acc0, p);
    acc1 = centered_reduce_i32x8(acc1, p);
    __m256i packed = _mm256_packs_epi32(acc0, acc1);
    packed = _mm256_permute4x64_epi64(packed, 0xD8);
    _mm256_store_si256((__m256i *)&r->coeffs[l], packed);
  }
}
#endif

static void viper_polyvec_basemul_acc_montgomery(nttpoly *r, const nttpolyvec *a, const nttpolyvec *b, const int16_t *pdata) {
#if KEM_K <= 9
  /*
   * The imported AVX2 assembly accumulator is generated for the active
   * compile-time KEM_K. It covers Viper's K=2/3/4/7/9 profiles; keep the
   * intrinsic fallback below only for unsupported future K values.
   */
  polyvec_basemul_acc_montgomery(r, a, b, pdata);
#else
  viper_polyvec_basemul_acc_montgomery_k_gt4(r, a, b, pdata);
#endif
}

void polyvec_uniform(polyvec *r, const uint8_t seed[POLYMUL_SYMBYTES], uint16_t nonce) {
  unsigned int i;
  for(i=0;i<KEM_K;i++)
    poly_uniform(&r->vec[i], seed, (nonce << 8) + i);
}

void polyvec_noise(polyvec *r, const uint8_t seed[POLYMUL_SYMBYTES], uint16_t nonce) {
  unsigned int i;
  for(i=0;i<KEM_K;i++)
    poly_noise(&r->vec[i], seed, (nonce << 8) + i);
}

void polyvec_ntt(nttpolyvec *r, const polyvec *a, const int16_t *pdata) {
  unsigned int i;
  for(i=0;i<KEM_K;i++)
    poly_ntt(&r->vec[i], &a->vec[i], pdata);
}

void polyvec_invntt_tomont(nttpolyvec *r, const nttpolyvec *a, const int16_t *pdata) {
  unsigned int i;
  for(i=0;i<KEM_K;i++)
    poly_invntt_tomont(&r->vec[i], &a->vec[i], pdata);
}

void polyvec_crt(polyvec *r, const nttpolyvec *a, const nttpolyvec *b) {
  unsigned int i;
  for(i=0;i<KEM_K;i++)
    poly_crt(&r->vec[i], &a->vec[i], &b->vec[i]);
}

void polyvec_matrix_vector_mul(polyvec *t, const polyvec a[KEM_K], const polyvec *s, int transpose) {
  unsigned int i, j;
  nttpolyvec shat, ahat, t0, t1;
  const char *op = transpose ? "matTvec_core" : "matvec_core";
  (void)op;

  NTT_STAGE_T0(p0_s_ntt);
  polyvec_ntt(&shat,s,PDATA0);
  NTT_STAGE_PRINT(op, "p0_short_forward_ntt", p0_s_ntt);
  for(i=0;i<KEM_K;i++) {
    NTT_STAGE_T0(p0_a_ntt);
    for(j=0;j<KEM_K;j++) {
      if(transpose)
        poly_ntt(&ahat.vec[j],&a[j].vec[i],PDATA0);
      else
        poly_ntt(&ahat.vec[j],&a[i].vec[j],PDATA0);
    }
    NTT_STAGE_PRINT(op, "p0_dense_row_forward_ntt", p0_a_ntt);
    NTT_STAGE_T0(p0_mul);
    viper_polyvec_basemul_acc_montgomery(&t0.vec[i],&ahat,&shat,PDATA0);
    NTT_STAGE_PRINT(op, "p0_basemul_acc", p0_mul);
  }

  NTT_STAGE_T0(p1_s_ntt);
  polyvec_ntt(&shat,s,PDATA1);
  NTT_STAGE_PRINT(op, "p1_short_forward_ntt", p1_s_ntt);
  for(i=0;i<KEM_K;i++) {
    NTT_STAGE_T0(p1_a_ntt);
    for(j=0;j<KEM_K;j++) {
      if(transpose)
        poly_ntt(&ahat.vec[j],&a[j].vec[i],PDATA1);
      else
        poly_ntt(&ahat.vec[j],&a[i].vec[j],PDATA1);
    }
    NTT_STAGE_PRINT(op, "p1_dense_row_forward_ntt", p1_a_ntt);
    NTT_STAGE_T0(p1_mul);
    viper_polyvec_basemul_acc_montgomery(&t1.vec[i],&ahat,&shat,PDATA1);
    NTT_STAGE_PRINT(op, "p1_basemul_acc", p1_mul);
  }

  NTT_STAGE_T0(inv0);
  polyvec_invntt_tomont(&t0,&t0,PDATA0);
  NTT_STAGE_PRINT(op, "p0_inverse_ntt", inv0);
  NTT_STAGE_T0(inv1);
  polyvec_invntt_tomont(&t1,&t1,PDATA1);
  NTT_STAGE_PRINT(op, "p1_inverse_ntt", inv1);
  NTT_STAGE_T0(crt);
  polyvec_crt(t,&t0,&t1);
  NTT_STAGE_PRINT(op, "crt_reconstruct", crt);
}

void polyvec_iprod(poly *r, const polyvec *a, const polyvec *b) {
  nttpoly r0, r1;
  nttpolyvec ahat;
  nttpolyvec bhat;

  NTT_STAGE_T0(p0_a);
  polyvec_ntt(&ahat,a,PDATA0);
  NTT_STAGE_PRINT("dot_core", "p0_a_forward_ntt", p0_a);
  NTT_STAGE_T0(p0_b);
  polyvec_ntt(&bhat,b,PDATA0);
  NTT_STAGE_PRINT("dot_core", "p0_b_forward_ntt", p0_b);
  NTT_STAGE_T0(p0_mul);
  viper_polyvec_basemul_acc_montgomery(&r0,&ahat,&bhat,PDATA0);
  NTT_STAGE_PRINT("dot_core", "p0_basemul_acc", p0_mul);

  NTT_STAGE_T0(p1_a);
  polyvec_ntt(&ahat,a,PDATA1);
  NTT_STAGE_PRINT("dot_core", "p1_a_forward_ntt", p1_a);
  NTT_STAGE_T0(p1_b);
  polyvec_ntt(&bhat,b,PDATA1);
  NTT_STAGE_PRINT("dot_core", "p1_b_forward_ntt", p1_b);
  NTT_STAGE_T0(p1_mul);
  viper_polyvec_basemul_acc_montgomery(&r1,&ahat,&bhat,PDATA1);
  NTT_STAGE_PRINT("dot_core", "p1_basemul_acc", p1_mul);

  NTT_STAGE_T0(inv0);
  poly_invntt_tomont(&r0,&r0,PDATA0);
  NTT_STAGE_PRINT("dot_core", "p0_inverse_ntt", inv0);
  NTT_STAGE_T0(inv1);
  poly_invntt_tomont(&r1,&r1,PDATA1);
  NTT_STAGE_PRINT("dot_core", "p1_inverse_ntt", inv1);
  NTT_STAGE_T0(crt);
  poly_crt(r,&r0,&r1);
  NTT_STAGE_PRINT("dot_core", "crt_reconstruct", crt);
}
