#ifndef CPUTIMER_H
#define CPUTIMER_H

#include <stdio.h>

#if defined(__linux__)
#include <unistd.h>
#elif defined(_WIN32)
#include <intrin.h>
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static void sleepms(int time) {
#if defined(__linux__)
    usleep(time * 1000);
#elif defined(_WIN32)
    Sleep(time);
#endif
}

static void print_bytes(uint8_t* arr, int len) {
    printf("[ %02x", arr[0]);
    for (int i = 1; i < len; i++)
        printf(", %02x", arr[i]);
    printf(" ]\n");
}

inline unsigned long long cputimer() {
#if _WIN32
    return __rdtsc();
#else
    unsigned int lo, hi;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((unsigned long long)hi << 32) | lo;
#endif
}

static unsigned long long CPUFrequency;

static unsigned long long GetFrequency() {
    unsigned long long t1 = cputimer();
    sleepms(1000);
    unsigned long long t2 = cputimer();
    CPUFrequency = t2 - t1;
    printf("CPU Frequency = %lld\n", CPUFrequency);
    return CPUFrequency;
}

static void __quick_sort(unsigned long long* arr, int begin, int end) {
    if (begin >= end)
        return;

    unsigned long long temp1 = arr[begin], temp2;
    int k = begin;
    for (int i = begin + 1; i <= end; i++) {
        if (temp1 > arr[i]) {
            temp2 = arr[i];
            int j;
            for (j = i - 1; j >= k; j--)
                arr[j + 1] = arr[j];
            arr[j + 1] = temp2;
            k++;
        }
    }
    __quick_sort(arr, begin, k - 1);
    __quick_sort(arr, k + 1, end);
}

inline void quick_sort(unsigned long long* arr, int size) {
    __quick_sort(arr, 0, size - 1);
}

static void Analyis(unsigned long long* timestamp_mem, int loop) {
    unsigned long long min, max, med, aver = 0;
    quick_sort(timestamp_mem, loop);
    min = timestamp_mem[0];
    max = timestamp_mem[loop - 1];
    med = timestamp_mem[loop >> 1];
    for (int i = 0; i < loop; i++) {
        aver += timestamp_mem[i];
    }
    aver /= loop;

    printf("\tMinimum\t%10lld cycles (%10.6f ms)\n", min, (double)min / CPUFrequency * 1000);
    printf("\tMaximum\t%10lld cycles (%10.6f ms)\n", max, (double)max / CPUFrequency * 1000);
    printf("\tMedian\t%10lld cycles (%10.6f ms)\n", med, (double)med / CPUFrequency * 1000);
    printf("\tAverage\t%10lld cycles (%10.6f ms)\n", aver, (double)aver / CPUFrequency * 1000);
}

#define Timer(code)                                                           \
    do {                                                                      \
        unsigned long long timestamp_start = cputimer();                      \
        code;                                                                 \
        unsigned long long timestamp_end = cputimer();                        \
        printf("time = %ld cycles (%f s)\n", timestamp_end - timestamp_start, \
               (double)(timestamp_end - timestamp_start) / CPUFrequency);     \
    } while (0)

#define Loop(loop, code, fmt, ...)                              \
    do {                                                        \
        unsigned long long timestamp_mem[loop];                 \
        for (int i = 0; i < loop; i++) {                        \
            unsigned long long timestamp_start = cputimer();    \
            code;                                               \
            unsigned long long timestamp_end = cputimer();      \
            timestamp_mem[i] = timestamp_end - timestamp_start; \
        }                                                       \
        printf("Time of \"" fmt "\" is \n", ##__VA_ARGS__);     \
        Analyis(timestamp_mem, loop);                           \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif  // CPUTIMER_H