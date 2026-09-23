#include <stdio.h>
#include <string.h>
#include "CryptHash_AlgorithmInstance.h"

/* Function print_hex: prints a byte string in hexadecimal. */
static void print_hex(const unsigned char *p, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        printf("%02X", p[i]);
    }
}

/* Function main: executes this standalone test, benchmark, or utility program. */
int main(void)
{
    unsigned char digest[DIGEST_BIT_LENGTH / 8];
    const unsigned char empty[1] = {0};
    const unsigned char abc[3] = {'a', 'b', 'c'};
    const unsigned char msb11[2] = {0xAC, 0x20}; /* 10101100001, MSB-first */

    if (CryptHash(DIGEST_BIT_LENGTH, empty, 0, digest) != 0) return 1;
    printf("%s empty = ", ALGORITHM_INSTANCE);
    print_hex(digest, DIGEST_BIT_LENGTH / 8);
    printf("\n");

    if (CryptHash(DIGEST_BIT_LENGTH, abc, 24, digest) != 0) return 1;
    printf("%s abc = ", ALGORITHM_INSTANCE);
    print_hex(digest, DIGEST_BIT_LENGTH / 8);
    printf("\n");

    if (CryptHash(DIGEST_BIT_LENGTH, msb11, 11, digest) != 0) return 1;
    printf("%s 11-bit-AC20 = ", ALGORITHM_INSTANCE);
    print_hex(digest, DIGEST_BIT_LENGTH / 8);
    printf("\n");

    return 0;
}
