#include "SIG_AlgorithmInstance.h"
#include "lib/drng.h"
#include "src/sydo_c_api.h"

#include <stdlib.h>

extern DRNG_ctx drng_algorithm;

unsigned long long sig_get_pk_len_bytes()
{
    return sydo_pk_capacity_c();
}

unsigned long long sig_get_sk_len_bytes()
{
    return sydo_sk_capacity_c();
}

unsigned long long sig_get_sn_len_bytes()
{
    return sydo_sn_capacity_c();
}

int sig_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    unsigned long long random_seed_len = sydo_keygen_random_seed_bytes_c();
    unsigned char *random_seed = (unsigned char *)malloc((size_t)random_seed_len);
    int ret;

    if (random_seed == NULL)
        return -1;
    if (get_random_number(&drng_algorithm, random_seed, random_seed_len * 8) != 0) {
        free(random_seed);
        return -2;
    }

    ret = sydo_keygen_c(pk, pk_len_bytes, sk, sk_len_bytes, random_seed, random_seed_len);
    free(random_seed);
    return ret;
}

int sig_sign(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes,
    unsigned char *sn, unsigned long long *sn_len_bytes)
{
    unsigned long long random_seed_len = sydo_sign_random_seed_bytes_c();
    unsigned char *random_seed = (unsigned char *)malloc((size_t)random_seed_len);
    int ret;

    if (random_seed == NULL)
        return -1;
    if (get_random_number(&drng_algorithm, random_seed, random_seed_len * 8) != 0) {
        free(random_seed);
        return -2;
    }

    ret = sydo_sign_c(sk, sk_len_bytes, m, m_len_bytes, sn, sn_len_bytes, random_seed,
                        random_seed_len);
    free(random_seed);
    return ret;
}

int sig_verify(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *sn, unsigned long long sn_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes)
{
    return sydo_verify_c(pk, pk_len_bytes, sn, sn_len_bytes, m, m_len_bytes);
}
