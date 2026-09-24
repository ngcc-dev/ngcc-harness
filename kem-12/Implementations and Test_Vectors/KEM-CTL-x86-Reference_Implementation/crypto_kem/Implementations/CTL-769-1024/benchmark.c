#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef DO_BENCH86
#if defined __i386__ || defined _M_IX86 || defined __x86_64__ || defined _M_X64
#define DO_BENCH86   1
#else
#define DO_BENCH86   0
#endif
#endif

#if DO_BENCH86
#include <immintrin.h>

static inline uint64_t get_cycles(void) {
#if defined __GNUC__ && !defined __clang__
    uint32_t hi, lo;
    _mm_lfence();
    __asm__ __volatile__ ("rdtsc" : "=d"(hi), "=a"(lo) : : );
    return ((uint64_t)hi << 32) | (uint64_t)lo;
#else
    _mm_lfence();
    return __rdtsc();
#endif
}
#elif defined _WIN32 || defined _WIN64
#include <windows.h>
static inline uint64_t get_cycles(void) {
    LARGE_INTEGER val;
    QueryPerformanceCounter(&val);
    return (uint64_t)val.QuadPart;
}
#elif defined __GNUC__ && defined __aarch64__
static inline uint64_t get_cycles(void) {
    uint64_t val;
    __asm__ volatile ("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}
#else
#include <time.h>
static inline uint64_t get_cycles(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000 + (uint64_t)ts.tv_nsec;
}
#endif

#include "KEM_AlgorithmInstance.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

#define KEM_PK_BYTES 1230
#define KEM_SK_BYTES 6030
#define KEM_CT_BYTES 1006
#define KEM_SS_BYTES 32

static unsigned char pk[KEM_PK_BYTES];
static unsigned char sk[KEM_SK_BYTES];
static unsigned char ct[KEM_CT_BYTES];
static unsigned char ss[KEM_SS_BYTES];
static unsigned long long pk_len, sk_len, ct_len, ss_len;

static int bench_keygen(void) {
    return kem_keygen(pk, &pk_len, sk, &sk_len);
}

static int bench_enc(void) {
    return kem_enc(pk, pk_len, ss, &ss_len, ct, &ct_len);
}

static int bench_dec(void) {
    return kem_dec(sk, sk_len, ct, ct_len, ss, &ss_len);
}

static uint64_t do_benchmark(int (*func)(void), unsigned long *iterations_out) {
    unsigned long num = 1000;
    uint64_t begin, end;
    double tt;
    
    func();
    func();
    func();
    func();
    func();
    
    for (;;) {
        begin = get_cycles();
        for (unsigned long i = 0; i < num; i++) {
            func();
        }
        end = get_cycles();
        
#if DO_BENCH86
        tt = (double)(end - begin) / (double)1000000000.0;
#else
        tt = (double)(end - begin) / (double)3000000000.0;
#endif
        
        if (tt >= 1.0) {
            *iterations_out = num;
            return end - begin;
        }
        
        if (tt < 0.1) {
            num <<= 1;
        } else {
            unsigned long num2 = (unsigned long)((double)num * 1.1 / tt);
            if (num2 <= num) {
                num2 = num + 1;
            }
            num = num2;
        }
    }
}

int main(void) {
    uint64_t t_keygen, t_enc, t_dec;
    unsigned long iter_keygen, iter_enc, iter_dec;
    int ret;
    
    unsigned char seed[55];
    memset(seed, 0xAA, 55);
    ret = init_random_number(&drng_algorithm, seed, 55);
    if (ret != 0) {
        printf("Error: DRNG initialization failed\n");
        return 1;
    }
    
    printf("CTL-769-1024 KEM Performance Benchmark\n");
    printf("=====================================\n");
    
    t_keygen = do_benchmark(bench_keygen, &iter_keygen);
    printf("Keygen:  %llu cycles\n", t_keygen / iter_keygen);
    
    t_enc = do_benchmark(bench_enc, &iter_enc);
    printf("Encaps:  %llu cycles\n", t_enc / iter_enc);
    
    t_dec = do_benchmark(bench_dec, &iter_dec);
    printf("Decaps:  %llu cycles\n", t_dec / iter_dec);
    
    printf("Total:   %llu cycles\n", (t_keygen / iter_keygen) + (t_enc / iter_enc) + (t_dec / iter_dec));
    
    return 0;
}
