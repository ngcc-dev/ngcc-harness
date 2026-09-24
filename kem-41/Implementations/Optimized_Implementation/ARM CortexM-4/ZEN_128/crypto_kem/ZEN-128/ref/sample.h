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

    /// @brief Load 4 bytes into a 32-bit unsigned integer in little-endian order
    /// @param[in] x Base address of input byte array
    /// @return 32-bit unsigned integer loaded from the input byte array x
    uint32_t load32_littleendian(const uint8_t *x);

    /// @brief Compute polynomial coefficients following a centered binomial distribution with parameter eta = 1
    /// @param[out] r Base address of output polynomial coefficient array
    /// @param[in] buf Base address of input uniformly random byte array
    /// @return None    
    void cbd1(int16_t *r, const uint8_t *buf);

    /// @brief Compute polynomial coefficients following a centered binomial distribution with parameter eta = 2
    /// @param[out] r Base address of output polynomial coefficient array
    /// @param[in] buf Base address of input uniformly random byte array
    /// @return None    
    void cbd2(int16_t *r, const uint8_t *buf);

    /// @brief Generate a ternary polynomial with coefficient distribution {-1:1/8, 0:3/4, 1:1/8} from an input byte array
    /// @param[out] r Base address of output polynomial coefficient array
    /// @param[in] buf Base address of input byte array
    /// @return None
    void tenary1_8(int16_t *r, const uint8_t *buf);

    /// @brief Generate a ternary polynomial with coefficient distribution {-1:3/32, 0:13/16, 1:3/32} from an input byte array
    /// @param[out] r Base address of output polynomial coefficient array
    /// @param[in] buf Base address of input byte array
    /// @return None
    void tenary3_32(int16_t *r, const uint8_t *buf);


#ifdef __cplusplus
}
#endif
#endif