#define _POSIX_C_SOURCE 199309L

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <x86intrin.h>

#include "params.h"
#include "drng.h"
#include "sign.h"
#include "poly.h"
#include "polyvec.h"

#define NTESTS 1000
#define PERF_MSG_BYTES 64
#define SEED_LEN_BYTES 64

DRNG_ctx drng_algorithm;

static uint64_t now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint64_t now_cycles(void) {
  unsigned int aux = 0;
  _mm_lfence();
  return __rdtscp(&aux);
}

static void sort_u64(uint64_t *a, size_t n) {
  size_t i;

  for(i = 1; i < n; i++) {
    uint64_t x = a[i];
    size_t j = i;

    while(j > 0 && a[j - 1] > x) {
      a[j] = a[j - 1];
      j--;
    }
    a[j] = x;
  }
}

static void print_stats_csv(const char *category,
                            const char *name,
                            const uint64_t *ns,
                            const uint64_t *cycles,
                            size_t n) {
  uint64_t ns_sorted[NTESTS];
  uint64_t cycles_sorted[NTESTS];
  uint64_t min_ns = ns[0];
  uint64_t min_cycles = cycles[0];
  uint64_t sum_ns = 0;
  uint64_t sum_cycles = 0;
  double avg_ns;
  double avg_cycles;
  double ops_per_sec;
  size_t i;

  for(i = 0; i < n; i++) {
    ns_sorted[i] = ns[i];
    cycles_sorted[i] = cycles[i];
    sum_ns += ns[i];
    sum_cycles += cycles[i];
    if(ns[i] < min_ns)
      min_ns = ns[i];
    if(cycles[i] < min_cycles)
      min_cycles = cycles[i];
  }

  sort_u64(ns_sorted, n);
  sort_u64(cycles_sorted, n);

  avg_ns = (double)sum_ns / (double)n;
  avg_cycles = (double)sum_cycles / (double)n;
  ops_per_sec = avg_ns > 0.0 ? 1000000000.0 / avg_ns : 0.0;

  printf("%s,%s,%zu,%llu,%llu,%.2f,%llu,%llu,%.2f,%.2f\n",
         category,
         name,
         n,
         (unsigned long long)min_ns,
         (unsigned long long)ns_sorted[n / 2],
         avg_ns,
         (unsigned long long)min_cycles,
         (unsigned long long)cycles_sorted[n / 2],
         avg_cycles,
         ops_per_sec);
}

