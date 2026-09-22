#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "decoder.h"
#include "gf2x.h"

// Returns the Hamming weight.
static inline uint32_t get_hamming_weight(const uint8_t *w)
{
    uint32_t weight = 0;
    for (size_t i = 0; i < R_SIZE_BYTES; i++)
    {
        weight += __builtin_popcount(w[i]);
    }
    return weight;
}

// Returns the threshold for the current iteration.
static inline uint32_t get_threshold(uint32_t sum_init, uint32_t unsat, size_t iter)
{
    double Tp = COA_FP * sum_init + COB_FP;

    double M = (PARAM_D + 1.0) / 2.0;
    uint32_t t = 0;

    if (iter == 0)
    {
        t = (uint32_t)ceil(Tp) + PARAM_DELTA;
    }
    else if (iter == 1)
    {
        t = (uint32_t)ceil((2.0 * Tp + M) / 3.0) + PARAM_DELTA;
    }
    else if (iter == 2)
    {
        t = (uint32_t)ceil((Tp + 2.0 * M) / 3.0) + PARAM_DELTA;
    }
    else
    {
        t = (uint32_t)ceil(M) + PARAM_DELTA;
    }

    uint32_t fs = (uint32_t)ceil(COA_FP * unsat + COB_FP);

    uint32_t mask = -(fs < t);

    return (t & ~mask) | (fs & mask);
}

// Calculates the unsatisfied parity check counts for each bit in the error vector.
void calc_upc_block(
    const uint8_t *s, const uint32_t *idx, uint32_t *unsat_cnt)
{
    for (size_t i = 0; i < PARAM_R; i++)
    {
        uint32_t count = 0;
        for (size_t k = 0; k < PARAM_D; k++)
        {
            uint32_t idx_to = (i + idx[k]) % PARAM_R;
            count += (s[idx_to >> 3] >> (idx_to & 7)) & 1;
        }
        unsat_cnt[i] = count;
    }
}

// Flips the bits in the error vector based on the unsatisfied parity check counts and the threshold.
void flip_bits(uint8_t *e, const uint32_t *unsat_cnt, uint32_t threshold)
{
    size_t idx = 0;
    for (size_t i = 0; i < R_SIZE_BYTES; i++)
    {
        uint8_t byte = 0;
        for (size_t j = 0; j < 8 && idx < PARAM_R; j++, idx++)
        {
            byte |= (uint8_t)(unsat_cnt[idx] >= threshold) << j;
        }
        e[i] ^= byte;
    }
}

// Calculates the syndrome based on the initial syndrome and the error vectors.
void calculate_syndrome(
    uint8_t *s, const uint8_t *s_init, const uint8_t *e0, const uint8_t *e1, const uint8_t *e2,
    const uint32_t *idx0, const uint32_t *idx1, const uint32_t *idx2)
{
    memcpy(s, s_init, R_SIZE_BYTES);
    uint8_t *temp = (uint8_t *)calloc(R_ZMM_SIZE_BYTES, sizeof(uint8_t));
    if (temp == NULL) return;

    for (size_t k = 0; k < PARAM_D; k++)
    {
        gf2x_shift(temp, e0, idx0[k]);
        gf2x_add(s, s, temp);
    }

    for (size_t k = 0; k < PARAM_D; k++)
    {
        gf2x_shift(temp, e1, idx1[k]);
        gf2x_add(s, s, temp);
    }

    for (size_t k = 0; k < PARAM_D; k++)
    {
        gf2x_shift(temp, e2, idx2[k]);
        gf2x_add(s, s, temp);
    }

    free(temp);
}

// Bit-flipping decoding algorithm.
void bit_flip_decode(
    uint8_t *e0, uint8_t *e1, uint8_t *e2,
    const uint8_t *s_init, const uint32_t *idx0, const uint32_t *idx1, const uint32_t *idx2)
{
    uint32_t *unsat_cnt = (uint32_t *)calloc(PARAM_N, sizeof(uint32_t));
    uint8_t *s = (uint8_t *)calloc(R_ZMM_SIZE_BYTES, sizeof(uint8_t));
    if (unsat_cnt == NULL || s == NULL)
    {
        free(unsat_cnt);
        free(s);
        return;
    }

    memcpy(s, s_init, R_ZMM_SIZE_BYTES);
    uint32_t sum_init = get_hamming_weight(s);
    uint32_t unsat = sum_init;

    for (size_t iter = 0; iter < BIT_FLIP_ITER; iter++)
    {
        uint32_t threshold = get_threshold(sum_init, unsat, iter);

        calc_upc_block(s, idx0, unsat_cnt);
        calc_upc_block(s, idx1, unsat_cnt + PARAM_R);
        calc_upc_block(s, idx2, unsat_cnt + PARAM_R * 2);

        flip_bits(e0, unsat_cnt, threshold);
        flip_bits(e1, unsat_cnt + PARAM_R, threshold);
        flip_bits(e2, unsat_cnt + PARAM_R * 2, threshold);

        calculate_syndrome(s, s_init, e0, e1, e2, idx0, idx1, idx2);
        unsat = get_hamming_weight(s);
    }

    free(unsat_cnt);
    free(s);
}

// Builds sparse index tables and decodes the error vector.
void decode(
    uint8_t *e0, uint8_t *e1, uint8_t *e2,
    const uint8_t *s, const uint32_t *idx0, const uint32_t *idx1, const uint32_t *idx2)
{
    bit_flip_decode(e0, e1, e2, s, idx0, idx1, idx2);
}