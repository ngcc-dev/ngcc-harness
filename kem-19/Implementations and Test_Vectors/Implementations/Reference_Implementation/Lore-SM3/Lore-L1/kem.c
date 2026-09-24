#include <stddef.h>
#include <string.h>
#include "kem.h"
#include "indcpa.h"
#include "symmetric.h"
#include "randombytes.h"
#include "verify.h"
#include "params.h"
#include "sm3.h"

/*************************************************
* Name:        hash_h
*
* Description: Hash function H used in KEM.
*
* Arguments:   - uint8_t *out:      pointer to output buffer (32 bytes)
* - const uint8_t *in: pointer to input buffer
* - size_t inlen:      length of input buffer in bytes
**************************************************/
/*
 * secure_zero: volatile-based zeroing that cannot be eliminated by the compiler.
 * Used to wipe sensitive key material from the stack before returning.
 */
static void secure_zero(void *ptr, size_t len)
{
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (len--) *p++ = 0;
}

static void hash_h(uint8_t *out, const uint8_t *in, size_t inlen) {
    sm3(out, in, inlen);
}

/*************************************************
* Name:        hash_g
*
* Description: Hash function G used in KEM.
* Computes SM3-based KDF with an output of 2 * LORE_SYMBYTES bytes.
*
* Arguments:   - uint8_t *out:      pointer to output buffer (2 * LORE_SYMBYTES bytes)
* - const uint8_t *in: pointer to input buffer
* - size_t inlen:      length of input buffer in bytes
**************************************************/
static void hash_g(uint8_t *out, const uint8_t *in, size_t inlen) {
    sm3_kdf(out, 2 * LORE_SYMBYTES, in, inlen);
}

/*************************************************
* Name:        crypto_kem_keypair_derand
*
* Description: Deterministic generation of public and secret key
* for the CCA-secure Lore KEM.
*
* Arguments:   - uint8_t *pk: pointer to output public key (length LORE_PUBLICKEYBYTES)
* - uint8_t *sk: pointer to output secret key (length LORE_SECRETKEYBYTES)
* - const uint8_t *coins: pointer to input randomness (length 2*LORE_SYMBYTES)
*
* Returns:     0 on success
**************************************************/
int crypto_kem_keypair_derand(unsigned char *pk, unsigned char *sk, const unsigned char *coins)
{
    indcpa_keypair_derand(pk, sk, coins);
    /* sk = sk' || pk || H(pk) || z */
    unsigned char *sk_rest = sk + LORE_SECRETKEYBYTES;
    memcpy(sk_rest, pk, LORE_PUBLICKEYBYTES);
    sk_rest += LORE_PUBLICKEYBYTES;
    hash_h(sk_rest, pk, LORE_PUBLICKEYBYTES);
    sk_rest += LORE_SYMBYTES;
    for(size_t i = 0; i < LORE_SYMBYTES; i++) {
        sk_rest[i] = coins[LORE_SYMBYTES + i];
    }

    return 0;
}

/*************************************************
* Name:        crypto_kem_keypair
*
* Description: Generates public and private key for the CCA-secure KEM.
* (randomized version)
*
* Arguments:   - unsigned char *pk:    pointer to output public key
* (an already allocated array of LORE_PUBLICKEYBYTES bytes)
* - unsigned char *sk:    pointer to output secret key
* (an already allocated array of LORE_SECRETKEYBYTES bytes)
*
* Returns 0 (success)
**************************************************/
int crypto_kem_keypair(unsigned char *pk, unsigned char *sk)
{
    unsigned char coins[2 * LORE_SYMBYTES];
    randombytes(coins, 2 * LORE_SYMBYTES);
    crypto_kem_keypair_derand(pk, sk, coins);
    secure_zero(coins, sizeof(coins));
    return 0;
}

/*************************************************
* Name:        crypto_kem_enc_derand
*
* Description: Generates ciphertext and shared secret for a given public key.
* (deterministic version)
*
* Arguments:   - unsigned char *ct:    pointer to output ciphertext
* (an already allocated array of LORE_CIPHERTEXTBYTES bytes)
* - unsigned char *ss:    pointer to output shared secret
* (an already allocated array of LORE_SYMBYTES bytes)
* - const unsigned char *pk:    pointer to input public key
* (an already allocated array of LORE_PUBLICKEYBYTES bytes)
* - const unsigned char *coins: pointer to input randomness
* (an already allocated array of LORE_SYMBYTES bytes)
*
* Returns 0 (success)
**************************************************/
int crypto_kem_enc_derand(unsigned char *ct, unsigned char *ss, const unsigned char *pk, const unsigned char *coins)
{
    unsigned char kr[2 * LORE_SYMBYTES];
    unsigned char buf[2 * LORE_SYMBYTES];

    hash_h(buf, coins, LORE_MSG_BYTES);
    hash_h(buf + LORE_SYMBYTES, pk, LORE_PUBLICKEYBYTES);
    hash_g(kr, buf, 2 * LORE_SYMBYTES);
    memset(ct, 0, LORE_CIPHERTEXTBYTES);

    indcpa_enc(ct, coins, pk, kr + LORE_SYMBYTES);

  
    unsigned char kdf_buf[2 * LORE_SYMBYTES];
    

    memcpy(kdf_buf, kr, LORE_SYMBYTES); 
    

    hash_h(kdf_buf + LORE_SYMBYTES, ct, LORE_CIPHERTEXTBYTES); 
    

    sm3_kdf(ss, LORE_SYMBYTES, kdf_buf, 2 * LORE_SYMBYTES);

    secure_zero(kr, sizeof(kr));
    secure_zero(buf, sizeof(buf));
    secure_zero(kdf_buf, sizeof(kdf_buf));
    return 0;
}

