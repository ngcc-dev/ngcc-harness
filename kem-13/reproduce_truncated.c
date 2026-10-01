#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "KEM_AlgorithmInstance.h"
#include "drng.h"

extern DRNG_ctx drng_algorithm;

int main(void) {
    unsigned char seed[55] = {0};
    unsigned long long pk_len = kem_get_pk_len_bytes();
    unsigned long long sk_len = kem_get_sk_len_bytes();
    unsigned long long ct_len = kem_get_ct_len_bytes();
    unsigned long long ss_len = kem_get_ss_len_bytes();
    unsigned long long out_len = 0;
    unsigned char *pk = malloc(pk_len), *sk = malloc(sk_len);
    unsigned char *ct = malloc(ct_len), *short_ct = malloc(ct_len - 1);
    unsigned char *ss = malloc(ss_len), *ss2 = malloc(ss_len);
    if (!pk || !sk || !ct || !short_ct || !ss || !ss2) return 2;
    if (init_random_number(&drng_algorithm, seed, sizeof seed)) return 3;
    if (kem_keygen(pk, &out_len, sk, &out_len)) return 4;
    if (kem_enc(pk, pk_len, ss, &out_len, ct, &out_len)) return 5;
    memcpy(short_ct, ct, ct_len - 1);
    return kem_dec(sk, sk_len, short_ct, ct_len - 1, ss2, &out_len);
}
