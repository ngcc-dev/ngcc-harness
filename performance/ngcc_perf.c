/* Single-operation timing of the public harness ABI.
 * One process measures one API operation, with deterministic preparation.
 * Output is tab-separated for performance/run.py and performance/campaign.py.
 * No candidate build script is run.
 *
 *   ngcc_perf LIB OP BYTES TRIALS LIMIT_SECONDS             time-bounded trials
 *   ngcc_perf LIB OP BYTES TRIALS LIMIT_SECONDS ITERS WARM  exactly ITERS timed
 *       operations per trial after WARM warm-ups; LIMIT_SECONDS is then unused
 *       (the caller enforces a hard timeout), so slow candidates still give data.
 *
 * KEM ops: keygen enc dec. Signature ops: keygen sign verify (64-byte message).
 * Hash op: hash (BYTES-byte message). XOF op: xof-SEED, a SEED-byte message and
 * BYTES bytes of output requested through CryptHash's length argument (the
 * output-length test of performance/supplement). KEX ops: exchange (init_a, init_b, every
 * pass, derive_a, derive_b), or one step of it: init_a init_b pass1..pass16
 * derive_a derive_b. A step is timed individually inside a complete protocol run.
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
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#include "../api/link_common.h"
#include "../api/link_kem.h"
#include "../api/link_sig.h"
#include "../api/link_hash.h"
#include "../api/link_kex.h"

#define KEX_MAX_PASSES 16
/* Extra signature buffer: a scheme that returns more bytes than it declares
 * (e.g. sign-15) is measured safely and the excess recorded. */
#define SIG_GUARD 4096

