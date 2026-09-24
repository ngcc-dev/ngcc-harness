#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "params.h"
#include "symmetric.h"

#ifndef SYMMETRIC_BENCH_DEFAULT_LOOP_COUNT
#define SYMMETRIC_BENCH_DEFAULT_LOOP_COUNT 10000
#endif

#if defined(__x86_64__) || defined(__i386__)
static inline uint64_t counter(void)
{
    uint64_t result;
    __asm__ volatile("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax"
                     : "=a"(result)
                     :
                     : "%rdx");
    return result;
}
#else
#error "counter unsupported on this architecture"
#endif

static uint64_t countergap;
static volatile uint64_t sink;

static uint64_t loops_from_env(void)
{
    const char *value = getenv("TEST_LOOP_COUNT");
    char *end = NULL;
    uint64_t loops;

    if (value == NULL || value[0] == '\0')
        return SYMMETRIC_BENCH_DEFAULT_LOOP_COUNT;

    loops = strtoull(value, &end, 10);
    if (end == value || *end != '\0' || loops == 0)
        return SYMMETRIC_BENCH_DEFAULT_LOOP_COUNT;
    return loops;
}

static void setup_counter(uint64_t loops)
{
    countergap = 0;
    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        const uint64_t end = counter();
        countergap += end - start;
    }
    countergap /= loops;
}

static uint64_t adjusted_delta(uint64_t start, uint64_t end)
{
    const uint64_t delta = end - start;
    return delta > countergap ? delta - countergap : 0;
}

static void fill_bytes(uint8_t *buf, size_t len, unsigned seed)
{
    uint32_t s = 0xA5A5A5A5u ^ seed;

    for (size_t i = 0; i < len; i++) {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        buf[i] = (uint8_t)s;
    }
}

static uint64_t checksum_bytes(const uint8_t *buf, size_t len)
{
    uint64_t s = 0;
    for (size_t i = 0; i < len; i += 17)
        s = (s << 5) ^ (s >> 2) ^ buf[i];
    return s;
}

static void print_result(const char *instance, const char *function,
                         uint64_t loops, uint64_t cycles)
{
    printf("%s,%s,%llu,%llu,%llu,%llu\n",
           instance, function,
           (unsigned long long)loops,
           (unsigned long long)countergap,
           (unsigned long long)(cycles / loops),
           (unsigned long long)sink);
}

static uint64_t bench_hash_G(uint64_t loops)
{
    uint8_t pk[NTRE_PUBLICKEYBYTES];
    uint8_t out[NTRE_GBYTES];
    uint64_t cycles = 0;

    fill_bytes(pk, sizeof(pk), 1);
    memset(out, 0, sizeof(out));

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        hash_G(out, pk);
        cycles += adjusted_delta(start, counter());
        pk[i % NTRE_PUBLICKEYBYTES] ^= (uint8_t)(i + 1);
    }

    sink ^= checksum_bytes(out, sizeof(out));
    return cycles;
}

static uint64_t bench_hash_H(uint64_t loops)
{
    uint8_t r[NTRE_RBYTES];
    uint8_t h_pk[NTRE_GBYTES];
    uint8_t out[HASH_H_OUTBYTES];
    uint64_t cycles = 0;

    fill_bytes(r, sizeof(r), 2);
    fill_bytes(h_pk, sizeof(h_pk), 3);
    memset(out, 0, sizeof(out));

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        hash_H(out, r, h_pk);
        cycles += adjusted_delta(start, counter());
        r[i % NTRE_RBYTES] ^= (uint8_t)(i + 5);
    }

    sink ^= checksum_bytes(out, sizeof(out));
    return cycles;
}

static uint64_t bench_hash_F(uint64_t loops)
{
    uint8_t m1[NTRE_M1BYTES];
    uint8_t out[NTRE_RBYTES];
    uint64_t cycles = 0;

    fill_bytes(m1, sizeof(m1), 4);
    memset(out, 0, sizeof(out));

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        hash_F(out, m1);
        cycles += adjusted_delta(start, counter());
        m1[i % NTRE_M1BYTES] ^= (uint8_t)(i + 7);
    }

    sink ^= checksum_bytes(out, sizeof(out));
    return cycles;
}

static uint64_t bench_sample_psi1(uint64_t loops)
{
    uint8_t seed[NTRE_SYMBYTES];
    uint8_t out[NTRE_SAMPLEBYTES];
    uint64_t cycles = 0;

    fill_bytes(seed, sizeof(seed), 5);
    memset(out, 0, sizeof(out));

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        sample_psi1(out, seed);
        cycles += adjusted_delta(start, counter());
        seed[i % NTRE_SYMBYTES] ^= (uint8_t)(i + 11);
    }

    sink ^= checksum_bytes(out, sizeof(out));
    return cycles;
}

int main(void)
{
    const uint64_t loops = loops_from_env();

    setup_counter(loops);
    printf("instance,function,loops,countergap,cycles,sink\n");
    print_result(NTRE_ALGNAME, "hash_G", loops, bench_hash_G(loops));
    print_result(NTRE_ALGNAME, "hash_H", loops, bench_hash_H(loops));
    print_result(NTRE_ALGNAME, "hash_F", loops, bench_hash_F(loops));
    print_result(NTRE_ALGNAME, "sample_psi1", loops, bench_sample_psi1(loops));
    return 0;
}
