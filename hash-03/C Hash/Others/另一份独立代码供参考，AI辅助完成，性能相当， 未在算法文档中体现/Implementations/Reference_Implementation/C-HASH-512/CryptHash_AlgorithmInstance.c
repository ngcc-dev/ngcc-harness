/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).
*/

#include "CryptHash_AlgorithmInstance.h"
#include "c-hash.h"

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
    if (digest_len_bits != 512) return -1;
    return c_hash_512_bits(msg, msg_len_bits, digest);
}
