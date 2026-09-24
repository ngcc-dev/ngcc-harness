#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "cpucycles.h"
#include "speed_print.h"
#include "drng.h"
#include "parameters.h"
#include "fprime.h"
#include "poly.h"
#include "ring.h"
#include "uniform.h"
#include "pke.h"
#include "kem.h"

#define NUMBER_OF_TESTS 1000
uint64_t t[NUMBER_OF_TESTS];

#define RNG_SEED_LENGTH 32
DRNG_ctx drng_algorithm;

extern int32_t rrlwr_pke_zetas[RRLWR_N];

static int cmp_uint64_local(const void *a, const void *b)
{
  uint64_t av = *(const uint64_t *)a;
  uint64_t bv = *(const uint64_t *)b;

  if (av < bv) return -1;
  if (av > bv) return 1;
  return 0;
}

static uint64_t average_local(const uint64_t *x, size_t n)
{
  uint64_t acc = 0;

  for (size_t i = 0; i < n; ++i) {
    acc += x[i];
  }

  return acc / n;
}

static uint64_t median_local(const uint64_t *x, size_t n)
{
  uint64_t tmp[NUMBER_OF_TESTS];

  memcpy(tmp, x, n * sizeof(tmp[0]));
  qsort(tmp, n, sizeof(tmp[0]), cmp_uint64_local);

  if (n & 1) {
    return tmp[n / 2];
  }

  return (tmp[n / 2 - 1] + tmp[n / 2]) / 2;
}

static void print_plain_stage_results(const char *label,
                                      const uint64_t *stage_cycles,
                                      size_t n)
{
  printf("%s\n", label);
  printf("median: %llu cycles/ticks\n",
         (unsigned long long)median_local(stage_cycles, n));
  printf("average: %llu cycles/ticks\n",
         (unsigned long long)average_local(stage_cycles, n));
  printf("\n");
}

#define MEASURE_PLAIN_STAGE(label, statement) do { \
  for(unsigned int i = 0; i < NUMBER_OF_TESTS; i++) { \
    uint64_t start = cpucycles(); \
    statement; \
    t[i] = cpucycles() - start - overhead; \
  } \
  print_plain_stage_results(label, t, NUMBER_OF_TESTS); \
} while(0)

#ifndef RRLWR_DISABLE_NTT_AVX
void ring_mul_Awin_row_avx(poly *r,
                           const poly *row,
                           const ring_element *b,
                           int k,
                           int32_t prime,
                           int32_t primeinv);
void ring_mul_Awin_row_k5_avx(poly *r,
                              const poly *row,
                              const ring_element *b,
                              int k,
                              int32_t prime,
                              int32_t primeinv);
void ring_mul_Awin_row_k9_avx(poly *r,
                              const poly *row,
                              const ring_element *b,
                              int k,
                              int32_t prime,
                              int32_t primeinv);
void ring_mul_Awin_row_k17_avx(poly *r,
                               const poly *row,
                               const ring_element *b,
                               int k,
                               int32_t prime,
                               int32_t primeinv);
void ring_mul_Awin_2rows_rev_k5_avx(poly *r,
                                    const poly *row,
                                    const ring_element *b,
                                    int32_t prime,
                                    int32_t primeinv);
void ring_mul_Awin_2rows_rev_k9_avx(poly *r,
                                    const poly *row,
                                    const ring_element *b,
                                    int32_t prime,
                                    int32_t primeinv);
void ring_mul_Awin_2rows_rev_k17_avx(poly *r,
                                     const poly *row,
                                     const ring_element *b,
                                     int32_t prime,
                                     int32_t primeinv);
void ring_mul_Awin_4rows_rev_k5_avx(poly *r,
                                    const poly *row,
                                    const ring_element *b,
                                    int32_t prime,
                                    int32_t primeinv);
void ring_mul_Awin_4rows_rev_k9_avx(poly *r,
                                    const poly *row,
                                    const ring_element *b,
                                    int32_t prime,
                                    int32_t primeinv);
void ring_mul_Awin_4rows_rev_k17_avx(poly *r,
                                     const poly *row,
                                     const ring_element *b,
                                     int32_t prime,
                                     int32_t primeinv);
void ring_mul_Awin_5rows_rev_k5_avx(poly *r,
                                    const poly *row,
                                    const ring_element *b,
                                    int32_t prime,
                                    int32_t primeinv);
