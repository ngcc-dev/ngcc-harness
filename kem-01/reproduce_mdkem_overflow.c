#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "api.h"

void randombytes(unsigned char *out, unsigned long long len) {
    for (unsigned long long i = 0; i < len; i++) out[i] = (unsigned char)(i * 7 + 1);
}

int main(void) {
    uint8_t pk[PK_BYTES], sk[SK_BYTES], rnd[SEED_BYTES];
    uint8_t ct[CT_BYTES], det_ss[KEM_BYTES], dec_ss[KEM_BYTES];
    uint8_t random_ct[CT_BYTES], random_ss[KEM_BYTES], random_dec_ss[KEM_BYTES];
    memset(rnd, 0x42, sizeof rnd);
    if (mkem_keygen(pk, sk) != 0) return 2;
    if (mdkem_enc(pk, rnd, det_ss, ct) != 0) return 3;
    if (mkem_dec(sk, ct, dec_ss) != 0) return 4;
    if (mkem_enc(pk, random_ss, random_ct) != 0) return 5;
    if (mkem_dec(sk, random_ct, random_dec_ss) != 0) return 6;
    printf("deterministic_match=%d random_match=%d\n",
           memcmp(det_ss, dec_ss, KEM_BYTES) == 0,
           memcmp(random_ss, random_dec_ss, KEM_BYTES) == 0);
    return 0;
}