int main(void) {
  uint64_t ns_samples[NTESTS];
  uint64_t cycle_samples[NTESTS];
  size_t siglen = 0;
  uint8_t pk[CRYPTO_PUBLICKEYBYTES];
  uint8_t sk[CRYPTO_SECRETKEYBYTES];
  uint8_t sig[CRYPTO_BYTES];
  uint8_t msg[PERF_MSG_BYTES];
  uint8_t seed_seed[SEEDBYTES];
  uint8_t seed_crh[CRHBYTES];
  uint8_t seed_ct[CTILDEBYTES];
  polyvecl mat[K];
  poly *a = &mat[0].vec[0];
  poly *b = &mat[0].vec[1];
  poly *c = &mat[0].vec[2];
  size_t i;

  {
    uint8_t seed[SEED_LEN_BYTES];

    for(i = 0; i < SEED_LEN_BYTES; i++)
      seed[i] = (uint8_t)(i * 3u + 1u);
    init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);
  }

  get_random_number(&drng_algorithm, seed_seed, (unsigned long long)sizeof(seed_seed) * 8ULL);
  get_random_number(&drng_algorithm, seed_crh, (unsigned long long)sizeof(seed_crh) * 8ULL);
  get_random_number(&drng_algorithm, seed_ct, (unsigned long long)sizeof(seed_ct) * 8ULL);
  get_random_number(&drng_algorithm, msg, (unsigned long long)sizeof(msg) * 8ULL);

  if(crypto_sign_keypair(pk, sk) != 0) {
    fprintf(stderr, "initial keypair generation failed\n");
    return 1;
  }
  if(crypto_sign_signature(sig, &siglen, msg, sizeof(msg), NULL, 0, sk) != 0) {
    fprintf(stderr, "initial signing failed\n");
    return 1;
  }
  if(crypto_sign_verify(sig, siglen, msg, sizeof(msg), NULL, 0, pk) != 0) {
    fprintf(stderr, "initial verification failed\n");
    return 1;
  }

  printf("category,operation,samples,min_ns,median_ns,avg_ns,min_cycles,median_cycles,avg_cycles,ops_per_sec\n");

  for(i = 0; i < NTESTS; i++) {
    uint64_t start_ns = now_ns();
    uint64_t start_cycles = now_cycles();

    if(crypto_sign_keypair(pk, sk) != 0) {
      fprintf(stderr, "keypair benchmark failed at iteration %zu\n", i);
      return 1;
    }

    cycle_samples[i] = now_cycles() - start_cycles;
    ns_samples[i] = now_ns() - start_ns;
  }
  print_stats_csv("core", "keygen", ns_samples, cycle_samples, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    uint64_t start_ns = now_ns();
    uint64_t start_cycles = now_cycles();

    if(crypto_sign_signature(sig, &siglen, msg, sizeof(msg), NULL, 0, sk) != 0) {
      fprintf(stderr, "sign benchmark failed at iteration %zu\n", i);
      return 1;
    }

    cycle_samples[i] = now_cycles() - start_cycles;
    ns_samples[i] = now_ns() - start_ns;
  }
  print_stats_csv("core", "sign_64B", ns_samples, cycle_samples, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    uint64_t start_ns = now_ns();
    uint64_t start_cycles = now_cycles();

    if(crypto_sign_verify(sig, siglen, msg, sizeof(msg), NULL, 0, pk) != 0) {
      fprintf(stderr, "verify benchmark failed at iteration %zu\n", i);
      return 1;
    }

    cycle_samples[i] = now_cycles() - start_cycles;
    ns_samples[i] = now_ns() - start_ns;
  }
  print_stats_csv("core", "verify_64B", ns_samples, cycle_samples, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    uint64_t start_ns = now_ns();
    uint64_t start_cycles = now_cycles();

    polyvec_matrix_expand(mat, seed_seed);

    cycle_samples[i] = now_cycles() - start_cycles;
    ns_samples[i] = now_ns() - start_ns;
  }
  print_stats_csv("supplemental", "polyvec_matrix_expand", ns_samples, cycle_samples, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    uint64_t start_ns = now_ns();
    uint64_t start_cycles = now_cycles();

    poly_uniform_eta(a, seed_crh, 0);

    cycle_samples[i] = now_cycles() - start_cycles;
    ns_samples[i] = now_ns() - start_ns;
  }
  print_stats_csv("supplemental", "poly_uniform_eta", ns_samples, cycle_samples, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    uint64_t start_ns = now_ns();
    uint64_t start_cycles = now_cycles();

    poly_uniform_gamma1(a, seed_crh, 0);

    cycle_samples[i] = now_cycles() - start_cycles;
    ns_samples[i] = now_ns() - start_ns;
  }
  print_stats_csv("supplemental", "poly_uniform_gamma1", ns_samples, cycle_samples, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    uint64_t start_ns = now_ns();
    uint64_t start_cycles = now_cycles();

    poly_ntt(a);

    cycle_samples[i] = now_cycles() - start_cycles;
    ns_samples[i] = now_ns() - start_ns;
  }
  print_stats_csv("supplemental", "poly_ntt", ns_samples, cycle_samples, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    uint64_t start_ns = now_ns();
    uint64_t start_cycles = now_cycles();

    poly_invntt_tomont(a);

    cycle_samples[i] = now_cycles() - start_cycles;
    ns_samples[i] = now_ns() - start_ns;
  }
  print_stats_csv("supplemental", "poly_invntt_tomont", ns_samples, cycle_samples, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    uint64_t start_ns = now_ns();
    uint64_t start_cycles = now_cycles();

    poly_pointwise_montgomery(c, a, b);

    cycle_samples[i] = now_cycles() - start_cycles;
    ns_samples[i] = now_ns() - start_ns;
  }
  print_stats_csv("supplemental", "poly_pointwise_montgomery", ns_samples, cycle_samples, NTESTS);

  for(i = 0; i < NTESTS; i++) {
    uint64_t start_ns = now_ns();
    uint64_t start_cycles = now_cycles();

    poly_challenge(c, seed_ct);

    cycle_samples[i] = now_cycles() - start_cycles;
    ns_samples[i] = now_ns() - start_ns;
  }
  print_stats_csv("supplemental", "poly_challenge", ns_samples, cycle_samples, NTESTS);

  return 0;
}
