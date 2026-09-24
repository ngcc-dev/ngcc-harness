// // SPDX-License-Identifier: Apache-2.0

// #include <stdlib.h>
// #include <string.h>
// #include <stdio.h>
// #include <inttypes.h>
// // #include <Windows.h>

// #if defined(TARGET_OS_UNIX) &&                                                                     \
//     (defined(TARGET_ARM) || defined(TARGET_ARM64) || defined(TARGET_OTHER))
// #include <time.h>
// #endif
// #if (defined(TARGET_ARM) || defined(TARGET_ARM64) || defined(TARGET_S390X) || defined(TARGET_OTHER))
// #define print_bench_unit printf("nsec\n");
// #else
// #define print_bench_unit printf("cycles\n");
// #endif

// #if (defined(TARGET_ARM) || defined(TARGET_ARM64) || defined(TARGET_S390X))
// #define BENCH_UNITS "nsec"
// #else
// #define BENCH_UNITS "cycles"
// #endif

// static int64_t
// cpucycles(void)
// {
//     long t = 0;
// 	HANDLE hPro = GetCurrentProcess();
// 	FILETIME start_time, end_time, k_time, user_time;
// 	GetProcessTimes(
// 		hPro,&start_time,&end_time,&k_time,&user_time);
// 	t = user_time.dwLowDateTime/10000;
//     return t;
// }

// static int
// cmpfunc(const void *a, const void *b)
// {
//     return (*(uint64_t *)a - *(uint64_t *)b);
// }

// #define BENCH_CODE_1(r)                                                                            \
//     cycles = 0;                                                                                    \
//     for (i = 0; i < (r); ++i) {                                                                    \
//         cycles1 = cpucycles();

// #define BENCH_CODE_2(name, csv)                                                                    \
//     cycles2 = cpucycles();                                                                         \
//     if (i < LIST_SIZE)                                                                             \
//         cycles_list[i] = (cycles2 - cycles1);                                                      \
//     cycles = cycles + (cycles2 - cycles1);                                                         \
//     }                                                                                              \
//     qsort(cycles_list, (runs < LIST_SIZE) ? runs : LIST_SIZE, sizeof(uint64_t), cmpfunc);          \
//     if (csv)                                                                                       \
//         printf("%2" PRId64 ",", cycles_list[(runs < LIST_SIZE) ? runs / 2 : LIST_SIZE / 2]);       \
//     else {                                                                                         \
//         printf("  %-20s-> median: %2" PRId64 ", average: %2" PRId64 " ",                           \
//                name,                                                                               \
//                cycles_list[(runs < LIST_SIZE) ? runs / 2 : LIST_SIZE / 2],                         \
//                (cycles / runs));                                                                   \
//         printf("%s\n", BENCH_UNITS);                                                               \
//     }



// SPDX-License-Identifier: Apache-2.0

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#if defined(_WIN32)
#include <Windows.h>
#elif defined(__i386__) || defined(__x86_64__)
#include <x86intrin.h>
#else
#include <time.h>
#endif
#if (defined(TARGET_ARM) || defined(TARGET_ARM64) || defined(TARGET_S390X) || defined(TARGET_OTHER))
#define print_bench_unit printf("nsec\n");
#else
#define print_bench_unit printf("cycles\n");
#endif

#if (defined(TARGET_ARM) || defined(TARGET_ARM64) || defined(TARGET_S390X))
#define BENCH_UNITS "nsec"
#else
#define BENCH_UNITS "cycles"
#endif

static int64_t
cpucycles(void)
{
#if defined(_WIN32)
    long t = 0;
    HANDLE hPro = GetCurrentProcess();
    FILETIME start_time, end_time, k_time, user_time;
    GetProcessTimes(hPro, &start_time, &end_time, &k_time, &user_time);
    t = user_time.dwLowDateTime / 10000;
    return t;
#elif defined(__i386__) || defined(__x86_64__)
    return (int64_t)__rdtsc();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
#endif
}

static int
cmpfunc(const void *a, const void *b)
{
    return (*(uint64_t *)a - *(uint64_t *)b);
}

#define BENCH_CODE_1(r)                                                                            \
    cycles = 0;                                                                                    \
    for (i = 0; i < (r); ++i) {                                                                    \
        cycles1 = cpucycles();

#define BENCH_CODE_2(name, csv)                                                                    \
    cycles2 = cpucycles();                                                                         \
    if (i < LIST_SIZE)                                                                             \
        cycles_list[i] = (cycles2 - cycles1);                                                      \
    cycles = cycles + (cycles2 - cycles1);                                                         \
    }                                                                                              \
    qsort(cycles_list, (runs < LIST_SIZE) ? runs : LIST_SIZE, sizeof(uint64_t), cmpfunc);          \
    if (csv)                                                                                       \
        printf("%2" PRId64 ",", cycles_list[(runs < LIST_SIZE) ? runs / 2 : LIST_SIZE / 2]);       \
    else {                                                                                         \
        printf("  %-20s-> median: %2" PRId64 ", average: %2" PRId64 " ",                           \
               name,                                                                               \
               cycles_list[(runs < LIST_SIZE) ? runs / 2 : LIST_SIZE / 2],                         \
               (cycles / runs));                                                                   \
        printf("%s\n", BENCH_UNITS);                                                               \
    }
