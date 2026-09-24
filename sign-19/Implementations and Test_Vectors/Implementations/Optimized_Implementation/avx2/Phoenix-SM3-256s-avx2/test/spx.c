#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../api.h"
#include "../params.h"
#include "../randombytes.h"

#define SPX_MLEN 32
#define SPX_SIGNATURES 10

int main(void)
{
    printf("===============================================================\n");
    printf("Testing SPHINCS+ variant: %s\n", PARAMNAME);
    printf("===============================================================\n");
    int ret = 0;
    int i;

    /* Make stdout buffer more responsive. */
    setbuf(stdout, NULL);

    unsigned char pk[SPX_PK_BYTES];
    unsigned char sk[SPX_SK_BYTES];
    unsigned char *m = malloc(SPX_MLEN);
    unsigned char *sm = malloc(SPX_BYTES + SPX_MLEN);
    unsigned char *mout = malloc(SPX_BYTES + SPX_MLEN);
    unsigned long long smlen;
    unsigned long long slen;
    unsigned long long tfslen;
    unsigned long long mlen;
    randombytes(m, SPX_MLEN);

    printf("Generating keypair.. ");

    if (crypto_sign_keypair(pk, sk)) {
        printf("failed!\n");
        return -1;
    }
    printf("successful.\n");

    printf("Testing %d signatures with different messages.. \n", SPX_SIGNATURES);

    for (i = 0; i < SPX_SIGNATURES; i++) {
       
        printf("\n  - iteration #%d:\n", i);

        crypto_sign(sm, &smlen, &slen, &tfslen, m, SPX_MLEN, sk);
        unsigned long long slen_tmp = slen;
        unsigned long long tfslen_tmp = tfslen;

        printf("    Generated signature of length %llu bytes.\n", smlen);

        /* Test if signature is valid. */
        if (crypto_sign_open(mout, &mlen, &slen_tmp, &tfslen_tmp, sm, smlen, pk)) {
            printf("    X verification failed!\n");
            ret = -1;
        } else {
            printf("    verification succeeded.\n");
        }

        /* Test if the correct message was recovered. */
        if (mlen != SPX_MLEN) {
            printf("    X mlen incorrect [%llu != %u]!\n", mlen, SPX_MLEN);
            ret = -1;
        }
        else {
            printf("    mlen as expected [%llu].\n", mlen);
        }
        if (memcmp(m, mout, SPX_MLEN)) {
            printf("    X output message incorrect!\n");
            ret = -1;
        }
        else {
            printf("    output message as expected.\n");
        }

        /* Test if signature is valid when validating in-place. */
        if (crypto_sign_open(sm, &mlen, &slen, &tfslen, sm, smlen, pk)) {
            printf("    X in-place verification failed!\n");
            ret = -1;
        }
        else {
            printf("    in-place verification succeeded.\n");
        }

        /* Test if flipping bits invalidates the signature (it should). */
        /* Flip the first bit of the message. Should invalidate. */
        sm[smlen - 1] ^= 1;
        if (!crypto_sign_open(mout, &mlen, &slen, &tfslen, sm, smlen, pk)) {
            printf("    X flipping a bit of m DID NOT invalidate signature!\n");
            ret = -1;
        }
        else {
            printf("    flipping a bit of m invalidates signature.\n");
        }
        sm[smlen - 1] ^= 1;
    }
    free(m);
    free(sm);
    free(mout);

    return ret;
}