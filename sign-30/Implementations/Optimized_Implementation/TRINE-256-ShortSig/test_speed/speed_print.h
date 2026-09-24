#ifndef PRINT_SPEED_H
#define PRINT_SPEED_H

#include <stddef.h>
#include <stdint.h>

void print_results(const char *s, uint64_t *t, size_t tlen);
void print_cycle_samples(const char *label, uint64_t *samples, size_t sample_count);

#endif
