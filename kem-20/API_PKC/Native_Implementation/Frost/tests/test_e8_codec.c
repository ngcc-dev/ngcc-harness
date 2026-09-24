#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if FROST_TEST_LEVEL == 128
#include "../src/api_frost128.h"
#define TEST_BYTES_MU 16
#define TEST_NBAR 8
#define TEST_LOGQ 16
#define TEST_B 2
#define TEST_U32 0
#define test_crypto_kem_keypair crypto_kem_keypair_Frost128
#define test_crypto_kem_enc crypto_kem_enc_Frost128
#define test_crypto_kem_dec crypto_kem_dec_Frost128
#elif FROST_TEST_LEVEL == 192
#include "../src/api_frost192.h"
#define TEST_BYTES_MU 24
#define TEST_NBAR 8
#define TEST_LOGQ 16
#define TEST_B 3
#define TEST_U32 0
#define test_crypto_kem_keypair crypto_kem_keypair_Frost192
#define test_crypto_kem_enc crypto_kem_enc_Frost192
#define test_crypto_kem_dec crypto_kem_dec_Frost192
#elif FROST_TEST_LEVEL == 256
#include "../src/api_frost256.h"
#define TEST_BYTES_MU 32
#define TEST_NBAR 8
#define TEST_LOGQ 16
#define TEST_B 4
#define TEST_U32 0
#define test_crypto_kem_keypair crypto_kem_keypair_Frost256
#define test_crypto_kem_enc crypto_kem_enc_Frost256
#define test_crypto_kem_dec crypto_kem_dec_Frost256
#elif FROST_TEST_LEVEL == 384
#include "../src/api_frost384.h"
#define TEST_BYTES_MU 48
#define TEST_NBAR 8
#define TEST_LOGQ 16
#define TEST_B 4
#define TEST_U32 0
#define test_crypto_kem_keypair crypto_kem_keypair_Frost384
#define test_crypto_kem_enc crypto_kem_enc_Frost384
#define test_crypto_kem_dec crypto_kem_dec_Frost384
#elif FROST_TEST_LEVEL == 512
#include "../src/api_frost512.h"
#define TEST_BYTES_MU 64
#define TEST_NBAR 8
#define TEST_LOGQ 16
#define TEST_B 4
#define TEST_U32 0
#define test_crypto_kem_keypair crypto_kem_keypair_Frost512
#define test_crypto_kem_enc crypto_kem_enc_Frost512
#define test_crypto_kem_dec crypto_kem_dec_Frost512
#else
#error Define FROST_TEST_LEVEL as 128, 192, 256, 384, or 512.
#endif


#if TEST_U32
extern void frost_key_encode_u32(uint32_t *out, const uint8_t *in);
extern void frost_key_decode_u32(uint8_t *out, const uint32_t *in);
#else
extern void frost_key_encode(uint16_t *out, const uint16_t *in);
extern void frost_key_decode(uint16_t *out, const uint16_t *in);
#endif

extern const char *frost_message_codec_name(void);
#ifdef FROST_CODEC_TRACE
extern void frost_codec_trace_reset(void);
extern unsigned long frost_codec_trace_e8_encode_calls(void);
extern unsigned long frost_codec_trace_e8_decode_calls(void);
#endif

static uint32_t rng_state = 1;

static uint8_t next_byte(void)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return (uint8_t)(rng_state >> 24);
}

static void codec_roundtrip(const uint8_t msg[TEST_BYTES_MU], uint8_t dec[TEST_BYTES_MU])
{
#if TEST_U32
    uint32_t enc[TEST_NBAR * (TEST_BYTES_MU / TEST_B)];
    frost_key_encode_u32(enc, msg);
    frost_key_decode_u32(dec, enc);
#else
    uint16_t enc[TEST_NBAR * (TEST_BYTES_MU / TEST_B)];
    frost_key_encode(enc, (const uint16_t *)msg);
    frost_key_decode((uint16_t *)dec, enc);
#endif
}

static int check_message(const uint8_t msg[TEST_BYTES_MU], const char *name)
{
    uint8_t dec[TEST_BYTES_MU];
    codec_roundtrip(msg, dec);
    if (memcmp(msg, dec, TEST_BYTES_MU) != 0) {
        fprintf(stderr, "codec identity failed for %s at level %d message_codec=%s\n", name, FROST_TEST_LEVEL, frost_message_codec_name());
        return 1;
    }
    return 0;
}

