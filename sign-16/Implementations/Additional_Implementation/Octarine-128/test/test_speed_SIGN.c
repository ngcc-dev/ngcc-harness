#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cpucycles.h"
#include "drng.h"
#include "sign.h"

#define NUMBER_OF_TESTS 1000
static uint64_t t[NUMBER_OF_TESTS];

#define RNG_SEED_LENGTH 32
#define BENCHMARK_MESSAGE_LEN 32
DRNG_ctx drng_algorithm;

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

static void print_stage_results(const char *label, const uint64_t *stage_cycles, size_t n)
{
  uint64_t stage_med = median_local(stage_cycles, n);
  uint64_t stage_avg = average_local(stage_cycles, n);

  printf("%s\n", label);
  printf("median: %llu cycles/ticks\n", (unsigned long long)stage_med);
  printf("average: %llu cycles/ticks\n", (unsigned long long)stage_avg);
  printf("\n");
}

int main() {
  unsigned char sk[RRLWR_SIGN_SK_LEN];
  unsigned char pk[RRLWR_SIGN_PK_LEN];
  unsigned char sn[RRLWR_SIGN_SIG_LEN];
  unsigned char m[BENCHMARK_MESSAGE_LEN] = {0};
  unsigned long long pk_len_bytes, sk_len_bytes, m_len_bytes = BENCHMARK_MESSAGE_LEN, sn_len_bytes;

  /* Initialize RNG*/
  const unsigned char seed[RNG_SEED_LENGTH] = {0};
  init_random_number(&drng_algorithm, seed, RNG_SEED_LENGTH);
  uint64_t overhead = cpucycles_overhead();

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    uint64_t start = cpucycles();
    sig_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
    t[i] = cpucycles() - start - overhead;
  }
  print_stage_results("keygen: ", t, NUMBER_OF_TESTS);

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    uint64_t start = cpucycles();
    sig_sign(sk, sk_len_bytes, m, m_len_bytes, sn, &sn_len_bytes);
    t[i] = cpucycles() - start - overhead;
  }
  print_stage_results("sign: ", t, NUMBER_OF_TESTS);

  for(unsigned int i=0;i<NUMBER_OF_TESTS;i++) {
    uint64_t start = cpucycles();
    sig_verify(pk, pk_len_bytes, sn, sn_len_bytes, m, m_len_bytes);
    t[i] = cpucycles() - start - overhead;
  }
  print_stage_results("verify: ", t, NUMBER_OF_TESTS);
}
