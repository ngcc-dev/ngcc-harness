#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "CryptHash_AlgorithmInstance.h"

void FIPS202_SHAKE256(const unsigned char *input, unsigned int inputByteLen,
                      unsigned char *output, int outputByteLen);

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest)
{
    unsigned long long msg_len_bytes;
    unsigned long long absorb_len_bytes;
    unsigned long long digest_len_bytes;
    unsigned int partial_bits;
    unsigned char *aligned_msg;
    const unsigned char empty_msg = 0;
    const unsigned char *hash_msg;

    if (digest == NULL || digest_len_bits < 0) {
        return -1;
    }
    if (msg == NULL && msg_len_bits != 0ULL) {
        return -1;
    }
    if ((digest_len_bits & 7) != 0) {
        return -1;
    }

    msg_len_bytes = msg_len_bits / 8ULL;
    partial_bits = (unsigned int)(msg_len_bits & 7ULL);
    absorb_len_bytes = msg_len_bytes + (partial_bits != 0U);
    digest_len_bytes = (unsigned long long)digest_len_bits / 8ULL;
    if (absorb_len_bytes > (unsigned long long)UINT_MAX ||
        digest_len_bytes > (unsigned long long)INT_MAX) {
        return -1;
    }

    aligned_msg = NULL;
    hash_msg = (msg_len_bits == 0ULL) ? &empty_msg : msg;
    if (partial_bits != 0U) {
        aligned_msg = (unsigned char *)malloc((size_t)absorb_len_bytes);
        if (aligned_msg == NULL) {
            return -1;
        }
        memcpy(aligned_msg, msg, (size_t)absorb_len_bytes);
        aligned_msg[absorb_len_bytes - 1ULL] &=
            (unsigned char)(0xFFU << (8U - partial_bits));
        hash_msg = aligned_msg;
    }

    FIPS202_SHAKE256(hash_msg, (unsigned int)absorb_len_bytes, digest,
                     (int)digest_len_bytes);
    free(aligned_msg);
    return 0;
}