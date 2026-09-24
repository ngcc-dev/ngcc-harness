#ifndef COUNTER_H
#define COUNTER_H

unsigned long long countergap;

#ifndef TEST_LOOP_COUNT
#define TEST_LOOP_COUNT 100000
#endif

#if defined(__aarch64__)
#include <stdint.h>

static inline uint64_t counter(void)
{
    uint64_t t;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(t));
    return t;
}

#elif defined(__x86_64__) || defined(__i386__)
#include <stdint.h>

static inline uint64_t counter(void)
{
    uint64_t result;
    __asm__ volatile ("rdtsc; shlq $32,%%rdx; orq %%rdx,%%rax"
                      : "=a" (result)
                      :
                      : "%rdx");
    return result;
}

#else
#error "counter unsupported on this architecture"
#endif

static inline void setup_counter(void)
{
    countergap = 0;
    for (int i = 0; i < TEST_LOOP_COUNT; i++)
    {
      unsigned long long count1;
      unsigned long long count2;

      count1 = counter();
      count2 = counter();
      countergap += count2 - count1;
    }
    countergap = countergap / TEST_LOOP_COUNT;
}

#endif
