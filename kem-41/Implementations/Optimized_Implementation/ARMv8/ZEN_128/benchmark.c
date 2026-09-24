/*
Copyright (c) 2026 Liu Tianhao.
Organization: School of Software Technology, Zhejiang University, Ningbo.
File Description: NGCC ARM self-evaluation benchmark for the optimized ZEN-128 KEM instance.
*/

#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__linux__)
#include <sched.h>
#endif

#if defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/types.h>
#endif

#if defined(__unix__) || defined(__APPLE__)
#include <sys/resource.h>
#include <sys/utsname.h>
#include <unistd.h>
#endif

#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "params.h"

#define BENCH_DEFAULT_ITERATIONS 1000ULL
#define BENCH_MIN_ITERATIONS 100ULL
#define BENCH_WARMUP_ITERATIONS 10ULL
#define BENCH_DEFAULT_CFLAGS "-O3 -march=armv8.2-a+crypto+sha3+dotprod+sve -flto -fomit-frame-pointer -std=c99 -Wpedantic -Wall -Wextra"
#define BENCH_TARGET_ISA "ARMv8.2-A+NEON+SVE+Crypto+SHA3+DotProd"

DRNG_ctx drng_algorithm;

static volatile uint64_t sink;

typedef struct {
    unsigned char pk[ZEN_PUBLICKEY_LEN_BYTES];
    unsigned char sk[ZEN_SECREKEY_LEN_BYTES];
    unsigned char ct[ZEN_CIPHERTEXT_LEN_BYTES];
    unsigned char ss[ZEN_SHAREDKEY_LEN_BYTES];
    unsigned char dec_ss[ZEN_SHAREDKEY_LEN_BYTES];
    unsigned char derand_seed[SEED_LEN_BYTES];
    unsigned char drng_seed[SEED_LEN_BYTES];
} bench_ctx;

typedef int (*operation_fn)(bench_ctx *ctx);

typedef struct {
    const char *operation;
    uint64_t iterations;
    uint64_t min_ns;
    uint64_t median_ns;
    uint64_t max_ns;
    long double average_ns;
    long double average_cycles;
    long double ops_per_second;
    unsigned int failures;
} result_summary;

typedef struct {
    uint64_t hz;
    const char *source;
} cpu_frequency;

typedef struct {
    uint64_t text_rodata_bytes;
    uint64_t writable_static_bytes;
    uint64_t total_static_bytes;
    const char *source;
} static_memory_info;

typedef struct {
    char start_time_utc[32];
    char end_time_utc[32];
    char os_release[160];
    char kernel[160];
    char machine[64];
    char cpu_model[160];
    char cpu_features[640];
    uint64_t mem_total_bytes;
    long online_cpus;
    int pin_cpu_requested;
    int pin_cpu_result;
    char compiler[160];
} environment_info;

typedef struct {
    int length_getters;
    int fixed_seed_round_trip;
    int randomized_round_trip;
} functional_summary;

#if defined(__linux__) && defined(__ELF__)
extern const char __executable_start[];
extern const char etext[];
extern const char edata[];
extern const char end[];
#endif

