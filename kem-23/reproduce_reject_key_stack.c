/* kem-23-4: hash_j hashes a buffer sized by sizeof(ciphertext_kem_t) but
 * fills only the serialized ciphertext. Prints the buffer size, the bytes
 * written and the gap, then decapsulates one invalid ciphertext twice after
 * filling the stack with different bytes and reports whether the two
 * implicit-rejection keys agree. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parameters.h"
#include "data_structures.h"
#include "KEM_AlgorithmInstance.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

__attribute__((noinline)) static void dirty_stack(int c)
{
    volatile unsigned char buf[1 << 20];
    for (size_t i = 0; i < sizeof buf; i++) buf[i] = (unsigned char)c;
}

int main(void)
{
    unsigned char seed[64], ss[64], a[64], b[64], c[64], d[64];
    unsigned long long pkl = kem_get_pk_len_bytes(), skl = kem_get_sk_len_bytes();
    unsigned long long ssl = kem_get_ss_len_bytes(), ctl = kem_get_ct_len_bytes();
    unsigned char *pk = calloc(pkl, 1), *sk = calloc(skl, 1), *ct = calloc(ctl, 1);
    size_t size = SEED_BYTES + PARAM_K_BYTES + sizeof(ciphertext_kem_t);
    size_t filled = SEED_BYTES + PARAM_K_BYTES + (size_t)PARAM_L * VEC_N_SIZE_BYTES +
                    VEC_N1N2_SIZE_BYTES + SALT_BYTES;

    memset(seed, 3, sizeof seed);
    init_random_number(&drng_algorithm, seed, 64);
    kem_keygen(pk, &pkl, sk, &skl);
    kem_enc(pk, pkl, ss, &ssl, ct, &ctl);
    dirty_stack(0x11); kem_dec(sk, skl, ct, ctl, a, &ssl);
    dirty_stack(0x22); kem_dec(sk, skl, ct, ctl, b, &ssl);
    ct[5] ^= 1;
    dirty_stack(0x11); kem_dec(sk, skl, ct, ctl, c, &ssl);
    dirty_stack(0x22); kem_dec(sk, skl, ct, ctl, d, &ssl);
    printf("hashed=%zu written=%zu gap=%zu valid_stable=%d reject_stable=%d\n", size, filled,
           size - filled, !memcmp(a, b, ssl) && !memcmp(a, ss, ssl), !memcmp(c, d, ssl));
    return 0;
}
