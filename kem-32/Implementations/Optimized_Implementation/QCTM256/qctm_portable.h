#ifndef QCTM_PORTABLE_H
#define QCTM_PORTABLE_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(__GNUC__) || defined(__clang__)
#define QCTM_UNUSED __attribute__((unused))
#else
#define QCTM_UNUSED
#endif

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

typedef struct {
    time_t tv_sec;
    long tv_nsec;
} qctm_timespec;

static QCTM_UNUSED int qctm_now_monotonic(qctm_timespec *ts)
{
    static LARGE_INTEGER frequency;
    static int frequency_ready = 0;
    LARGE_INTEGER counter;
    LONGLONG seconds;
    LONGLONG ticks;

    if (ts == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (!frequency_ready) {
        if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0) {
            errno = EINVAL;
            return -1;
        }
        frequency_ready = 1;
    }
    if (!QueryPerformanceCounter(&counter)) {
        errno = EINVAL;
        return -1;
    }

    seconds = counter.QuadPart / frequency.QuadPart;
    ticks = counter.QuadPart % frequency.QuadPart;
    ts->tv_sec = (time_t)seconds;
    ts->tv_nsec = (long)((ticks * 1000000000LL) / frequency.QuadPart);
    return 0;
}

static QCTM_UNUSED int qctm_localtime(const time_t *now, struct tm *out)
{
    struct tm *tmp;

    if (now == NULL || out == NULL) {
        return -1;
    }
    tmp = localtime(now);
    if (tmp == NULL) {
        return -1;
    }
    *out = *tmp;
    return 0;
}

static QCTM_UNUSED void qctm_platform_string(char *out, size_t out_len)
{
    SYSTEM_INFO info;
    const char *arch = "unknown";

    if (out == NULL || out_len == 0) {
        return;
    }

    GetNativeSystemInfo(&info);
    switch (info.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
        arch = "x86_64";
        break;
    case PROCESSOR_ARCHITECTURE_ARM64:
        arch = "arm64";
        break;
    case PROCESSOR_ARCHITECTURE_INTEL:
        arch = "x86";
        break;
    default:
        break;
    }
    snprintf(out, out_len, "Windows %s", arch);
}
#else
#include <sys/utsname.h>

typedef struct timespec qctm_timespec;

static QCTM_UNUSED int qctm_now_monotonic(qctm_timespec *ts)
{
    return clock_gettime(CLOCK_MONOTONIC, ts);
}

static QCTM_UNUSED int qctm_localtime(const time_t *now, struct tm *out)
{
    return localtime_r(now, out) == NULL ? -1 : 0;
}

static QCTM_UNUSED void qctm_platform_string(char *out, size_t out_len)
{
    struct utsname uts;

    if (out == NULL || out_len == 0) {
        return;
    }
    if (uname(&uts) != 0) {
        snprintf(out, out_len, "unknown unknown unknown");
        return;
    }
    snprintf(out, out_len, "%s %s %s", uts.sysname, uts.release, uts.machine);
}
#endif

static QCTM_UNUSED double qctm_elapsed_ns(const qctm_timespec *start,
                                          const qctm_timespec *end)
{
    time_t sec = end->tv_sec - start->tv_sec;
    long nsec = end->tv_nsec - start->tv_nsec;

    if (nsec < 0) {
        sec--;
        nsec += 1000000000L;
    }
    return (double)sec * 1000000000.0 + (double)nsec;
}

static QCTM_UNUSED double qctm_elapsed_ms(const qctm_timespec *start,
                                          const qctm_timespec *end)
{
    return qctm_elapsed_ns(start, end) / 1000000.0;
}

static QCTM_UNUSED double qctm_now_ms(void)
{
    qctm_timespec ts;

    if (qctm_now_monotonic(&ts) != 0) {
        return 0.0;
    }
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static QCTM_UNUSED long qctm_getline(char **lineptr, size_t *n, FILE *stream)
{
    size_t len = 0;
    int ch;

    if (lineptr == NULL || n == NULL || stream == NULL) {
        return -1;
    }
    if (*lineptr == NULL || *n == 0) {
        *n = 256;
        *lineptr = (char *)malloc(*n);
        if (*lineptr == NULL) {
            *n = 0;
            return -1;
        }
    }

    while ((ch = fgetc(stream)) != EOF) {
        if (len + 1 >= *n) {
            size_t new_cap = (*n) * 2;
            char *new_line = (char *)realloc(*lineptr, new_cap);

            if (new_line == NULL) {
                return -1;
            }
            *lineptr = new_line;
            *n = new_cap;
        }
        (*lineptr)[len++] = (char)ch;
        if (ch == '\n') {
            break;
        }
    }

    if (len == 0 && ch == EOF) {
        return -1;
    }
    (*lineptr)[len] = '\0';
    return (long)len;
}

#endif
