#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "cpucycles.h"
#include "speed_print.h"

static int cmp_uint64(const void *a, const void *b)
{
    const uint64_t x = *(const uint64_t *)a;
    const uint64_t y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

static uint64_t median(uint64_t *values, size_t count)
{
    qsort(values, count, sizeof(*values), cmp_uint64);

    if ((count & 1U) != 0)
        return values[count / 2];

    const uint64_t lo = values[count / 2 - 1];
    const uint64_t hi = values[count / 2];
    return lo + (hi - lo) / 2;
}

static long double average(const uint64_t *values, size_t count)
{
    long double sum = 0.0L;
    for (size_t i = 0; i < count; ++i)
        sum += (long double)values[i];
    return sum / (long double)count;
}

void print_results(const char *label, uint64_t *timestamps, size_t timestamp_count)
{
    if (timestamp_count < 2) {
        fprintf(stderr, "ERROR: Need at least two timestamps.\n");
        return;
    }

    const size_t sample_count = timestamp_count - 1;
    for (size_t i = 0; i < sample_count; ++i) {
        if (timestamps[i + 1] <= timestamps[i]) {
            fprintf(stderr, "ERROR: Non-monotonic timestamp at sample %zu.\n", i);
            return;
        }

        timestamps[i] = timestamps[i + 1] - timestamps[i];
    }

    print_cycle_samples(label, timestamps, sample_count);
}

void print_cycle_samples(const char *label, uint64_t *samples, size_t sample_count)
{
    static uint64_t overhead = UINT64_MAX;
    size_t invalid_samples = 0;

    if (sample_count == 0) {
        fprintf(stderr, "ERROR: Need at least one sample.\n");
        return;
    }

    if (overhead == UINT64_MAX)
        overhead = cpucycles_overhead();

    for (size_t i = 0; i < sample_count; ++i) {
        if (samples[i] <= overhead) {
            samples[i] = 0;
            ++invalid_samples;
        } else {
            samples[i] -= overhead;
        }
    }

    const long double avg_ticks = average(samples, sample_count);
    const uint64_t median_ticks = median(samples, sample_count);
    const double tsc_hz = cpucycles_per_second();

    printf("%s\n", label);
    printf("median: %llu cycles\n", (unsigned long long)median_ticks);
    printf("average: %.2Lf cycles\n", avg_ticks);

    if (tsc_hz > 0.0) {
        const double ms_per_operation = (double)(avg_ticks * 1000.0L / tsc_hz);
        printf("ms per operation: %.9f\n", ms_per_operation);
        if (ms_per_operation > 0.0)
            printf("operations per second: %.3f\n", 1000.0 / ms_per_operation);
    } else {
        fprintf(stderr, "WARNING: Could not calibrate TSC frequency; time results omitted.\n");
    }

    if (invalid_samples != 0) {
        fprintf(stderr,
                "WARNING: %zu samples were not larger than timer overhead; "
                "benchmark more operations per sample.\n",
                invalid_samples);
    }

    putchar('\n');
}
