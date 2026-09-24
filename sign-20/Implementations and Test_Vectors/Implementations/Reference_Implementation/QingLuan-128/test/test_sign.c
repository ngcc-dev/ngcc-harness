/*
 * test_sign.c - CROSS-RSDP end-to-end (QL_TOY): keygen -> sign -> verify,
 * tamper/wrong-message/wrong-key rejection, and crypto_sign/open roundtrip.
 */
#include "api.h"
#include "params.h"
#include "sign.h"
#include "utils.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    uint8_t seed[48];
    for (int i = 0; i < 48; i++) seed[i] = (uint8_t)(i + 1);
    drbg_seed(seed, sizeof(seed));            /* deterministic test run */

    unsigned char pk[QINGLUAN_PK_BYTES], sk[QINGLUAN_SK_BYTES];
    assert(crypto_sign_keypair(pk, sk) == 0);

    const char *msg = "QingLuan v2 CROSS-RSDP roundtrip";
    size_t mlen = strlen(msg);

    unsigned char sig[QINGLUAN_SIG_BYTES];
    size_t siglen = 0;
    assert(crypto_sign_signature(sig, &siglen, (const unsigned char *)msg, mlen, sk) == 0);
    assert(siglen == (size_t)QINGLUAN_SIG_BYTES);

    /* honest signature verifies */
    assert(crypto_sign_verify(sig, siglen, (const unsigned char *)msg, mlen, pk) == 0);

    /* tampered signature (flip last byte) is rejected */
    unsigned char bad[QINGLUAN_SIG_BYTES];
    memcpy(bad, sig, siglen);
    bad[QINGLUAN_SIG_BYTES - 1] ^= 0x01;
    assert(crypto_sign_verify(bad, siglen, (const unsigned char *)msg, mlen, pk) != 0);

    /* tampered first digest byte rejected */
    memcpy(bad, sig, siglen);
    bad[SIG_OFF_DIGEST_CMT] ^= 0x80;
    assert(crypto_sign_verify(bad, siglen, (const unsigned char *)msg, mlen, pk) != 0);

    /* wrong message rejected */
    assert(crypto_sign_verify(sig, siglen, (const unsigned char *)"different", 9, pk) != 0);

    /* wrong public key rejected (key binding) */
    unsigned char pk2[QINGLUAN_PK_BYTES], sk2[QINGLUAN_SK_BYTES];
    assert(crypto_sign_keypair(pk2, sk2) == 0);
    assert(crypto_sign_verify(sig, siglen, (const unsigned char *)msg, mlen, pk2) != 0);

    /* combined crypto_sign / crypto_sign_open */
    unsigned char *sm = (unsigned char *)malloc((size_t)QINGLUAN_SIG_BYTES + mlen);
    unsigned long long smlen = 0;
    assert(crypto_sign(sm, &smlen, (const unsigned char *)msg, mlen, sk) == 0);
    assert(smlen == (unsigned long long)QINGLUAN_SIG_BYTES + mlen);

    unsigned char *mout = (unsigned char *)malloc(mlen + 1);
    unsigned long long moutlen = 0;
    assert(crypto_sign_open(mout, &moutlen, sm, smlen, pk) == 0);
    assert(moutlen == mlen);
    assert(memcmp(mout, msg, mlen) == 0);
    assert(crypto_sign_open(mout, &moutlen, sm, smlen, pk2) != 0);   /* wrong pk */

    free(sm);
    free(mout);
    printf("test_sign OK (pk=%d sk=%d sig=%d bytes)\n",
           QINGLUAN_PK_BYTES, QINGLUAN_SK_BYTES, QINGLUAN_SIG_BYTES);
    return 0;
}
