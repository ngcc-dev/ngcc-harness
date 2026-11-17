/* hash-07-2: a fixed-output CryptHash ignores digest_len_bits.
 *
 * Calls the submitted CryptHash with a requested length shorter than the
 * instance's digest, in a buffer that is large enough and pre-filled with a
 * sentinel. Reports the return code, how many bytes were written, and whether
 * they equal the full-length digest. No out-of-bounds write is performed here.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int CryptHash(int digest_len_bits, const unsigned char *msg,
              unsigned long long msg_len_bits, unsigned char *digest);

int main(int argc, char **argv)
{
    int full_bits = atoi(argv[1]), req_bits = atoi(argv[2]);
    unsigned char msg[3] = { 'a', 'b', 'c' }, out[512], ref[512];
    int rc, i, written = 0;

    memset(ref, 0x5A, sizeof ref);
    if (CryptHash(full_bits, msg, 24, ref) != 0) return 2;
    memset(out, 0x5A, sizeof out);
    rc = CryptHash(req_bits, msg, 24, out);
    for (i = 0; i < (int)sizeof out; i++)
        if (out[i] != 0x5A) written = i + 1;
    printf("requested=%d rc=%d written_bytes=%d equals_full=%d\n", req_bits, rc,
           written, memcmp(out, ref, sizeof out) == 0);
    return 0;
}
