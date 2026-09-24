#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _DARWIN_C_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>

#if defined(__APPLE__) && (defined(__aarch64__) || defined(__arm64__))
#include <dlfcn.h>
#endif

#include "SIG_AlgorithmInstance.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

#define DEFAULT_ITERS      10ULL
#define MESSAGE_LEN_BYTES  64ULL
#define SEED_LEN_BYTES     64ULL

static int cycle_ok = 0;
static int cycle_32bit = 0;
static const char *cycle_source_name = "none";

#if defined(__x86_64__) || defined(__i386__)

static inline uint64_t cycle_read_raw(void)
{
    uint32_t lo;
    uint32_t hi;
#if defined(__x86_64__)
    __asm__ __volatile__("lfence\n\trdtsc\n\tlfence" : "=a" (lo), "=d" (hi) :: "memory");
#else
    __asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi) :: "memory");
#endif
    return ((uint64_t)hi << 32) | (uint64_t)lo;
}

static int cycle_init(void)
{
    cycle_ok = 1;
    cycle_32bit = 0;
    cycle_source_name = "rdtsc";
    return 1;
}

static uint64_t cycle_read(void)
{
    return cycle_read_raw();
}

#elif defined(__APPLE__) && (defined(__aarch64__) || defined(__arm64__))

#define KPC_CLASS_FIXED 0
#define KPC_CLASS_CONFIGURABLE 1
#define KPC_CLASS_FIXED_MASK (1u << KPC_CLASS_FIXED)
#define KPC_CLASS_CONFIGURABLE_MASK (1u << KPC_CLASS_CONFIGURABLE)
#define KPC_MAX_COUNTERS 32

typedef uint64_t kpc_config_t;
typedef struct kpep_db kpep_db;
typedef struct kpep_config kpep_config;
typedef struct kpep_event kpep_event;

typedef int kpc_force_all_ctrs_get_proc(int *);
typedef int kpc_force_all_ctrs_set_proc(int);
typedef uint32_t kpc_get_counter_count_proc(uint32_t);
typedef uint32_t kpc_get_config_count_proc(uint32_t);
typedef int kpc_set_config_proc(uint32_t, kpc_config_t *);
typedef int kpc_get_thread_counters_proc(uint32_t, uint32_t, uint64_t *);
typedef int kpc_set_counting_proc(uint32_t);
typedef int kpc_set_thread_counting_proc(uint32_t);
typedef int kpep_db_create_proc(const char *, kpep_db **);
typedef void kpep_db_free_proc(kpep_db *);
typedef int kpep_db_event_proc(kpep_db *, const char *, kpep_event **);
typedef int kpep_config_create_proc(kpep_db *, kpep_config **);
typedef void kpep_config_free_proc(kpep_config *);
typedef int kpep_config_force_counters_proc(kpep_config *);
typedef int kpep_config_add_event_proc(kpep_config *, kpep_event **, uint32_t, uint32_t *);
typedef int kpep_config_kpc_classes_proc(kpep_config *, uint32_t *);
typedef int kpep_config_kpc_count_proc(kpep_config *, size_t *);
typedef int kpep_config_kpc_map_proc(kpep_config *, size_t *, size_t);
typedef int kpep_config_kpc_proc(kpep_config *, kpc_config_t *, size_t);

static kpc_force_all_ctrs_get_proc *p_kpc_force_all_ctrs_get;
static kpc_force_all_ctrs_set_proc *p_kpc_force_all_ctrs_set;
static kpc_get_counter_count_proc *p_kpc_get_counter_count;
static kpc_get_config_count_proc *p_kpc_get_config_count;
static kpc_set_config_proc *p_kpc_set_config;
static kpc_get_thread_counters_proc *p_kpc_get_thread_counters;
static kpc_set_counting_proc *p_kpc_set_counting;
static kpc_set_thread_counting_proc *p_kpc_set_thread_counting;
static kpep_db_create_proc *p_kpep_db_create;
static kpep_db_free_proc *p_kpep_db_free;
static kpep_db_event_proc *p_kpep_db_event;
static kpep_config_create_proc *p_kpep_config_create;
static kpep_config_free_proc *p_kpep_config_free;
static kpep_config_force_counters_proc *p_kpep_config_force_counters;
static kpep_config_add_event_proc *p_kpep_config_add_event;
static kpep_config_kpc_classes_proc *p_kpep_config_kpc_classes;
static kpep_config_kpc_count_proc *p_kpep_config_kpc_count;
static kpep_config_kpc_map_proc *p_kpep_config_kpc_map;
static kpep_config_kpc_proc *p_kpep_config_kpc;

