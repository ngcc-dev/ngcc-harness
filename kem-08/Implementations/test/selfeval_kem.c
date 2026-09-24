/*
 * NICCS x86 self-evaluation harness for KEM (BW-KEM).
 *
 * Single-process measurement covering the official self-evaluation metrics:
 *   - functional correctness (KAT-style keygen->enc->dec consistency)
 *   - performance: median CPU cycles + wall-clock derived throughput (ops/s)
 *   - resource: peak resident set size via getrusage
 *   - transfer/storage: pk/sk/ct/ss sizes via the ICCS API layer
 *
 * Output is a stable KEY=VALUE block consumed by report_selfeval.py.
 */
#define _POSIX_C_SOURCE 200809L
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>

#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "cpucycles.h"

#ifndef SELFEVAL_NTESTS
#define SELFEVAL_NTESTS 1000
#endif
#define SEED_LEN_BYTES 64

DRNG_ctx drng_algorithm;

static uint64_t cyc[SELFEVAL_NTESTS];
static volatile uint8_t sink;

static void fill_bytes(uint8_t *buf, size_t len, uint8_t seed)
{
  size_t i;
  for(i = 0; i < len; i++)
    buf[i] = (uint8_t)(seed + 17u * (uint8_t)i + (uint8_t)(i >> 1));
}

static int cmp_u64(const void *a, const void *b)
{
  uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
  return (x > y) - (x < y);
}

static uint64_t median_u64(uint64_t *t, size_t n)
{
  qsort(t, n, sizeof(uint64_t), cmp_u64);
  return (n & 1) ? t[n/2] : (t[n/2-1] + t[n/2]) / 2;
}

static double now_sec(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

int main(void)
{
  uint8_t seed[SEED_LEN_BYTES];
  unsigned long long pk_len = kem_get_pk_len_bytes();
  unsigned long long sk_len = kem_get_sk_len_bytes();
  unsigned long long ss_len = kem_get_ss_len_bytes();
  unsigned long long ct_len = kem_get_ct_len_bytes();
  unsigned char *pk = calloc((size_t)pk_len, 1);
  unsigned char *sk = calloc((size_t)sk_len, 1);
  unsigned char *ct = calloc((size_t)ct_len, 1);
  unsigned char *ss = calloc((size_t)ss_len, 1);
  unsigned char *ss2 = calloc((size_t)ss_len, 1);
  unsigned int i;
  int func_ok = 1;
  uint64_t med_keygen, med_enc, med_dec;
  double t0, t1, ops_keygen, ops_enc, ops_dec;
  struct rusage ru;

  if(!pk || !sk || !ct || !ss || !ss2)
    return 2;

  fill_bytes(seed, sizeof(seed), 0x42);
  if(init_random_number(&drng_algorithm, seed, sizeof(seed)) != 0)
    return 3;

  /* ---- functional correctness ---- */
  {
    unsigned long long pl = pk_len, skl = sk_len, sl = ss_len, cl = ct_len, dl = ss_len;
    if(kem_keygen(pk, &pl, sk, &skl) != 0) func_ok = 0;
    if(func_ok && kem_enc(pk, pl, ss, &sl, ct, &cl) != 0) func_ok = 0;
    if(func_ok && kem_dec(sk, skl, ct, cl, ss2, &dl) != 0) func_ok = 0;
    if(func_ok && (sl != dl || memcmp(ss, ss2, (size_t)sl) != 0)) func_ok = 0;
  }

  /* ---- performance: keygen ---- */
  for(i = 0; i < SELFEVAL_NTESTS; i++) {
    cyc[i] = cpucycles();
    kem_keygen(pk, &pk_len, sk, &sk_len);
    sink ^= pk[0];
  }
  for(i = 0; i + 1 < SELFEVAL_NTESTS; i++) cyc[i] = cyc[i+1] - cyc[i];
  med_keygen = median_u64(cyc, SELFEVAL_NTESTS - 1);
  t0 = now_sec();
  for(i = 0; i < SELFEVAL_NTESTS; i++) { kem_keygen(pk, &pk_len, sk, &sk_len); sink ^= pk[0]; }
  t1 = now_sec();
  ops_keygen = (double)SELFEVAL_NTESTS / (t1 - t0);

  /* ---- performance: enc ---- */
  for(i = 0; i < SELFEVAL_NTESTS; i++) {
    cyc[i] = cpucycles();
    kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len);
    sink ^= ct[0];
  }
  for(i = 0; i + 1 < SELFEVAL_NTESTS; i++) cyc[i] = cyc[i+1] - cyc[i];
  med_enc = median_u64(cyc, SELFEVAL_NTESTS - 1);
  t0 = now_sec();
  for(i = 0; i < SELFEVAL_NTESTS; i++) { kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len); sink ^= ct[0]; }
  t1 = now_sec();
  ops_enc = (double)SELFEVAL_NTESTS / (t1 - t0);

  /* ---- performance: dec ---- */
  for(i = 0; i < SELFEVAL_NTESTS; i++) {
    cyc[i] = cpucycles();
    kem_dec(sk, sk_len, ct, ct_len, ss2, &ss_len);
    sink ^= ss2[0];
  }
  for(i = 0; i + 1 < SELFEVAL_NTESTS; i++) cyc[i] = cyc[i+1] - cyc[i];
  med_dec = median_u64(cyc, SELFEVAL_NTESTS - 1);
  t0 = now_sec();
  for(i = 0; i < SELFEVAL_NTESTS; i++) { kem_dec(sk, sk_len, ct, ct_len, ss2, &ss_len); sink ^= ss2[0]; }
  t1 = now_sec();
  ops_dec = (double)SELFEVAL_NTESTS / (t1 - t0);

  getrusage(RUSAGE_SELF, &ru);

  printf("instance=%s\n", ALGORITHM_INSTANCE);
  printf("function=KEM\n");
  printf("ntests=%d\n", SELFEVAL_NTESTS);
  printf("func_kat=%s\n", func_ok ? "PASS" : "FAIL");
  printf("cycles_keygen=%llu\n", (unsigned long long)med_keygen);
  printf("cycles_enc=%llu\n", (unsigned long long)med_enc);
  printf("cycles_dec=%llu\n", (unsigned long long)med_dec);
  printf("ops_keygen=%.2f\n", ops_keygen);
  printf("ops_enc=%.2f\n", ops_enc);
  printf("ops_dec=%.2f\n", ops_dec);
  printf("size_pk=%llu\n", pk_len);
  printf("size_sk=%llu\n", sk_len);
  printf("size_ct=%llu\n", ct_len);
  printf("size_ss=%llu\n", ss_len);
  printf("peak_rss_bytes=%ld\n", ru.ru_maxrss * 1024L);

  free(ss2); free(ss); free(ct); free(sk); free(pk);
  (void)sink;
  return func_ok ? 0 : 1;
}
