/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares polynomial sampling routines for the optimized POLARLAC-512 instance.
*/

#ifndef SAMPLE_H
#define SAMPLE_H

#include <stdint.h>
#include "params.h"
#include "poly.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /// @brief Generate a ternary polynomial from an input seed and nonce.
    /// @param[out] a Base address of output polynomial coefficient array
    /// @param[in] seed Base address of KEM_SEED_LEN_BYTES input seed byte array
    /// @param[in] nonce Single-byte nonce used to diversify the pseudoXOF input
    /// @details For this parameter set, coefficients follow the
    ///          distribution {-1:3/16, 0:5/8, 1:3/16}.
    void poly_generate_tenary(int16_t *a, const uint8_t *seed, uint8_t nonce);

    /// @brief Generate the public matrix A, or A^T when transposed is nonzero.
    /// @param[out] a Base address of output polynomial matrix
    /// @param[in] seed Base address of PK_SEED_LEN_BYTES input seed byte array
    /// @param[in] transposed Nonzero to generate the transposed matrix A^T
    /// @details Matrix entries are sampled uniformly modulo RL_KEM_Q and used
    ///          directly in the multiplication-domain representation.
    void poly_generate_uniformQ(polarlac_polymat *a, const uint8_t *seed, int transposed);


#ifdef POLARLAC_B_TEST_HOOKS
    void polarlac_b_test_trace_reset(void);
    uint64_t polarlac_b_test_trace_ternary_bytes(void);
    uint64_t polarlac_b_test_trace_uniform_bytes(void);
    uint32_t polarlac_b_test_trace_q5_calls(void);
    uint32_t polarlac_b_test_trace_q4_calls(void);
    uint64_t polarlac_b_test_divmod_769(uint64_t x, uint64_t *remainder);
#endif

#ifdef __cplusplus
}
#endif
#endif
