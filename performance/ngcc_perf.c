/* Bounded, single-operation timing of the public harness ABI.
 * One process measures one API operation, with deterministic preparation.
 * Output is tab-separated for performance/run.py. No candidate build script is run.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <linux/perf_event.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#include "../api/link_common.h"
#include "../api/link_kem.h"
#include "../api/link_sig.h"
#include "../api/link_hash.h"

typedef struct {
    void *library;
    const ngcc_meta_t *meta;
    const char *operation;
    size_t input_bytes;
    unsigned char *pk, *sk, *ct, *ss, *ss2, *sig, *msg, *digest;
    uint64_t pk_cap, sk_cap, ct_cap, ss_cap, sig_cap, digest_cap;
    unsigned long long pk_len, sk_len, ct_len, ss_len, sig_len;
    ngcc_kem_api_t kem;
    ngcc_sig_api_t sig_api;
    ngcc_hash_api_t hash_api;
} context_t;

static void *required(void *library, const char *symbol)
{
    void *function = dlsym(library, symbol);
    if (!function) {
        fprintf(stderr, "missing symbol %s: %s\n", symbol, dlerror());
        exit(2);
    }
    return function;
}

/* POSIX defines dlsym for function pointers; memcpy avoids ISO C's diagnostic
 * about direct conversion from an object pointer to a function pointer. */
#define LOAD_FN(target, library, symbol) do { \
    void *ngcc_symbol = required((library), (symbol)); \
    memcpy(&(target), &ngcc_symbol, sizeof(target)); \
} while (0)

static unsigned char *buffer(uint64_t n)
{
    if (n > (1ULL << 29)) {
        fprintf(stderr, "declared buffer too large: %" PRIu64 " bytes\n", n);
        exit(2);
    }
    unsigned char *p = calloc((size_t)n + 64, 1);
    if (!p) { perror("calloc"); exit(2); }
    return p;
}

static uint64_t now_ns(void)
{
    struct timespec time;
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &time)) { perror("clock_gettime"); exit(2); }
    return (uint64_t)time.tv_sec * UINT64_C(1000000000) + (uint64_t)time.tv_nsec;
}

static uint64_t resident_bytes(void)
{
    long pages = 0, resident = 0;
    FILE *stream = fopen("/proc/self/statm", "r");
    if (!stream) return 0;
    int ok = fscanf(stream, "%ld %ld", &pages, &resident);
    fclose(stream);
    return ok == 2 ? (uint64_t)resident * (uint64_t)sysconf(_SC_PAGESIZE) : 0;
}

static int cycles_fd(void)
{
    struct perf_event_attr attr;
    memset(&attr, 0, sizeof attr);
    attr.type = PERF_TYPE_HARDWARE;
    attr.size = sizeof attr;
    attr.config = PERF_COUNT_HW_CPU_CYCLES;
    attr.disabled = 1;
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    return (int)syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);
}

static int operate(context_t *c)
{
    const char *op = c->operation;
    if (c->meta->type == NGCC_TYPE_KEM) {
        if (!strcmp(op, "keygen")) {
            c->pk_len = c->pk_cap; c->sk_len = c->sk_cap;
            return c->kem.keygen(c->pk, &c->pk_len, c->sk, &c->sk_len) ||
                   c->pk_len > c->pk_cap || c->sk_len > c->sk_cap;
        }
        if (!strcmp(op, "enc")) {
            c->ss_len = c->ss_cap; c->ct_len = c->ct_cap;
            return c->kem.enc(c->pk, c->pk_len, c->ss, &c->ss_len,
                              c->ct, &c->ct_len) ||
                   c->ss_len > c->ss_cap || c->ct_len > c->ct_cap;
        }
        if (!strcmp(op, "dec")) {
            unsigned long long length = c->ss_cap;
            int rc = c->kem.dec(c->sk, c->sk_len, c->ct, c->ct_len,
                                c->ss2, &length);
            return rc || length > c->ss_cap || length != c->ss_len;
        }
    }
    if (c->meta->type == NGCC_TYPE_SIG) {
        if (!strcmp(op, "keygen")) {
            c->pk_len = c->pk_cap; c->sk_len = c->sk_cap;
            return c->sig_api.keygen(c->pk, &c->pk_len, c->sk, &c->sk_len) ||
                   c->pk_len > c->pk_cap || c->sk_len > c->sk_cap;
        }
        if (!strcmp(op, "sign")) {
            c->sig_len = c->sig_cap;
            return c->sig_api.sign(c->sk, c->sk_len, c->msg, 64,
                                   c->sig, &c->sig_len) || c->sig_len > c->sig_cap;
        }
        if (!strcmp(op, "verify"))
            return c->sig_api.verify(c->pk, c->pk_len, c->sig, c->sig_len,
                                     c->msg, 64);
    }
    if (c->meta->type == NGCC_TYPE_HASH && !strcmp(op, "hash"))
        return c->hash_api.hash((int)((const ngcc_meta_hash_t *)c->meta)->digest_bits,
                                c->msg, (unsigned long long)c->input_bytes * 8,
                                c->digest);
    fprintf(stderr, "unsupported operation %s for %s\n", op, ngcc_type_name(c->meta->type));
    exit(2);
}

