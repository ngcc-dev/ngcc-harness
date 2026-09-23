/*
 * ICCS API_CryptHash interface header for AFS-TrEDM-1024.
 */

#ifndef CRYPTHASH_ALGORITHM_INSTANCE_H
#define CRYPTHASH_ALGORITHM_INSTANCE_H

/* Set to 0 to generate filled test vectors with KAT_CryptHash.c. */
#define OUTPUT_BLANK_TEST_VECTORS 0

/* Algorithm instance name. Only letters, numbers, '-' and '_' are used. */
#define ALGORITHM_INSTANCE "AFS-TrEDM-1024"

/* Fixed digest length for this instance, in bits. */
#define DIGEST_BIT_LENGTH 1024

#ifdef __cplusplus
extern "C"
{
#endif

int CryptHash(int digest_len_bits,
              const unsigned char *msg,
              unsigned long long msg_len_bits,
              unsigned char *digest);

#ifdef __cplusplus
}
#endif

#endif /* CRYPTHASH_ALGORITHM_INSTANCE_H */
