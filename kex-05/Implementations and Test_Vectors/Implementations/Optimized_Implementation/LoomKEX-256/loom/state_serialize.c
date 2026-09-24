#include "state_serialize.h"
#include "loom_loc.h"
#include "params.h"
#include <string.h>
#include <stdlib.h>

void loom_store_u32_le(uint8_t out[4], uint32_t x)
{
    out[0] = (uint8_t)(x & 0xFFU);
    out[1] = (uint8_t)((x >> 8) & 0xFFU);
    out[2] = (uint8_t)((x >> 16) & 0xFFU);
    out[3] = (uint8_t)((x >> 24) & 0xFFU);
}

uint32_t loom_load_u32_le(const uint8_t in[4])
{
    return ((uint32_t)in[0])
         | ((uint32_t)in[1] << 8)
         | ((uint32_t)in[2] << 16)
         | ((uint32_t)in[3] << 24);
}

void loom_store_i32_le(uint8_t out[4], int32_t x)
{
    loom_store_u32_le(out, (uint32_t)x);
}

int32_t loom_load_i32_le(const uint8_t in[4])
{
    return (int32_t)loom_load_u32_le(in);
}

int loom_write_field(uint8_t *out, size_t outcap, size_t *off,
                     const uint8_t *buf, size_t len)
{
    if (!out || !off || (!buf && len != 0)) {
        return -1;
    }
    if (*off > outcap || len > outcap - *off) {
        return -2;
    }
    if (len > 0) {
        memcpy(out + *off, buf, len);
    }
    *off += len;
    return 0;
}

int loom_read_field(const uint8_t *in, size_t inlen, size_t *off,
                    uint8_t *buf, size_t expected_len)
{
    if (!in || !off || !buf) {
        return -1;
    }
    if (*off > inlen || expected_len > inlen - *off) {
        return -2;
    }
    memcpy(buf, in + *off, expected_len);
    *off += expected_len;
    return 0;
}

void loom_secure_zero(void *ptr, size_t len)
{
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    size_t i;

    if (!ptr) {
        return;
    }
    for (i = 0; i < len; ++i) {
        p[i] = 0;
    }
}

void loom_kex_ctx_secure_wipe(LOOM_KEX_CTX *ctx)
{
    if (!ctx) {
        return;
    }
    loom_secure_zero(ctx->self_nonce, sizeof(ctx->self_nonce));
    loom_secure_zero(ctx->peer_nonce, sizeof(ctx->peer_nonce));
    loom_secure_zero(ctx->self_exch, sizeof(ctx->self_exch));
    loom_secure_zero(ctx->peer_exch, sizeof(ctx->peer_exch));
    loom_secure_zero(ctx->authenticator.mk_self, sizeof(ctx->authenticator.mk_self));
    loom_secure_zero(ctx->authenticator.mk_peer, sizeof(ctx->authenticator.mk_peer));
    loom_secure_zero(ctx->secret.skkem, sizeof(ctx->secret.skkem));
    loom_secure_zero(ctx->secret.keymat, sizeof(ctx->secret.keymat));
    loom_secure_zero(ctx->secret.ss, sizeof(ctx->secret.ss));
    if (ctx->authenticator.self_id) {
        loom_secure_zero(ctx->authenticator.self_id, ctx->authenticator.self_id_len);
    }
}

static uint8_t loom_expected_protocol_state(uint8_t stage)
{
    switch (stage) {
    case LOOM_STATE_STAGE_INIT:
        return (uint8_t)255; /* maps to ctx->state == -1 */
    case LOOM_STATE_STAGE_PASS1:
        return 1;
    case LOOM_STATE_STAGE_PASS2:
        return 2;
    case LOOM_STATE_STAGE_PASS3:
        return 3;
    case LOOM_STATE_STAGE_PASS4:
        return 4;
    default:
        return 255;
    }
}

static int loom_protocol_state_matches_stage(int protocol_state, uint8_t stage)
{
    if (stage == LOOM_STATE_STAGE_INIT) {
        return protocol_state == -1;
    }
    return protocol_state == (int)stage;
}

unsigned long long loom_kex_state_packed_len_bytes(void)
{
    return 8ULL
         + 4ULL /* protocol_state */
         + 1ULL /* initiator flag */
         + (unsigned long long)LOOM_SPIBYTES
         + (unsigned long long)LOOM_NONCEBYTES
         + (unsigned long long)LOOM_NONCEBYTES
         + 4ULL
         + (unsigned long long)LOOM_EXCH_LENMAX
         + 4ULL
         + (unsigned long long)LOOM_EXCH_LENMAX
         + 4ULL
         + (unsigned long long)LOOM_MAX_IDBYTES
         + (unsigned long long)LOOM_MACBYTES
         + (unsigned long long)LOOM_MACBYTES
         + (unsigned long long)LOOM_KEM_SKBYTES
         + (unsigned long long)LOOM_KEM_SSBYTES
         + (unsigned long long)LOOM_KEM_SSBYTES;
}

