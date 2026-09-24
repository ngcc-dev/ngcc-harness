#if defined(__APPLE__)
#define _DARWIN_C_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>

#ifndef NGCC_HASH_BITS
#define NGCC_HASH_BITS 512
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
#define NGCC_TARGET_PLATFORM "aarch64"
#define NGCC_TARGET_VARIANT "ARM-SHA3"
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386) ||            \
    defined(_M_IX86)
#define NGCC_TARGET_PLATFORM "x86"
#if defined(__AVX512F__) && defined(__AVX512VL__)
#define NGCC_TARGET_VARIANT "x86-AVX512"
#elif defined(__AVX2__)
#define NGCC_TARGET_VARIANT "x86-AVX2-asm"
#else
#define NGCC_TARGET_VARIANT "x86"
#endif
#else
#define NGCC_TARGET_PLATFORM "generic"
#define NGCC_TARGET_VARIANT "generic"
#endif

#ifndef NGCC_HASH_HEADER
#if NGCC_HASH_BITS == 512
#if defined(__aarch64__) || defined(_M_ARM64)
#define NGCC_HASH_HEADER                                                       \
  "Cuishen-512/CryptHash_Cuishen-512.h"
#elif defined(__AVX512F__) && defined(__AVX512VL__)
#define NGCC_HASH_HEADER                                                       \
  "Cuishen-512/CryptHash_Cuishen-512.h"
#else
#define NGCC_HASH_HEADER                                                       \
  "Cuishen-512/CryptHash_Cuishen-512.h"
#endif
#elif NGCC_HASH_BITS == 768
#if defined(__aarch64__) || defined(_M_ARM64)
#define NGCC_HASH_HEADER                                                       \
  "Cuishen-768/CryptHash_Cuishen-768.h"
#elif defined(__AVX512F__) && defined(__AVX512VL__)
#define NGCC_HASH_HEADER                                                       \
  "Cuishen-768/CryptHash_Cuishen-768.h"
#else
#define NGCC_HASH_HEADER                                                       \
  "Cuishen-768/CryptHash_Cuishen-768.h"
#endif
#elif NGCC_HASH_BITS == 1024
#if defined(__aarch64__) || defined(_M_ARM64)
#define NGCC_HASH_HEADER                                                       \
  "Cuishen-1024/CryptHash_Cuishen-1024.h"
#elif defined(__AVX512F__) && defined(__AVX512VL__)
#define NGCC_HASH_HEADER                                                       \
  "Cuishen-1024/CryptHash_Cuishen-1024.h"
#else
#define NGCC_HASH_HEADER                                                       \
  "Cuishen-1024/CryptHash_Cuishen-1024.h"
#endif
#else
#error "NGCC_HASH_BITS must be one of 512, 768, or 1024."
#endif
#endif
#include NGCC_HASH_HEADER

#if DIGEST_BIT_LENGTH != NGCC_HASH_BITS
#error "NGCC_HASH_BITS does not match DIGEST_BIT_LENGTH from NGCC_HASH_HEADER."
#endif

#define DIGEST_BYTES (DIGEST_BIT_LENGTH / 8)
#define DEFAULT_MIN_ITERATIONS 100ULL
#define DEFAULT_MIN_SECONDS 0.25
#define INPUT_SEED 0x4e474343U

typedef struct {
  const char *level;
  unsigned long long bytes;
} bench_case;

typedef struct {
  const char *level;
  unsigned long long bytes;
  unsigned long long bits;
  unsigned long long iterations;
  double elapsed_seconds;
  double avg_ns;
  double avg_cycles;
  const char *cycle_source;
  double throughput_mbps;
  double throughput_mibps;
  uint64_t peak_rss_bytes;
  unsigned int digest_xor;
} bench_result;

static volatile unsigned int benchmark_sink = 0;

