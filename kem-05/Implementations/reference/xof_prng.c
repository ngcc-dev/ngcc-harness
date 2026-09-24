/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/

#include <limits.h>
#include <string.h>

#include "auxfunc.h"
#include "xof_prng.h"

status_t xof_prng_init(
    const uint8_t *input,
    uint64_t input_len,
    xof_prng_state_t *state)
{
    if (input == NULL || state == NULL || input_len > ULLONG_MAX / 8ULL)
    {
        return E_XOF_FAIL;
    }

    memset(state, 0, sizeof(*state));
    if (pseudoXOF(
            sizeof(state->buffer) * 8ULL,
            input,
            input_len * 8ULL,
            state->buffer) != 0)
    {
        return E_XOF_FAIL;
    }

    return SUCCESS;
}

status_t xof_prng_generate(
    uint8_t *output,
    xof_prng_state_t *state,
    uint32_t output_len)
{
    if (output == NULL || state == NULL ||
        state->pos > sizeof(state->buffer) ||
        output_len > sizeof(state->buffer) - state->pos)
    {
        return E_XOF_OVER_USED;
    }

    memcpy(output, &state->buffer[state->pos], output_len);
    state->pos += output_len;
    return SUCCESS;
}
