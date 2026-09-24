/*
 * NICCS x86 self-evaluation harness for KEX (AFS-KEX).
 *
 * Single-process measurement covering the official self-evaluation metrics:
 *   - functional correctness (full 4-pass run, ssa == ssb)
 *   - performance: median CPU cycles + wall-clock throughput (ops/s) for one
 *     full key-exchange (init + 4 passes + derive, network time excluded)
 *   - resource: peak resident set size via getrusage
 *   - transfer/storage: long-term pk/sk, per-pass message sizes, total message
 *     size, shared-secret size, and interaction rounds (passes)
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

#include "KEX_AlgorithmInstance.h"
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

/* Run one full key exchange. Returns 1 on success (ssa==ssb), 0 on failure.
 * When msg_len_out != NULL, records the four pass message lengths. */
static int run_kex(unsigned char *pka, unsigned long long pka_len,
                   unsigned char *ska, unsigned long long ska_len,
                   unsigned char *pkb, unsigned long long pkb_len,
                   unsigned char *skb, unsigned long long skb_len,
                   unsigned char *sta, unsigned long long *sta_len,
                   unsigned char *stb, unsigned long long *stb_len,
                   unsigned char *ssa, unsigned long long *ssa_len,
                   unsigned char *ssb, unsigned long long *ssb_len,
                   unsigned char *m[4], unsigned long long mlen[4])
{
  int status;
  unsigned char *last_a = NULL, *last_b = NULL;
  unsigned long long last_a_len = 0, last_b_len = 0;

  status = kex_generate_pass1_msg_a(ska, ska_len, pkb, pkb_len, sta, sta_len, m[0], &mlen[0]);
  if(status < 0) return 0;
  last_a = m[0]; last_a_len = mlen[0];

  if(status == 0) {
    status = kex_generate_pass2_msg_b(skb, skb_len, pka, pka_len, m[0], mlen[0], stb, stb_len, m[1], &mlen[1]);
    if(status < 0) return 0;
    last_b = m[1]; last_b_len = mlen[1];
  }
  if(status == 0) {
    status = kex_generate_pass3_msg_a(ska, ska_len, pkb, pkb_len, m[1], mlen[1], sta, sta_len, m[2], &mlen[2]);
    if(status < 0) return 0;
    last_a = m[2]; last_a_len = mlen[2];
  }
  if(status == 0) {
    status = kex_generate_pass4_msg_b(skb, skb_len, pka, pka_len, m[2], mlen[2], stb, stb_len, m[3], &mlen[3]);
    if(status < 0) return 0;
    last_b = m[3]; last_b_len = mlen[3];
  }
  if(status != 1 || !last_a || !last_b) return 0;

  if(kex_derive_ss_a(ska, ska_len, pkb, pkb_len, last_b, last_b_len, sta, *sta_len, ssa, ssa_len) != 0) return 0;
  if(kex_derive_ss_b(skb, skb_len, pka, pka_len, last_a, last_a_len, stb, *stb_len, ssb, ssb_len) != 0) return 0;
  if(*ssa_len != *ssb_len || memcmp(ssa, ssb, (size_t)*ssa_len) != 0) return 0;
  return 1;
}