static uint64_t now_ns(void) {
  struct timespec ts;
#ifdef CLOCK_MONOTONIC_RAW
  const clockid_t clock_id = CLOCK_MONOTONIC_RAW;
#else
  const clockid_t clock_id = CLOCK_MONOTONIC;
#endif

  if (clock_gettime(clock_id, &ts) != 0) {
    perror("clock_gettime");
    exit(2);
  }

  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static int read_arch_counter(uint64_t *value) {
#if defined(__aarch64__)
  __asm__ __volatile__("isb\n\tmrs %0, cntvct_el0" : "=r"(*value));
  return 1;
#elif defined(__x86_64__) || defined(__i386)
  uint32_t lo;
  uint32_t hi;
  __asm__ __volatile__("lfence\n\trdtsc"
                       : "=a"(lo), "=d"(hi)
                       :
                       : "memory");
  *value = ((uint64_t)hi << 32) | (uint64_t)lo;
  return 1;
#else
  (void)value;
  return 0;
#endif
}

static const char *arch_counter_source(void) {
#if defined(__aarch64__)
  return "cntvct_el0_ticks";
#elif defined(__x86_64__) || defined(__i386)
  return "rdtsc_ticks";
#else
  return "unavailable";
#endif
}

static uint64_t current_peak_rss_bytes(void) {
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage) != 0) {
    return 0;
  }
#if defined(__APPLE__)
  return (uint64_t)usage.ru_maxrss;
#else
  if (usage.ru_maxrss > 0) {
    return (uint64_t)usage.ru_maxrss * 1024ULL;
  }

  {
    FILE *fp = fopen("/proc/self/status", "r");
    char line[128];
    unsigned long long kb = 0;

    if (fp == NULL) {
      return 0;
    }
    while (fgets(line, sizeof(line), fp) != NULL) {
      if (sscanf(line, "VmHWM: %llu kB", &kb) == 1) {
        break;
      }
    }
    fclose(fp);
    return kb * 1024ULL;
  }
#endif
}

static void fill_message(unsigned char *msg, size_t len) {
  size_t i;
  uint32_t x = INPUT_SEED;

  for (i = 0; i < len; ++i) {
    x = x * 1664525U + 1013904223U;
    msg[i] = (unsigned char)(x >> 24);
  }
}

static int run_hash_loop(const unsigned char *msg,
                         unsigned long long msg_len_bits,
                         unsigned long long iterations,
                         unsigned int *digest_xor) {
  unsigned char digest[DIGEST_BYTES];
  unsigned long long i;

  memset(digest, 0, sizeof(digest));
  for (i = 0; i < iterations; ++i) {
    int rc = CryptHash(DIGEST_BIT_LENGTH, msg, msg_len_bits, digest);
    if (rc != 0) {
      fprintf(stderr, "CryptHash failed with %d\n", rc);
      return 1;
    }
    benchmark_sink ^= digest[i % DIGEST_BYTES];
  }

  *digest_xor = benchmark_sink;
  return 0;
}

static int benchmark_one(const bench_case *c, const unsigned char *msg,
                         unsigned long long min_iterations,
                         double min_seconds, double cpu_hz,
                         bench_result *result) {
  unsigned long long iterations = min_iterations;
  unsigned int digest_xor = 0;
  uint64_t start_ns;
  uint64_t end_ns;
  uint64_t start_counter = 0;
  uint64_t end_counter = 0;
  int have_counter = 0;
  double elapsed;

  if (run_hash_loop(msg, c->bytes * 8ULL, 8ULL, &digest_xor) != 0) {
    return 1;
  }

  for (;;) {
    have_counter = read_arch_counter(&start_counter);
    start_ns = now_ns();
    if (run_hash_loop(msg, c->bytes * 8ULL, iterations, &digest_xor) != 0) {
      return 1;
    }
    end_ns = now_ns();
    if (have_counter) {
      (void)read_arch_counter(&end_counter);
    }

    elapsed = (double)(end_ns - start_ns) / 1000000000.0;
    if (elapsed >= min_seconds || iterations > (1ULL << 62)) {
      break;
    }
    iterations *= 2ULL;
  }

  result->level = c->level;
  result->bytes = c->bytes;
  result->bits = c->bytes * 8ULL;
  result->iterations = iterations;
  result->elapsed_seconds = elapsed;
  result->avg_ns = elapsed * 1000000000.0 / (double)iterations;
  if (cpu_hz > 0.0) {
    result->avg_cycles = elapsed * cpu_hz / (double)iterations;
    result->cycle_source = "cpu_hz";
  } else if (have_counter) {
    result->avg_cycles =
        (double)(end_counter - start_counter) / (double)iterations;
    result->cycle_source = arch_counter_source();
  } else {
    result->avg_cycles = 0.0;
    result->cycle_source = "unavailable";
  }
  result->throughput_mbps =
      ((double)c->bytes * (double)iterations) / elapsed / 1000000.0;
  result->throughput_mibps =
      ((double)c->bytes * (double)iterations) / elapsed / 1048576.0;
  result->peak_rss_bytes = current_peak_rss_bytes();
  result->digest_xor = digest_xor;

  return 0;
}

