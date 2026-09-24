/******************************************************************************
 * BIKE_MLThre
 ******************************************************************************/

#include "decode_ml.h"
#include "utilities.h"

#include "kem.h"
#include "sampling.h"

#include "ring_buffer.h"
#include "mlthre_policy.h"
#include "mlthre_runtime_sampling.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

// count number of 1's in tmp:
uint32_t getHammingWeight(const uint8_t tmp[R_BITS], const uint32_t length)
{
    uint32_t count = 0;
    for (uint32_t i = 0; i < length; i++)
    {
        count+=tmp[i];
    }

    return count;
}

// function (not constant time) to check if an array is zero:
uint32_t isZero(uint8_t s[R_BITS])
{
    for (uint32_t i = 0; i < R_BITS; i++)
    {
        if (s[i])
        {
            return 0;
        }
    }
    return 1;
}

void recompute_syndrome(uint8_t s[R_BITS],
        const uint32_t pos,
        const uint32_t h0_compact[DV],
        const uint32_t h1_compact[DV])
{
    if (pos < R_BITS)
    {
        for (uint32_t j = 0; j < DV; j++)
        {
            if (h0_compact[j] <= pos)
            {
                s[pos - h0_compact[j]] ^= 1;
            }
            else
            {
                s[R_BITS - h0_compact[j] +  pos] ^= 1;
            }
        }
    }
    else
    {
       for (uint32_t j = 0; j < DV; j++)
       {
          if (h1_compact[j] <= (pos - R_BITS))
               s[(pos - R_BITS) - h1_compact[j]] ^= 1;
           else
               s[R_BITS - h1_compact[j] + (pos - R_BITS)] ^= 1;
       }
    }
}

uint32_t ctr(
        uint32_t h_compact_col[DV],
        int position,
        uint8_t s[R_BITS])
{
    uint32_t count = 0;
    for (uint32_t i = 0; i < DV; i++)
    {
        if (s[(h_compact_col[i] + position) % R_BITS])
            count++;
    }
    return count;
}

void getCol(
        uint32_t h_compact_col[DV],
        uint32_t h_compact_row[DV])
{
    if (h_compact_row[0] == 0)
    {
        h_compact_col[0] = 0;

        for (uint32_t i = 1; i < DV; i++)
        {
            // set indices in increasing order:
            h_compact_col[i] = R_BITS - h_compact_row[DV-i];
        }
    } else
    {
        for (uint32_t i = 0; i < DV; i++)
        {
            // set indices in increasing order:
            h_compact_col[i] = R_BITS - h_compact_row[DV-1-i];
        }
    }
}

// The position in e is adjusted because syndrome is transposed.
void flipAdjustedErrorPosition(uint8_t e[R_BITS*2], uint32_t position)
{
    uint32_t adjustedPosition = position;
    if (position != 0 && position != R_BITS)
    {
        adjustedPosition = (position > R_BITS) ? \
                ((N_BITS - position)+R_BITS) : (R_BITS - position);
    }
    e[adjustedPosition] ^= 1;
}

void BFMaskedIter(uint8_t e[R_BITS*2],
    uint8_t s[R_BITS],
    uint8_t mask[R_BITS*2],
    uint32_t T,
    uint32_t h0_compact[DV],
    uint32_t h1_compact[DV],
    uint32_t h0_compact_col[DV],
    uint32_t h1_compact_col[DV])
{

    uint8_t pos[R_BITS*2] = {0};

    for (uint32_t j = 0; j < R_BITS; j++)
    {
        uint32_t counter = ctr(h0_compact_col, j, s);
        if (counter >= T && mask[j])
        {
            //e[j] ^= 1;
            flipAdjustedErrorPosition(e, j);
            pos[j] = 1;
        }
    }

    for (uint32_t j = 0; j < R_BITS; j++)
    {
        uint32_t counter = ctr(h1_compact_col, j, s);
        if (counter >= T && mask[R_BITS+j])
        {
            //e[R_BITS+j] ^= 1;
            flipAdjustedErrorPosition(e, R_BITS+j);
            pos[R_BITS+j] = 1;
        }
    }

    // flip bits at the end - as defined in the BGF decoder
    for(uint32_t j=0; j < 2*R_BITS; j++){
        if(pos[j] == 1){
            recompute_syndrome(s, j, h0_compact, h1_compact);
        }
    }
}

void BFIter(uint8_t e[R_BITS*2],
    uint8_t black[R_BITS*2],
    uint8_t gray[R_BITS*2],
    uint8_t s[R_BITS],
    uint32_t T,
    uint32_t h0_compact[DV],
    uint32_t h1_compact[DV],
    uint32_t h0_compact_col[DV],
    uint32_t h1_compact_col[DV])
{

    uint8_t pos[R_BITS*2] = {0};

    for (uint32_t j = 0; j < R_BITS; j++)
    {
        uint32_t counter = ctr(h0_compact_col, j, s);
        uint32_t gray_threshold = (T > tau) ? T - tau : 0;
        if (counter >= T)
        {
            //e[j] ^= 1;
            flipAdjustedErrorPosition(e, j);
            pos[j] = 1;
            black[j] = 1;
        } else if(counter >= gray_threshold)
        {
            gray[j] = 1;
        }
    }
    for (uint32_t j = 0; j < R_BITS; j++)
    {
        uint32_t counter = ctr(h1_compact_col, j, s);
        uint32_t gray_threshold = (T > tau) ? T - tau : 0;

        if (counter >= T)
        {
            //e[R_BITS+j] ^= 1;
            flipAdjustedErrorPosition(e, R_BITS+j);
            pos[R_BITS+j] = 1;
            black[R_BITS+j] = 1;
        } else if(counter >= gray_threshold)
            {
                gray[R_BITS+j] = 1;
            }
    }

    // flip bits at the end
    for(uint32_t j=0; j < 2*R_BITS; j++){
        if(pos[j] == 1){
            recompute_syndrome(s, j, h0_compact, h1_compact);
        }
    }
}