static uint64_t now_ns(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static int compare_u64(const void *lhs, const void *rhs)
{
    const uint64_t a = *(const uint64_t *)lhs;
    const uint64_t b = *(const uint64_t *)rhs;
    return (a > b) - (a < b);
}

static void fill_seed(unsigned char *seed, size_t seed_len, uint32_t domain)
{
    uint32_t x = 0x9e3779b9u ^ (domain * 0x85ebca6bu);
    size_t i;

    for (i = 0; i < seed_len; i++) {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        seed[i] = (unsigned char)(x + (uint32_t)i * 17u + domain);
    }
}

static int reset_drng(bench_ctx *ctx, uint32_t domain)
{
    fill_seed(ctx->drng_seed, sizeof(ctx->drng_seed), domain);
    return init_random_number(&drng_algorithm, ctx->drng_seed,
                              sizeof(ctx->drng_seed));
}

static uint64_t parse_u64_text(const char *text)
{
    char *end = NULL;
    unsigned long long value;

    if (text == NULL || *text == '\0') {
        return 0;
    }
    errno = 0;
    value = strtoull(text, &end, 10);
    if (errno != 0 || end == text) {
        return 0;
    }
    return (uint64_t)value;
}

static uint64_t read_u64_file(const char *path)
{
    FILE *fp = fopen(path, "r");
    char buf[64];
    uint64_t value = 0;

    if (fp == NULL) {
        return 0;
    }
    if (fgets(buf, sizeof(buf), fp) != NULL) {
        value = parse_u64_text(buf);
    }
    fclose(fp);
    return value;
}

static cpu_frequency detect_cpu_frequency(void)
{
    cpu_frequency info;
    const char *env_hz = getenv("ZEN_BENCH_CPU_HZ");
    uint64_t khz;

    info.hz = parse_u64_text(env_hz);
    info.source = info.hz ? "ZEN_BENCH_CPU_HZ" : "unavailable";
    if (info.hz != 0) {
        return info;
    }

    khz = read_u64_file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
    if (khz != 0) {
        info.hz = khz * 1000ULL;
        info.source = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq";
        return info;
    }
    khz = read_u64_file("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
    if (khz != 0) {
        info.hz = khz * 1000ULL;
        info.source = "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq";
        return info;
    }

#if defined(__APPLE__)
    {
        uint64_t hz = 0;
        size_t len = sizeof(hz);
        if (sysctlbyname("hw.cpufrequency", &hz, &len, NULL, 0) == 0 && hz != 0) {
            info.hz = hz;
            info.source = "hw.cpufrequency";
            return info;
        }
    }
#endif

    return info;
}

static uint64_t estimate_timer_overhead_ns(void)
{
    uint64_t best = UINT64_MAX;
    unsigned int i;

    for (i = 0; i < 10000U; i++) {
        uint64_t start = now_ns();
        uint64_t end = now_ns();
        if (end >= start && end - start < best) {
            best = end - start;
        }
    }
    return best == UINT64_MAX ? 0 : best;
}

static uint64_t estimate_cycles(uint64_t ns, uint64_t cpu_hz)
{
    if (cpu_hz == 0) {
        return 0;
    }
    return (uint64_t)(((long double)ns * (long double)cpu_hz / 1000000000.0L) + 0.5L);
}

static uint64_t get_peak_rss_bytes(void)
{
#if defined(__unix__) || defined(__APPLE__)
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0;
    }
#if defined(__APPLE__)
    return (uint64_t)usage.ru_maxrss;
#else
    return (uint64_t)usage.ru_maxrss * 1024ULL;
#endif
#else
    return 0;
#endif
}

static char *trim_ascii(char *text)
{
    char *end;

    while (*text != '\0' && isspace((unsigned char)*text)) {
        text++;
    }
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) {
        *--end = '\0';
    }
    return text;
}

static void copy_text(char *dst, size_t dst_len, const char *src)
{
    if (dst_len == 0) {
        return;
    }
    if (src == NULL) {
        src = "";
    }
    snprintf(dst, dst_len, "%s", src);
}

static void read_os_release(char *out, size_t out_len)
{
    FILE *fp = fopen("/etc/os-release", "r");
    char line[256];

    copy_text(out, out_len, "unavailable");
    if (fp == NULL) {
        return;
    }
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
            char *value = trim_ascii(line + 12);
            size_t len = strlen(value);
            if (len >= 2U && value[0] == '"' && value[len - 1U] == '"') {
                value[len - 1U] = '\0';
                value++;
            }
            copy_text(out, out_len, value);
            break;
        }
    }
    fclose(fp);
}

static void read_proc_cpu_field(const char *field, char *out, size_t out_len)
{
    FILE *fp = fopen("/proc/cpuinfo", "r");
    char line[768];

    copy_text(out, out_len, "unavailable");
    if (fp == NULL) {
        return;
    }
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *colon = strchr(line, ':');
        char *key;
        char *value;
        if (colon == NULL) {
            continue;
        }
        *colon = '\0';
        key = trim_ascii(line);
        value = trim_ascii(colon + 1);
        if (strcmp(key, field) == 0) {
            char *newline = strchr(value, '\n');
            if (newline != NULL) {
                *newline = '\0';
            }
            copy_text(out, out_len, value);
            break;
        }
    }
    fclose(fp);
}

static void read_cpu_model(char *out, size_t out_len)
{
    read_proc_cpu_field("model name", out, out_len);
    if (strcmp(out, "unavailable") == 0) {
        read_proc_cpu_field("Processor", out, out_len);
    }
    if (strcmp(out, "unavailable") == 0) {
        read_proc_cpu_field("Hardware", out, out_len);
    }
}

static uint64_t read_mem_total_bytes(void)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    char key[64];
    char unit[32];
    unsigned long long value = 0;

    if (fp == NULL) {
        return 0;
    }
    while (fscanf(fp, "%63s %llu %31s", key, &value, unit) == 3) {
        if (strcmp(key, "MemTotal:") == 0) {
            fclose(fp);
            return (uint64_t)value * 1024ULL;
        }
    }
    fclose(fp);
    return 0;
}

static void format_utc_time(char *out, size_t out_len)
{
    time_t now = time(NULL);
    struct tm *tm_now = gmtime(&now);

    if (out_len == 0) {
        return;
    }
    if (tm_now == NULL ||
        strftime(out, out_len, "%Y-%m-%dT%H:%M:%SZ", tm_now) == 0) {
        copy_text(out, out_len, "unavailable");
    }
}

