/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include "KEX_LoomKEX-256.h"
#include "drng.h"
#include "state_serialize.h"

/* ---------------------------------------------------------------
 *      Set Parties' ID strings (length <= LOOM_MAX_IDBYTES = 32)
 * --------------------------------------------------------------- */
#define ID_INITIATOR "initiator"
#define ID_RESPONDER "responder"
#define ID_INITIATOR_LEN (strlen(ID_INITIATOR))
#define ID_RESPONDER_LEN (strlen(ID_RESPONDER))

/* ---------------------------------------------------------------
 * Algorithm source files (reference implementation)
 * --------------------------------------------------------------- */
#include "params.h"
#include "api.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* ---------------------------------------------------------------
 * Global SM3-DRNG context, declared extern in drng.h / KAT_KEM.c
 * --------------------------------------------------------------- */
extern DRNG_ctx drng_algorithm;

static int kex_pack_state(unsigned char *buf, unsigned long long *buf_len,
                          LOOM_KEX_CTX *ctx, uint8_t role, uint8_t stage)
{
    size_t packed_len = 0;
    int ret;

    if (!buf || !buf_len || !ctx) {
        return -1;
    }
    if (*buf_len < loom_kex_state_packed_len_bytes()) {
        return -2;
    }

    ret = loom_kex_ctx_pack(buf, (size_t)*buf_len, &packed_len, ctx, role, stage);
    if (ret) {
        return ret;
    }

    *buf_len = (unsigned long long)packed_len;
    loom_kex_ctx_secure_wipe(ctx);
    destroy_ctx(ctx);
    return 0;
}

static int kex_unpack_state(LOOM_KEX_CTX **ctx_out,
                            const unsigned char *buf, unsigned long long buf_len,
                            uint8_t role, uint8_t stage,
                            unsigned char *self_pk, unsigned char *self_sk)
{
    LOOM_KEX_CTX *ctx = NULL;
    int ret;

    if (!ctx_out || !buf || !self_sk) {
        return -1;
    }

    ctx = create_ctx();
    if (!ctx) {
        return -2;
    }

    ret = loom_kex_ctx_unpack(ctx, buf, (size_t)buf_len, role, stage, self_pk, self_sk);
    if (ret) {
        loom_kex_ctx_secure_wipe(ctx);
        destroy_ctx(ctx);
        return ret;
    }

    *ctx_out = ctx;
    return 0;
}

unsigned long long kex_get_passes_num()
{
	return 4;
}

unsigned long long kex_get_pk_len_bytes()
{
	return CRYPTO_LOOM_PUBLICKEYBYTES;
}

unsigned long long kex_get_sk_len_bytes()
{
	return CRYPTO_LOOM_SECRETKEYBYTES;
}

unsigned long long kex_get_sta_len_bytes()
{
	return loom_kex_state_packed_len_bytes();
}

unsigned long long kex_get_stb_len_bytes()
{
	return loom_kex_state_packed_len_bytes();
}

unsigned long long kex_get_ss_len_bytes()
{
	return CRYPTO_LOOM_SSBYTES;
}

unsigned long long kex_get_total_msg_len_bytes()
{
	return (CRYPTO_LOOM_MSG1BYTES + CRYPTO_LOOM_MSG2BYTES + CRYPTO_LOOM_MSG3BYTES + CRYPTO_LOOM_MSG4BYTES);
}

