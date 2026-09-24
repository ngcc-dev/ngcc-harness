#ifndef __MLTHRE_POLICY_H_INCLUDED__
#define __MLTHRE_POLICY_H_INCLUDED__

#include "types.h"

#define MLTHRE_NUM_BINS 5

typedef struct mlthre_state_s
{
    uint32_t iteration;
    uint32_t model_security_level; // Legacy model feature: 1, 3, 5, or 7.
    uint32_t initial_syndrome_weight;
    uint32_t syndrome_weight;
    uint32_t prev_syndrome_weight;
    uint32_t syndrome_delta;
    uint32_t z0;
    uint32_t z1;
    uint32_t black_count;
    uint32_t gray_count;
    uint32_t histogram[MLTHRE_NUM_BINS];
    int32_t prev_delta;
} mlthre_state_t;

uint32_t mlthre_compute_base_threshold(IN const uint32_t model_security_level,
        IN const uint32_t current_weight,
        IN const uint32_t initial_weight,
        IN const uint32_t iteration);

int32_t mlthre_select_delta(IN const mlthre_state_t *state,
        IN const uint32_t base_threshold);

#endif
