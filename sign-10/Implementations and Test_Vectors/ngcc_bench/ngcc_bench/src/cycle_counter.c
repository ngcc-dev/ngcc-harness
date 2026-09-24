#include "cycle_counter.h"

#ifdef __linux__
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#endif

#if defined(__APPLE__) && (defined(__aarch64__) || defined(__arm64__))
#include <dlfcn.h>
#include <stddef.h>
#endif

#if defined(__riscv) && defined(__linux__)
#include <setjmp.h>
#include <signal.h>
#endif

#include <stdint.h>
#include <string.h>
#include <unistd.h>

#if defined(__x86_64__)
static unsigned long long read_tsc(void) {
    unsigned int lo;
    unsigned int hi;

    __asm__ volatile ("lfence\n\t"
                      "rdtsc"
                      : "=a"(lo), "=d"(hi)
                      :
                      : "memory");

    return ((unsigned long long) hi << 32) | lo;
}
#endif

#if defined(__APPLE__) && (defined(__aarch64__) || defined(__arm64__))

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
static uint32_t apple_kpc_classes;
static kpep_db *apple_db;
static kpep_config *apple_config_obj;

static int apple_kpc_initialized;
static int apple_force_saved;
static int apple_force_saved_valid;

static int apple_load_symbol(void *h, void **dst, const char *name) {
    *dst = dlsym(h, name);
    return *dst != 0;
}

static void *apple_dlopen2(const char *a, const char *b) {
    void *h;

    h = dlopen(a, RTLD_LAZY | RTLD_LOCAL);
    if (h) {
        return h;
    }

    return dlopen(b, RTLD_LAZY | RTLD_LOCAL);
}

static int apple_load_frameworks(void) {
    static int attempted;
    static int loaded;
    void *kperf;
    void *kperfdata;

    if (attempted) {
        return loaded;
    }

    attempted = 1;

    kperf = apple_dlopen2(
        "/System/Library/PrivateFrameworks/kperf.framework/kperf",
        "/System/Library/PrivateFrameworks/kperf.framework/Versions/A/kperf"
    );
    if (!kperf) {
        loaded = 0;
        return 0;
    }

    kperfdata = apple_dlopen2(
        "/System/Library/PrivateFrameworks/kperfdata.framework/kperfdata",
        "/System/Library/PrivateFrameworks/kperfdata.framework/Versions/A/kperfdata"
    );
    if (!kperfdata) {
        loaded = 0;
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
        loaded = 0;
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
        loaded = 0;
        return 0;
    }

    loaded = 1;
    return 1;
}

