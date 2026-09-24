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
#define ALGORITHM_INSTANCE "Dragon-512"

// Set "DIGEST_BIT_LENGTH" as the message digest length of your algorithm instance
#define DIGEST_BIT_LENGTH 512
#define DIGEST_LANE_NUM (DIGEST_BIT_LENGTH / 64) // 64 bits per lane

/* Set "CAPACITY_BITS" as the capacity in bits */
#define CAPACITY_BITS 576
#define IV_LANE_NUM (CAPACITY_BITS / 64) // 64 bits per lane

/* Set "RATE_BITS" as the rate in bits */
#define RATE_BITS (1600 - CAPACITY_BITS)
#define RATE_BYTES (RATE_BITS / 8)
#define MSG_LANE_NUM (RATE_BITS / 64) // 64 bits per lane

/* Initial Values:
 * Note: IV_0 corresponds to the last 64-bit word,
 * i.e., IV_0 = 0x0B6B2E774D8F675B.
 */
typedef uint64_t u64;
static const u64 IV[IV_LANE_NUM] = {0x2D5EBD181E489EBB, 0xDDAA68D459FF4EE7, 0xD8F5D940D993F0D3, 0x86F2A0CA3B8E0D1E, 0xDC1851BCA4509C55, 0x8760145EFF9D8EB9, 0x9B39251883C81294, 0x4402CE26DD9509A5, 0x0B6B2E774D8F675B};

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