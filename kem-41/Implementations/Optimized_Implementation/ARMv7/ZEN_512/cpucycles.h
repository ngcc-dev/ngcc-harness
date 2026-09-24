#ifndef CPUCYCLES_H
#define CPUCYCLES_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

// 使用 clock_gettime 作为后备方案
static inline uint64_t cpucycles_fallback(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// 初始化 CPU 周期计数器（在 main 函数开始时调用）
void cpucycles_init(void);

// 获取 CPU 周期数（优先使用 perf_event，失败则使用 clock_gettime）
uint64_t cpucycles(void);

// 清理资源
void cpucycles_cleanup(void);

uint64_t cpucycles_overhead(void);
void print_results(const char *s, uint64_t *t, size_t tlen);

#endif /* CPUCYCLES_H */