/*
Eijen adapter for the ICCS CryptHash API.
*/

#include "CryptHash_AlgorithmInstance.h"
#include "eijen.h"

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest)
{
    if (digest == 0) {
        return -1;
    }
    if (digest_len_bits != DIGEST_BIT_LENGTH) {
        return -1;
    }
    if (msg == 0 && msg_len_bits != 0ULL) {
        return -1;
    }
    return eijen_hash_bits(digest_len_bits, msg, msg_len_bits, digest);
}