static void prepare(context_t *c)
{
    unsigned char seed[48];
    for (size_t i = 0; i < sizeof seed; i++) seed[i] = (unsigned char)i;
    int (*seed_library)(const unsigned char *, unsigned long long);
    LOAD_FN(seed_library, c->library, "ngcc_seed");
    if (seed_library(seed, sizeof seed)) { fprintf(stderr, "ngcc_seed failed\n"); exit(3); }

    if (c->meta->type == NGCC_TYPE_KEM) {
        const ngcc_meta_kem_t *m = (const ngcc_meta_kem_t *)c->meta;
#define X(field, symbol) LOAD_FN(c->kem.field, c->library, symbol);
        NGCC_KEM_SYMBOLS
#undef X
        c->pk_cap = m->pk_len; c->sk_cap = m->sk_len;
        c->ct_cap = m->ct_len; c->ss_cap = m->ss_len;
        c->pk = buffer(c->pk_cap); c->sk = buffer(c->sk_cap);
        c->ct = buffer(c->ct_cap); c->ss = buffer(c->ss_cap); c->ss2 = buffer(c->ss_cap);
        c->pk_len = c->pk_cap; c->sk_len = c->sk_cap;
        if (strcmp(c->operation, "keygen")) {
            if (c->kem.keygen(c->pk, &c->pk_len, c->sk, &c->sk_len) ||
                c->pk_len > c->pk_cap || c->sk_len > c->sk_cap)
                { fprintf(stderr, "setup keygen failed\n"); exit(3); }
            c->ct_len = c->ct_cap; c->ss_len = c->ss_cap;
            if (c->kem.enc(c->pk, c->pk_len, c->ss, &c->ss_len, c->ct, &c->ct_len) ||
                c->ss_len > c->ss_cap || c->ct_len > c->ct_cap)
                { fprintf(stderr, "setup encapsulation failed\n"); exit(3); }
            if (!strcmp(c->operation, "dec") &&
                (operate(c) || memcmp(c->ss, c->ss2, c->ss_len)))
                { fprintf(stderr, "honest decapsulation mismatch; refusing timing\n"); exit(3); }
        }
    } else if (c->meta->type == NGCC_TYPE_SIG) {
        const ngcc_meta_sig_t *m = (const ngcc_meta_sig_t *)c->meta;
#define X(field, symbol) LOAD_FN(c->sig_api.field, c->library, symbol);
        NGCC_SIG_SYMBOLS
#undef X
        c->pk_cap = m->pk_len; c->sk_cap = m->sk_len; c->sig_cap = m->sn_len;
        c->pk = buffer(c->pk_cap); c->sk = buffer(c->sk_cap); c->sig = buffer(c->sig_cap);
        c->msg = buffer(64);
        for (size_t i = 0; i < 64; i++) c->msg[i] = (unsigned char)(i * 131U + 64U);
        c->pk_len = c->pk_cap; c->sk_len = c->sk_cap;
        if (strcmp(c->operation, "keygen")) {
            if (c->sig_api.keygen(c->pk, &c->pk_len, c->sk, &c->sk_len) ||
                c->pk_len > c->pk_cap || c->sk_len > c->sk_cap)
                { fprintf(stderr, "setup keygen failed\n"); exit(3); }
            c->sig_len = c->sig_cap;
            if (c->sig_api.sign(c->sk, c->sk_len, c->msg, 64, c->sig, &c->sig_len) ||
                c->sig_len > c->sig_cap)
                { fprintf(stderr, "setup signing failed\n"); exit(3); }
            if (c->sig_api.verify(c->pk, c->pk_len, c->sig, c->sig_len, c->msg, 64))
                { fprintf(stderr, "setup signature does not verify\n"); exit(3); }
        }
    } else if (c->meta->type == NGCC_TYPE_HASH) {
        const ngcc_meta_hash_t *m = (const ngcc_meta_hash_t *)c->meta;
#define X(field, symbol) LOAD_FN(c->hash_api.field, c->library, symbol);
        NGCC_HASH_SYMBOLS
#undef X
        if (c->input_bytes < 32 || c->input_bytes > 65536) {
            fprintf(stderr, "hash input must be one of the guide's 32..65536-byte sizes\n"); exit(2);
        }
        c->digest_cap = m->digest_len;
        c->msg = buffer(c->input_bytes); c->digest = buffer(c->digest_cap);
        for (size_t i = 0; i < c->input_bytes; i++)
            c->msg[i] = (unsigned char)(i * 131U + c->input_bytes);
    } else {
        fprintf(stderr, "KEX timing is not implemented; no partial KEX metric will be reported\n");
        exit(2);
    }
}