static void usage(const char *program) {
  fprintf(stderr,
          "Usage: %s [--param-name NAME] [--min-iterations N]\n"
          "          [--min-seconds SEC] [--cpu-hz HZ]\n"
          "          [--csv PATH] [--json PATH]\n"
          "\n"
          "Compile-time options:\n"
          "  -DNGCC_HASH_BITS=512|768|1024\n"
          "  -DNGCC_HASH_HEADER='\"path/to/CryptHash_Cuishen-*.h\"'\n",
          program);
}

static int parse_ull(const char *s, unsigned long long *out) {
  char *end = NULL;
  errno = 0;
  *out = strtoull(s, &end, 10);
  return errno == 0 && end != s && *end == '\0';
}

static int parse_double(const char *s, double *out) {
  char *end = NULL;
  errno = 0;
  *out = strtod(s, &end);
  return errno == 0 && end != s && *end == '\0';
}

static int write_csv(const char *path, const char *param_name,
                     const bench_result *results, size_t result_count) {
  FILE *fp = fopen(path, "w");
  size_t i;

  if (fp == NULL) {
    perror(path);
    return 1;
  }

  fprintf(fp,
          "algorithm,target_platform,target_variant,level,message_bytes,"
          "message_bits,iterations,"
          "elapsed_seconds,avg_ns,avg_cycles,cycle_source,"
          "throughput_MBps,throughput_MiBps,peak_rss_bytes,digest_xor\n");
  for (i = 0; i < result_count; ++i) {
    const bench_result *r = &results[i];
    fprintf(fp,
            "%s,%s,%s,%s,%llu,%llu,%llu,%.9f,%.3f,%.3f,%s,%.6f,%.6f,"
            "%" PRIu64 ",%u\n",
            param_name, NGCC_TARGET_PLATFORM, NGCC_TARGET_VARIANT, r->level,
            r->bytes, r->bits, r->iterations, r->elapsed_seconds, r->avg_ns,
            r->avg_cycles, r->cycle_source, r->throughput_mbps,
            r->throughput_mibps, r->peak_rss_bytes, r->digest_xor);
  }

  if (fclose(fp) != 0) {
    perror(path);
    return 1;
  }
  return 0;
}

static int write_json(const char *path, const char *param_name,
                      unsigned long long min_iterations, double min_seconds,
                      double cpu_hz, const bench_result *results,
                      size_t result_count) {
  FILE *fp = fopen(path, "w");
  size_t i;

  if (fp == NULL) {
    perror(path);
    return 1;
  }

  fprintf(fp, "{\n");
  fprintf(fp, "  \"algorithm\": \"%s\",\n", param_name);
  fprintf(fp, "  \"digest_bits\": %d,\n", DIGEST_BIT_LENGTH);
  fprintf(fp, "  \"target_platform\": \"%s\",\n", NGCC_TARGET_PLATFORM);
  fprintf(fp, "  \"target_variant\": \"%s\",\n", NGCC_TARGET_VARIANT);
  fprintf(fp, "  \"interface\": \"CryptHash\",\n");
  fprintf(fp, "  \"min_iterations\": %llu,\n", min_iterations);
  fprintf(fp, "  \"min_seconds_per_case\": %.9f,\n", min_seconds);
  if (cpu_hz > 0.0) {
    fprintf(fp, "  \"cpu_hz\": %.3f,\n", cpu_hz);
  } else {
    fprintf(fp, "  \"cpu_hz\": null,\n");
  }
  fprintf(fp,
          "  \"input_generation\": {\"method\": \"LCG\", "
          "\"seed_hex\": \"0x%08x\", \"max_bytes\": 65536},\n",
          INPUT_SEED);
  fprintf(fp, "  \"cases\": [\n");
  for (i = 0; i < result_count; ++i) {
    const bench_result *r = &results[i];
    fprintf(fp,
            "    {\"level\": \"%s\", \"message_bytes\": %llu, "
            "\"message_bits\": %llu, \"iterations\": %llu, "
            "\"elapsed_seconds\": %.9f, \"avg_ns\": %.3f, "
            "\"avg_cycles\": %.3f, \"cycle_source\": \"%s\", "
            "\"throughput_MBps\": %.6f, \"throughput_MiBps\": %.6f, "
            "\"peak_rss_bytes\": %" PRIu64 ", \"digest_xor\": %u}%s\n",
            r->level, r->bytes, r->bits, r->iterations, r->elapsed_seconds,
            r->avg_ns, r->avg_cycles, r->cycle_source, r->throughput_mbps,
            r->throughput_mibps, r->peak_rss_bytes, r->digest_xor,
            i + 1U == result_count ? "" : ",");
  }
  fprintf(fp, "  ]\n");
  fprintf(fp, "}\n");

  if (fclose(fp) != 0) {
    perror(path);
    return 1;
  }
  return 0;
}

