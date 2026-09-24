/*
 * Standalone benchmark for arith/poly.S against scalar C implementations.
 *
 * Build from the repository root, without using the Makefile:
 *
 *   gcc -O3 -fno-tree-vectorize -fno-tree-slp-vectorize -mavx2 \
 *     -I. -I./arith -I./utils \
 *     test/bench_poly_s_vs_c.c arith/poly.S \
 *     -o test/bench_poly_s_vs_c
 *
 * This intentionally compiles the C reference code at -O3 with GCC's
 * auto-vectorizers disabled, while still linking the handwritten AVX2
 * assembly from arith/poly.S.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parameters.h"
#include "poly.h"

#define SAMPLES 20000

static uint64_t g_rng = 0x123456789abcdef0ULL;

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

static void fill_pow2_signed(poly *r, int32_t d)
{
  int32_t half = (int32_t)1 << (d - 1);
  int32_t mod = (int32_t)1 << d;

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = (int32_t)(rand32_local() & (uint32_t)(mod - 1)) - half;
  }
}

static int poly_equal(const poly *a, const poly *b)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    if(a->coeffs[i] != b->coeffs[i]) {
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

static int32_t montgomery_reduce32_c(int64_t a, int32_t prime,
                                     int32_t primeinv)
{
  int32_t t = (int32_t)a * primeinv;

  return (int32_t)((a + (int64_t)t * prime) >> 32);
}

static int32_t montgomery_mul32_c(int32_t a, int32_t b, int32_t prime,
                                  int32_t primeinv)
{
  return montgomery_reduce32_c((int64_t)a * b, prime, primeinv);
}

static int32_t conditional_reduce32_c(int32_t r, int32_t prime)
{
  int32_t sign_mask = -((r >> 31) & 1);
  int32_t shift = ((2 * prime) & sign_mask) - prime;

  return r + shift;
}

static int32_t conditional_final_reduce32_c(int32_t r, int32_t prime)
{
  int32_t offset = (prime - 1) >> 1;
  int32_t sign_mask_neg = -(((r + (offset - 1)) >> 31) & 1);
  int32_t sign_mask_pos = -(((r - (offset + 1)) >> 31) & 1);
  int32_t shift = (prime & sign_mask_neg) - (prime & ~sign_mask_pos);

  return r + shift;
}

__attribute__((noinline))
static void poly_basemul32_c(poly *r, poly *f, poly *g, int32_t prime,
                             int32_t primeinv)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = montgomery_mul32_c(f->coeffs[i], g->coeffs[i], prime,
                                      primeinv);
  }
}

__attribute__((noinline))
static void poly_add32_c(poly *r, poly *f, poly *g, int32_t prime)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = conditional_reduce32_c(f->coeffs[i] + g->coeffs[i], prime);
  }
}

__attribute__((noinline))
static void poly_add_c(poly *r, poly *f, poly *g)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = f->coeffs[i] + g->coeffs[i];
  }
}

__attribute__((noinline))
static void poly_sub32_c(poly *r, poly *f, poly *g, int32_t prime)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = conditional_reduce32_c(f->coeffs[i] - g->coeffs[i], prime);
  }
}

__attribute__((noinline))
static void poly_sub_c(poly *r, poly *f, poly *g)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = f->coeffs[i] - g->coeffs[i];
  }
}

__attribute__((noinline))
static void poly_reduce_pow2_c(poly *r, poly *f, int32_t d)
{
  int32_t pow2div2 = (int32_t)1 << (d - 1);
  int32_t pow2 = (int32_t)1 << d;

  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = ((f->coeffs[i] + pow2div2) & (pow2 - 1)) - pow2div2;
  }
}

__attribute__((noinline))
static void poly_conditional_final_reduce32_c(poly *r, int32_t prime)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = conditional_final_reduce32_c(r->coeffs[i], prime);
  }
}

__attribute__((noinline))
static void poly_round_xtoy_c(poly *r, const poly *f, int32_t x, int32_t y)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] = f->coeffs[i] + ((int32_t)1 << (x - (y + 1)));
    r->coeffs[i] >>= (x - y);
    r->coeffs[i] &= ((int32_t)1 << y) - 1;
  }
}

__attribute__((noinline))
static void poly_compress_c(poly *r, int32_t x)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] >>= x;
  }
}

__attribute__((noinline))
static void poly_decompress_c(poly *r, int32_t x)
{
  for(unsigned int i = 0; i < RRLWR_N; i++) {
    r->coeffs[i] <<= x;
  }
}

static uint64_t bench_binary_c(void (*fn)(poly *, poly *, poly *),
                               poly *r, poly *f, poly *g, uint64_t overhead)
{
  static uint64_t samples[SAMPLES];

  for(unsigned int i = 0; i < SAMPLES; i++) {
    uint64_t start = cpucycles_local();
    fn(r, f, g);
    samples[i] = cpucycles_local() - start - overhead;
  }

  return median_local(samples);
}

static uint64_t bench_binary_s(void (*fn)(poly *, poly *, poly *),
                               poly *r, poly *f, poly *g, uint64_t overhead)
{
  return bench_binary_c(fn, r, f, g, overhead);
}

static void print_result(const char *name, uint64_t c_cycles,
                         uint64_t s_cycles)
{
  double speedup = s_cycles == 0 ? 0.0 : (double)c_cycles / (double)s_cycles;

  printf("%-36s C median: %8llu  asm median: %8llu  speedup: %6.2fx\n",
         name, (unsigned long long)c_cycles, (unsigned long long)s_cycles,
         speedup);
}

int main(void)
{
  uint64_t overhead = cpucycles_overhead_local();
  poly f;
  poly g;
  poly rc;
  poly rs;
  poly tmp;
  uint64_t samples_c[SAMPLES];
  uint64_t samples_s[SAMPLES];

  fill_mod_prime(&f);
  fill_mod_prime(&g);

#define CHECK_BINARY(name, c_fn, s_fn)                                             \
  do {                                                                            \
    memset(&rc, 0, sizeof(rc));                                                    \
    memset(&rs, 0, sizeof(rs));                                                    \
    c_fn(&rc, &f, &g);                                                             \
    s_fn(&rs, &f, &g);                                                             \
    if(!poly_equal(&rc, &rs)) {                                                    \
      fprintf(stderr, "%s correctness failed\n", name);                           \
      return 1;                                                                    \
    }                                                                              \
    uint64_t c_med = bench_binary_c(c_fn, &rc, &f, &g, overhead);                  \
    uint64_t s_med = bench_binary_s(s_fn, &rs, &f, &g, overhead);                  \
    print_result(name, c_med, s_med);                                              \
  } while(0)

  printf("poly.S vs scalar C, C compiled with -O3 and auto-vectorization off\n");
  printf("samples: %u, rdtsc overhead: %llu cycles/ticks\n\n",
         SAMPLES, (unsigned long long)overhead);

  memset(&rc, 0, sizeof(rc));
  memset(&rs, 0, sizeof(rs));
  poly_basemul32_c(&rc, &f, &g, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
  poly_basemul32(&rs, &f, &g, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
  if(!poly_equal(&rc, &rs)) {
    fprintf(stderr, "poly_basemul32 correctness failed\n");
    return 1;
  }
  for(unsigned int i = 0; i < SAMPLES; i++) {
    uint64_t start = cpucycles_local();
    poly_basemul32_c(&rc, &f, &g, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
    samples_c[i] = cpucycles_local() - start - overhead;
    start = cpucycles_local();
    poly_basemul32(&rs, &f, &g, RRLWR_PKE_PRIME, RRLWR_PKE_PRIMEINV);
    samples_s[i] = cpucycles_local() - start - overhead;
  }
  print_result("poly_basemul32", median_local(samples_c), median_local(samples_s));

  memset(&rc, 0, sizeof(rc));
  memset(&rs, 0, sizeof(rs));
  poly_add32_c(&rc, &f, &g, RRLWR_PKE_PRIME);
  poly_add32(&rs, &f, &g, RRLWR_PKE_PRIME);
  if(!poly_equal(&rc, &rs)) {
    fprintf(stderr, "poly_add32 correctness failed\n");
    return 1;
  }
  for(unsigned int i = 0; i < SAMPLES; i++) {
    uint64_t start = cpucycles_local();
    poly_add32_c(&rc, &f, &g, RRLWR_PKE_PRIME);
    samples_c[i] = cpucycles_local() - start - overhead;
    start = cpucycles_local();
    poly_add32(&rs, &f, &g, RRLWR_PKE_PRIME);
    samples_s[i] = cpucycles_local() - start - overhead;
  }
  print_result("poly_add32", median_local(samples_c), median_local(samples_s));

  CHECK_BINARY("poly_add", poly_add_c, poly_add);

  memset(&rc, 0, sizeof(rc));
  memset(&rs, 0, sizeof(rs));
  poly_sub32_c(&rc, &f, &g, RRLWR_PKE_PRIME);
  poly_sub32(&rs, &f, &g, RRLWR_PKE_PRIME);
  if(!poly_equal(&rc, &rs)) {
    fprintf(stderr, "poly_sub32 correctness failed\n");
    return 1;
  }
  for(unsigned int i = 0; i < SAMPLES; i++) {
    uint64_t start = cpucycles_local();
    poly_sub32_c(&rc, &f, &g, RRLWR_PKE_PRIME);
    samples_c[i] = cpucycles_local() - start - overhead;
    start = cpucycles_local();
    poly_sub32(&rs, &f, &g, RRLWR_PKE_PRIME);
    samples_s[i] = cpucycles_local() - start - overhead;
  }
  print_result("poly_sub32", median_local(samples_c), median_local(samples_s));

  CHECK_BINARY("poly_sub", poly_sub_c, poly_sub);

  fill_pow2_signed(&f, RRLWR_PKE_LOGQ);
  poly_reduce_pow2_c(&rc, &f, RRLWR_PKE_LOGQ);
  poly_reduce_pow2(&rs, &f, RRLWR_PKE_LOGQ);
  if(!poly_equal(&rc, &rs)) {
    fprintf(stderr, "poly_reduce_pow2 correctness failed\n");
    return 1;
  }
  for(unsigned int i = 0; i < SAMPLES; i++) {
    uint64_t start = cpucycles_local();
    poly_reduce_pow2_c(&rc, &f, RRLWR_PKE_LOGQ);
    samples_c[i] = cpucycles_local() - start - overhead;
    start = cpucycles_local();
    poly_reduce_pow2(&rs, &f, RRLWR_PKE_LOGQ);
    samples_s[i] = cpucycles_local() - start - overhead;
  }
  print_result("poly_reduce_pow2(logq)", median_local(samples_c),
               median_local(samples_s));

  fill_mod_prime(&tmp);
  rc = tmp;
  rs = tmp;
  poly_conditional_final_reduce32_c(&rc, RRLWR_PKE_PRIME);
  poly_conditional_final_reduce32(&rs, RRLWR_PKE_PRIME);
  if(!poly_equal(&rc, &rs)) {
    fprintf(stderr, "poly_conditional_final_reduce32 correctness failed\n");
    return 1;
  }
  for(unsigned int i = 0; i < SAMPLES; i++) {
    rc = tmp;
    uint64_t start = cpucycles_local();
    poly_conditional_final_reduce32_c(&rc, RRLWR_PKE_PRIME);
    samples_c[i] = cpucycles_local() - start - overhead;
    rs = tmp;
    start = cpucycles_local();
    poly_conditional_final_reduce32(&rs, RRLWR_PKE_PRIME);
    samples_s[i] = cpucycles_local() - start - overhead;
  }
  print_result("poly_cond_final_reduce32", median_local(samples_c),
               median_local(samples_s));

  fill_pow2_signed(&f, RRLWR_PKE_LOGQ);
  poly_round_xtoy_c(&rc, &f, RRLWR_PKE_LOGQ, RRLWR_PKE_LOGP);
  poly_round_xtoy(&rs, &f, RRLWR_PKE_LOGQ, RRLWR_PKE_LOGP);
  if(!poly_equal(&rc, &rs)) {
    fprintf(stderr, "poly_round_xtoy q_to_p correctness failed\n");
    return 1;
  }
  for(unsigned int i = 0; i < SAMPLES; i++) {
    uint64_t start = cpucycles_local();
    poly_round_xtoy_c(&rc, &f, RRLWR_PKE_LOGQ, RRLWR_PKE_LOGP);
    samples_c[i] = cpucycles_local() - start - overhead;
    start = cpucycles_local();
    poly_round_xtoy(&rs, &f, RRLWR_PKE_LOGQ, RRLWR_PKE_LOGP);
    samples_s[i] = cpucycles_local() - start - overhead;
  }
  print_result("poly_round_xtoy(logq,logp)", median_local(samples_c),
               median_local(samples_s));

  fill_pow2_signed(&f, RRLWR_PKE_LOGP);
  poly_round_xtoy_c(&rc, &f, RRLWR_PKE_LOGP, 1);
  poly_round_xtoy(&rs, &f, RRLWR_PKE_LOGP, 1);
  if(!poly_equal(&rc, &rs)) {
    fprintf(stderr, "poly_round_xtoy p_to_1 correctness failed\n");
    return 1;
  }
  for(unsigned int i = 0; i < SAMPLES; i++) {
    uint64_t start = cpucycles_local();
    poly_round_xtoy_c(&rc, &f, RRLWR_PKE_LOGP, 1);
    samples_c[i] = cpucycles_local() - start - overhead;
    start = cpucycles_local();
    poly_round_xtoy(&rs, &f, RRLWR_PKE_LOGP, 1);
    samples_s[i] = cpucycles_local() - start - overhead;
  }
  print_result("poly_round_xtoy(logp,1)", median_local(samples_c),
               median_local(samples_s));

  fill_pow2_signed(&tmp, RRLWR_PKE_LOGP);
  rc = tmp;
  rs = tmp;
  poly_compress_c(&rc, RRLWR_PKE_LOGP - 3);
  poly_compress(&rs, RRLWR_PKE_LOGP - 3);
  if(!poly_equal(&rc, &rs)) {
    fprintf(stderr, "poly_compress correctness failed\n");
    return 1;
  }
  for(unsigned int i = 0; i < SAMPLES; i++) {
    rc = tmp;
    uint64_t start = cpucycles_local();
    poly_compress_c(&rc, RRLWR_PKE_LOGP - 3);
    samples_c[i] = cpucycles_local() - start - overhead;
    rs = tmp;
    start = cpucycles_local();
    poly_compress(&rs, RRLWR_PKE_LOGP - 3);
    samples_s[i] = cpucycles_local() - start - overhead;
  }
  print_result("poly_compress(logp-3)", median_local(samples_c),
               median_local(samples_s));

  fill_pow2_signed(&tmp, 3);
  rc = tmp;
  rs = tmp;
  poly_decompress_c(&rc, RRLWR_PKE_LOGP - 3);
  poly_decompress(&rs, RRLWR_PKE_LOGP - 3);
  if(!poly_equal(&rc, &rs)) {
    fprintf(stderr, "poly_decompress correctness failed\n");
    return 1;
  }
  for(unsigned int i = 0; i < SAMPLES; i++) {
    rc = tmp;
    uint64_t start = cpucycles_local();
    poly_decompress_c(&rc, RRLWR_PKE_LOGP - 3);
    samples_c[i] = cpucycles_local() - start - overhead;
    rs = tmp;
    start = cpucycles_local();
    poly_decompress(&rs, RRLWR_PKE_LOGP - 3);
    samples_s[i] = cpucycles_local() - start - overhead;
  }
  print_result("poly_decompress(logp-3)", median_local(samples_c),
               median_local(samples_s));

  return 0;
}
