/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "CryptHash_AlgorithmInstance.h"
#include "mastercube.h"

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
    if (digest_len_bits != DIGEST_BIT_LENGTH)
    {
        return 1;
    }

#if DIGEST_BIT_LENGTH == 512
    return MasterCube512(msg, msg_len_bits, digest);
#elif DIGEST_BIT_LENGTH == 768
    return MasterCube768(msg, msg_len_bits, digest);
#elif DIGEST_BIT_LENGTH == 1024
    return MasterCube1024(msg, msg_len_bits, digest);
#else
    return 1;
#endif
}