int main(int argc, char **argv) {
  static const bench_case cases[] = {
      {"S1", 32ULL},    {"S2", 128ULL},   {"S3", 512ULL},
      {"S4", 1024ULL},  {"S5", 4096ULL},  {"S6", 8192ULL},
      {"S7", 16384ULL}, {"S8", 65536ULL},
  };
  const size_t case_count = sizeof(cases) / sizeof(cases[0]);
  bench_result results[sizeof(cases) / sizeof(cases[0])];
  const char *param_name = ALGORITHM_INSTANCE;
  const char *csv_path = NULL;
  const char *json_path = NULL;
  unsigned long long min_iterations = DEFAULT_MIN_ITERATIONS;
  double min_seconds = DEFAULT_MIN_SECONDS;
  double cpu_hz = 0.0;
  unsigned char *msg;
  size_t i;

  for (i = 1; i < (size_t)argc; ++i) {
    const char *arg = argv[i];
    if (strcmp(arg, "--param-name") == 0 && i + 1U < (size_t)argc) {
      param_name = argv[++i];
    } else if (strcmp(arg, "--min-iterations") == 0 &&
               i + 1U < (size_t)argc) {
      if (!parse_ull(argv[++i], &min_iterations) || min_iterations < 100ULL) {
        fprintf(stderr, "--min-iterations must be an integer >= 100\n");
        return 1;
      }
    } else if (strcmp(arg, "--min-seconds") == 0 &&
               i + 1U < (size_t)argc) {
      if (!parse_double(argv[++i], &min_seconds) || min_seconds <= 0.0) {
        fprintf(stderr, "--min-seconds must be positive\n");
        return 1;
      }
    } else if (strcmp(arg, "--cpu-hz") == 0 && i + 1U < (size_t)argc) {
      if (!parse_double(argv[++i], &cpu_hz) || cpu_hz <= 0.0) {
        fprintf(stderr, "--cpu-hz must be positive\n");
        return 1;
      }
    } else if (strcmp(arg, "--csv") == 0 && i + 1U < (size_t)argc) {
      csv_path = argv[++i];
    } else if (strcmp(arg, "--json") == 0 && i + 1U < (size_t)argc) {
      json_path = argv[++i];
    } else if (strcmp(arg, "--help") == 0) {
      usage(argv[0]);
      return 0;
    } else {
      usage(argv[0]);
      return 1;
    }
  }

  msg = (unsigned char *)malloc(65536U);
  if (msg == NULL) {
    fprintf(stderr, "Failed to allocate message buffer\n");
    return 1;
  }
  fill_message(msg, 65536U);

  printf("%s NGCC hash benchmark\n", param_name);
  printf("target=%s variant=%s digest_bits=%d min_iterations=%llu "
         "min_seconds=%.3f\n",
         NGCC_TARGET_PLATFORM, NGCC_TARGET_VARIANT, DIGEST_BIT_LENGTH,
         min_iterations, min_seconds);
  printf("%-4s %12s %12s %12s %12s %16s %12s\n", "case", "bytes",
         "iters", "avg_ns", "avg_cycles", "MB/s", "peak_rss");

  for (i = 0; i < case_count; ++i) {
    if (benchmark_one(&cases[i], msg, min_iterations, min_seconds, cpu_hz,
                      &results[i]) != 0) {
      free(msg);
      return 1;
    }
    printf("%-4s %12llu %12llu %12.2f %12.2f %16.2f %12" PRIu64 "\n",
           results[i].level, results[i].bytes, results[i].iterations,
           results[i].avg_ns, results[i].avg_cycles, results[i].throughput_mbps,
           results[i].peak_rss_bytes);
  }
  printf("sink=%u\n", benchmark_sink);

  if (csv_path != NULL && write_csv(csv_path, param_name, results, case_count)) {
    free(msg);
    return 1;
  }
  if (json_path != NULL &&
      write_json(json_path, param_name, min_iterations, min_seconds, cpu_hz,
                 results, case_count)) {
    free(msg);
    return 1;
  }

  free(msg);
  return 0;
}