int main(int argc, char **argv)
{
    if (argc != 6) {
        fprintf(stderr, "usage: %s LIB OP HASH_BYTES TRIALS LIMIT_SECONDS\n", argv[0]);
        return 2;
    }
    int trials = atoi(argv[4]);
    double limit_sec = atof(argv[5]);
    if (trials != 5 || !(limit_sec > 0 && limit_sec <= 600)) {
        fprintf(stderr, "expected five trials and a positive limit <= 600s\n"); return 2;
    }
    context_t c = {0};
    c.operation = argv[2];
    c.input_bytes = (size_t)strtoull(argv[3], NULL, 10);
    c.library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!c.library) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 2; }
    const ngcc_meta_t *(*metadata)(void);
    LOAD_FN(metadata, c.library, "ngcc_meta");
    c.meta = metadata();
    if (!c.meta || c.meta->magic != NGCC_META_MAGIC || c.meta->abi != NGCC_LINK_ABI)
        { fprintf(stderr, "invalid harness metadata\n"); return 2; }
    prepare(&c);

    printf("META\tid\t%s\nMETA\tinstance\t%s\nMETA\tvariant\t%s\n",
           c.meta->id, c.meta->instance, c.meta->variant);
    printf("META\toperation\t%s\nMETA\tinput_bytes\t%zu\n", c.operation, c.input_bytes);
    printf("META\tbuild_flags\t%s\n", c.meta->build_flags);
    printf("META\tbaseline_rss_bytes\t%" PRIu64 "\n", resident_bytes());
    printf("META\tpk_bytes\t%" PRIu64 "\nMETA\tsk_bytes\t%" PRIu64 "\n",
           c.pk_cap, c.sk_cap);
    printf("META\tct_bytes\t%" PRIu64 "\nMETA\tss_bytes\t%" PRIu64 "\n",
           c.ct_cap, c.ss_cap);
    printf("META\tsignature_bytes\t%" PRIu64 "\nMETA\tdigest_bytes\t%" PRIu64 "\n",
           c.sig_cap, c.digest_cap);
    fflush(stdout);

    int perf_fd = cycles_fd();
    printf("META\tcpu_cycles_available\t%s\n", perf_fd >= 0 ? "yes" : "no");
    uint64_t start = now_ns();
    uint64_t deadline = start + (uint64_t)(limit_sec * 1e9);
    int warmups = 0;
    for (; warmups < 3 && now_ns() < deadline; warmups++)
        if (operate(&c)) { fprintf(stderr, "warm-up operation failed\n"); return 3; }
    printf("META\twarmups\t%d\n", warmups);

    int completed_trials = 0;
    unsigned long long total_samples = 0;
    for (int trial = 0; trial < trials; trial++) {
        uint64_t trial_start = now_ns();
        if (trial_start >= deadline) break;
        uint64_t trial_deadline = start +
            (uint64_t)((double)(trial + 1) * limit_sec * 1e9 / trials);
        unsigned long long count = 0;
        uint64_t cycles = 0;
        if (perf_fd >= 0) {
            ioctl(perf_fd, PERF_EVENT_IOC_RESET, 0);
            ioctl(perf_fd, PERF_EVENT_IOC_ENABLE, 0);
        }
        do {
            if (operate(&c)) { fprintf(stderr, "measured operation failed\n"); return 3; }
            count++;
            if ((count & 15ULL) == 0 || count < 20) {
                uint64_t current = now_ns();
                if (current >= trial_deadline || current >= deadline) break;
                if (count >= 100 && current - trial_start >= UINT64_C(1000000000)) break;
                if (count >= 20000) break;
            }
        } while (1);
        uint64_t trial_end = now_ns();
        if (perf_fd >= 0) {
            ioctl(perf_fd, PERF_EVENT_IOC_DISABLE, 0);
            if (read(perf_fd, &cycles, sizeof cycles) != sizeof cycles) cycles = 0;
        }
        printf("TRIAL\t%d\t%llu\t%.9f\t%" PRIu64 "\n", trial + 1,
               count, (double)(trial_end - trial_start) / 1e9, cycles);
        fflush(stdout);
        total_samples += count;
        completed_trials++;
    }
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("META\tpeak_rss_bytes\t%llu\n", (unsigned long long)usage.ru_maxrss * 1024ULL);
    printf("META\tmeasurement_count\t%llu\n", total_samples);
    printf("META\tguide_100_measurements\t%s\n", total_samples >= 100 ? "met" : "not_met");
    printf("STATUS\t%s\n", completed_trials == trials ? "complete" : "partial");
    if (perf_fd >= 0) close(perf_fd);
    dlclose(c.library);
    return completed_trials == trials ? 0 : 4;
}
