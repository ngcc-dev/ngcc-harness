/*
 * HARE KEM x86 self-evaluation benchmark driver.
 *
 * Scope:
 * - Benchmark helper only; this file is not part of the reference algorithm core.
 * - Splits key generation, encapsulation, and decapsulation timing.
 * - Uses a deterministic two-level input model:
 *     instance_count independent deterministic initializations,
 *     repeat_count timed operations under each initialization.
 * - Reports avg/min/max/median/stddev, ops/s, cycles/op when x86 TSC is available,
 *   KEM data sizes, deterministic seed model, and process peak RSS proxy.
 *
 * The driver is written for the HARE KR submission line. It
 * records both aggregate statistics and optional raw per-sample CSV data for
 * auditability.
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>

#include "KEM_AlgorithmInstance.h"
#include "symmetric.h"

#ifdef HARE_SYMMETRIC_MODE_B
#include "drng.h"
DRNG_ctx drng_algorithm;
#endif

#ifndef HARE_INSTANCE_NAME
#define HARE_INSTANCE_NAME "unknown"
#endif

#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#define HARE_HAS_X86_TSC 1
#else
#define HARE_HAS_X86_TSC 0
#endif

typedef struct {
    double avg;
    double min;
    double max;
    double median;
    double p05;
    double p95;
    double mad;
    double trimmed_mean;
    double stddev;
} hare_sample_stats_t;

typedef struct {
    hare_sample_stats_t us;
    hare_sample_stats_t cycles;
    int has_cycles;
    double ops_per_sec;
    double cycles_per_op;
} hare_op_stats_t;

typedef struct {
    unsigned int instances;
    unsigned int repeats;
    unsigned int warmup_repeats;
    unsigned int total_samples;
} hare_count_model_t;

/**
 * Return elapsed process CPU time in microseconds for a clock() interval.
 */
static double elapsed_us(clock_t start, clock_t end) {
    const double seconds = (double)(end - start) / (double)CLOCKS_PER_SEC;
    return seconds * 1000000.0;
}

/**
 * qsort comparator for finite double samples.
 */
static int cmp_double(const void *a, const void *b) {
    const double da = *(const double *)a;
    const double db = *(const double *)b;
    if (da < db) {
        return -1;
    }
    if (da > db) {
        return 1;
    }
    return 0;
}

/**
 * Compute a median from a sorted sample array.
 */
static double median_sorted(const double *sorted, unsigned int count) {
    if ((count % 2u) == 0u) {
        return (sorted[count / 2u - 1u] + sorted[count / 2u]) / 2.0;
    }
    return sorted[count / 2u];
}

/**
 * Compute a nearest-rank percentile from a sorted sample array.
 */
static double percentile_sorted(const double *sorted, unsigned int count, double percentile) {
    unsigned int idx;
    if (count == 1u) {
        return sorted[0];
    }
    idx = (unsigned int)((double)(count - 1u) * percentile + 0.5);
    if (idx >= count) {
        idx = count - 1u;
    }
    return sorted[idx];
}

/**
 * Compute average, range, median, p05/p95, MAD, trimmed mean and stddev.
 */