typedef struct {
    void *library;
    const ngcc_meta_t *meta;
    const char *operation;
    size_t input_bytes;
    unsigned long long xof_seed, xof_out;   /* op xof-SEED: message and output bytes */
    unsigned char *pk, *sk, *ct, *ss, *ss2, *sig, *msg, *digest;
    uint64_t pk_cap, sk_cap, ct_cap, ss_cap, sig_cap, digest_cap;
    unsigned long long pk_len, sk_len, ct_len, ss_len, sig_len;
    ngcc_kem_api_t kem;
    ngcc_sig_api_t sig_api;
    ngcc_hash_api_t hash_api;
    /* KEX */
    ngcc_kex_api_t kex;
    ngcc_kex_passn_fn passfn[KEX_MAX_PASSES + 1];
    unsigned long long passes, kex_steps;   /* protocol passes; last pass run */
    int kex_step;                            /* -1 = full exchange, else step index */
    unsigned char *pkb, *skb, *sta, *stb, *ssb, *msgs[KEX_MAX_PASSES + 1];
    uint64_t sta_cap, stb_cap, msg_cap;
    unsigned long long pkb_len, skb_len, mlen[KEX_MAX_PASSES + 1];
    unsigned long long sign_failures, msg_tweaks;   /* failed sig_sign calls retried */
    int setup_verify_failed;                        /* honest signature rejected in setup */
    unsigned long long len_excess;                  /* bytes returned beyond the declared length */
    unsigned long long verify_rejects;              /* honest signature rejected while timed */
    int perf_fd;
    uint64_t step_ns, step_cycles;           /* accumulated timing of one KEX step */
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

/* getrusage's ru_maxrss survives execve, so it reports the launching Python
 * process; VmHWM belongs to this process's own address space. */
static uint64_t peak_resident_bytes(void)
{
    char line[128];
    unsigned long long kib = 0;
    FILE *stream = fopen("/proc/self/status", "r");
    if (!stream) return 0;
    while (fgets(line, sizeof line, stream))
        if (sscanf(line, "VmHWM: %llu kB", &kib) == 1) break;
    fclose(stream);
    return (uint64_t)kib * 1024U;
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

/* KEX step indices: 0 init_a, 1 init_b, 1+k pass k, KEX_DERIVE_A/B derives */
#define KEX_DERIVE_A (KEX_MAX_PASSES + 2)
#define KEX_DERIVE_B (KEX_MAX_PASSES + 3)

/* Optional observer of the timed KEX step (used by performance/hashprof). */
static void (*step_hook)(int on);

static void step_begin(context_t *c, int step, uint64_t *t0)
{
    if (c->kex_step != step) return;
    if (step_hook) step_hook(1);
    if (c->perf_fd >= 0) { ioctl(c->perf_fd, PERF_EVENT_IOC_RESET, 0); ioctl(c->perf_fd, PERF_EVENT_IOC_ENABLE, 0); }
    *t0 = now_ns();
}

static void step_end(context_t *c, int step, uint64_t t0)
{
    if (c->kex_step != step) return;
    uint64_t t1 = now_ns(), cycles = 0;
    if (c->perf_fd >= 0) {
        ioctl(c->perf_fd, PERF_EVENT_IOC_DISABLE, 0);
        if (read(c->perf_fd, &cycles, sizeof cycles) != sizeof cycles) cycles = 0;
    }
    if (step_hook) step_hook(0);
    c->step_ns += t1 - t0;
    c->step_cycles += cycles;
}

/* One complete key exchange, as in the official KAT_KEX.c driver. Returns
 * nonzero on any failure, including different initiator/responder secrets. */
static int kex_exchange(context_t *c)
{
    uint64_t t0 = 0;
    unsigned long long pka_len = c->pk_cap, ska_len = c->sk_cap, sta_len = c->sta_cap,
                       stb_len = c->stb_cap, ssa_len = c->ss_cap, ssb_len = c->ss_cap,
                       ma_len = 0, mb_len = 0;
    unsigned char *ma = NULL, *mb = NULL;
    c->pkb_len = c->pk_cap; c->skb_len = c->sk_cap;
    step_begin(c, 0, &t0);
    int r = c->kex.init_a(c->pk, &pka_len, c->sk, &ska_len, c->sta, &sta_len);
    step_end(c, 0, t0);
    if (r < 0 || pka_len > c->pk_cap || ska_len > c->sk_cap || sta_len > c->sta_cap) return 1;
    step_begin(c, 1, &t0);
    r = c->kex.init_b(c->pkb, &c->pkb_len, c->skb, &c->skb_len, c->stb, &stb_len);
    step_end(c, 1, t0);
    if (r < 0 || c->pkb_len > c->pk_cap || c->skb_len > c->sk_cap || stb_len > c->stb_cap) return 1;
    unsigned long long need = c->passes < 3 ? 3 : c->passes, k;
    for (k = 1; k <= need; k++) {
        int a_side = (int)(k & 1);
        if (k > 1 && !c->passfn[k]) {
            if (c->passes >= k) return 1;
            break;
        }
        c->mlen[k] = c->msg_cap;
        step_begin(c, 1 + (int)k, &t0);
        if (k == 1)
            r = c->kex.pass1_a(c->sk, ska_len, c->pkb, c->pkb_len, c->sta, &sta_len, c->msgs[1], &c->mlen[1]);
        else if (a_side)
            r = c->passfn[k](c->sk, ska_len, c->pkb, c->pkb_len, c->msgs[k-1], c->mlen[k-1],
                             c->sta, &sta_len, c->msgs[k], &c->mlen[k]);
        else
            r = c->passfn[k](c->skb, c->skb_len, c->pk, pka_len, c->msgs[k-1], c->mlen[k-1],
                             c->stb, &stb_len, c->msgs[k], &c->mlen[k]);
        step_end(c, 1 + (int)k, t0);
        if (r < 0 || c->mlen[k] > c->msg_cap || sta_len > c->sta_cap || stb_len > c->stb_cap) return 1;
        if (a_side) { ma = c->msgs[k]; ma_len = c->mlen[k]; } else { mb = c->msgs[k]; mb_len = c->mlen[k]; }
        if (r == 1) { if (c->passes != k) return 1; break; }
    }
    c->kex_steps = k > need ? need : k;
    step_begin(c, KEX_DERIVE_A, &t0);
    r = c->kex.derive_a(c->sk, ska_len, c->pkb, c->pkb_len, mb, mb_len, c->sta, sta_len, c->ss, &ssa_len);
    step_end(c, KEX_DERIVE_A, t0);
    if (r < 0 || ssa_len > c->ss_cap) return 1;
    step_begin(c, KEX_DERIVE_B, &t0);
    r = c->kex.derive_b(c->skb, c->skb_len, c->pk, pka_len, ma, ma_len, c->stb, stb_len, c->ssb, &ssb_len);
    step_end(c, KEX_DERIVE_B, t0);
    if (r < 0 || ssb_len != ssa_len || memcmp(c->ss, c->ssb, ssa_len)) return 1;
    c->ss_len = ssa_len;
    return 0;
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
            /* A scheme whose signing can fail for some (message, salt) pairs is
             * retried, as an application would; the retries are timed and counted. */
            for (int tries = 0; tries < 64; tries++) {
                c->sig_len = c->sig_cap;
                if (!c->sig_api.sign(c->sk, c->sk_len, c->msg, 64, c->sig, &c->sig_len)) {
                    if (c->sig_len > c->sig_cap && c->sig_len - c->sig_cap > c->len_excess)
                        c->len_excess = c->sig_len - c->sig_cap;
                    return c->sig_len > c->sig_cap + SIG_GUARD;   /* beyond the guard: unsafe */
                }
                c->sign_failures++;
            }
            return 1;
        }
        if (!strcmp(op, "verify")) {
            /* the honest signature verified once in setup; a verifier that later
             * rejects it (nondeterministic verdict) is still timed, and counted */
            if (c->sig_api.verify(c->pk, c->pk_len, c->sig, c->sig_len, c->msg, 64))
                c->verify_rejects++;
            return 0;
        }
    }
    if (c->meta->type == NGCC_TYPE_KEX)
        return kex_exchange(c);
    if (c->meta->type == NGCC_TYPE_HASH && !strcmp(op, "hash"))
        return c->hash_api.hash((int)((const ngcc_meta_hash_t *)c->meta)->digest_bits,
                                c->msg, (unsigned long long)c->input_bytes * 8,
                                c->digest);
    if (c->meta->type == NGCC_TYPE_HASH && c->xof_out)
        return c->hash_api.hash((int)(c->xof_out * 8), c->msg, c->xof_seed * 8, c->digest);
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
        c->pk = buffer(c->pk_cap); c->sk = buffer(c->sk_cap); c->sig = buffer(c->sig_cap + SIG_GUARD);
        c->msg = buffer(64);
        for (size_t i = 0; i < 64; i++) c->msg[i] = (unsigned char)(i * 131U + 64U);
        c->pk_len = c->pk_cap; c->sk_len = c->sk_cap;
        if (strcmp(c->operation, "keygen")) {
            if (c->sig_api.keygen(c->pk, &c->pk_len, c->sk, &c->sk_len) ||
                c->pk_len > c->pk_cap || c->sk_len > c->sk_cap)
                { fprintf(stderr, "setup keygen failed\n"); exit(3); }
        }
        if (!strcmp(c->operation, "verify")) {
            /* One honest signature to verify; correctness itself is established by
             * the KAT stage, and rejections while timing are counted. A signer that
             * fails is retried; a deterministic signer that keeps failing on this
             * message gets a different 64-byte message (last byte tweaked). */
            int ok = 0;
            for (int tweak = 0; tweak < 16 && !ok; tweak++) {
                if (tweak) { c->msg[63]++; c->msg_tweaks++; }
                for (int tries = 0; tries < 16 && !ok; tries++) {
                    c->sig_len = c->sig_cap;
                    ok = !c->sig_api.sign(c->sk, c->sk_len, c->msg, 64, c->sig, &c->sig_len) &&
                         c->sig_len <= c->sig_cap + SIG_GUARD;
                    if (ok && c->sig_len > c->sig_cap && c->sig_len - c->sig_cap > c->len_excess)
                        c->len_excess = c->sig_len - c->sig_cap;
                    if (!ok) c->sign_failures++;
                }
            }
            if (!ok) { fprintf(stderr, "setup signing failed\n"); exit(3); }
        }
    } else if (c->meta->type == NGCC_TYPE_HASH) {
        const ngcc_meta_hash_t *m = (const ngcc_meta_hash_t *)c->meta;
#define X(field, symbol) LOAD_FN(c->hash_api.field, c->library, symbol);
        NGCC_HASH_SYMBOLS
#undef X
        if (c->input_bytes > (1U << 24)) {
            fprintf(stderr, "hash input must be at most 16 MiB\n"); exit(2);
        }
        if (!strncmp(c->operation, "xof-", 4)) {
            /* output-length test: BYTES is the requested output, the seed is short */
            char *end = NULL;
            c->xof_seed = strtoull(c->operation + 4, &end, 10);
            c->xof_out = c->input_bytes;
            if (!end || *end || !c->xof_seed || c->xof_seed > 4096 || !c->xof_out) {
                fprintf(stderr, "xof-SEED needs 1 <= SEED <= 4096 and BYTES >= 1\n"); exit(2);
            }
            c->digest_cap = c->xof_out > m->digest_len ? c->xof_out : m->digest_len;
            c->msg = buffer(c->xof_seed); c->digest = buffer(c->digest_cap);
            for (size_t i = 0; i < c->xof_seed; i++)
                c->msg[i] = (unsigned char)(i * 131U + c->xof_seed);
        } else {
            c->digest_cap = m->digest_len;
            c->msg = buffer(c->input_bytes); c->digest = buffer(c->digest_cap);
            for (size_t i = 0; i < c->input_bytes; i++)
                c->msg[i] = (unsigned char)(i * 131U + c->input_bytes);
        }
    } else if (c->meta->type == NGCC_TYPE_KEX) {
        const ngcc_meta_kex_t *m = (const ngcc_meta_kex_t *)c->meta;
        memset(&c->kex, 0, sizeof c->kex);
#define X(field, symbol) LOAD_FN(c->kex.field, c->library, symbol);
        NGCC_KEX_SYMBOLS
#undef X
#define X(field, symbol) do { void *f_ = dlsym(c->library, symbol); memcpy(&c->kex.field, &f_, sizeof c->kex.field); } while (0);
        NGCC_KEX_OPTIONAL_SYMBOLS
#undef X
        c->passfn[2] = c->kex.pass2_b; c->passfn[3] = c->kex.pass3_a;
        for (int k = 4; k <= KEX_MAX_PASSES; k++) {
            char name[64];
            snprintf(name, sizeof name, "kex_generate_pass%d_msg_%c", k, (k & 1) ? 'a' : 'b');
            void *f = dlsym(c->library, name);
            memcpy(&c->passfn[k], &f, sizeof c->passfn[k]);
        }
        c->passes = m->passes;
        if (c->passes > KEX_MAX_PASSES) { fprintf(stderr, "unsupported pass count\n"); exit(2); }
        c->pk_cap = m->pk_len; c->sk_cap = m->sk_len; c->sta_cap = m->sta_len; c->stb_cap = m->stb_len;
        c->ss_cap = m->ss_len; c->msg_cap = m->total_msg_len;
        c->pk = buffer(c->pk_cap); c->sk = buffer(c->sk_cap); c->pkb = buffer(c->pk_cap); c->skb = buffer(c->sk_cap);
        c->sta = buffer(c->sta_cap); c->stb = buffer(c->stb_cap); c->ss = buffer(c->ss_cap); c->ssb = buffer(c->ss_cap);
        for (int k = 1; k <= KEX_MAX_PASSES; k++) c->msgs[k] = buffer(c->msg_cap);
        const char *op = c->operation;
        if (!strcmp(op, "exchange")) c->kex_step = -1;
        else if (!strcmp(op, "init_a")) c->kex_step = 0;
        else if (!strcmp(op, "init_b")) c->kex_step = 1;
        else if (!strcmp(op, "derive_a")) c->kex_step = KEX_DERIVE_A;
        else if (!strcmp(op, "derive_b")) c->kex_step = KEX_DERIVE_B;
        else if (!strncmp(op, "pass", 4) && atoi(op + 4) >= 1 && atoi(op + 4) <= (int)c->passes)
            c->kex_step = 1 + atoi(op + 4);
        else { fprintf(stderr, "unsupported KEX operation %s\n", op); exit(2); }
        c->perf_fd = -1;
        int saved = c->kex_step;
        c->kex_step = -2;   /* untimed honest run: checks agreement and records message lengths */
        if (kex_exchange(c)) { fprintf(stderr, "setup key exchange failed or secrets differ\n"); exit(3); }
        c->kex_step = saved;
    } else {
        fprintf(stderr, "unsupported algorithm type\n");
        exit(2);
    }
}

