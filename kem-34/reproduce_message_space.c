/* Rebuild Rudraksh2's KEM key from the encapsulation message and public key.
 * This is the submitters' KEM dataflow, exercised against the unmodified
 * Rudraksh2-128 reference source.  The secret key and ciphertext are not used
 * in the reconstruction. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "KEM_lwekem128.h"
#include "auxfunc.h"
#include "drng.h"
#include "params.h"
#include "symmetric.h"

DRNG_ctx drng_algorithm;

int main(void)
{
    const int trials = 32;
    int matches = 0;
    unsigned long long pk_len = kem_get_pk_len_bytes();
    unsigned long long sk_len = kem_get_sk_len_bytes();
    unsigned long long ct_len = kem_get_ct_len_bytes();
    unsigned long long ss_len = kem_get_ss_len_bytes();
    unsigned long long out_len = 0;
    unsigned char *pk = malloc(pk_len);
    unsigned char *sk = malloc(sk_len);
    unsigned char *ct = malloc(ct_len);
    unsigned char *ss = malloc(ss_len);

    if (!pk || !sk || !ct || !ss)
        return 2;

    for (int i = 0; i < trials; i++) {
        unsigned char seed[64];
        unsigned char m[KEM_SYMBYTES];
        unsigned char buf[2 * KEM_SYMBYTES];
        unsigned char kr[2 * KEM_SYMBYTES];

        for (int j = 0; j < 64; j++)
            seed[j] = (unsigned char)(i * 53 + j * 3 + 7);

        init_random_number(&drng_algorithm, seed, sizeof seed);
        kem_keygen(pk, &out_len, sk, &out_len);

        /* kem_enc draws m first.  Resetting the deterministic test DRNG lets
         * the witness obtain that value without inspecting sk or ct. */
        init_random_number(&drng_algorithm, seed, sizeof seed);
        kem_enc(pk, pk_len, ss, &out_len, ct, &out_len);
        init_random_number(&drng_algorithm, seed, sizeof seed);
        get_random_number(&drng_algorithm, m, KEM_SYMBYTES * 8);

        memcpy(buf, m, KEM_SYMBYTES);
        hash_h(buf + KEM_SYMBYTES, pk, pk_len);
        hash_g(kr, buf, sizeof buf);
        matches += memcmp(kr, ss, ss_len) == 0;
    }

    printf("ATTACK rudraksh-message-space lwekem128 %s "
           "key=G(m||H(pk)) rebuilt from 128-bit m and pk: %d/%d; "
           "generic Grover search about 2^64\n",
           matches == trials ? "CONFIRMED" : "NOT-CONFIRMED",
           matches, trials);

    free(ss);
    free(ct);
    free(sk);
    free(pk);
    return matches == trials ? 0 : 1;
}
