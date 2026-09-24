#include <string.h>

#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "api.h"
#include "kem.h"
#include "FromNIST/rng.h"

extern DRNG_ctx drng_algorithm;

static int seed_bike_rng_from_kat_drng(void)
{
    unsigned char seed[64] = {0};
    int rtn = get_random_number(&drng_algorithm, seed, sizeof(seed) * 8);
    if (rtn)
        return -1;

    randombytes_init_with_seed_len(seed, sizeof(seed), NULL, 0, 256);
    memset(seed, 0, sizeof(seed));
    return 0;
}

unsigned long long kem_get_pk_len_bytes(void)
{
    return CRYPTO_PUBLICKEYBYTES;
}

unsigned long long kem_get_sk_len_bytes(void)
{
    return CRYPTO_SECRETKEYBYTES;
}

unsigned long long kem_get_ss_len_bytes(void)
{
    return CRYPTO_BYTES;
}

unsigned long long kem_get_ct_len_bytes(void)
{
    return CRYPTO_CIPHERTEXTBYTES;
}

int kem_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    int rtn;

    if (seed_bike_rng_from_kat_drng())
        return -1;

    rtn = crypto_kem_keypair(pk, sk);
    *pk_len_bytes = CRYPTO_PUBLICKEYBYTES;
    *sk_len_bytes = CRYPTO_SECRETKEYBYTES;

    return rtn == 0 ? 0 : -2;
}

int kem_enc(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes,
    unsigned char *ct, unsigned long long *ct_len_bytes)
{
    int rtn;

    if (pk_len_bytes != CRYPTO_PUBLICKEYBYTES)
        return -1;
    if (seed_bike_rng_from_kat_drng())
        return -2;

    rtn = crypto_kem_enc(ct, ss, pk);
    *ss_len_bytes = CRYPTO_BYTES;
    *ct_len_bytes = CRYPTO_CIPHERTEXTBYTES;

    return rtn == 0 ? 0 : -3;
}

int kem_dec(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *ct, unsigned long long ct_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes)
{
    int rtn;

    if (sk_len_bytes != CRYPTO_SECRETKEYBYTES ||
        ct_len_bytes != CRYPTO_CIPHERTEXTBYTES)
        return -1;

    rtn = crypto_kem_dec(ss, ct, sk);
    *ss_len_bytes = CRYPTO_BYTES;

    return rtn == 0 ? 0 : -2;
}