int loom_kex_ctx_pack(uint8_t *out, size_t outcap, size_t *outlen,
                      const LOOM_KEX_CTX *ctx, uint8_t role, uint8_t stage)
{
    size_t off = 0;
    uint8_t u32buf[4];
    const size_t expected = (size_t)loom_kex_state_packed_len_bytes();

    if (!out || !outlen || !ctx) {
        return -1;
    }
    if (outcap < expected) {
        return -2;
    }
    if (role != LOOM_STATE_ROLE_INITIATOR && role != LOOM_STATE_ROLE_RESPONDER) {
        return -3;
    }
    if (stage > LOOM_STATE_STAGE_PASS4) {
        return -3;
    }
    if (!loom_protocol_state_matches_stage(ctx->state, stage)) {
        return -4;
    }
    if (ctx->authenticator.self_id_len == 0
        || ctx->authenticator.self_id_len > LOOM_MAX_IDBYTES
        || !ctx->authenticator.self_id) {
        return -5;
    }

    uint8_t version_byte = LOOM_STATE_VERSION;
    uint8_t suite_byte = (uint8_t)LOOM_MODE;

    if (loom_write_field(out, outcap, &off, (const uint8_t *)"LSTA", 4)
        || loom_write_field(out, outcap, &off, &version_byte, 1)
        || loom_write_field(out, outcap, &off, &suite_byte, 1)
        || loom_write_field(out, outcap, &off, &role, 1)
        || loom_write_field(out, outcap, &off, &stage, 1)) {
        return -6;
    }

    loom_store_i32_le(u32buf, (int32_t)ctx->state);
    if (loom_write_field(out, outcap, &off, u32buf, 4)) {
        return -6;
    }

    {
        uint8_t initiator_flag = (uint8_t)(ctx->initiator ? 1U : 0U);
        if (loom_write_field(out, outcap, &off, &initiator_flag, 1)) {
            return -6;
        }
    }

    if (loom_write_field(out, outcap, &off, ctx->spi, LOOM_SPIBYTES)
        || loom_write_field(out, outcap, &off, ctx->self_nonce, LOOM_NONCEBYTES)
        || loom_write_field(out, outcap, &off, ctx->peer_nonce, LOOM_NONCEBYTES)) {
        return -6;
    }

    loom_store_u32_le(u32buf, (uint32_t)ctx->self_exchlen);
    if (loom_write_field(out, outcap, &off, u32buf, 4)
        || loom_write_field(out, outcap, &off, ctx->self_exch, LOOM_EXCH_LENMAX)) {
        return -6;
    }

    loom_store_u32_le(u32buf, (uint32_t)ctx->peer_exchlen);
    if (loom_write_field(out, outcap, &off, u32buf, 4)
        || loom_write_field(out, outcap, &off, ctx->peer_exch, LOOM_EXCH_LENMAX)) {
        return -6;
    }

    loom_store_u32_le(u32buf, (uint32_t)ctx->authenticator.self_id_len);
    if (loom_write_field(out, outcap, &off, u32buf, 4)) {
        return -6;
    }

    {
        uint8_t idbuf[LOOM_MAX_IDBYTES];
        memset(idbuf, 0, sizeof(idbuf));
        memcpy(idbuf, ctx->authenticator.self_id, ctx->authenticator.self_id_len);
        if (loom_write_field(out, outcap, &off, idbuf, LOOM_MAX_IDBYTES)) {
            return -6;
        }
    }

    if (loom_write_field(out, outcap, &off, ctx->authenticator.mk_self, LOOM_MACBYTES)
        || loom_write_field(out, outcap, &off, ctx->authenticator.mk_peer, LOOM_MACBYTES)
        || loom_write_field(out, outcap, &off, ctx->secret.skkem, LOOM_KEM_SKBYTES)
        || loom_write_field(out, outcap, &off, ctx->secret.keymat, LOOM_KEM_SSBYTES)
        || loom_write_field(out, outcap, &off, ctx->secret.ss, LOOM_KEM_SSBYTES)) {
        return -6;
    }

    if (off != expected) {
        return -7;
    }

    *outlen = off;
    return 0;
}

