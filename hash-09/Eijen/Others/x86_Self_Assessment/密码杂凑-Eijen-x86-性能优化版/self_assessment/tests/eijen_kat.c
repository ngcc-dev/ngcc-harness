#include "eijen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int digest_bits;
    const char *name;
    int kind;
    const char *expected_hex;
} kat_case;

static const kat_case KAT_CASES[] = {
    {256, "zero", 0, "80228A17A23CBEAB2887F1A68F207F64CF37AFDE64829FB7B51A409CE71378A7"},
    {256, "one", 1, "7E3B2A5EE4CF56D3FF4646C9A3577E97059933A754462ECACDE04218D1AE0004"},
    {256, "abc", 2, "C832C8FBDAB9CBB60824D2E879ACE7CEB1F902E54FCAA9DC81E09F3C71B04F86"},
    {384, "zero", 0, "B073FF1383DED03B501800B493D0A9D980228A17A23CBEAB2887F1A68F207F64CF37AFDE64829FB7B51A409CE71378A7"},
    {384, "one", 1, "AAB7CF11B814EB943A3702838F26095DA09ADCDBBBE5233724CCEE1FA4DC4B4DA6A3EA682BF08970BF19275E9FABBEFD"},
    {384, "abc", 2, "A03123E2B9D22A2302B49E012D880194C832C8FBDAB9CBB60824D2E879ACE7CEB1F902E54FCAA9DC81E09F3C71B04F86"},
    {512, "zero", 0, "20720A80BCA293539A5310A46EFA2926B073FF1383DED03B501800B493D0A9D980228A17A23CBEAB2887F1A68F207F64CF37AFDE64829FB7B51A409CE71378A7"},
    {512, "one", 1, "3B175FF348C41499955B22A6E54982D808847F0E2FE7C269B51E1DCA1C1FF9C825897C9F871AFFE411E0B646553C2F16C2E5DD95F186CA013D4F2019FBAA0174"},
    {512, "abc", 2, "B0F1FC89DFF8997405CEAE73C34D9A97A03123E2B9D22A2302B49E012D880194C832C8FBDAB9CBB60824D2E879ACE7CEB1F902E54FCAA9DC81E09F3C71B04F86"},
    {768, "zero", 0, "C3DD02B9FE08D75E316A87479AE6A8F47CB4F076F3ED6B07849B986B2CF1689B20720A80BCA293539A5310A46EFA2926B073FF1383DED03B501800B493D0A9D980228A17A23CBEAB2887F1A68F207F64CF37AFDE64829FB7B51A409CE71378A7"},
    {768, "one", 1, "20C2AF953AA666FED2EFC6FE70D832C9E77E830538FCE9F202EC29ED5A4AA7BA13FB284B9AFA00840A2C31EF6796BC6C729A14AD6770E1F1D25DA3BAA5EB3C4BDCF5767C3EB3637AAC725CD8211C9C71D8D88CBA9CC08757D9D8E74F1EC4789D"},
    {768, "abc", 2, "FC9E471C3278C9416638EE7D845FF6F1CE5EF11EABAAB83B444CB95EAC455791B0F1FC89DFF8997405CEAE73C34D9A97A03123E2B9D22A2302B49E012D880194C832C8FBDAB9CBB60824D2E879ACE7CEB1F902E54FCAA9DC81E09F3C71B04F86"},
    {1024, "zero", 0, "B3F4CB5B73AB79B20EDC62546C428718A1049A487EF76599FC96E7B6494278A9C3DD02B9FE08D75E316A87479AE6A8F47CB4F076F3ED6B07849B986B2CF1689B20720A80BCA293539A5310A46EFA2926B073FF1383DED03B501800B493D0A9D980228A17A23CBEAB2887F1A68F207F64CF37AFDE64829FB7B51A409CE71378A7"},
    {1024, "one", 1, "10CA8AA88377EB4A7487213EE0384BBA5CBB5A9300C000CE9192DE12D5D81E85535A50AAD23556E4F795F11F73702C7BFF42DB838BE183B1B1A31D9C96B025BD47B0CD040C6B45CCBC54F8D7B3BEB6CE4C4D2BBDACB1D7EF7918BD2E0912F86E895BFDE5EB24DCFA1EE5124527E17473B73B1C6FFE41F7810CF8C022BA818EEA"},
    {1024, "abc", 2, "738521FE5D9A3EB4A08418A650071F29D83208984325304064AAE1A2402E5B30FC9E471C3278C9416638EE7D845FF6F1CE5EF11EABAAB83B444CB95EAC455791B0F1FC89DFF8997405CEAE73C34D9A97A03123E2B9D22A2302B49E012D880194C832C8FBDAB9CBB60824D2E879ACE7CEB1F902E54FCAA9DC81E09F3C71B04F86"}
};

