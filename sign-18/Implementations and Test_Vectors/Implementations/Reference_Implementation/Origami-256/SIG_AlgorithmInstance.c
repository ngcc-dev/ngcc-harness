#include <stddef.h>
#include <stdint.h>

#include "SIG_AlgorithmInstance.h"
#include "drng.h"
#include "auxfunc.h"
#include "origami.h"

extern DRNG_ctx drng_algorithm;

unsigned long long sig_get_pk_len_bytes(void) {
    return BYTES_PK;
}

unsigned long long sig_get_sk_len_bytes(void) {
    return BYTES_SK;
}

unsigned long long sig_get_sn_len_bytes(void) {
    return BYTES_SIGNATURE;
}

int sig_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    uint8_t seed[SEED_LENGTH];
    if (get_random_number(&drng_algorithm, seed, SEED_LENGTH * 8ULL) != 0) {
        return -1;
    }
    if (ORIGAMI_NAMESPACE(genkeys)(pk, sk, seed) != 0) {
        return -2;
    }
    *pk_len_bytes = BYTES_PK;
    *sk_len_bytes = BYTES_SK;
    return 0;
}

int sig_sign(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes,
    unsigned char *sn, unsigned long long *sn_len_bytes)
{
    uint8_t digest[BYTES_DIGEST];
    uint8_t salt[BYTES_SALT];
    ph_expanded_SK skx;
    if (sk_len_bytes != BYTES_SK) {
        return -5;
    }

    if (pseudohash(BYTES_DIGEST * 8, m, m_len_bytes * 8ULL, digest) != 0) {
        return -1;
    }
    if (get_random_number(&drng_algorithm, salt, BYTES_SALT * 8ULL) != 0) {
        return -2;
    }
    if (ORIGAMI_NAMESPACE(sk_expand)(&skx, sk) != 0) {
        return -3;
    }
    if (ORIGAMI_NAMESPACE(sign)(&skx, sn, digest, BYTES_DIGEST, salt) != 0) {
        ORIGAMI_NAMESPACE(sk_free)(&skx);
        return -4;
    }
    ORIGAMI_NAMESPACE(sk_free)(&skx);
    *sn_len_bytes = BYTES_SIGNATURE;
    return 0;
}

int sig_verify(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *sn, unsigned long long sn_len_bytes,
    unsigned char *m, unsigned long long m_len_bytes)
{
    uint8_t digest[BYTES_DIGEST];
    ph_expanded_PK pkx;
    if (pk_len_bytes != BYTES_PK || sn_len_bytes != BYTES_SIGNATURE) {
        return -3;
    }

    if (pseudohash(BYTES_DIGEST * 8, m, m_len_bytes * 8ULL, digest) != 0) {
        return -1;
    }
    if (ORIGAMI_NAMESPACE(pk_expand)(&pkx, pk) != 0) {
        return -2;
    }
    if (ORIGAMI_NAMESPACE(verify)(&pkx, sn, digest, BYTES_DIGEST) != 0) {
        ORIGAMI_NAMESPACE(pk_free)(&pkx);
        return -1;
    }
    ORIGAMI_NAMESPACE(pk_free)(&pkx);
    return 0;
}