static static_memory_info get_static_memory_info(void)
{
    static_memory_info info;

    memset(&info, 0, sizeof(info));
    info.source = "unavailable";
#if defined(__linux__) && defined(__ELF__)
    const char *image_start = &__executable_start[0];
    const char *text_end = &etext[0];
    const char *data_end = &edata[0];
    const char *image_end = &end[0];

    if (text_end >= image_start && data_end >= text_end && image_end >= data_end) {
        info.text_rodata_bytes = (uint64_t)(text_end - image_start);
        info.writable_static_bytes = (uint64_t)(image_end - text_end);
        info.total_static_bytes = (uint64_t)(image_end - image_start);
        info.source = "ELF linker symbols (__executable_start, etext, edata, end)";
    }
#endif
    return info;
}

static int pin_to_requested_cpu(void)
{
#if defined(__linux__)
    const char *pin = getenv("ZEN_BENCH_PIN_CPU");
    unsigned long cpu;
    char *endptr = NULL;
    cpu_set_t set;

    if (pin == NULL || *pin == '\0') {
        return 0;
    }
    errno = 0;
    cpu = strtoul(pin, &endptr, 10);
    if (errno != 0 || endptr == pin || *endptr != '\0') {
        return -1;
    }
    CPU_ZERO(&set);
    CPU_SET((int)cpu, &set);
    return sched_setaffinity(0, sizeof(set), &set) == 0 ? 1 : -1;
#else
    return 0;
#endif
}

static void collect_environment(environment_info *env)
{
#if defined(__unix__) || defined(__APPLE__)
    struct utsname uts;
#endif

    memset(env, 0, sizeof(*env));
    format_utc_time(env->start_time_utc, sizeof(env->start_time_utc));
    copy_text(env->end_time_utc, sizeof(env->end_time_utc), "unavailable");
    read_os_release(env->os_release, sizeof(env->os_release));
    read_cpu_model(env->cpu_model, sizeof(env->cpu_model));
    read_proc_cpu_field("Features", env->cpu_features, sizeof(env->cpu_features));
    env->mem_total_bytes = read_mem_total_bytes();
    env->online_cpus = -1;
    env->pin_cpu_requested = getenv("ZEN_BENCH_PIN_CPU") != NULL ? 1 : 0;
    env->pin_cpu_result = pin_to_requested_cpu();
    snprintf(env->compiler, sizeof(env->compiler), "GCC/compatible C compiler: %s",
             __VERSION__);

#if defined(__unix__) || defined(__APPLE__)
    if (uname(&uts) == 0) {
        snprintf(env->kernel, sizeof(env->kernel), "%s %s", uts.sysname, uts.release);
        copy_text(env->machine, sizeof(env->machine), uts.machine);
    } else {
        copy_text(env->kernel, sizeof(env->kernel), "unavailable");
        copy_text(env->machine, sizeof(env->machine), "unavailable");
    }
    env->online_cpus = sysconf(_SC_NPROCESSORS_ONLN);
#else
    copy_text(env->kernel, sizeof(env->kernel), "unavailable");
    copy_text(env->machine, sizeof(env->machine), "unavailable");
#endif
}

static void json_write_string(FILE *json, const char *text)
{
    const unsigned char *p = (const unsigned char *)text;

    fputc('"', json);
    if (text != NULL) {
        while (*p != '\0') {
            switch (*p) {
            case '\\':
                fputs("\\\\", json);
                break;
            case '"':
                fputs("\\\"", json);
                break;
            case '\n':
                fputs("\\n", json);
                break;
            case '\r':
                fputs("\\r", json);
                break;
            case '\t':
                fputs("\\t", json);
                break;
            default:
                if (*p < 0x20U) {
                    fprintf(json, "\\u%04x", (unsigned int)*p);
                } else {
                    fputc(*p, json);
                }
                break;
            }
            p++;
        }
    }
    fputc('"', json);
}

static int check_lengths(void)
{
    if (kem_get_pk_len_bytes() != ZEN_PUBLICKEY_LEN_BYTES ||
        kem_get_sk_len_bytes() != ZEN_SECREKEY_LEN_BYTES ||
        kem_get_ct_len_bytes() != ZEN_CIPHERTEXT_LEN_BYTES ||
        kem_get_ss_len_bytes() != ZEN_SHAREDKEY_LEN_BYTES) {
        fprintf(stderr, "length getter mismatch\n");
        return -1;
    }
    return 0;
}

