/* Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0"
 */

#include "mlthre_sampling.h"

#if defined(BIKE_MLTHRE_SAMPLING) && (BIKE_MLTHRE_SAMPLING != 0)

#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>

#  include "utilities.h"

typedef struct mlthre_sampling_ctx_s {
  int      enabled;
  FILE    *candidates_fp;
  uint32_t sample_id;
  e_t      true_error;
  int      have_true_error;
} mlthre_sampling_ctx_t;

static mlthre_sampling_ctx_t g_sampling = {0};

int mlthre_sampling_init_from_env(void)
{
  const char *path = getenv("BIKE_MLTHRE_CANDIDATES_CSV");
  if((path == NULL) || (path[0] == '\0')) {
    return 0;
  }

  g_sampling.candidates_fp = fopen(path, "w");
  if(g_sampling.candidates_fp == NULL) {
    return -1;
  }

  g_sampling.enabled = 1;
  fprintf(g_sampling.candidates_fp,
          "sample_id,security_level,iteration,initial_syndrome_weight,"
          "prev_syndrome_weight,current_syndrome_weight,syndrome_delta,z0,z1,"
          "var_threshold,lower_bound,applied_threshold,candidate_threshold,"
          "delta_from_applied,candidate_residual_after,"
          "candidate_syndrome_weight_after,candidate_guess_weight_after,"
          "best_threshold,best_delta,best_residual_after,"
          "best_syndrome_weight_after,best_guess_weight_after,is_best_threshold\n");
  printf("ML threshold sampling enabled: %s\n", path);
  return 0;
}

int mlthre_sampling_enabled(void)
{
  return g_sampling.enabled;
}

void mlthre_sampling_close(void)
{
  if(g_sampling.candidates_fp != NULL) {
    fclose(g_sampling.candidates_fp);
  }
  memset(&g_sampling, 0, sizeof(g_sampling));
}

void mlthre_sampling_set_sample(uint32_t sample_id)
{
  if(!g_sampling.enabled) {
    return;
  }
  g_sampling.sample_id        = sample_id;
  g_sampling.have_true_error  = 0;
}

uint32_t mlthre_sampling_current_sample(void)
{
  return g_sampling.sample_id;
}

void mlthre_sampling_capture_error(const pad_e_t *error)
{
  if(!g_sampling.enabled) {
    return;
  }

  g_sampling.true_error.val[0] = error->val[0].val;
  g_sampling.true_error.val[1] = error->val[1].val;
  g_sampling.have_true_error   = 1;
}

uint32_t mlthre_sampling_residual(const e_t *guess)
{
  uint32_t residual = 0;

  if(!g_sampling.have_true_error) {
    return 0;
  }

  for(size_t i = 0; i < N0; i++) {
    for(size_t j = 0; j < (R_BYTES - 1); j++) {
      residual += __builtin_popcount(
        guess->val[i].raw[j] ^ g_sampling.true_error.val[i].raw[j]);
    }

    const uint8_t diff =
      (guess->val[i].raw[R_BYTES - 1] ^
       g_sampling.true_error.val[i].raw[R_BYTES - 1]) &
      LAST_R_BYTE_MASK;
    residual += __builtin_popcount(diff);
  }

  return residual;
}

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
                                      uint32_t is_best_threshold)
{
  if(!g_sampling.enabled) {
    return;
  }

  fprintf(g_sampling.candidates_fp,
          "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%u,%d,%u,%u,%u,%u\n",
          sample_id,
          security_level,
          iteration,
          initial_syndrome_weight,
          prev_syndrome_weight,
          current_syndrome_weight,
          syndrome_delta,
          z0,
          z1,
          var_threshold,
          lower_bound,
          applied_threshold,
          candidate_threshold,
          delta_from_applied,
          candidate_residual_after,
          candidate_syndrome_weight_after,
          candidate_guess_weight_after,
          best_threshold,
          best_delta,
          best_residual_after,
          best_syndrome_weight_after,
          best_guess_weight_after,
          is_best_threshold);
}

#endif
