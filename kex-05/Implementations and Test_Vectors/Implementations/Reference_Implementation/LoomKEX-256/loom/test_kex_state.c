#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "drng.h"
#include "KEX_LoomKEX-256.h"
#include "state_serialize.h"
#include "params.h"
#include "api.h"
#include "loom_loc.h"

extern DRNG_ctx drng_algorithm;
DRNG_ctx drng_algorithm;

#define ID_INITIATOR "initiator"
#define ID_RESPONDER "responder"
#define ID_INITIATOR_LEN (sizeof(ID_INITIATOR) - 1)
#define ID_RESPONDER_LEN (sizeof(ID_RESPONDER) - 1)

static int fail(const char *msg)
{
    fprintf(stderr, "test_kex_state: FAIL %s\n", msg);
    return 1;
}

static int test_roundtrip_stage(LOOM_KEX_CTX *ctx, uint8_t role, uint8_t stage)
{
    uint8_t buf_a[16384];
    uint8_t buf_b[16384];
    size_t len_a = 0;
    size_t len_b = 0;
    LOOM_KEX_CTX *restored = NULL;
    int ret;

    len_a = sizeof(buf_a);
    ret = loom_kex_ctx_pack(buf_a, sizeof(buf_a), &len_a, ctx, role, stage);
    if (ret) {
        return fail("pack");
    }

    restored = create_ctx();
    if (!restored) {
        return fail("create_ctx");
    }
    ret = loom_kex_ctx_unpack(restored, buf_a, len_a, role, stage, NULL,
                              ctx->authenticator.self_privkey);
    if (ret) {
        destroy_ctx(restored);
        return fail("unpack");
    }

    len_b = sizeof(buf_b);
    ret = loom_kex_ctx_pack(buf_b, sizeof(buf_b), &len_b, restored, role, stage);
    destroy_ctx(restored);
    if (ret) {
        return fail("repack");
    }
    if (len_a != len_b || memcmp(buf_a, buf_b, len_a) != 0) {
        return fail("roundtrip bytes differ");
    }
    return 0;
}

static int run_protocol_roundtrips(void)
{
    unsigned char seed[64];
    unsigned char pka[CRYPTO_LOOM_PUBLICKEYBYTES];
    unsigned char ska[CRYPTO_LOOM_SECRETKEYBYTES];
    unsigned char pkb[CRYPTO_LOOM_PUBLICKEYBYTES];
    unsigned char skb[CRYPTO_LOOM_SECRETKEYBYTES];
    unsigned char sta[16384];
    unsigned char stb[16384];
    unsigned char m1[CRYPTO_LOOM_MSG1BYTES];
    unsigned char m2[CRYPTO_LOOM_MSG2BYTES];
    unsigned char m3[CRYPTO_LOOM_MSG3BYTES];
    unsigned char m4[CRYPTO_LOOM_MSG4BYTES];
    unsigned long long pka_len, ska_len, pkb_len, skb_len;
    unsigned long long sta_len, stb_len, m1_len, m2_len, m3_len, m4_len;
    LOOM_KEX_CTX *ctx = NULL;
    int ret;

    memset(seed, 0xA5, sizeof(seed));
    init_random_number(&drng_algorithm, seed, sizeof(seed));

    pka_len = ska_len = pkb_len = skb_len = 0;
    sta_len = stb_len = 0;
    ret = kex_init_a(pka, &pka_len, ska, &ska_len, sta, &sta_len);
    if (ret) {
        return fail("kex_init_a");
    }
    ctx = create_ctx();
    ret = loom_kex_ctx_unpack(ctx, sta, (size_t)sta_len,
                              LOOM_STATE_ROLE_INITIATOR, LOOM_STATE_STAGE_INIT,
                              pka, ska);
    if (ret || test_roundtrip_stage(ctx, LOOM_STATE_ROLE_INITIATOR, LOOM_STATE_STAGE_INIT)) {
        destroy_ctx(ctx);
        return 1;
    }
    destroy_ctx(ctx);

    ret = kex_init_b(pkb, &pkb_len, skb, &skb_len, stb, &stb_len);
    if (ret) {
        return fail("kex_init_b");
    }

    m1_len = 0;
    sta_len = kex_get_sta_len_bytes();
    ret = kex_generate_pass1_msg_a(ska, ska_len, pkb, pkb_len, sta, &sta_len, m1, &m1_len);
    if (ret) {
        return fail("pass1");
    }
    ctx = create_ctx();
    ret = loom_kex_ctx_unpack(ctx, sta, (size_t)sta_len,
                              LOOM_STATE_ROLE_INITIATOR, LOOM_STATE_STAGE_PASS1,
                              NULL, ska);
    if (ret || test_roundtrip_stage(ctx, LOOM_STATE_ROLE_INITIATOR, LOOM_STATE_STAGE_PASS1)) {
        destroy_ctx(ctx);
        return 1;
    }
    destroy_ctx(ctx);

    m2_len = 0;
    stb_len = kex_get_stb_len_bytes();
    ret = kex_generate_pass2_msg_b(skb, skb_len, pka, pka_len, m1, m1_len, stb, &stb_len, m2, &m2_len);
    if (ret) {
        return fail("pass2");
    }
    ctx = create_ctx();
    ret = loom_kex_ctx_unpack(ctx, stb, (size_t)stb_len,
                              LOOM_STATE_ROLE_RESPONDER, LOOM_STATE_STAGE_PASS2,
                              NULL, skb);
    if (ret || test_roundtrip_stage(ctx, LOOM_STATE_ROLE_RESPONDER, LOOM_STATE_STAGE_PASS2)) {
        destroy_ctx(ctx);
        return 1;
    }
    destroy_ctx(ctx);

    m3_len = 0;
    sta_len = kex_get_sta_len_bytes();
    ret = kex_generate_pass3_msg_a(ska, ska_len, pkb, pkb_len, m2, m2_len, sta, &sta_len, m3, &m3_len);
    if (ret) {
        return fail("pass3");
    }
    ctx = create_ctx();
    ret = loom_kex_ctx_unpack(ctx, sta, (size_t)sta_len,
                              LOOM_STATE_ROLE_INITIATOR, LOOM_STATE_STAGE_PASS3,
                              NULL, ska);
    if (ret || test_roundtrip_stage(ctx, LOOM_STATE_ROLE_INITIATOR, LOOM_STATE_STAGE_PASS3)) {
        destroy_ctx(ctx);
        return 1;
    }
    destroy_ctx(ctx);

    m4_len = 0;
    stb_len = kex_get_stb_len_bytes();
    ret = kex_generate_pass4_msg_b(skb, skb_len, pka, pka_len, m3, m3_len, stb, &stb_len, m4, &m4_len);
    if (ret != 1) {
        return fail("pass4");
    }
    ctx = create_ctx();
    ret = loom_kex_ctx_unpack(ctx, stb, (size_t)stb_len,
                              LOOM_STATE_ROLE_RESPONDER, LOOM_STATE_STAGE_PASS4,
                              NULL, skb);
    if (ret || test_roundtrip_stage(ctx, LOOM_STATE_ROLE_RESPONDER, LOOM_STATE_STAGE_PASS4)) {
        destroy_ctx(ctx);
        return 1;
    }
    destroy_ctx(ctx);
    return 0;
}

int main(void)
{
    if (run_protocol_roundtrips() != 0) {
        return 1;
    }
    printf("test_kex_state: PASS\n");
    return 0;
}
