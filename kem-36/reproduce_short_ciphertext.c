/* kem-36-11: kem_dec ignores ct_len_bytes and reads a fixed-size ciphertext_t.
 * The driver passes a heap buffer of exactly ct_len_bytes = |ct| - 1 bytes
 * holding a prefix of an honest ciphertext (built with -fsanitize=address).
 * Control: the same call with a full-length heap buffer. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drng.h"
#include "KEM_AlgorithmInstance.h"
#include "trike_types.h"
DRNG_ctx drng_algorithm;
int main(int argc, char **argv)
{
    static uint8_t pk[sizeof(public_key_t)], sk[sizeof(secret_key_t)], ct[sizeof(ciphertext_t)], ss[64], ssd[64];
    uint8_t seed[48] = {1};
    unsigned long long a, b, n = sizeof(ciphertext_t) - (argc > 1 ? 1 : 0);
    init_random_number(&drng_algorithm, seed, 48);
    kem_keygen(pk, &a, sk, &b);
    kem_enc(pk, sizeof pk, ss, &a, ct, &b);
    uint8_t *buf = malloc(n);
    memcpy(buf, ct, n);
    printf("calling kem_dec with a %llu-byte heap ciphertext (ct_len_bytes=%llu, expected %zu)\n", n, n, sizeof(ciphertext_t));
    int rc = kem_dec(sk, sizeof sk, buf, n, ssd, &a);
    printf("kem_dec returned %d, shared secrets match=%d\n", rc, !memcmp(ss, ssd, sizeof(shared_secret_t)));
    free(buf);
    return 0;
}
