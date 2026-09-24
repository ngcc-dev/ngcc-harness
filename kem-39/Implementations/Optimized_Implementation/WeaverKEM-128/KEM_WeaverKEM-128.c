/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.

WeaverKEM-128 interface implementation.
Security level: 128-bit classical / 80-bit quantum (WEAVER_MODE=1).
*/

/* Select WEAVER_MODE=1 (WeaverKEM-512, 128-bit classical security) */
#define WEAVER_MODE 1

#include "KEM_WeaverKEM-128.h"
#include "drng.h"

/* ---------------------------------------------------------------
 * Algorithm source files (reference implementation)
 * --------------------------------------------------------------- */
#include "params.h"
#include "api.h"
#include <string.h>
#include <stdint.h>

/* ---------------------------------------------------------------
 * Global SM3-DRNG context, declared extern in drng.h / KAT_KEM.c
 * --------------------------------------------------------------- */
extern DRNG_ctx drng_algorithm;

/* ---------------------------------------------------------------
 * Length query functions
 * --------------------------------------------------------------- */

unsigned long long kem_get_pk_len_bytes()
{
    return (unsigned long long)WEAVER_PUBLICKEYBYTES;
}

unsigned long long kem_get_sk_len_bytes()
{
    return (unsigned long long)WEAVER_SECRETKEYBYTES;
}

unsigned long long kem_get_ss_len_bytes()
{
    return (unsigned long long)WEAVER_SSBYTES;
}

unsigned long long kem_get_ct_len_bytes()
{
    return (unsigned long long)WEAVER_CIPHERTEXTBYTES;
}

/* ---------------------------------------------------------------
 * KEM operations
 * --------------------------------------------------------------- */

int kem_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    uint8_t coins[2*WEAVER_SYMBYTES];
    int ret = 0;
    randombytes(coins, 2*WEAVER_SYMBYTES);
    ret = crypto_kem_keypair_derand(pk, sk, coins);
    if (ret != 0)
        return ret;
    *pk_len_bytes = (unsigned long long)WEAVER_PUBLICKEYBYTES;
    *sk_len_bytes = (unsigned long long)WEAVER_SECRETKEYBYTES;
    return ret;
}

int kem_enc(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes,
    unsigned char *ct, unsigned long long *ct_len_bytes)
{
    uint8_t coins[WEAVER_INDCPA_MSGBYTES]; /* coins --> used as encrypted m for PKE */
    (void)pk_len_bytes;
    int ret = 0;
    randombytes(coins, WEAVER_INDCPA_MSGBYTES);
    ret = crypto_kem_enc_derand(ct, ss, pk, coins);
    if (ret != 0)
        return ret;
    *ss_len_bytes = (unsigned long long)WEAVER_SSBYTES;
    *ct_len_bytes = (unsigned long long)WEAVER_CIPHERTEXTBYTES;
    return ret;
}

int kem_dec(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *ct, unsigned long long ct_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes)
{
    (void)sk_len_bytes;
    (void)ct_len_bytes;
    int ret = crypto_kem_dec(ss, ct, sk);
    if (ret != 0)
        return -1;
    *ss_len_bytes = (unsigned long long)WEAVER_SSBYTES;
    return 0;
}
