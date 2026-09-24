/*
Embedded 32-bit implementation profile.
*/

#ifndef CRYPTHASH_ZC1280_EMB32_INSTANCE_H
#define CRYPTHASH_ZC1280_EMB32_INSTANCE_H

#define OUTPUT_BLANK_TEST_VECTORS 0
#define ALGORITHM_INSTANCE "ZC-DM-1280_Emb32-1024"
#define DIGEST_BIT_LENGTH 1024

#ifdef __cplusplus
extern "C" {
#endif

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

#ifdef __cplusplus
}
#endif

#endif
