/*
Copyright (c) 2026 Yu Zhang, Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences
File Description: Declares unified symmetric helper routines for the optimized POLARLAC-Light instance.
                  Supports SHAKE (via fips202) and SM3 (via auxfunc) through BIT_USE_SHAKE.
*/

#ifndef SYMMETRIC_H
#define SYMMETRIC_H

#include <stddef.h>
#include <stdint.h>
#include "params.h"


#if BIT_USE_SHAKE
#include "fips202.h"
#include "fips202x4.h"
#endif

typedef struct {
#if BIT_USE_SHAKE
    keccak_state state;
#else
    const unsigned char *seed;
    size_t seed_len_bytes;
    uint32_t block;
#endif
    uint8_t nonce;
} xof_stream;

typedef struct {
#if BIT_USE_SHAKE
    keccak_state state;
#else
    const unsigned char *seed;
    size_t seed_len_bytes;
    uint32_t block;
#endif
    uint8_t nonce;
} xof128_stream;

#ifdef __cplusplus
extern "C"
{
#endif

    /// @brief Project-wide 256-bit hash entry point.
    /// @param[in] msg Input byte array
    /// @param[in] msg_len_bits Input length in bits
    /// @param[out] digest 32-byte output digest
    /// @return 0 for success, others for error
    int bit_hash_256(const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

    /// @brief Project-wide XOF entry point.
    /// @param[in] output_len_bits Total bits of output
    /// @param[in] msg Input byte array
    /// @param[in] msg_len_bits Input length in bits
    /// @param[out] output Output byte array
    /// @return 0 for success, others for error
    int bit_xof(unsigned long long output_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *output);

    /// @brief Byte-oriented 256-bit hash entry point; reuses the selected symmetric backend.
    int bit_hash_256_bytes(const unsigned char *msg, size_t msg_len_bytes, unsigned char *digest);

    /// @brief Byte-oriented XOF entry point; avoids repeated bit-length checks on byte-aligned paths.
    int bit_xof_bytes(unsigned char *output, size_t output_len_bytes, const unsigned char *msg, size_t msg_len_bytes);

    /// @brief Byte-oriented 256-bit hash over two concatenated inputs.
    int bit_hash_256_concat2_bytes(const unsigned char *msg0, size_t msg0_len_bytes,
                                   const unsigned char *msg1, size_t msg1_len_bytes,
                                   unsigned char *digest);

    /// @brief Byte-oriented XOF over two concatenated inputs.
    int bit_xof_concat2_bytes(unsigned char *output, size_t output_len_bytes,
                              const unsigned char *msg0, size_t msg0_len_bytes,
                              const unsigned char *msg1, size_t msg1_len_bytes);

    /// @brief Initialize a project-wide XOF stream (SHAKE256 / pseudoXOF).
    /// @param[out] stream XOF stream state
    /// @param[in] seed Domain-separated seed/prefix
    /// @param[in] seed_len_bytes Seed length in bytes
    /// @param[in] nonce Domain-separation nonce
    void bit_xof_stream_init(xof_stream *stream, const unsigned char *seed, size_t seed_len_bytes, uint8_t nonce);

    /// @brief Squeeze bytes from a project-wide XOF stream.
    /// @param[in,out] stream XOF stream state
    /// @param[out] output Output byte array
    /// @param[in] output_len_bytes Number of bytes to squeeze
    /// @return 0 for success, others for error
    int bit_xof_stream_squeeze(xof_stream *stream, unsigned char *output, size_t output_len_bytes);

    /// @brief Release resources held by an XOF stream.
    /// @param[in,out] stream XOF stream state
    void bit_xof_stream_release(xof_stream *stream);

    /// @brief Initialize a SHAKE128 XOF stream for public matrix expansion.
    /// @param[out] stream XOF stream state
    /// @param[in] seed Domain-separated seed/prefix
    /// @param[in] seed_len_bytes Seed length in bytes
    /// @param[in] nonce Domain-separation nonce
    void bit_xof128_stream_init(xof128_stream *stream, const unsigned char *seed, size_t seed_len_bytes, uint8_t nonce);

    /// @brief Squeeze bytes from a SHAKE128 XOF stream.
    /// @param[in,out] stream XOF stream state
    /// @param[out] output Output byte array
    /// @param[in] output_len_bytes Number of bytes to squeeze
    /// @return 0 for success, others for error
    int bit_xof128_stream_squeeze(xof128_stream *stream, unsigned char *output, size_t output_len_bytes);

    /// @brief Four-way byte-oriented SHAKE128/pseudoXOF expansion without an appended nonce.
    int bit_xof128x4_bytes(unsigned char *out0, unsigned char *out1, unsigned char *out2, unsigned char *out3,
                           size_t out_len_bytes,
                           const unsigned char *in0, const unsigned char *in1,
                           const unsigned char *in2, const unsigned char *in3,
                           size_t in_len_bytes);

    /// @brief Single-stream SHAKE128/pseudoXOF expansion for public matrix generation with appended nonce 0.
    ///        This is used by the SM3/pseudoXOF A-matrix path to avoid the x4 fallback wrapper overhead.
    int bit_xof128_bytes_nonce0(unsigned char *out, size_t out_len_bytes,
                                 const unsigned char *in, size_t in_len_bytes);

    /// @brief Four-way SHAKE128 expansion for public matrix generation with appended nonce 0.
    int bit_xof128x4_bytes_nonce0(unsigned char *out0, unsigned char *out1, unsigned char *out2, unsigned char *out3,
                                  size_t out_len_bytes,
                                  const unsigned char *in0, const unsigned char *in1,
                                  const unsigned char *in2, const unsigned char *in3,
                                  size_t in_len_bytes);

    /// @brief Release resources held by a SHAKE128 XOF stream.
    /// @param[in,out] stream XOF stream state
    void bit_xof128_stream_release(xof128_stream *stream);

    /// @brief Read the next nonce value after this stream domain.
    /// @param[in] stream XOF stream state
    /// @return Next domain-separation nonce
    uint8_t bit_xof_stream_next_nonce(const xof_stream *stream);

    /// @brief Expand an input message and a nonce into an output bit string.
    ///        Backward-compatible wrapper — prefer bit_xof_stream_* for new code.
    /// @param[in] output_len_bits Total bits of output
    /// @param[in] msg Base address of input message byte array
    /// @param[in] msg_len_bits Total bits of input message
    /// @param[out] output Base address of output byte array
    /// @param[in] nonce Single-byte nonce used to diversify the XOF input
    void rl_kem_pseudoXOF(unsigned long long output_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *output, uint8_t nonce);

#ifdef __cplusplus
}
#endif
#endif
