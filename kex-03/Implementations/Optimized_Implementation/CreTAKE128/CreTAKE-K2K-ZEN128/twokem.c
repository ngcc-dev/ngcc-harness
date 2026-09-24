#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "twokem.h"
#include "primitive_interfaces.h"
#include "cretake_params.h"
#include "drng.h"
#include "auxfunc.h"

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

int twokem_keygen1(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    kem_keygen(pk, pk_len_bytes, sk, sk_len_bytes);

    return 0;
}

int twokem_keygen2(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    kem_keygen(pk, pk_len_bytes, sk, sk_len_bytes);

    return 0;
}

int twokem_keygen2_withseed(
    const unsigned char *seed, unsigned long long seed_len_bytes,
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    if(seed == NULL || seed_len_bytes != SEED_BYTES){
        return -1;
    }

    pke_keygen_derand(pk, sk, seed);
    *pk_len_bytes = TPK_LEN;
    *sk_len_bytes = TSK_LEN;

    return 0;
}



int twokem_enc(
    unsigned char *pk1, unsigned long long pk1_len_bytes,
    unsigned char *pk2, unsigned long long pk2_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes,
    unsigned char *ct, unsigned long long *ct_len_bytes)
{

    unsigned char m[MSG_LEN_BYTES];
    unsigned char seed_enc[SEED_BYTES];
    unsigned char *c1 = ct;
    unsigned char *c2 = ct + ZEN_CIPHERTEXT_LEN_BYTES;
    unsigned long long c1_len_bytes=0, ss1_len_bytes=0;
    unsigned char ss1[KEM_K_LEN];
    unsigned char buf[SEED_BYTES], buf2[MSG_LEN_BYTES + SEED_BYTES];
    unsigned char buf3[PKI_LEN + KEM_K_LEN + MSG_LEN_BYTES];

    if (pk1 == NULL || ss == NULL || ss_len_bytes == NULL || pk2 == NULL || ct == NULL || ct_len_bytes == NULL) {
        return -1;
    }

    if (pk1_len_bytes != kem_get_pk_len_bytes()) {
        return -2;
    }

    if (pk2_len_bytes != kem_get_pk_len_bytes()) {
        return -2;
    }
    

    //*ss_len_bytes = kem_get_ss_len_bytes();
    //*ct_len_bytes = kem_get_ct_len_bytes();

    if(kem_enc(pk1, pk1_len_bytes, ss1, &ss1_len_bytes, c1, &c1_len_bytes) != 0){
        return -1;
    }

    if (get_random_number(&drng_algorithm, buf, SEED_BYTES * 8ULL) != 0) {
        return -1;
    }
	pseudoXOF((MSG_LEN_BYTES + SEED_BYTES) * 8, buf, SEED_BYTES, buf2);
	memcpy(m, buf2, MSG_LEN_BYTES);
	memcpy(seed_enc, buf2 + MSG_LEN_BYTES, SEED_BYTES);
	
	pke_enc(pk2, m, seed_enc, c2);
    
    *ct_len_bytes = c1_len_bytes + ZEN_INDCPA_CIPHERTEXT_LEN_BYTES;

    memcpy(buf3, pk1, pk1_len_bytes);
    memcpy(buf3 + pk1_len_bytes, ss1, ss1_len_bytes);
    memcpy(buf3 + pk1_len_bytes + ss1_len_bytes, m, MSG_LEN_BYTES);

    pseudoXOF(KEM_K_LEN*8, buf3, sizeof(buf3)*8, ss);

    *ss_len_bytes = KEM_K_LEN;

    return 0;
}

int twokem_dec(
	unsigned char *sk1, unsigned long long sk1_len_bytes,
    unsigned char *sk2, unsigned long long sk2_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
    unsigned char ss1[KEM_K_LEN];
    unsigned char *c1 = ct;
    unsigned char *c2 = ct + ZEN_CIPHERTEXT_LEN_BYTES;
    unsigned long long ss1_len_bytes=0;
    unsigned char m[MSG_LEN_BYTES];
    const unsigned char *pk1 = sk1 + TSK_LEN;
    unsigned char buf3[PKI_LEN + KEM_K_LEN + MSG_LEN_BYTES];
    (void)sk2_len_bytes;(void)ct_len_bytes;

    if(kem_dec(sk1, sk1_len_bytes, c1, ZEN_CIPHERTEXT_LEN_BYTES, ss1, &ss1_len_bytes) != 0){
        return -1;
    }

    pke_dec(sk2, c2, m);

    memcpy(buf3, pk1, PKI_LEN);
    memcpy(buf3 + PKI_LEN, ss1, ss1_len_bytes);
    memcpy(buf3 + PKI_LEN + ss1_len_bytes, m, MSG_LEN_BYTES);

    pseudoXOF(KEM_K_LEN*8, buf3, sizeof(buf3)*8, ss);
    *ss_len_bytes = KEM_K_LEN;

    return 0;

}   