static uint64_t apple_counters[KPC_MAX_COUNTERS];
static uint32_t apple_counter_count;
static size_t apple_cycle_index;
static kpep_db *apple_db;
static kpep_config *apple_config_obj;

static int apple_load_symbol(void *h, void **dst, const char *name)
{
    *dst = dlsym(h, name);
    return *dst != NULL;
}

static void *apple_dlopen2(const char *a, const char *b)
{
    void *h;

    h = dlopen(a, RTLD_LAZY | RTLD_LOCAL);
    if (h) {
        return h;
    }

    return dlopen(b, RTLD_LAZY | RTLD_LOCAL);
}

static int apple_load_frameworks(void)
{
    void *kperf;
    void *kperfdata;

    kperf = apple_dlopen2("/System/Library/PrivateFrameworks/kperf.framework/kperf",
                          "/System/Library/PrivateFrameworks/kperf.framework/Versions/A/kperf");
    if (!kperf) {
        return 0;
    }

    kperfdata = apple_dlopen2("/System/Library/PrivateFrameworks/kperfdata.framework/kperfdata",
                              "/System/Library/PrivateFrameworks/kperfdata.framework/Versions/A/kperfdata");
    if (!kperfdata) {
        return 0;
    }

    if (!apple_load_symbol(kperf, (void **)&p_kpc_force_all_ctrs_get, "kpc_force_all_ctrs_get") ||
        !apple_load_symbol(kperf, (void **)&p_kpc_force_all_ctrs_set, "kpc_force_all_ctrs_set") ||
        !apple_load_symbol(kperf, (void **)&p_kpc_get_counter_count, "kpc_get_counter_count") ||
        !apple_load_symbol(kperf, (void **)&p_kpc_get_config_count, "kpc_get_config_count") ||
        !apple_load_symbol(kperf, (void **)&p_kpc_set_config, "kpc_set_config") ||
        !apple_load_symbol(kperf, (void **)&p_kpc_get_thread_counters, "kpc_get_thread_counters") ||
        !apple_load_symbol(kperf, (void **)&p_kpc_set_counting, "kpc_set_counting") ||
        !apple_load_symbol(kperf, (void **)&p_kpc_set_thread_counting, "kpc_set_thread_counting")) {
        return 0;
    }

    if (!apple_load_symbol(kperfdata, (void **)&p_kpep_db_create, "kpep_db_create") ||
        !apple_load_symbol(kperfdata, (void **)&p_kpep_db_free, "kpep_db_free") ||
        !apple_load_symbol(kperfdata, (void **)&p_kpep_db_event, "kpep_db_event") ||
        !apple_load_symbol(kperfdata, (void **)&p_kpep_config_create, "kpep_config_create") ||
        !apple_load_symbol(kperfdata, (void **)&p_kpep_config_free, "kpep_config_free") ||
        !apple_load_symbol(kperfdata, (void **)&p_kpep_config_force_counters, "kpep_config_force_counters") ||
        !apple_load_symbol(kperfdata, (void **)&p_kpep_config_add_event, "kpep_config_add_event") ||
        !apple_load_symbol(kperfdata, (void **)&p_kpep_config_kpc_classes, "kpep_config_kpc_classes") ||
        !apple_load_symbol(kperfdata, (void **)&p_kpep_config_kpc_count, "kpep_config_kpc_count") ||
        !apple_load_symbol(kperfdata, (void **)&p_kpep_config_kpc_map, "kpep_config_kpc_map") ||
        !apple_load_symbol(kperfdata, (void **)&p_kpep_config_kpc, "kpep_config_kpc")) {
        return 0;
    }

    return 1;
}

static int apple_find_cycles_event(kpep_db *db, kpep_event **ev)
{
    static const char *names[] = {
        "FIXED_CYCLES",
        "CPU_CLK_UNHALTED.THREAD",
        "CPU_CLK_UNHALTED.CORE",
        0
    };
    int i;

    for (i = 0; names[i]; i++) {
        if (p_kpep_db_event(db, names[i], ev) == 0 && *ev) {
            return 1;
        }
    }

    return 0;
}

static int apple_cycle_read(uint64_t *out)
{
    if (!p_kpc_get_thread_counters || apple_counter_count == 0 ||
        apple_cycle_index >= (size_t)apple_counter_count) {
        return 0;
    }

    if (p_kpc_get_thread_counters(0, apple_counter_count, apple_counters) != 0) {
        return 0;
    }

    *out = apple_counters[apple_cycle_index];
    return 1;
}

