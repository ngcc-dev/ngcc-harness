/* kem-12-3: run the archived CTL-512 ctl_encrypt_3329 once on a binary s of
 * the given Hamming weight (argv[1]), with a fixed public h. The driver's own
 * work does not depend on the weight. Prints the weight, the return value and
 * a digest of the ciphertext. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint32_t ctl_encrypt_3329(int16_t *c, const uint8_t *sbuf, const uint16_t *h,
                          unsigned logn, uint32_t *tmp);

int main(int argc, char **argv)
{
    static uint16_t h[2048];
    static int16_t c[2048];
    static uint32_t tmp[65536];
    uint8_t sbuf[256] = {0};
    unsigned w = argc > 1 ? (unsigned)atoi(argv[1]) : 0, x = 1, sum = 0;

    for (unsigned i = 0; i < 2048; i++) {
        x = x * 1103515245u + 12345u;
        h[i] = (uint16_t)((x >> 8) % 3329);
        sbuf[i >> 3] |= (uint8_t)((uint32_t)(i - w) >> 31 << (i & 7));
    }
    uint32_t ok = ctl_encrypt_3329(c, sbuf, h, 11, tmp);
    for (unsigned i = 0; i < 2048; i++)
        sum = sum * 31 + (uint16_t)c[i];
    printf("%u %u %08x\n", w, ok, sum);
    return 0;
}
