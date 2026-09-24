/*
 * ARM-portable CPU cycle counter using Linux perf_event.
 *
 * Replaces x86 rdtsc with PERF_COUNT_HW_CPU_CYCLES, which reads
 * the PMU hardware cycle counter — equivalent to rdtsc on x86.
 *
 * The perf event file descriptor is opened once at program startup
 * via __attribute__((constructor)) and closed at exit via
 * __attribute__((destructor)). Upper-layer benchmark code requires
 * no modifications.
 */

#define _POSIX_C_SOURCE 200809L
#include "cpucycles.h"

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>

#ifdef CLOCK_MONOTONIC_RAW
#define BENCH_CLOCK CLOCK_MONOTONIC_RAW
#else
#define BENCH_CLOCK CLOCK_MONOTONIC
#endif

/* ------------------------------------------------------------------ */
/*  perf_event infrastructure                                         */
/* ------------------------------------------------------------------ */

/* File descriptor for the perf event counter */
static int perf_fd = -1;

/*
 * Wrapper for the perf_event_open system call.
 * glibc does not provide a direct wrapper, so we use syscall().
 */
static long perf_event_open(struct perf_event_attr *hw_event,
                            pid_t pid, int cpu, int group_fd,
                            unsigned long flags)
{
    return syscall(__NR_perf_event_open, hw_event, pid, cpu,
                   group_fd, flags);
}

/*
 * Automatically called before main().
 * Opens a perf event counting hardware CPU cycles in user-space only.
 */
__attribute__((constructor))
static void cpucycles_init(void)
{
    struct perf_event_attr pe;

    memset(&pe, 0, sizeof(pe));
    pe.type           = PERF_TYPE_HARDWARE;
    pe.size           = sizeof(pe);
    pe.config         = PERF_COUNT_HW_CPU_CYCLES;
    pe.disabled       = 1;
    pe.exclude_kernel = 1;  /* Only count user-space cycles */
    pe.exclude_hv     = 1;  /* Exclude hypervisor cycles    */

    /* pid=0: current process, cpu=-1: any CPU */
    perf_fd = perf_event_open(&pe, 0, -1, -1, 0);
    if (perf_fd < 0) {
        perror("perf_event_open failed");
        fprintf(stderr,
            "Hint: try  sudo sysctl kernel.perf_event_paranoid=1\n"
            "  or  echo 1 | sudo tee "
            "/proc/sys/kernel/perf_event_paranoid\n");
        exit(EXIT_FAILURE);
    }

    /* Reset counter to zero and start counting */
    ioctl(perf_fd, PERF_EVENT_IOC_RESET,  0);
    ioctl(perf_fd, PERF_EVENT_IOC_ENABLE, 0);
}

/*
 * Automatically called after main() returns or exit() is called.
 * Stops the counter and closes the file descriptor.
 */
__attribute__((destructor))
static void cpucycles_fini(void)
{
    if (perf_fd >= 0) {
        ioctl(perf_fd, PERF_EVENT_IOC_DISABLE, 0);
        close(perf_fd);
        perf_fd = -1;
    }
}

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

/*
 * Read the current CPU cycle count.
 * Equivalent to rdtsc on x86: returns real CPU clock cycles,
 * tracks CPU frequency changes, user-space only.
 */
uint64_t cpucycles(void)
{
    long long count = 0;
    if (read(perf_fd, &count, sizeof(count)) != sizeof(count)) {
        perror("cpucycles: read failed");
        return 0;
    }
    return (uint64_t)count;
}

/*
 * Measure the overhead of a single cpucycles() call.
 * Returns the minimum observed cost over 100000 iterations.
 */
uint64_t cpucycles_overhead(void)
{
    uint64_t overhead = UINT64_MAX;

    for (size_t i = 0; i < 10000; ++i) {
        const uint64_t t0 = cpucycles();
        __asm__ volatile("");
        const uint64_t t1 = cpucycles();
        const uint64_t delta = t1 - t0;
        if (delta < overhead)
            overhead = delta;
    }
    return overhead;
}

/* ------------------------------------------------------------------ */
/*  Frequency calibration (kept from x86 ref for full API compat)     */
/* ------------------------------------------------------------------ */

static double timespec_diff_seconds(const struct timespec *end,
                                    const struct timespec *begin)
{
    return (double)(end->tv_sec - begin->tv_sec)
         + (double)(end->tv_nsec - begin->tv_nsec) * 1e-9;
}

static uint64_t cycles_at_clock_read(struct timespec *ts)
{
    const uint64_t before = cpucycles();
    if (clock_gettime(BENCH_CLOCK, ts) != 0)
        return 0;
    const uint64_t after = cpucycles();
    return before + (after - before) / 2;
}

static int cmp_double(const void *a, const void *b)
{
    const double x = *(const double *)a;
    const double y = *(const double *)b;
    return (x > y) - (x < y);
}

/*
 * cpucycles_per_second() — calibrate the cycle counter frequency.
 *
 * Takes 5 wall-clock samples (~100 ms each), computes
 * cycles / second for each, and returns the median.
 * The result is cached after the first successful call.
 */
double cpucycles_per_second(void)
{
    static double cached_hz = 0.0;
    double samples[5];

    if (cached_hz > 0.0)
        return cached_hz;

    for (size_t i = 0; i < 5; ++i) {
        struct timespec begin, end;
        struct timespec delay = { .tv_sec = 0, .tv_nsec = 100000000L };
        const uint64_t c0 = cycles_at_clock_read(&begin);

        if (c0 == 0)
            return 0.0;

        while (nanosleep(&delay, &delay) != 0) {
            if (errno != EINTR)
                return 0.0;
        }

        const uint64_t c1 = cycles_at_clock_read(&end);
        const double seconds = timespec_diff_seconds(&end, &begin);
        if (c1 <= c0 || seconds <= 0.0)
            return 0.0;

        samples[i] = (double)(c1 - c0) / seconds;
    }

    qsort(samples, 5, sizeof(samples[0]), cmp_double);
    cached_hz = samples[2];
    return cached_hz;
}
