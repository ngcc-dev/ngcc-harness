#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "params.h"
#include "poly.h"

#ifndef POLY_BENCH_DEFAULT_LOOP_COUNT
#define POLY_BENCH_DEFAULT_LOOP_COUNT 10000
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
        return POLY_BENCH_DEFAULT_LOOP_COUNT;

    loops = strtoull(value, &end, 10);
    if (end == value || *end != '\0' || loops == 0)
        return POLY_BENCH_DEFAULT_LOOP_COUNT;
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

static void fill_poly(poly *a, int seed)
{
    for (size_t i = 0; i < NTRE_N; i++)
        a->coeffs[i] = (int16_t)((seed + 17 * (int)i + 3 * (int)(i >> 3)) % NTRE_Q);
}

static void fill_unit_blocks(poly *a)
{
    memset(a, 0, sizeof(*a));
    for (size_t i = 0; i < NTRE_N; i += NTRE_D)
        a->coeffs[i] = 1;
}

static void fill_bytes(uint8_t *buf, size_t len, unsigned seed)
{
    uint32_t s = 0x9E3779B9u ^ seed;

    for (size_t i = 0; i < len; i++) {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        buf[i] = (uint8_t)s;
    }
}

static uint64_t checksum_poly(const poly *a)
{
    uint64_t s = 0;
    for (size_t i = 0; i < NTRE_N; i += 17)
        s = (s << 7) ^ (s >> 3) ^ (uint16_t)a->coeffs[i];
    return s;
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

static uint64_t bench_poly_tobytes(uint64_t loops)
{
    poly a;
    uint8_t bytes[NTRE_POLYBYTES];
    uint64_t cycles = 0;

    fill_poly(&a, 1);
    memset(bytes, 0, sizeof(bytes));

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        poly_tobytes(bytes, &a);
        cycles += adjusted_delta(start, counter());
        a.coeffs[i % NTRE_N] = (int16_t)(a.coeffs[i % NTRE_N] + 1);
    }

    sink ^= checksum_bytes(bytes, sizeof(bytes));
    return cycles;
}

static uint64_t bench_poly_frombytes(uint64_t loops)
{
    poly a;
    uint8_t bytes[NTRE_POLYBYTES];
    uint64_t cycles = 0;

    fill_bytes(bytes, sizeof(bytes), 2);
    memset(&a, 0, sizeof(a));

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        poly_frombytes(&a, bytes);
        cycles += adjusted_delta(start, counter());
        bytes[i % NTRE_POLYBYTES] ^= (uint8_t)i;
    }

    sink ^= checksum_poly(&a);
    return cycles;
}

static uint64_t bench_poly_cbd1(uint64_t loops)
{
    poly a;
    uint8_t bytes[NTRE_SAMPLEBYTES];
    uint64_t cycles = 0;

    fill_bytes(bytes, sizeof(bytes), 3);
    memset(&a, 0, sizeof(a));

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        poly_cbd1(&a, bytes);
        cycles += adjusted_delta(start, counter());
        bytes[i % NTRE_SAMPLEBYTES] ^= (uint8_t)(i + 1);
    }

    sink ^= checksum_poly(&a);
    return cycles;
}

static uint64_t bench_poly_cbd1_prime(uint64_t loops)
{
    poly a;
    uint8_t msg[NTRE_MSGBYTES];
    uint8_t coins[NTRE_ERROR_RANDOMBYTES];
    uint64_t cycles = 0;

    fill_bytes(msg, sizeof(msg), 4);
    fill_bytes(coins, sizeof(coins), 5);
    memset(&a, 0, sizeof(a));

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        poly_cbd1_prime(&a, msg, coins);
        cycles += adjusted_delta(start, counter());
        msg[i % NTRE_MSGBYTES] ^= (uint8_t)(i + 3);
    }

    sink ^= checksum_poly(&a);
    return cycles;
}

static uint64_t bench_poly_mod2(uint64_t loops)
{
    poly a;
    uint8_t msg[NTRE_MSGBYTES];
    uint64_t cycles = 0;

    fill_poly(&a, 6);
    memset(msg, 0, sizeof(msg));

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        poly_msg_mod2_to_bytes(msg, &a);
        cycles += adjusted_delta(start, counter());
        a.coeffs[i % NTRE_N] = (int16_t)(a.coeffs[i % NTRE_N] + 1);
    }

    sink ^= checksum_bytes(msg, sizeof(msg));
    return cycles;
}

static uint64_t bench_poly_ntt(uint64_t loops)
{
    poly a;
    uint64_t cycles = 0;

    fill_poly(&a, 7);

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        poly_ntt(&a);
        cycles += adjusted_delta(start, counter());
    }

    sink ^= checksum_poly(&a);
    return cycles;
}

