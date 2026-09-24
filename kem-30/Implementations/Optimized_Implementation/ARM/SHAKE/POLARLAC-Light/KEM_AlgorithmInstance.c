/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

/*
Copyright (c) 2026 Ying Liu.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements the API_PKC KEM wrapper for the optimized POLARLAC-Light instance.
*/

//  FO transform (BIT_USE_SHAKE selects SHAKE or SM3)：

// G = bit_xof(m || pk) -> K || seed
// H = bit_xof(z || ct), where z is stored in sk
// Verify = re-encrypt and compare ct

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "KEM_AlgorithmInstance.h"
#include "symmetric.h"
#include "drng.h"
#include "random_fips.h"
#include "params.h"
#include "pke.h"


// DRNG_ctx for generating pseudorandom numbers within the KEM scheme
extern DRNG_ctx drng_algorithm;

static uint8_t ct_verify(const uint8_t *a, const uint8_t *b, size_t len)
{
    uint8_t r = 0;

    for (size_t i = 0; i < len; i++) {
        r |= (uint8_t)(a[i] ^ b[i]);
    }

    return (uint8_t)((-(uint64_t)r) >> 63);
}

static void ct_cmov(uint8_t *r, const uint8_t *x, size_t len, uint8_t b)
{
    b = (uint8_t)-b;
    for (size_t i = 0; i < len; i++) {
        r[i] ^= (uint8_t)(b & (r[i] ^ x[i]));
    }
}

static int derive_g(uint8_t *ss, uint8_t *seed_enc, const uint8_t *m, const uint8_t *pk)
{
    uint8_t derivation_input[PKE_MESSAGE_BYTES + PKE_PUBLIC_KEY_BYTES];
    uint8_t g_output[KEM_SS_BYTES + KEM_SEED_LEN_BYTES];

    memcpy(derivation_input, m, PKE_MESSAGE_BYTES);
    memcpy(derivation_input + PKE_MESSAGE_BYTES, pk, PKE_PUBLIC_KEY_BYTES);

    if (bit_xof((KEM_SS_BYTES + KEM_SEED_LEN_BYTES) * 8ULL, derivation_input,
            sizeof(derivation_input) * 8ULL, g_output) != 0) {
        return -10;
    }

    memcpy(ss, g_output, KEM_SS_BYTES);
    memcpy(seed_enc, g_output + KEM_SS_BYTES, KEM_SEED_LEN_BYTES);
    return 0;
}

static int derive_reject_key(uint8_t *ss, const uint8_t *reject_seed, const uint8_t *ct)
{
    uint8_t kdf_input[KEM_REJECT_SEED_BYTES + PKE_CIPHERTEXT_BYTES];

    memcpy(kdf_input, reject_seed, KEM_REJECT_SEED_BYTES);
    memcpy(kdf_input + KEM_REJECT_SEED_BYTES, ct, PKE_CIPHERTEXT_BYTES);

    if (bit_xof(KEM_SS_BYTES * 8ULL, kdf_input,
            sizeof(kdf_input) * 8ULL, ss) != 0) {
        return -13;
    }

    return 0;
}

unsigned long long kem_get_pk_len_bytes(void)
{
    return PKE_PUBLIC_KEY_BYTES;
}

unsigned long long kem_get_sk_len_bytes(void)
{
    return KEM_SK_BYTES;
}

unsigned long long kem_get_ss_len_bytes(void)
{
    return KEM_SS_BYTES;
}

unsigned long long kem_get_ct_len_bytes(void)
{
    return PKE_CIPHERTEXT_BYTES;
}

