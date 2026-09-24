/*
 * Stage S6-02B round-component microbenchmark.
 *
 * Each instance has a tiny wrapper that includes afs_p1600.c before including
 * this file. That keeps the benchmark tied to the exact active round core and
 * lets it time the static nonlinear/round helpers without exporting new API.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "CryptHash_AlgorithmInstance.h"

#if defined(__x86_64__) || defined(__i386__)
/* Function rdtsc_read: reads a processor cycle counter or monotonic fallback value for benchmarking. */
static uint64_t rdtsc_read(void)
{
# if defined(__x86_64__)
    unsigned int lo, hi;
    __asm__ __volatile__("lfence\n\t"
                         "rdtsc\n\t"
                         : "=a"(lo), "=d"(hi)
                         :
                         : "memory");
    return ((uint64_t)hi << 32) | (uint64_t)lo;
# else
    unsigned long long v;
    __asm__ __volatile__("lfence\n\t"
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

static volatile uint64_t afs_microbench_sink;

/* Function wall_seconds: returns a wall-clock timestamp used by benchmark drivers. */
static double wall_seconds(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

static const char *sbox_backend_name(void)
{
#if defined(AFS_TREDM_USE_AVX2) && defined(__AVX2__)
    return "avx2";
#else
    return "opt64";
#endif
}

static const char *implementation_name(void)
{
#if defined(AFS_TREDM_USE_AVX2) && defined(__AVX2__)
    return "canonical-avx2-sbox-opt64-linear";
#else
    return "portable-opt64";
#endif
}

/* Function init_state: initializes a deterministic state or stream-hash context. */
static void init_state(uint64_t A[25], uint64_t domain)
{
    uint64_t x = UINT64_C(0x9e3779b97f4a7c15) ^ domain;
    unsigned i;
    for (i = 0; i < 25U; i++) {
        x += UINT64_C(0x9e3779b97f4a7c15);
        x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
        x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
        A[i] = x ^ (x >> 31);
    }
}

/* Function checksum_state: computes a simple checksum of a 1600-bit state for microbenchmarks. */
static uint64_t checksum_state(const uint64_t A[25])
{
    uint64_t x = UINT64_C(0x6a09e667f3bcc909);
    unsigned i;
    for (i = 0; i < 25U; i++) {
        x ^= A[i] + UINT64_C(0x9e3779b97f4a7c15) + (x << 6) + (x >> 2);
    }
    return x;
}

/* Function fill_message: fills a deterministic test or benchmark message buffer. */
static void fill_message(unsigned char *msg, size_t nbytes)
{
    uint64_t x = UINT64_C(0x243f6a8885a308d3);
    size_t i;
    for (i = 0; i < nbytes; i++) {
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        msg[i] = (unsigned char)(x >> 56);
    }
}

/* Function rate_bits_for_instance: returns the rate in bits for a benchmark instance. */
static unsigned rate_bits_for_instance(void)
{
#if DIGEST_BIT_LENGTH == 512
    return 1024U;
#elif DIGEST_BIT_LENGTH == 768
    return 768U;
#elif DIGEST_BIT_LENGTH == 1024
    return 512U;
#else
    return 1024U;
#endif
}

/* Function permutations_for_message_bits: estimates the number of permutations for a message length. */
static unsigned long long permutations_for_message_bits(unsigned long long msg_bits)
{
    const unsigned rate_bits = rate_bits_for_instance();
    const unsigned long long full_blocks = msg_bits / (unsigned long long)rate_bits;
    const unsigned rem_bits = (unsigned)(msg_bits % (unsigned long long)rate_bits);
    const unsigned total_no_pad = rem_bits + 1U + 128U;
    const unsigned zero_pad = (rate_bits - (total_no_pad % rate_bits)) % rate_bits;
    const unsigned final_blocks = (total_no_pad + zero_pad) / rate_bits;
    return full_blocks + (unsigned long long)final_blocks;
}

typedef void (*round_component_fn)(uint64_t A[25], unsigned round);

struct component_result {
    double elapsed;
    uint64_t cycles;
    unsigned long iterations;
    unsigned long long work_units;
    uint64_t checksum;
};

/* Function do_sbox_only: runs only the nonlinear layer component in the microbenchmark. */
static void do_sbox_only(uint64_t A[25], unsigned round)
{
    afs_p1600_nonlinear(A, AFS_RC[round % AFS_P1600_EFFECTIVE_ROUNDS]);
}

/* Function do_linear_only: runs only the linear layer component in the microbenchmark. */
static void do_linear_only(uint64_t A[25], unsigned round)
{
    afs_lmds1600_s6_opt64(A, round);
}

/* Function do_full_round: runs a full round component in the microbenchmark. */
static void do_full_round(uint64_t A[25], unsigned round)
{
    afs_p1600_round_opt(A, round, AFS_RC[round % AFS_P1600_EFFECTIVE_ROUNDS]);
}

/* Function run_round_component: benchmarks a round subcomponent. */
static struct component_result run_round_component(round_component_fn fn,
                                                   double target_seconds,
                                                   unsigned long initial_iterations)
{
    struct component_result res;
    uint64_t A[25];
    unsigned long iterations = initial_iterations;
    unsigned long i;
    unsigned r;

    memset(&res, 0, sizeof(res));
    init_state(A, (uint64_t)(uintptr_t)fn);

    do {
        double t0, t1;
        uint64_t c0 = 0, c1 = 0;
        t0 = wall_seconds();
#if HAVE_TSC
        c0 = rdtsc_read();
#endif
        for (i = 0; i < iterations; i++) {
            for (r = 0; r < AFS_P1600_EFFECTIVE_ROUNDS; r++) {
                fn(A, r);
            }
            A[(i + iterations) % 25UL] ^= (uint64_t)i + UINT64_C(0x100000001b3);
        }
#if HAVE_TSC
        c1 = rdtsc_read();
        res.cycles = c1 - c0;
#endif
        t1 = wall_seconds();
        res.elapsed = t1 - t0;
        if (res.elapsed < target_seconds && iterations < 10000000UL) {
            iterations *= 2UL;
        } else {
            break;
        }
    } while (1);

    res.iterations = iterations;
    res.work_units = (unsigned long long)iterations *
                     (unsigned long long)AFS_P1600_EFFECTIVE_ROUNDS;
    res.checksum = checksum_state(A);
    afs_microbench_sink ^= res.checksum;
    return res;
}

/* Function run_permutation_component: benchmarks the permutation component. */
static struct component_result run_permutation_component(double target_seconds,
                                                         unsigned long initial_iterations)
{
    struct component_result res;
    uint64_t A[25];
    unsigned long iterations = initial_iterations;
    unsigned long i;

    memset(&res, 0, sizeof(res));
    init_state(A, UINT64_C(0x7065726d75746531));

    do {
        double t0, t1;
        uint64_t c0 = 0, c1 = 0;
        t0 = wall_seconds();
#if HAVE_TSC
        c0 = rdtsc_read();
#endif
        for (i = 0; i < iterations; i++) {
            afs_p1600_permute(A, 0U, AFS_P1600_EFFECTIVE_ROUNDS);
            A[i % 25UL] ^= (uint64_t)i + UINT64_C(0xd1b54a32d192ed03);
        }
#if HAVE_TSC
        c1 = rdtsc_read();
        res.cycles = c1 - c0;
#endif
        t1 = wall_seconds();
        res.elapsed = t1 - t0;
        if (res.elapsed < target_seconds && iterations < 10000000UL) {
            iterations *= 2UL;
        } else {
            break;
        }
    } while (1);

    res.iterations = iterations;
    res.work_units = (unsigned long long)iterations;
    res.checksum = checksum_state(A);
    afs_microbench_sink ^= res.checksum;
    return res;
}

/* Function run_crypt_hash_component: benchmarks the complete CryptHash component. */
static struct component_result run_crypt_hash_component(double target_seconds,
                                                        unsigned long long msg_bits,
                                                        unsigned long initial_iterations)
{
    struct component_result res;
    const size_t digest_bytes = (size_t)DIGEST_BIT_LENGTH / 8U;
    const size_t msg_bytes = (size_t)((msg_bits + 7ULL) / 8ULL);
    unsigned char *msg = NULL;
    unsigned char *digest = NULL;
    unsigned long iterations = initial_iterations;
    unsigned long i;
    int rc;

    memset(&res, 0, sizeof(res));
    msg = (unsigned char *)malloc(msg_bytes ? msg_bytes : 1U);
    digest = (unsigned char *)calloc(digest_bytes, 1U);
    if (!msg || !digest) {
        fprintf(stderr, "microbench allocation failed\n");
        free(digest);
        free(msg);
        exit(1);
    }
    fill_message(msg, msg_bytes);

    rc = CryptHash(DIGEST_BIT_LENGTH, msg, msg_bits, digest);
    if (rc != 0) {
        fprintf(stderr, "CryptHash warm-up failed, rc=%d\n", rc);
        free(digest);
        free(msg);
        exit(1);
    }

    do {
        double t0, t1;
        uint64_t c0 = 0, c1 = 0;
        t0 = wall_seconds();
#if HAVE_TSC
        c0 = rdtsc_read();
#endif
        for (i = 0; i < iterations; i++) {
            rc = CryptHash(DIGEST_BIT_LENGTH, msg, msg_bits, digest);
            if (rc != 0) {
                fprintf(stderr, "CryptHash failed, rc=%d\n", rc);
                free(digest);
                free(msg);
                exit(1);
            }
            res.checksum ^= (uint64_t)digest[i % digest_bytes] << ((i & 7UL) * 8U);
        }
#if HAVE_TSC
        c1 = rdtsc_read();
        res.cycles = c1 - c0;
#endif
        t1 = wall_seconds();
        res.elapsed = t1 - t0;
        if (res.elapsed < target_seconds && iterations < 100000UL) {
            iterations *= 2UL;
        } else {
            break;
        }
    } while (1);

    res.iterations = iterations;
    res.work_units = (unsigned long long)iterations;
    afs_microbench_sink ^= res.checksum;
    free(digest);
    free(msg);
    return res;
}

/* Function print_component_row: prints one microbenchmark component result row. */
static void print_component_row(const char *component,
                                const char *unit,
                                unsigned long long msg_bits,
                                struct component_result res)
{
    printf("%s,%s,%llu,", component, unit, res.work_units);
    if (msg_bits == 0ULL) {
        printf("NA,");
    } else {
        printf("%llu,", msg_bits);
    }
    printf("%lu,%.6f,", res.iterations, res.elapsed);
#if HAVE_TSC
    if (res.work_units != 0ULL) {
        printf("%.2f,", (double)res.cycles / (double)res.work_units);
    } else {
        printf("NA,");
    }
    if (msg_bits != 0ULL && res.iterations != 0UL) {
        const double total_bytes = ((double)msg_bits / 8.0) * (double)res.iterations;
        printf("%.2f,", (double)res.cycles / total_bytes);
    } else {
        printf("NA,");
    }
#else
    printf("NA,NA,");
#endif
    printf("0x%016llX\n", (unsigned long long)res.checksum);
}

/* Function print_overhead_row: prints benchmark overhead accounting information. */
static void print_overhead_row(struct component_result hash_res,
                               double permutation_cycles,
                               unsigned long long msg_bits)
{
    const unsigned long long perms = permutations_for_message_bits(msg_bits);
    const double estimated_perm_cycles =
        permutation_cycles * (double)perms * (double)hash_res.iterations;
    double overhead_cycles = 0.0;
#if HAVE_TSC
    overhead_cycles = (double)hash_res.cycles - estimated_perm_cycles;
#endif
    printf("tredm_absorb_final_overhead_estimate,hash,%lu,%llu,%lu,%.6f,",
           hash_res.iterations, msg_bits, hash_res.iterations, hash_res.elapsed);
#if HAVE_TSC
    printf("%.2f,%.2f,",
           overhead_cycles / (double)hash_res.iterations,
           overhead_cycles / (((double)msg_bits / 8.0) * (double)hash_res.iterations));
#else
    printf("NA,NA,");
#endif
    printf("0x%016llX\n", (unsigned long long)hash_res.checksum);
}

/* Function main: executes this standalone test, benchmark, or utility program. */
int main(int argc, char **argv)
{
    double target_seconds = 0.08;
    unsigned long initial_round_iterations = 1000UL;
    unsigned long initial_perm_iterations = 500UL;
    unsigned long initial_hash_iterations = 4UL;
    const unsigned long long hash_msg_bits = 8388608ULL;
    struct component_result sbox_res;
    struct component_result linear_res;
    struct component_result round_res;
    struct component_result perm_res;
    struct component_result hash_res;
    double permutation_cycles = 0.0;

    if (argc > 1) {
        if (strcmp(argv[1], "--quick") == 0) {
            target_seconds = 0.025;
            initial_round_iterations = 200UL;
            initial_perm_iterations = 100UL;
            initial_hash_iterations = 1UL;
        } else if (strcmp(argv[1], "--full") == 0) {
            target_seconds = 0.20;
            initial_round_iterations = 2000UL;
            initial_perm_iterations = 1000UL;
            initial_hash_iterations = 4UL;
        } else {
            fprintf(stderr, "Usage: %s [--quick|--full]\n", argv[0]);
            return 2;
        }
    }

    printf("# algorithm_instance=%s\n", ALGORITHM_INSTANCE);
    printf("# implementation=%s\n", implementation_name());
    printf("# digest_bits=%d\n", DIGEST_BIT_LENGTH);
    printf("# split_rounds=%u\n", (unsigned)AFS_P1600_SPLIT_ROUNDS);
    printf("# effective_rounds=%u\n", (unsigned)AFS_P1600_EFFECTIVE_ROUNDS);
    printf("# sbox_backend=%s\n", sbox_backend_name());
    printf("# linear_backend=opt64-s6\n");
    printf("# avx2_linear_candidate_status=not_selected_single_state_irregular_xor_network_and_no_avx2_u64_variable_rotate\n");
#if HAVE_TSC
    printf("# tsc=available\n");
#else
    printf("# tsc=not_available\n");
#endif
    printf("# columns=component,unit,work_units,msg_bits,iterations,elapsed_seconds,cycles_per_unit,cycles_per_byte,checksum\n");

    sbox_res = run_round_component(do_sbox_only, target_seconds, initial_round_iterations);
    linear_res = run_round_component(do_linear_only, target_seconds, initial_round_iterations);
    round_res = run_round_component(do_full_round, target_seconds, initial_round_iterations);
    perm_res = run_permutation_component(target_seconds, initial_perm_iterations);
    hash_res = run_crypt_hash_component(target_seconds, hash_msg_bits, initial_hash_iterations);

#if HAVE_TSC
    if (perm_res.work_units != 0ULL) {
        permutation_cycles = (double)perm_res.cycles / (double)perm_res.work_units;
    }
#endif

    print_component_row("sbox_only", "round", 0ULL, sbox_res);
    print_component_row("s6_linear_only", "round", 0ULL, linear_res);
    print_component_row("full_round", "round", 0ULL, round_res);
    print_component_row("full_permutation", "permutation", 0ULL, perm_res);
    print_component_row("crypt_hash_1mib", "hash", hash_msg_bits, hash_res);
    print_overhead_row(hash_res, permutation_cycles, hash_msg_bits);

    return (afs_microbench_sink == 0U) ? 0 : 0;
}
