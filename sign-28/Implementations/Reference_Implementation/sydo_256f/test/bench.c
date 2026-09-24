#include "drng.h"
#include "SIG_AlgorithmInstance.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef WARMUP_ROUNDS
#define WARMUP_ROUNDS 50
#endif
#ifndef MEASURE_ROUNDS
#define MEASURE_ROUNDS 200
#endif

DRNG_ctx drng_algorithm;

static int init_bench_drng(void) {
  unsigned char seed[64] = {0};
  for (size_t i = 0; i < sizeof(seed); ++i) {
    seed[i] = (unsigned char)(0xa5u ^ (unsigned int)i);
  }
  return init_random_number(&drng_algorithm, seed, sizeof(seed));
}

#if !defined(BENCH_PORTABLE_ISO_C) && (defined(__x86_64__) || defined(__i386__))
#define BENCH_HAS_X86_TSC 1
static uint64_t cpucycles_start(void) {
  unsigned hi;
  unsigned lo;
  /* Benchmark-only x86 TSC reader. It is not used by the SYDO algorithm implementation. */
  __asm__ __volatile__("CPUID\n\t"
                       "RDTSC\n\t"
                       "mov %%edx, %0\n\t"
                       "mov %%eax, %1\n\t"
                       : "=r"(hi), "=r"(lo)
                       :
                       : "%rax", "%rbx", "%rcx", "%rdx");
  return ((uint64_t)lo) ^ (((uint64_t)hi) << 32);
}

static uint64_t cpucycles_stop(void) {
  unsigned hi;
  unsigned lo;
  /* Benchmark-only x86 TSC reader. It is not used by the SYDO algorithm implementation. */
  __asm__ __volatile__("RDTSCP\n\t"
                       "mov %%edx, %0\n\t"
                       "mov %%eax, %1\n\t"
                       "CPUID\n\t"
                       : "=r"(hi), "=r"(lo)
                       :
                       : "%rax", "%rbx", "%rcx", "%rdx");
  return ((uint64_t)lo) ^ (((uint64_t)hi) << 32);
}
#else
#define BENCH_HAS_X86_TSC 0
static uint64_t cpucycles_start(void) {
  return 0;
}

static uint64_t cpucycles_stop(void) {
  return 0;
}
#endif

#ifndef BENCH_CPU_HZ
#define BENCH_CPU_HZ 0ull
#endif

static uint64_t now_ticks(void) {
  return (uint64_t)clock();
}

static double ticks_to_us(uint64_t ticks) {
  return ((double)ticks * 1000000.0) / (double)CLOCKS_PER_SEC;
}

static uint64_t ticks_to_estimated_cycles(uint64_t ticks) {
  return BENCH_CPU_HZ ? (uint64_t)((ticks_to_us(ticks) * (double)BENCH_CPU_HZ) / 1000000.0) : 0;
}

