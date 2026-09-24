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
#define ALGORITHM_INSTANCE "Dragon-XOF-512"

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
 * Note: IV_0 corresponds to the last 64-bit word,
 * i.e., IV_0 = 0xBD52175FC753D3D4.
 */
typedef uint64_t u64;
static const u64 IV[IV_LANE_NUM] = {0xD3B4A68FC9CC970F, 0x077B759252B1C628, 0x32CADAF2F882C6A3, 0xC1A02AEEB836F700, 0xF68F22D737F1EA69, 0xCDBEBA91248FD4E3, 0x55F7FADCD1110E22, 0x6F248BBD94D4B9B9, 0xEA2D075670D4FD64, 0xBC2CD1D926C6C690, 0xF02BFB02360C2345, 0x73D6F28EEC922318, 0xF33330E05F38A3C2, 0xF0E203D2561601C9, 0x519343F95981BBCF, 0xBF62000ACB12405F, 0xBD52175FC753D3D4};

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
