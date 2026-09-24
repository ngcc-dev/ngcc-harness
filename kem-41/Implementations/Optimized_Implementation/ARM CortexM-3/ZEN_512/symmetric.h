/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-512 instance.
*/
#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include <stdint.h>
#include "params.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /// @brief Generate pseudorandom output using pseudoXOF with an appended nonce
    /// @param[in] output_len_bits Desired output length in bits
    /// @param[in] msg Base address of input message or seed byte array
    /// @param[in] msg_len_bits Input message length in bits
    /// @param[out] output Base address of output pseudorandom byte array
    /// @param[in] nonce Domain-separation nonce appended to the input message
    /// @return None
    void zen_pseudoXOF(unsigned long long output_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *output, uint8_t nonce);


#ifdef __cplusplus
}
#endif
#endif