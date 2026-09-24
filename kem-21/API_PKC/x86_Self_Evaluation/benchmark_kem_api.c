#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DRNG_ctx drng_algorithm;
static volatile unsigned long long viper_bench_accumulator = 0;

#ifndef INSTANCE_NAME
#define INSTANCE_NAME "unknown"
#endif
#ifndef IMPLEMENTATION_NAME
#define IMPLEMENTATION_NAME "unknown"
#endif
#ifndef VIPER_LEVEL
#define VIPER_LEVEL 0
#endif

static int cmp_u64(const void *a, const void *b) {
  uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
  return (x > y) - (x < y);
}

#if defined(__x86_64__) || defined(__i386__)
static uint64_t ticks(void) {
  unsigned hi, lo;
  __asm__ __volatile__("lfence\n\trdtsc" : "=a"(lo), "=d"(hi) :: "memory");
  return ((uint64_t)hi << 32) | lo;
}
static uint64_t ticks_end(void) {
  unsigned hi, lo;
  __asm__ __volatile__("rdtscp\n\tlfence" : "=a"(lo), "=d"(hi) :: "rcx", "memory");
  return ((uint64_t)hi << 32) | lo;
}
#else
#include <time.h>
static uint64_t ticks(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec; }
static uint64_t ticks_end(void) { return ticks(); }
#endif

static int seed_drng(unsigned int domain) {
  unsigned char seed[64];
  for (size_t i = 0; i < sizeof(seed); i++) seed[i] = (unsigned char)((i * 13U + domain * 29U + 1U) & 0xffU);
  return init_random_number(&drng_algorithm, seed, sizeof(seed));
}

static void stats(uint64_t *a, int n, double *mean, double *median, uint64_t *minv, uint64_t *maxv, double *sd) {
  uint64_t *tmp = malloc((size_t)n * sizeof(uint64_t));
  double sum = 0.0, var = 0.0;
  memcpy(tmp, a, (size_t)n * sizeof(uint64_t));
  qsort(tmp, (size_t)n, sizeof(uint64_t), cmp_u64);
  *minv = tmp[0]; *maxv = tmp[n - 1];
  for (int i = 0; i < n; i++) sum += (double)a[i];
  *mean = sum / (double)n;
  *median = (n & 1) ? (double)tmp[n / 2] : ((double)tmp[n / 2 - 1] + (double)tmp[n / 2]) / 2.0;
  for (int i = 0; i < n; i++) { double d = (double)a[i] - *mean; var += d * d; }
  *sd = sqrt(var / (double)n);
  free(tmp);
}

static void emit(const char *op, uint64_t *a, int n, int warmup, unsigned long long pkb, unsigned long long skb, unsigned long long ssb, unsigned long long ctb) {
  double mean, median, sd; uint64_t minv, maxv;
  stats(a, n, &mean, &median, &minv, &maxv, &sd);
  printf("RESULT,%s,%s,%d,%s,%d,%d,%.2f,%.2f,%llu,%llu,%.2f,%.6f,%llu,%llu,%llu,%llu,PASS,api-wrapper\n",
         IMPLEMENTATION_NAME, INSTANCE_NAME, VIPER_LEVEL, op, warmup, n, mean, median,
         (unsigned long long)minv, (unsigned long long)maxv, sd, mean > 0.0 ? 1.0 / mean : 0.0,
         pkb, skb, ssb, ctb);
}

int main(int argc, char **argv) {
  int runs = argc > 1 ? atoi(argv[1]) : 1000;
  int warmup = argc > 2 ? atoi(argv[2]) : 100;
  if (runs <= 0 || warmup < 0) return 2;
  unsigned long long pkb = kem_get_pk_len_bytes(), skb = kem_get_sk_len_bytes(), ssb = kem_get_ss_len_bytes(), ctb = kem_get_ct_len_bytes();
  unsigned char *pk = calloc(pkb,1), *sk = calloc(skb,1), *ss = calloc(ssb,1), *ss2 = calloc(ssb,1), *ct = calloc(ctb,1), *badct = calloc(ctb,1);
  uint64_t *kg = calloc((size_t)runs,sizeof(uint64_t)), *en = calloc((size_t)runs,sizeof(uint64_t)), *de = calloc((size_t)runs,sizeof(uint64_t));
  if (!pk || !sk || !ss || !ss2 || !ct || !badct || !kg || !en || !de) return 3;
  printf("INFO,%s,%s,%d,pk=%llu,sk=%llu,ss=%llu,ct=%llu,warmup=%d,runs=%d\n", IMPLEMENTATION_NAME, INSTANCE_NAME, VIPER_LEVEL, pkb, skb, ssb, ctb, warmup, runs);
  if (seed_drng(1)) return 4;
  for (int i = 0; i < warmup; i++) {
    unsigned long long pl=0, sl=0, ssl=0, ctl=0, ssl2=0;
    if (kem_keygen(pk,&pl,sk,&sl) || kem_enc(pk,pl,ss,&ssl,ct,&ctl) || kem_dec(sk,sl,ct,ctl,ss2,&ssl2) || ssl != ssl2 || memcmp(ss,ss2,ssb)) return 5;
    viper_bench_accumulator += pk[0] + sk[0] + ct[0] + ss[0];
  }
  if (seed_drng(2)) return 6;
  for (int i = 0; i < runs; i++) { unsigned long long pl=0, sl=0; uint64_t s=ticks(); int r=kem_keygen(pk,&pl,sk,&sl); uint64_t e=ticks_end(); if(r||pl!=pkb||sl!=skb)return 7; kg[i]=e-s; viper_bench_accumulator += pk[i%pkb]^sk[i%skb]; }
  if (seed_drng(3)) return 8;
  if (kem_keygen(pk, &(unsigned long long){0}, sk, &(unsigned long long){0})) return 9;
  for (int i = 0; i < runs; i++) { unsigned long long ssl=0, ctl=0; uint64_t s=ticks(); int r=kem_enc(pk,pkb,ss,&ssl,ct,&ctl); uint64_t e=ticks_end(); if(r||ssl!=ssb||ctl!=ctb)return 10; en[i]=e-s; if ((i % 17)==0) { unsigned long long ssl2=0; if(kem_dec(sk,skb,ct,ctb,ss2,&ssl2)||ssl2!=ssb||memcmp(ss,ss2,ssb)) return 11; } viper_bench_accumulator += ct[i%ctb]^ss[i%ssb]; }
  if (kem_enc(pk,pkb,ss,&(unsigned long long){0},ct,&(unsigned long long){0})) return 12;
  for (int i = 0; i < runs; i++) { unsigned long long ssl2=0; uint64_t s=ticks(); int r=kem_dec(sk,skb,ct,ctb,ss2,&ssl2); uint64_t e=ticks_end(); if(r||ssl2!=ssb||memcmp(ss,ss2,ssb))return 13; de[i]=e-s; viper_bench_accumulator += ss2[i%ssb]; }
  memcpy(badct, ct, ctb); badct[0] ^= 0x80; unsigned long long bl=0; if (kem_dec(sk,skb,badct,ctb,ss2,&bl) || bl != ssb) return 14; puts("CHECK,implicit_rejection,PASS");
  emit("keygen", kg, runs, warmup, pkb, skb, ssb, ctb); emit("encaps", en, runs, warmup, pkb, skb, ssb, ctb); emit("decaps", de, runs, warmup, pkb, skb, ssb, ctb);
  fprintf(stderr, "accumulator=%llu\n", viper_bench_accumulator);
  free(pk); free(sk); free(ss); free(ss2); free(ct); free(badct); free(kg); free(en); free(de); return 0;
}
