#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include <stdint.h>
#include "params.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /// @brief Expand an input message and a nonce into an output bit string using pseudoXOF
    /// @param[in] output_len_bits Total bits of output
    /// @param[in] msg Base address of input message byte array
    /// @param[in] msg_len_bits Total bits of input message
    /// @param[out] output Base address of output byte array
    /// @param[in] nonce Single-byte nonce used to diversify the pseudoXOF input
    /// @return None
    void ZEN_pseudoXOF(unsigned long long output_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *output, uint8_t nonce);


#ifdef __cplusplus
}
#endif
#endif