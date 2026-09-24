#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../hash_sm3.h"

static void fill_input(unsigned char *buf, unsigned long len, unsigned int seed)
{
    for (unsigned long i = 0; i < len; i++) {
        buf[i] = (unsigned char)((seed + 13U * (unsigned int)i +
                                  (unsigned int)(i >> 1)) & 0xffU);
    }
}

static int check_case(unsigned long inlen, unsigned long outlen)
{
    unsigned char in0[160], in1[160], in2[160], in3[160];
    unsigned char scalar0[128], scalar1[128], scalar2[128], scalar3[128];
    unsigned char x4_0[128], x4_1[128], x4_2[128], x4_3[128];

    fill_input(in0, inlen, 0x11);
    fill_input(in1, inlen, 0x37);
    fill_input(in2, inlen, 0x59);
    fill_input(in3, inlen, 0x83);

    memset(scalar0, 0xa5, sizeof(scalar0));
    memset(scalar1, 0xa5, sizeof(scalar1));
    memset(scalar2, 0xa5, sizeof(scalar2));
    memset(scalar3, 0xa5, sizeof(scalar3));
    memset(x4_0, 0x5a, sizeof(x4_0));
    memset(x4_1, 0x5a, sizeof(x4_1));
    memset(x4_2, 0x5a, sizeof(x4_2));
    memset(x4_3, 0x5a, sizeof(x4_3));

    sm3_xof(scalar0, outlen, in0, inlen);
    sm3_xof(scalar1, outlen, in1, inlen);
    sm3_xof(scalar2, outlen, in2, inlen);
    sm3_xof(scalar3, outlen, in3, inlen);

    sm3_xofx4(x4_0, x4_1, x4_2, x4_3, outlen,
              in0, in1, in2, in3, inlen);

    if (memcmp(scalar0, x4_0, outlen) != 0 ||
        memcmp(scalar1, x4_1, outlen) != 0 ||
        memcmp(scalar2, x4_2, outlen) != 0 ||
        memcmp(scalar3, x4_3, outlen) != 0) {
        printf("sm3_xofx4 mismatch: inlen=%lu outlen=%lu\n", inlen, outlen);
        return -1;
    }

    return 0;
}

int main(void)
{
    static const unsigned long inlens[] = {
        0, 1, 31, 32, 33, 51, 52, 53, 55, 56, 57,
        63, 64, 65, 119, 120, 121
    };
    static const unsigned long outlens[] = {
        1, 16, 31, 32, 33, 63, 64, 65, 96
    };

    for (unsigned int i = 0; i < sizeof(inlens) / sizeof(inlens[0]); i++) {
        for (unsigned int j = 0; j < sizeof(outlens) / sizeof(outlens[0]); j++) {
            if (check_case(inlens[i], outlens[j]) != 0) {
                return 1;
            }
        }
    }

    printf("sm3_xofx4 scalar-equivalence tests passed\n");
    return 0;
}
