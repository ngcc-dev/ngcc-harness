/*
 * AFS-TrEDM benchmark driver.
 *
 * This file is not part of the ICCS CryptHash API.  It is included to make the
 * performance claims in the submission reproducible.  It builds a deterministic
 * public message of several lengths, calls CryptHash() repeatedly, and reports
 * wall-clock timing and, on x86/x86_64, approximate TSC cycle counts.
 *
 * Usage:
 *   make benchmark
 *   ./benchmark                         # default, moderate timing window
 *   ./benchmark --quick                 # shorter timing window for CI/smoke tests
 *   ./benchmark --full                  # longer timing window
 *   ./benchmark --sizes 64,192,1048576  # byte lengths for live tables
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include "CryptHash_AlgorithmInstance.h"

#if defined(__x86_64__) || defined(__i386__)
/* Function rdtsc_read: reads a processor cycle counter or monotonic fallback value for benchmarking. */
static uint64_t rdtsc_read(void)
{
# if defined(__x86_64__)
    unsigned int lo, hi;
    __asm__ __volatile__ ("lfence\n\t"
                          "rdtsc\n\t"
                          : "=a"(lo), "=d"(hi)
                          :
                          : "memory");
    return ((uint64_t)hi << 32) | (uint64_t)lo;
# else
    unsigned long long v;
    __asm__ __volatile__ ("lfence\n\t"
                          "rdtsc\n\t"
                          : "=A"(v)
                          :
                          : "memory");
    return (uint64_t)v;
# endif
}
# define HAVE_TSC 1
#else
# define HAVE_TSC 0
#endif