static int prepare_fixed_kem(bench_ctx *ctx)
{
    unsigned long long pk_len = ZEN_PUBLICKEY_LEN_BYTES;
    unsigned long long sk_len = ZEN_SECREKEY_LEN_BYTES;
    unsigned long long ct_len = ZEN_CIPHERTEXT_LEN_BYTES;
    unsigned long long ss_len = ZEN_SHAREDKEY_LEN_BYTES;

    memset(ctx, 0, sizeof(*ctx));
    fill_seed(ctx->derand_seed, sizeof(ctx->derand_seed), 0x128001u);
    if (reset_drng(ctx, 0x128101u) != 0) {
        return -1;
    }
    if (kem_keygen_derand(ctx->pk, &pk_len, ctx->sk, &sk_len,
                          ctx->derand_seed) != 0) {
        return -1;
    }
    if (reset_drng(ctx, 0x128201u) != 0) {
        return -1;
    }
    if (kem_enc(ctx->pk, pk_len, ctx->ss, &ss_len, ctx->ct, &ct_len) != 0) {
        return -1;
    }
    if (kem_dec(ctx->sk, sk_len, ctx->ct, ct_len, ctx->dec_ss, &ss_len) != 0) {
        return -1;
    }
    if (memcmp(ctx->ss, ctx->dec_ss, ZEN_SHAREDKEY_LEN_BYTES) != 0) {
        fprintf(stderr, "KEM round-trip mismatch\n");
        return -1;
    }
    return 0;
}

static int functional_tests(bench_ctx *ctx, functional_summary *summary)
{
    unsigned long long pk_len = ZEN_PUBLICKEY_LEN_BYTES;
    unsigned long long sk_len = ZEN_SECREKEY_LEN_BYTES;
    unsigned long long ct_len = ZEN_CIPHERTEXT_LEN_BYTES;
    unsigned long long ss_len = ZEN_SHAREDKEY_LEN_BYTES;

    memset(summary, 0, sizeof(*summary));
    if (check_lengths() != 0) {
        return -1;
    }
    summary->length_getters = 1;

    if (prepare_fixed_kem(ctx) != 0) {
        return -1;
    }
    summary->fixed_seed_round_trip = 1;

    memset(ctx, 0, sizeof(*ctx));
    if (reset_drng(ctx, 0x128301u) != 0) {
        return -1;
    }
    if (kem_keygen(ctx->pk, &pk_len, ctx->sk, &sk_len) != 0) {
        return -1;
    }
    if (reset_drng(ctx, 0x128401u) != 0) {
        return -1;
    }
    if (kem_enc(ctx->pk, pk_len, ctx->ss, &ss_len, ctx->ct, &ct_len) != 0) {
        return -1;
    }
    if (kem_dec(ctx->sk, sk_len, ctx->ct, ct_len, ctx->dec_ss, &ss_len) != 0) {
        return -1;
    }
    if (memcmp(ctx->ss, ctx->dec_ss, ZEN_SHAREDKEY_LEN_BYTES) != 0) {
        fprintf(stderr, "randomized KEM round-trip mismatch\n");
        return -1;
    }
    summary->randomized_round_trip = 1;
    return 0;
}

static int op_kem_keygen(bench_ctx *ctx)
{
    unsigned long long pk_len = ZEN_PUBLICKEY_LEN_BYTES;
    unsigned long long sk_len = ZEN_SECREKEY_LEN_BYTES;
    int rc = kem_keygen(ctx->pk, &pk_len, ctx->sk, &sk_len);
    sink ^= ctx->pk[0];
    sink ^= ctx->sk[ZEN_SECREKEY_LEN_BYTES - 1];
    return rc;
}

static int op_kem_keygen_derand(bench_ctx *ctx)
{
    unsigned long long pk_len = ZEN_PUBLICKEY_LEN_BYTES;
    unsigned long long sk_len = ZEN_SECREKEY_LEN_BYTES;
    int rc = kem_keygen_derand(ctx->pk, &pk_len, ctx->sk, &sk_len,
                               ctx->derand_seed);
    sink ^= ctx->pk[0];
    sink ^= ctx->sk[ZEN_SECREKEY_LEN_BYTES - 1];
    return rc;
}

static int op_kem_enc(bench_ctx *ctx)
{
    unsigned long long ss_len = ZEN_SHAREDKEY_LEN_BYTES;
    unsigned long long ct_len = ZEN_CIPHERTEXT_LEN_BYTES;
    int rc = kem_enc(ctx->pk, ZEN_PUBLICKEY_LEN_BYTES, ctx->ss, &ss_len,
                     ctx->ct, &ct_len);
    sink ^= ctx->ss[0];
    sink ^= ctx->ct[ZEN_CIPHERTEXT_LEN_BYTES - 1];
    return rc;
}

static int op_kem_dec(bench_ctx *ctx)
{
    unsigned long long ss_len = ZEN_SHAREDKEY_LEN_BYTES;
    int rc = kem_dec(ctx->sk, ZEN_SECREKEY_LEN_BYTES, ctx->ct,
                     ZEN_CIPHERTEXT_LEN_BYTES, ctx->dec_ss, &ss_len);
    sink ^= ctx->dec_ss[0];
    sink ^= ctx->dec_ss[ZEN_SHAREDKEY_LEN_BYTES - 1];
    return rc;
}

