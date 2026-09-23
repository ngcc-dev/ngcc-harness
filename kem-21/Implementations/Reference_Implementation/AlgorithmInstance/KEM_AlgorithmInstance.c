/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.

BRIDGE IMPLEMENTATION: This file connects the Viper KEM implementation to
the API_PKC template interface. The Viper KEM functions are called through
their standard API, with randomness provided by the API_PKC DRNG.
*/

#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include <string.h>

/* Viper KEM API */
#include "api.h"

/* Viper internal parameters for sizing */
#include "viper_params.h"

/* External DRNG context from KAT_KEM.c */
extern DRNG_ctx drng_algorithm;

/* Override Viper's randombytes to use API_PKC's DRNG */
int randombytes(unsigned char *x, unsigned long long xlen)
{
    get_random_number(&drng_algorithm, x, xlen * 8);
    return 0;
}

/* Override Viper's randombytes_init (called by Viper KAT but not by API_PKC KAT) */
void randombytes_init(unsigned char *entropy_input,
                      unsigned char *personalization_string,
                      int security_strength)
{
    (void)security_strength;
    unsigned char seed_material[48];
    memcpy(seed_material, entropy_input, 48);
    if (personalization_string)
        for (int i = 0; i < 48; i++)
            seed_material[i] ^= personalization_string[i];
    init_random_number(&drng_algorithm, seed_material, 48);
}

unsigned long long kem_get_pk_len_bytes()
{
    return CRYPTO_PUBLICKEYBYTES;
}

unsigned long long kem_get_sk_len_bytes()
{
    return CRYPTO_SECRETKEYBYTES;
}

unsigned long long kem_get_ss_len_bytes()
{
    return CRYPTO_BYTES;
}

unsigned long long kem_get_ct_len_bytes()
{
    return CRYPTO_CIPHERTEXTBYTES;
}

int kem_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    int rtn = crypto_kem_keypair(pk, sk);
    *pk_len_bytes = CRYPTO_PUBLICKEYBYTES;
    *sk_len_bytes = CRYPTO_SECRETKEYBYTES;
    return rtn;
}

int kem_enc(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes,
    unsigned char *ct, unsigned long long *ct_len_bytes)
{
    (void)pk_len_bytes;
    int rtn = crypto_kem_enc(ct, ss, pk);
    *ss_len_bytes = CRYPTO_BYTES;
    *ct_len_bytes = CRYPTO_CIPHERTEXTBYTES;
    return rtn;
}

int kem_dec(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *ct, unsigned long long ct_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes)
{
    (void)sk_len_bytes;
    (void)ct_len_bytes;
    int rtn = crypto_kem_dec(ss, ct, sk);
    *ss_len_bytes = CRYPTO_BYTES;
    return rtn;
}
