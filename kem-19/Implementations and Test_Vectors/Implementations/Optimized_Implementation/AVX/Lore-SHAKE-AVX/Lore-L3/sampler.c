#include "sampler.h"
#include "params.h"
#include "symmetric.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/*************************************************
* Name:        sample_fixed_weight 
*
* Description: Unified rejection sampling to generate a fixed-weight 
* error vector across all k*N positions using the Fisher-Yates shuffle.
*
* Arguments:   - poly_crt_vec *r_crt_vec:     pointer to the output CRT polynomial vector
* - poly_sparse *r_sparse_vec:   pointer to the output sparse polynomial vector
* - const unsigned char *seed:   pointer to the input seed
* - unsigned char nonce:         a single-byte nonce
**************************************************/
void sample_fixed_weight(poly_crt_vec *r_crt_vec, poly_sparse *r_sparse_vec, const unsigned char *seed, unsigned char nonce)
{
    // Buffer with a slight redundancy for the rare case of rejection events
    #define BUF_SIZE (LORE_K * LORE_HWT_TOTAL * 2 + 128)
    uint8_t buf[BUF_SIZE];
    uint16_t positions[LORE_K * LORE_N];

    // 1. PRF - Generate all necessary random bytes for the sampling process in one go
    prf(buf, BUF_SIZE, seed, nonce);
    uint32_t buf_pos = 0;

    // 2. Initialize all output structures
    memset(r_crt_vec, 0, sizeof(poly_crt_vec));
    for (int i = 0; i < LORE_K; ++i) {
        memset(&r_sparse_vec[i], 0, sizeof(poly_sparse));
    }
    
    // 3. Efficient "merged sampling" main loop
    const int hwt_counts[] = {LORE_HWT_P1, LORE_HWT_M1, LORE_HWT_P2, LORE_HWT_M2};
    const int16_t hwt_values[] = {1, -1, 2, -2};

    // Initialize the total position pool
    for(uint16_t j = 0; j < LORE_K * LORE_N; ++j) {
        positions[j] = j;
    }
    uint16_t current_len = LORE_K * LORE_N;

    // Uniformly sample all types of non-zero coefficients for all s_i
    for (int type = 0; type < 4; ++type) {
        int16_t total_count = (int16_t)(hwt_counts[type] * LORE_K);
        int16_t value = hwt_values[type];

        for (int j = 0; j < total_count; ++j) {
            
            uint32_t m;
            uint16_t rand_val;
            uint16_t choice_idx;
            
            // Rejection sampling loop for unbiased index selection.
            do {
                if (buf_pos + 2 > BUF_SIZE) {
                    /* PRF buffer exhausted: this should never happen with the
                     * current BUF_SIZE, but a silent return would produce a
                     * polynomial with incorrect Hamming weight, which is
                     * cryptographically dangerous. Abort loudly instead. */
                    abort();
                }
                rand_val = (uint16_t)(buf[buf_pos] | ((uint16_t)buf[buf_pos + 1] << 8));
                buf_pos += 2;
                m = (uint32_t)rand_val * current_len;
            } while ((uint16_t)m < current_len); // Rejection condition: lower 16 bits of m < current_len

            choice_idx = (uint16_t)(m >> 16); // Take the upper 16 bits of m as the unbiased result


            uint16_t global_pos = positions[choice_idx];

            // Core step of Fisher-Yates Shuffle: replace the selected position with the last element
            positions[choice_idx] = positions[current_len - 1];
            current_len--;

            // Map the global position to a specific polynomial index and local position
            int poly_idx = global_pos / LORE_N;
            uint16_t local_pos = global_pos % LORE_N;
            // Directly populate the CRT and sparse structures of the poly_idx-th polynomial
            poly_crt *crt = &r_crt_vec->vec[poly_idx];
            poly_sparse *sparse = &r_sparse_vec[poly_idx];
            
            // Populate CRT
            crt->q_poly.coeffs[local_pos] = value;
            crt->t_poly.coeffs[local_pos] = value;

            // Populate sparse structure
            sparse->pos[sparse->n_coeffs] = local_pos;
            sparse->val[sparse->n_coeffs] = (int8_t)value;
            sparse->n_coeffs++;
        }
    }
}


