#ifndef STATS_UTIL_H
#define STATS_UTIL_H

#include <stddef.h>
#include <time.h>

typedef struct {
    unsigned long long count;
    double mean;
    double m2;
    double min;
    double max;
} running_stats_t;

void stats_init(running_stats_t *stats);
void stats_update(running_stats_t *stats, double value);
double stats_stddev(const running_stats_t *stats);

int ngcc_monotonic_clock_gettime(struct timespec *ts);
double timespec_ms_diff(const struct timespec *start, const struct timespec *end);

#endif
