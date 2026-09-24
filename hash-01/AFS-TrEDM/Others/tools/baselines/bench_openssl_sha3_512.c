/*
 * Live benchmark driver for OpenSSL EVP SHA3-512.
 *
 * This driver intentionally calls the EVP SHA3-512 API directly; it does not
 * read historical CSV data.  Output uses the same CSV shape as the AFS
 * benchmark drivers so S6 live performance tables can parse it.
 */
#include <openssl/evp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define SHA3_512_EMPTY_HEX \
    "A69F73CCA23A9AC5C8B567DC185A756E97C982164FE25859E0D1DCC1475C80A615B2123AF1F5F94C11E3E9402C3AC558F500199D95B6D3E301758586281DCD26"
#define SHA3_512_ABC_HEX \
    "B751850B1A57168A5693CD924B6B096E08F621827444F70D884F5D0240D2712E10E116E9192AF3C91A7EC57647E3934057340B4CF408D5A56592F8274EEC53F0"

#if defined(__x86_64__) || defined(__i386__)
static uint64_t rdtsc_read(void)
{
# if defined(__x86_64__)
    unsigned int lo, hi;
    __asm__ __volatile__("lfence\n\trdtsc\n\t" : "=a"(lo), "=d"(hi) : : "memory");
    return ((uint64_t)hi << 32) | (uint64_t)lo;
# else
    unsigned long long v;
    __asm__ __volatile__("lfence\n\trdtsc\n\t" : "=A"(v) : : "memory");
    return (uint64_t)v;
# endif
}
# define HAVE_TSC 1
#else
# define HAVE_TSC 0
#endif

static double wall_seconds(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

static void fill_message(unsigned char *msg, size_t nbytes)
{
    uint64_t x = UINT64_C(0x6a09e667f3bcc909);
    size_t i;
    for (i = 0; i < nbytes; i++) {
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        msg[i] = (unsigned char)(x >> 56);
    }
}

static void print_hex32(const unsigned char *p, size_t n)
{
    size_t i;
    size_t limit = n < 32U ? n : 32U;
    for (i = 0; i < limit; i++) {
        printf("%02X", p[i]);
    }
}

static void digest_to_hex(const unsigned char *p, size_t n, char *out, size_t out_cap)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t i;
    if (out_cap < n * 2U + 1U) {
        if (out_cap != 0U) {
            out[0] = '\0';
        }
        return;
    }
    for (i = 0; i < n; i++) {
        out[2U * i] = hex[p[i] >> 4];
        out[2U * i + 1U] = hex[p[i] & 0x0FU];
    }
    out[2U * n] = '\0';
}

static unsigned long select_reps(size_t msg_bytes, double target_seconds)
{
    if (msg_bytes <= 64U) return target_seconds < 0.05 ? 1000UL : 12000UL;
    if (msg_bytes <= 1024U) return target_seconds < 0.05 ? 300UL : 3000UL;
    if (msg_bytes <= 65536U) return target_seconds < 0.05 ? 80UL : 800UL;
    if (msg_bytes <= 1048576U) return target_seconds < 0.05 ? 8UL : 80UL;
    return target_seconds < 0.05 ? 2UL : 12UL;
}