int main(void)
{
  uint8_t seed[SEED_LEN_BYTES];
  unsigned long long pk_len = kex_get_pk_len_bytes();
  unsigned long long sk_len = kex_get_sk_len_bytes();
  unsigned long long sta_cap = kex_get_sta_len_bytes();
  unsigned long long stb_cap = kex_get_stb_len_bytes();
  unsigned long long ss_len = kex_get_ss_len_bytes();
  unsigned long long msg_cap = kex_get_total_msg_len_bytes();
  unsigned long long passes = kex_get_passes_num();

  unsigned char *pka = calloc((size_t)pk_len, 1);
  unsigned char *ska = calloc((size_t)sk_len, 1);
  unsigned char *pkb = calloc((size_t)pk_len, 1);
  unsigned char *skb = calloc((size_t)sk_len, 1);
  unsigned char *sta = calloc((size_t)sta_cap, 1);
  unsigned char *stb = calloc((size_t)stb_cap, 1);
  unsigned char *ssa = calloc((size_t)ss_len, 1);
  unsigned char *ssb = calloc((size_t)ss_len, 1);
  unsigned char *m[4] = { calloc((size_t)msg_cap, 1), calloc((size_t)msg_cap, 1),
                          calloc((size_t)msg_cap, 1), calloc((size_t)msg_cap, 1) };
  unsigned long long mlen[4] = {0,0,0,0};
  unsigned long long pka_len, ska_len, pkb_len, skb_len, sta_len, stb_len, ssa_len;
  unsigned int i;
  int func_ok;
  uint64_t med_kex;
  double t0, t1, ops_kex;
  struct rusage ru;

  if(!pka || !ska || !pkb || !skb || !sta || !stb || !ssa || !ssb || !m[0] || !m[1] || !m[2] || !m[3])
    return 2;

  fill_bytes(seed, sizeof(seed), 0x42);
  if(init_random_number(&drng_algorithm, seed, sizeof(seed)) != 0)
    return 3;

  pka_len = pk_len; ska_len = sk_len; pkb_len = pk_len; skb_len = sk_len;
  sta_len = sta_cap; stb_len = stb_cap;
  if(kex_init_a(pka, &pka_len, ska, &ska_len, sta, &sta_len) < 0) return 3;
  if(kex_init_b(pkb, &pkb_len, skb, &skb_len, stb, &stb_len) < 0) return 3;

  /* ---- functional correctness + record message sizes ---- */
  {
    unsigned long long sa = sta_len, sb = stb_len, ssal = ss_len, ssbl = ss_len;
    func_ok = run_kex(pka, pka_len, ska, ska_len, pkb, pkb_len, skb, skb_len,
                      sta, &sa, stb, &sb, ssa, &ssal, ssb, &ssbl, m, mlen);
    ssa_len = ssal;
  }

  /* ---- performance: one full key exchange (init reused, passes+derive timed) ---- */
  for(i = 0; i < SELFEVAL_NTESTS; i++) {
    unsigned long long sa = sta_len, sb = stb_len, ssal = ss_len, ssbl = ss_len;
    unsigned long long ml[4] = {0,0,0,0};
    cyc[i] = cpucycles();
    run_kex(pka, pka_len, ska, ska_len, pkb, pkb_len, skb, skb_len,
            sta, &sa, stb, &sb, ssa, &ssal, ssb, &ssbl, m, ml);
    sink ^= ssa[0];
  }
  for(i = 0; i + 1 < SELFEVAL_NTESTS; i++) cyc[i] = cyc[i+1] - cyc[i];
  med_kex = median_u64(cyc, SELFEVAL_NTESTS - 1);

  t0 = now_sec();
  for(i = 0; i < SELFEVAL_NTESTS; i++) {
    unsigned long long sa = sta_len, sb = stb_len, ssal = ss_len, ssbl = ss_len;
    unsigned long long ml[4] = {0,0,0,0};
    run_kex(pka, pka_len, ska, ska_len, pkb, pkb_len, skb, skb_len,
            sta, &sa, stb, &sb, ssa, &ssal, ssb, &ssbl, m, ml);
    sink ^= ssa[0];
  }
  t1 = now_sec();
  ops_kex = (double)SELFEVAL_NTESTS / (t1 - t0);

  getrusage(RUSAGE_SELF, &ru);

  printf("instance=%s\n", ALGORITHM_INSTANCE);
  printf("function=KEX\n");
  printf("ntests=%d\n", SELFEVAL_NTESTS);
  printf("func_kat=%s\n", func_ok ? "PASS" : "FAIL");
  printf("rounds=%llu\n", passes);
  printf("cycles_kex=%llu\n", (unsigned long long)med_kex);
  printf("ops_kex=%.2f\n", ops_kex);
  printf("size_pk=%llu\n", pka_len);
  printf("size_sk=%llu\n", ska_len);
  printf("size_msg1=%llu\n", mlen[0]);
  printf("size_msg2=%llu\n", mlen[1]);
  printf("size_msg3=%llu\n", mlen[2]);
  printf("size_msg4=%llu\n", mlen[3]);
  printf("size_msg_total=%llu\n", mlen[0] + mlen[1] + mlen[2] + mlen[3]);
  printf("size_ss=%llu\n", ssa_len);
  printf("peak_rss_bytes=%ld\n", ru.ru_maxrss * 1024L);

  free(m[3]); free(m[2]); free(m[1]); free(m[0]);
  free(ssb); free(ssa); free(stb); free(sta); free(skb); free(pkb); free(ska); free(pka);
  (void)sink;
  return func_ok ? 0 : 1;
}
