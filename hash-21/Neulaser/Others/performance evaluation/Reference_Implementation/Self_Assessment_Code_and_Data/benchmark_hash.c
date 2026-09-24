/*
Benchmark harness for the Neulaser reference implementation.

This file is not part of the submitted CryptHash API.  It measures the
CryptHash() entry point on the S1--S8 input sizes recommended by the x86
self-assessment guideline.
*/

#include "CryptHash_AlgorithmInstance.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <x86intrin.h>

static volatile unsigned int bench_sink = 0;

static uint64_t bench_rdtsc(void)
{
    _mm_lfence();
    {
        uint64_t t = __rdtsc();
        _mm_lfence();
        return t;
    }
}

static double bench_seconds(void)
{
    static LARGE_INTEGER freq;
    LARGE_INTEGER now;

    if (freq.QuadPart == 0)
        QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart / (double)freq.QuadPart;
}

static void bench_fill(unsigned char *buf, size_t len)
{
    uint32_t x = 0x6e65756cU ^ (uint32_t)DIGEST_BIT_LENGTH;
    size_t i;

    for (i = 0; i < len; i++)
    {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        buf[i] = (unsigned char)(x >> 24);
    }
}

int main(void)
{
    static const size_t sizes[] = {
        32U, 128U, 512U, 1024U, 4096U, 8192U, 16384U, 65536U
    };
    static const char *levels[] = {
        "S1", "S2", "S3", "S4", "S5", "S6", "S7", "S8"
    };
    const int samples = 100;
    unsigned char *msg = NULL;
    unsigned char digest[DIGEST_BIT_LENGTH / 8];
    size_t max_len = sizes[sizeof(sizes) / sizeof(sizes[0]) - 1U];
    size_t idx;

    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)1);

    msg = (unsigned char *)malloc(max_len);
    if (msg == NULL)
    {
        fprintf(stderr, "memory allocation failed\n");
        return 1;
    }
    bench_fill(msg, max_len);
    memset(digest, 0, sizeof(digest));

    printf("Algorithm,%s\n", ALGORITHM_INSTANCE);
    printf("DigestBits,%d\n", DIGEST_BIT_LENGTH);
    printf("Samples,%d\n", samples);
    printf("Level,MessageBytes,Calls,TotalBytes,AvgCyclesPerHash,CyclesPerByte,AvgTimeUsPerHash,ThroughputMBps\n");

    for (idx = 0; idx < sizeof(sizes) / sizeof(sizes[0]); idx++)
    {
        size_t len = sizes[idx];
        int inner = (int)(65536U / len);
        int s;
        uint64_t total_cycles = 0;
        uint64_t calls = 0;
        double total_seconds = 0.0;
        uint64_t total_bytes;

        if (inner < 1)
            inner = 1;

        for (s = 0; s < 10; s++)
            (void)CryptHash(DIGEST_BIT_LENGTH, msg, (unsigned long long)len * 8ULL, digest);

        for (s = 0; s < samples; s++)
        {
            int j;
            double t0;
            double t1;
            uint64_t c0;
            uint64_t c1;

            c0 = bench_rdtsc();
            t0 = bench_seconds();
            for (j = 0; j < inner; j++)
            {
                if (CryptHash(DIGEST_BIT_LENGTH, msg, (unsigned long long)len * 8ULL, digest) != 0)
                {
                    free(msg);
                    fprintf(stderr, "CryptHash failed\n");
                    return 2;
                }
                bench_sink ^= digest[(unsigned int)j % sizeof(digest)];
            }
            t1 = bench_seconds();
            c1 = bench_rdtsc();

            total_cycles += c1 - c0;
            total_seconds += t1 - t0;
            calls += (uint64_t)inner;
        }

        total_bytes = calls * (uint64_t)len;
        printf("%s,%lu,%llu,%llu,%.2f,%.4f,%.4f,%.2f\n",
               levels[idx],
               (unsigned long)len,
               (unsigned long long)calls,
               (unsigned long long)total_bytes,
               (double)total_cycles / (double)calls,
               (double)total_cycles / (double)total_bytes,
               (total_seconds * 1000000.0) / (double)calls,
               ((double)total_bytes / (1024.0 * 1024.0)) / total_seconds);
    }

    fprintf(stderr, "sink=%u\n", bench_sink);
    free(msg);
    return 0;
}
