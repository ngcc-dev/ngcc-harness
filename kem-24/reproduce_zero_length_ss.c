/* kem-24-2 follow-up: with ct_len_bytes = 0, decapsulating an all-zero
 * ciphertext buffer returns a shared secret computable from the public key.
 * Built against one Scabbard reference tree with -DKEM_HDR. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "params.h"
#include "drng.h"
#include "auxfunc.h"
#include "indcpa.h"
#include KEM_HDR

DRNG_ctx drng_algorithm;

/* KDF(K(m0) || H(empty)), with m0 the decryption of an all-zero ciphertext:
 * u = 0 makes the inner product zero for every secret key. */
static void predict(uint8_t ss[SCABBARD_SSBYTES], const uint8_t *pk, unsigned long long pk_len)
{
    static const uint8_t zero_ct[SCABBARD_CIPHERTEXTBYTES], any_sk[SCABBARD_INDCPA_SECRETKEYBYTES];
    uint8_t buf[2 * SCABBARD_SYMBYTES], kr[2 * SCABBARD_SYMBYTES];
    indcpa_dec(buf, zero_ct, any_sk);
    pseudoXOF(SCABBARD_SYMBYTES * 8, pk, pk_len * 8, buf + SCABBARD_SYMBYTES);
    pseudoXOF(2 * SCABBARD_SYMBYTES * 8, buf, 2 * SCABBARD_SYMBYTES * 8, kr);
    pseudoXOF(SCABBARD_SYMBYTES * 8, zero_ct, 0, kr + SCABBARD_SYMBYTES);
    pseudoXOF(SCABBARD_SSBYTES * 8, kr, 2 * SCABBARD_SYMBYTES * 8, ss);
}

int main(void)
{
    unsigned char seed[64] = {7};
    static unsigned char pk[SCABBARD_PUBLICKEYBYTES], sk[SCABBARD_SECRETKEYBYTES];
    static unsigned char ct[SCABBARD_CIPHERTEXTBYTES];
    unsigned char ss[SCABBARD_SSBYTES], pred[SCABBARD_SSBYTES];
    int zero_len_matches = 0, full_len_matches = 0, keys = 8;

    init_random_number(&drng_algorithm, seed, sizeof seed);
    for (int t = 0; t < keys; t++) {
        unsigned long long pkl = sizeof pk, skl = sizeof sk, ssl;
        kem_keygen(pk, &pkl, sk, &skl);
        predict(pred, pk, pkl);
        memset(ct, 0, sizeof ct);
        kem_dec(sk, skl, ct, 0, ss, &ssl);
        zero_len_matches += memcmp(ss, pred, sizeof ss) == 0;
        kem_dec(sk, skl, ct, sizeof ct, ss, &ssl);
        full_len_matches += memcmp(ss, pred, sizeof ss) == 0;
    }
    printf("keys=%d zero_length_predicted=%d full_length_predicted=%d\n",
           keys, zero_len_matches, full_len_matches);
    return 0;
}