static uint64_t bench_poly_invntt(uint64_t loops)
{
    poly a;
    uint64_t cycles = 0;

    fill_poly(&a, 8);
    poly_ntt(&a);

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        poly_invntt(&a);
        cycles += adjusted_delta(start, counter());
    }

    sink ^= checksum_poly(&a);
    return cycles;
}

static uint64_t bench_poly_baseinv(uint64_t loops)
{
    poly a;
    poly r;
    uint64_t cycles = 0;
    int failures = 0;

    fill_unit_blocks(&a);
    memset(&r, 0, sizeof(r));

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        failures |= poly_baseinv(&r, &a);
        cycles += adjusted_delta(start, counter());
    }

    sink ^= checksum_poly(&r) ^ (uint64_t)failures;
    return cycles;
}

static uint64_t bench_poly_basemul(uint64_t loops)
{
    poly a;
    poly b;
    poly r;
    uint64_t cycles = 0;

    fill_poly(&a, 9);
    fill_poly(&b, 10);
    memset(&r, 0, sizeof(r));

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        poly_basemul(&r, &a, &b);
        cycles += adjusted_delta(start, counter());
        a.coeffs[i % NTRE_N] = (int16_t)(a.coeffs[i % NTRE_N] + 1);
    }

    sink ^= checksum_poly(&r);
    return cycles;
}

static uint64_t bench_poly_basemul_add(uint64_t loops)
{
    poly a;
    poly b;
    poly c;
    poly r;
    uint64_t cycles = 0;

    fill_poly(&a, 11);
    fill_poly(&b, 12);
    fill_poly(&c, 13);
    memset(&r, 0, sizeof(r));

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        poly_basemul_add(&r, &a, &b, &c);
        cycles += adjusted_delta(start, counter());
        b.coeffs[i % NTRE_N] = (int16_t)(b.coeffs[i % NTRE_N] + 1);
    }

    sink ^= checksum_poly(&r);
    return cycles;
}

static uint64_t bench_poly_sub(uint64_t loops)
{
    poly a;
    poly b;
    poly r;
    uint64_t cycles = 0;

    fill_poly(&a, 14);
    fill_poly(&b, 15);
    memset(&r, 0, sizeof(r));

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        poly_sub(&r, &a, &b);
        cycles += adjusted_delta(start, counter());
        a.coeffs[i % NTRE_N] = (int16_t)(a.coeffs[i % NTRE_N] + 1);
    }

    sink ^= checksum_poly(&r);
    return cycles;
}

static uint64_t bench_poly_double(uint64_t loops)
{
    poly a;
    poly r;
    uint64_t cycles = 0;

    fill_poly(&a, 16);
    memset(&r, 0, sizeof(r));

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        poly_double(&r, &a);
        cycles += adjusted_delta(start, counter());
        a.coeffs[i % NTRE_N] = (int16_t)(a.coeffs[i % NTRE_N] + 1);
    }

    sink ^= checksum_poly(&r);
    return cycles;
}

int main(void)
{
    const uint64_t loops = loops_from_env();

    setup_counter(loops);
    printf("instance,function,loops,countergap,cycles,sink\n");
    print_result(NTRE_ALGNAME, "poly_tobytes", loops, bench_poly_tobytes(loops));
    print_result(NTRE_ALGNAME, "poly_frombytes", loops, bench_poly_frombytes(loops));
    print_result(NTRE_ALGNAME, "poly_cbd1", loops, bench_poly_cbd1(loops));
    print_result(NTRE_ALGNAME, "poly_cbd1_prime", loops, bench_poly_cbd1_prime(loops));
    print_result(NTRE_ALGNAME, "poly_msg_mod2_to_bytes", loops, bench_poly_mod2(loops));
    print_result(NTRE_ALGNAME, "poly_ntt", loops, bench_poly_ntt(loops));
    print_result(NTRE_ALGNAME, "poly_invntt", loops, bench_poly_invntt(loops));
    print_result(NTRE_ALGNAME, "poly_baseinv", loops, bench_poly_baseinv(loops));
    print_result(NTRE_ALGNAME, "poly_basemul", loops, bench_poly_basemul(loops));
    print_result(NTRE_ALGNAME, "poly_basemul_add", loops, bench_poly_basemul_add(loops));
    print_result(NTRE_ALGNAME, "poly_sub", loops, bench_poly_sub(loops));
    print_result(NTRE_ALGNAME, "poly_double", loops, bench_poly_double(loops));
    return 0;
}
