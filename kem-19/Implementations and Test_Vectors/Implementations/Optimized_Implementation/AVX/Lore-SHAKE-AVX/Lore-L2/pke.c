#include "pke.h"
#include "indcpa.h"
#include "symmetric.h"
#include "randombytes.h"
#include "string.h"
#include "api.h"

/*************************************************
* Name:        crypto_pke_keypair
*
* Description: Generates public and private key for the Lore PKE.
*
* Arguments:   - unsigned char *pk: pointer to output public key
* - unsigned char *sk: pointer to output private key
*
* Returns 0 (success)
**************************************************/
int crypto_pke_keypair(unsigned char *pk, unsigned char *sk) {
    indcpa_keypair(pk, sk);
    return 0;
}

/*************************************************
* Name:        crypto_pke_encrypt
*
* Description: Encrypts a message using the Lore PKE.
* The message length is fixed to LORE_SYMBYTES.
*
* Arguments:   - unsigned char *ct: pointer to output ciphertext
* - const unsigned char *m: pointer to input message
* - unsigned long long mlen: length of the message
* - const unsigned char *pk: pointer to public key
*
* Returns 0 (success) or -1 (failure)
**************************************************/
int crypto_pke_encrypt(unsigned char *ct,
                       const unsigned char *m,
                       unsigned long long mlen,
                       const unsigned char *pk) {
    if (mlen != LORE_MSG_BYTES) {
        return -1; // Message length must be 32 bytes
    }

    unsigned char coins[LORE_SYMBYTES];
    randombytes(coins, LORE_SYMBYTES); // Coins for randomness

    indcpa_enc(ct, m, pk, coins);
    return 0;
}

/*************************************************
* Name:        crypto_pke_decrypt
*
* Description: Decrypts a ciphertext using the Lore PKE.
*
* Arguments:   - unsigned char *m: pointer to output decrypted message
* - unsigned long long *mlen: pointer to output message length
* - const unsigned char *ct: pointer to input ciphertext
* - const unsigned char *sk: pointer to secret key
*
* Returns 0 (success)
**************************************************/
int crypto_pke_decrypt(unsigned char *m,
                       unsigned long long *mlen,
                       const unsigned char *ct,
                       const unsigned char *sk) {
    indcpa_dec(m, ct, sk);
    *mlen = LORE_MSG_BYTES;
    return 0;
}

