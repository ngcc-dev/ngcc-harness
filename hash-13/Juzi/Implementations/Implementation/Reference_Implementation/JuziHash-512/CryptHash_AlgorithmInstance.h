/*
 * ICCS NGCC CryptHash API header for JuziHash-512.
 * Adapted from the API_CryptHash.zip programming interface.
 */
#ifndef CRYPTHASH_ALGORITHM_INSTANCE_H
#define CRYPTHASH_ALGORITHM_INSTANCE_H

/* 0: generate KAT files; 1: generate blank templates */
#define OUTPUT_BLANK_TEST_VECTORS 0

/* Algorithm instance name: only letters, numbers, '-' or '_' */
#define ALGORITHM_INSTANCE "JuziHash-512"

/* Message digest length in bits */
#define DIGEST_BIT_LENGTH 512

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Input message to get message digest of the specified length.
 * digest_len_bits: requested digest length in bits.
 * msg: base address of message.
 * msg_len_bits: total number of message bits. For JuziHash, 0 <= msg_len_bits < 2^64.
 * digest: output digest buffer. It must have DIGEST_BIT_LENGTH/8 bytes.
 * Return 0 for success, nonzero for error.
 */
int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest);

#ifdef __cplusplus
}
#endif
#endif