int kex_init_a(
	unsigned char *pka, unsigned long long *pka_len_bytes,
	unsigned char *ska, unsigned long long *ska_len_bytes,
	unsigned char *sta, unsigned long long *sta_len_bytes)
{
    LOOM_KEX_CTX *init = NULL;
    int ret = -1;

    if(!pka || !pka_len_bytes || !ska || !ska_len_bytes || !sta || !sta_len_bytes)
    {
        goto end;
    }

    init = create_ctx();
    if(init == NULL) {
        printf("**Error: create_ctx\n");
        goto end;
    }
    
    ret = crypto_loom_initialize_state(init, pka, ska, LOOM_INITIATOR, (const uint8_t*)ID_INITIATOR, ID_INITIATOR_LEN);
    if(ret) {
        printf("**Error: crypto_loom_initialize_state [%d]\n", ret);
        goto end;
    }
    *pka_len_bytes = kex_get_pk_len_bytes();
    *ska_len_bytes = kex_get_sk_len_bytes();
    *sta_len_bytes = kex_get_sta_len_bytes();
    ret = kex_pack_state(sta, sta_len_bytes, init, LOOM_STATE_ROLE_INITIATOR, LOOM_STATE_STAGE_INIT);

end:
	return ret;
}

int kex_init_b(
	unsigned char *pkb, unsigned long long *pkb_len_bytes,
	unsigned char *skb, unsigned long long *skb_len_bytes,
	unsigned char *stb, unsigned long long *stb_len_bytes)
{
    LOOM_KEX_CTX *resp = NULL;
    int ret = -1;

    if(!pkb || !pkb_len_bytes || !skb || !skb_len_bytes || !stb || !stb_len_bytes)
    {
        goto end;
    }

    resp = create_ctx();
    if(resp == NULL) {
        printf("**Error: create_ctx\n");
        goto end;
    }
    
    ret = crypto_loom_initialize_state(resp, pkb, skb, LOOM_RESPONDER, (const uint8_t*)ID_RESPONDER, ID_RESPONDER_LEN);
    if(ret) {
        printf("**Error: crypto_loom_initialize_state [%d]\n", ret);
        goto end;
    }

    *pkb_len_bytes = kex_get_pk_len_bytes();
    *skb_len_bytes = kex_get_sk_len_bytes();
    *stb_len_bytes = kex_get_stb_len_bytes();
    ret = kex_pack_state(stb, stb_len_bytes, resp, LOOM_STATE_ROLE_RESPONDER, LOOM_STATE_STAGE_INIT);

end:
	return ret;
}

int kex_generate_pass1_msg_a(
	unsigned char *ska, unsigned long long ska_len_bytes,
	unsigned char *pkb, unsigned long long pkb_len_bytes,
	unsigned char *sta, unsigned long long *sta_len_bytes,
	unsigned char *m1, unsigned long long *m1_len_bytes)
{
    LOOM_KEX_CTX *init = NULL;
    int ret = -1;

    (void)ska_len_bytes;
    (void)pkb_len_bytes;

    if(!ska || !pkb || !sta || !sta_len_bytes || !m1 || !m1_len_bytes)
    {
        goto end;
    }

    ret = kex_unpack_state(&init, sta, *sta_len_bytes,
                           LOOM_STATE_ROLE_INITIATOR, LOOM_STATE_STAGE_INIT,
                           NULL, ska);
    if (ret) {
        printf("**Error: unpack initiator init state [%d]\n", ret);
        goto end;
    }

    ret = crypto_loom_init(init, m1);
    if(ret) {
        printf("**Error:1 crypto_loom_init [%d]\n", ret);
        loom_kex_ctx_secure_wipe(init);
        destroy_ctx(init);
        goto end;
    }
    *m1_len_bytes = CRYPTO_LOOM_MSG1BYTES;

    *sta_len_bytes = kex_get_sta_len_bytes();
    ret = kex_pack_state(sta, sta_len_bytes, init, LOOM_STATE_ROLE_INITIATOR, LOOM_STATE_STAGE_PASS1);
    
end:
	return ret;
}