static int validate_after_operation(const char *operation, const bench_ctx *ctx)
{
    if (strcmp(operation, "kem_dec") == 0 &&
        memcmp(ctx->ss, ctx->dec_ss, ZEN_SHAREDKEY_LEN_BYTES) != 0) {
        return -1;
    }
    return 0;
}

static void summarize_samples(const char *operation, const uint64_t *samples,
                              uint64_t iterations, uint64_t cpu_hz,
                              unsigned int failures, result_summary *summary)
{
    uint64_t *sorted = (uint64_t *)malloc((size_t)iterations * sizeof(*sorted));
    uint64_t i;
    long double total = 0.0L;

    memset(summary, 0, sizeof(*summary));
    summary->operation = operation;
    summary->iterations = iterations;
    summary->failures = failures;
    if (sorted == NULL) {
        summary->failures++;
        return;
    }

    memcpy(sorted, samples, (size_t)iterations * sizeof(*sorted));
    qsort(sorted, (size_t)iterations, sizeof(*sorted), compare_u64);
    for (i = 0; i < iterations; i++) {
        total += (long double)samples[i];
    }

    summary->min_ns = sorted[0];
    summary->median_ns = sorted[iterations / 2U];
    summary->max_ns = sorted[iterations - 1U];
    summary->average_ns = total / (long double)iterations;
    summary->average_cycles = cpu_hz == 0 ? 0.0L :
        summary->average_ns * (long double)cpu_hz / 1000000000.0L;
    summary->ops_per_second = summary->average_ns == 0.0L ? 0.0L :
        1000000000.0L / summary->average_ns;
    free(sorted);
}

static int run_operation(const char *operation, operation_fn fn, uint32_t drng_domain,
                         bench_ctx *ctx, uint64_t iterations, uint64_t timer_overhead,
                         uint64_t cpu_hz, FILE *csv, result_summary *summary)
{
    uint64_t *samples = (uint64_t *)malloc((size_t)iterations * sizeof(*samples));
    uint64_t i;
    unsigned int failures = 0;

    if (samples == NULL) {
        fprintf(stderr, "failed to allocate benchmark samples for %s\n", operation);
        return -1;
    }
    if (prepare_fixed_kem(ctx) != 0 || reset_drng(ctx, drng_domain) != 0) {
        free(samples);
        return -1;
    }

    for (i = 0; i < BENCH_WARMUP_ITERATIONS; i++) {
        if (fn(ctx) != 0) {
            failures++;
        }
    }

    for (i = 0; i < iterations; i++) {
        uint64_t start = now_ns();
        uint64_t elapsed;
        int rc = fn(ctx);
        uint64_t end = now_ns();
        int sample_failed = 0;
        if (end < start) {
            elapsed = 0;
            sample_failed = 1;
        } else {
            elapsed = end - start;
            if (elapsed > timer_overhead) {
                elapsed -= timer_overhead;
            }
        }
        if (rc != 0 || validate_after_operation(operation, ctx) != 0) {
            sample_failed = 1;
        }
        if (sample_failed) {
            failures++;
        }
        samples[i] = elapsed;
        fprintf(csv,
                "%s,key_encapsulation,%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64
                ",%llu,%llu,%llu,%llu,%s\n",
                ALGORITHM_INSTANCE,
                operation,
                i + 1U,
                elapsed,
                estimate_cycles(elapsed, cpu_hz),
                (unsigned long long)ZEN_PUBLICKEY_LEN_BYTES,
                (unsigned long long)ZEN_SECREKEY_LEN_BYTES,
                (unsigned long long)ZEN_CIPHERTEXT_LEN_BYTES,
                (unsigned long long)ZEN_SHAREDKEY_LEN_BYTES,
                sample_failed ? "fail" : "pass");
    }

    summarize_samples(operation, samples, iterations, cpu_hz, failures, summary);
    free(samples);
    return failures == 0 ? 0 : -1;
}

static uint64_t parse_iterations(int argc, char **argv)
{
    uint64_t iterations = BENCH_DEFAULT_ITERATIONS;

    if (argc > 1) {
        iterations = parse_u64_text(argv[1]);
        if (iterations == 0) {
            fprintf(stderr, "usage: %s [iterations>=100]\n", argv[0]);
            exit(2);
        }
    }
    if (iterations < BENCH_MIN_ITERATIONS) {
        fprintf(stderr, "iterations raised to %" PRIu64 " to satisfy NGCC self-evaluation guidance\n",
                (uint64_t)BENCH_MIN_ITERATIONS);
        iterations = BENCH_MIN_ITERATIONS;
    }
    return iterations;
}