static void collect_threshold_metrics(
    uint8_t s[R_BITS],
    uint32_t T,
    uint32_t h0_compact_col[DV],
    uint32_t h1_compact_col[DV],
    uint32_t histogram[MLTHRE_NUM_BINS],
    uint32_t *black_count,
    uint32_t *gray_count)
{
    for (uint32_t j = 0; j < R_BITS; j++)
    {
        uint32_t counter = ctr(h0_compact_col, j, s);

        if ((counter + 2U) >= T && (counter <= T + 2U))
        {
            histogram[counter + 2U - T]++;
        }

        uint32_t gray_threshold = (T > tau) ? T - tau : 0;
        if (counter >= T)
        {
            (*black_count)++;
        }
        else if(counter >= gray_threshold)
        {
            (*gray_count)++;
        }
    }

    for (uint32_t j = 0; j < R_BITS; j++)
    {
        uint32_t counter = ctr(h1_compact_col, j, s);

        if ((counter + 2U) >= T && (counter <= T + 2U))
        {
            histogram[counter + 2U - T]++;
        }

        uint32_t gray_threshold = (T > tau) ? T - tau : 0;
        if (counter >= T)
        {
            (*black_count)++;
        }
        else if(counter >= gray_threshold)
        {
            (*gray_count)++;
        }
    }
}

// Algorithm BGF - Black-Gray-Flip Decoder
int BGF_decoder(uint8_t e[R_BITS*2],
    uint8_t s[R_BITS],
    uint32_t h0_compact[DV],
    uint32_t h1_compact[DV])
{
    memset(e, 0, R_BITS*2);

    // computing the first column of each parity-check block:
    uint32_t h0_compact_col[DV] = {0};
    uint32_t h1_compact_col[DV] = {0};
    getCol(h0_compact_col, h0_compact);
    getCol(h1_compact_col, h1_compact);

    uint8_t black[R_BITS*2] = {0};
    uint8_t gray[R_BITS*2] = {0};
    uint8_t prev_s[R_BITS] = {0};
    memcpy(prev_s, s, R_BITS);
    const uint32_t initial_weight = getHammingWeight(s, R_BITS);

    mlthre_state_t state = {0};
    state.model_security_level = BIKE_MODEL_SECURITY_LEVEL;
    state.initial_syndrome_weight = initial_weight;

    for (int i = 1; i <= NbIter; i++)
    {
        uint32_t histogram[MLTHRE_NUM_BINS] = {0};
        uint32_t black_count = 0;
        uint32_t gray_count = 0;
        uint32_t z0 = 0;
        uint32_t z1 = 0;

        memset(black, 0, R_BITS*2);
        memset(gray, 0, R_BITS*2);

        for (uint32_t bit = 0; bit < R_BITS; bit++)
        {
            if (s[bit])
            {
                if (prev_s[bit])
                {
                    z1++;
                }
                else
                {
                    z0++;
                }
            }
        }

        const uint32_t current_weight = getHammingWeight(s, R_BITS);
#if defined(BIKE_MLTHRE_ENABLED) && (BIKE_MLTHRE_ENABLED != 0)
        const uint32_t base_threshold = mlthre_compute_base_threshold(
                BIKE_MODEL_SECURITY_LEVEL, current_weight, initial_weight,
                (uint32_t)i);
#else
        const uint32_t base_threshold = floor(VAR_TH_FCT(current_weight));
#endif

        state.iteration = i;
        state.initial_syndrome_weight = initial_weight;
        state.syndrome_weight = current_weight;
        state.prev_syndrome_weight = (i == 1) ? current_weight :
                getHammingWeight(prev_s, R_BITS);
        state.syndrome_delta = (state.prev_syndrome_weight > current_weight) ?
                (state.prev_syndrome_weight - current_weight) : 0;
        state.z0 = z0;
        state.z1 = z1;

        collect_threshold_metrics(s, base_threshold, h0_compact_col,
                h1_compact_col, histogram, &black_count, &gray_count);

        state.black_count = black_count;
        state.gray_count = gray_count;
        memcpy(state.histogram, histogram, sizeof(histogram));

        int32_t delta = mlthre_select_delta(&state, base_threshold);
        int32_t corrected_threshold = (int32_t)base_threshold + delta;
        if (corrected_threshold < (int32_t)((DV + 1U) / 2U))
        {
            corrected_threshold = (int32_t)((DV + 1U) / 2U);
        }
        if (corrected_threshold > (int32_t)DV)
        {
            corrected_threshold = DV;
        }

        uint32_t T = (uint32_t)corrected_threshold;

        BFIter(e, black, gray, s, T, h0_compact, h1_compact, h0_compact_col,
                h1_compact_col);
        state.prev_delta = delta;

        if (i == 1)
        {
            BFMaskedIter(e, s, black, (DV+1)/2 + 1, h0_compact, h1_compact, h0_compact_col, h1_compact_col);
            BFMaskedIter(e, s, gray, (DV+1)/2 + 1, h0_compact, h1_compact, h0_compact_col, h1_compact_col);
        }

        mlthre_runtime_sampling_record_iteration(&state, base_threshold, delta,
                T, e, s);
        memcpy(prev_s, s, R_BITS);
    }
    mlthre_runtime_sampling_record_summary(
            (getHammingWeight(s, R_BITS) == 0) ? 0 : 1, e, s);
    if (getHammingWeight(s, R_BITS) == 0)
        return 0; // SUCCESS
    else
        return 1; // FAILURE
}
