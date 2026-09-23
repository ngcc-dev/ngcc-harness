/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "CryptHash_Garnet.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#define STATE_4x4 16

extern int CryptHash_512(const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);
extern int CryptHash_768(const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);
extern int CryptHash_1024(const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);


int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
    if (digest_len_bits == 512) {
        return CryptHash_512(msg, msg_len_bits, digest);
    }
    else
    if (digest_len_bits == 768) {
        return CryptHash_768(msg, msg_len_bits, digest);
    }
    else if (digest_len_bits == 1024) {
        return CryptHash_1024(msg, msg_len_bits, digest);
    }
    else {
        fprintf(stderr, "[Garnet-768 Adapter] Error: Unsupported digest length %d bits. Only 768 bits is supported.\n", digest_len_bits);
        return -1; 
    }
}