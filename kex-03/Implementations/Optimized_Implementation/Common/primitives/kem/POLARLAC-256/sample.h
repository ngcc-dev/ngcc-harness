/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares polynomial sampling routines for the optimized POLARLAC-256 instance.
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
    ///          distribution {-1:1/8, 0:3/4, 1:1/8}.
    void poly_generate_tenary(int16_t *a, const uint8_t *seed, uint8_t nonce);

    /// @brief Generate the public matrix A, or A^T when transposed is nonzero.
    /// @param[out] a Base address of output polynomial matrix
    /// @param[in] seed Base address of PK_SEED_LEN_BYTES input seed byte array
    /// @param[in] transposed Nonzero to generate the transposed matrix A^T
    void poly_generate_uniformQ(polarlac_polymat *a, const uint8_t *seed, int transposed);


#ifdef __cplusplus
}
#endif
#endif
