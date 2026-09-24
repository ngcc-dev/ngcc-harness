#include "mlthre_policy.h"

#include "defs.h"
#include "mlthre_model.h"
#include "threshold.h"

#include <math.h>

static uint32_t mlthre_clamp_threshold(size_t threshold)
{
    if (threshold < 1U)
    {
        return 1U;
    }
    if (threshold > DV)
    {
        return (uint32_t)DV;
    }
    return (uint32_t)threshold;
}

static uint32_t mlthre_dynamic_threshold(uint32_t syndrome_weight)
{
    return mlthre_clamp_threshold(compute_threshold(R_BITS, N_BITS, DV,
                2 * DV, syndrome_weight, T1));
}

static double mlthre_var_threshold(uint32_t model_security_level,
        uint32_t syndrome_weight)
{
    if (model_security_level == 7U)
    {
        return (double)mlthre_dynamic_threshold(syndrome_weight);
    }
    if (model_security_level == 1U)
    {
        return 11.101432337243956 + 0.006254868353074983 *
                (double)syndrome_weight;
    }
    if (model_security_level == 3U)
    {
        return 13.282669604666431 + 0.004533882596007288 *
                (double)syndrome_weight;
    }

    return 15.430866686308178 + 0.0036083738659016262 *
            (double)syndrome_weight;
}

static double mlthre_lower_bound(uint32_t model_security_level,
        uint32_t initial_weight,
        uint32_t iteration)
{
    if (model_security_level == 7U)
    {
        (void)iteration;
        return (double)mlthre_dynamic_threshold(initial_weight);
    }

    const double t0 = mlthre_var_threshold(model_security_level, initial_weight);
    const double x = (t0 - ((double)DV + 1.0) / 2.0) / 3.0;

    if (model_security_level == 1U)
    {
        if (iteration == 1U)
        {
            return t0 + 3.0;
        }
        if (iteration == 2U)
        {
            return t0 + 3.0 - x;
        }
        if (iteration == 3U)
        {
            return t0 + 3.0 - 2.0 * x;
        }
        return ((double)DV + 1.0) / 2.0 + 3.0;
    }

    if (model_security_level == 3U)
    {
        if (iteration == 1U)
        {
            return t0 + 5.0;
        }
        if (iteration == 2U)
        {
            return t0 + 5.0 - x;
        }
        if (iteration == 3U)
        {
            return t0 + 5.0 - 2.0 * x;
        }
        return ((double)DV + 1.0) / 2.0 + 5.0;
    }

    if (iteration == 1U)
    {
        return t0 + 6.0;
    }
    if (iteration == 2U)
    {
        return t0 + 6.0 - x;
    }
    if (iteration == 3U)
    {
        return t0 + 6.0 - 2.0 * x;
    }
    return ((double)DV + 1.0) / 2.0 + 6.0;
}

uint32_t mlthre_compute_base_threshold(IN const uint32_t model_security_level,
        IN const uint32_t current_weight,
        IN const uint32_t initial_weight,
        IN const uint32_t iteration)
{
    if (model_security_level == 7U)
    {
        (void)initial_weight;
        (void)iteration;
        return mlthre_dynamic_threshold(current_weight);
    }

    const double var_threshold = mlthre_var_threshold(model_security_level,
            current_weight);
    const double lower_bound = mlthre_lower_bound(model_security_level,
            initial_weight, iteration);
    uint32_t threshold = (uint32_t)ceil(var_threshold > lower_bound ?
            var_threshold : lower_bound);

    if (threshold < (uint32_t)((DV + 1U) / 2U))
    {
        threshold = (uint32_t)((DV + 1U) / 2U);
    }
    if (threshold > (uint32_t)DV)
    {
        threshold = (uint32_t)DV;
    }
    return threshold;
}

int32_t mlthre_select_delta(IN const mlthre_state_t *state,
        IN const uint32_t base_threshold)
{
#if !defined(BIKE_MLTHRE_ENABLED) || (BIKE_MLTHRE_ENABLED == 0)
    (void)state;
    (void)base_threshold;
    return 0;
#else
#if !defined(BIKE_MLTHRE_512_MODEL_ENABLED) || (BIKE_MLTHRE_512_MODEL_ENABLED == 0)
    if (state->model_security_level == 7U)
    {
        (void)base_threshold;
        return 0;
    }
#endif

    if (state->iteration > 2U)
    {
        return 0;
    }

    const float features[MLTHRE_MODEL_INPUT_DIM] = {
        (float)state->model_security_level,
        (float)state->iteration,
        (float)state->initial_syndrome_weight,
        (float)state->prev_syndrome_weight,
        (float)state->syndrome_weight,
        (float)state->syndrome_delta,
        (float)state->z0,
        (float)state->z1,
        (float)mlthre_var_threshold(state->model_security_level,
                state->syndrome_weight),
        (float)mlthre_lower_bound(state->model_security_level,
                state->initial_syndrome_weight, state->iteration),
        (float)base_threshold
    };

    return mlthre_model_predict_delta(features);
#endif
}
