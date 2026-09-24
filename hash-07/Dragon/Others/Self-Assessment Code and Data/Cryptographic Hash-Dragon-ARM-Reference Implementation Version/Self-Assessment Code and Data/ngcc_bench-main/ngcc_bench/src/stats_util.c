#include "stats_util.h"

#include <math.h>
#include <string.h>
#include <time.h>

int ngcc_monotonic_clock_gettime(struct timespec *ts) {
    if (ts == NULL) {
        return -1;
    }

#if defined(CLOCK_MONOTONIC_RAW)
    if (clock_gettime(CLOCK_MONOTONIC_RAW, ts) == 0) {
        return 0;
    }
#endif

#if defined(CLOCK_MONOTONIC)
    return clock_gettime(CLOCK_MONOTONIC, ts);
#else
    return -1;
#endif
}

double timespec_ms_diff(const struct timespec *start, const struct timespec *end) {
    long sec = end->tv_sec - start->tv_sec;
    long nsec = end->tv_nsec - start->tv_nsec;
    return (double) sec * 1000.0 + (double) nsec / 1000000.0;
}

void stats_init(running_stats_t *stats) {
    memset(stats, 0, sizeof(*stats));
}

void stats_update(running_stats_t *stats, double value) {
    if (stats->count == 0) {
        stats->count = 1;
        stats->mean = value;
        stats->m2 = 0.0;
        stats->min = value;
        stats->max = value;
        return;
    }

    stats->count++;
    {
        double delta = value - stats->mean;
        stats->mean += delta / (double) stats->count;
        {
            double delta2 = value - stats->mean;
            stats->m2 += delta * delta2;
        }
    }

    if (value < stats->min) {
        stats->min = value;
    }
    if (value > stats->max) {
        stats->max = value;
    }
}

double stats_stddev(const running_stats_t *stats) {
    if (stats->count < 2) {
        return 0.0;
    }
    return sqrt(stats->m2 / (double) (stats->count - 1));
}
