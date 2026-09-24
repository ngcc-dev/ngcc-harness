#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <rng.h>
#include <sig.h>
#include <torsion_constants.h>
#include <encoded_sizes.h>

#define CHECK(label, expr)                         \
    do {                                           \
        if (!(expr)) {                             \
            fprintf(stderr, "%s failed\n", label); \
            return 1;                              \
        }                                          \
    } while (0)

int
main(void)
{
    unsigned char pk[PUBLICKEY_BYTES + 1];
    unsigned char pk2[PUBLICKEY_BYTES];
    unsigned char sk[SECRETKEY_BYTES + 1];
    unsigned char sk2[SECRETKEY_BYTES];
    unsigned char bad_sk[SECRETKEY_BYTES];
    unsigned char sig[SIGNATURE_LEN + 1];
    unsigned char sm[SIGNATURE_LEN + 32];
    unsigned char opened[32];
    unsigned char msg[32] = { 0 };
    unsigned char tampered_msg[32] = { 0 };
    unsigned long long siglen = 0;
    unsigned long long smlen = 0;
    unsigned long long opened_len = 0;

    randombytes_init((unsigned char *)"some", (unsigned char *)"string", 128);

    pk[PUBLICKEY_BYTES] = 0xa5;
    sk[SECRETKEY_BYTES] = 0x5a;
    sig[SIGNATURE_LEN] = 0x3c;

    CHECK("keypair", sqisign_keypair(pk, sk) == 0);
    CHECK("pk guard", pk[PUBLICKEY_BYTES] == 0xa5);
    CHECK("sk guard", sk[SECRETKEY_BYTES] == 0x5a);

    CHECK("signature", sqisign_signature(sig, &siglen, msg, sizeof(msg), sk) == 0);
    CHECK("signature length", siglen == SIGNATURE_LEN);
    CHECK("signature guard", sig[SIGNATURE_LEN] == 0x3c);

    CHECK("verify", sqisign_verify(msg, sizeof(msg), sig, siglen, pk) == 0);
    CHECK("verify short signature", sqisign_verify(msg, sizeof(msg), sig, siglen - 1, pk) == 1);

    tampered_msg[0] = 1;
    CHECK("verify tampered message", sqisign_verify(tampered_msg, sizeof(tampered_msg), sig, siglen, pk) == 1);

    memcpy(bad_sk, sk, SECRETKEY_BYTES);
    bad_sk[0] ^= 1;
    CHECK("unknown secret key", sqisign_signature(sig, &siglen, msg, sizeof(msg), bad_sk) == 1);

    CHECK("second keypair", sqisign_keypair(pk2, sk2) == 0);
    CHECK("verify wrong public key", sqisign_verify(msg, sizeof(msg), sig, siglen, pk2) == 1);

    CHECK("sign", sqisign_sign(sm, &smlen, msg, sizeof(msg), sk) == 0);
    CHECK("signed message length", smlen == SIGNATURE_LEN + sizeof(msg));
    CHECK("open", sqisign_open(opened, &opened_len, sm, smlen, pk) == 0);
    CHECK("opened length", opened_len == sizeof(msg));
    CHECK("opened message", memcmp(opened, msg, sizeof(msg)) == 0);

    return 0;
}
