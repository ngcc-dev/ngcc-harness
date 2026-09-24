/********************************************************************************************
* MAMBA-Frost-CC: test utilities.
*********************************************************************************************/

#include "../../common/random/random.h"
#include "../../common/sha3/fips202.h"

#ifdef DO_VALGRIND_CHECK
#include <valgrind/memcheck.h>
#endif

extern const char *frost_message_codec_name(void);

#if defined(DO_VALGRIND_CHECK) || defined(_PPC_)
#define KEM_TEST_ITERATIONS   1
#else
#define KEM_TEST_ITERATIONS 100
#endif
#define KEM_BENCH_SECONDS     1

static int get_env_int(const char *name, int fallback)
{
    const char *v = getenv(name);
    if (v == NULL || *v == '\0') return fallback;
    char *end = NULL;
    long x = strtol(v, &end, 10);
    if (end == NULL || *end != '\0' || x <= 0) return fallback;
    return (int)x;
}



#define FROST_CODEC_U32_PATH 0

#define FROST_CODEC_MATRIX_WORDS 128
#define FROST_CODEC_DEFAULT_INNER_REPS 10000
#define FROST_CODEC_DEFAULT_OUTER_ITERS 20

#if FROST_CODEC_U32_PATH
extern void frost_key_encode_u32(uint32_t *out, const uint8_t *in);
extern void frost_key_decode_u32(uint8_t *out, const uint32_t *in);
#else
extern void frost_key_encode(uint16_t *out, const uint16_t *in);
extern void frost_key_decode(uint16_t *out, const uint16_t *in);
#endif

static volatile uint64_t codec_bench_sink = 0;

static uint64_t codec_cycle_diff(uint64_t start, uint64_t end)
{
    return (end >= start) ? (end - start) : ((UINT64_MAX - start) + end + 1u);
}

static uint64_t codec_checksum_bytes(const uint8_t *buf, size_t len)
{
    uint64_t acc = 0x9E3779B97F4A7C15ull;
    for (size_t i = 0; i < len; i++) acc = (acc << 5) ^ (acc >> 2) ^ buf[i];
    return acc;
}

#if FROST_CODEC_U32_PATH
static uint64_t codec_checksum_words(const uint32_t *buf, size_t len)
#else
static uint64_t codec_checksum_words(const uint16_t *buf, size_t len)
#endif
{
    uint64_t acc = 0xD1B54A32D192ED03ull;
    for (size_t i = 0; i < len; i++) acc = (acc << 7) ^ (acc >> 3) ^ (uint64_t)buf[i];
    return acc;
}

static int codec_identity_check(const uint8_t *msg)
{
    uint8_t dec[CRYPTO_BYTES];
#if FROST_CODEC_U32_PATH
    uint32_t enc[FROST_CODEC_MATRIX_WORDS];
    frost_key_encode_u32(enc, msg);
    frost_key_decode_u32(dec, enc);
#else
    uint16_t enc[FROST_CODEC_MATRIX_WORDS];
    frost_key_encode(enc, (const uint16_t *)msg);
    frost_key_decode((uint16_t *)dec, enc);
#endif
    return memcmp(msg, dec, CRYPTO_BYTES) == 0;
}

static int codec_microbench(void)
{
    uint8_t msg[CRYPTO_BYTES];
    uint8_t dec[CRYPTO_BYTES];
    const int inner_reps = get_env_int("FROST_CODEC_INNER_REPS", FROST_CODEC_DEFAULT_INNER_REPS);
    const int outer_iters = get_env_int("FROST_CODEC_OUTER_ITERATIONS", FROST_CODEC_DEFAULT_OUTER_ITERS);
#if FROST_CODEC_U32_PATH
    uint32_t enc[FROST_CODEC_MATRIX_WORDS];
    const char *encode_wrapper = "frost_key_encode_u32";
    const char *decode_wrapper = "frost_key_decode_u32";
#else
    uint16_t enc[FROST_CODEC_MATRIX_WORDS];
    const char *encode_wrapper = "frost_key_encode";
    const char *decode_wrapper = "frost_key_decode";
#endif

    for (size_t i = 0; i < sizeof(msg); i++) msg[i] = (uint8_t)(0xA5u ^ (uint8_t)(31u * i));
    if (!codec_identity_check(msg)) {
        fprintf(stderr, "codec microbench identity check failed message_codec=%s\n", frost_message_codec_name());
        return false;
    }

#if FROST_CODEC_U32_PATH
    frost_key_encode_u32(enc, msg);
#else
    frost_key_encode(enc, (const uint16_t *)msg);
#endif

    for (int outer = 0; outer < outer_iters; outer++) {
        uint64_t start = rdtsc();
        for (int i = 0; i < inner_reps; i++) {
#if FROST_CODEC_U32_PATH
            frost_key_encode_u32(enc, msg);
#else
            frost_key_encode(enc, (const uint16_t *)msg);
#endif
        }
        uint64_t end = rdtsc();
        uint64_t checksum = codec_checksum_words(enc, FROST_CODEC_MATRIX_WORDS);
        codec_bench_sink ^= checksum;
        printf("[codec-bench] message_codec=%s component=message_encode cycles=%llu outer_iteration=%d inner_reps=%d checksum=%llu wrapper=%s status=ok\n",
               frost_message_codec_name(), (unsigned long long)(codec_cycle_diff(start, end) / (uint64_t)inner_reps),
               outer, inner_reps, (unsigned long long)checksum, encode_wrapper);

        start = rdtsc();
        for (int i = 0; i < inner_reps; i++) {
#if FROST_CODEC_U32_PATH
            frost_key_decode_u32(dec, enc);
#else
            frost_key_decode((uint16_t *)dec, enc);
#endif
        }
        end = rdtsc();
        checksum = codec_checksum_bytes(dec, sizeof(dec));
        codec_bench_sink ^= checksum;
        printf("[codec-bench] message_codec=%s component=message_decode cycles=%llu outer_iteration=%d inner_reps=%d checksum=%llu wrapper=%s status=ok\n",
               frost_message_codec_name(), (unsigned long long)(codec_cycle_diff(start, end) / (uint64_t)inner_reps),
               outer, inner_reps, (unsigned long long)checksum, decode_wrapper);
    }
    return true;
}