static hare_sample_stats_t compute_stats(const double *samples, unsigned int count) {
    hare_sample_stats_t s;
    double sum = 0.0;
    double var_acc = 0.0;
    double trimmed_sum = 0.0;
    unsigned int trim;
    unsigned int trim_begin;
    unsigned int trim_end;
    double *tmp;
    double *abs_dev;

    if (samples == NULL || count == 0u) {
        fprintf(stderr, "invalid sample vector\n");
        exit(90);
    }

    tmp = (double *)malloc((size_t)count * sizeof(double));
    abs_dev = (double *)malloc((size_t)count * sizeof(double));
    if (tmp == NULL || abs_dev == NULL) {
        fprintf(stderr, "allocation failure in compute_stats\n");
        free(tmp);
        free(abs_dev);
        exit(91);
    }

    s.min = samples[0];
    s.max = samples[0];
    for (unsigned int i = 0; i < count; ++i) {
        const double v = samples[i];
        if (v < s.min) {
            s.min = v;
        }
        if (v > s.max) {
            s.max = v;
        }
        sum += v;
        tmp[i] = v;
    }
    s.avg = sum / (double)count;

    qsort(tmp, (size_t)count, sizeof(double), cmp_double);
    s.median = median_sorted(tmp, count);
    s.p05 = percentile_sorted(tmp, count, 0.05);
    s.p95 = percentile_sorted(tmp, count, 0.95);

    trim = count / 20u;
    trim_begin = trim;
    trim_end = count - trim;
    if (trim_begin >= trim_end) {
        trim_begin = 0u;
        trim_end = count;
    }
    for (unsigned int i = trim_begin; i < trim_end; ++i) {
        trimmed_sum += tmp[i];
    }
    s.trimmed_mean = trimmed_sum / (double)(trim_end - trim_begin);

    for (unsigned int i = 0; i < count; ++i) {
        const double d = samples[i] - s.avg;
        var_acc += d * d;
        abs_dev[i] = fabs(samples[i] - s.median);
    }
    qsort(abs_dev, (size_t)count, sizeof(double), cmp_double);
    s.mad = median_sorted(abs_dev, count);
    s.stddev = sqrt(var_acc / (double)count);

    free(tmp);
    free(abs_dev);
    return s;
}
static uint64_t read_cycles_begin(void) {
#if HARE_HAS_X86_TSC
    _mm_lfence();
    return __rdtsc();
#else
    return 0u;
#endif
}

/**
 * Read an ordered TSC end value when x86 TSC is available.
 */
static uint64_t read_cycles_end(void) {
#if HARE_HAS_X86_TSC
    uint64_t t;
    _mm_lfence();
    t = __rdtsc();
    _mm_lfence();
    return t;
#else
    return 0u;
#endif
}

/**
 * Return process peak RSS in bytes using getrusage().
 */
static unsigned long long get_peak_rss_bytes(void) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0ULL;
    }
#if defined(__APPLE__)
    return (unsigned long long)usage.ru_maxrss;
#else
    return (unsigned long long)usage.ru_maxrss * 1024ULL;
#endif
}

/**
 * Generate a deterministic SplitMix64 word for benchmark seed derivation.
 */
