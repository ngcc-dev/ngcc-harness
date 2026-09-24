#include "bench_cycles.h"
#include "fields.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef FIELD_BENCH_ITERS
#define FIELD_BENCH_ITERS 1000000u
#endif

#define BENCH_POOL 256u

static uint64_t rng_state = UINT64_C(0x123456789abcdef0);

static uint64_t rng64(void) {
  uint64_t x = rng_state;
  x ^= x << 7;
  x ^= x >> 9;
  x ^= x << 8;
  rng_state = x;
  return x;
}

static bf256_t random_bf256(void) {
  const bf256_t ret = BF256C(rng64(), rng64(), rng64(), rng64());
  return ret;
}

static int bf256_equal(bf256_t lhs, bf256_t rhs) {
  return lhs.values[0] == rhs.values[0] && lhs.values[1] == rhs.values[1] &&
         lhs.values[2] == rhs.values[2] && lhs.values[3] == rhs.values[3];
}

static int selftest_bf256(void) {
  for (unsigned int i = 0; i != 1024; ++i) {
    const bf256_t a = random_bf256();
    if (!bf256_equal(bf256_mul(a, bf256_one()), a) ||
        !bf256_equal(bf256_mul(bf256_one(), a), a) ||
        !bf256_equal(bf256_mul(a, bf256_zero()), bf256_zero())) {
      fprintf(stderr, "bf256 selftest failed at vector %u\n", i);
      return -1;
    }
  }
  return 0;
}

static double bench_bf256(size_t iters, bf256_t* digest) {
  bf256_t a[BENCH_POOL], b[BENCH_POOL];
  for (unsigned int i = 0; i != BENCH_POOL; ++i) {
    a[i] = random_bf256();
    b[i] = random_bf256();
  }

  bf256_t acc = bf256_one();
  const uint64_t begin = bench_read_cycles();
  for (size_t i = 0; i != iters; ++i) {
    const unsigned int idx = (unsigned int)i & (BENCH_POOL - 1u);
    acc = bf256_mul(bf256_add(a[idx], acc), b[idx]);
  }
  const uint64_t end = bench_read_cycles();
  *digest = acc;
  return (double)(end - begin) / (double)iters;
}

static double bench_bf256_x4(size_t iters, bf256_t* digest) {
  bf256_t a[BENCH_POOL], b[BENCH_POOL];
  for (unsigned int i = 0; i != BENCH_POOL; ++i) {
    a[i] = random_bf256();
    b[i] = random_bf256();
  }

  bf256_t acc[4] = {bf256_one(), bf256_one(), bf256_one(), bf256_one()};
  const uint64_t begin = bench_read_cycles();
  for (size_t i = 0; i != iters; ++i) {
    for (unsigned int lane = 0; lane != 4; ++lane) {
      const unsigned int idx = ((unsigned int)i * 4u + lane) & (BENCH_POOL - 1u);
      acc[lane] = bf256_mul(bf256_add(a[idx], acc[lane]), b[idx]);
    }
  }
  const uint64_t end = bench_read_cycles();
  *digest = bf256_add(bf256_add(acc[0], acc[1]), bf256_add(acc[2], acc[3]));
  return (double)(end - begin) / (double)(iters * 4u);
}

int main(int argc, char** argv) {
  const size_t iters = argc > 1 ? (size_t)strtoull(argv[1], NULL, 10) : FIELD_BENCH_ITERS;
  if (iters == 0) {
    fprintf(stderr, "usage: %s [iterations]\n", argv[0]);
    return 1;
  }

  if (selftest_bf256() != 0) {
    return 1;
  }

  bf256_t digest;
  bf256_t digest_x4;
  const double c256 = bench_bf256(iters, &digest);
  const double c256_x4 = bench_bf256_x4(iters, &digest_x4);

  printf("field_mul_bench: ublockith optimized\n");
  printf("iterations: %zu\n", iters);
  printf("cycle source: %s\n", bench_cycles_source());
  printf("bf256_mul cycles/op: %.2f digest=%016" PRIx64 "%016" PRIx64
         "%016" PRIx64 "%016" PRIx64 "\n",
         c256, digest.values[3], digest.values[2], digest.values[1], digest.values[0]);
  printf("bf256_mul x4 throughput cycles/op: %.2f digest=%016" PRIx64 "%016" PRIx64
         "%016" PRIx64 "%016" PRIx64 "\n",
         c256_x4, digest_x4.values[3], digest_x4.values[2], digest_x4.values[1],
         digest_x4.values[0]);
  return 0;
}