static int has_arg(int argc, char **argv, const char *needle)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(needle, argv[i]) == 0) return true;
    }
    return false;
}

static int kem_test(const char *named_parameters, int iterations)
{
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ss_encap[CRYPTO_BYTES], ss_decap[CRYPTO_BYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    unsigned char bytes[4];
    uint32_t* pos = (uint32_t*)bytes;
    uint8_t Fin[CRYPTO_CIPHERTEXTBYTES + CRYPTO_BYTES];

    #ifdef DO_VALGRIND_CHECK
        if (!RUNNING_ON_VALGRIND) {
            fprintf(stderr, "This test can only usefully be run inside valgrind.\n");
            fprintf(stderr, "valgrind Frost/frostcc128/test_KEM (or frostcc192 or frostcc256)\n");
            exit(1);
        }
    #endif

    printf("\n");
    printf("=============================================================================================================================\n");
    printf("Testing correctness of key encapsulation mechanism (KEM), system %s, tests for %d iterations\n", named_parameters, iterations);
    printf("=============================================================================================================================\n");

    for (int i = 0; i < iterations; i++) {
        if (crypto_kem_keypair(pk, sk) != 0) {
            printf("\n ERROR -- key generation failed!\n");
            return false;
        }
        if (crypto_kem_enc(ct, ss_encap, pk) != 0) {
            printf("\n ERROR -- encapsulation mechanism failed!\n");
            return false;
        }
        crypto_kem_dec(ss_decap, ct, sk);
#ifdef DO_VALGRIND_CHECK
        VALGRIND_MAKE_MEM_DEFINED(ss_encap, CRYPTO_BYTES);
        VALGRIND_MAKE_MEM_DEFINED(ss_decap, CRYPTO_BYTES);
#endif
        if (memcmp(ss_encap, ss_decap, CRYPTO_BYTES) != 0) {
            printf("\n ERROR -- encapsulation/decapsulation mechanism failed!\n");
	        return false;
        }

        // Testing decapsulation after changing random bits of a random 16-bit digit of ct
        randombytes(bytes, 4);
        *pos %= CRYPTO_CIPHERTEXTBYTES/2;
        if (*pos == 0) {
            *pos = 1;
        }
        ((uint16_t*)ct)[*pos] ^= *pos;
        crypto_kem_dec(ss_decap, ct, sk);
#ifdef DO_VALGRIND_CHECK
        VALGRIND_MAKE_MEM_DEFINED(ss_decap, CRYPTO_BYTES);
#endif

        // Compute ss = F(ct || s) with modified ct
        memcpy(Fin, ct, CRYPTO_CIPHERTEXTBYTES);
        memcpy(&Fin[CRYPTO_CIPHERTEXTBYTES], sk, CRYPTO_BYTES);
        shake(ss_encap, CRYPTO_BYTES, Fin, CRYPTO_CIPHERTEXTBYTES + CRYPTO_BYTES);

#ifdef DO_VALGRIND_CHECK
        VALGRIND_MAKE_MEM_DEFINED(ss_encap, CRYPTO_BYTES);
#endif
        if (memcmp(ss_encap, ss_decap, CRYPTO_BYTES) != 0) {
            printf("\n ERROR -- changing random bits of the ciphertext should cause a failure!\n");
	        return false;
        }
    }
    printf("Tests PASSED. All session keys matched.\n");
    printf("\n\n");

    return true;
}


static void kem_bench(const int seconds)
{
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ss_encap[CRYPTO_BYTES], ss_decap[CRYPTO_BYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];

    TIME_OPERATION_SECONDS({ crypto_kem_keypair(pk, sk); }, "Key generation", seconds);

    crypto_kem_keypair(pk, sk);
    TIME_OPERATION_SECONDS({ crypto_kem_enc(ct, ss_encap, pk); }, "KEM encapsulate", seconds);

    crypto_kem_enc(ct, ss_encap, pk);
    TIME_OPERATION_SECONDS({ crypto_kem_dec(ss_decap, ct, sk); }, "KEM decapsulate", seconds);
}


int main(int argc, char **argv)
{
    if (has_arg(argc, argv, "codec")) {
        printf("message_codec=%s\n", frost_message_codec_name());
        return EXIT_SUCCESS;
    }
    int OK = true;
    int test_iterations = get_env_int("FROST_KEM_TEST_ITERATIONS", KEM_TEST_ITERATIONS);
    int bench_seconds = get_env_int("FROST_KEM_BENCH_SECONDS", KEM_BENCH_SECONDS);

    OK = kem_test(SYSTEM_NAME, test_iterations);
    if (OK != true) {
        goto exit;
    }

    if (has_arg(argc, argv, "codecbench")) {
        OK = codec_microbench();
        if (OK != true) {
            goto exit;
        }
    }

    if (has_arg(argc, argv, "nobench")) {}
    else {
        PRINT_TIMER_HEADER
        kem_bench(bench_seconds);
    }

exit:
    return (OK == true) ? EXIT_SUCCESS : EXIT_FAILURE;
}
