/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-128 instance.
*/
#ifndef SAMPLE_H
#define SAMPLE_H

#include <stdint.h>
#include "params.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /// @brief Sample polynomial coefficients from a centered binomial distribution with parameter 1
    /// @param[out] r Base address of output polynomial coefficient array
    /// @param[in] buf Base address of input pseudorandom byte array
    /// @return None
    void cbd1(int16_t *r, const uint8_t *buf);

    /// @brief Sample polynomial coefficients from a centered binomial distribution with parameter 2
    /// @param[out] r Base address of output polynomial coefficient array
    /// @param[in] buf Base address of input pseudorandom byte array
    /// @return None
    void cbd2(int16_t *r, const uint8_t *buf);

    /// @brief Sample ternary polynomial coefficients with probability 1/8 of being nonzero
    /// @param[out] r Base address of output ternary polynomial coefficient array
    /// @param[in] buf Base address of input pseudorandom byte array
    /// @return None
    void tenary1_8(int16_t *r, const uint8_t *buf);

    /// @brief Sample ternary polynomial coefficients with probability 3/32 of being nonzero
    /// @param[out] r Base address of output ternary polynomial coefficient array
    /// @param[in] buf Base address of input pseudorandom byte array
    /// @return None
    void tenary3_32(int16_t *r, const uint8_t *buf);



#ifdef __cplusplus
}
#endif
#endif