/* Deterministic whole-protocol decryption-failure search for LoomKEX.
 *
 * This is linked directly to one submitted KEX implementation.  Every trial
 * seeds the official ICCS DRNG, generates both long-term key pairs, and attempts
 * all four honest protocol passes and both shared-secret derivations.  A witness
 * contains the seed, keys, serialized protocol states, wire
 * messages and return code needed to replay the complete scheme failure.
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "drng.h"

#ifndef LOOM_SEARCH_HEADER
#error "LOOM_SEARCH_HEADER must name the submitted KEX header"
#endif
#include LOOM_SEARCH_HEADER

#ifndef LOOM_SEARCH_INSTANCE
#define LOOM_SEARCH_INSTANCE "LoomKEX-unknown"
#endif
#ifndef LOOM_SEARCH_IMPLEMENTATION
#define LOOM_SEARCH_IMPLEMENTATION "submitted-implementation"
#endif

#define LOOM_SEED_BYTES 64

DRNG_ctx drng_algorithm;

typedef struct {
    unsigned char *pka, *ska, *pkb, *skb, *sta, *stb;
    unsigned char *m1, *m2, *m3, *m4, *ssa, *ssb;
    unsigned long long pk_cap, sk_cap, sta_cap, stb_cap, msg_cap, ss_cap;
    unsigned long long pka_n, ska_n, pkb_n, skb_n, sta_n, stb_n;
    unsigned long long m1_n, m2_n, m3_n, m4_n, ssa_n, ssb_n;
    const char *stage;
    int code;
} trial_t;

static uint64_t splitmix64(uint64_t *state)
{
    uint64_t z = (*state += UINT64_C(0x9e3779b97f4a7c15));
    z = (z ^ (z >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94d049bb133111eb);
    return z ^ (z >> 31);
}

static void seed_for_index(uint64_t index, unsigned char seed[LOOM_SEED_BYTES])
{
    uint64_t state = index ^ UINT64_C(0x4c4f4f4d2d444652); /* "LOOM-DFR" */
    for (size_t i = 0; i < LOOM_SEED_BYTES / 8; i++) {
        uint64_t word = splitmix64(&state);
        for (size_t j = 0; j < 8; j++) seed[8 * i + j] = (unsigned char)(word >> (8 * j));
    }
}

static int allocate_trial(trial_t *t)
{
    memset(t, 0, sizeof(*t));
    t->pk_cap = kex_get_pk_len_bytes();
    t->sk_cap = kex_get_sk_len_bytes();
    t->sta_cap = kex_get_sta_len_bytes();
    t->stb_cap = kex_get_stb_len_bytes();
    t->msg_cap = kex_get_total_msg_len_bytes();
    t->ss_cap = kex_get_ss_len_bytes();
#define ALLOC(field, size) do { t->field = calloc((size) ? (size) : 1, 1); if (!t->field) return -1; } while (0)
    ALLOC(pka, t->pk_cap); ALLOC(ska, t->sk_cap);
    ALLOC(pkb, t->pk_cap); ALLOC(skb, t->sk_cap);
    ALLOC(sta, t->sta_cap); ALLOC(stb, t->stb_cap);
    ALLOC(m1, t->msg_cap); ALLOC(m2, t->msg_cap);
    ALLOC(m3, t->msg_cap); ALLOC(m4, t->msg_cap);
    ALLOC(ssa, t->ss_cap); ALLOC(ssb, t->ss_cap);
#undef ALLOC
    return 0;
}

static void free_trial(trial_t *t)
{
    free(t->pka); free(t->ska); free(t->pkb); free(t->skb);
    free(t->sta); free(t->stb); free(t->m1); free(t->m2);
    free(t->m3); free(t->m4); free(t->ssa); free(t->ssb);
    memset(t, 0, sizeof(*t));
}

