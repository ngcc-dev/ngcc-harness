/*
 * WChain-V2-1024 API_CryptHash submission interface.
 */

#ifndef CRYPTHASH_ALGORITHM_INSTANCE_H
#define CRYPTHASH_ALGORITHM_INSTANCE_H

#define OUTPUT_BLANK_TEST_VECTORS 0
#define ALGORITHM_INSTANCE "WChain-V2-1024"
#define DIGEST_BIT_LENGTH 1024

#ifdef __cplusplus
extern "C" {
#endif

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest);

#ifdef __cplusplus
}
#endif

#endif