static int cycle_init(void)
{
    kpep_event *ev;
    uint32_t err;
    uint32_t classes;
    size_t reg_count;
    uint32_t config_counter_count;
    kpc_config_t regs[KPC_MAX_COUNTERS];
    size_t counter_map[KPC_MAX_COUNTERS];
    int force_ctrs;
    uint64_t a;
    uint64_t b;

    if (!apple_load_frameworks()) {
        return 0;
    }

    force_ctrs = 0;
    if (p_kpc_force_all_ctrs_get(&force_ctrs) != 0) {
        return 0;
    }

    if (p_kpep_db_create(0, &apple_db) != 0 || !apple_db) {
        return 0;
    }

    if (p_kpep_config_create(apple_db, &apple_config_obj) != 0 || !apple_config_obj) {
        return 0;
    }

    if (p_kpep_config_force_counters(apple_config_obj) != 0) {
        return 0;
    }

    ev = 0;
    if (!apple_find_cycles_event(apple_db, &ev)) {
        return 0;
    }

    err = 0;
    if (p_kpep_config_add_event(apple_config_obj, &ev, 0, &err) != 0) {
        return 0;
    }

    classes = 0;
    reg_count = 0;
    memset(regs, 0, sizeof(regs));
    memset(counter_map, 0, sizeof(counter_map));

    if (p_kpep_config_kpc_classes(apple_config_obj, &classes) != 0 ||
        p_kpep_config_kpc_count(apple_config_obj, &reg_count) != 0 ||
        p_kpep_config_kpc_map(apple_config_obj, counter_map, sizeof(counter_map)) != 0 ||
        p_kpep_config_kpc(apple_config_obj, regs, sizeof(regs)) != 0) {
        return 0;
    }

    if (classes == 0 || reg_count > KPC_MAX_COUNTERS || counter_map[0] >= KPC_MAX_COUNTERS) {
        return 0;
    }

    if (p_kpc_force_all_ctrs_set(1) != 0) {
        return 0;
    }

    config_counter_count = p_kpc_get_config_count(classes & KPC_CLASS_CONFIGURABLE_MASK);
    if ((classes & KPC_CLASS_CONFIGURABLE_MASK) && config_counter_count > 0) {
        if (p_kpc_set_config(classes, regs) != 0) {
            return 0;
        }
    }

    apple_counter_count = p_kpc_get_counter_count(classes);
    apple_cycle_index = counter_map[0];

    if (apple_counter_count == 0 || apple_counter_count > KPC_MAX_COUNTERS ||
        apple_cycle_index >= (size_t)apple_counter_count) {
        return 0;
    }

    if (p_kpc_set_counting(classes) != 0) {
        return 0;
    }

    if (p_kpc_set_thread_counting(classes) != 0) {
        return 0;
    }

    if (!apple_cycle_read(&a) || !apple_cycle_read(&b)) {
        return 0;
    }

    cycle_ok = 1;
    cycle_32bit = 0;
    cycle_source_name = "apple_kpep_cycles";
    return 1;
}

static uint64_t cycle_read(void)
{
    uint64_t x;

    if (!apple_cycle_read(&x)) {
        return 0;
    }

    return x;
}

#elif defined(__aarch64__)

static inline uint64_t cycle_read_raw(void)
{
    uint64_t x;
    __asm__ __volatile__("isb\n\tmrs %0, pmccntr_el0" : "=r" (x) :: "memory");
    return x;
}

static int cycle_init(void)
{
    cycle_ok = 1;
    cycle_32bit = 1;
    cycle_source_name = "pmccntr_el0";
    return 1;
}

static uint64_t cycle_read(void)
{
    return cycle_read_raw();
}

#elif defined(__arm__)

static inline uint64_t cycle_read_raw(void)
{
    uint32_t x;
    __asm__ __volatile__("mrc p15, 0, %0, c9, c13, 0" : "=r" (x) :: "memory");
    return (uint64_t)x;
}

static int cycle_init(void)
{
    cycle_ok = 1;
    cycle_32bit = 1;
    cycle_source_name = "pmccntr";
    return 1;
}

static uint64_t cycle_read(void)
{
    return cycle_read_raw();
}

#elif defined(__riscv)

