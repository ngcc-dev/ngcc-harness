#include "KEM_AlgorithmInstance.h"

#include <stddef.h>

#include "kem.h"
#include "params.h"
#include "drng.h"

/* DRNG context is initialized by KAT_KEM.c before kem_keygen / kem_enc */
extern DRNG_ctx drng_algorithm;

/*
 * Return codes:
 *   0   success
 *  -1   semantic decapsulation failure
 * <=-2  internal/parameter errors
 */
#define LORE_KAT_SUCCESS              0
#define LORE_KAT_ERR_NULLPTR         -2
#define LORE_KAT_ERR_LEN_MISMATCH    -3
#define LORE_KAT_ERR_INTERNAL_KEYGEN -4
#define LORE_KAT_ERR_INTERNAL_ENC    -5
#define LORE_KAT_ERR_INTERNAL_DEC    -6
#define LORE_KAT_ERR_DRNG            -7

static int lore_check_exact_len(unsigned long long got, unsigned long long expected)
{
    return got == expected ? 0 : -1;
}

unsigned long long kem_get_pk_len_bytes(void)
{
    return (unsigned long long)LORE_KEM_PUBLICKEYBYTES;
}

unsigned long long kem_get_sk_len_bytes(void)
{
    return (unsigned long long)LORE_KEM_SECRETKEYBYTES;
}

unsigned long long kem_get_ss_len_bytes(void)
{
    return (unsigned long long)LORE_SSBYTES;
}

unsigned long long kem_get_ct_len_bytes(void)
{
    return (unsigned long long)LORE_CIPHERTEXTBYTES;
}

int kem_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    int ret;
    unsigned char coins[2 * LORE_SYMBYTES];

    if (pk == NULL || pk_len_bytes == NULL || sk == NULL || sk_len_bytes == NULL) {
        return LORE_KAT_ERR_NULLPTR;
    }

    *pk_len_bytes = kem_get_pk_len_bytes();
    *sk_len_bytes = kem_get_sk_len_bytes();

    ret = get_random_number(&drng_algorithm, coins, (unsigned long long)sizeof(coins) * 8ULL);
    if (ret != 0) {
        return LORE_KAT_ERR_DRNG;
    }

    ret = crypto_kem_keypair_derand(pk, sk, coins);
    if (ret != 0) {
        return LORE_KAT_ERR_INTERNAL_KEYGEN;
    }

    return LORE_KAT_SUCCESS;
}

int kem_enc(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes,
    unsigned char *ct, unsigned long long *ct_len_bytes)
{
    int ret;
    unsigned char coins[LORE_MSG_BYTES];

    if (pk == NULL || ss == NULL || ss_len_bytes == NULL || ct == NULL || ct_len_bytes == NULL) {
        return LORE_KAT_ERR_NULLPTR;
    }

    if (lore_check_exact_len(pk_len_bytes, kem_get_pk_len_bytes()) != 0) {
        return LORE_KAT_ERR_LEN_MISMATCH;
    }

    *ss_len_bytes = kem_get_ss_len_bytes();
    *ct_len_bytes = kem_get_ct_len_bytes();

    ret = get_random_number(&drng_algorithm, coins, (unsigned long long)sizeof(coins) * 8ULL);
    if (ret != 0) {
        return LORE_KAT_ERR_DRNG;
    }

    ret = crypto_kem_enc_derand(ct, ss, pk, coins);
    if (ret != 0) {
        return LORE_KAT_ERR_INTERNAL_ENC;
    }

    return LORE_KAT_SUCCESS;
}

int kem_dec(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *ct, unsigned long long ct_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes)
{
    int ret;

    if (sk == NULL || ct == NULL || ss == NULL || ss_len_bytes == NULL) {
        return LORE_KAT_ERR_NULLPTR;
    }

    if (lore_check_exact_len(sk_len_bytes, kem_get_sk_len_bytes()) != 0) {
        return LORE_KAT_ERR_LEN_MISMATCH;
    }

    if (lore_check_exact_len(ct_len_bytes, kem_get_ct_len_bytes()) != 0) {
        return LORE_KAT_ERR_LEN_MISMATCH;
    }

    *ss_len_bytes = kem_get_ss_len_bytes();

    ret = crypto_kem_dec(ss, ct, sk);
    if (ret == 1) {
        return -1;
    }
    if (ret != 0) {
        return LORE_KAT_ERR_INTERNAL_DEC;
    }

    return LORE_KAT_SUCCESS;
}