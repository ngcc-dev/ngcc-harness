#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "KEM_AlgorithmInstance.h"
#include "symmetric.h"
#ifdef HARE_SYMMETRIC_MODE_B
#include "drng.h"
DRNG_ctx drng_algorithm;
#endif
/**
 * Fill deterministic DRNG seed material for the tamper test.
 */
static void fill_seed_material(uint8_t *a, size_t alen, uint8_t *b, size_t blen) {
    for (size_t i = 0; i < alen; ++i) a[i] = (uint8_t)(0x33u + i);
    for (size_t i = 0; i < blen; ++i) b[i] = (uint8_t)(0x77u ^ (uint8_t)i);
}
/**
 * Verify that modified ciphertexts trigger FO implicit rejection behavior.
 */
int main(void){
    unsigned long long pk_len=kem_get_pk_len_bytes(), sk_len=kem_get_sk_len_bytes(), ct_len=kem_get_ct_len_bytes(), ss_len=kem_get_ss_len_bytes();
    unsigned char *pk=calloc(pk_len,1),*sk=calloc(sk_len,1),*ct=calloc(ct_len,1),*ss=calloc(ss_len,1),*ss2=calloc(ss_len,1),*ss3=calloc(ss_len,1);
    uint8_t seed[48], pers[48];
    if(!pk||!sk||!ct||!ss||!ss2||!ss3) return 2;
    fill_seed_material(seed,sizeof(seed),pers,sizeof(pers));
    prng_init(seed,pers,48,48);
#ifdef HARE_SYMMETRIC_MODE_B
    if (init_random_number(&drng_algorithm, seed, sizeof(seed)) != 0) return 7;
#endif
    if(kem_keygen(pk,&pk_len,sk,&sk_len)!=0) return 3;
    if(kem_enc(pk,pk_len,ss,&ss_len,ct,&ct_len)!=0) return 4;
    ct[0]^=1;
    if (kem_dec(sk,sk_len,ct,ct_len,ss2,&ss_len)!=-1) return 5;
    if (kem_dec(sk,sk_len,ct,ct_len,ss3,&ss_len)!=-1) return 6;
    if(memcmp(ss2,ss3,ss_len)!=0) return 8;
    puts("tamper_pass");
    free(pk); free(sk); free(ct); free(ss); free(ss2); free(ss3);
    return 0;
}
