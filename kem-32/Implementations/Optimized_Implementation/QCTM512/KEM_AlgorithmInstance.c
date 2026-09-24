#include <stddef.h>

#include "KEM_AlgorithmInstance.h"
#include "scheme_api.h"

#define KEM_API_INVALID_ARGUMENT -2
#define KEM_API_LENGTH_MISMATCH -3
#define KEM_API_DRNG_FAILURE -4

#ifdef API_PKC_KAT_DRNG_BRIDGE
#include "drng.h"
#include "rng.h"

extern DRNG_ctx drng_algorithm;

static int api_pkc_rng_ready = 0;

static int init_local_rng_from_api_pkc_drng(void)
{
    unsigned char entropy_input[48];

    if (get_random_number(&drng_algorithm, entropy_input,
                          sizeof(entropy_input) * 8ULL) != 0) {
        return KEM_API_DRNG_FAILURE;
    }
    randombytes_init(entropy_input, NULL, 256);
    api_pkc_rng_ready = 1;
    return 0;
}

static int ensure_local_rng_from_api_pkc_drng(void)
{
    if (api_pkc_rng_ready) {
        return 0;
    }
    return init_local_rng_from_api_pkc_drng();
}
#else
static int init_local_rng_from_api_pkc_drng(void)
{
    return 0;
}

static int ensure_local_rng_from_api_pkc_drng(void)
{
    return 0;
}
#endif

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

int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes)
{
    int status;

    if (pk == NULL || sk == NULL || pk_len_bytes == NULL ||
        sk_len_bytes == NULL) {
        return KEM_API_INVALID_ARGUMENT;
    }

    status = init_local_rng_from_api_pkc_drng();
    if (status != 0) {
        return status;
    }

    status = crypto_kem_keypair(pk, sk);
    if (status != SUCCESS) {
        return -1;
    }

    *pk_len_bytes = kem_get_pk_len_bytes();
    *sk_len_bytes = kem_get_sk_len_bytes();
    return 0;
}

int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes)
{
    int status;

    if (pk == NULL || ss == NULL || ss_len_bytes == NULL || ct == NULL ||
        ct_len_bytes == NULL) {
        return KEM_API_INVALID_ARGUMENT;
    }
    if (pk_len_bytes != kem_get_pk_len_bytes()) {
        return KEM_API_LENGTH_MISMATCH;
    }

    status = ensure_local_rng_from_api_pkc_drng();
    if (status != 0) {
        return status;
    }

    status = crypto_kem_enc(ct, ss, pk);
    if (status != SUCCESS) {
        return -1;
    }

    *ss_len_bytes = kem_get_ss_len_bytes();
    *ct_len_bytes = kem_get_ct_len_bytes();
    return 0;
}

int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes,
            unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes)
{
    int status;

    if (sk == NULL || ct == NULL || ss == NULL || ss_len_bytes == NULL) {
        return KEM_API_INVALID_ARGUMENT;
    }
    if (sk_len_bytes != kem_get_sk_len_bytes() ||
        ct_len_bytes != kem_get_ct_len_bytes()) {
        return KEM_API_LENGTH_MISMATCH;
    }

    status = crypto_kem_dec(ss, ct, sk);
    if (status != SUCCESS) {
        return -1;
    }

    *ss_len_bytes = kem_get_ss_len_bytes();
    return 0;
}