static int check_patterns(void)
{
    uint8_t msg[TEST_BYTES_MU];

    memset(msg, 0x00, sizeof(msg));
    if (check_message(msg, "all-zero") != 0) return 1;

    memset(msg, 0xFF, sizeof(msg));
    if (check_message(msg, "all-one") != 0) return 1;

    for (size_t i = 0; i < sizeof(msg); i++) msg[i] = (i & 1u) ? 0x55u : 0xAAu;
    if (check_message(msg, "alternating") != 0) return 1;

    memset(msg, 0x00, sizeof(msg));
    for (size_t bit = 0; bit < TEST_BYTES_MU * 8u; bit++) {
        memset(msg, 0x00, sizeof(msg));
        msg[bit >> 3] = (uint8_t)(1u << (bit & 7));
        if (check_message(msg, "single-bit") != 0) return 1;
    }

    return 0;
}

static int check_exhaustive_or_random_blocks(void)
{
    uint8_t msg[TEST_BYTES_MU];

#if TEST_B == 2
    for (uint32_t v = 0; v < 65536u; v++) {
        memset(msg, 0, sizeof(msg));
        msg[0] = (uint8_t)v;
        msg[1] = (uint8_t)(v >> 8);
        if (check_message(msg, "exhaustive B=2 block") != 0) return 1;
    }
#else
    for (uint32_t trial = 0; trial < 20000u; trial++) {
        memset(msg, 0, sizeof(msg));
        const size_t block_bytes = TEST_B;
        for (size_t i = 0; i < block_bytes; i++) msg[i] = next_byte();
        if (check_message(msg, "random block") != 0) return 1;
    }
#endif

    return 0;
}

static int check_random_messages(void)
{
    uint8_t msg[TEST_BYTES_MU];
    for (uint32_t trial = 0; trial < 2000u; trial++) {
        for (size_t i = 0; i < sizeof(msg); i++) msg[i] = next_byte();
        if (check_message(msg, "random message") != 0) return 1;
    }
    return 0;
}

static int check_kem(void)
{
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss[CRYPTO_BYTES];
    uint8_t ss2[CRYPTO_BYTES];

    for (unsigned i = 0; i < 8; i++) {
        if (test_crypto_kem_keypair(pk, sk) != 0) return 1;
        if (test_crypto_kem_enc(ct, ss, pk) != 0) return 1;
        if (test_crypto_kem_dec(ss2, ct, sk) != 0) return 1;
        if (memcmp(ss, ss2, CRYPTO_BYTES) != 0) {
            fprintf(stderr, "KEM shared-secret mismatch at level %d message_codec=%s\n", FROST_TEST_LEVEL, frost_message_codec_name());
            return 1;
        }
    }
    return 0;
}


static int check_codec_trace_after_kem(void)
{
#ifdef FROST_CODEC_TRACE
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss[CRYPTO_BYTES];
    uint8_t ss2[CRYPTO_BYTES];

    frost_codec_trace_reset();
    if (test_crypto_kem_keypair(pk, sk) != 0) return 1;
    if (test_crypto_kem_enc(ct, ss, pk) != 0) return 1;
    if (test_crypto_kem_dec(ss2, ct, sk) != 0) return 1;
    if (memcmp(ss, ss2, CRYPTO_BYTES) != 0) return 1;

#ifdef FROST_USE_E8_CODE
    if (frost_codec_trace_e8_encode_calls() == 0 || frost_codec_trace_e8_decode_calls() == 0) {
        fprintf(stderr, "E8 trace did not observe encode/decode calls at level %d message_codec=%s encode_calls=%lu decode_calls=%lu\n",
                FROST_TEST_LEVEL, frost_message_codec_name(),
                frost_codec_trace_e8_encode_calls(), frost_codec_trace_e8_decode_calls());
        return 1;
    }
#else
    if (frost_codec_trace_e8_encode_calls() != 0 || frost_codec_trace_e8_decode_calls() != 0) {
        fprintf(stderr, "Scalar trace unexpectedly observed E8 calls at level %d message_codec=%s encode_calls=%lu decode_calls=%lu\n",
                FROST_TEST_LEVEL, frost_message_codec_name(),
                frost_codec_trace_e8_encode_calls(), frost_codec_trace_e8_decode_calls());
        return 1;
    }
#endif
#endif
    return 0;
}

int main(void)
{
    if (check_patterns() != 0) return 1;
    if (check_exhaustive_or_random_blocks() != 0) return 1;
    if (check_random_messages() != 0) return 1;
    if (check_kem() != 0) return 1;
    if (check_codec_trace_after_kem() != 0) return 1;
    printf("codec identity tests passed for Frost-%d message_codec=%s\n", FROST_TEST_LEVEL, frost_message_codec_name());
    return 0;
}
