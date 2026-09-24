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
#include "parameters.h"
#include "dkecpa.h"
#include "dkecca.h"
#include "drng.h"

// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
DRNG_ctx drng_algorithm;

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

unsigned long long kem_get_pk_len_bytes() { return DKE_PKBYTES; }
unsigned long long kem_get_sk_len_bytes() { return DKE_SKBYTES; }
unsigned long long kem_get_ss_len_bytes() { return DKE_SSBYTES; }
unsigned long long kem_get_ct_len_bytes() { return DKE_CTBYTES; }

int kem_keygen(unsigned char *pk, unsigned long long *pk_len_bytes,
               unsigned char *sk, unsigned long long *sk_len_bytes) {
    uint8_t coins[DKE_SEEDBYTES + DKE_SSBYTES];
    int rtn = get_random_number(&drng_algorithm, coins, (DKE_SEEDBYTES + DKE_SSBYTES) * 8);
    if (rtn != 0) return rtn;
    DKE_CCA_keygen_derand(pk, sk, coins);
    (void)pk_len_bytes; (void)sk_len_bytes;
    return 0;
}

int kem_enc(unsigned char *pk, unsigned long long pk_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes,
            unsigned char *ct, unsigned long long *ct_len_bytes) {
    uint8_t coins[DKE_SEEDBYTES];
    int rtn = get_random_number(&drng_algorithm, coins, DKE_SEEDBYTES * 8);
    if (rtn != 0) return rtn;
    DKE_CCA_enc_derand(ct, ss, pk, coins);
    (void)pk_len_bytes; (void)ss_len_bytes; (void)ct_len_bytes;
    return 0;
}

int kem_dec(unsigned char *sk, unsigned long long sk_len_bytes,
            unsigned char *ct, unsigned long long ct_len_bytes,
            unsigned char *ss, unsigned long long *ss_len_bytes) {
    DKE_CCA_dec(ss, sk, ct);
    (void)sk_len_bytes; (void)ct_len_bytes; (void)ss_len_bytes;
    return 0;
}
