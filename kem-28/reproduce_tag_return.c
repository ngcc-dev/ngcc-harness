/* kem-28-2: decapsulate an honest ciphertext with one confirmation-tag bit
 * flipped, for each bit position of one tag byte, and with a tag byte offset
 * chosen by argv[1]. Prints, for every bit, the return value and whether the
 * output buffer holds the honest shared secret. */
#include <stdio.h>
#include <string.h>
#include "KEM_AlgorithmInstance.h"
#include "params.h"
#include "drng.h"

DRNG_ctx drng_algorithm;

int main(void)
{
    static unsigned char pk[NTRUOAEP_PUBLICKEYBYTES], sk[NTRUOAEP_SECRETKEYBYTES];
    static unsigned char ct[NTRUOAEP_CIPHERTEXTBYTES], c2[NTRUOAEP_CIPHERTEXTBYTES];
    unsigned char seed[48] = {7}, ss[NTRUOAEP_SSBYTES], s2[NTRUOAEP_SSBYTES];
    unsigned long long pkl, skl, ssl, ctl;
    int planes = 0, keys = 0, valid;

    init_random_number(&drng_algorithm, seed, 48);
    kem_keygen(pk, &pkl, sk, &skl);
    kem_enc(pk, pkl, ss, &ssl, ct, &ctl);
    valid = kem_dec(sk, skl, ct, ctl, s2, &ssl);
    for (int bit = 0; bit < 8; bit++) {
        memcpy(c2, ct, sizeof ct);
        c2[NTRUOAEP_POLYBYTES + 5] ^= (unsigned char)(1u << bit);
        memset(s2, 0, sizeof s2);
        int rc = kem_dec(sk, skl, c2, ctl, s2, &ssl);
        planes += rc == (int)(signed char)(1u << bit);
        keys += !memcmp(ss, s2, sizeof ss);
    }
    printf("valid_rc=%d plane_returns=%d key_written=%d\n", valid, planes, keys);
    return 0;
}
