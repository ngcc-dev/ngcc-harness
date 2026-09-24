#include "CryptHash_AlgorithmInstance.h"
#include "wchain_c.h"

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest) {
    if (digest_len_bits != DIGEST_BIT_LENGTH) return -1;
    return wchain_crypt_hash_c(digest_len_bits, msg, msg_len_bits, digest);
}
