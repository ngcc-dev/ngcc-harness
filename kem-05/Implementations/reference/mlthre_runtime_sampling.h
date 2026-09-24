/******************************************************************************
 * BIKE_MLThre runtime dataset sampling
 ******************************************************************************/

#ifndef __MLTHRE_RUNTIME_SAMPLING_H_INCLUDED__
#define __MLTHRE_RUNTIME_SAMPLING_H_INCLUDED__

#include "mlthre_policy.h"

int mlthre_runtime_sampling_init(const char *output_prefix,
        uint32_t total_samples,
        uint32_t progress_every);
int mlthre_runtime_sampling_is_enabled(void);
void mlthre_runtime_sampling_close(void);

void mlthre_runtime_sampling_set_sample(uint32_t code_test_idx,
        uint32_t enc_test_idx,
        uint32_t sample_idx);
void mlthre_runtime_sampling_capture_enc_error(const uint8_t *e_bytes);

void mlthre_runtime_sampling_record_iteration(const mlthre_state_t *state,
        uint32_t base_threshold,
        int32_t applied_delta,
        uint32_t applied_threshold,
        const uint8_t *guess_bits,
        const uint8_t *syndrome_bits);

void mlthre_runtime_sampling_record_summary(int decoder_rc,
        const uint8_t *guess_bits,
        const uint8_t *syndrome_bits);

#endif
