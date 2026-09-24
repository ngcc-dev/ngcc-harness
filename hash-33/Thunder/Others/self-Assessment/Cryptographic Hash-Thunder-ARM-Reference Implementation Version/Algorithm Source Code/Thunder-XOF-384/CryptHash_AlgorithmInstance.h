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
#define ALGORITHM_INSTANCE "Thunder-XOF-384"

// Set "DIGEST_BIT_LENGTH" as the message digest length of your algorithm instance
#define DIGEST_BIT_LENGTH 1152

/* Set "SQUEEZE_BIT_LENGTH" as the number of output bits per squeezing step */
#define SQUEEZE_BIT_LENGTH 1152
#define SQUEEZE_LANE_NUM (SQUEEZE_BIT_LENGTH / 64) // 64 bits per lane

/* Set "CAPACITY_BITS" as the capacity in bits */
#define CAPACITY_BITS 832
#define IV_LANE_NUM (CAPACITY_BITS / 64) // 64 bits per lane

/* Set "RATE_BITS" as the rate in bits */
#define RATE_BITS (1600 - CAPACITY_BITS)
#define RATE_BYTES (RATE_BITS / 8)
#define MSG_LANE_NUM (RATE_BITS / 64) // 64 bits per lane

/* Initial Values:
 */
typedef uint64_t u64;
static const u64 IV[IV_LANE_NUM] = {
    0x12213C610D4668BFULL,
    0x63A3C93B73413AB0ULL,
    0x03BF159A8C56AEEAULL,
    0x103353E2853DC514ULL,
    0x93B2116B09264243ULL,
    0x37F1F11B1F666C79ULL,
    0xA69B502F299DF9E8ULL,
    0x6322E49D1D66FD68ULL,
    0xF63C53A3C826DD36ULL,
    0xF4E8D96C8C2AA889ULL,
    0x3F8ADB0DCF6E3C57ULL,
    0x6663F2D34B981CE9ULL,
    0xF45C092FAC272ECDULL
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
