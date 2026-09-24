#ifndef SM3X4_H
#define SM3X4_H

#include <stdint.h>

void sm3_bit_4x(const unsigned char *msg0,
                const unsigned char *msg1,
                const unsigned char *msg2,
                const unsigned char *msg3,
                unsigned long long msg_bitlen,
                unsigned char *dgst0,
                unsigned char *dgst1,
                unsigned char *dgst2,
                unsigned char *dgst3);

#endif