int main(void) {
  const unsigned char message[] = "SYndrome DecOding (SYDO) signature scheme";
  const unsigned long long message_len = sizeof(message) - 1;
  const unsigned long long pk_len = sig_get_pk_len_bytes();
  const unsigned long long sk_len = sig_get_sk_len_bytes();
  const unsigned long long sig_len = sig_get_sn_len_bytes();
  unsigned char* pk = (unsigned char*)calloc(pk_len, 1);
  unsigned char* sk = (unsigned char*)calloc(sk_len, 1);
  unsigned char* sig = (unsigned char*)calloc(sig_len, 1);
  uint64_t elapsed_ticks = 0;
  uint64_t elapsed_cycles = 0;
  uint64_t keygen_cycles = 0;
  double keygen_us = 0.0;
  uint64_t sign_cycles = 0;
  double sign_us = 0.0;
  uint64_t verify_cycles = 0;
  double verify_us = 0.0;
  int failures = 0;

  if (!pk || !sk || !sig) {
    fprintf(stderr, "allocation failed\n");
    free(pk);
    free(sk);
    free(sig);
    return 1;
  }

  if (init_bench_drng() != 0) {
    fprintf(stderr, "drng initialization failed\n");
    free(pk);
    free(sk);
    free(sig);
    return 1;
  }

  for (int i = 0; i < WARMUP_ROUNDS; ++i) {
    unsigned long long cur_pk_len = pk_len;
    unsigned long long cur_sk_len = sk_len;
    if (sig_keygen(pk, &cur_pk_len, sk, &cur_sk_len) != 0) {
      fprintf(stderr, "sig_keygen failed during warmup\n");
      free(pk);
      free(sk);
      free(sig);
      return 1;
    }
  }

  elapsed_ticks = 0;
  elapsed_cycles = 0;
  for (int i = 0; i < MEASURE_ROUNDS; ++i) {
    unsigned long long cur_pk_len = pk_len;
    unsigned long long cur_sk_len = sk_len;
    const uint64_t c1 = cpucycles_start();
    const uint64_t t1 = now_ticks();
    if (sig_keygen(pk, &cur_pk_len, sk, &cur_sk_len) != 0) {
      fprintf(stderr, "sig_keygen failed\n");
      free(pk);
      free(sk);
      free(sig);
      return 1;
    }
    elapsed_ticks += now_ticks() - t1;
    if (BENCH_HAS_X86_TSC) {
      elapsed_cycles += cpucycles_stop() - c1;
    }
  }
  if (!BENCH_HAS_X86_TSC) {
    elapsed_cycles = ticks_to_estimated_cycles(elapsed_ticks);
  }
  keygen_cycles = elapsed_cycles / (uint64_t)MEASURE_ROUNDS;
  keygen_us = ticks_to_us(elapsed_ticks) / (double)MEASURE_ROUNDS;

  {
    unsigned long long cur_pk_len = pk_len;
    unsigned long long cur_sk_len = sk_len;
    if (sig_keygen(pk, &cur_pk_len, sk, &cur_sk_len) != 0) {
      fprintf(stderr, "sig_keygen failed before sign benchmark\n");
      free(pk);
      free(sk);
      free(sig);
      return 1;
    }
  }
  for (int i = 0; i < WARMUP_ROUNDS; ++i) {
    unsigned long long cur_sig_len = sig_len;
    if (sig_sign(sk, sk_len, (unsigned char*)message, message_len, sig, &cur_sig_len) != 0) {
      fprintf(stderr, "sig_sign failed during warmup\n");
      free(pk);
      free(sk);
      free(sig);
      return 1;
    }
  }
  elapsed_ticks = 0;
  elapsed_cycles = 0;
  for (int i = 0; i < MEASURE_ROUNDS; ++i) {
    unsigned long long cur_sig_len = sig_len;
    const uint64_t c1 = cpucycles_start();
    const uint64_t t1 = now_ticks();
    if (sig_sign(sk, sk_len, (unsigned char*)message, message_len, sig, &cur_sig_len) != 0) {
      fprintf(stderr, "sig_sign failed\n");
      free(pk);
      free(sk);
      free(sig);
      return 1;
    }
    elapsed_ticks += now_ticks() - t1;
    if (BENCH_HAS_X86_TSC) {
      elapsed_cycles += cpucycles_stop() - c1;
    }
  }
  if (!BENCH_HAS_X86_TSC) {
    elapsed_cycles = ticks_to_estimated_cycles(elapsed_ticks);
  }
  sign_cycles = elapsed_cycles / (uint64_t)MEASURE_ROUNDS;
  sign_us = ticks_to_us(elapsed_ticks) / (double)MEASURE_ROUNDS;

  {
    unsigned long long cur_pk_len = pk_len;
    unsigned long long cur_sk_len = sk_len;
    unsigned long long cur_sig_len = sig_len;
    if (sig_keygen(pk, &cur_pk_len, sk, &cur_sk_len) != 0 ||
        sig_sign(sk, sk_len, (unsigned char*)message, message_len, sig, &cur_sig_len) != 0) {
      fprintf(stderr, "setup failed before verify benchmark\n");
      free(pk);
      free(sk);
      free(sig);
      return 1;
    }
    for (int i = 0; i < WARMUP_ROUNDS; ++i) {
      if (sig_verify(pk, pk_len, sig, cur_sig_len, (unsigned char*)message, message_len) != 0) {
        failures++;
      }
    }
    elapsed_ticks = 0;
    elapsed_cycles = 0;
    for (int i = 0; i < MEASURE_ROUNDS; ++i) {
      const uint64_t c1 = cpucycles_start();
      const uint64_t t1 = now_ticks();
      if (sig_verify(pk, pk_len, sig, cur_sig_len, (unsigned char*)message, message_len) != 0) {
        failures++;
      }
      elapsed_ticks += now_ticks() - t1;
      if (BENCH_HAS_X86_TSC) {
        elapsed_cycles += cpucycles_stop() - c1;
      }
    }
    if (!BENCH_HAS_X86_TSC) {
      elapsed_cycles = ticks_to_estimated_cycles(elapsed_ticks);
    }
  }
  verify_cycles = elapsed_cycles / (uint64_t)MEASURE_ROUNDS;
  verify_us = ticks_to_us(elapsed_ticks) / (double)MEASURE_ROUNDS;

  printf("instance      : %s\n", ALGORITHM_INSTANCE);
  printf("measure rounds: %d\n", MEASURE_ROUNDS);
  printf("cycle source  : %s\n",
         BENCH_HAS_X86_TSC ? "x86 TSC" : (BENCH_CPU_HZ ? "estimated from BENCH_CPU_HZ" : "unavailable"));
  printf("failures      : %d\n", failures);
  printf("keygen_us     : %.2f\n", keygen_us);
  printf("keygen_cycles : %llu\n", (unsigned long long)keygen_cycles);
  printf("keygen_mcycles: %.3f\n", (double)keygen_cycles / 1000000.0);
  printf("sign_us       : %.2f\n", sign_us);
  printf("sign_cycles   : %llu\n", (unsigned long long)sign_cycles);
  printf("sign_mcycles  : %.3f\n", (double)sign_cycles / 1000000.0);
  printf("verify_us     : %.2f\n", verify_us);
  printf("verify_cycles : %llu\n", (unsigned long long)verify_cycles);
  printf("verify_mcycles: %.3f\n", (double)verify_cycles / 1000000.0);
  printf("pk_bytes      : %llu\n", pk_len);
  printf("sk_bytes      : %llu\n", sk_len);
  printf("sig_bytes     : %llu\n", sig_len);

  free(pk);
  free(sk);
  free(sig);
  return failures == 0 ? 0 : 1;
}
