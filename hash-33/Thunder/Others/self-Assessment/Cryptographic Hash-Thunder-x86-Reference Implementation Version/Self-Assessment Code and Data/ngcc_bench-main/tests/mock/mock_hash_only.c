/**
 * Minimal mock .so that exports ONLY the CryptHash symbol.
 * Used to verify that --test hash works without requiring sig/kem/kex symbols.
 */
#include <string.h>

int CryptHash(int digest_len_bits,
              const unsigned char *msg,
              unsigned long long msg_len_bits,
              unsigned char *digest) {
    unsigned long long msg_len = msg_len_bits / 8ULL;
    unsigned char acc = (unsigned char) digest_len_bits;
    unsigned long long i;

    for (i = 0; i < msg_len; ++i) {
        acc ^= msg[i];
    }
    digest[0] = acc;
    return 0;
}