static int apple_find_cycles_event(kpep_db *db, kpep_event **ev) {
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

static int apple_cycle_read(uint64_t *out) {
    if (!p_kpc_get_thread_counters ||
        apple_counter_count == 0 ||
        apple_cycle_index >= (size_t) apple_counter_count) {
        return 0;
    }

    if (p_kpc_get_thread_counters(0, apple_counter_count, apple_counters) != 0) {
        return 0;
    }

    *out = apple_counters[apple_cycle_index];
    return 1;
}

static int apple_cycle_init(void) {
    kpep_event *ev;
    uint32_t err;
    uint32_t classes;
    size_t reg_count;
    uint32_t config_counter_count;
    kpc_config_t regs[KPC_MAX_COUNTERS];
    size_t counter_map[KPC_MAX_COUNTERS];
    uint64_t a;
    uint64_t b;

    if (apple_kpc_initialized) {
        return 1;
    }

    if (!apple_load_frameworks()) {
        return 0;
    }

    if (p_kpc_force_all_ctrs_get(&apple_force_saved) != 0) {
        return 0;
    }
    apple_force_saved_valid = 1;

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
        p_kpep_config_kpc_count(apple_config_obj, &reg_count) != 0) {
        return 0;
    }

    if (classes == 0 || reg_count > KPC_MAX_COUNTERS || counter_map[0] >= KPC_MAX_COUNTERS) {
        return 0;
    }

    if (p_kpep_config_kpc_map(apple_config_obj, counter_map, KPC_MAX_COUNTERS) != 0 ||
        p_kpep_config_kpc(apple_config_obj, regs, KPC_MAX_COUNTERS) != 0) {
        return 0;
    }

    if (counter_map[0] >= KPC_MAX_COUNTERS) {
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
    apple_kpc_classes = classes;

    if (apple_counter_count == 0 ||
        apple_counter_count > KPC_MAX_COUNTERS ||
        apple_cycle_index >= (size_t) apple_counter_count) {
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

    apple_kpc_initialized = 1;
    return 1;
}

static unsigned long long apple_cycle_counter_read(void) {
    uint64_t x;

    if (!apple_cycle_read(&x)) {
        return 0;
    }

    return (unsigned long long) x;
}

static void apple_cycle_close(void) {
    if (p_kpc_set_thread_counting && apple_kpc_classes) {
        p_kpc_set_thread_counting(0);
    }

    if (p_kpc_set_counting && apple_kpc_classes) {
        p_kpc_set_counting(0);
    }

    if (p_kpc_force_all_ctrs_set && apple_force_saved_valid) {
        p_kpc_force_all_ctrs_set(apple_force_saved);
    }

    if (apple_config_obj && p_kpep_config_free) {
        p_kpep_config_free(apple_config_obj);
    }

    if (apple_db && p_kpep_db_free) {
        p_kpep_db_free(apple_db);
    }

    apple_config_obj = 0;
    apple_db = 0;
    apple_counter_count = 0;
    apple_cycle_index = 0;
    apple_kpc_classes = 0;
    apple_kpc_initialized = 0;
    apple_force_saved = 0;
    apple_force_saved_valid = 0;
}

#endif

#if defined(__aarch64__) && !defined(__APPLE__)
/**
 * ARMv8 PMU cycle counter (PMCCNTR_EL0).
 * Reads true CPU cycles.
 * Requires user-space access to PMCCNTR_EL0 if perf_event_open is unavailable.
 */
static int g_pmu_initialized = 0;

static void armv8_init_pmu(void) {
    unsigned long long val = 1;

    __asm__ volatile("MSR PMCR_EL0, %0" :: "r"(val));

    val = 0x80000000ULL;
    __asm__ volatile("MSR PMCNTENSET_EL0, %0" :: "r"(val));
}

static unsigned long long armv8_read_pmccntr(void) {
    unsigned long long val;

    __asm__ volatile("mrs %0, pmccntr_el0" : "=r"(val));

    return val;
}

static void armv8_memory_barrier(void) {
    __asm__ volatile("dsb sy" ::: "memory");
    __asm__ volatile("isb" ::: "memory");
}
#endif

#if defined(__riscv)

static unsigned long long riscv_read_cycle_raw(void) {
#if defined(__riscv_xlen) && (__riscv_xlen == 64)
    unsigned long long x;

    __asm__ volatile("rdcycle %0" : "=r"(x) :: "memory");

    return x;
#else
    uint32_t lo;
    uint32_t hi;
    uint32_t hi2;

    do {
        __asm__ volatile("rdcycleh %0" : "=r"(hi) :: "memory");
        __asm__ volatile("rdcycle %0" : "=r"(lo) :: "memory");
        __asm__ volatile("rdcycleh %0" : "=r"(hi2) :: "memory");
    } while (hi != hi2);

    return ((unsigned long long) hi << 32) | (unsigned long long) lo;
#endif
}

static void riscv_cycle_barrier(void) {
    __asm__ volatile("fence iorw, iorw" ::: "memory");
}

#if defined(__linux__)

static sigjmp_buf riscv_cycle_jmp;
static volatile sig_atomic_t riscv_cycle_testing;

static void riscv_cycle_sigill(int signo) {
    (void) signo;

    if (riscv_cycle_testing) {
        siglongjmp(riscv_cycle_jmp, 1);
    }
}

static int riscv_rdcycle_available(void) {
    struct sigaction sa;
    struct sigaction old_sa;
    unsigned long long a;
    unsigned long long b;
    int ok;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = riscv_cycle_sigill;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGILL, &sa, &old_sa) != 0) {
        return 0;
    }

    ok = 0;
    riscv_cycle_testing = 1;

    if (sigsetjmp(riscv_cycle_jmp, 1) == 0) {
        a = riscv_read_cycle_raw();
        b = riscv_read_cycle_raw();
        (void) a;
        (void) b;
        ok = 1;
    }

    riscv_cycle_testing = 0;
    sigaction(SIGILL, &old_sa, 0);

    return ok;
}

#endif

#endif

void cycle_counter_close(cycle_counter_t *counter) {
    if (!counter) {
        return;
    }

    if (counter->perf_fd >= 0) {
        close(counter->perf_fd);
    }

#if defined(__APPLE__) && (defined(__aarch64__) || defined(__arm64__))
    if (counter->source == CYCLE_SOURCE_APPLE_KPEP) {
        apple_cycle_close();
    }
#endif

    counter->perf_fd = -1;
    counter->source = CYCLE_SOURCE_NONE;
}

