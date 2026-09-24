/******************************************************************************
 * BIKE_MLThre runtime dataset sampling
 ******************************************************************************/

#include "mlthre_runtime_sampling.h"

#include <stdio.h>
#include <string.h>

#include "conversions.h"
#include "defs.h"

typedef struct mlthre_runtime_sampling_ctx_s
{
    int enabled;
    FILE *iteration_fp;
    FILE *summary_fp;
    uint32_t total_samples;
    uint32_t progress_every;
    uint32_t sample_idx;
    uint32_t code_test_idx;
    uint32_t enc_test_idx;
    uint8_t true_error_bits[N_BITS];
    uint32_t true_error_weight;
    int have_true_error;
} mlthre_runtime_sampling_ctx_t;

static mlthre_runtime_sampling_ctx_t g_sampling = {0};

static uint32_t bit_weight(const uint8_t *bits, uint32_t length)
{
    uint32_t weight = 0;

    for (uint32_t i = 0; i < length; i++)
    {
        weight += bits[i] ? 1U : 0U;
    }

    return weight;
}

static uint32_t residual_diffs(const uint8_t *guess_bits)
{
    uint32_t diffs = 0;

    if (!g_sampling.have_true_error)
    {
        return 0;
    }

    for (uint32_t i = 0; i < N_BITS; i++)
    {
        if (guess_bits[i] != g_sampling.true_error_bits[i])
        {
            diffs++;
        }
    }

    return diffs;
}

static void write_headers(void)
{
    fprintf(g_sampling.iteration_fp,
            "sample_id,code_test,enc_test,model_security_level,iteration,"
            "base_threshold,applied_threshold,applied_delta,prev_delta,"
            "syndrome_weight,prev_syndrome_weight,syndrome_delta,z0,z1,"
            "black_count,gray_count,hist_m2,hist_m1,hist_0,hist_p1,hist_p2,"
            "true_error_weight,guess_weight,residual_diffs,"
            "syndrome_weight_after_iter\n");

    fprintf(g_sampling.summary_fp,
            "sample_id,code_test,enc_test,security_bits,true_error_weight,"
            "final_guess_weight,final_residual_diffs,final_syndrome_weight,"
            "decoder_rc\n");
}

int mlthre_runtime_sampling_init(const char *output_prefix,
        uint32_t total_samples,
        uint32_t progress_every)
{
    char iteration_path[1024] = {0};
    char summary_path[1024] = {0};

    if (output_prefix == NULL || output_prefix[0] == '\0')
    {
        return 0;
    }

    snprintf(iteration_path, sizeof(iteration_path), "%s_iterations.csv",
            output_prefix);
    snprintf(summary_path, sizeof(summary_path), "%s_samples.csv",
            output_prefix);

    g_sampling.iteration_fp = fopen(iteration_path, "w");
    g_sampling.summary_fp = fopen(summary_path, "w");
    if (g_sampling.iteration_fp == NULL || g_sampling.summary_fp == NULL)
    {
        if (g_sampling.iteration_fp != NULL)
        {
            fclose(g_sampling.iteration_fp);
        }
        if (g_sampling.summary_fp != NULL)
        {
            fclose(g_sampling.summary_fp);
        }
        memset(&g_sampling, 0, sizeof(g_sampling));
        return -1;
    }

    g_sampling.enabled = 1;
    g_sampling.total_samples = total_samples;
    g_sampling.progress_every = (progress_every == 0) ? 1U : progress_every;
    write_headers();

    printf("Runtime dataset sampling enabled.\n");
    printf("  Iteration CSV: %s\n", iteration_path);
    printf("  Summary CSV:   %s\n", summary_path);
    printf("  Total samples: %u\n", total_samples);
    printf("  Progress step: %u\n", g_sampling.progress_every);
    fflush(stdout);

    return 0;
}

int mlthre_runtime_sampling_is_enabled(void)
{
    return g_sampling.enabled;
}

void mlthre_runtime_sampling_close(void)
{
    if (!g_sampling.enabled)
    {
        return;
    }

    fclose(g_sampling.iteration_fp);
    fclose(g_sampling.summary_fp);
    memset(&g_sampling, 0, sizeof(g_sampling));
}

void mlthre_runtime_sampling_set_sample(uint32_t code_test_idx,
        uint32_t enc_test_idx,
        uint32_t sample_idx)
{
    if (!g_sampling.enabled)
    {
        return;
    }

    g_sampling.code_test_idx = code_test_idx;
    g_sampling.enc_test_idx = enc_test_idx;
    g_sampling.sample_idx = sample_idx;
    g_sampling.have_true_error = 0;

    if (sample_idx == 1 ||
            sample_idx == g_sampling.total_samples ||
            (sample_idx % g_sampling.progress_every) == 0)
    {
        printf("[progress] sample %u/%u (code=%u, enc=%u)\n",
                sample_idx, g_sampling.total_samples, code_test_idx,
                enc_test_idx);
        fflush(stdout);
    }
}

void mlthre_runtime_sampling_capture_enc_error(const uint8_t *e_bytes)
{
    if (!g_sampling.enabled)
    {
        return;
    }

    memset(g_sampling.true_error_bits, 0, sizeof(g_sampling.true_error_bits));
    convertByteToBinary(g_sampling.true_error_bits, (uint8_t *)e_bytes, N_BITS);
    g_sampling.true_error_weight = bit_weight(g_sampling.true_error_bits, N_BITS);
    g_sampling.have_true_error = 1;
}

void mlthre_runtime_sampling_record_iteration(const mlthre_state_t *state,
        uint32_t base_threshold,
        int32_t applied_delta,
        uint32_t applied_threshold,
        const uint8_t *guess_bits,
        const uint8_t *syndrome_bits)
{
    const uint32_t guess_weight = bit_weight(guess_bits, N_BITS);
    const uint32_t diffs = residual_diffs(guess_bits);
    const uint32_t syndrome_weight_after = bit_weight(syndrome_bits, R_BITS);

    if (!g_sampling.enabled)
    {
        return;
    }

    fprintf(g_sampling.iteration_fp,
            "%u,%u,%u,%u,%u,%u,%u,%d,%d,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,"
            "%u,%u,%u,%u\n",
            g_sampling.sample_idx, g_sampling.code_test_idx,
            g_sampling.enc_test_idx, state->model_security_level, state->iteration,
            base_threshold, applied_threshold, applied_delta, state->prev_delta,
            state->syndrome_weight, state->prev_syndrome_weight,
            state->syndrome_delta, state->z0, state->z1, state->black_count,
            state->gray_count, state->histogram[0], state->histogram[1],
            state->histogram[2], state->histogram[3], state->histogram[4],
            g_sampling.true_error_weight, guess_weight, diffs,
            syndrome_weight_after);
}

void mlthre_runtime_sampling_record_summary(int decoder_rc,
        const uint8_t *guess_bits,
        const uint8_t *syndrome_bits)
{
    const uint32_t guess_weight = bit_weight(guess_bits, N_BITS);
    const uint32_t diffs = residual_diffs(guess_bits);
    const uint32_t final_syndrome_weight = bit_weight(syndrome_bits, R_BITS);

    if (!g_sampling.enabled)
    {
        return;
    }

    fprintf(g_sampling.summary_fp,
            "%u,%u,%u,%u,%u,%u,%u,%u,%d\n",
            g_sampling.sample_idx, g_sampling.code_test_idx,
            g_sampling.enc_test_idx, BIKE_SECURITY_BITS,
            g_sampling.true_error_weight, guess_weight, diffs,
            final_syndrome_weight, decoder_rc);
}
