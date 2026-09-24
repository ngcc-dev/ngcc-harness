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

#include "ccakem.h"
#include "cpapke.h"
#include "params.h"

// The following should be used to get pseudorandom numbers
// get_random_number(&drng_algorithm, random_number, random_number_len_bits);

unsigned long long kem_get_pk_len_bytes() {
    return RLWE_CCA_PK_LEN;
}

unsigned long long kem_get_sk_len_bytes() {
    return RLWE_CCA_SK_LEN;
}

unsigned long long kem_get_ss_len_bytes() {
    return RLWE_KEY_LEN;
}

unsigned long long kem_get_ct_len_bytes() {
    return RLWE_CCA_CT_LEN;
}

int kem_keygen(
    unsigned char* pk,
    unsigned long long* pk_len_bytes,
    unsigned char* sk,
    unsigned long long* sk_len_bytes) {
    CCAKEM_KeyGen(sk, pk);
    *pk_len_bytes = RLWE_CCA_PK_LEN;
    *sk_len_bytes = RLWE_CCA_SK_LEN;
    return 0;
}

int kem_enc(
    unsigned char* pk,
    unsigned long long pk_len_bytes,
    unsigned char* ss,
    unsigned long long* ss_len_bytes,
    unsigned char* ct,
    unsigned long long* ct_len_bytes) {
    CCAKEM_Encaps(ct, ss, pk);
    *ss_len_bytes = RLWE_KEY_LEN;
    *ct_len_bytes = RLWE_CCA_CT_LEN;
    return 0;
}

int kem_dec(
    unsigned char* sk,
    unsigned long long sk_len_bytes,
    unsigned char* ct,
    unsigned long long ct_len_bytes,
    unsigned char* ss,
    unsigned long long* ss_len_bytes) {
    *ss_len_bytes = RLWE_KEY_LEN;
    return CCAKEM_Decaps(ss, sk, ct);
}