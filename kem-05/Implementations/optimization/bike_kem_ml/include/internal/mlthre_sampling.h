/* Runtime ML threshold sampling support.
 * This is a training/debug path only and is not used by the production KEM
 * unless BIKE_MLTHRE_SAMPLING is enabled at compile time.
 */

#pragma once

#include <stdint.h>

#include "types.h"

#if defined(BIKE_MLTHRE_SAMPLING) && (BIKE_MLTHRE_SAMPLING != 0)

int      mlthre_sampling_init_from_env(void);
int      mlthre_sampling_enabled(void);
void     mlthre_sampling_close(void);
void     mlthre_sampling_set_sample(uint32_t sample_id);
uint32_t mlthre_sampling_current_sample(void);
void     mlthre_sampling_capture_error(const pad_e_t *error);
uint32_t mlthre_sampling_residual(const e_t *guess);

void mlthre_sampling_record_candidate(uint32_t sample_id,
                                      uint32_t security_level,
                                      uint32_t iteration,
                                      uint32_t initial_syndrome_weight,
                                      uint32_t prev_syndrome_weight,
                                      uint32_t current_syndrome_weight,
                                      uint32_t syndrome_delta,
                                      uint32_t z0,
                                      uint32_t z1,
                                      uint32_t var_threshold,
                                      uint32_t lower_bound,
                                      uint32_t applied_threshold,
                                      uint32_t candidate_threshold,
                                      int32_t  delta_from_applied,
                                      uint32_t candidate_residual_after,
                                      uint32_t candidate_syndrome_weight_after,
                                      uint32_t candidate_guess_weight_after,
                                      uint32_t best_threshold,
                                      int32_t  best_delta,
                                      uint32_t best_residual_after,
                                      uint32_t best_syndrome_weight_after,
                                      uint32_t best_guess_weight_after,
                                      uint32_t is_best_threshold);

#else

static inline int mlthre_sampling_init_from_env(void)
{
  return 0;
}

static inline int mlthre_sampling_enabled(void)
{
  return 0;
}

static inline void mlthre_sampling_close(void)
{}

static inline void mlthre_sampling_set_sample(uint32_t sample_id)
{
  (void)sample_id;
}

static inline uint32_t mlthre_sampling_current_sample(void)
{
  return 0;
}

static inline void mlthre_sampling_capture_error(const pad_e_t *error)
{
  (void)error;
}

#endif
