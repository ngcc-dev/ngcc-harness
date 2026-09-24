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
        buf[i] = (unsigned char)(13U + 29U * i + (unsigned)(i >> 3));
}

static int run_case(const char *name, const unsigned char *msg, unsigned long long bits)
{
    unsigned char digest[DIGEST_BIT_LENGTH / 8];
    unsigned i;

    if (CryptHash(DIGEST_BIT_LENGTH, msg, bits, digest) != 0) {
        printf("ERROR %s\n", name);
        return 1;
    }

    printf("%s %llu ", name, bits);
    for (i = 0; i < sizeof(digest); i++)
        printf("%02x", digest[i]);
    putchar('\n');
    return 0;
}

int main(void)
{
    static const unsigned char abc[] = {'a', 'b', 'c'};
    static const unsigned char short_msg[] = {0x80, 0x7f, 0x01, 0xfe};
    unsigned char msg[65536];
    unsigned char bit_msg[2] = {0xb6, 0x80};
    int fail = 0;

    fill(msg, sizeof(msg));
    fail |= run_case("empty", 0, 0);
    fail |= run_case("abc", abc, 24);
    fail |= run_case("short", short_msg, 29);
    fail |= run_case("long", msg, 4096ULL * 8ULL);
    fail |= run_case("32B", msg, 32ULL * 8ULL);
    fail |= run_case("128B", msg, 128ULL * 8ULL);
    fail |= run_case("512B", msg, 512ULL * 8ULL);
    fail |= run_case("1024B", msg, 1024ULL * 8ULL);
    fail |= run_case("4096B", msg, 4096ULL * 8ULL);
    fail |= run_case("8192B", msg, 8192ULL * 8ULL);
    fail |= run_case("16384B", msg, 16384ULL * 8ULL);
    fail |= run_case("65536B", msg, 65536ULL * 8ULL);
    fail |= run_case("1bit", bit_msg, 1);
    fail |= run_case("7bit", bit_msg, 7);
    fail |= run_case("9bit", bit_msg, 9);
    fail |= run_case("15bit", bit_msg, 15);
    return fail;
}
