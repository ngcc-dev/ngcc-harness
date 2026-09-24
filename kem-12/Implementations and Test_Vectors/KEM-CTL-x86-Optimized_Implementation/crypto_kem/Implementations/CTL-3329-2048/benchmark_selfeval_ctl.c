/*
 * CTL KEM x86 self-evaluation benchmark.
 *
 * This file is intended to be built with MinGW/GCC on Windows.
 * It measures:
 *   1) performance: cycles/op, ns/op, ops/s for keygen, encapsulation, decapsulation;
 *   2) resource consumption: process static/baseline memory and observed peak memory;
 *   3) transmission/storage overhead: pk/sk/ct/shared-secret sizes.
 *
 * Notes:
 *   - On Windows, memory is measured through GetProcessMemoryInfo.
 *     Link with -lpsapi when using MinGW.
 *   - The memory number is process-level memory, not an instruction-level
 *     decomposition of only the CTL functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>

#if defined(_WIN32) || defined(_WIN64)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "ctl.h"

DRNG_ctx drng_algorithm;

#define DEFAULT_ITERATIONS 100
#define MIN_ITERATIONS     100
#define WARMUP_ITERATIONS  10
#define DRNG_SEED_LEN      55

static volatile unsigned char g_sink = 0;

typedef struct {
    uint64_t min;
    uint64_t max;
    long double sum;
    unsigned long count;
} stat_u64_t;

static void stat_init(stat_u64_t *s) {
    s->min = UINT64_MAX;
    s->max = 0;
    s->sum = 0.0L;
    s->count = 0;
}

static void stat_add(stat_u64_t *s, uint64_t v) {
    if (v == 0) {
        return;
    }
    if (v < s->min) {
        s->min = v;
    }
    if (v > s->max) {
        s->max = v;
    }
    s->sum += (long double)v;
    s->count++;
}

static long double stat_avg(const stat_u64_t *s) {
    if (s->count == 0) {
        return 0.0L;
    }
    return s->sum / (long double)s->count;
}

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
static uint64_t read_cycles(void) {
#if defined(_MSC_VER)
    return (uint64_t)__rdtsc();
#elif defined(__GNUC__) || defined(__clang__)
    uint32_t lo = 0;
    uint32_t hi = 0;
    __asm__ __volatile__(
        "lfence\n\t"
        "rdtsc\n\t"
        : "=a"(lo), "=d"(hi)
        :
        : "memory"
    );
    return ((uint64_t)hi << 32) | (uint64_t)lo;
#else
    return 0;
#endif
}
#else
static uint64_t read_cycles(void) {
    return 0;
}
#endif

static uint64_t now_ns(void) {
#if defined(_WIN32) || defined(_WIN64)
    LARGE_INTEGER freq;
    LARGE_INTEGER counter;
    if (!QueryPerformanceFrequency(&freq) || !QueryPerformanceCounter(&counter)) {
        return 0;
    }
    return (uint64_t)((long double)counter.QuadPart * 1000000000.0L /
                      (long double)freq.QuadPart);
#else
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        return 0;
    }
    return (uint64_t)tv.tv_sec * 1000000000ULL + (uint64_t)tv.tv_usec * 1000ULL;
#endif
}

static uint64_t current_memory_bytes(void) {
#if defined(_WIN32) || defined(_WIN64)
    PROCESS_MEMORY_COUNTERS pmc;
    memset(&pmc, 0, sizeof(pmc));
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, (DWORD)sizeof(pmc))) {
        return (uint64_t)pmc.WorkingSetSize;
    }
    return 0;
#else
    FILE *fp = fopen("/proc/self/statm", "r");
    long rss_pages = 0;
    long dummy = 0;
    long page_size = 0;

    if (fp == NULL) {
        return 0;
    }
    if (fscanf(fp, "%ld %ld", &dummy, &rss_pages) != 2) {
        fclose(fp);
        return 0;
    }
    fclose(fp);

    page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0 || rss_pages < 0) {
        return 0;
    }
    return (uint64_t)rss_pages * (uint64_t)page_size;
#endif
}

static uint64_t peak_memory_bytes(void) {
#if defined(_WIN32) || defined(_WIN64)
    PROCESS_MEMORY_COUNTERS pmc;
    memset(&pmc, 0, sizeof(pmc));
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, (DWORD)sizeof(pmc))) {
        return (uint64_t)pmc.PeakWorkingSetSize;
    }
    return 0;
#else
    FILE *fp = fopen("/proc/self/status", "r");
    char line[256];

    if (fp == NULL) {
        return 0;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        unsigned long kb = 0;
        if (sscanf(line, "VmHWM: %lu kB", &kb) == 1) {
            fclose(fp);
            return (uint64_t)kb * 1024ULL;
        }
    }

    fclose(fp);
    return current_memory_bytes();
#endif
}

static void make_seed(unsigned char seed[DRNG_SEED_LEN]) {
    int i;
    for (i = 0; i < DRNG_SEED_LEN; i++) {
        seed[i] = (unsigned char)(0xA5U ^ (unsigned int)(i * 17U + 3U));
    }
}

static int checked_size(unsigned long long v, size_t *out, const char *name) {
    if (v == 0ULL || v > (unsigned long long)SIZE_MAX) {
        printf("Error: invalid %s length: %llu\n", name, v);
        return -1;
    }
    *out = (size_t)v;
    return 0;
}

static int parse_iterations(int argc, char **argv) {
    long n = DEFAULT_ITERATIONS;

    if (argc >= 2) {
        char *endp = NULL;
        n = strtol(argv[1], &endp, 10);
        if (endp == argv[1] || *endp != '\0' || n <= 0) {
            printf("Warning: invalid iteration count, use default %d.\n",
                   DEFAULT_ITERATIONS);
            n = DEFAULT_ITERATIONS;
        }
    }

    if (n < MIN_ITERATIONS) {
        printf("Warning: file-1 requires repeated tests no fewer than %d times; "
               "iteration count is adjusted to %d.\n",
               MIN_ITERATIONS, MIN_ITERATIONS);
        n = MIN_ITERATIONS;
    }

    return (int)n;
}

static void consume_bytes(const unsigned char *buf, size_t len) {
    if (buf != NULL && len > 0U) {
        g_sink ^= buf[0];
        g_sink ^= buf[len - 1U];
    }
}

static int kem_once(unsigned char *pk, size_t pk_cap,
                    unsigned char *sk, size_t sk_cap,
                    unsigned char *ct, size_t ct_cap,
                    unsigned char *ss_enc, size_t ss_cap,
                    unsigned char *ss_dec, size_t ss_dec_cap) {
    unsigned long long pk_len = 0;
    unsigned long long sk_len = 0;
    unsigned long long ct_len = 0;
    unsigned long long ss_len = 0;
    unsigned long long ss2_len = 0;
    int ret;

    (void)pk_cap;
    (void)sk_cap;
    (void)ct_cap;
    (void)ss_cap;
    (void)ss_dec_cap;

    ret = kem_keygen(pk, &pk_len, sk, &sk_len);
    if (ret != 0) {
        return -1;
    }

    ret = kem_enc(pk, pk_len, ss_enc, &ss_len, ct, &ct_len);
    if (ret != 0) {
        return -2;
    }

    ret = kem_dec(sk, sk_len, ct, ct_len, ss_dec, &ss2_len);
    if (ret != 0) {
        return -3;
    }

    if (ss_len != ss2_len || memcmp(ss_enc, ss_dec, (size_t)ss_len) != 0) {
        return -4;
    }

    consume_bytes(pk, (size_t)pk_len);
    consume_bytes(sk, (size_t)sk_len);
    consume_bytes(ct, (size_t)ct_len);
    consume_bytes(ss_enc, (size_t)ss_len);

    return 0;
}

static void print_op_result(const char *name,
                            const stat_u64_t *cycles,
                            const stat_u64_t *ns,
                            int iterations) {
    long double avg_ns = stat_avg(ns);
    long double ops_s = 0.0L;

    if (avg_ns > 0.0L) {
        ops_s = 1000000000.0L / avg_ns;
    }

    printf("%-12s avg_cycles=%12.2Lf  min=%" PRIu64 "  max=%" PRIu64
           "  avg_time_ns=%12.2Lf  throughput=%12.2Lf ops/s\n",
           name,
           stat_avg(cycles),
           cycles->count ? cycles->min : 0,
           cycles->max,
           avg_ns,
           ops_s);

    (void)iterations;
}

static void write_summary_csv(const char *path,
                              int iterations,
                              unsigned long long pk_len,
                              unsigned long long sk_len,
                              unsigned long long ct_len,
                              unsigned long long ss_len,
                              const stat_u64_t *keygen_cycles,
                              const stat_u64_t *enc_cycles,
                              const stat_u64_t *dec_cycles,
                              const stat_u64_t *keygen_ns,
                              const stat_u64_t *enc_ns,
                              const stat_u64_t *dec_ns,
                              const stat_u64_t *static_mem,
                              const stat_u64_t *iter_peak_mem,
                              uint64_t os_peak) {
    FILE *fp = fopen(path, "w");
    long double keygen_ops = 0.0L;
    long double enc_ops = 0.0L;
    long double dec_ops = 0.0L;

    if (fp == NULL) {
        printf("Warning: cannot write %s\n", path);
        return;
    }

    if (stat_avg(keygen_ns) > 0.0L) {
        keygen_ops = 1000000000.0L / stat_avg(keygen_ns);
    }
    if (stat_avg(enc_ns) > 0.0L) {
        enc_ops = 1000000000.0L / stat_avg(enc_ns);
    }
    if (stat_avg(dec_ns) > 0.0L) {
        dec_ops = 1000000000.0L / stat_avg(dec_ns);
    }

    fprintf(fp, "metric,operation,value,unit\n");
    fprintf(fp, "iterations,all,%d,count\n", iterations);
    fprintf(fp, "avg_cycles,keygen,%.3Lf,cycles/op\n", stat_avg(keygen_cycles));
    fprintf(fp, "avg_cycles,encapsulation,%.3Lf,cycles/op\n", stat_avg(enc_cycles));
    fprintf(fp, "avg_cycles,decapsulation,%.3Lf,cycles/op\n", stat_avg(dec_cycles));
    fprintf(fp, "avg_time,keygen,%.3Lf,ns/op\n", stat_avg(keygen_ns));
    fprintf(fp, "avg_time,encapsulation,%.3Lf,ns/op\n", stat_avg(enc_ns));
    fprintf(fp, "avg_time,decapsulation,%.3Lf,ns/op\n", stat_avg(dec_ns));
    fprintf(fp, "throughput,keygen,%.3Lf,ops/s\n", keygen_ops);
    fprintf(fp, "throughput,encapsulation,%.3Lf,ops/s\n", enc_ops);
    fprintf(fp, "throughput,decapsulation,%.3Lf,ops/s\n", dec_ops);
    fprintf(fp, "static_memory_min,process,%" PRIu64 ",Bytes\n",
            static_mem->count ? static_mem->min : 0);
    fprintf(fp, "static_memory_avg,process,%.3Lf,Bytes\n", stat_avg(static_mem));
    fprintf(fp, "static_memory_max,process,%" PRIu64 ",Bytes\n",
            static_mem->max);
    fprintf(fp, "observed_peak_memory_min,process,%" PRIu64 ",Bytes\n",
            iter_peak_mem->count ? iter_peak_mem->min : 0);
    fprintf(fp, "observed_peak_memory_avg,process,%.3Lf,Bytes\n",
            stat_avg(iter_peak_mem));
    fprintf(fp, "observed_peak_memory_max,process,%" PRIu64 ",Bytes\n",
            iter_peak_mem->max);
    fprintf(fp, "os_peak_working_set,process,%" PRIu64 ",Bytes\n", os_peak);
    fprintf(fp, "data_size,public_key,%llu,Bytes\n", pk_len);
    fprintf(fp, "data_size,private_key,%llu,Bytes\n", sk_len);
    fprintf(fp, "data_size,ciphertext,%llu,Bytes\n", ct_len);
    fprintf(fp, "data_size,shared_secret,%llu,Bytes\n", ss_len);
    fclose(fp);
}

int main(int argc, char **argv) {
    const int iterations = parse_iterations(argc, argv);
    unsigned char seed[DRNG_SEED_LEN];

    unsigned long long pk_len_decl = kem_get_pk_len_bytes();
    unsigned long long sk_len_decl = kem_get_sk_len_bytes();
    unsigned long long ct_len_decl = kem_get_ct_len_bytes();
    unsigned long long ss_len_decl = kem_get_ss_len_bytes();

    size_t pk_size = 0;
    size_t sk_size = 0;
    size_t ct_size = 0;
    size_t ss_size = 0;

    unsigned char *pk = NULL;
    unsigned char *sk = NULL;
    unsigned char *ct = NULL;
    unsigned char *ss_enc = NULL;
    unsigned char *ss_dec = NULL;

    FILE *raw = NULL;

    stat_u64_t keygen_cycles;
    stat_u64_t enc_cycles;
    stat_u64_t dec_cycles;
    stat_u64_t keygen_ns;
    stat_u64_t enc_ns;
    stat_u64_t dec_ns;
    stat_u64_t static_mem;
    stat_u64_t iter_peak_mem;

    int i;
    int ret;

    stat_init(&keygen_cycles);
    stat_init(&enc_cycles);
    stat_init(&dec_cycles);
    stat_init(&keygen_ns);
    stat_init(&enc_ns);
    stat_init(&dec_ns);
    stat_init(&static_mem);
    stat_init(&iter_peak_mem);

    printf("CTL KEM x86 Self-evaluation Benchmark\n");
    printf("=====================================\n");
    printf("Algorithm name     : CTL-3329-2048\n");
    printf("Iterations         : %d\n", iterations);
    printf("Compiler           : %s\n", __VERSION__);
    printf("Build date/time    : %s %s\n", __DATE__, __TIME__);
#if defined(_WIN32) || defined(_WIN64)
    printf("OS API             : Windows / MinGW\n");
#else
    printf("OS API             : POSIX-like\n");
#endif
#if defined(__x86_64__) || defined(_M_X64)
    printf("Target pointer     : 64-bit x86\n");
#elif defined(__i386__) || defined(_M_IX86)
    printf("Target pointer     : 32-bit x86\n");
#else
    printf("Target pointer     : non-x86 or unknown\n");
#endif
    printf("\n");

    if (checked_size(pk_len_decl, &pk_size, "pk") != 0 ||
        checked_size(sk_len_decl, &sk_size, "sk") != 0 ||
        checked_size(ct_len_decl, &ct_size, "ct") != 0 ||
        checked_size(ss_len_decl, &ss_size, "ss") != 0) {
        return 1;
    }

    make_seed(seed);
    ret = init_random_number(&drng_algorithm, seed, DRNG_SEED_LEN);
    if (ret != 0) {
        printf("Error: init_random_number failed, ret=%d\n", ret);
        return 1;
    }

    /*
     * Static/baseline process memory after loading this program and initializing
     * the deterministic RNG, before KEM core operations are timed.
     */
    stat_add(&static_mem, current_memory_bytes());

    pk = (unsigned char *)calloc(pk_size, 1U);
    sk = (unsigned char *)calloc(sk_size, 1U);
    ct = (unsigned char *)calloc(ct_size, 1U);
    ss_enc = (unsigned char *)calloc(ss_size, 1U);
    ss_dec = (unsigned char *)calloc(ss_size, 1U);

    if (pk == NULL || sk == NULL || ct == NULL || ss_enc == NULL || ss_dec == NULL) {
        printf("Error: memory allocation failed.\n");
        free(pk);
        free(sk);
        free(ct);
        free(ss_enc);
        free(ss_dec);
        return 1;
    }

    raw = fopen("ctl_kem_benchmark_raw.csv", "w");
    if (raw != NULL) {
        fprintf(raw, "iteration,keygen_cycles,encapsulation_cycles,decapsulation_cycles,"
                     "keygen_ns,encapsulation_ns,decapsulation_ns,"
                     "static_memory_before_bytes,observed_memory_after_keygen_bytes,"
                     "observed_memory_after_encapsulation_bytes,"
                     "observed_memory_after_decapsulation_bytes,"
                     "observed_iteration_peak_bytes\n");
    } else {
        printf("Warning: cannot write ctl_kem_benchmark_raw.csv\n");
    }

    for (i = 0; i < WARMUP_ITERATIONS; i++) {
        ret = kem_once(pk, pk_size, sk, sk_size, ct, ct_size,
                       ss_enc, ss_size, ss_dec, ss_size);
        if (ret != 0) {
            printf("Error: warm-up KEM test failed, ret=%d\n", ret);
            fclose(raw);
            free(pk);
            free(sk);
            free(ct);
            free(ss_enc);
            free(ss_dec);
            return 1;
        }
    }

    make_seed(seed);
    ret = init_random_number(&drng_algorithm, seed, DRNG_SEED_LEN);
    if (ret != 0) {
        printf("Error: init_random_number failed before benchmark, ret=%d\n", ret);
        fclose(raw);
        free(pk);
        free(sk);
        free(ct);
        free(ss_enc);
        free(ss_dec);
        return 1;
    }

    for (i = 0; i < iterations; i++) {
        unsigned long long pk_len = 0;
        unsigned long long sk_len = 0;
        unsigned long long ct_len = 0;
        unsigned long long ss_len = 0;
        unsigned long long ss2_len = 0;

        uint64_t c0 = 0;
        uint64_t c1 = 0;
        uint64_t n0 = 0;
        uint64_t n1 = 0;

        uint64_t keygen_c = 0;
        uint64_t enc_c = 0;
        uint64_t dec_c = 0;
        uint64_t keygen_t = 0;
        uint64_t enc_t = 0;
        uint64_t dec_t = 0;

        uint64_t mem_before = current_memory_bytes();
        uint64_t mem_after_keygen = 0;
        uint64_t mem_after_enc = 0;
        uint64_t mem_after_dec = 0;
        uint64_t iter_peak = 0;

        stat_add(&static_mem, mem_before);

        c0 = read_cycles();
        n0 = now_ns();
        ret = kem_keygen(pk, &pk_len, sk, &sk_len);
        n1 = now_ns();
        c1 = read_cycles();
        if (ret != 0) {
            printf("Error: keygen failed at iteration %d, ret=%d\n", i, ret);
            fclose(raw);
            free(pk);
            free(sk);
            free(ct);
            free(ss_enc);
            free(ss_dec);
            return 1;
        }
        keygen_c = c1 - c0;
        keygen_t = n1 - n0;
        mem_after_keygen = current_memory_bytes();

        c0 = read_cycles();
        n0 = now_ns();
        ret = kem_enc(pk, pk_len, ss_enc, &ss_len, ct, &ct_len);
        n1 = now_ns();
        c1 = read_cycles();
        if (ret != 0) {
            printf("Error: encapsulation failed at iteration %d, ret=%d\n", i, ret);
            fclose(raw);
            free(pk);
            free(sk);
            free(ct);
            free(ss_enc);
            free(ss_dec);
            return 1;
        }
        enc_c = c1 - c0;
        enc_t = n1 - n0;
        mem_after_enc = current_memory_bytes();

        c0 = read_cycles();
        n0 = now_ns();
        ret = kem_dec(sk, sk_len, ct, ct_len, ss_dec, &ss2_len);
        n1 = now_ns();
        c1 = read_cycles();
        if (ret != 0) {
            printf("Error: decapsulation failed at iteration %d, ret=%d\n", i, ret);
            fclose(raw);
            free(pk);
            free(sk);
            free(ct);
            free(ss_enc);
            free(ss_dec);
            return 1;
        }
        dec_c = c1 - c0;
        dec_t = n1 - n0;
        mem_after_dec = current_memory_bytes();

        if (pk_len != pk_len_decl || sk_len != sk_len_decl ||
            ct_len != ct_len_decl || ss_len != ss_len_decl ||
            ss2_len != ss_len_decl) {
            printf("Error: length mismatch at iteration %d.\n", i);
            printf("Declared pk/sk/ct/ss = %llu/%llu/%llu/%llu\n",
                   pk_len_decl, sk_len_decl, ct_len_decl, ss_len_decl);
            printf("Actual   pk/sk/ct/ss/ss2 = %llu/%llu/%llu/%llu/%llu\n",
                   pk_len, sk_len, ct_len, ss_len, ss2_len);
            fclose(raw);
            free(pk);
            free(sk);
            free(ct);
            free(ss_enc);
            free(ss_dec);
            return 1;
        }

        if (memcmp(ss_enc, ss_dec, ss_size) != 0) {
            printf("Error: shared secrets differ at iteration %d.\n", i);
            fclose(raw);
            free(pk);
            free(sk);
            free(ct);
            free(ss_enc);
            free(ss_dec);
            return 1;
        }

        consume_bytes(pk, pk_size);
        consume_bytes(sk, sk_size);
        consume_bytes(ct, ct_size);
        consume_bytes(ss_enc, ss_size);

        iter_peak = mem_after_keygen;
        if (mem_after_enc > iter_peak) {
            iter_peak = mem_after_enc;
        }
        if (mem_after_dec > iter_peak) {
            iter_peak = mem_after_dec;
        }

        stat_add(&keygen_cycles, keygen_c);
        stat_add(&enc_cycles, enc_c);
        stat_add(&dec_cycles, dec_c);
        stat_add(&keygen_ns, keygen_t);
        stat_add(&enc_ns, enc_t);
        stat_add(&dec_ns, dec_t);
        stat_add(&iter_peak_mem, iter_peak);

        if (raw != NULL) {
            fprintf(raw, "%d,%" PRIu64 ",%" PRIu64 ",%" PRIu64
                         ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
                         ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
                         ",%" PRIu64 "\n",
                    i + 1,
                    keygen_c, enc_c, dec_c,
                    keygen_t, enc_t, dec_t,
                    mem_before,
                    mem_after_keygen,
                    mem_after_enc,
                    mem_after_dec,
                    iter_peak);
        }
    }

    if (raw != NULL) {
        fclose(raw);
    }

    printf("Performance results\n");
    printf("-------------------\n");
    print_op_result("keygen", &keygen_cycles, &keygen_ns, iterations);
    print_op_result("encaps", &enc_cycles, &enc_ns, iterations);
    print_op_result("decaps", &dec_cycles, &dec_ns, iterations);
    printf("\n");

    printf("Resource consumption results\n");
    printf("----------------------------\n");
    printf("Static/baseline memory, process working set: "
           "min=%" PRIu64 "  avg=%.2Lf  max=%" PRIu64 " Bytes\n",
           static_mem.count ? static_mem.min : 0,
           stat_avg(&static_mem),
           static_mem.max);
    printf("Observed per-iteration peak working set:     "
           "min=%" PRIu64 "  avg=%.2Lf  max=%" PRIu64 " Bytes\n",
           iter_peak_mem.count ? iter_peak_mem.min : 0,
           stat_avg(&iter_peak_mem),
           iter_peak_mem.max);
    printf("OS reported peak working set:                %" PRIu64 " Bytes\n",
           peak_memory_bytes());
    printf("\n");

    printf("Transmission and storage overhead\n");
    printf("---------------------------------\n");
    printf("public_key_bytes      = %llu\n", pk_len_decl);
    printf("private_key_bytes     = %llu\n", sk_len_decl);
    printf("ciphertext_bytes      = %llu\n", ct_len_decl);
    printf("shared_secret_bytes   = %llu\n", ss_len_decl);
    printf("\n");

    write_summary_csv("ctl_kem_benchmark_summary.csv",
                      iterations,
                      pk_len_decl, sk_len_decl, ct_len_decl, ss_len_decl,
                      &keygen_cycles, &enc_cycles, &dec_cycles,
                      &keygen_ns, &enc_ns, &dec_ns,
                      &static_mem, &iter_peak_mem, peak_memory_bytes());

    printf("Raw CSV     : ctl_kem_benchmark_raw.csv\n");
    printf("Summary CSV : ctl_kem_benchmark_summary.csv\n");
    printf("Result sink : %u\n", (unsigned int)g_sink);

    free(pk);
    free(sk);
    free(ct);
    free(ss_enc);
    free(ss_dec);

    return 0;
}
