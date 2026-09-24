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
#ifndef HARE_INSTANCE_NAME
#define HARE_INSTANCE_NAME "unknown"
#endif
#ifndef HARE_KEM_LOOP_ROUNDS
#define HARE_KEM_LOOP_ROUNDS 100
#endif
/**
 * Fill deterministic DRNG seed material for one KEM loop round.
 */
static void fill_seed_material(uint8_t *a, size_t alen, uint8_t *b, size_t blen, unsigned round) {
    for (size_t i = 0; i < alen; ++i) a[i] = (uint8_t)(0x11u + round + i);
    for (size_t i = 0; i < blen; ++i) b[i] = (uint8_t)(0xA5u ^ (uint8_t)(round + i));
}
/**
 * Run deterministic keygen/encaps/decaps loops and check shared-secret equality.
 */
int main(void){
    unsigned long long pk_len=kem_get_pk_len_bytes(), sk_len=kem_get_sk_len_bytes(), ct_len=kem_get_ct_len_bytes(), ss_len=kem_get_ss_len_bytes();
    unsigned char *pk=calloc(pk_len,1),*sk=calloc(sk_len,1),*ct=calloc(ct_len,1),*ss=calloc(ss_len,1),*ss2=calloc(ss_len,1);
    if(!pk||!sk||!ct||!ss||!ss2) return 2;
    for(int i=0;i<HARE_KEM_LOOP_ROUNDS;i++){
        uint8_t seed[48], pers[48];
        fill_seed_material(seed, sizeof(seed), pers, sizeof(pers), (unsigned)i);
        prng_init(seed, pers, 48, 48);
#ifdef HARE_SYMMETRIC_MODE_B
        if (init_random_number(&drng_algorithm, seed, sizeof(seed)) != 0) return 7;
#endif
        if(kem_keygen(pk,&pk_len,sk,&sk_len)!=0) return 3;
        if(kem_enc(pk,pk_len,ss,&ss_len,ct,&ct_len)!=0) return 4;
        if(kem_dec(sk,sk_len,ct,ct_len,ss2,&ss_len)!=0) return 5;
        if(memcmp(ss,ss2,ss_len)!=0) return 6;
    }
    printf("kem_loop_pass %s\n", HARE_INSTANCE_NAME);
    free(pk); free(sk); free(ct); free(ss); free(ss2);
    return 0;
}
