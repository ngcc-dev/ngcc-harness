/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/
#include <stdint.h>

#ifndef CRYPTHASH_ALGORITHM_INSTANCE_H
#define CRYPTHASH_ALGORITHM_INSTANCE_H

// Set "OUTPUT_BLANK_TEST_VECTORS" as 0 to generate test vector files
// Set "OUTPUT_BLANK_TEST_VECTORS" as 1 to generate blank template (default)
#define OUTPUT_BLANK_TEST_VECTORS 0

// Set "ALGORITHM_INSTANCE" as your algorithm instance name (no more than 64 bytes)
// Only letters, numbers, '-' or '_' are permitted
#define ALGORITHM_INSTANCE "Thunder-768"

// Set "DIGEST_BIT_LENGTH" as the message digest length of your algorithm instance
#define DIGEST_BIT_LENGTH 768
#define DIGEST_LANE_NUM (DIGEST_BIT_LENGTH / 64) // 64 bits per lane

/* Set "CAPACITY_BITS" as the capacity in bits */
#define CAPACITY_BITS 832
#define IV_LANE_NUM (CAPACITY_BITS / 64) // 64 bits per lane

/* Set "RATE_BITS" as the rate in bits */
#define RATE_BITS (1600 - CAPACITY_BITS)
#define RATE_BYTES (RATE_BITS / 8)
#define MSG_LANE_NUM (RATE_BITS / 64) // 64 bits per lane

/* Initial Values: IV[0] is the least significant lane */
typedef uint64_t u64;
static const u64 IV[IV_LANE_NUM] = {
    0xE1F7C9010E8EC178ULL,
    0x676B3791EBA6281CULL,
    0xA7A16EAC10689E8CULL,
    0x9CCCEF7ACDC19758ULL,
    0x900E93CA9A4A9953ULL,
    0x55FBD3148A3A352AULL,
    0xDCFA816272A35C8CULL,
    0xB9839A4AF0CAE3D5ULL,
    0xDB34E00BB8122876ULL,
    0xCC3FA3279D6371F6ULL,
    0x1E6D7FF8F050881CULL,
    0x9C67610B0EF5484CULL,
    0x1EE2C8609716E1FDULL
};
#ifdef __cplusplus
extern "C"
{
#endif

    /// @brief Input message to get message digests of specified lengths
    /// @param[in] digest_len_bits The total BITS of digest
    /// @param[in] msg The base address of message
    /// @param[in] msg_len_bits The total BITS of message
    /// @param[out] digest The base address of digest
    /// @return 0 for success, others for error
    int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

#ifdef __cplusplus
}
#endif
#endif