void ring_mul_Awin_5rows_rev_k9_avx(poly *r,
                                    const poly *row,
                                    const ring_element *b,
                                    int32_t prime,
                                    int32_t primeinv);
void ring_mul_Awin_5rows_rev_k17_avx(poly *r,
                                     const poly *row,
                                     const ring_element *b,
                                     int32_t prime,
                                     int32_t primeinv);
#endif

static void speed_ring_mul_Awin_ntt_dot32_kernel(poly *r,
                                                 const ring_element_Awin *a,
                                                 ring_element *b,
                                                 int ncoeffs,
                                                 int32_t prime,
                                                 int32_t primeinv)
{
#ifndef RRLWR_DISABLE_NTT_AVX
#if RRLWR_K == 5
  int out = 0;
  for(; out + 4 < ncoeffs; out += 5) {
    ring_mul_Awin_5rows_rev_k5_avx(&r[out], &a->x[ncoeffs - 5 - out],
                                   b, prime, primeinv);
  }
  for(; out + 3 < ncoeffs; out += 4) {
    ring_mul_Awin_4rows_rev_k5_avx(&r[out], &a->x[ncoeffs - 4 - out],
                                   b, prime, primeinv);
  }
  for(; out + 1 < ncoeffs; out += 2) {
    ring_mul_Awin_2rows_rev_k5_avx(&r[out], &a->x[ncoeffs - 2 - out],
                                   b, prime, primeinv);
  }
  if(out < ncoeffs) {
    ring_mul_Awin_row_k5_avx(&r[out], &a->x[0], b, RRLWR_K, prime, primeinv);
  }
#elif RRLWR_K == 9
  int out = 0;
  for(; out + 4 < ncoeffs; out += 5) {
    ring_mul_Awin_5rows_rev_k9_avx(&r[out], &a->x[ncoeffs - 5 - out],
                                   b, prime, primeinv);
  }
  for(; out + 3 < ncoeffs; out += 4) {
    ring_mul_Awin_4rows_rev_k9_avx(&r[out], &a->x[ncoeffs - 4 - out],
                                   b, prime, primeinv);
  }
  for(; out + 1 < ncoeffs; out += 2) {
    ring_mul_Awin_2rows_rev_k9_avx(&r[out], &a->x[ncoeffs - 2 - out],
                                   b, prime, primeinv);
  }
  if(out < ncoeffs) {
    ring_mul_Awin_row_k9_avx(&r[out], &a->x[0], b, RRLWR_K, prime, primeinv);
  }
#elif RRLWR_K == 17
  int out = 0;
  for(; out + 4 < ncoeffs; out += 5) {
    ring_mul_Awin_5rows_rev_k17_avx(&r[out], &a->x[ncoeffs - 5 - out],
                                    b, prime, primeinv);
  }
  for(; out + 3 < ncoeffs; out += 4) {
    ring_mul_Awin_4rows_rev_k17_avx(&r[out], &a->x[ncoeffs - 4 - out],
                                    b, prime, primeinv);
  }
  for(; out + 1 < ncoeffs; out += 2) {
    ring_mul_Awin_2rows_rev_k17_avx(&r[out], &a->x[ncoeffs - 2 - out],
                                    b, prime, primeinv);
  }
  if(out < ncoeffs) {
    ring_mul_Awin_row_k17_avx(&r[out], &a->x[0], b, RRLWR_K, prime, primeinv);
  }
#else
  for(int out = 0; out < ncoeffs; out++) {
    const poly *row = &a->x[ncoeffs - 1 - out];

    ring_mul_Awin_row_avx(&r[out], row, b, RRLWR_K, prime, primeinv);
  }
#endif
#else
  poly tmp;
  int row_min = RRLWR_K - ncoeffs;

  for(int i = RRLWR_K - 1; i >= row_min; i--) {
    int out = i - row_min;
    const poly *row = &a->x[RRLWR_K - 1 - i];

    for(unsigned int j = 0; j < RRLWR_N; j++) {
      r[out].coeffs[j] = 0;
    }

    for(int j = 0; j < RRLWR_K; j++) {
      poly_basemul32(&tmp, (poly *)&row[j], &b->x[j], prime, primeinv);
      poly_add32(&r[out], &r[out], &tmp, prime);
    }
  }
#endif
}