int main(int argc, char **argv)
{
    if (argc != 6 && argc != 8) {
        fprintf(stderr, "usage: %s LIB OP BYTES TRIALS LIMIT_SECONDS [ITERS WARMUPS]\n", argv[0]);
        return 2;
    }
    int trials = atoi(argv[4]);
    double limit_sec = atof(argv[5]);
    long long fixed_iters = argc == 8 ? atoll(argv[6]) : 0;
    int fixed_warmups = argc == 8 ? atoi(argv[7]) : 3;
    if (argc == 6 && (trials != 5 || !(limit_sec > 0 && limit_sec <= 600))) {
        fprintf(stderr, "expected five trials and a positive limit <= 600s\n"); return 2;
    }
    if (argc == 8 && (trials < 1 || trials > 16 || fixed_iters < 1 || fixed_warmups < 0)) {
        fprintf(stderr, "expected 1..16 trials, ITERS >= 1 and WARMUPS >= 0\n"); return 2;
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
    const char *(*full_flags)(void) = NULL;
    void *ff = dlsym(c.library, "ngcc_build_flags");
    memcpy(&full_flags, &ff, sizeof full_flags);
    printf("META\tbuild_flags\t%s\n", full_flags ? full_flags() : c.meta->build_flags);
    printf("META\tbaseline_rss_bytes\t%" PRIu64 "\n", resident_bytes());
    printf("META\tpk_bytes\t%" PRIu64 "\nMETA\tsk_bytes\t%" PRIu64 "\n",
           c.pk_cap, c.sk_cap);
    printf("META\tct_bytes\t%" PRIu64 "\nMETA\tss_bytes\t%" PRIu64 "\n",
           c.ct_cap, c.ss_cap);
    printf("META\tsignature_bytes\t%" PRIu64 "\nMETA\tdigest_bytes\t%" PRIu64 "\n",
           c.sig_cap, c.digest_cap);
    if (c.xof_out)
        printf("META\txof_seed_bytes\t%llu\nMETA\txof_output_bytes\t%llu\n", c.xof_seed, c.xof_out);
    if (c.meta->type == NGCC_TYPE_KEX) {
        unsigned long long total = 0;
        printf("META\tkex_passes\t%llu\nMETA\tkex_passes_run\t%llu\n", c.passes, c.kex_steps);
        printf("META\tsta_bytes\t%" PRIu64 "\nMETA\tstb_bytes\t%" PRIu64 "\n", c.sta_cap, c.stb_cap);
        for (unsigned long long k = 1; k <= c.kex_steps; k++) {
            printf("META\tmsg%llu_bytes\t%llu\n", k, c.mlen[k]);
            total += c.mlen[k];
        }
        printf("META\ttotal_msg_bytes\t%llu\nMETA\tss_actual_bytes\t%llu\n", total, c.ss_len);
    }
    printf("META\tmode\t%s\n", fixed_iters ? "fixed_iterations" : "time_bounded");
    fflush(stdout);

    int perf_fd = cycles_fd();
    int step_mode = c.meta->type == NGCC_TYPE_KEX && c.kex_step >= 0;
    c.perf_fd = step_mode ? perf_fd : -1;
    printf("META\tcpu_cycles_available\t%s\n", perf_fd >= 0 ? "yes" : "no");
    printf("META\ttiming_scope\t%s\n", step_mode ? "per_call_step" : "loop");
    uint64_t start = now_ns();
    uint64_t deadline = fixed_iters ? UINT64_MAX : start + (uint64_t)(limit_sec * 1e9);
    int warmups = 0;
    for (; warmups < fixed_warmups && now_ns() < deadline; warmups++)
        if (operate(&c)) { fprintf(stderr, "warm-up operation failed\n"); return 3; }
    printf("META\twarmups\t%d\n", warmups);
    fflush(stdout);

    int completed_trials = 0;
    unsigned long long total_samples = 0;
    for (int trial = 0; trial < trials; trial++) {
        uint64_t trial_start = now_ns();
        if (trial_start >= deadline) break;
        uint64_t trial_deadline = fixed_iters ? UINT64_MAX : start +
            (uint64_t)((double)(trial + 1) * limit_sec * 1e9 / trials);
        unsigned long long count = 0;
        uint64_t cycles = 0;
        c.step_ns = c.step_cycles = 0;
        if (perf_fd >= 0 && !step_mode) {
            ioctl(perf_fd, PERF_EVENT_IOC_RESET, 0);
            ioctl(perf_fd, PERF_EVENT_IOC_ENABLE, 0);
        }
        do {
            if (operate(&c)) { fprintf(stderr, "measured operation failed\n"); return 3; }
            count++;
            if (fixed_iters) {
                if (count >= (unsigned long long)fixed_iters) break;
            } else if ((count & 15ULL) == 0 || count < 20) {
                uint64_t current = now_ns();
                if (current >= trial_deadline || current >= deadline) break;
                if (count >= 100 && current - trial_start >= UINT64_C(1000000000)) break;
                if (count >= 20000) break;
            }
        } while (1);
        uint64_t trial_end = now_ns();
        if (perf_fd >= 0 && !step_mode) {
            ioctl(perf_fd, PERF_EVENT_IOC_DISABLE, 0);
            if (read(perf_fd, &cycles, sizeof cycles) != sizeof cycles) cycles = 0;
        }
        double seconds = (double)(trial_end - trial_start) / 1e9;
        if (step_mode) { seconds = (double)c.step_ns / 1e9; cycles = c.step_cycles; }
        printf("TRIAL\t%d\t%llu\t%.9f\t%" PRIu64 "\n", trial + 1, count, seconds, cycles);
        fflush(stdout);
        total_samples += count;
        completed_trials++;
    }
    if (c.meta->type == NGCC_TYPE_SIG)
        printf("META\tsign_failures_retried\t%llu\nMETA\tmessage_tweaks\t%llu\n"
               "META\tsetup_verify_failed\t%d\nMETA\tsignature_length_excess\t%llu\n"
               "META\tverify_rejections\t%llu\n",
               c.sign_failures, c.msg_tweaks, c.setup_verify_failed, c.len_excess, c.verify_rejects);
    printf("META\tpeak_rss_bytes\t%" PRIu64 "\n", peak_resident_bytes());
    printf("META\tmeasurement_count\t%llu\n", total_samples);
    printf("META\tguide_100_measurements\t%s\n", total_samples >= 100 ? "met" : "not_met");
    printf("STATUS\t%s\n", completed_trials == trials ? "complete" : "partial");
    if (perf_fd >= 0) close(perf_fd);
    dlclose(c.library);
    return completed_trials == trials ? 0 : 4;
}