static inline uint64_t cycle_read_raw(void)
{
#if defined(__riscv_xlen) && (__riscv_xlen == 64)
    uint64_t x;

    __asm__ __volatile__("csrr %0, mcycle" : "=r" (x) :: "memory");
    return x;
#else
    uint32_t lo;
    uint32_t hi;
    uint32_t hi2;

    do {
        __asm__ __volatile__("csrr %0, mcycleh" : "=r" (hi) :: "memory");
        __asm__ __volatile__("csrr %0, mcycle" : "=r" (lo) :: "memory");
        __asm__ __volatile__("csrr %0, mcycleh" : "=r" (hi2) :: "memory");
    } while (hi != hi2);

    return ((uint64_t)hi << 32) | (uint64_t)lo;
#endif
}

static int cycle_init(void)
{
    cycle_ok = 1;
    cycle_32bit = 0;
    cycle_source_name = "riscv_mcycle";
    return 1;
}

static uint64_t cycle_read(void)
{
    return cycle_read_raw();
}

#else

static int cycle_init(void)
{
    cycle_ok = 0;
    cycle_32bit = 0;
    cycle_source_name = "none";
    return 0;
}

static uint64_t cycle_read(void)
{
    return 0;
}

#endif

static uint64_t cycle_delta(uint64_t a, uint64_t b)
{
    if (cycle_32bit) {
        return (uint64_t)(uint32_t)(b - a);
    }
    return b - a;
}

static uint64_t time_ns(void)
{
#if defined(CLOCK_MONOTONIC_RAW)
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &t) == 0) {
        return (uint64_t)t.tv_sec * 1000000000ULL + (uint64_t)t.tv_nsec;
    }
#elif defined(CLOCK_MONOTONIC)
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t) == 0) {
        return (uint64_t)t.tv_sec * 1000000000ULL + (uint64_t)t.tv_nsec;
    }
#endif
    return (uint64_t)((long double)clock() * 1000000000.0L / (long double)CLOCKS_PER_SEC);
}

static void die_if(int condition, const char *what, int code)
{
    if (condition) {
        fprintf(stderr, "ERROR: %s failed with code %d\n", what, code);
        exit(EXIT_FAILURE);
    }
}

static void init_bench_drng(const char *label)
{
    unsigned char seed[SEED_LEN_BYTES];
    size_t label_len;
    size_t i;

    label_len = strlen(label);
    die_if(label_len == 0, "empty DRNG label", -1);

    for (i = 0; i < sizeof(seed); i++) {
        seed[i] = (unsigned char)(0xA5u ^ (unsigned int)i ^ (unsigned int)label[i % label_len]);
    }

    die_if(init_random_number(&drng_algorithm, seed, sizeof(seed)) != 0,
           "init_random_number", -1);
}

static void print_result(const char *name,
                         unsigned long long calls,
                         uint64_t n0,
                         uint64_t n1,
                         uint64_t cycles)
{
    uint64_t ns;
    double total_ms;
    double avg_ms;
    double avg_cycles;

    ns = n1 - n0;
    total_ms = (double)ns / 1000000.0;
    avg_ms = total_ms / (double)calls;

    if (cycle_ok) {
        avg_cycles = (double)cycles / (double)calls;
        printf("%-10s calls=%llu total_ms=%.6f avg_ms=%.9f total_cycles=%llu avg_cycles=%.2f\n",
               name, calls, total_ms, avg_ms, (unsigned long long)cycles, avg_cycles);
    } else {
        printf("%-10s calls=%llu total_ms=%.6f avg_ms=%.9f\n",
               name, calls, total_ms, avg_ms);
    }
}

static void mutate_message(unsigned char *m, unsigned long long i)
{
    m[0] = (unsigned char)(i & 0xffu);
    m[1] = (unsigned char)((i >> 8) & 0xffu);
    m[2] = (unsigned char)((i >> 16) & 0xffu);
    m[3] = (unsigned char)((i >> 24) & 0xffu);
}