static void reset_trial(trial_t *t)
{
    /* A failed pass may leave later outputs untouched.  Clear every output so
     * that a witness depends only on this trial, not on the preceding one. */
    memset(t->pka, 0, t->pk_cap); memset(t->ska, 0, t->sk_cap);
    memset(t->pkb, 0, t->pk_cap); memset(t->skb, 0, t->sk_cap);
    memset(t->sta, 0, t->sta_cap); memset(t->stb, 0, t->stb_cap);
    memset(t->m1, 0, t->msg_cap); memset(t->m2, 0, t->msg_cap);
    memset(t->m3, 0, t->msg_cap); memset(t->m4, 0, t->msg_cap);
    memset(t->ssa, 0, t->ss_cap); memset(t->ssb, 0, t->ss_cap);
    t->pka_n = t->pkb_n = t->pk_cap;
    t->ska_n = t->skb_n = t->sk_cap;
    t->sta_n = t->sta_cap; t->stb_n = t->stb_cap;
    t->m1_n = t->m2_n = t->m3_n = t->m4_n = 0;
    t->ssa_n = t->ssb_n = 0;
    t->stage = "complete"; t->code = 0;
}

/* Return 0 for agreement, 1 for an observable whole-scheme failure. */
static int run_trial(trial_t *t, const unsigned char seed[LOOM_SEED_BYTES])
{
    int r;
    reset_trial(t);
    r = init_random_number(&drng_algorithm, seed, LOOM_SEED_BYTES);
    if (r) { t->stage = "seed"; t->code = r; return 1; }

#define CALL_ZERO(label, expression) do { \
    r = (expression); \
    if (r != 0) { t->stage = (label); t->code = r; return 1; } \
} while (0)
    CALL_ZERO("init_a", kex_init_a(t->pka, &t->pka_n, t->ska, &t->ska_n, t->sta, &t->sta_n));
    CALL_ZERO("init_b", kex_init_b(t->pkb, &t->pkb_n, t->skb, &t->skb_n, t->stb, &t->stb_n));
    CALL_ZERO("pass1", kex_generate_pass1_msg_a(
        t->ska, t->ska_n, t->pkb, t->pkb_n, t->sta, &t->sta_n, t->m1, &t->m1_n));
    CALL_ZERO("pass2", kex_generate_pass2_msg_b(
        t->skb, t->skb_n, t->pka, t->pka_n, t->m1, t->m1_n,
        t->stb, &t->stb_n, t->m2, &t->m2_n));
    CALL_ZERO("pass3", kex_generate_pass3_msg_a(
        t->ska, t->ska_n, t->pkb, t->pkb_n, t->m2, t->m2_n,
        t->sta, &t->sta_n, t->m3, &t->m3_n));
#undef CALL_ZERO

    r = kex_generate_pass4_msg_b(
        t->skb, t->skb_n, t->pka, t->pka_n, t->m3, t->m3_n,
        t->stb, &t->stb_n, t->m4, &t->m4_n);
    if (r != 1) { t->stage = "pass4"; t->code = r; return 1; }

    t->ssa_n = t->ssb_n = t->ss_cap;
    r = kex_derive_ss_a(
        t->ska, t->ska_n, t->pkb, t->pkb_n, t->m4, t->m4_n,
        t->sta, t->sta_n, t->ssa, &t->ssa_n);
    if (r != 0) { t->stage = "derive_a"; t->code = r; return 1; }
    r = kex_derive_ss_b(
        t->skb, t->skb_n, t->pka, t->pka_n, t->m3, t->m3_n,
        t->stb, t->stb_n, t->ssb, &t->ssb_n);
    if (r != 0) { t->stage = "derive_b"; t->code = r; return 1; }
    if (t->ssa_n != t->ssb_n || memcmp(t->ssa, t->ssb, t->ssa_n)) {
        t->stage = "shared_secret_mismatch"; t->code = -1; return 1;
    }
    return 0;
}

static void write_hex(FILE *f, const char *name, const unsigned char *value, unsigned long long n)
{
    fprintf(f, "%s_len=%llu\n%s=", name, n, name);
    for (unsigned long long i = 0; i < n; i++) fprintf(f, "%02x", value[i]);
    fputc('\n', f);
}

