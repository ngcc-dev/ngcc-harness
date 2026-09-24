#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif
#if defined(__GNUC__) || defined(_MSC_VER)
#include <x86intrin.h>
#endif
#include "registry.h"

static unsigned long long now_ns(void)
{
#ifdef _WIN32
    LARGE_INTEGER freq;
    LARGE_INTEGER counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (unsigned long long)((counter.QuadPart * 1000000000ULL) / freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec * 1000000000ULL + (unsigned long long)ts.tv_nsec;
#endif
}

static void timestamp(char *out, size_t out_len)
{
    time_t t = time(NULL);
    struct tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    strftime(out, out_len, "%Y%m%d_%H%M%S", &tmv);
}

static int functional_self_test(const algorithm_t *alg)
{
    static const unsigned char abc[] = { 'a', 'b', 'c' };
    unsigned char bit_msg[2] = { 0xb6U, 0x80U };
    unsigned char digest[128];
    unsigned char long_msg[4096];
    size_t i;

    for (i = 0; i < sizeof(long_msg); i++)
        long_msg[i] = (unsigned char)(11U + 37U * i);
    memset(digest, 0, sizeof(digest));

    if (alg->hash(alg->digest_bits, NULL, 0, digest) != 0) return 1;
    if (alg->hash(alg->digest_bits, abc, 24, digest) != 0) return 1;
    if (alg->hash(alg->digest_bits, long_msg, (unsigned long long)sizeof(long_msg) * 8ULL, digest) != 0) return 1;
    if (alg->hash(alg->digest_bits, bit_msg, 1, digest) != 0) return 1;
    if (alg->hash(alg->digest_bits, bit_msg, 7, digest) != 0) return 1;
    if (alg->hash(alg->digest_bits, bit_msg, 9, digest) != 0) return 1;
    if (alg->hash(alg->digest_bits, bit_msg, 15, digest) != 0) return 1;
    return alg->hash(alg->digest_bits == 512 ? 768 : 512, abc, 24, digest) == 0;
}

int run_hash_bench(const algorithm_t *alg, unsigned int times, size_t len_bytes)
{
    unsigned char *input;
    unsigned char digest[128];
    unsigned int i;
    unsigned int aux = 0;
    uint64_t total_cycles = 0;
    uint64_t min_cycles = UINT64_MAX;
    uint64_t max_cycles = 0;
    unsigned long long t0, t1;
    double avg_cycles;
    double mbps;
    char stamp[32];
    char report_path[256];
    char log_path[256];
    FILE *json;
    FILE *log;

    input = (unsigned char *)calloc(len_bytes == 0U ? 1U : len_bytes, 1U);
    if (input == NULL)
        return 1;

    timestamp(stamp, sizeof(stamp));
#ifdef _WIN32
    _mkdir("reports");
    _mkdir("logs");
#else
    mkdir("reports", 0777);
    mkdir("logs", 0777);
#endif
    snprintf(report_path, sizeof(report_path), "reports/%s_%s.json", alg->name, stamp);
    snprintf(log_path, sizeof(log_path), "logs/ngcc_bench_%s.log", stamp);
    log = fopen(log_path, "a");
    if (log == NULL)
        log = stdout;

    fprintf(stdout, "Running bench for %s\n", alg->name);
    fprintf(log, "Running bench for %s\n", alg->name);
    if (functional_self_test(alg) != 0) {
        fprintf(stderr, "Functional self-test failed for %s\n", alg->id);
        if (log != stdout) fclose(log);
        free(input);
        return 1;
    }
    fprintf(stdout, "Functional self-test PASS\n");
    fprintf(log, "Functional self-test PASS\n");

    t0 = now_ns();
    for (i = 0; i < times; i++) {
        uint64_t c0 = __rdtscp(&aux);
        if (alg->hash(alg->digest_bits, input, (unsigned long long)len_bytes * 8ULL, digest) != 0) {
            if (log != stdout) fclose(log);
            free(input);
            return 1;
        }
        uint64_t c1 = __rdtscp(&aux);
        uint64_t c = c1 - c0;
        total_cycles += c;
        if (c < min_cycles) min_cycles = c;
        if (c > max_cycles) max_cycles = c;
    }
    t1 = now_ns();

    avg_cycles = times == 0U ? 0.0 : (double)total_cycles / (double)times;
    mbps = (t1 == t0) ? 0.0 : ((double)len_bytes * (double)times * 8.0 * 1000.0) / (double)(t1 - t0);
    fprintf(stdout, "average cycles %.2f, Mbps %.2f\n", avg_cycles, mbps);
    fprintf(log, "average cycles %.2f, Mbps %.2f\n", avg_cycles, mbps);

    json = fopen(report_path, "w");
    if (json != NULL) {
        fprintf(json, "{\n");
        fprintf(json, "  \"algorithm\": \"%s\",\n", alg->id);
        fprintf(json, "  \"name\": \"%s\",\n", alg->name);
        fprintf(json, "  \"input_bytes\": %zu,\n", len_bytes);
        fprintf(json, "  \"times\": %u,\n", times);
        fprintf(json, "  \"average_cycles\": %.6f,\n", avg_cycles);
        fprintf(json, "  \"min_cycles\": %llu,\n", (unsigned long long)min_cycles);
        fprintf(json, "  \"max_cycles\": %llu,\n", (unsigned long long)max_cycles);
        fprintf(json, "  \"throughput_mbps\": %.6f,\n", mbps);
        fprintf(json, "  \"digest_bits\": %d,\n", alg->digest_bits);
        fprintf(json, "  \"library_path\": \"%s\"\n", alg->library_path);
        fprintf(json, "}\n");
        fclose(json);
    }

    if (log != stdout) fclose(log);
    free(input);
    return 0;
}