/* Function wall_seconds: returns a wall-clock timestamp used by benchmark drivers. */
static double wall_seconds(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

/* Function fill_message: fills a deterministic test or benchmark message buffer. */
static void fill_message(unsigned char *msg, size_t nbytes)
{
    uint64_t x = 0x6a09e667f3bcc909ULL;
    size_t i;
    for (i = 0; i < nbytes; i++) {
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        msg[i] = (unsigned char)(x >> 56);
    }
}

/* Function mask_unused_low_bits: clears unused low bits in the last partial message byte. */
static void mask_unused_low_bits(unsigned char *msg, unsigned long long bit_len)
{
    unsigned rem = (unsigned)(bit_len & 7ULL);
    size_t nbytes;
    unsigned char mask;
    if (rem == 0 || bit_len == 0) {
        return;
    }
    nbytes = (size_t)((bit_len + 7ULL) / 8ULL);
    mask = (unsigned char)(0xffU << (8U - rem));
    msg[nbytes - 1] &= mask;
}

/* Function print_hex32: prints the leading bytes of a digest in hexadecimal. */
static void print_hex32(const unsigned char *p, size_t n)
{
    size_t i;
    size_t limit = n < 32U ? n : 32U;
    for (i = 0; i < limit; i++) {
        printf("%02X", p[i]);
    }
}

/* Function select_reps: chooses a benchmark repetition count for the requested message size. */
static unsigned long select_reps(unsigned long long msg_bits, double target_seconds)
{
    if (msg_bits <= 9ULL) return target_seconds < 0.05 ? 2000UL : 20000UL;
    if (msg_bits <= 512ULL) return target_seconds < 0.05 ? 1000UL : 12000UL;
    if (msg_bits <= 8192ULL) return target_seconds < 0.05 ? 300UL : 3000UL;
    if (msg_bits <= 65536ULL) return target_seconds < 0.05 ? 80UL : 800UL;
    if (msg_bits <= 1048576ULL) return target_seconds < 0.05 ? 8UL : 80UL;
    return target_seconds < 0.05 ? 2UL : 12UL;
}

/* Function parse_u64: parses an unsigned decimal integer command-line argument. */
static int parse_u64(const char *s, unsigned long long *out)
{
    char *endp = NULL;
    unsigned long long v;

    if (s == NULL || *s == '\0') {
        return 0;
    }
    v = strtoull(s, &endp, 10);
    if (endp == s || *endp != '\0') {
        return 0;
    }
    *out = v;
    return 1;
}

/* Function parse_sizes_arg: parses a comma-separated message-size list. */
static int parse_sizes_arg(const char *arg,
                           unsigned long long *out_bits,
                           size_t out_cap,
                           size_t *out_count)
{
    const char *p = arg;
    size_t count = 0U;

    if (arg == NULL || *arg == '\0') {
        return 0;
    }
    while (*p != '\0') {
        char *endp = NULL;
        unsigned long long bytes = strtoull(p, &endp, 10);
        if (endp == p || bytes > (~0ULL / 8ULL) || count >= out_cap) {
            return 0;
        }
        out_bits[count++] = bytes * 8ULL;
        if (*endp == ',') {
            p = endp + 1;
            if (*p == '\0') {
                return 0;
            }
        } else if (*endp == '\0') {
            p = endp;
        } else {
            return 0;
        }
    }
    *out_count = count;
    return count != 0U;
}

/* Function run_one: runs one single-message benchmark case. */
static int run_one(unsigned long long msg_bits, double target_seconds)
{
    const size_t digest_bytes = (size_t)(DIGEST_BIT_LENGTH / 8);
    const size_t msg_bytes = (size_t)((msg_bits + 7ULL) / 8ULL);
    unsigned char *msg = NULL;
    unsigned char *digest = NULL;
    unsigned long reps = select_reps(msg_bits, target_seconds);
    unsigned long i;
    int rc = 0;
    double t0, t1, elapsed;
    uint64_t c0 = 0, c1 = 0;
    unsigned char checksum = 0;

    if (msg_bytes > 0) {
        msg = (unsigned char *)malloc(msg_bytes);
        if (!msg) {
            fprintf(stderr, "malloc failed for %zu bytes\n", msg_bytes);
            return 1;
        }
        fill_message(msg, msg_bytes);
        mask_unused_low_bits(msg, msg_bits);
    }

    digest = (unsigned char *)calloc(digest_bytes, 1U);
    if (!digest) {
        free(msg);
        fprintf(stderr, "calloc failed for digest\n");
        return 1;
    }

    /* One warm-up call. */
    rc = CryptHash(DIGEST_BIT_LENGTH, msg, msg_bits, digest);
    if (rc != 0) {
        fprintf(stderr, "CryptHash warm-up failed, rc=%d\n", rc);
        free(digest);
        free(msg);
        return 1;
    }

    /* Increase repetitions if the initial estimate is too small. */
    do {
        t0 = wall_seconds();
#if HAVE_TSC
        c0 = rdtsc_read();
#endif
        for (i = 0; i < reps; i++) {
            rc = CryptHash(DIGEST_BIT_LENGTH, msg, msg_bits, digest);
            if (rc != 0) {
                fprintf(stderr, "CryptHash failed, rc=%d\n", rc);
                free(digest);
                free(msg);
                return 1;
            }
            checksum ^= digest[i % digest_bytes];
        }
#if HAVE_TSC
        c1 = rdtsc_read();
#endif
        t1 = wall_seconds();
        elapsed = t1 - t0;
        if (elapsed < target_seconds && reps < 1000000UL) {
            reps *= 2UL;
        } else {
            break;
        }
    } while (1);

    {
        double ns_per_hash = elapsed * 1000000000.0 / (double)reps;
        double mib_s = 0.0;
        double cpb = 0.0;
        double cycles_per_hash = 0.0;
        double msg_mib_total = ((double)msg_bits / 8.0) * (double)reps / (1024.0 * 1024.0);

        if (elapsed > 0.0 && msg_bits > 0ULL) {
            mib_s = msg_mib_total / elapsed;
        }
#if HAVE_TSC
        cycles_per_hash = (double)(c1 - c0) / (double)reps;
        if (msg_bits > 0ULL) {
            cpb = cycles_per_hash / ((double)msg_bits / 8.0);
        }
#endif

        printf("%llu,%zu,%lu,%.6f,%.2f,", msg_bits, msg_bytes, reps, elapsed, ns_per_hash);
#if HAVE_TSC
        if (msg_bits > 0ULL) {
            printf("%.2f,%.2f,", cycles_per_hash, cpb);
        } else {
            printf("%.2f,NA,", cycles_per_hash);
        }
#else
        printf("NA,NA,");
#endif
        if (msg_bits > 0ULL) {
            printf("%.3f,", mib_s);
        } else {
            printf("NA,");
        }
        printf("0x%02X,", checksum);
        print_hex32(digest, digest_bytes);
        printf("\n");
    }

    free(digest);
    free(msg);
    return 0;
}

/* Function main: executes this standalone test, benchmark, or utility program. */
int main(int argc, char **argv)
{
    double target_seconds = 0.08;
    unsigned long long custom_bits[128];
    size_t custom_count = 0U;
    static const unsigned long long msg_bits[] = {
        0ULL,
        1ULL,
        7ULL,
        8ULL,
        9ULL,
        24ULL,           /* 3 bytes */
        512ULL,          /* 64 bytes */
        1024ULL,         /* 128 bytes */
        8192ULL,         /* 1 KiB */
        524288ULL,       /* 64 KiB */
        8388608ULL       /* 1 MiB */
    };
    const unsigned long long *run_bits = msg_bits;
    size_t run_count = sizeof(msg_bits) / sizeof(msg_bits[0]);
    int ai;
    size_t i;

    for (ai = 1; ai < argc; ai++) {
        if (strcmp(argv[ai], "--quick") == 0) {
            target_seconds = 0.02;
        } else if (strcmp(argv[ai], "--full") == 0) {
            target_seconds = 0.20;
        } else if (strcmp(argv[ai], "--csv") == 0) {
            /* Output is CSV by default; keep the flag for live-table tooling. */
        } else if (strcmp(argv[ai], "--sizes") == 0 && ai + 1 < argc) {
            if (!parse_sizes_arg(argv[++ai], custom_bits,
                                 sizeof(custom_bits) / sizeof(custom_bits[0]),
                                 &custom_count)) {
                fprintf(stderr, "invalid --sizes value\n");
                return 2;
            }
        } else if (strncmp(argv[ai], "--sizes=", 8) == 0) {
            if (!parse_sizes_arg(argv[ai] + 8, custom_bits,
                                 sizeof(custom_bits) / sizeof(custom_bits[0]),
                                 &custom_count)) {
                fprintf(stderr, "invalid --sizes value\n");
                return 2;
            }
        } else if (strcmp(argv[ai], "--bits") == 0 && ai + 1 < argc) {
            if (!parse_u64(argv[++ai], &custom_bits[0])) {
                fprintf(stderr, "invalid --bits value\n");
                return 2;
            }
            custom_count = 1U;
        } else if (strncmp(argv[ai], "--bits=", 7) == 0) {
            if (!parse_u64(argv[ai] + 7, &custom_bits[0])) {
                fprintf(stderr, "invalid --bits value\n");
                return 2;
            }
            custom_count = 1U;
        } else if (strncmp(argv[ai], "--target-seconds=", 17) == 0) {
            char *endp = NULL;
            target_seconds = strtod(argv[ai] + 17, &endp);
            if (endp == argv[ai] + 17 || *endp != '\0' ||
                target_seconds <= 0.0 || target_seconds > 60.0) {
                fprintf(stderr, "invalid --target-seconds value\n");
                return 2;
            }
        } else {
            fprintf(stderr, "Usage: %s [--quick|--full] [--csv] [--sizes B1,B2,...] [--bits N] [--target-seconds=S]\n", argv[0]);
            return 2;
        }
    }

    if (custom_count != 0U) {
        run_bits = custom_bits;
        run_count = custom_count;
    }

    printf("# algorithm_instance=%s\n", ALGORITHM_INSTANCE);
#ifdef AFS_TREDM_OPT64_PORTABLE
    printf("# implementation=portable-opt64\n");
#else
    printf("# implementation=reference\n");
#endif
    printf("# digest_bits=%d\n", DIGEST_BIT_LENGTH);
#if HAVE_TSC
    printf("# tsc=available\n");
#else
    printf("# tsc=not_available\n");
#endif
    printf("# columns=msg_bits,msg_bytes,repetitions,elapsed_seconds,ns_per_hash,cycles_per_hash,cycles_per_byte,MiB_per_second,checksum,digest_prefix_32_bytes\n");

    for (i = 0; i < run_count; i++) {
        if (run_one(run_bits[i], target_seconds) != 0) {
            return 1;
        }
    }

    return 0;
}