int main() {

  poly f __attribute__((aligned(32)));
  poly g __attribute__((aligned(32)));
  poly h __attribute__((aligned(32)));
  ring_element r __attribute__((aligned(32)));
  ring_element a __attribute__((aligned(32)));
  ring_element s __attribute__((aligned(32)));
  ring_element s_ntt __attribute__((aligned(32)));
  ring_element s_work __attribute__((aligned(32)));
  ring_element_Awin aw __attribute__((aligned(32)));
  ring_element_Awin bp_aw __attribute__((aligned(32)));
  poly vp[RRLWR_PKE_ELL] __attribute__((aligned(32)));
  unsigned char seedA[RRLWR_PKE_SEED_A_LEN];
  unsigned char seedS[RRLWR_SEED_S_LEN];
  unsigned char seedSp[RRLWR_SEED_S_LEN];
  unsigned char sk[RRLWR_PKE_SK_LEN];
  unsigned char pk[RRLWR_PKE_PK_LEN];
  unsigned char ct[RRLWR_PKE_CT_LEN];
  unsigned char m[RRLWR_PKE_MESSAGE_LEN];
  unsigned char mp[RRLWR_PKE_MESSAGE_LEN];
  unsigned char kem_sk[RRLWR_KEM_SK_LEN];
  unsigned char kem_pk[RRLWR_KEM_PK_LEN];
  unsigned char kem_ct[RRLWR_KEM_CT_LEN];
  unsigned char ss[RRLWR_KEM_SS_LEN];
  unsigned long long length;

  /* Initialize RNG*/
  const unsigned char seed[RNG_SEED_LENGTH] = {0};
  init_random_number(&drng_algorithm, seed, RNG_SEED_LENGTH);
  uint64_t overhead = cpucycles_overhead();

  GENERATE_RANDOM_BYTES(seedA, RRLWR_PKE_SEED_A_LEN, &drng_algorithm);
  GENERATE_RANDOM_BYTES(seedS, RRLWR_SEED_S_LEN, &drng_algorithm);
  GENERATE_RANDOM_BYTES(seedSp, RRLWR_SEED_S_LEN, &drng_algorithm);
  GENERATE_RANDOM_BYTES(m, RRLWR_PKE_MESSAGE_LEN, &drng_algorithm);

  MEASURE_PLAIN_STAGE("sample_Awin(A): ",
    ring_uniform_Awin(&aw, RRLWR_PKE_LOGQ, seedA, RRLWR_PKE_SEED_A_LEN,
                      RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                      RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME,
                      rrlwr_pke_zetas));

  MEASURE_PLAIN_STAGE("ring_uniform_Awin_base(A): ",
    ring_uniform_Awin_base(&aw, RRLWR_PKE_LOGQ, seedA,
                           RRLWR_PKE_SEED_A_LEN));

  MEASURE_PLAIN_STAGE("sample_secret(s): ",
    ring_uniform(&s, RRLWR_PKE_LOG_ETA+1, seedS, RRLWR_SEED_S_LEN));

  MEASURE_PLAIN_STAGE("poly_ntt32: ",
    poly_ntt32(&f, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV, rrlwr_pke_zetas));

  MEASURE_PLAIN_STAGE("poly_invntt32: ",
    poly_invntt32(&f, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                  RRLWR_NTTINV_FINALCONST, rrlwr_pke_zetas));

  MEASURE_PLAIN_STAGE("poly_basemul32: ",
    poly_basemul32(&h, &f, &g, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV));

#ifndef RRLWR_DISABLE_NTT_AVX
  MEASURE_PLAIN_STAGE("poly_basemul32_avx(Awin prepare): ",
    poly_basemul32_avx(&h, &f, &g, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV));

  MEASURE_PLAIN_STAGE("poly_basemul_add32(accumulate): ",
    poly_basemul_add32(&h, &f, &g, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV));
#endif

  MEASURE_PLAIN_STAGE("poly_add32: ",
    poly_add32(&h, &f, &g, RRLWR_PKE_PRIME));

  MEASURE_PLAIN_STAGE("poly_add: ",
    poly_add(&h, &f, &g));

  MEASURE_PLAIN_STAGE("poly_sub32: ",
    poly_sub32(&h, &f, &g, RRLWR_PKE_PRIME));

  MEASURE_PLAIN_STAGE("ring_ntt32(s): ",
    ring_ntt32(&s_ntt, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV, rrlwr_pke_zetas));

  MEASURE_PLAIN_STAGE("ring_to_Awin_ncoeffs(ell): ",
    ring_to_Awin_ncoeffs(&bp_aw, &a, RRLWR_PKE_ELL,
                         RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                         RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME,
                         rrlwr_pke_zetas));

  MEASURE_PLAIN_STAGE("ring_unpack_Awin_ncoeffs(ell): ",
    ring_unpack_Awin_ncoeffs(&bp_aw, pk + RRLWR_PKE_SEED_A_LEN,
                             RRLWR_PKE_LOGP, RRLWR_PKE_ELL,
                             RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                             RRLWR_KEM_RMODPRIME, RRLWR_KEM_2RMODPRIME,
                             rrlwr_pke_zetas));

  s_ntt = s;
  ring_ntt32(&s_ntt, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV, rrlwr_pke_zetas);

  MEASURE_PLAIN_STAGE("ring_Awin_dot32(full, ntt-domain): ",
    speed_ring_mul_Awin_ntt_dot32_kernel(r.x, &aw, &s_ntt, RRLWR_K,
                                         RRLWR_PKE_PRIME,
                                         RRLWR_PKE_PRIMEINV));

  MEASURE_PLAIN_STAGE("ring_Awin_dot32(ell, ntt-domain): ",
    speed_ring_mul_Awin_ntt_dot32_kernel(vp, &bp_aw, &s_ntt, RRLWR_PKE_ELL,
                                         RRLWR_PKE_PRIME,
                                         RRLWR_PKE_PRIMEINV));

  MEASURE_PLAIN_STAGE("ring_Awin_invntt_round_xtoy_32(full): ",
    ring_mul_Awin_invntt_round_xtoy_32(r.x, &aw, &s_ntt, RRLWR_K,
                                       RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                                       RRLWR_NTTINV_FINALCONST,
                                       RRLWR_PKE_LOGQ, RRLWR_PKE_LOGP,
                                       rrlwr_pke_zetas));

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    s_work = s;
    uint64_t start = cpucycles();
    ring_mul_Awin_round_xtoy_32(r.x, &aw, &s_work, RRLWR_K,
                                RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                                RRLWR_NTTINV_FINALCONST,
                                RRLWR_PKE_LOGQ, RRLWR_PKE_LOGP,
                                rrlwr_pke_zetas);
    t[i] = cpucycles() - start - overhead;
  }
  print_plain_stage_results("ring_Awin_round_xtoy_32(full): ", t, NUMBER_OF_TESTS);

  MEASURE_PLAIN_STAGE("ring_Awin_invntt_reduce_pow2_32(ell): ",
    ring_mul_Awin_invntt_reduce_pow2_32(vp, &bp_aw, &s_ntt, RRLWR_PKE_ELL,
                                        RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                                        RRLWR_NTTINV_FINALCONST,
                                        RRLWR_PKE_LOGP,
                                        rrlwr_pke_zetas));

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    s_work = s;
    uint64_t start = cpucycles();
    ring_mul_Awin_reduce_pow2_32(vp, &bp_aw, &s_work, RRLWR_PKE_ELL,
                                 RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV,
                                 RRLWR_NTTINV_FINALCONST, RRLWR_PKE_LOGP,
                                 rrlwr_pke_zetas);
    t[i] = cpucycles() - start - overhead;
  }
  print_plain_stage_results("ring_Awin_reduce_pow2_32(ell): ", t, NUMBER_OF_TESTS);

  MEASURE_PLAIN_STAGE("pke_keygen: ",
    pke_keygen(pk, sk, seedA, seedS));

  MEASURE_PLAIN_STAGE("pke_encrypt: ",
    pke_encrypt(ct, pk, m, seedSp));

  MEASURE_PLAIN_STAGE("pke_decrypt: ",
    pke_decrypt(mp, ct, sk));

  MEASURE_PLAIN_STAGE("kem_keygen: ",
    kem_keygen(kem_pk, &length, kem_sk, &length));

  MEASURE_PLAIN_STAGE("kem_encaps: ",
    kem_enc(kem_pk, RRLWR_KEM_PK_LEN, ss, &length, kem_ct, &length));

  MEASURE_PLAIN_STAGE("kem_decaps: ",
    kem_dec(kem_sk, RRLWR_KEM_SK_LEN, kem_ct, RRLWR_KEM_CT_LEN, ss, &length));
}
