/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#ifndef CRYPTHASH_ALGORITHM_INSTANCE_H
#define CRYPTHASH_ALGORITHM_INSTANCE_H

// Set "OUTPUT_BLANK_TEST_VECTORS" as 0 to generate test vector files
// Set "OUTPUT_BLANK_TEST_VECTORS" as 1 to generate blank template (default)
#define OUTPUT_BLANK_TEST_VECTORS 0

// Set "ALGORITHM_INSTANCE" as your algorithm instance name (no more than 64
// bytes) Only letters, numbers, '-' or '_' are permitted
#define ALGORITHM_INSTANCE "Duet-768"

// Set "DIGEST_BIT_LENGTH" as the message digest length of your algorithm
// instance
#define DIGEST_BIT_LENGTH 768

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Input message to get message digests of specified lengths
/// @param[in] digest_len_bits The total BITS of digest
/// @param[in] msg The base address of message
/// @param[in] msg_len_bits The total BITS of message
/// @param[out] digest The base address of digest
/// @return 0 for success, others for error
int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest);

#ifdef __cplusplus
}
#endif
#endif
