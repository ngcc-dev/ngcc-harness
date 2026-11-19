/* hash-02: digests of four byte-aligned messages and one partial-byte message.
 *
 * Build with -DDIG=512, 768 or 1024 against one AXIS instance directory.
 * Prints one line per message: "<label> <digest hex>". The partial-byte
 * message takes a different code path and serves as a control.
 */
#include <stdio.h>
#include <string.h>

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

int main(void)
{
    static unsigned char m[1024];
    unsigned char d[DIG / 8];
    const char *label[5] = { "empty", "abc", "zero512", "pattern8192", "pattern8191" };
    unsigned long long len[5] = { 0, 24, 512, 8192, 8191 };
    int i, k;

    for (k = 0; k < 5; k++) {
        memset(m, 0, sizeof m);
        if (k == 1) memcpy(m, "abc", 3);
        if (k >= 3) for (i = 0; i < (int)sizeof m; i++) m[i] = (unsigned char)(i * 167 + 13);
        if (CryptHash(DIG, m, len[k], d) != 0) return 1;
        printf("%s ", label[k]);
        for (i = 0; i < DIG / 8; i++) printf("%02x", d[i]);
        printf("\n");
    }
    return 0;
}
