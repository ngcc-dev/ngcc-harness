#include "cycle_counter.h"

#ifdef __linux__
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#endif

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

#if defined(__aarch64__)
/**
 * ARMv8 PMU cycle counter (PMCCNTR_EL0)
 * Reads true CPU cycles. Requires kernel module for user-space access.
 * See: https://github.com/mupq/pqax#enable-access-to-performance-counters
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

void cycle_counter_close(cycle_counter_t *counter) {
    if (counter->perf_fd >= 0) {
        close(counter->perf_fd);
    }
    counter->perf_fd = -1;
    counter->source = CYCLE_SOURCE_NONE;
}

int cycle_counter_open(cycle_counter_t *counter, int cycles_enabled) {
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

#if defined(__x86_64__)
    counter->source = CYCLE_SOURCE_TSC;
    return 0;
#elif defined(__aarch64__)
    if (!g_pmu_initialized) {
        armv8_init_pmu();
        g_pmu_initialized = 1;
    }
    counter->source = CYCLE_SOURCE_ARMV8_PMU;
    return 0;
#else
    return -1;
#endif
}

unsigned long long cycle_counter_begin(cycle_counter_t *counter) {
#ifdef __linux__
    if (counter->source == CYCLE_SOURCE_PERF) {
        ioctl(counter->perf_fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(counter->perf_fd, PERF_EVENT_IOC_ENABLE, 0);
        return 0;
    }
#endif

#if defined(__x86_64__)
    if (counter->source == CYCLE_SOURCE_TSC) {
        return read_tsc();
    }
#endif

#if defined(__aarch64__)
    if (counter->source == CYCLE_SOURCE_ARMV8_PMU) {
        armv8_memory_barrier();
        return armv8_read_pmccntr();
    }
#endif

    return 0;
}

unsigned long long cycle_counter_end(cycle_counter_t *counter, unsigned long long start_cycles) {
    unsigned long long cycles = 0;

#ifdef __linux__
    if (counter->source == CYCLE_SOURCE_PERF) {
        ioctl(counter->perf_fd, PERF_EVENT_IOC_DISABLE, 0);
        if (read(counter->perf_fd, &cycles, sizeof(cycles)) != (ssize_t) sizeof(cycles)) {
            return 0;
        }
        return cycles;
    }
#endif

#if defined(__x86_64__)
    if (counter->source == CYCLE_SOURCE_TSC) {
        unsigned long long end_cycles = read_tsc();
        return end_cycles - start_cycles;
    }
#endif

#if defined(__aarch64__)
    if (counter->source == CYCLE_SOURCE_ARMV8_PMU) {
        unsigned long long end_cycles = armv8_read_pmccntr();
        armv8_memory_barrier();
        return end_cycles - start_cycles;
    }
#endif

    (void) start_cycles;
    return 0;
}