static void write_json_summary(FILE *json, const result_summary *summaries,
                               size_t summary_count, uint64_t iterations,
                               uint64_t start_rss, uint64_t peak_rss,
                               cpu_frequency cpu_info,
                               static_memory_info static_info,
                               const environment_info *env,
                               const functional_summary *functional,
                               const char *csv_path,
                               const char *json_path)
{
    size_t i;

    fprintf(json, "{\n");
    fprintf(json, "  \"assessment_guidance\": \"NGCC ARM architecture implementation self-evaluation guidance, June 2026\",\n");
    fprintf(json, "  \"algorithm_category\": \"public-key cryptography\",\n");
    fprintf(json, "  \"algorithm_name\": \"ZEN\",\n");
    fprintf(json, "  \"sub_function\": \"key encapsulation\",\n");
    fprintf(json, "  \"implementation_version\": \"ARM performance optimized\",\n");
    fprintf(json, "  \"algorithm_instance\": \"%s\",\n", ALGORITHM_INSTANCE);
    fprintf(json, "  \"security_level_or_parameter_set\": \"%s\",\n", ALGORITHM_INSTANCE);
    fprintf(json, "  \"iterations\": %" PRIu64 ",\n", iterations);
    fprintf(json, "  \"warmup_iterations\": %" PRIu64 ",\n", (uint64_t)BENCH_WARMUP_ITERATIONS);
    fprintf(json, "  \"minimum_required_iterations\": %" PRIu64 ",\n", (uint64_t)BENCH_MIN_ITERATIONS);
    fprintf(json, "  \"start_time_utc\": ");
    json_write_string(json, env->start_time_utc);
    fprintf(json, ",\n");
    fprintf(json, "  \"end_time_utc\": ");
    json_write_string(json, env->end_time_utc);
    fprintf(json, ",\n");
    fprintf(json, "  \"environment\": {\n");
    fprintf(json, "    \"os_release\": ");
    json_write_string(json, env->os_release);
    fprintf(json, ",\n");
    fprintf(json, "    \"kernel\": ");
    json_write_string(json, env->kernel);
    fprintf(json, ",\n");
    fprintf(json, "    \"machine\": ");
    json_write_string(json, env->machine);
    fprintf(json, ",\n");
    fprintf(json, "    \"cpu_model\": ");
    json_write_string(json, env->cpu_model);
    fprintf(json, ",\n");
    fprintf(json, "    \"cpu_features\": ");
    json_write_string(json, env->cpu_features);
    fprintf(json, ",\n");
    fprintf(json, "    \"online_cpus\": %ld,\n", env->online_cpus);
    fprintf(json, "    \"mem_total_bytes\": %" PRIu64 ",\n", env->mem_total_bytes);
    fprintf(json, "  \"cpu_frequency_hz\": %" PRIu64 ",\n", cpu_info.hz);
    fprintf(json, "  \"cpu_frequency_source\": \"%s\",\n", cpu_info.source);
    fprintf(json, "    \"single_threaded_benchmark\": true,\n");
    fprintf(json, "    \"pin_cpu_env\": \"ZEN_BENCH_PIN_CPU\",\n");
    fprintf(json, "    \"pin_cpu_requested\": %s,\n", env->pin_cpu_requested ? "true" : "false");
    fprintf(json, "    \"pin_cpu_result\": %d\n", env->pin_cpu_result);
    fprintf(json, "  },\n");
    fprintf(json, "  \"build_and_dependency_info\": {\n");
    fprintf(json, "    \"compiler\": ");
    json_write_string(json, env->compiler);
    fprintf(json, ",\n");
    fprintf(json, "    \"build_system\": \"Makefile; CMake not used by this package\",\n");
    fprintf(json, "    \"default_makefile_cflags\": ");
    json_write_string(json, BENCH_DEFAULT_CFLAGS);
    fprintf(json, ",\n");
    fprintf(json, "    \"target_instruction_set\": ");
    json_write_string(json, BENCH_TARGET_ISA);
    fprintf(json, ",\n");
    fprintf(json, "    \"optimization_methods\": \"AArch64 assembly, NEON SIMD, Crypto/PMULL and SHA3 EOR3 instructions, loop and data-layout optimizations\",\n");
    fprintf(json, "    \"third_party_dependencies\": \"none; uses submitted auxfunc.c and drng.c from the NGCC programming interface package\",\n");
    fprintf(json, "    \"randomness_method\": \"DRNG from drng.c initialized with fixed seeds generated by fill_seed(domain)\"\n");
    fprintf(json, "  },\n");
    fprintf(json, "  \"cycles_note\": \"%s\",\n",
            cpu_info.hz == 0 ? "cycles fields are zero because no CPU frequency was provided or detected" :
                               "cycles are estimated as elapsed_seconds multiplied by cpu_frequency_hz");
    fprintf(json, "  \"test_data_generation\": \"fixed DRNG seeds generated by benchmark.c fill_seed(domain); KEM randomness comes from auxfunc/drng through the submitted programming interface\",\n");
    fprintf(json, "  \"functional_tests\": {\n");
    fprintf(json, "    \"interface_length_getters\": \"%s\",\n", functional->length_getters ? "pass" : "fail");
    fprintf(json, "    \"fixed_seed_kem_round_trip\": \"%s\",\n", functional->fixed_seed_round_trip ? "pass" : "fail");
    fprintf(json, "    \"drng_seeded_kem_round_trip\": \"%s\",\n", functional->randomized_round_trip ? "pass" : "fail");
    fprintf(json, "    \"kat_vector_source\": \"KAT_KEM.c\",\n");
    fprintf(json, "    \"kat_generation_command\": \"make KAT_KEM && ./KAT_KEM\"\n");
    fprintf(json, "  },\n");
    fprintf(json, "  \"sizes_bytes\": {\n");
    fprintf(json, "    \"public_key\": %llu,\n", (unsigned long long)ZEN_PUBLICKEY_LEN_BYTES);
    fprintf(json, "    \"secret_key\": %llu,\n", (unsigned long long)ZEN_SECREKEY_LEN_BYTES);
    fprintf(json, "    \"ciphertext\": %llu,\n", (unsigned long long)ZEN_CIPHERTEXT_LEN_BYTES);
    fprintf(json, "    \"shared_secret\": %llu,\n", (unsigned long long)ZEN_SHAREDKEY_LEN_BYTES);
    fprintf(json, "    \"signature\": \"not_applicable\",\n");
    fprintf(json, "    \"key_exchange_message\": \"not_applicable\"\n");
    fprintf(json, "  },\n");
    fprintf(json, "  \"interaction_rounds\": {\n");
    fprintf(json, "    \"key_encapsulation\": \"not_applicable\",\n");
    fprintf(json, "    \"key_exchange\": \"not_submitted_in_this_package\"\n");
    fprintf(json, "  },\n");
    fprintf(json, "  \"memory_bytes\": {\n");
    fprintf(json, "    \"static_text_rodata\": %" PRIu64 ",\n", static_info.text_rodata_bytes);
    fprintf(json, "    \"static_writable\": %" PRIu64 ",\n", static_info.writable_static_bytes);
    fprintf(json, "    \"static_total\": %" PRIu64 ",\n", static_info.total_static_bytes);
    fprintf(json, "    \"static_memory_source\": ");
    json_write_string(json, static_info.source);
    fprintf(json, ",\n");
    fprintf(json, "    \"rss_at_start\": %" PRIu64 ",\n", start_rss);
    fprintf(json, "    \"peak_rss_after_tests\": %" PRIu64 ",\n", peak_rss);
    fprintf(json, "    \"benchmark_context\": %llu\n", (unsigned long long)sizeof(bench_ctx));
    fprintf(json, "  },\n");
    fprintf(json, "  \"evidence_index\": {\n");
    fprintf(json, "    \"build_command\": \"make benchmark CC=gcc CFLAGS=\\\"" BENCH_DEFAULT_CFLAGS "\\\"\",\n");
    fprintf(json, "    \"run_command\": \"ZEN_BENCH_PIN_CPU=0 ZEN_BENCH_CPU_HZ=<cpu_frequency_hz> ./benchmark <iterations>\",\n");
    fprintf(json, "    \"kat_command\": \"make KAT_KEM && ./KAT_KEM\",\n");
    fprintf(json, "    \"raw_csv\": ");
    json_write_string(json, csv_path);
    fprintf(json, ",\n");
    fprintf(json, "    \"summary_json\": ");
    json_write_string(json, json_path);
    fprintf(json, ",\n");
    fprintf(json, "    \"calculation\": \"average_ns is arithmetic mean over raw CSV rows; average_cycles_estimated = average_ns * cpu_frequency_hz / 1e9; ops_per_second = 1e9 / average_ns\"\n");
    fprintf(json, "  },\n");
    fprintf(json, "  \"results\": [\n");
    for (i = 0; i < summary_count; i++) {
        const result_summary *s = &summaries[i];
        fprintf(json, "    {\n");
        fprintf(json, "      \"operation\": \"%s\",\n", s->operation);
        fprintf(json, "      \"iterations\": %" PRIu64 ",\n", s->iterations);
        fprintf(json, "      \"average_ns\": %.2Lf,\n", s->average_ns);
        fprintf(json, "      \"median_ns\": %" PRIu64 ",\n", s->median_ns);
        fprintf(json, "      \"min_ns\": %" PRIu64 ",\n", s->min_ns);
        fprintf(json, "      \"max_ns\": %" PRIu64 ",\n", s->max_ns);
        fprintf(json, "      \"average_cycles_estimated\": %.2Lf,\n", s->average_cycles);
        fprintf(json, "      \"ops_per_second\": %.2Lf,\n", s->ops_per_second);
        fprintf(json, "      \"failures\": %u\n", s->failures);
        fprintf(json, "    }%s\n", i + 1U == summary_count ? "" : ",");
    }
    fprintf(json, "  ]\n");
    fprintf(json, "}\n");
}

