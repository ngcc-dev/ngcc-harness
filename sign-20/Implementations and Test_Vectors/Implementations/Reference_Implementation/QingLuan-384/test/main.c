/*
 * QingLuan Digital Signature Scheme
 * test/main.c - Basic correctness test
 */

#include "api.h"
#include "params.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_hex(const char *label, const unsigned char *data, size_t len)
{
    printf("%s (%zu bytes): ", label, len);
    size_t print_len = len < 32 ? len : 32;
    for (size_t i = 0; i < print_len; i++) {
        printf("%02x", data[i]);
    }
    if (len > 32) printf("...");
    printf("\n");
}

int main(void)
{
    printf("=== QingLuan Digital Signature Scheme ===\n");
    printf("Parameter set: %s\n", CRYPTO_ALGNAME);
    printf("Public key size: %d bytes\n", CRYPTO_PUBLICKEYBYTES);
    printf("Secret key size: %d bytes\n", CRYPTO_SECRETKEYBYTES);
    printf("Max signature size: %d bytes\n", CRYPTO_BYTES);
    printf("\n");

    /* GM/T-style power-on self-test: SM3 KAT, DRBG, keygen, sign/verify. */
    printf("[0] Power-on self-test...\n");
    int h_ok, d_ok, k_ok, s_ok, v_ok;
    if (crypto_sign_selftest_detailed(&h_ok, &d_ok, &k_ok, &s_ok, &v_ok) != 0) {
        fprintf(stderr, "    Self-test FAILED (hash=%d drbg=%d keygen=%d sign=%d verify=%d)\n",
                h_ok, d_ok, k_ok, s_ok, v_ok);
        return 1;
    }
    printf("    Self-test PASSED (SM3 KAT, DRBG, keygen, sign/verify).\n\n");

    /* Allocate keys */
    unsigned char *pk = (unsigned char *)malloc(CRYPTO_PUBLICKEYBYTES);
    unsigned char *sk = (unsigned char *)malloc(CRYPTO_SECRETKEYBYTES);
    if (!pk || !sk) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    /* Key generation */
    printf("[1] Key generation...\n");
    int ret = crypto_sign_keypair(pk, sk);
    if (ret != 0) {
        fprintf(stderr, "Key generation failed!\n");
        return 1;
    }
    printf("    Key generation successful.\n");
    print_hex("    Public key", pk, CRYPTO_PUBLICKEYBYTES);
    print_hex("    Secret key", sk, CRYPTO_SECRETKEYBYTES);
    printf("\n");

    /* Test message */
    const char *test_msg = "QingLuan post-quantum digital signature test message.";
    size_t msg_len = strlen(test_msg);

    /* Signature generation (detached) */
    printf("[2] Signature generation...\n");
    unsigned char *sig = (unsigned char *)malloc(CRYPTO_BYTES);
    size_t sig_len;
    if (!sig) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    ret = crypto_sign_signature(sig, &sig_len,
                                (const unsigned char *)test_msg, msg_len, sk);
    if (ret != 0) {
        fprintf(stderr, "Signature generation failed!\n");
        return 1;
    }
    printf("    Signature generation successful.\n");
    printf("    Actual signature size: %zu bytes\n", sig_len);
    print_hex("    Signature", sig, sig_len);
    printf("\n");

    /* Signature verification */
    printf("[3] Signature verification...\n");
    ret = crypto_sign_verify(sig, sig_len,
                             (const unsigned char *)test_msg, msg_len, pk);
    if (ret == 0) {
        printf("    Verification PASSED (valid signature accepted).\n");
    } else {
        printf("    Verification FAILED!\n");
        return 1;
    }
    printf("\n");

    /* Test with modified message (should fail) */
    printf("[4] Verification with modified message...\n");
    char modified_msg[256];
    strncpy(modified_msg, test_msg, sizeof(modified_msg) - 1);
    modified_msg[0] ^= 0x01; /* Flip one bit */

    ret = crypto_sign_verify(sig, sig_len,
                             (const unsigned char *)modified_msg, msg_len, pk);
    if (ret != 0) {
        printf("    Correctly REJECTED modified message.\n");
    } else {
        printf("    ERROR: Modified message was accepted!\n");
        return 1;
    }
    printf("\n");

    /* Test with modified signature (should fail) */
    printf("[5] Verification with modified signature...\n");
    sig[sig_len / 2] ^= 0x01; /* Flip one bit in signature */

    ret = crypto_sign_verify(sig, sig_len,
                             (const unsigned char *)test_msg, msg_len, pk);
    if (ret != 0) {
        printf("    Correctly REJECTED modified signature.\n");
    } else {
        printf("    ERROR: Modified signature was accepted!\n");
        return 1;
    }
    printf("\n");

    /* Test combined sign/open */
    printf("[6] Combined sign/open test...\n");
    sig[sig_len / 2] ^= 0x01; /* Restore the signature bit */

    unsigned char *sm = (unsigned char *)malloc(CRYPTO_BYTES + msg_len);
    unsigned long long smlen;
    unsigned char *m_out = (unsigned char *)malloc(CRYPTO_BYTES + msg_len);
    unsigned long long m_out_len;

    if (!sm || !m_out) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    ret = crypto_sign(sm, &smlen,
                      (const unsigned char *)test_msg, msg_len, sk);
    if (ret != 0) {
        fprintf(stderr, "Combined sign failed!\n");
        return 1;
    }

    ret = crypto_sign_open(m_out, &m_out_len, sm, smlen, pk);
    if (ret == 0 && m_out_len == msg_len &&
        memcmp(m_out, test_msg, msg_len) == 0) {
        printf("    Combined sign/open PASSED.\n");
    } else {
        printf("    Combined sign/open FAILED!\n");
        return 1;
    }
    printf("\n");

    printf("=== All tests passed! ===\n");

    /* Cleanup */
    free(pk);
    free(sk);
    free(sig);
    free(sm);
    free(m_out);

    return 0;
}
