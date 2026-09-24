/*
 * test_determinism.c - determinism / KAT vectors (Task 6.2).
 *
 * (a) DRBG: same seed -> same stream; different seed -> different stream.
 * (b) Full scheme: with a deterministically seeded DRBG, the sequence
 *        drbg_seed(S); keygen(); sign(msg)
 *     is reproducible byte-for-byte across runs, and a pinned SM3 checksum of
 *     (pk || sk || sig) acts as a Known-Answer-Test that catches ANY change in
 *     keygen/sign output.  A different seed yields a different checksum, and the
 *     produced signature verifies.  (KAT is for QL_TOY parameters.)
 */
#include "api.h"
#include "params.h"
#include "hash.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* Run drbg_seed(seed)->keygen->sign(msg); return outputs + checksum. */
static void run_kat(const uint8_t *seed, size_t seedlen,
                    const unsigned char *msg, size_t mlen,
                    unsigned char *pk, unsigned char *sk,
                    unsigned char *sig, size_t *siglen,
                    uint8_t *checksum)
{
    drbg_seed(seed, seedlen);
    assert(crypto_sign_keypair(pk, sk) == 0);
    assert(crypto_sign_signature(sig, siglen, msg, mlen, sk) == 0);

    hash_ctx_t h;
    hash_init(&h);
    hash_update(&h, pk, QINGLUAN_PK_BYTES);
    hash_update(&h, sk, QINGLUAN_SK_BYTES);
    hash_update(&h, sig, *siglen);
    hash_final(&h, checksum);
}

static void print_hex(const char *label, const uint8_t *p, size_t n)
{
    printf("%s", label);
    for (size_t i = 0; i < n; i++) printf("%02x", p[i]);
    printf("\n");
}

int main(void)
{
    /* (a) DRBG stream determinism */
    uint8_t seed[64]; for (int i = 0; i < 64; i++) seed[i] = (uint8_t)i;
    uint8_t a[48], b[48];
    drbg_seed(seed, sizeof(seed)); randombytes(a, sizeof(a));
    drbg_seed(seed, sizeof(seed)); randombytes(b, sizeof(b));
    assert(memcmp(a, b, sizeof(a)) == 0);

    uint8_t seed2[64]; memset(seed2, 0xAB, 64);
    drbg_seed(seed2, sizeof(seed2)); uint8_t c[48]; randombytes(c, sizeof(c));
    assert(memcmp(a, c, sizeof(a)) != 0);

    /* (b) full keygen+sign determinism + KAT */
    uint8_t kseed[48]; for (int i = 0; i < 48; i++) kseed[i] = (uint8_t)(0x10 + i);
    const unsigned char *msg = (const unsigned char *)"QingLuan determinism KAT";
    size_t mlen = strlen((const char *)msg);

    unsigned char pk1[QINGLUAN_PK_BYTES], sk1[QINGLUAN_SK_BYTES];
    unsigned char pk2[QINGLUAN_PK_BYTES], sk2[QINGLUAN_SK_BYTES];
    unsigned char *sig1 = (unsigned char *)malloc(QINGLUAN_SIG_BYTES);
    unsigned char *sig2 = (unsigned char *)malloc(QINGLUAN_SIG_BYTES);
    assert(sig1 && sig2);
    size_t sl1 = 0, sl2 = 0;
    uint8_t cs1[PARAM_HASH_BYTES], cs2[PARAM_HASH_BYTES];

    run_kat(kseed, sizeof(kseed), msg, mlen, pk1, sk1, sig1, &sl1, cs1);
    run_kat(kseed, sizeof(kseed), msg, mlen, pk2, sk2, sig2, &sl2, cs2);

    /* reproducible byte-for-byte across runs */
    assert(sl1 == sl2);
    assert(memcmp(pk1, pk2, QINGLUAN_PK_BYTES) == 0);
    assert(memcmp(sk1, sk2, QINGLUAN_SK_BYTES) == 0);
    assert(memcmp(sig1, sig2, sl1) == 0);
    assert(memcmp(cs1, cs2, PARAM_HASH_BYTES) == 0);

    /* produced signature verifies */
    assert(crypto_sign_verify(sig1, sl1, msg, mlen, pk1) == 0);

    /* a different seed yields a different checksum */
    uint8_t kseed3[48]; for (int i = 0; i < 48; i++) kseed3[i] = (uint8_t)(0x55 + i);
    unsigned char pk3[QINGLUAN_PK_BYTES], sk3[QINGLUAN_SK_BYTES];
    unsigned char *sig3 = (unsigned char *)malloc(QINGLUAN_SIG_BYTES);
    assert(sig3);
    size_t sl3 = 0; uint8_t cs3[PARAM_HASH_BYTES];
    run_kat(kseed3, sizeof(kseed3), msg, mlen, pk3, sk3, sig3, &sl3, cs3);
    assert(memcmp(cs1, cs3, PARAM_HASH_BYTES) != 0);

    print_hex("KAT checksum = ", cs1, PARAM_HASH_BYTES);

#ifdef QL_TOY
    /* Pinned QL_TOY KAT: any change in keygen/sign/hash/params output breaks it. */
    {
        static const char *KAT_TOY =
            "44c03f0cd6d3e42a2751d22030809bdd4588e310adb1a9338d923097a7537143";
        uint8_t expected[PARAM_HASH_BYTES];
        for (int i = 0; i < PARAM_HASH_BYTES; i++) {
            unsigned v; sscanf(KAT_TOY + 2 * i, "%2x", &v); expected[i] = (uint8_t)v;
        }
        assert(memcmp(cs1, expected, PARAM_HASH_BYTES) == 0);
    }
#endif

    free(sig1); free(sig2); free(sig3);
    printf("drbg determinism OK; keygen+sign reproducible (KAT)\n");
    return 0;
}