int loom_kex_ctx_unpack(LOOM_KEX_CTX *ctx, const uint8_t *in, size_t inlen,
                        uint8_t expected_role, uint8_t expected_stage,
                        const uint8_t *self_pk, const uint8_t *self_sk)
{
    size_t off = 0;
    uint8_t hdr[8];
    uint8_t u32buf[4];
    uint32_t self_exchlen = 0;
    uint32_t peer_exchlen = 0;
    uint32_t self_id_len = 0;
    uint8_t idbuf[LOOM_MAX_IDBYTES];
    const size_t expected = (size_t)loom_kex_state_packed_len_bytes();

    if (!ctx || !in || !self_sk) {
        return -1;
    }
    if (inlen != expected) {
        return -2;
    }

    loom_kex_ctx_secure_wipe(ctx);
    free(ctx->authenticator.self_id);
    memset(ctx, 0, sizeof(*ctx));

    if (loom_read_field(in, inlen, &off, hdr, 8)) {
        return -3;
    }
    if (hdr[0] != LOOM_STATE_MAGIC0 || hdr[1] != LOOM_STATE_MAGIC1
        || hdr[2] != LOOM_STATE_MAGIC2 || hdr[3] != LOOM_STATE_MAGIC3) {
        return -4;
    }
    if (hdr[4] != LOOM_STATE_VERSION) {
        return -5;
    }
    if (hdr[5] != (uint8_t)LOOM_MODE) {
        return -6;
    }
    if (hdr[6] != expected_role) {
        return -7;
    }
    if (hdr[7] != expected_stage) {
        return -8;
    }

    if (loom_read_field(in, inlen, &off, u32buf, 4)) {
        return -9;
    }
    ctx->state = loom_load_i32_le(u32buf);
    if (!loom_protocol_state_matches_stage(ctx->state, expected_stage)) {
        loom_kex_ctx_secure_wipe(ctx);
        return -10;
    }

    {
        uint8_t initiator_flag = 0;
        if (loom_read_field(in, inlen, &off, &initiator_flag, 1)) {
            return -9;
        }
        ctx->initiator = initiator_flag ? 1 : 0;
        if ((expected_role == LOOM_STATE_ROLE_INITIATOR && ctx->initiator != 1)
            || (expected_role == LOOM_STATE_ROLE_RESPONDER && ctx->initiator != 0)) {
            loom_kex_ctx_secure_wipe(ctx);
            return -11;
        }
    }

    if (loom_read_field(in, inlen, &off, ctx->spi, LOOM_SPIBYTES)
        || loom_read_field(in, inlen, &off, ctx->self_nonce, LOOM_NONCEBYTES)
        || loom_read_field(in, inlen, &off, ctx->peer_nonce, LOOM_NONCEBYTES)) {
        loom_kex_ctx_secure_wipe(ctx);
        return -9;
    }

    if (loom_read_field(in, inlen, &off, u32buf, 4)) {
        loom_kex_ctx_secure_wipe(ctx);
        return -9;
    }
    self_exchlen = loom_load_u32_le(u32buf);
    if (self_exchlen > LOOM_EXCH_LENMAX) {
        loom_kex_ctx_secure_wipe(ctx);
        return -12;
    }
    ctx->self_exchlen = self_exchlen;
    if (loom_read_field(in, inlen, &off, ctx->self_exch, LOOM_EXCH_LENMAX)) {
        loom_kex_ctx_secure_wipe(ctx);
        return -9;
    }

    if (loom_read_field(in, inlen, &off, u32buf, 4)) {
        loom_kex_ctx_secure_wipe(ctx);
        return -9;
    }
    peer_exchlen = loom_load_u32_le(u32buf);
    if (peer_exchlen > LOOM_EXCH_LENMAX) {
        loom_kex_ctx_secure_wipe(ctx);
        return -12;
    }
    ctx->peer_exchlen = peer_exchlen;
    if (loom_read_field(in, inlen, &off, ctx->peer_exch, LOOM_EXCH_LENMAX)) {
        loom_kex_ctx_secure_wipe(ctx);
        return -9;
    }

    if (loom_read_field(in, inlen, &off, u32buf, 4)) {
        loom_kex_ctx_secure_wipe(ctx);
        return -9;
    }
    self_id_len = loom_load_u32_le(u32buf);
    if (self_id_len == 0 || self_id_len > LOOM_MAX_IDBYTES) {
        loom_kex_ctx_secure_wipe(ctx);
        return -12;
    }
    if (loom_read_field(in, inlen, &off, idbuf, LOOM_MAX_IDBYTES)) {
        loom_kex_ctx_secure_wipe(ctx);
        return -9;
    }
    ctx->authenticator.self_id = (unsigned char *)malloc(self_id_len);
    if (!ctx->authenticator.self_id) {
        loom_kex_ctx_secure_wipe(ctx);
        return -13;
    }
    memcpy(ctx->authenticator.self_id, idbuf, self_id_len);
    ctx->authenticator.self_id_len = self_id_len;

    if (loom_read_field(in, inlen, &off, ctx->authenticator.mk_self, LOOM_MACBYTES)
        || loom_read_field(in, inlen, &off, ctx->authenticator.mk_peer, LOOM_MACBYTES)
        || loom_read_field(in, inlen, &off, ctx->secret.skkem, LOOM_KEM_SKBYTES)
        || loom_read_field(in, inlen, &off, ctx->secret.keymat, LOOM_KEM_SSBYTES)
        || loom_read_field(in, inlen, &off, ctx->secret.ss, LOOM_KEM_SSBYTES)) {
        loom_kex_ctx_secure_wipe(ctx);
        return -9;
    }

    if (off != expected) {
        loom_kex_ctx_secure_wipe(ctx);
        return -14;
    }

    ctx->authenticator.self_pubkey = (unsigned char *)self_pk;
    ctx->authenticator.self_privkey = (unsigned char *)self_sk;
    (void)self_pk;
    (void)loom_expected_protocol_state(expected_stage);
    return 0;
}
