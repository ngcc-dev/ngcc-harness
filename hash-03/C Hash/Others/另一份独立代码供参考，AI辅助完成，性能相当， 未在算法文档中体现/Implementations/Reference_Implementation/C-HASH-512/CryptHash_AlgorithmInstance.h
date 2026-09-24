/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).
*/

#ifndef CRYPTHASH_ALGORITHM_INSTANCE_H
#define CRYPTHASH_ALGORITHM_INSTANCE_H

#define OUTPUT_BLANK_TEST_VECTORS 0
#define ALGORITHM_INSTANCE "C-HASH-512"
#define DIGEST_BIT_LENGTH 512

#ifdef __cplusplus
extern "C"
{
#endif

    int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

#ifdef __cplusplus
}
#endif
#endif