static int parse_sizes_arg(const char *arg, size_t *out, size_t cap, size_t *count)
{
    const char *p = arg;
    size_t n = 0U;
    if (arg == NULL || *arg == '\0') {
        return 0;
    }
    while (*p != '\0') {
        char *endp = NULL;
        unsigned long v = strtoul(p, &endp, 10);
        if (endp == p || n >= cap) {
            return 0;
        }
        out[n++] = (size_t)v;
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
    *count = n;
    return n != 0U;
}

static int hash_once(unsigned char out[64], const unsigned char *msg, size_t msg_bytes)
{
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    unsigned int outlen = 0U;

    if (ctx == NULL) {
        return 1;
    }
    if (EVP_DigestInit_ex(ctx, EVP_sha3_512(), NULL) != 1) {
        EVP_MD_CTX_free(ctx);
        return 1;
    }
    if (EVP_DigestUpdate(ctx, msg, msg_bytes) != 1) {
        EVP_MD_CTX_free(ctx);
        return 1;
    }
    if (EVP_DigestFinal_ex(ctx, out, &outlen) != 1 || outlen != 64U) {
        EVP_MD_CTX_free(ctx);
        return 1;
    }
    EVP_MD_CTX_free(ctx);
    return 0;
}

static int run_kat(const char *kat_name, const unsigned char *msg, size_t msg_bytes, const char *expected_hex)
{
    unsigned char digest[64];
    char actual[sizeof(SHA3_512_EMPTY_HEX)];
    if (hash_once(digest, msg, msg_bytes) != 0) {
        fprintf(stderr, "openssl-evp-sha3-512 KAT failed: hash_once error\n");
        return 1;
    }
    digest_to_hex(digest, sizeof(digest), actual, sizeof(actual));
    if (strcmp(actual, expected_hex) != 0) {
        fprintf(stderr, "openssl-evp-sha3-512 %s KAT failed: expected=%s actual=%s\n", kat_name, expected_hex, actual);
        return 1;
    }
    printf("openssl-evp-sha3-512 %s OK %s source=OpenSSL EVP_sha3_512\n", kat_name, actual);
    return 0;
}

static int run_kat_empty(void)
{
    return run_kat("empty", (const unsigned char *)"", 0U, SHA3_512_EMPTY_HEX);
}

static int run_kat_abc(void)
{
    return run_kat("abc", (const unsigned char *)"abc", 3U, SHA3_512_ABC_HEX);
}

static int run_digest_generated(size_t msg_bytes)
{
    unsigned char *msg;
    unsigned char digest[64];
    char actual[sizeof(SHA3_512_EMPTY_HEX)];

    msg = (unsigned char *)malloc(msg_bytes == 0U ? 1U : msg_bytes);
    if (msg == NULL) {
        fprintf(stderr, "openssl-evp-sha3-512 digest-generated failed: allocation\n");
        return 1;
    }
    fill_message(msg, msg_bytes);
    if (hash_once(digest, msg, msg_bytes) != 0) {
        free(msg);
        fprintf(stderr, "openssl-evp-sha3-512 digest-generated failed: hash_once\n");
        return 1;
    }
    free(msg);
    digest_to_hex(digest, sizeof(digest), actual, sizeof(actual));
    printf("openssl-evp-sha3-512 digest-generated-%zu %s source=OpenSSL EVP_sha3_512\n", msg_bytes, actual);
    return 0;
}

static int run_one(size_t msg_bytes, double target_seconds, unsigned long fixed_reps)
{
    unsigned char *msg = NULL;
    unsigned char digest[64];
    unsigned long reps = fixed_reps != 0UL ? fixed_reps : select_reps(msg_bytes, target_seconds);
    unsigned long i;
    double t0, t1, elapsed;
    uint64_t c0 = 0, c1 = 0;
    unsigned char checksum = 0U;

    msg = (unsigned char *)malloc(msg_bytes == 0U ? 1U : msg_bytes);
    if (msg == NULL) {
        return 1;
    }
    fill_message(msg, msg_bytes);
    if (hash_once(digest, msg, msg_bytes) != 0) {
        free(msg);
        return 1;
    }

    do {
        t0 = wall_seconds();
#if HAVE_TSC
        c0 = rdtsc_read();
#endif
        for (i = 0UL; i < reps; i++) {
            if (hash_once(digest, msg, msg_bytes) != 0) {
                free(msg);
                return 1;
            }
            checksum ^= digest[i % sizeof(digest)];
        }
#if HAVE_TSC
        c1 = rdtsc_read();
#endif
        t1 = wall_seconds();
        elapsed = t1 - t0;
        if (fixed_reps == 0UL && elapsed < target_seconds && reps < 1000000UL) {
            reps *= 2UL;
        } else {
            break;
        }
    } while (1);

    {
        const unsigned long long msg_bits = (unsigned long long)msg_bytes * 8ULL;
        double ns_per_hash = elapsed * 1000000000.0 / (double)reps;
        double cycles_per_hash = 0.0;
        double cpb = 0.0;
        double mb_s = 0.0;
        double mib_s = 0.0;
        if (elapsed > 0.0 && msg_bytes != 0U) {
            mb_s = ((double)msg_bytes * (double)reps) / 1000000.0 / elapsed;
            mib_s = ((double)msg_bytes * (double)reps) / (1024.0 * 1024.0) / elapsed;
        }
        printf("openssl-evp-sha3-512,%llu,%zu,%lu,%.6f,%.2f,", msg_bits, msg_bytes, reps, elapsed, ns_per_hash);
#if HAVE_TSC
        cycles_per_hash = (double)(c1 - c0) / (double)reps;
        printf("%.2f,", cycles_per_hash);
        if (msg_bytes != 0U) {
            cpb = cycles_per_hash / (double)msg_bytes;
            printf("%.2f,", cpb);
        } else {
            printf("NA,");
        }
#else
        printf("NA,NA,");
#endif
        if (msg_bytes != 0U) {
            printf("%.3f,%.3f,", mb_s, mib_s);
        } else {
            printf("NA,NA,");
        }
        printf("0x%02X,", checksum);
        print_hex32(digest, sizeof(digest));
        printf("\n");
    }

    free(msg);
    return 0;
}

int main(int argc, char **argv)
{
    double target_seconds = 0.08;
    unsigned long fixed_reps = 0UL;
    size_t sizes[128] = {64U, 192U, 1024U, 1536U, 65536U, 1048576U};
    size_t size_count = 6U;
    int ai;
    size_t i;

    for (ai = 1; ai < argc; ai++) {
        if (strcmp(argv[ai], "--kat-empty") == 0) {
            return run_kat_empty();
        } else if (strcmp(argv[ai], "--kat-abc") == 0) {
            return run_kat_abc();
        } else if (strcmp(argv[ai], "--kat-all") == 0) {
            return run_kat_empty() || run_kat_abc();
        } else if (strcmp(argv[ai], "--digest-generated") == 0 && ai + 1 < argc) {
            return run_digest_generated((size_t)strtoul(argv[++ai], NULL, 10));
        } else if (strncmp(argv[ai], "--digest-generated=", 19) == 0) {
            return run_digest_generated((size_t)strtoul(argv[ai] + 19, NULL, 10));
        } else if (strcmp(argv[ai], "--quick") == 0 || (strcmp(argv[ai], "--mode") == 0 && ai + 1 < argc && strcmp(argv[ai + 1], "quick") == 0)) {
            if (strcmp(argv[ai], "--mode") == 0) {
                ai++;
            }
            target_seconds = 0.02;
        } else if (strcmp(argv[ai], "--full") == 0 || (strcmp(argv[ai], "--mode") == 0 && ai + 1 < argc && strcmp(argv[ai + 1], "full") == 0)) {
            if (strcmp(argv[ai], "--mode") == 0) {
                ai++;
            }
            target_seconds = 0.20;
        } else if (strcmp(argv[ai], "--csv") == 0) {
            /* CSV is the only output format. */
        } else if (strcmp(argv[ai], "--sizes") == 0 && ai + 1 < argc) {
            if (!parse_sizes_arg(argv[++ai], sizes, sizeof(sizes) / sizeof(sizes[0]), &size_count)) {
                fprintf(stderr, "invalid --sizes value\n");
                return 2;
            }
        } else if (strncmp(argv[ai], "--sizes=", 8) == 0) {
            if (!parse_sizes_arg(argv[ai] + 8, sizes, sizeof(sizes) / sizeof(sizes[0]), &size_count)) {
                fprintf(stderr, "invalid --sizes value\n");
                return 2;
            }
        } else if (strcmp(argv[ai], "--repetitions") == 0 && ai + 1 < argc) {
            fixed_reps = strtoul(argv[++ai], NULL, 10);
        } else if (strncmp(argv[ai], "--repetitions=", 14) == 0) {
            fixed_reps = strtoul(argv[ai] + 14, NULL, 10);
        } else {
            fprintf(stderr, "Usage: %s [--kat-empty|--kat-abc|--kat-all|--digest-generated N] [--quick|--full|--mode quick|--mode full] [--csv] [--sizes B1,B2,...] [--repetitions N]\n", argv[0]);
            return 2;
        }
    }

    printf("# algorithm_instance=sha3-512\n");
    printf("# implementation=OpenSSL EVP SHA3-512\n");
    printf("# backend=openssl-evp-sha3-512\n");
    printf("# digest_bits=512\n");
    printf("# source_origin=OpenSSL EVP_sha3_512\n");
    printf("# source_file=tools/baselines/bench_openssl_sha3_512.c\n");
    printf("# backend_class=external_sha3_deployment\n");
    printf("# simd=library-dependent\n");
    printf("# api=single-message EVP\n");
#if HAVE_TSC
    printf("# tsc=available\n");
#else
    printf("# tsc=not_available\n");
#endif
    printf("# columns=backend,msg_bits,msg_bytes,repetitions,elapsed_seconds,ns_per_hash,cycles_per_hash,cycles_per_byte,MB_per_second,MiB_per_second,checksum,digest_prefix_32_bytes\n");

    for (i = 0U; i < size_count; i++) {
        if (run_one(sizes[i], target_seconds, fixed_reps) != 0) {
            return 1;
        }
    }
    return 0;
}