int main(int argc, char **argv)
{
    unsigned long long iters;
    unsigned long long pk_len;
    unsigned long long sk_len;
    unsigned long long sn_len;
    unsigned char *pk;
    unsigned char *sk;
    unsigned char *sn;
    unsigned char msg[MESSAGE_LEN_BYTES];
    unsigned long long i;
    uint64_t n0;
    uint64_t n1;
    uint64_t c0;
    uint64_t c1;
    uint64_t cycles;
    volatile int verify_sink;

    iters = DEFAULT_ITERS;
    verify_sink = 0;

    if (argc >= 2) {
        iters = strtoull(argv[1], NULL, 10);
        if (iters == 0) {
            iters = DEFAULT_ITERS;
        }
    }

    cycle_init();

    pk_len = sig_get_pk_len_bytes();
    sk_len = sig_get_sk_len_bytes();
    sn_len = sig_get_sn_len_bytes();

    pk = (unsigned char *)calloc((size_t)pk_len, 1u);
    sk = (unsigned char *)calloc((size_t)sk_len, 1u);
    sn = (unsigned char *)calloc((size_t)sn_len, 1u);

    die_if(pk == NULL || sk == NULL || sn == NULL, "calloc", -1);

    for (i = 0; i < MESSAGE_LEN_BYTES; i++) {
        msg[i] = (unsigned char)(0x30u + (unsigned int)(i & 0x3fu));
    }

    printf("Algorithm instance: %s\n", ALGORITHM_INSTANCE);
    printf("Iterations: %llu\n", iters);
    printf("PK bytes: %llu, SK bytes: %llu, signature bytes: %llu\n",
           pk_len, sk_len, sn_len);
    printf("Cycle source: %s\n", cycle_source_name);

    init_bench_drng("facto-dsa-keygen-benchmark");

    cycles = 0;
    n0 = time_ns();

    for (i = 0; i < iters; i++) {
        unsigned long long out_pk_len;
        unsigned long long out_sk_len;
        int rc;

        out_pk_len = pk_len;
        out_sk_len = sk_len;

        c0 = cycle_ok ? cycle_read() : 0;
        rc = sig_keygen(pk, &out_pk_len, sk, &out_sk_len);
        c1 = cycle_ok ? cycle_read() : 0;

        if (cycle_ok) {
            cycles += cycle_delta(c0, c1);
        }

        die_if(rc != 0, "sig_keygen", rc);
        die_if(out_pk_len != pk_len || out_sk_len != sk_len,
               "sig_keygen length check", -2);
    }

    n1 = time_ns();

    print_result("keygen", iters, n0, n1, cycles);

    init_bench_drng("facto-dsa-setup-key");

    {
        unsigned long long out_pk_len;
        unsigned long long out_sk_len;
        int rc;

        out_pk_len = pk_len;
        out_sk_len = sk_len;

        rc = sig_keygen(pk, &out_pk_len, sk, &out_sk_len);
        die_if(rc != 0, "setup sig_keygen", rc);
    }

    init_bench_drng("facto-dsa-sign-benchmark");

    cycles = 0;
    n0 = time_ns();

    for (i = 0; i < iters; i++) {
        unsigned long long out_sn_len;
        int rc;

        out_sn_len = sn_len;
        mutate_message(msg, i);

        c0 = cycle_ok ? cycle_read() : 0;
        rc = sig_sign(sk, sk_len, msg, MESSAGE_LEN_BYTES, sn, &out_sn_len);
        c1 = cycle_ok ? cycle_read() : 0;

        if (cycle_ok) {
            cycles += cycle_delta(c0, c1);
        }

        die_if(rc != 0, "sig_sign", rc);
        die_if(out_sn_len != sn_len, "sig_sign length check", -3);
    }

    n1 = time_ns();

    print_result("sign", iters, n0, n1, cycles);

    mutate_message(msg, 0);
    init_bench_drng("facto-dsa-single-signature");

    {
        unsigned long long out_sn_len;
        int rc;

        out_sn_len = sn_len;

        rc = sig_sign(sk, sk_len, msg, MESSAGE_LEN_BYTES, sn, &out_sn_len);
        die_if(rc != 0, "setup sig_sign", rc);
    }

    cycles = 0;
    n0 = time_ns();

    for (i = 0; i < iters; i++) {
        int rc;

        c0 = cycle_ok ? cycle_read() : 0;
        rc = sig_verify(pk, pk_len, sn, sn_len, msg, MESSAGE_LEN_BYTES);
        c1 = cycle_ok ? cycle_read() : 0;

        if (cycle_ok) {
            cycles += cycle_delta(c0, c1);
        }

        die_if(rc != 0, "sig_verify", rc);

        verify_sink ^= rc;
    }

    n1 = time_ns();

    print_result("verify", iters, n0, n1, cycles);

    if (verify_sink != 0) {
        fprintf(stderr, "ERROR: impossible verification sink state\n");
        free(sn);
        free(sk);
        free(pk);
        return EXIT_FAILURE;
    }

    free(sn);
    free(sk);
    free(pk);

    return EXIT_SUCCESS;
}
