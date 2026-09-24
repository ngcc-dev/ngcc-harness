#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "params.h"
#include "symmetric.h"
#include "auxfunc.h"

#ifndef AUXFUNC_BENCH_DEFAULT_LOOP_COUNT
#define AUXFUNC_BENCH_DEFAULT_LOOP_COUNT 10000
#endif

#define BITS(n) ((unsigned long long)(n) * 8ULL)

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
        return AUXFUNC_BENCH_DEFAULT_LOOP_COUNT;

    loops = strtoull(value, &end, 10);
    if (end == value || *end != '\0' || loops == 0)
        return AUXFUNC_BENCH_DEFAULT_LOOP_COUNT;
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
    uint32_t s = 0x6D2B79F5u ^ seed;

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

static void print_result(const char *function, size_t inbytes, size_t outbytes,
                         uint64_t loops, uint64_t cycles)
{
    const size_t sm3_calls = (outbytes + 31) / 32;

    printf("%s,%s,%zu,%zu,%zu,%llu,%llu,%llu,%llu\n",
           NTRE_ALGNAME, function, inbytes, outbytes, sm3_calls,
           (unsigned long long)loops,
           (unsigned long long)countergap,
           (unsigned long long)(cycles / loops),
           (unsigned long long)sink);
}

static uint64_t bench_pseudoXOF(const char *tag, uint8_t *msg, size_t msg_len,
                                uint8_t *out, size_t out_len, uint64_t loops)
{
    uint64_t cycles = 0;
    int failures = 0;
    (void)tag;

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        failures |= pseudoXOF(BITS(out_len), msg, BITS(msg_len), out);
        cycles += adjusted_delta(start, counter());
        msg[i % msg_len] ^= (uint8_t)(i + 1);
    }

    sink ^= checksum_bytes(out, out_len) ^ (uint64_t)failures;
    return cycles;
}

static uint64_t bench_sm3hash(uint8_t *msg, size_t msg_len, uint8_t out[32],
                              uint64_t loops)
{
    uint64_t cycles = 0;
    int failures = 0;

    for (uint64_t i = 0; i < loops; i++) {
        const uint64_t start = counter();
        failures |= sm3hash(256, msg, BITS(msg_len), out);
        cycles += adjusted_delta(start, counter());
        msg[i % msg_len] ^= (uint8_t)(i + 3);
    }

    sink ^= checksum_bytes(out, 32) ^ (uint64_t)failures;
    return cycles;
}

int main(void)
{
    const uint64_t loops = loops_from_env();
    uint8_t h_msg[1 + NTRE_RBYTES + NTRE_GBYTES];
    uint8_t h_out[HASH_H_OUTBYTES];
    uint8_t f_msg[1 + NTRE_M1BYTES];
    uint8_t f_out[NTRE_RBYTES];
    uint8_t psi_msg[1 + NTRE_SYMBYTES];
    uint8_t psi_out[NTRE_SAMPLEBYTES];
    uint8_t pk_msg[NTRE_PUBLICKEYBYTES];
    uint8_t sm3_out[32];

    fill_bytes(h_msg, sizeof(h_msg), 1);
    fill_bytes(f_msg, sizeof(f_msg), 2);
    fill_bytes(psi_msg, sizeof(psi_msg), 3);
    fill_bytes(pk_msg, sizeof(pk_msg), 4);
    memset(h_out, 0, sizeof(h_out));
    memset(f_out, 0, sizeof(f_out));
    memset(psi_out, 0, sizeof(psi_out));
    memset(sm3_out, 0, sizeof(sm3_out));

    setup_counter(loops);
    printf("instance,function,input_bytes,output_bytes,sm3_calls,loops,countergap,cycles,sink\n");
    print_result("pseudoXOF_hash_H_shape", sizeof(h_msg), sizeof(h_out), loops,
                 bench_pseudoXOF("H", h_msg, sizeof(h_msg), h_out, sizeof(h_out), loops));
    print_result("pseudoXOF_hash_F_shape", sizeof(f_msg), sizeof(f_out), loops,
                 bench_pseudoXOF("F", f_msg, sizeof(f_msg), f_out, sizeof(f_out), loops));
    print_result("pseudoXOF_sample_psi1_shape", sizeof(psi_msg), sizeof(psi_out), loops,
                 bench_pseudoXOF("psi", psi_msg, sizeof(psi_msg), psi_out, sizeof(psi_out), loops));
    print_result("sm3hash_pk_shape", sizeof(pk_msg), sizeof(sm3_out), loops,
                 bench_sm3hash(pk_msg, sizeof(pk_msg), sm3_out, loops));
    return 0;
}
