/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#ifndef AUXFUNC_H
#define AUXFUNC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /// @brief The SM3 hash function
    /// @param[in] digest_len_bits Total bits (only 256) of output digest
    /// @param[in] msg Base address of input message byte array
    /// @param[in] msg_len_bits Total bits of input message
    /// @param[out] digest Base address of output message digest byte array
    /// @return 0 for success, others for error
    int sm3hash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

    /// @brief A pseudo-hash function constructed using SM3 and HMAC-SM3 to generate a 512/768/1024-bit hash value
    ///        It is only used for correctness verification of implementations, and does not guarantee security
    /// @param[in] digest_len_bits Total bits (only 512/768/1024) of output digest
    /// @param[in] msg Base address of input message byte array
    /// @param[in] msg_len_bits Total bits of input message
    /// @param[out] digest Base address of output message digest byte array
    /// @return 0 for success, others for error
    int pseudohash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

    /// @brief An eXtensible Output Function (XOF) constructed using KDF-SM3 to generate an output of specified length
    ///        It is only used for correctness verification of implementations, and does not guarantee security
    /// @param[in] output_len_bits Total bits (less than (2^40-2^8)) of output
    /// @param[in] msg Base address of input message byte array
    /// @param[in] msg_len_bits Total bits of input message
    /// @param[out] output Base address of output byte array
    /// @return 0 for success, others for error
    int pseudoXOF(unsigned long long output_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *output);

    /// @brief Byte-aligned KDF-SM3 XOF helper without heap allocation
    /// @param[out] output Base address of output byte array
    /// @param[in] output_len_bytes Total bytes of output
    /// @param[in] msg Base address of input message byte array
    /// @param[in] msg_len_bytes Total bytes of input message
    /// @return 0 for success, others for error
    int iccs_pseudoxof_bytes_scalar(unsigned char *output, size_t output_len_bytes, const unsigned char *msg, size_t msg_len_bytes);

    /// @brief One byte-aligned KDF-SM3 counter block SM3(msg || counter_be32)
    /// @param[out] output Base address of 32-byte output block
    /// @param[in] msg Base address of input message byte array
    /// @param[in] msg_len_bytes Total bytes of input message
    /// @param[in] counter KDF counter value, encoded as big-endian
    /// @return 0 for success, others for error
    int iccs_sm3_counter_block(unsigned char output[32], const unsigned char *msg, size_t msg_len_bytes, unsigned int counter);

#ifdef __cplusplus
}
#endif

#endif
