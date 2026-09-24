/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "KEM_AlgorithmInstance.h"
#include "drng.h"
#include "kem.h"
#include "params.h"

// Global DRNG context provided by the NGCC framework
extern DRNG_ctx drng_algorithm;

// Key/CT/SS length getters
unsigned long long kem_get_pk_len_bytes() {
    return COMPASS_KEM_PUBLICKEYBYTES;
}

unsigned long long kem_get_sk_len_bytes() {
    return COMPASS_KEM_SECRETKEYBYTES;
}

unsigned long long kem_get_ss_len_bytes() {
    return COMPASS_KEM_SSBYTES;
}

unsigned long long kem_get_ct_len_bytes() {
    return COMPASS_KEM_CIPHERTEXTBYTES;
}

// Core cryptographic function adapters
int kem_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    *pk_len_bytes = COMPASS_KEM_PUBLICKEYBYTES;
    *sk_len_bytes = COMPASS_KEM_SECRETKEYBYTES;
    return crypto_kem_keypair(pk, sk);
}

int kem_enc(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes,
    unsigned char *ct, unsigned long long *ct_len_bytes)
{
    (void)pk_len_bytes; // Suppress unused parameter warning
    *ss_len_bytes = COMPASS_KEM_SSBYTES;
    *ct_len_bytes = COMPASS_KEM_CIPHERTEXTBYTES;
    return crypto_kem_enc(ct, ss, pk);
}

int kem_dec(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *ct, unsigned long long ct_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes)
{
    (void)sk_len_bytes; // Suppress unused parameter warning
    (void)ct_len_bytes; // Suppress unused parameter warning
    *ss_len_bytes = COMPASS_KEM_SSBYTES;
    return crypto_kem_dec(ss, ct, sk);
}
