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
#define ALGORITHM_INSTANCE "Thunder-XOF-512"

// Set "DIGEST_BIT_LENGTH" as the message digest length of your algorithm instance
#define DIGEST_BIT_LENGTH 1024

/* Set "SQUEEZE_BIT_LENGTH" as the number of output bits per squeezing step */
#define SQUEEZE_BIT_LENGTH 1024
#define SQUEEZE_LANE_NUM (SQUEEZE_BIT_LENGTH / 64) // 64 bits per lane

/* Set "CAPACITY_BITS" as the capacity in bits */
#define CAPACITY_BITS 1088
#define IV_LANE_NUM (CAPACITY_BITS / 64) // 64 bits per lane

/* Set "RATE_BITS" as the rate in bits */
#define RATE_BITS (1600 - CAPACITY_BITS)
#define RATE_BYTES (RATE_BITS / 8)
#define MSG_LANE_NUM (RATE_BITS / 64) // 64 bits per lane

/* Initial Values:
 */
typedef uint64_t u64;
static const u64 IV[IV_LANE_NUM] = {
    0x9AEEEF2B632AA55EULL,
    0xFFF86452843778BCULL,
    0x7EECDF54AA725D35ULL,
    0x9A48816B41176EB4ULL,
    0x0CB97494B14EA9E1ULL,
    0x01C0349676559785ULL,
    0x9DE98219698CF8DDULL,
    0x621BC66649423A58ULL,
    0x3E1620F081AF14FAULL,
    0xD5E7B03035AB65EFULL,
    0xFA818BD5EB11B50CULL,
    0x3B3C04DD07203847ULL,
    0x7B767D98DA24EC2CULL,
    0xF081647F0AD178DDULL,
    0xBAB84A614A2350C9ULL,
    0x66A72645A0F40A18ULL,
    0xC2884D525C702B64ULL
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
