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