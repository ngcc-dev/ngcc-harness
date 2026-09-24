#include <stdio.h>
#include <string.h>

#ifndef DIGEST_BIT_LENGTH
#error DIGEST_BIT_LENGTH must be provided by CryptHash_AlgorithmInstance.h
#endif

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest);

static void fill(unsigned char *buf, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++)
        buf[i] = (unsigned char)(19U + 23U * i + (unsigned)(i >> 2));
}

int main(void)
{
    static const unsigned long long rate_bits = 1472ULL;
    static const unsigned long long cases[] = {
        0ULL, 1ULL, 7ULL, 9ULL, 15ULL,
        rate_bits - 1ULL, rate_bits, rate_bits + 1ULL,
        2ULL * rate_bits, 2ULL * rate_bits + 1ULL,
        4096ULL * 8ULL, 65536ULL * 8ULL
    };
    unsigned char msg[65536];
    unsigned char digest[DIGEST_BIT_LENGTH / 8];
    unsigned i, j;

    if (DIGEST_BIT_LENGTH != 512) {
        puts("FAIL this boundary test is for Iphe-512 only");
        return 1;
    }

    fill(msg, sizeof(msg));
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        if (CryptHash(DIGEST_BIT_LENGTH, msg, cases[i], digest) != 0) {
            printf("FAIL bits=%llu\n", cases[i]);
            return 1;
        }
        printf("%llu ", cases[i]);
        for (j = 0; j < sizeof(digest); j++)
            printf("%02x", digest[j]);
        putchar('\n');
    }
    return 0;
}