int kex_generate_pass2_msg_b(
	unsigned char *skb, unsigned long long skb_len_bytes,
	unsigned char *pka, unsigned long long pka_len_bytes,
	unsigned char *m1, unsigned long long m1_len_bytes,
	unsigned char *stb, unsigned long long *stb_len_bytes,
	unsigned char *m2, unsigned long long *m2_len_bytes)
{
    LOOM_KEX_CTX *resp = NULL;
    int ret = -1;

    (void)skb_len_bytes;
    (void)pka_len_bytes;
    (void)m1_len_bytes;

	if(!skb || !pka || !m1 || !stb || !stb_len_bytes || !m2 || !m2_len_bytes)
    {
        goto end;
    }

    ret = kex_unpack_state(&resp, stb, *stb_len_bytes,
                           LOOM_STATE_ROLE_RESPONDER, LOOM_STATE_STAGE_INIT,
                           NULL, skb);
    if (ret) {
        printf("**Error: unpack responder init state [%d]\n", ret);
        goto end;
    }

    ret = crypto_loom_respond(resp, m1, m2);
    if(ret) {
        printf("**Error:2 crypto_loom_respond [%d]\n", ret);
        loom_kex_ctx_secure_wipe(resp);
        destroy_ctx(resp);
        goto end;
    }
    *m2_len_bytes = CRYPTO_LOOM_MSG2BYTES;

    *stb_len_bytes = kex_get_stb_len_bytes();
    ret = kex_pack_state(stb, stb_len_bytes, resp, LOOM_STATE_ROLE_RESPONDER, LOOM_STATE_STAGE_PASS2);
    
end:
	return ret;
}

int kex_generate_pass3_msg_a(
	unsigned char *ska, unsigned long long ska_len_bytes,
	unsigned char *pkb, unsigned long long pkb_len_bytes,
	unsigned char *m2, unsigned long long m2_len_bytes,
	unsigned char *sta, unsigned long long *sta_len_bytes,
	unsigned char *m3, unsigned long long *m3_len_bytes)
{
    LOOM_KEX_CTX *init = NULL;
    int ret = -1;

    (void)ska_len_bytes;
    (void)pkb_len_bytes;
    (void)m2_len_bytes;

	if(!ska || !pkb || !m2 || !sta || !sta_len_bytes || !m3 || !m3_len_bytes)
	{
        goto end;
	}

    ret = kex_unpack_state(&init, sta, *sta_len_bytes,
                           LOOM_STATE_ROLE_INITIATOR, LOOM_STATE_STAGE_PASS1,
                           NULL, ska);
    if (ret) {
        printf("**Error: unpack initiator pass1 state [%d]\n", ret);
        goto end;
    }

    ret = crypto_loom_auth_init(init, m2, m3);
    if(ret) {
        printf("**Error:3 crypto_loom_auth_init [%d]\n", ret);
        loom_kex_ctx_secure_wipe(init);
        destroy_ctx(init);
        goto end;
    }
    *m3_len_bytes = CRYPTO_LOOM_MSG3BYTES;

    *sta_len_bytes = kex_get_sta_len_bytes();
    ret = kex_pack_state(sta, sta_len_bytes, init, LOOM_STATE_ROLE_INITIATOR, LOOM_STATE_STAGE_PASS3);

end:
	return ret;
}


int kex_generate_pass4_msg_b(
	unsigned char *skb, unsigned long long skb_len_bytes,
	unsigned char *pka, unsigned long long pka_len_bytes,
	unsigned char *m3, unsigned long long m3_len_bytes,
	unsigned char *stb, unsigned long long *stb_len_bytes,
	unsigned char *m4, unsigned long long *m4_len_bytes)
{
    LOOM_KEX_CTX *resp = NULL;
    int ret = -1;

    (void)skb_len_bytes;
    (void)pka_len_bytes;
    (void)m3_len_bytes;

	if(!skb || !pka || !m3 || !stb || !stb_len_bytes || !m4 || !m4_len_bytes)
	{
        goto end;
	}

    ret = kex_unpack_state(&resp, stb, *stb_len_bytes,
                           LOOM_STATE_ROLE_RESPONDER, LOOM_STATE_STAGE_PASS2,
                           NULL, skb);
    if (ret) {
        printf("**Error: unpack responder pass2 state [%d]\n", ret);
        goto end;
    }

    ret = crypto_loom_auth_resp(resp, pka, (const uint8_t*)ID_INITIATOR, ID_INITIATOR_LEN, m3, m4);
    if(ret) {
        printf("**Error:4 crypto_loom_auth_resp [%d]\n", ret);
        loom_kex_ctx_secure_wipe(resp);
        destroy_ctx(resp);
        goto end;
    }
    *m4_len_bytes = CRYPTO_LOOM_MSG4BYTES;

    *stb_len_bytes = kex_get_stb_len_bytes();
    ret = kex_pack_state(stb, stb_len_bytes, resp, LOOM_STATE_ROLE_RESPONDER, LOOM_STATE_STAGE_PASS4);
    if (ret) {
        goto end;
    }

    ret = 1;
end:
	return ret; 
}