static int write_witness(const char *path, uint64_t index,
                         const unsigned char seed[LOOM_SEED_BYTES], const trial_t *t)
{
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fprintf(f, "format=ngcc-loom-whole-scheme-failure-v1\n");
    fprintf(f, "instance=%s\nimplementation=%s\n",
            LOOM_SEARCH_INSTANCE, LOOM_SEARCH_IMPLEMENTATION);
    fprintf(f, "index=%" PRIu64 "\nfailure_stage=%s\nreturn_code=%d\n", index, t->stage, t->code);
    write_hex(f, "seed", seed, LOOM_SEED_BYTES);
    write_hex(f, "pka", t->pka, t->pka_n); write_hex(f, "ska", t->ska, t->ska_n);
    write_hex(f, "pkb", t->pkb, t->pkb_n); write_hex(f, "skb", t->skb, t->skb_n);
    write_hex(f, "sta", t->sta, t->sta_n); write_hex(f, "stb", t->stb, t->stb_n);
    write_hex(f, "m1", t->m1, t->m1_n); write_hex(f, "m2", t->m2, t->m2_n);
    write_hex(f, "m3", t->m3, t->m3_n); write_hex(f, "m4", t->m4, t->m4_n);
    write_hex(f, "ssa", t->ssa, t->ssa_n); write_hex(f, "ssb", t->ssb, t->ssb_n);
    return fclose(f) ? -1 : 0;
}

static double seconds_since(const struct timespec *start)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)(now.tv_sec - start->tv_sec) + 1e-9 * (double)(now.tv_nsec - start->tv_nsec);
}

static int parse_u64(const char *s, uint64_t *out)
{
    char *end = NULL;
    errno = 0;
    unsigned long long value = strtoull(s, &end, 0);
    if (errno || !end || *end) return -1;
    *out = (uint64_t)value;
    return 0;
}

int main(int argc, char **argv)
{
    uint64_t start, stride, trials, progress = 100000;
    unsigned char seed[LOOM_SEED_BYTES];
    struct timespec began;
    trial_t trial;

    if (argc < 5 || argc > 6 || parse_u64(argv[1], &start) ||
        parse_u64(argv[2], &stride) || parse_u64(argv[3], &trials) || !stride ||
        (argc == 6 && parse_u64(argv[5], &progress))) {
        fprintf(stderr, "usage: %s START STRIDE TRIALS WITNESS [PROGRESS]\n", argv[0]);
        return 2;
    }
    if (kex_get_passes_num() != 4) {
        fprintf(stderr, "%s: expected four passes, got %llu\n", argv[0], kex_get_passes_num());
        return 2;
    }
    if (allocate_trial(&trial)) {
        fprintf(stderr, "%s: allocation failed\n", argv[0]);
        return 2;
    }
    clock_gettime(CLOCK_MONOTONIC, &began);
    for (uint64_t i = 0, index = start; i < trials; i++, index += stride) {
        seed_for_index(index, seed);
        if (run_trial(&trial, seed)) {
            if (write_witness(argv[4], index, seed, &trial)) {
                fprintf(stderr, "%s: could not write witness %s\n", argv[0], argv[4]);
                free_trial(&trial);
                return 2;
            }
            fprintf(stdout, "FOUND instance=%s index=%" PRIu64 " stage=%s code=%d trials=%" PRIu64 " seconds=%.3f\n",
                    LOOM_SEARCH_INSTANCE, index, trial.stage, trial.code, i + 1, seconds_since(&began));
            free_trial(&trial);
            return 0;
        }
        if (progress && (i + 1) % progress == 0) {
            double elapsed = seconds_since(&began);
            fprintf(stdout, "PROGRESS instance=%s start=%" PRIu64 " trials=%" PRIu64 " rate=%.1f/s\n",
                    LOOM_SEARCH_INSTANCE, start, i + 1, (double)(i + 1) / elapsed);
            fflush(stdout);
        }
    }
    fprintf(stdout, "DONE instance=%s start=%" PRIu64 " trials=%" PRIu64 " seconds=%.3f\n",
            LOOM_SEARCH_INSTANCE, start, trials, seconds_since(&began));
    free_trial(&trial);
    return 1;
}
