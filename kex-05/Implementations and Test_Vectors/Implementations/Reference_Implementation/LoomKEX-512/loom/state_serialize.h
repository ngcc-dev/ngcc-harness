#ifndef LOOM_STATE_SERIALIZE_H
#define LOOM_STATE_SERIALIZE_H

#include <stddef.h>
#include <stdint.h>

struct loom_kex_state_st;

#define LOOM_STATE_MAGIC0 'L'
#define LOOM_STATE_MAGIC1 'S'
#define LOOM_STATE_MAGIC2 'T'
#define LOOM_STATE_MAGIC3 'A'
#define LOOM_STATE_VERSION 1

#define LOOM_STATE_ROLE_INITIATOR 0x01U
#define LOOM_STATE_ROLE_RESPONDER 0x02U

#define LOOM_STATE_STAGE_INIT  0U
#define LOOM_STATE_STAGE_PASS1 1U
#define LOOM_STATE_STAGE_PASS2 2U
#define LOOM_STATE_STAGE_PASS3 3U
#define LOOM_STATE_STAGE_PASS4 4U

void loom_store_u32_le(uint8_t out[4], uint32_t x);
uint32_t loom_load_u32_le(const uint8_t in[4]);
void loom_store_i32_le(uint8_t out[4], int32_t x);
int32_t loom_load_i32_le(const uint8_t in[4]);

int loom_write_field(uint8_t *out, size_t outcap, size_t *off,
                     const uint8_t *buf, size_t len);
int loom_read_field(const uint8_t *in, size_t inlen, size_t *off,
                    uint8_t *buf, size_t expected_len);

void loom_secure_zero(void *ptr, size_t len);
void loom_kex_ctx_secure_wipe(struct loom_kex_state_st *ctx);

unsigned long long loom_kex_state_packed_len_bytes(void);

int loom_kex_ctx_pack(uint8_t *out, size_t outcap, size_t *outlen,
                      const struct loom_kex_state_st *ctx, uint8_t role, uint8_t stage);

int loom_kex_ctx_unpack(struct loom_kex_state_st *ctx, const uint8_t *in, size_t inlen,
                        uint8_t expected_role, uint8_t expected_stage,
                        const uint8_t *self_pk, const uint8_t *self_sk);

/* self_pk may be NULL when only the signing secret key is required. */

#endif