static void print_summary_table(const result_summary *summaries, size_t count,
                                cpu_frequency cpu_info)
{
    size_t i;

    printf("NGCC ARM KEM self-evaluation benchmark: %s\n", ALGORITHM_INSTANCE);
    printf("cpu_frequency_hz: %" PRIu64 " (%s)\n", cpu_info.hz, cpu_info.source);
    printf("sizes: pk=%llu sk=%llu ct=%llu ss=%llu bytes\n",
           (unsigned long long)ZEN_PUBLICKEY_LEN_BYTES,
           (unsigned long long)ZEN_SECREKEY_LEN_BYTES,
           (unsigned long long)ZEN_CIPHERTEXT_LEN_BYTES,
           (unsigned long long)ZEN_SHAREDKEY_LEN_BYTES);
    printf("%-20s %12s %12s %12s %14s %14s\n",
           "operation", "avg ns", "median ns", "avg cycles", "ops/s", "failures");
    for (i = 0; i < count; i++) {
        printf("%-20s %12.2Lf %12" PRIu64 " %12.2Lf %14.2Lf %14u\n",
               summaries[i].operation,
               summaries[i].average_ns,
               summaries[i].median_ns,
               summaries[i].average_cycles,
               summaries[i].ops_per_second,
               summaries[i].failures);
    }
}