static int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static int hex_to_bytes(const char *hex, uint8_t *out, size_t out_len) {
    size_t i;
    if (strlen(hex) != out_len * 2U) {
        return -1;
    }
    for (i = 0; i < out_len; i++) {
        int hi = hex_value(hex[2U * i]);
        int lo = hex_value(hex[2U * i + 1U]);
        if (hi < 0 || lo < 0) {
            return -1;
        }
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return 0;
}

static void fill_message(const kat_case *tc, uint8_t *msg, size_t *len) {
    size_t rate = eijen_rate_size(tc->digest_bits);
    memset(msg, 0, EIJEN_MAX_RATE_BYTES);
    if (tc->kind == 0) {
        *len = rate;
    } else if (tc->kind == 1) {
        memset(msg, 0xff, rate);
        *len = rate;
    } else {
        msg[0] = (uint8_t)'a';
        msg[1] = (uint8_t)'b';
        msg[2] = (uint8_t)'c';
        *len = 3U;
    }
}

static int run_one_shot(const kat_case *tc) {
    uint8_t msg[EIJEN_MAX_RATE_BYTES];
    uint8_t got[EIJEN_MAX_DIGEST_BYTES];
    uint8_t expected[EIJEN_MAX_DIGEST_BYTES];
    size_t len;
    size_t digest_len = eijen_digest_size(tc->digest_bits);

    fill_message(tc, msg, &len);
    if (hex_to_bytes(tc->expected_hex, expected, digest_len) != 0) {
        fprintf(stderr, "bad expected hex for Eijen-%d %s\n", tc->digest_bits, tc->name);
        return 1;
    }
    if (eijen_hash(tc->digest_bits, msg, len, got) != 0) {
        fprintf(stderr, "hash failed for Eijen-%d %s\n", tc->digest_bits, tc->name);
        return 1;
    }
    if (memcmp(got, expected, digest_len) != 0) {
        fprintf(stderr, "KAT mismatch for Eijen-%d %s\n", tc->digest_bits, tc->name);
        return 1;
    }
    return 0;
}

static int run_streaming_abc(int digest_bits) {
    eijen_ctx ctx;
    uint8_t got[EIJEN_MAX_DIGEST_BYTES];
    uint8_t one;
    uint8_t expected[EIJEN_MAX_DIGEST_BYTES];
    const kat_case *tc = NULL;
    size_t i;
    for (i = 0; i < sizeof(KAT_CASES) / sizeof(KAT_CASES[0]); i++) {
        if (KAT_CASES[i].digest_bits == digest_bits && KAT_CASES[i].kind == 2) {
            tc = &KAT_CASES[i];
            break;
        }
    }
    if (tc == NULL) {
        return 1;
    }
    if (hex_to_bytes(tc->expected_hex, expected, eijen_digest_size(digest_bits)) != 0) {
        return 1;
    }
    if (eijen_init(&ctx, digest_bits) != 0) {
        return 1;
    }
    one = (uint8_t)'a';
    if (eijen_update(&ctx, &one, 1U) != 0) return 1;
    one = (uint8_t)'b';
    if (eijen_update(&ctx, &one, 1U) != 0) return 1;
    one = (uint8_t)'c';
    if (eijen_update(&ctx, &one, 1U) != 0) return 1;
    if (eijen_final(&ctx, got) != 0) return 1;
    return memcmp(got, expected, eijen_digest_size(digest_bits)) == 0 ? 0 : 1;
}

static int run_long_streaming_consistency(int digest_bits) {
    size_t rate = eijen_rate_size(digest_bits);
    size_t lengths[5];
    uint8_t msg[EIJEN_MAX_RATE_BYTES * 3U];
    uint8_t one_shot[EIJEN_MAX_DIGEST_BYTES];
    uint8_t streamed[EIJEN_MAX_DIGEST_BYTES];
    size_t i;
    size_t j;
    size_t digest_len = eijen_digest_size(digest_bits);

    lengths[0] = rate - 1U;
    lengths[1] = rate;
    lengths[2] = rate + 1U;
    lengths[3] = 2U * rate;
    lengths[4] = (2U * rate) + 13U;

    for (i = 0; i < sizeof(msg); i++) {
        msg[i] = (uint8_t)((i * 17U + (size_t)digest_bits) & 0xffU);
    }

    for (i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
        eijen_ctx ctx;
        size_t off = 0U;
        if (eijen_hash(digest_bits, msg, lengths[i], one_shot) != 0) {
            return 1;
        }
        if (eijen_init(&ctx, digest_bits) != 0) {
            return 1;
        }
        j = 1U;
        while (off < lengths[i]) {
            size_t chunk = (j * 7U) % 31U;
            if (chunk == 0U) {
                chunk = 1U;
            }
            if (chunk > lengths[i] - off) {
                chunk = lengths[i] - off;
            }
            if (eijen_update(&ctx, msg + off, chunk) != 0) {
                return 1;
            }
            off += chunk;
            j++;
        }
        if (eijen_final(&ctx, streamed) != 0) {
            return 1;
        }
        if (memcmp(one_shot, streamed, digest_len) != 0) {
            return 1;
        }
    }
    return 0;
}

static int run_bit_length_consistency(int digest_bits) {
    uint8_t msg_a[8];
    uint8_t msg_b[8];
    uint8_t byte_hash[EIJEN_MAX_DIGEST_BYTES];
    uint8_t bit_hash[EIJEN_MAX_DIGEST_BYTES];
    uint8_t partial_a[EIJEN_MAX_DIGEST_BYTES];
    uint8_t partial_b[EIJEN_MAX_DIGEST_BYTES];
    size_t i;
    size_t digest_len = eijen_digest_size(digest_bits);

    for (i = 0; i < sizeof(msg_a); i++) {
        msg_a[i] = (uint8_t)(0x31U + (i * 19U));
        msg_b[i] = msg_a[i];
    }
    if (eijen_hash(digest_bits, msg_a, 3U, byte_hash) != 0) {
        return 1;
    }
    if (eijen_hash_bits(digest_bits, msg_a, 24ULL, bit_hash) != 0) {
        return 1;
    }
    if (memcmp(byte_hash, bit_hash, digest_len) != 0) {
        return 1;
    }

    msg_a[1] = 0xa0U;
    msg_b[1] = 0xbfU;
    if (eijen_hash_bits(digest_bits, msg_a, 11ULL, partial_a) != 0) {
        return 1;
    }
    if (eijen_hash_bits(digest_bits, msg_b, 11ULL, partial_b) != 0) {
        return 1;
    }
    return memcmp(partial_a, partial_b, digest_len) == 0 ? 0 : 1;
}

#ifdef EIJEN_HAS_HASH4
static int run_hash4_test(int digest_bits) {
    uint8_t msg0[32];
    uint8_t msg1[32];
    uint8_t msg2[32];
    uint8_t msg3[32];
    uint8_t out0[EIJEN_MAX_DIGEST_BYTES];
    uint8_t out1[EIJEN_MAX_DIGEST_BYTES];
    uint8_t out2[EIJEN_MAX_DIGEST_BYTES];
    uint8_t out3[EIJEN_MAX_DIGEST_BYTES];
    uint8_t exp0[EIJEN_MAX_DIGEST_BYTES];
    uint8_t exp1[EIJEN_MAX_DIGEST_BYTES];
    uint8_t exp2[EIJEN_MAX_DIGEST_BYTES];
    uint8_t exp3[EIJEN_MAX_DIGEST_BYTES];
    size_t i;
    size_t digest_len = eijen_digest_size(digest_bits);

    for (i = 0; i < sizeof(msg0); i++) {
        msg0[i] = (uint8_t)i;
        msg1[i] = (uint8_t)(0xa5U ^ i);
        msg2[i] = (uint8_t)(0x5aU + i);
        msg3[i] = (uint8_t)(0xffU - i);
    }
    if (eijen_hash(digest_bits, msg0, sizeof(msg0), exp0) != 0) return 1;
    if (eijen_hash(digest_bits, msg1, sizeof(msg1), exp1) != 0) return 1;
    if (eijen_hash(digest_bits, msg2, sizeof(msg2), exp2) != 0) return 1;
    if (eijen_hash(digest_bits, msg3, sizeof(msg3), exp3) != 0) return 1;
    if (eijen_hash4_same(digest_bits, msg0, msg1, msg2, msg3, sizeof(msg0),
                         out0, out1, out2, out3) != 0) {
        return 1;
    }
    return memcmp(out0, exp0, digest_len) == 0 &&
           memcmp(out1, exp1, digest_len) == 0 &&
           memcmp(out2, exp2, digest_len) == 0 &&
           memcmp(out3, exp3, digest_len) == 0 ? 0 : 1;
}
#endif

int main(void) {
    size_t i;
    int failures = 0;
    int digest_bits[] = {256, 384, 512, 768, 1024};

    printf("implementation: %s\n", eijen_implementation_name());
    for (i = 0; i < sizeof(KAT_CASES) / sizeof(KAT_CASES[0]); i++) {
        int failed = run_one_shot(&KAT_CASES[i]);
        printf("%s Eijen-%d %s one-shot\n",
               failed ? "FAIL" : "OK", KAT_CASES[i].digest_bits, KAT_CASES[i].name);
        failures += failed;
    }
    for (i = 0; i < sizeof(digest_bits) / sizeof(digest_bits[0]); i++) {
        int failed = run_streaming_abc(digest_bits[i]);
        printf("%s Eijen-%d abc streaming\n", failed ? "FAIL" : "OK", digest_bits[i]);
        failures += failed;
    }
    for (i = 0; i < sizeof(digest_bits) / sizeof(digest_bits[0]); i++) {
        int failed = run_long_streaming_consistency(digest_bits[i]);
        printf("%s Eijen-%d long streaming consistency\n",
               failed ? "FAIL" : "OK", digest_bits[i]);
        failures += failed;
    }
    for (i = 0; i < sizeof(digest_bits) / sizeof(digest_bits[0]); i++) {
        int failed = run_bit_length_consistency(digest_bits[i]);
        printf("%s Eijen-%d bit-length API consistency\n",
               failed ? "FAIL" : "OK", digest_bits[i]);
        failures += failed;
    }
#ifdef EIJEN_HAS_HASH4
    for (i = 0; i < sizeof(digest_bits) / sizeof(digest_bits[0]); i++) {
        int failed = run_hash4_test(digest_bits[i]);
        printf("%s Eijen-%d hash4 same-len short messages\n",
               failed ? "FAIL" : "OK", digest_bits[i]);
        failures += failed;
    }
#endif
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
