/* hash-19-3: the fixed-output MoFang CryptHash wrappers ignore digest_len_bits.
 *
 * Calls the submitted CryptHash with a requested length other than the
 * instance's digest, in a buffer that is large enough and pre-filled with a
 * sentinel. Two different sentinels (0x5A, 0xA5) are used, so a written byte
 * that happens to equal one sentinel is still counted. Reports the return
 * code, how many bytes were written, and whether they equal the full-length
 * digest. No out-of-bounds write is performed here.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest);

int main(int argc, char **argv)
{
    int full_bits = atoi(argv[1]), req_bits = atoi(argv[2]);
    unsigned char msg[3] = { 'a', 'b', 'c' }, out1[512], out2[512], ref[512];
    int rc, rc2, i, written = 0;

    memset(ref, 0x5A, sizeof ref);
    if (CryptHash(full_bits, msg, 24, ref) != 0) return 2;
    memset(out1, 0x5A, sizeof out1);
    rc = CryptHash(req_bits, msg, 24, out1);
    memset(out2, 0xA5, sizeof out2);
    rc2 = CryptHash(req_bits, msg, 24, out2);
    if (rc2 != rc) return 3;
    for (i = 0; i < (int)sizeof out1; i++)
        if (out1[i] != 0x5A || out2[i] != 0xA5) written = i + 1;
    printf("requested=%d rc=%d written_bytes=%d equals_full=%d\n", req_bits, rc,
           written, written == full_bits / 8 && memcmp(out1, ref, written) == 0 &&
           memcmp(out2, ref, written) == 0);
    return 0;
}
