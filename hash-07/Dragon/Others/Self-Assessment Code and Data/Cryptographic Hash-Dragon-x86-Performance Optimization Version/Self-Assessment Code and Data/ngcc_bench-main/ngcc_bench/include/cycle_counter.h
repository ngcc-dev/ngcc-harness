#ifndef CYCLE_COUNTER_H
#define CYCLE_COUNTER_H

#include <stddef.h>

typedef enum {
    CYCLE_SOURCE_NONE = 0,
    CYCLE_SOURCE_PERF,
    CYCLE_SOURCE_TSC,
    CYCLE_SOURCE_ARMV8_PMU
} cycle_source_t;

typedef struct {
    cycle_source_t source;
    int perf_fd;
} cycle_counter_t;

int cycle_counter_open(cycle_counter_t *counter, int cycles_enabled);
void cycle_counter_close(cycle_counter_t *counter);
unsigned long long cycle_counter_begin(cycle_counter_t *counter);
unsigned long long cycle_counter_end(cycle_counter_t *counter, unsigned long long start_cycles);

#endif