int kex_derive_ss_a(
	unsigned char *ska, unsigned long long ska_len_bytes,
	unsigned char *pkb, unsigned long long pkb_len_bytes,
	unsigned char *mb, unsigned long long mb_len_bytes,
	unsigned char *sta, unsigned long long sta_len_bytes,
	unsigned char *ssa, unsigned long long *ssa_len_bytes)
{
    LOOM_KEX_CTX *init = NULL;
    int ret = -1;

    (void) ska;
    (void) ska_len_bytes;
    (void) pkb_len_bytes;

	if(!pkb || !mb || !sta || !ssa || !ssa_len_bytes)
	{
        goto end;
	}
    if(mb_len_bytes != CRYPTO_LOOM_MSG4BYTES) {
        printf("**Error: wrong last msg length\n");
        goto end;
    }

    ret = kex_unpack_state(&init, sta, sta_len_bytes,
                           LOOM_STATE_ROLE_INITIATOR, LOOM_STATE_STAGE_PASS3,
                           NULL, ska);
    if (ret) {
        printf("**Error: unpack initiator pass3 state [%d]\n", ret);
        goto end;
    }

    ret = crypto_loom_last_init(init, pkb, (const uint8_t*)ID_RESPONDER, ID_RESPONDER_LEN, mb);
    if(ret) {
        printf("**Error:5 crypto_loom_last_init [%d]\n", ret);
        loom_kex_ctx_secure_wipe(init);
        destroy_ctx(init);
        goto end;
    }
    ret = crypto_loom_finalize(init, ssa, ssa_len_bytes);
    if(ret) {
        printf("**Error: init crypto_loom_finalize [%d]\n", ret);
        loom_kex_ctx_secure_wipe(init);
        destroy_ctx(init);
        goto end;
    }
    loom_kex_ctx_secure_wipe(init);
    destroy_ctx(init);
    ret = 0;
end:
	return ret;
}

int kex_derive_ss_b(
	unsigned char *skb, unsigned long long skb_len_bytes,
	unsigned char *pka, unsigned long long pka_len_bytes,
	unsigned char *ma, unsigned long long ma_len_bytes,
	unsigned char *stb, unsigned long long stb_len_bytes,
	unsigned char *ssb, unsigned long long *ssb_len_bytes)
{
    LOOM_KEX_CTX *resp = NULL;
    int ret = -1;

    (void) skb;
    (void) skb_len_bytes;
    (void) pka;
    (void) ma;
    (void) ma_len_bytes;

    if(!stb || !ssb || !ssb_len_bytes)
	{
        goto end;
	}

    ret = kex_unpack_state(&resp, stb, stb_len_bytes,
                           LOOM_STATE_ROLE_RESPONDER, LOOM_STATE_STAGE_PASS4,
                           NULL, skb);
    if (ret) {
        printf("**Error: unpack responder pass4 state [%d]\n", ret);
        goto end;
    }

    ret = crypto_loom_finalize(resp, ssb, ssb_len_bytes);
    if(ret) {
        printf("**Error: resp crypto_loom_finalize [%d]\n", ret);
        loom_kex_ctx_secure_wipe(resp);
        destroy_ctx(resp);
        goto end;
    }
    loom_kex_ctx_secure_wipe(resp);
    destroy_ctx(resp);
    ret = 0;
end:
	return ret;
}
