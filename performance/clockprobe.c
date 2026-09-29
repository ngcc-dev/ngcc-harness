/* Measured core clock of the CPU this process runs on (pin it with taskset).
 *
 * Counts user-mode core cycles (perf_event_open, as ngcc_perf.c does) over
 * busy windows of wall time (CLOCK_MONOTONIC_RAW) and prints the median, lowest
 * and highest cycles per second. campaign.py uses it where cpufreq does not
 * expose a fixed frequency (e.g. an AArch64 host whose clock is set by firmware):
 * the median is the reference clock of the run and the spread shows whether the
 * clock is stable. A window that lost the CPU to another task reads low, hence
 * the median.
 *
 *   clockprobe [WINDOWS [WINDOW_MS]]      default 21 windows of 50 ms
 *
 * Output: "clock_hz\t<median>", "clock_min_hz\t<min>", "clock_max_hz\t<max>",
 * "clock_iqr_hz\t<interquartile range>", "windows\t<n>"; exit status 2 if the
 * cycle counter is unavailable.
 */
#define _GNU_SOURCE
#include <linux/perf_event.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

static uint64_t now_ns(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return (uint64_t)t.tv_sec * UINT64_C(1000000000) + (uint64_t)t.tv_nsec;
}

static int cmp(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

int main(int argc, char **argv)
{
    int windows = argc > 1 ? atoi(argv[1]) : 21;
    long window_ms = argc > 2 ? atol(argv[2]) : 50;
    if (windows < 1 || windows > 10000 || window_ms < 1) {
        fprintf(stderr, "usage: clockprobe [WINDOWS [WINDOW_MS]]\n");
        return 1;
    }
    struct perf_event_attr attr;
    memset(&attr, 0, sizeof attr);
    attr.type = PERF_TYPE_HARDWARE;
    attr.size = sizeof attr;
    attr.config = PERF_COUNT_HW_CPU_CYCLES;
    attr.disabled = 1;
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    int fd = (int)syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);
    if (fd < 0) {
        perror("clockprobe: perf_event_open");
        return 2;
    }
    double *hz = calloc((size_t)windows, sizeof *hz);
    if (!hz) return 1;
    volatile uint64_t sink = 0;
    /* one untimed window lets the core leave any idle state */
    for (int w = -1; w < windows; w++) {
        uint64_t cycles = 0, t0, t1;
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
        t0 = now_ns();
        do {
            for (int i = 0; i < 10000; i++) sink += (uint64_t)i;
            t1 = now_ns();
        } while (t1 - t0 < (uint64_t)window_ms * 1000000u);
        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        if (read(fd, &cycles, sizeof cycles) != sizeof cycles || cycles == 0) {
            fprintf(stderr, "clockprobe: no cycle count\n");
            return 2;
        }
        if (w >= 0) hz[w] = (double)cycles / ((double)(t1 - t0) / 1e9);
    }
    qsort(hz, (size_t)windows, sizeof *hz, cmp);
    printf("clock_hz\t%.0f\nclock_min_hz\t%.0f\nclock_max_hz\t%.0f\nclock_iqr_hz\t%.0f\nwindows\t%d\n",
           hz[windows / 2], hz[0], hz[windows - 1], hz[3 * windows / 4] - hz[windows / 4], windows);
    free(hz);
    close(fd);
    return 0;
}
