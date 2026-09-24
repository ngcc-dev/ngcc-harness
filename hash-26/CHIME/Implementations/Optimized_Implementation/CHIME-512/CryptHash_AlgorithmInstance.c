/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include <string.h>
#include "CryptHash_AlgorithmInstance.h"
#include "chime512.hpp"

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
#if DIGEST_BIT_LENGTH == 512
    if (digest_len_bits != 512) {
        return -1;
    }
    return chime_512_bit_padding(msg, msg_len_bits, digest);
#elif DIGEST_BIT_LENGTH == 1024
    if (digest_len_bits != 1024) {
        return -1;
    }
    return chime_1024_bit_padding(msg, msg_len_bits, digest);
#endif
}
