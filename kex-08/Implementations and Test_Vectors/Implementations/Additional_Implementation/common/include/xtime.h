// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-License-Identifier: Apache-2.0

#ifndef XTIME_H_
#define XTIME_H_

#include <stdint.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
#  include <windows.h>
#endif

static inline void xtime_monotonic(struct timespec *ts)
{
#if defined(_WIN32) || defined(_WIN64)
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    uint64_t ns = (uint64_t)(1e9 * cnt.QuadPart / freq.QuadPart);
    ts->tv_sec  = (time_t)(ns / 1000000000ULL);
    ts->tv_nsec = (long)(ns % 1000000000ULL);

#elif defined(CLOCK_MONOTONIC_RAW)
    clock_gettime(CLOCK_MONOTONIC_RAW, ts);

#elif defined(__MACH__) && defined(__APPLE__)
    clock_gettime(CLOCK_MONOTONIC, ts);

#else
    clock_gettime(CLOCK_MONOTONIC, ts);
#endif
}

#endif /* XTIME_H_ */