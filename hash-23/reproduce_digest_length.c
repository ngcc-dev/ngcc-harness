/* hash-23-1: hash a fixed message with a caller-chosen digest_len_bits. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int CryptHash(int, const unsigned char *, unsigned long long, unsigned char *);

int main(int argc, char **argv)
{
    int bits = argc > 1 ? atoi(argv[1]) : 512;
    unsigned long long msg_bits = argc > 2 ? strtoull(argv[2], NULL, 10) : 2623;
    unsigned char *msg = malloc(msg_bits / 8 + 1), out[256];
    unsigned long long i;
    int rc;

    if (!msg)
        return 2;
    for (i = 0; i < msg_bits / 8 + 1; i++)
        msg[i] = (unsigned char)(31 * i + 7);
    memset(out, 0xa5, sizeof out);
    rc = CryptHash(bits, msg, msg_bits, out);
    printf("digest_len_bits=%d rc=%d out[0..3]=%02x%02x%02x%02x\n",
           bits, rc, out[0], out[1], out[2], out[3]);
    free(msg);
    return 0;
}