int cycle_counter_open(cycle_counter_t *counter, int cycles_enabled) {
    if (!counter) {
        return -1;
    }

    counter->source = CYCLE_SOURCE_NONE;
    counter->perf_fd = -1;

    if (!cycles_enabled) {
        return 0;
    }

#ifdef __linux__
    {
        struct perf_event_attr pe;

        memset(&pe, 0, sizeof(pe));
        pe.type = PERF_TYPE_HARDWARE;
        pe.size = sizeof(pe);
        pe.config = PERF_COUNT_HW_CPU_CYCLES;
        pe.disabled = 1;
        pe.exclude_kernel = 0;
        pe.exclude_hv = 0;

        counter->perf_fd = (int) syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
        if (counter->perf_fd >= 0) {
            counter->source = CYCLE_SOURCE_PERF;
            return 0;
        }
    }
#endif

#if defined(__APPLE__) && (defined(__aarch64__) || defined(__arm64__))
    if (apple_cycle_init()) {
        counter->source = CYCLE_SOURCE_APPLE_KPEP;
        return 0;
    }
#elif defined(__x86_64__)
    counter->source = CYCLE_SOURCE_TSC;
    return 0;
#elif defined(__aarch64__)
    if (!g_pmu_initialized) {
        armv8_init_pmu();
        g_pmu_initialized = 1;
    }

    counter->source = CYCLE_SOURCE_ARMV8_PMU;
    return 0;
#elif defined(__riscv)
#if defined(__linux__)
    if (!riscv_rdcycle_available()) {
        return -1;
    }
#endif

    counter->source = CYCLE_SOURCE_RISCV_CYCLE;
    return 0;
#else
    return -1;
#endif
}

unsigned long long cycle_counter_begin(cycle_counter_t *counter) {
    if (!counter) {
        return 0;
    }

#ifdef __linux__
    if (counter->source == CYCLE_SOURCE_PERF) {
        ioctl(counter->perf_fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(counter->perf_fd, PERF_EVENT_IOC_ENABLE, 0);
        return 0;
    }
#endif

#if defined(__APPLE__) && (defined(__aarch64__) || defined(__arm64__))
    if (counter->source == CYCLE_SOURCE_APPLE_KPEP) {
        return apple_cycle_counter_read();
    }
#endif

#if defined(__x86_64__)
    if (counter->source == CYCLE_SOURCE_TSC) {
        return read_tsc();
    }
#endif

#if defined(__aarch64__) && !defined(__APPLE__)
    if (counter->source == CYCLE_SOURCE_ARMV8_PMU) {
        armv8_memory_barrier();
        return armv8_read_pmccntr();
    }
#endif

#if defined(__riscv)
    if (counter->source == CYCLE_SOURCE_RISCV_CYCLE) {
        riscv_cycle_barrier();
        return riscv_read_cycle_raw();
    }
#endif

    return 0;
}

unsigned long long cycle_counter_end(cycle_counter_t *counter,
                                     unsigned long long start_cycles) {
    unsigned long long cycles = 0;

    if (!counter) {
        return 0;
    }

#ifdef __linux__
    if (counter->source == CYCLE_SOURCE_PERF) {
        ioctl(counter->perf_fd, PERF_EVENT_IOC_DISABLE, 0);

        if (read(counter->perf_fd, &cycles, sizeof(cycles)) != (ssize_t) sizeof(cycles)) {
            return 0;
        }

        return cycles;
    }
#endif

#if defined(__APPLE__) && (defined(__aarch64__) || defined(__arm64__))
    if (counter->source == CYCLE_SOURCE_APPLE_KPEP) {
        unsigned long long end_cycles = apple_cycle_counter_read();

        if (end_cycles < start_cycles) {
            return 0;
        }

        return end_cycles - start_cycles;
    }
#endif

#if defined(__x86_64__)
    if (counter->source == CYCLE_SOURCE_TSC) {
        unsigned long long end_cycles = read_tsc();

        return end_cycles - start_cycles;
    }
#endif

#if defined(__aarch64__) && !defined(__APPLE__)
    if (counter->source == CYCLE_SOURCE_ARMV8_PMU) {
        unsigned long long end_cycles = armv8_read_pmccntr();

        armv8_memory_barrier();

        return end_cycles - start_cycles;
    }
#endif

#if defined(__riscv)
    if (counter->source == CYCLE_SOURCE_RISCV_CYCLE) {
        unsigned long long end_cycles = riscv_read_cycle_raw();

        riscv_cycle_barrier();

        return end_cycles - start_cycles;
    }
#endif

    (void) start_cycles;
    return 0;
}