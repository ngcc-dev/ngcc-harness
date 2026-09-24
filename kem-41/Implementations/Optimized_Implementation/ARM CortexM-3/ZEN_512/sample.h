/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-512 instance.
*/
#ifndef SAMPLE_H
#define SAMPLE_H

#include <stdint.h>
#include "params.h"

#ifdef __cplusplus
extern "C"
{
#endif

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