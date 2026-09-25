/* Replay the fixed public-key-only VDOO-128 forgery in Feussner's 2026-09-25
 * preliminary disclosure. The secret key is discarded before verification.
 * This validates the supplied transcript, not the public-key attack search.
 */
#include "SIG_AlgorithmInstance.h"
#include "rng.h"
#include "drng.h"
#include <openssl/sha.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DRNG_ctx drng_algorithm;

static void require(int ok, const char *label)
{
    if (!ok) { fprintf(stderr, "FAIL: %s\n", label); exit(1); }
}

static unsigned nibble(char c)
{
    if (c >= '0' && c <= '9') return (unsigned)(c - '0');
    if (c >= 'a' && c <= 'f') return (unsigned)(c - 'a' + 10);
    require(0, "invalid signature hex");
    return 0;
}

static void parse_hex(unsigned char *out, size_t len, const char *hex)
{
    require(strlen(hex) == len * 2, "signature length");
    for (size_t i = 0; i < len; ++i)
        out[i] = (unsigned char)((nibble(hex[2*i]) << 4) | nibble(hex[2*i+1]));
}

static void check_sha256(const char *label, const unsigned char *input, size_t len,
                         const char *expected)
{
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(input, len, digest);
    for (size_t i = 0; i < sizeof(digest); ++i) {
        if (nibble(expected[2*i]) != (unsigned)(digest[i] >> 4) ||
            nibble(expected[2*i+1]) != (unsigned)(digest[i] & 15)) {
            fprintf(stderr, "FAIL: %s SHA-256 mismatch (actual ", label);
            for (size_t j = 0; j < sizeof(digest); ++j) fprintf(stderr, "%02x", digest[j]);
            fprintf(stderr, ")\n");
            exit(1);
        }
    }
}

int main(void)
{
    const unsigned char message[] =
        "Independent-key public-key-only "
        "structural forgery against submitted VDOO-128";
    const char *sig_hex =
        "9f3606a0748596e7bf00a719fd1234dbb562a86fa6f465e377f81d1f6b8f8d63"
        "f28b5577b42cc809d5d76d9fdbb77c7b6533b561a2aef559b986dbc4972c1311"
        "3ba7d2e00f33d253ff531ba22f82951e499884ed23";
    const char *pk_digest = "f0bf3e7faaa200c14c8b37c12d4a0d7195d2ff6c0ce8081e29ea303a4b3d91c6";
    const char *msg_digest = "31b942f4df86978090308e2bc13a54379dd0333c66d5210831d4ddf49ceeb575";
    const char *sig_digest = "575c13a9f988e6210c562c2cab861dc13228878c1aa45d7d7357f6188ea5b08c";
    unsigned char seed[48];
    for (unsigned i = 0; i < sizeof(seed); ++i) seed[i] = (unsigned char)i;
    init_randombytes(seed, sizeof(seed));

    unsigned long long pk_len = 0, sk_len = 0;
    unsigned char *pk = malloc((size_t)sig_get_pk_len_bytes());
    unsigned char *sk = malloc((size_t)sig_get_sk_len_bytes());
    require(pk && sk, "allocation");
    require(sig_keygen(pk, &pk_len, sk, &sk_len) == 0, "submitted keygen");
    require(pk_len == sig_get_pk_len_bytes(), "public key length");
    check_sha256("public key", pk, (size_t)pk_len, pk_digest);
    memset(sk, 0, (size_t)sk_len);
    free(sk);

    unsigned char signature[85];
    parse_hex(signature, sizeof(signature), sig_hex);
    check_sha256("message", message, sizeof(message) - 1, msg_digest);
    check_sha256("signature", signature, sizeof(signature), sig_digest);
    require(sig_verify(pk, pk_len, signature, sizeof(signature),
              (unsigned char *)message, sizeof(message) - 1) == 0,
            "posted signature rejected");

    unsigned char changed_message[sizeof(message)];
    memcpy(changed_message, message, sizeof(message));
    changed_message[0] ^= 1;
    require(sig_verify(pk, pk_len, signature, sizeof(signature),
              changed_message, sizeof(message) - 1) != 0,
            "changed-message control accepted");
    signature[0] ^= 1;
    require(sig_verify(pk, pk_len, signature, sizeof(signature),
              (unsigned char *)message, sizeof(message) - 1) != 0,
            "changed-signature control accepted");
    free(pk);
    puts("SECURITY\tsign-33\tVDOO-128\tpublic-forgery-replay\tCONFIRMED\tposted independently seeded public key and fixed signature accepted; controls rejected");
    return 0;
}
