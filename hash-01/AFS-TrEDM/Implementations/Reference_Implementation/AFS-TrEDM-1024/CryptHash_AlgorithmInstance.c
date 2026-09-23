/*
 * AFS-TrEDM CryptHash() wrapper.
 *
 * This file implements the ICCS API_CryptHash programming interface.
 */

#include <string.h>

#include "CryptHash_AlgorithmInstance.h"
#include "afs_tredm.h"

/* Function CryptHash: implements the ICCS API_CryptHash entry point for this fixed digest-length instance. */
int CryptHash(int digest_len_bits,
              const unsigned char *msg,
              unsigned long long msg_len_bits,
              unsigned char *digest)
{
    int rt;

    if (digest == 0) {
        return -10;
    }
    if (digest_len_bits != DIGEST_BIT_LENGTH) {
        return -11;
    }
    if (msg == 0 && msg_len_bits != 0ULL) {
        return -12;
    }

    memset(digest, 0, (unsigned long long)DIGEST_BIT_LENGTH / 8ULL);
    rt = afs_tredm_hash(digest_len_bits, msg, msg_len_bits, digest);
    if (rt != AFS_TREDM_SUCCESS) {
        return -20 + rt;
    }
    return 0;
}