static uint64_t splitmix64_next(uint64_t *state) {
    uint64_t z;
    *state += 0x9E3779B97F4A7C15ULL;
    z = *state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/**
 * Parse a positive uint32-like value from an environment variable.
 */
static unsigned int read_env_u32(const char *name, unsigned int default_value) {
    const char *value = getenv(name);
    char *endptr = NULL;
    unsigned long parsed;

    if (value == NULL || value[0] == '\0') {
        return default_value;
    }

    parsed = strtoul(value, &endptr, 10);
    if (endptr == value || *endptr != '\0' || parsed == 0UL || parsed > 1000000UL) {
        return default_value;
    }
    return (unsigned int)parsed;
}

/**
 * Parse a positive uint32-like value from an argv slot.
 */
static unsigned int read_arg_u32(int argc, char **argv, int index, unsigned int default_value) {
    char *endptr = NULL;
    unsigned long parsed;

    if (argc <= index || argv[index] == NULL || argv[index][0] == '\0') {
        return default_value;
    }

    parsed = strtoul(argv[index], &endptr, 10);
    if (endptr == argv[index] || *endptr != '\0' || parsed == 0UL || parsed > 1000000UL) {
        return default_value;
    }
    return (unsigned int)parsed;
}

/**
 * Derive the deterministic master seed associated with one benchmark instance.
 */
static void build_master_instance_seed(unsigned int instance_id, unsigned char master_seed[32]) {
    uint64_t s = 0x484152455F4E4743ULL ^ ((uint64_t)instance_id * 0xD1342543DE82EF95ULL);

    for (size_t i = 0; i < 32u; ++i) {
        master_seed[i] = (unsigned char)splitmix64_next(&s);
    }
}

/**
 * Derive entropy and personalization inputs for one labelled benchmark phase.
 */
static void derive_seed_pair_from_master(const unsigned char master_seed[32],
                                         unsigned char label,
                                         unsigned char entropy[48],
                                         unsigned char personalization[48]) {
    uint64_t s = 0x42454E43485F4B45ULL ^ (uint64_t)label;

    for (size_t i = 0; i < 32u; ++i) {
        s ^= ((uint64_t)master_seed[i] << ((i % 8u) * 8u));
        (void)splitmix64_next(&s);
    }

    for (size_t i = 0; i < 48u; ++i) {
        entropy[i] = (unsigned char)(splitmix64_next(&s) ^ (uint64_t)(label + (unsigned char)i));
        personalization[i] = (unsigned char)(splitmix64_next(&s) ^ (uint64_t)(label * 3u + (unsigned char)(i * 7u)));
    }
}

/**
 * Initialize the package PRNG and, when compiled, the API_PKC DRNG shim.
 */
static int init_prng_from_seed_pair(unsigned char entropy[48], unsigned char personalization[48]) {
    prng_init(entropy, personalization, 48u, 48u);
#ifdef HARE_SYMMETRIC_MODE_B
    if (init_random_number(&drng_algorithm, entropy, 48u) != 0) {
        return -1;
    }
#endif
    return 0;
}

/**
 * Print a named 32-byte seed as lowercase hexadecimal.
 */
static void print_hex_seed(const char *name, const unsigned char seed[32]) {
    printf("%s=", name);
    for (size_t i = 0; i < 32u; ++i) {
        printf("%02x", seed[i]);
    }
    printf("\n");
}

/**
 * Print aggregate statistics for one benchmarked operation.
 */
static void print_op_stats(const char *name, const hare_op_stats_t *s) {
    printf("%s_avg_us=%.3f\n", name, s->us.avg);
    printf("%s_min_us=%.3f\n", name, s->us.min);
    printf("%s_max_us=%.3f\n", name, s->us.max);
    printf("%s_median_us=%.3f\n", name, s->us.median);
    printf("%s_p05_us=%.3f\n", name, s->us.p05);
    printf("%s_p95_us=%.3f\n", name, s->us.p95);
    printf("%s_mad_us=%.3f\n", name, s->us.mad);
    printf("%s_trimmed_mean_us=%.3f\n", name, s->us.trimmed_mean);
    printf("%s_stddev_us=%.3f\n", name, s->us.stddev);
    printf("%s_ops_per_sec=%.6f\n", name, s->ops_per_sec);

    if (s->has_cycles) {
        printf("%s_avg_cycles=%.3f\n", name, s->cycles.avg);
        printf("%s_min_cycles=%.3f\n", name, s->cycles.min);
        printf("%s_max_cycles=%.3f\n", name, s->cycles.max);
        printf("%s_median_cycles=%.3f\n", name, s->cycles.median);
        printf("%s_p05_cycles=%.3f\n", name, s->cycles.p05);
        printf("%s_p95_cycles=%.3f\n", name, s->cycles.p95);
        printf("%s_mad_cycles=%.3f\n", name, s->cycles.mad);
        printf("%s_trimmed_mean_cycles=%.3f\n", name, s->cycles.trimmed_mean);
        printf("%s_stddev_cycles=%.3f\n", name, s->cycles.stddev);
        printf("%s_cycles_per_op=%.3f\n", name, s->cycles_per_op);
    } else {
        printf("%s_avg_cycles=N/A\n", name);
        printf("%s_min_cycles=N/A\n", name);
        printf("%s_max_cycles=N/A\n", name);
        printf("%s_median_cycles=N/A\n", name);
        printf("%s_p05_cycles=N/A\n", name);
        printf("%s_p95_cycles=N/A\n", name);
        printf("%s_mad_cycles=N/A\n", name);
        printf("%s_trimmed_mean_cycles=N/A\n", name);
        printf("%s_stddev_cycles=N/A\n", name);
        printf("%s_cycles_per_op=N/A\n", name);
    }
}

/**
 * Convert raw timing samples into final operation statistics.
 */
static void finalize_op_stats(hare_op_stats_t *out,
                              const double *us_samples,
                              const double *cycle_samples,
                              unsigned int total_samples) {
    out->us = compute_stats(us_samples, total_samples);
    out->has_cycles = HARE_HAS_X86_TSC;
    out->ops_per_sec = (out->us.avg > 0.0) ? (1000000.0 / out->us.avg) : 0.0;
    out->cycles_per_op = 0.0;

    if (HARE_HAS_X86_TSC) {
        out->cycles = compute_stats(cycle_samples, total_samples);
        out->cycles_per_op = out->cycles.avg;
    } else {
        memset(&out->cycles, 0, sizeof(out->cycles));
    }
}

/**
 * Reject zero KEM buffer sizes before allocating benchmark buffers.
 */
static int checked_lengths(unsigned long long pk_len,
                           unsigned long long sk_len,
                           unsigned long long ct_len,
                           unsigned long long ss_len) {
    if (pk_len == 0ULL || sk_len == 0ULL || ct_len == 0ULL || ss_len == 0ULL) {
        fprintf(stderr, "invalid zero KEM length\n");
        return -1;
    }
    return 0;
}

/**
 * Write raw per-sample timing data for independent audit of aggregate metrics.
 */
static void write_raw_samples(const char *path,
                              const double *keygen_us, const double *keygen_cycles,
                              const double *enc_us, const double *enc_cycles,
                              const double *dec_us, const double *dec_cycles,
                              unsigned int instances, unsigned int repeats) {
    FILE *fp;

    if (path == NULL || path[0] == '\0') {
        return;
    }

    fp = fopen(path, "w");
    if (fp == NULL) {
        fprintf(stderr, "failed to open raw sample CSV: %s\n", path);
        exit(92);
    }

    fprintf(fp, "operation,sample_index,instance_index,repeat_index,us,cycles\n");
    for (unsigned int inst = 0; inst < instances; ++inst) {
        for (unsigned int rep = 0; rep < repeats; ++rep) {
            const unsigned int idx = inst * repeats + rep;
            fprintf(fp, "keygen,%u,%u,%u,%.3f,%.3f\n", idx, inst, rep, keygen_us[idx], keygen_cycles[idx]);
            fprintf(fp, "encaps,%u,%u,%u,%.3f,%.3f\n", idx, inst, rep, enc_us[idx], enc_cycles[idx]);
            fprintf(fp, "decaps,%u,%u,%u,%.3f,%.3f\n", idx, inst, rep, dec_us[idx], dec_cycles[idx]);
        }
    }

    fclose(fp);
}

/**
 * Run deterministic KEM self-evaluation for the selected algorithm instance.
 */
int main(int argc, char **argv) {
    const unsigned int default_instances = read_env_u32("HARE_BENCH_INSTANCES", 10u);
    const unsigned int default_repeats = read_env_u32("HARE_BENCH_REPEATS", 100u);
    const unsigned int instances = read_arg_u32(argc, argv, 1, default_instances);
    const unsigned int repeats = read_arg_u32(argc, argv, 2, default_repeats);
    const unsigned int warmup_repeats = read_env_u32("HARE_BENCH_WARMUP_REPEATS", 1u);
    const unsigned int total_samples = instances * repeats;
    const hare_count_model_t count_model = { instances, repeats, warmup_repeats, total_samples };

    unsigned long long pk_len = kem_get_pk_len_bytes();
    unsigned long long sk_len = kem_get_sk_len_bytes();
    unsigned long long ct_len = kem_get_ct_len_bytes();
    unsigned long long ss_len = kem_get_ss_len_bytes();

    unsigned char *pk;
    unsigned char *sk;
    unsigned char *ct;
    unsigned char *ss;
    unsigned char *ss_out;
    unsigned char *keypair_bank_pk;
    unsigned char *keypair_bank_sk;
    unsigned char *dec_inputs;
    unsigned char *dec_expected;
    unsigned char *dec_outputs;
    int *dec_status;
    double *keygen_us;
    double *enc_us;
    double *dec_us;
    double *keygen_cycles;
    double *enc_cycles;
    double *dec_cycles;
    unsigned int dec_ok = 0;
    unsigned char first_master_seed[32];
    hare_op_stats_t keygen_stats;
    hare_op_stats_t enc_stats;
    hare_op_stats_t dec_stats;

    if (checked_lengths(pk_len, sk_len, ct_len, ss_len) != 0) {
        return 2;
    }
    if (instances == 0u || repeats == 0u || total_samples / repeats != instances) {
        fprintf(stderr, "invalid benchmark count model\n");
        return 3;
    }

    pk = (unsigned char *)calloc((size_t)pk_len, 1u);
    sk = (unsigned char *)calloc((size_t)sk_len, 1u);
    ct = (unsigned char *)calloc((size_t)ct_len, 1u);
    ss = (unsigned char *)calloc((size_t)ss_len, 1u);
    ss_out = (unsigned char *)calloc((size_t)ss_len, 1u);
    keypair_bank_pk = (unsigned char *)calloc((size_t)instances * (size_t)pk_len, 1u);
    keypair_bank_sk = (unsigned char *)calloc((size_t)instances * (size_t)sk_len, 1u);
    dec_inputs = (unsigned char *)calloc((size_t)repeats * (size_t)ct_len, 1u);
    dec_expected = (unsigned char *)calloc((size_t)repeats * (size_t)ss_len, 1u);
    dec_outputs = (unsigned char *)calloc((size_t)repeats * (size_t)ss_len, 1u);
    dec_status = (int *)calloc((size_t)repeats, sizeof(int));
    keygen_us = (double *)calloc((size_t)total_samples, sizeof(double));
    enc_us = (double *)calloc((size_t)total_samples, sizeof(double));
    dec_us = (double *)calloc((size_t)total_samples, sizeof(double));
    keygen_cycles = (double *)calloc((size_t)total_samples, sizeof(double));
    enc_cycles = (double *)calloc((size_t)total_samples, sizeof(double));
    dec_cycles = (double *)calloc((size_t)total_samples, sizeof(double));

    if (pk == NULL || sk == NULL || ct == NULL || ss == NULL || ss_out == NULL ||
        keypair_bank_pk == NULL || keypair_bank_sk == NULL || dec_inputs == NULL ||
        dec_expected == NULL || dec_outputs == NULL || dec_status == NULL ||
        keygen_us == NULL || enc_us == NULL || dec_us == NULL ||
        keygen_cycles == NULL || enc_cycles == NULL || dec_cycles == NULL) {
        fprintf(stderr, "allocation failure\n");
        return 4;
    }

    build_master_instance_seed(0u, first_master_seed);

    for (unsigned int inst = 0; inst < instances; ++inst) {
        unsigned char master_seed[32];
        unsigned char entropy[48];
        unsigned char personalization[48];

        build_master_instance_seed(inst, master_seed);

        if (warmup_repeats > 0u) {
            derive_seed_pair_from_master(master_seed, 0x10u, entropy, personalization);
            if (init_prng_from_seed_pair(entropy, personalization) != 0) {
                fprintf(stderr, "DRNG initialization failed for keygen warm-up\n");
                return 5;
            }
            for (unsigned int warm = 0; warm < warmup_repeats; ++warm) {
                unsigned long long pk_out_len = pk_len;
                unsigned long long sk_out_len = sk_len;
                if (kem_keygen(pk, &pk_out_len, sk, &sk_out_len) != 0) {
                    fprintf(stderr, "kem_keygen failed during warm-up\n");
                    return 6;
                }
            }
        }

        derive_seed_pair_from_master(master_seed, 0x11u, entropy, personalization);
        if (init_prng_from_seed_pair(entropy, personalization) != 0) {
            fprintf(stderr, "DRNG initialization failed for keygen\n");
            return 5;
        }

        for (unsigned int rep = 0; rep < repeats; ++rep) {
            const unsigned int idx = inst * repeats + rep;
            unsigned long long pk_out_len = pk_len;
            unsigned long long sk_out_len = sk_len;
            uint64_t c0;
            uint64_t c1;
            clock_t t0;
            clock_t t1;

            c0 = read_cycles_begin();
            t0 = clock();
            if (kem_keygen(pk, &pk_out_len, sk, &sk_out_len) != 0) {
                fprintf(stderr, "kem_keygen failed during timing\n");
                return 6;
            }
            t1 = clock();
            c1 = read_cycles_end();

            if (pk_out_len != pk_len || sk_out_len != sk_len) {
                fprintf(stderr, "kem_keygen returned unexpected length\n");
                return 7;
            }
            keygen_us[idx] = elapsed_us(t0, t1);
            keygen_cycles[idx] = (double)(c1 - c0);
        }

        memcpy(keypair_bank_pk + ((size_t)inst * (size_t)pk_len), pk, (size_t)pk_len);
        memcpy(keypair_bank_sk + ((size_t)inst * (size_t)sk_len), sk, (size_t)sk_len);
    }

    for (unsigned int inst = 0; inst < instances; ++inst) {
        unsigned char master_seed[32];
        unsigned char entropy[48];
        unsigned char personalization[48];

        memcpy(pk, keypair_bank_pk + ((size_t)inst * (size_t)pk_len), (size_t)pk_len);
        memcpy(sk, keypair_bank_sk + ((size_t)inst * (size_t)sk_len), (size_t)sk_len);

        build_master_instance_seed(inst, master_seed);

        if (warmup_repeats > 0u) {
            derive_seed_pair_from_master(master_seed, 0x21u, entropy, personalization);
            if (init_prng_from_seed_pair(entropy, personalization) != 0) {
                fprintf(stderr, "DRNG initialization failed for encaps warm-up\n");
                return 8;
            }
            for (unsigned int warm = 0; warm < warmup_repeats; ++warm) {
                unsigned long long ct_out_len = ct_len;
                unsigned long long ss_out_len = ss_len;
                if (kem_enc(pk, pk_len, ss, &ss_out_len, ct, &ct_out_len) != 0) {
                    fprintf(stderr, "kem_enc failed during warm-up\n");
                    return 9;
                }
            }
        }

        derive_seed_pair_from_master(master_seed, 0x22u, entropy, personalization);
        if (init_prng_from_seed_pair(entropy, personalization) != 0) {
            fprintf(stderr, "DRNG initialization failed for encaps\n");
            return 8;
        }

        for (unsigned int rep = 0; rep < repeats; ++rep) {
            const unsigned int idx = inst * repeats + rep;
            unsigned long long ct_out_len = ct_len;
            unsigned long long ss_out_len = ss_len;
            uint64_t c0;
            uint64_t c1;
            clock_t t0;
            clock_t t1;

            c0 = read_cycles_begin();
            t0 = clock();
            if (kem_enc(pk, pk_len, ss, &ss_out_len, ct, &ct_out_len) != 0) {
                fprintf(stderr, "kem_enc failed during timing\n");
                return 9;
            }
            t1 = clock();
            c1 = read_cycles_end();

            if (ct_out_len != ct_len || ss_out_len != ss_len) {
                fprintf(stderr, "kem_enc returned unexpected length\n");
                return 10;
            }
            enc_us[idx] = elapsed_us(t0, t1);
            enc_cycles[idx] = (double)(c1 - c0);
        }

        derive_seed_pair_from_master(master_seed, 0x33u, entropy, personalization);
        if (init_prng_from_seed_pair(entropy, personalization) != 0) {
            fprintf(stderr, "DRNG initialization failed for dec input preparation\n");
            return 11;
        }

        for (unsigned int rep = 0; rep < repeats; ++rep) {
            unsigned long long ct_out_len = ct_len;
            unsigned long long ss_out_len = ss_len;
            unsigned char *ct_i = dec_inputs + ((size_t)rep * (size_t)ct_len);
            unsigned char *ss_i = dec_expected + ((size_t)rep * (size_t)ss_len);

            if (kem_enc(pk, pk_len, ss, &ss_out_len, ct, &ct_out_len) != 0) {
                fprintf(stderr, "kem_enc failed while preparing decapsulation input\n");
                return 12;
            }
            if (ct_out_len != ct_len || ss_out_len != ss_len) {
                fprintf(stderr, "kem_enc returned unexpected prep length\n");
                return 13;
            }
            memcpy(ct_i, ct, (size_t)ct_len);
            memcpy(ss_i, ss, (size_t)ss_len);
        }

        if (warmup_repeats > 0u) {
            unsigned char *ct_warm = dec_inputs;
            for (unsigned int warm = 0; warm < warmup_repeats; ++warm) {
                unsigned long long ss_dec_len = ss_len;
                if (kem_dec(sk, sk_len, ct_warm, ct_len, ss_out, &ss_dec_len) != 0) {
                    fprintf(stderr, "kem_dec failed during warm-up\n");
                    return 15;
                }
            }
        }

        for (unsigned int rep = 0; rep < repeats; ++rep) {
            const unsigned int idx = inst * repeats + rep;
            unsigned long long ss_dec_len = ss_len;
            unsigned char *ct_i = dec_inputs + ((size_t)rep * (size_t)ct_len);
            unsigned char *ss_out_i = dec_outputs + ((size_t)rep * (size_t)ss_len);
            uint64_t c0;
            uint64_t c1;
            clock_t t0;
            clock_t t1;

            memset(ss_out_i, 0, (size_t)ss_len);
            c0 = read_cycles_begin();
            t0 = clock();
            dec_status[rep] = kem_dec(sk, sk_len, ct_i, ct_len, ss_out_i, &ss_dec_len);
            t1 = clock();
            c1 = read_cycles_end();

            if (ss_dec_len != ss_len) {
                fprintf(stderr, "kem_dec returned unexpected length\n");
                return 14;
            }
            dec_us[idx] = elapsed_us(t0, t1);
            dec_cycles[idx] = (double)(c1 - c0);
        }

        for (unsigned int rep = 0; rep < repeats; ++rep) {
            const unsigned char *ss_i = dec_expected + ((size_t)rep * (size_t)ss_len);
            const unsigned char *ss_out_i = dec_outputs + ((size_t)rep * (size_t)ss_len);
            if (dec_status[rep] != 0) {
                fprintf(stderr, "kem_dec failed during timing\n");
                return 15;
            }
            if (memcmp(ss_i, ss_out_i, (size_t)ss_len) != 0) {
                fprintf(stderr, "kem_dec shared secret mismatch\n");
                return 16;
            }
            dec_ok++;
        }
    }

    finalize_op_stats(&keygen_stats, keygen_us, keygen_cycles, total_samples);
    finalize_op_stats(&enc_stats, enc_us, enc_cycles, total_samples);
    finalize_op_stats(&dec_stats, dec_us, dec_cycles, total_samples);

    printf("hare_kem_benchmark_v2\n");
    printf("algorithm_instance=%s\n", HARE_INSTANCE_NAME);
#ifdef ALGORITHM_INSTANCE
    printf("algorithm_instance_api=%s\n", ALGORITHM_INSTANCE);
#endif
    printf("benchmark_model=instance_repeat_two_level\n");
    printf("operation_split=keygen,encaps,decaps\n");
    printf("timing_source_us=clock_process_cpu_time\n");
    printf("cycles_source=%s\n", HARE_HAS_X86_TSC ? "x86_rdtsc_lfence" : "unavailable_non_x86");
    printf("seed_model=deterministic_master_instance_seed_plus_labelled_subseeds\n");
    print_hex_seed("first_master_instance_seed", first_master_seed);
    printf("instances=%u\n", count_model.instances);
    printf("repeats_per_instance=%u\n", count_model.repeats);
    printf("warmup_repeats_per_instance=%u\n", count_model.warmup_repeats);
    printf("total_samples_per_operation=%u\n", count_model.total_samples);
    printf("pk_bytes=%llu\n", pk_len);
    printf("sk_bytes=%llu\n", sk_len);
    printf("ct_bytes=%llu\n", ct_len);
    printf("ss_bytes=%llu\n", ss_len);

    print_op_stats("keygen", &keygen_stats);
    print_op_stats("encaps", &enc_stats);
    print_op_stats("decaps", &dec_stats);

    printf("decaps_shared_secret_match_count=%u/%u\n", dec_ok, total_samples);
    printf("peak_rss_bytes=%llu\n", get_peak_rss_bytes());

    write_raw_samples(getenv("HARE_BENCH_RAW_CSV"),
                      keygen_us, keygen_cycles,
                      enc_us, enc_cycles,
                      dec_us, dec_cycles,
                      instances, repeats);

    free(pk);
    free(sk);
    free(ct);
    free(ss);
    free(ss_out);
    free(keypair_bank_pk);
    free(keypair_bank_sk);
    free(dec_inputs);
    free(dec_expected);
    free(dec_outputs);
    free(dec_status);
    free(keygen_us);
    free(enc_us);
    free(dec_us);
    free(keygen_cycles);
    free(enc_cycles);
    free(dec_cycles);

    return 0;
}