/*************************************************
* Name:        crypto_kem_enc
*
* Description: Generates ciphertext and shared secret for a given public key.
* (randomized version)
*
* Arguments:   - unsigned char *ct:    pointer to output ciphertext
* (an already allocated array of LORE_CIPHERTEXTBYTES bytes)
* - unsigned char *ss:    pointer to output shared secret
* (an already allocated array of LORE_SYMBYTES bytes)
* - const unsigned char *pk:    pointer to input public key
* (an already allocated array of LORE_PUBLICKEYBYTES bytes)
*
* Returns 0 (success)
**************************************************/
int crypto_kem_enc(unsigned char *ct, unsigned char *ss, const unsigned char *pk)
{
    unsigned char coins[LORE_MSG_BYTES];
    randombytes(coins, LORE_MSG_BYTES);
    crypto_kem_enc_derand(ct, ss, pk, coins);
    secure_zero(coins, sizeof(coins));
    return 0;
}

/*************************************************
* Name:        crypto_kem_dec
*
* Description: Generates shared secret for a given ciphertext and private key.
*
* Arguments:   - unsigned char *ss:    pointer to output shared secret
* (an already allocated array of LORE_SYMBYTES bytes)
* - const unsigned char *ct:    pointer to input ciphertext
* (an already allocated array of LORE_CIPHERTEXTBYTES bytes)
* - const unsigned char *sk:    pointer to input secret key
* (an already allocated array of LORE_SECRETKEYBYTES bytes)
*
* Returns 0 (success)
**************************************************/
int crypto_kem_dec(unsigned char *ss, const unsigned char *ct, const unsigned char *sk)
{
    int fail;
    unsigned char mu[LORE_MSG_BYTES]; 
    unsigned char kr[2 * LORE_SYMBYTES];
    unsigned char buf[2 * LORE_SYMBYTES];
    unsigned char ct_cmp[LORE_CIPHERTEXTBYTES];
    unsigned char ss_invalid[LORE_SYMBYTES];

    const unsigned char *sk_indcpa = sk;
    const unsigned char *pk = sk + LORE_SECRETKEYBYTES;
    const unsigned char *pkh = pk + LORE_PUBLICKEYBYTES;
    const unsigned char *z = pkh + LORE_SYMBYTES;

    indcpa_dec(mu, ct, sk_indcpa);

    hash_h(buf, mu, LORE_MSG_BYTES);
    memcpy(buf + LORE_SYMBYTES, pkh, LORE_SYMBYTES);
    hash_g(kr, buf, 2 * LORE_SYMBYTES);
    memset(ct_cmp, 0, LORE_CIPHERTEXTBYTES);

    indcpa_enc(ct_cmp, mu, pk, kr + LORE_SYMBYTES);

    fail = verify(ct, ct_cmp, LORE_CIPHERTEXTBYTES);

    // KDF(z || H(c)) for implicit rejection
    {
        unsigned char hash_cp[LORE_SYMBYTES];
        hash_h(hash_cp, ct, LORE_CIPHERTEXTBYTES);  // implicit rejection binds to input ct
        unsigned char fail_kdf[2 * LORE_SYMBYTES];
        memcpy(fail_kdf, z, LORE_SYMBYTES);
        memcpy(fail_kdf + LORE_SYMBYTES, hash_cp, LORE_SYMBYTES);
        sm3_kdf(ss_invalid, LORE_SYMBYTES, fail_kdf, 2 * LORE_SYMBYTES);
    }


    unsigned char kdf_buf[2 * LORE_SYMBYTES];
    unsigned char ss_valid[LORE_SYMBYTES];
    

    memcpy(kdf_buf, kr, LORE_SYMBYTES); 
    

    hash_h(kdf_buf + LORE_SYMBYTES, ct_cmp, LORE_CIPHERTEXTBYTES); 
    

    sm3_kdf(ss_valid, LORE_SYMBYTES, kdf_buf, 2 * LORE_SYMBYTES);



    memcpy(ss, ss_invalid, LORE_SYMBYTES);            /* 默认：隐式拒绝 */
    cmov(ss, ss_valid, LORE_SYMBYTES, (unsigned char)(fail ^ 1)); /* 成功时覆盖 */

    secure_zero(mu, sizeof(mu));
    secure_zero(kr, sizeof(kr));
    secure_zero(buf, sizeof(buf));
    secure_zero(kdf_buf, sizeof(kdf_buf));
    secure_zero(ss_valid, sizeof(ss_valid));
    secure_zero(ss_invalid, sizeof(ss_invalid));
    return 0;
}
