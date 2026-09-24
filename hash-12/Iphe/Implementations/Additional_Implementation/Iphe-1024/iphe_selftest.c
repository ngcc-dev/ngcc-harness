#include "CryptHash_AlgorithmInstance.h"
#include <stdio.h>
#include <string.h>

static void fill(unsigned char *buf, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++)
        buf[i] = (unsigned char)(11U + 37U * i);
}

static int run_case(const unsigned char *msg, unsigned long long bits, const char *name)
{
    unsigned char a[DIGEST_BIT_LENGTH / 8], b[DIGEST_BIT_LENGTH / 8];
    memset(a, 0, sizeof(a));
    memset(b, 0xff, sizeof(b));
    if (CryptHash(DIGEST_BIT_LENGTH, msg, bits, a) != 0 ||
        CryptHash(DIGEST_BIT_LENGTH, msg, bits, b) != 0 ||
        memcmp(a, b, sizeof(a)) != 0) {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    static const unsigned char abc[] = {'a', 'b', 'c'};
    static const unsigned char short_msg[] = {0x80, 0x7f, 0x01, 0xfe};
    unsigned char long_msg[4096], bit_msg[2] = {0xb6, 0x80}, out[DIGEST_BIT_LENGTH / 8];
    int fail = 0;

    fill(long_msg, sizeof(long_msg));
    fail |= run_case(0, 0, "empty");
    fail |= run_case(abc, 24, "abc");
    fail |= run_case(short_msg, 29, "short");
    fail |= run_case(long_msg, (unsigned long long)sizeof(long_msg) * 8ULL, "long");
    fail |= run_case(bit_msg, 1, "1-bit");
    fail |= run_case(bit_msg, 7, "7-bit");
    fail |= run_case(bit_msg, 9, "9-bit");
    fail |= run_case(bit_msg, 15, "15-bit");

    if (CryptHash(DIGEST_BIT_LENGTH == 512 ? 768 : 512, abc, 24, out) == 0) {
        puts("FAIL invalid digest length");
        fail = 1;
    }
    puts(fail ? "FAIL" : "PASS");
    return fail;
}
