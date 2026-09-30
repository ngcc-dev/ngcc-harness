/* Deliberately give kem_dec a one-byte-short allocation and its true length.
 * AddressSanitizer must catch the implementation's hard-sized read. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "KEM_lwekem128.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

int main(void)
{
    unsigned long long pk_len = kem_get_pk_len_bytes();
    unsigned long long sk_len = kem_get_sk_len_bytes();
    unsigned long long ct_len = kem_get_ct_len_bytes();
    unsigned long long ss_len = kem_get_ss_len_bytes();
    unsigned long long out_len = 0;
    unsigned char seed[64];
    unsigned char *pk = malloc(pk_len);
    unsigned char *sk = malloc(sk_len);
    unsigned char *ct = malloc(ct_len);
    unsigned char *short_ct = malloc(ct_len - 1);
    unsigned char *ss = malloc(ss_len);
    unsigned char *ss2 = malloc(ss_len);

    for (int i = 0; i < 64; i++)
        seed[i] = (unsigned char)(i + 1);
    init_random_number(&drng_algorithm, seed, sizeof seed);
    kem_keygen(pk, &out_len, sk, &out_len);
    kem_enc(pk, pk_len, ss, &out_len, ct, &out_len);
    memcpy(short_ct, ct, ct_len - 1);

    fprintf(stderr, "calling kem_dec with %llu allocated bytes and declared length %llu\n",
            ct_len - 1, ct_len - 1);
    kem_dec(sk, sk_len, short_ct, ct_len - 1, ss2, &out_len);
    return 0;
}