int main(int argc, char **argv)
{
    const char *csv_path = "benchmark_" ALGORITHM_INSTANCE "_raw.csv";
    const char *json_path = "benchmark_" ALGORITHM_INSTANCE "_summary.json";
    uint64_t iterations = parse_iterations(argc, argv);
    environment_info env;
    functional_summary functional;
    static_memory_info static_info;
    uint64_t start_rss;
    uint64_t timer_overhead;
    cpu_frequency cpu_info;
    result_summary summaries[4];
    bench_ctx *ctx;
    FILE *csv;
    FILE *json;
    int status = 0;

    collect_environment(&env);
    static_info = get_static_memory_info();
    start_rss = get_peak_rss_bytes();
    timer_overhead = estimate_timer_overhead_ns();
    cpu_info = detect_cpu_frequency();
    ctx = (bench_ctx *)calloc(1, sizeof(*ctx));

    if (ctx == NULL) {
        fprintf(stderr, "failed to allocate benchmark context\n");
        return 1;
    }
    if (functional_tests(ctx, &functional) != 0) {
        free(ctx);
        return 1;
    }

    csv = fopen(csv_path, "w");
    if (csv == NULL) {
        fprintf(stderr, "failed to open %s\n", csv_path);
        free(ctx);
        return 1;
    }
    fprintf(csv,
            "algorithm_instance,function,operation,iteration,elapsed_ns,cycles_estimated,"
            "public_key_bytes,secret_key_bytes,ciphertext_bytes,shared_secret_bytes,result\n");

    if (run_operation("kem_keygen", op_kem_keygen, 0x128501u, ctx, iterations,
                      timer_overhead, cpu_info.hz, csv, &summaries[0]) != 0) {
        status = 1;
    }
    if (run_operation("kem_keygen_derand", op_kem_keygen_derand, 0x128601u, ctx,
                      iterations, timer_overhead, cpu_info.hz, csv,
                      &summaries[1]) != 0) {
        status = 1;
    }
    if (run_operation("kem_enc", op_kem_enc, 0x128701u, ctx, iterations,
                      timer_overhead, cpu_info.hz, csv, &summaries[2]) != 0) {
        status = 1;
    }
    if (run_operation("kem_dec", op_kem_dec, 0x128801u, ctx, iterations,
                      timer_overhead, cpu_info.hz, csv, &summaries[3]) != 0) {
        status = 1;
    }
    fclose(csv);
    format_utc_time(env.end_time_utc, sizeof(env.end_time_utc));

    json = fopen(json_path, "w");
    if (json == NULL) {
        fprintf(stderr, "failed to open %s\n", json_path);
        free(ctx);
        return 1;
    }
    write_json_summary(json, summaries, 4U, iterations, start_rss,
                       get_peak_rss_bytes(), cpu_info, static_info,
                       &env, &functional, csv_path, json_path);
    fclose(json);

    print_summary_table(summaries, 4U, cpu_info);
    printf("static_memory_bytes: %" PRIu64 " (%s)\n",
           static_info.total_static_bytes, static_info.source);
    printf("raw_csv: %s\n", csv_path);
    printf("summary_json: %s\n", json_path);
    printf("timer_overhead_ns: %" PRIu64 "\n", timer_overhead);
    printf("peak_rss_bytes: %" PRIu64 "\n", get_peak_rss_bytes());
    printf("pin_cpu_result: %d\n", env.pin_cpu_result);
    printf("sink: %" PRIu64 "\n", sink);
    if (cpu_info.hz == 0) {
        puts("warning: set ZEN_BENCH_CPU_HZ=<cpu_frequency_hz> to populate cycles for the official report");
    }

    free(ctx);
    return status;
}
