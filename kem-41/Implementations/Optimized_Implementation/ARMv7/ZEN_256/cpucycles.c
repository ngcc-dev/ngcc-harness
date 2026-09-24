#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <errno.h>
#include <time.h>
#include "cpucycles.h"

static int perf_fd = -1;
static int use_fallback = 0;

// perf_event_open 系统调用封装
static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                           int cpu, int group_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

void cpucycles_init(void)
{
    struct perf_event_attr attr;
    memset(&attr, 0, sizeof(attr));
    
    attr.type = PERF_TYPE_HARDWARE;
    attr.size = sizeof(attr);
    attr.config = PERF_COUNT_HW_CPU_CYCLES;
    attr.disabled = 1;
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    attr.exclude_idle = 1;
    
    // 尝试打开性能事件
    perf_fd = perf_event_open(&attr, 0, -1, -1, 0);
    
    if (perf_fd == -1) {
        // 如果失败，尝试不排除内核态
        attr.exclude_kernel = 0;
        perf_fd = perf_event_open(&attr, 0, -1, -1, 0);
    }
    
    if (perf_fd == -1) {
        // 再次失败，使用软件事件
        attr.type = PERF_TYPE_SOFTWARE;
        attr.config = PERF_COUNT_SW_CPU_CLOCK;
        attr.exclude_kernel = 0;
        perf_fd = perf_event_open(&attr, 0, -1, -1, 0);
    }
    
    if (perf_fd == -1) {
        fprintf(stderr, "Warning: perf_event_open failed: %s\n", strerror(errno));
        fprintf(stderr, "Using clock_gettime fallback (nanosecond precision)\n");
        fprintf(stderr, "Try running with: sudo ./SPEED_KEM for cycle precision\n");
        use_fallback = 1;
        return;
    }
    
    // 启用计数器
    if (ioctl(perf_fd, PERF_EVENT_IOC_ENABLE, 0) == -1) {
        fprintf(stderr, "Warning: Failed to enable perf event: %s\n", strerror(errno));
        use_fallback = 1;
    } else {
        printf("Performance counter initialized successfully (CPU cycles)\n");
    }
}

uint64_t cpucycles(void)
{
    if (use_fallback) {
        return cpucycles_fallback();
    }
    
    uint64_t count = 0;
    if (perf_fd >= 0) {
        if (read(perf_fd, &count, sizeof(count)) != sizeof(count)) {
            return cpucycles_fallback();
        }
        return count;
    }
    
    return cpucycles_fallback();
}

void cpucycles_cleanup(void)
{
    if (perf_fd >= 0) {
        ioctl(perf_fd, PERF_EVENT_IOC_DISABLE, 0);
        close(perf_fd);
        perf_fd = -1;
    }
}

uint64_t cpucycles_overhead(void)
{
    uint64_t t0, t1, overhead = UINT64_MAX;
    unsigned int i;

    for(i=0; i<1000; i++) 
    {
        t0 = cpucycles();
        t1 = cpucycles();
        if(t1 - t0 < overhead)
            overhead = t1 - t0;
    }

    return overhead;
}

static uint64_t average(uint64_t *t, size_t tlen) 
{
    size_t i;
    uint64_t acc=0;

    for(i=0;i<tlen;i++)
        acc += t[i];

    return acc/tlen;
}

void print_results(const char *s, uint64_t *t, size_t tlen) 
{
    size_t i;
    static uint64_t overhead = UINT64_MAX;

    if(tlen < 2) 
    {
        fprintf(stderr, "ERROR: Need a least two cycle counts!\n");
        return;
    }

    if(overhead == UINT64_MAX)
        overhead = cpucycles_overhead();

    tlen--;
    for(i=0;i<tlen;++i)
        t[i] = t[i+1] - t[i] - overhead;

    printf("%s\n", s);
    if (use_fallback) {
        printf("average: %llu ns\n", (unsigned long long)average(t, tlen));
    } else {
        printf("average: %llu cycles\n", (unsigned long long)average(t, tlen));
    }
    printf("\n");
}