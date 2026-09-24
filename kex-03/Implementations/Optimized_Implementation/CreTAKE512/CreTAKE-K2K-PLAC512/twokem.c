#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "twokem.h"
#include "twopke.h"
#include "primitive_interfaces.h"
#include "cretake_params.h"
#include "drng.h"
#include "auxfunc.h"

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

static uint8_t ct_verify(const uint8_t *a, const uint8_t *b, size_t len)
{
    uint8_t r = 0;

    for (size_t i = 0; i < len; i++) {
        r |= (uint8_t)(a[i] ^ b[i]);
    }

    return (uint8_t)((-(uint64_t)r) >> 63);
}

static void ct_cmov(uint8_t *r, const uint8_t *x, size_t len, uint8_t b)
{
    b = (uint8_t)-b;
    for (size_t i = 0; i < len; i++) {
        r[i] ^= (uint8_t)(b & (r[i] ^ x[i]));
    }
}

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

    if (PKE_KeyGen(pk, sk, seed) != 0) {
        return -1;
    }
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

    unsigned char m[PKE_MESSAGE_BYTES];
    unsigned char seed_enc1[SEED_BYTES], seed_enc2[SEED_BYTES];
    unsigned char buf[2*PKE_PUBLIC_KEY_BYTES], buf2[SEED_BYTES];
    unsigned char buf3[PKE_MESSAGE_BYTES + SEED_BYTES], buf4[2*SEED_BYTES+SS_KEY_BYTES];

    if (pk1 == NULL || ss == NULL || ss_len_bytes == NULL || pk2 == NULL || ct == NULL || ct_len_bytes == NULL) {
        return -1;
    }

    if (pk1_len_bytes != kem_get_pk_len_bytes()) {
        return -2;
    }

    if (pk2_len_bytes != kem_get_pk_len_bytes()) {
        return -2;
    }

    if (get_random_number(&drng_algorithm, m, PKE_MESSAGE_BYTES * 8ULL) != 0) {
        return -1;
    }

    memcpy(buf, pk1, PKE_PUBLIC_KEY_BYTES);
    memcpy(buf + PKE_PUBLIC_KEY_BYTES, pk2, PKE_PUBLIC_KEY_BYTES);

    if(pseudohash(SEED_BYTES*8, buf, sizeof(buf), buf2) != 0){
        return -1;
    }
    memcpy(buf3, m, PKE_MESSAGE_BYTES);
    memcpy(buf3 + PKE_MESSAGE_BYTES, buf2, sizeof(buf2));
    if(pseudoXOF((2*SEED_BYTES+SS_KEY_BYTES)*8, buf3, sizeof(buf3), buf4) != 0){
        return -1;
    }

    memcpy(seed_enc1, buf4, SEED_BYTES);
    memcpy(seed_enc2, buf4 + SEED_BYTES, SEED_BYTES);
    memcpy(ss, buf4 + 2*SEED_BYTES, SS_KEY_BYTES);
    *ss_len_bytes = SS_KEY_BYTES;

    twopke_enc(pk1, pk2, m, seed_enc1, seed_enc2, ct);
    *ct_len_bytes = 2 * C1_LEN_BYTES + C2_LEN_BYTES;

    return 0;
}

int twokem_dec(
	unsigned char *sk1, unsigned long long sk1_len_bytes,
    unsigned char *sk2, unsigned long long sk2_len_bytes,
    unsigned char *pk2, unsigned long long pk2_len_bytes,
	unsigned char *ct, unsigned long long ct_len_bytes,
	unsigned char *ss, unsigned long long *ss_len_bytes)
{
    unsigned char ss_reject[SS_KEY_BYTES], ss_valid[SS_KEY_BYTES];
    unsigned char m[PKE_MESSAGE_BYTES];
    unsigned char buf_rej[KEM_REJECT_SEED_BYTES + 2 * PKE_CIPHERTEXT_BYTES];
    const unsigned char *sk1_pke = sk1, *sk2_pke = sk2;
    const unsigned char *pk1 = sk1 + PKE_SECRET_KEY_BYTES;
    const unsigned char *reject_seed = sk1 + PKE_SECRET_KEY_BYTES + PKE_PUBLIC_KEY_BYTES;
    unsigned char buf[2*PKE_PUBLIC_KEY_BYTES], buf2[SEED_BYTES];
    unsigned char buf3[PKE_MESSAGE_BYTES + SEED_BYTES], buf4[2*SEED_BYTES+SS_KEY_BYTES];
    unsigned char seed_enc1[SEED_BYTES], seed_enc2[SEED_BYTES];
    unsigned char ct_check[2 * C1_LEN_BYTES + C2_LEN_BYTES];
    uint8_t fail;
    (void)sk1_len_bytes;(void)sk2_len_bytes;(void)pk2_len_bytes;

    if(ct_len_bytes != 2 * C1_LEN_BYTES + C2_LEN_BYTES){
        return -1;
    }
    twopke_dec(sk1_pke, sk2_pke, ct, m);

    memcpy(buf, pk1, PKE_PUBLIC_KEY_BYTES);
    memcpy(buf + PKE_PUBLIC_KEY_BYTES, pk2, PKE_PUBLIC_KEY_BYTES);

    if(pseudohash(SEED_BYTES*8, buf, sizeof(buf), buf2) != 0){
        return -1;
    }
    memcpy(buf3, m, PKE_MESSAGE_BYTES);
    memcpy(buf3 + PKE_MESSAGE_BYTES, buf2, sizeof(buf2));
    if(pseudoXOF((2*SEED_BYTES+SS_KEY_BYTES)*8, buf3, sizeof(buf3), buf4) != 0){
        return -1;
    }
    memcpy(seed_enc1, buf4, SEED_BYTES);
    memcpy(seed_enc2, buf4 + SEED_BYTES, SEED_BYTES);
    memcpy(ss_valid, buf4 + 2*SEED_BYTES, SS_KEY_BYTES);
    
    memcpy(buf_rej, reject_seed, KEM_REJECT_SEED_BYTES);
    memcpy(buf_rej + KEM_REJECT_SEED_BYTES, ct, ct_len_bytes);
    if(pseudoXOF(SS_KEY_BYTES*8, buf_rej, sizeof(buf_rej), ss_reject) != 0){
        return -1;
    }

    twopke_enc(pk1, pk2, m, seed_enc1, seed_enc2, ct_check);
    fail = ct_verify(ct, ct_check, 2 * C1_LEN_BYTES + C2_LEN_BYTES);

    memcpy(ss, ss_reject, KEM_SS_BYTES);
    ct_cmov(ss, ss_valid, KEM_SS_BYTES, (uint8_t)(fail ^ 1U));
    *ss_len_bytes = SS_KEY_BYTES;

    return 0;

}   