int kem_keygen(
    unsigned char *pk, unsigned long long *pk_len_bytes,
    unsigned char *sk, unsigned long long *sk_len_bytes)
{
    uint8_t seed_kg[KEM_SEED_LEN_BYTES];
    int ret;

    if (pk == NULL || pk_len_bytes == NULL || sk == NULL || sk_len_bytes == NULL) {
        return -1;
    }

    *pk_len_bytes = kem_get_pk_len_bytes();
    *sk_len_bytes = kem_get_sk_len_bytes();

    if (polarlac_get_random_number(&drng_algorithm, seed_kg, KEM_SEED_LEN_BYTES * 8ULL) != 0) {
        return -2;
    }

    ret = PKE_KeyGen(pk, sk, seed_kg);
    if (ret != 0) {
        return -3;
    }

    memcpy(sk + PKE_SECRET_KEY_BYTES, pk, PKE_PUBLIC_KEY_BYTES);
    if (polarlac_get_random_number(&drng_algorithm,
            sk + PKE_SECRET_KEY_BYTES + PKE_PUBLIC_KEY_BYTES,
            KEM_REJECT_SEED_BYTES * 8ULL) != 0) {
        return -4;
    }
    return 0;
}

int kem_enc(
    unsigned char *pk, unsigned long long pk_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes,
    unsigned char *ct, unsigned long long *ct_len_bytes)
{
    uint8_t m[PKE_MESSAGE_BYTES];
    uint8_t seed_enc[KEM_SEED_LEN_BYTES];
    int ret;

    if (pk == NULL || ss == NULL || ss_len_bytes == NULL || ct == NULL || ct_len_bytes == NULL) {
        return -1;
    }
    if (pk_len_bytes != kem_get_pk_len_bytes()) {
        return -2;
    }

    *ss_len_bytes = kem_get_ss_len_bytes();
    *ct_len_bytes = kem_get_ct_len_bytes();

    if (polarlac_get_random_number(&drng_algorithm, m, PKE_MESSAGE_BYTES * 8ULL) != 0) {
        return -3;
    }

    ret = derive_g(ss, seed_enc, m, pk);
    if (ret != 0) {
        return ret;
    }

    PKE_Encrypt(ct, pk, m, seed_enc);

    return 0;
}

int kem_dec(
    unsigned char *sk, unsigned long long sk_len_bytes,
    unsigned char *ct, unsigned long long ct_len_bytes,
    unsigned char *ss, unsigned long long *ss_len_bytes)
{
    uint8_t m[PKE_MESSAGE_BYTES];
    uint8_t seed_enc[KEM_SEED_LEN_BYTES];
    uint8_t ct_check[PKE_CIPHERTEXT_BYTES];
    uint8_t ss_valid[KEM_SS_BYTES];
    uint8_t ss_reject[KEM_SS_BYTES];
    const uint8_t *sk_pke;
    const uint8_t *pk_stored;
    const uint8_t *reject_seed;
    uint8_t fail;
    int ret;

    if (sk == NULL || ct == NULL || ss == NULL || ss_len_bytes == NULL) {
        return -2;
    }
    if (sk_len_bytes != kem_get_sk_len_bytes() || ct_len_bytes != kem_get_ct_len_bytes()) {
        return -3;
    }

    *ss_len_bytes = kem_get_ss_len_bytes();
    sk_pke = sk;
    pk_stored = sk + PKE_SECRET_KEY_BYTES;
    reject_seed = sk + PKE_SECRET_KEY_BYTES + PKE_PUBLIC_KEY_BYTES;

    PKE_Decrypt(m, ct, sk_pke);

    ret = derive_g(ss_valid, seed_enc, m, pk_stored);
    if (ret != 0) {
        return ret;
    }

    ret = derive_reject_key(ss_reject, reject_seed, ct);
    if (ret != 0) {
        return ret;
    }

    PKE_Encrypt(ct_check, pk_stored, m, seed_enc);
    fail = ct_verify(ct, ct_check, PKE_CIPHERTEXT_BYTES);
    memcpy(ss, ss_reject, KEM_SS_BYTES);
    ct_cmov(ss, ss_valid, KEM_SS_BYTES, (uint8_t)(fail ^ 1U));

    return 0;
}
