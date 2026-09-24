/*
 * Standalone benchmark for the fused Awin row-dot kernel in arith/pointwise.S.
 *
 * Build from the repository root, without using the Makefile:
 *
 *   gcc -O3 -mavx2 -I. -I./arith -I./utils \
 *     -DRRLWR_SECURITY_LEVEL=128 \
 *     test/bench_awin_row_dot.c arith/pointwise.S \
 *     -o test/bench_awin_row_dot_128
 *
 * Repeat with RRLWR_SECURITY_LEVEL=256 or 512 to measure K=9 or K=17.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parameters.h"
#include "ring.h"

#define SAMPLES 20000

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

static uint64_t g_rng = 0x3141592653589793ULL;

static inline uint64_t cpucycles_local(void)
{
  uint64_t result;

  __asm__ volatile ("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax"
    : "=a" (result) : : "%rdx");

  return result;
}

static uint64_t cpucycles_overhead_local(void)
{
  uint64_t overhead = UINT64_MAX;

  for(unsigned int i = 0; i < 100000; i++) {
    uint64_t t0 = cpucycles_local();
    __asm__ volatile ("");
    uint64_t t1 = cpucycles_local();

    if(t1 - t0 < overhead) {
      overhead = t1 - t0;
    }
  }

  return overhead;
}

static uint32_t rand32_local(void)
{
  uint64_t x = g_rng;

  x ^= x << 7;
  x ^= x >> 9;
  x ^= x << 8;
  g_rng = x;

  return (uint32_t)x;
}

static void fill_mod_prime(poly *r)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = (int32_t)(rand32_local() % (2U * RRLWR_PKE_PRIME + 1U)) -
                   RRLWR_PKE_PRIME;
  }
}

static int poly_equal(const poly *a, const poly *b)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    if(a->coeffs[i] != b->coeffs[i]) {
      fprintf(stderr, "mismatch at coeff %u: %d != %d\n",
              i, a->coeffs[i], b->coeffs[i]);
      return 0;
    }
  }

  return 1;
}

static int cmp_uint64_local(const void *a, const void *b)
{
  uint64_t av = *(const uint64_t *)a;
  uint64_t bv = *(const uint64_t *)b;

  if(av < bv) return -1;
  if(av > bv) return 1;
  return 0;
}

static uint64_t median_local(uint64_t *x)
{
  qsort(x, SAMPLES, sizeof(x[0]), cmp_uint64_local);

  return (x[SAMPLES / 2 - 1] + x[SAMPLES / 2]) / 2;
}

static void old_row_dot(poly *r, const poly *row, const ring_element *b)
{
  memset(r, 0, sizeof(*r));

  for(int j = 0; j < RRLWR_K; j++) {
    poly_basemul_add32(r, &row[j], &b->x[j],
                       RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
  }
}

static void new_row_dot(poly *r, const poly *row, const ring_element *b)
{
#if RRLWR_K == 5
  ring_mul_Awin_row_k5_avx(r, row, b, RRLWR_K,
                           RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
#elif RRLWR_K == 9
  ring_mul_Awin_row_k9_avx(r, row, b, RRLWR_K,
                           RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
#elif RRLWR_K == 17
  ring_mul_Awin_row_k17_avx(r, row, b, RRLWR_K,
                            RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
#else
  ring_mul_Awin_row_avx(r, row, b, RRLWR_K,
                        RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
#endif
}

static void old_matrix_dot(poly r[RRLWR_K],
                           const ring_element_Awin *aw,
                           const ring_element *b)
{
  for(int out = 0; out < RRLWR_K; out++) {
    old_row_dot(&r[out], &aw->x[RRLWR_K - 1 - out], b);
  }
}

static void new_matrix_dot(poly r[RRLWR_K],
                           const ring_element_Awin *aw,
                           const ring_element *b)
{
#if RRLWR_K == 5
  int out = 0;
  for(; out + 4 < RRLWR_K; out += 5) {
    ring_mul_Awin_5rows_rev_k5_avx(&r[out], &aw->x[RRLWR_K - 5 - out],
                                   b, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
  }
  for(; out + 3 < RRLWR_K; out += 4) {
    ring_mul_Awin_4rows_rev_k5_avx(&r[out], &aw->x[RRLWR_K - 4 - out],
                                   b, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
  }
  for(; out + 1 < RRLWR_K; out += 2) {
    ring_mul_Awin_2rows_rev_k5_avx(&r[out], &aw->x[RRLWR_K - 2 - out],
                                   b, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
  }
  if(out < RRLWR_K) {
    ring_mul_Awin_row_k5_avx(&r[out], &aw->x[0], b, RRLWR_K,
                             RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
  }
#elif RRLWR_K == 9
  int out = 0;
  for(; out + 4 < RRLWR_K; out += 5) {
    ring_mul_Awin_5rows_rev_k9_avx(&r[out], &aw->x[RRLWR_K - 5 - out],
                                   b, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
  }
  for(; out + 3 < RRLWR_K; out += 4) {
    ring_mul_Awin_4rows_rev_k9_avx(&r[out], &aw->x[RRLWR_K - 4 - out],
                                   b, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
  }
  for(; out + 1 < RRLWR_K; out += 2) {
    ring_mul_Awin_2rows_rev_k9_avx(&r[out], &aw->x[RRLWR_K - 2 - out],
                                   b, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
  }
  if(out < RRLWR_K) {
    ring_mul_Awin_row_k9_avx(&r[out], &aw->x[0], b, RRLWR_K,
                             RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
  }
#elif RRLWR_K == 17
  int out = 0;
  for(; out + 4 < RRLWR_K; out += 5) {
    ring_mul_Awin_5rows_rev_k17_avx(&r[out], &aw->x[RRLWR_K - 5 - out],
                                    b, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
  }
  for(; out + 3 < RRLWR_K; out += 4) {
    ring_mul_Awin_4rows_rev_k17_avx(&r[out], &aw->x[RRLWR_K - 4 - out],
                                    b, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
  }
  for(; out + 1 < RRLWR_K; out += 2) {
    ring_mul_Awin_2rows_rev_k17_avx(&r[out], &aw->x[RRLWR_K - 2 - out],
                                    b, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
  }
  if(out < RRLWR_K) {
    ring_mul_Awin_row_k17_avx(&r[out], &aw->x[0], b, RRLWR_K,
                              RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
  }
#else
  for(int out = 0; out < RRLWR_K; out++) {
    new_row_dot(&r[out], &aw->x[RRLWR_K - 1 - out], b);
  }
#endif
}

int main(void)
{
  ring_element_Awin aw;
  ring_element b;
  poly old_r;
  poly new_r;
  poly old_mat[RRLWR_K];
  poly new_mat[RRLWR_K];
  uint64_t old_samples[SAMPLES];
  uint64_t new_samples[SAMPLES];
  uint64_t overhead = cpucycles_overhead_local();

  for(unsigned int i = 0; i < 2 * RRLWR_K - 1; i++) {
    fill_mod_prime(&aw.x[i]);
  }

  for(unsigned int i = 0; i < RRLWR_K; i++) {
    fill_mod_prime(&b.x[i]);
  }

  old_row_dot(&old_r, aw.x, &b);
  new_row_dot(&new_r, aw.x, &b);

  if(!poly_equal(&old_r, &new_r)) {
    fprintf(stderr, "ring_mul_Awin_row_avx correctness failed for K=%d\n", RRLWR_K);
    return 1;
  }

  for(unsigned int i = 0; i < SAMPLES; i++) {
    uint64_t start = cpucycles_local();
    old_row_dot(&old_r, aw.x, &b);
    old_samples[i] = cpucycles_local() - start - overhead;

    start = cpucycles_local();
    new_row_dot(&new_r, aw.x, &b);
    new_samples[i] = cpucycles_local() - start - overhead;
  }

  uint64_t old_med = median_local(old_samples);
  uint64_t new_med = median_local(new_samples);

  printf("Awin row-dot K=%d, samples=%u, rdtsc overhead=%llu\n",
         RRLWR_K, SAMPLES, (unsigned long long)overhead);
  printf("old K*poly_basemul_add32 median: %llu cycles/ticks\n",
         (unsigned long long)old_med);
  printf("fused row kernel median:        %llu cycles/ticks\n",
         (unsigned long long)new_med);
  printf("speedup: %.2fx\n", new_med == 0 ? 0.0 : (double)old_med / (double)new_med);

  old_matrix_dot(old_mat, &aw, &b);
  new_matrix_dot(new_mat, &aw, &b);

  for(unsigned int i = 0; i < RRLWR_K; i++) {
    if(!poly_equal(&old_mat[i], &new_mat[i])) {
      fprintf(stderr, "Awin matrix-vector dot failed at row %u for K=%d\n",
              i, RRLWR_K);
      return 1;
    }
  }

  for(unsigned int i = 0; i < SAMPLES; i++) {
    uint64_t start = cpucycles_local();
    old_matrix_dot(old_mat, &aw, &b);
    old_samples[i] = cpucycles_local() - start - overhead;

    start = cpucycles_local();
    new_matrix_dot(new_mat, &aw, &b);
    new_samples[i] = cpucycles_local() - start - overhead;
  }

  old_med = median_local(old_samples);
  new_med = median_local(new_samples);

  printf("old Awin matrix-vector median: %llu cycles/ticks\n",
         (unsigned long long)old_med);
  printf("fused matrix-vector median:    %llu cycles/ticks\n",
         (unsigned long long)new_med);
  printf("matrix-vector speedup: %.2fx\n",
         new_med == 0 ? 0.0 : (double)old_med / (double)new_med);

  return 0;
}
