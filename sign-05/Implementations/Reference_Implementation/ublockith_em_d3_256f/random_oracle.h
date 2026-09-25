/*
 * This file implements a random oracle abstraction.
 */

#ifndef RANDOM_ORACLE_H
#define RANDOM_ORACLE_H

#include "macros.h"
#include "params.h"

#include <stddef.h>
#include <stdint.h>

// reusable hash helpers

typedef struct {
    uint8_t* buffer;
    size_t buffer_len;
    size_t buffer_cap;
    uint8_t* output;
    size_t output_bytes_len;
    size_t output_off;
    int finalized;
} new_hash_context_t;

void new_hash_init(new_hash_context_t* ctx);
void new_hash_reset(new_hash_context_t* ctx);
void new_hash_update(new_hash_context_t* ctx, const uint8_t* input, size_t input_bytes_len);
void new_hash_final(new_hash_context_t* ctx, uint8_t domain_sep, size_t output_bytes_len);
void new_hash_squeeze(new_hash_context_t* ctx, uint8_t* dst, size_t output_bytes_len);
void new_hash_clear(new_hash_context_t* ctx);

// implementation of H_0

typedef new_hash_context_t H0_context_t;

void H0_init(H0_context_t* H0_ctx);
// No need for update in H0
void H0_update(H0_context_t* H0_ctx, const uint8_t* input, size_t bytes_len);
void H0_final_for_squeeze(const params_t* params, H0_context_t* H0_ctx);
// only for use with H0_final_for_squeeze
void H0_squeeze(H0_context_t* H0_ctx, uint8_t* dst, size_t len);
void H0_clear(H0_context_t* H0_ctx);

// implementation of H_1

typedef new_hash_context_t H1_context_t;

void H1_init(H1_context_t* H1_ctx);
void H1_reset(H1_context_t* H1_ctx);
void H1_update(H1_context_t* H1_ctx, const uint8_t* src, size_t len);
void H1_final_keep(const params_t* params, H1_context_t* H1_ctx, uint8_t* digest);
void H1_final(const params_t* params, H1_context_t* H1_ctx, uint8_t* digest);
void H1_clear(H1_context_t* H1_ctx);

// implementation of H_2

typedef new_hash_context_t H2_context_t;

void H2_init(H2_context_t* ctx);
void H2_copy(H2_context_t* new_ctx, const H2_context_t* ctx);
void H2_update(H2_context_t* ctx, const uint8_t* src, size_t bytes_len);
void H2_update_u32_le(H2_context_t* ctx, uint32_t v);
void H2_0_final(const params_t* params, H2_context_t* ctx, uint8_t* digest);
void H2_1_final(const params_t* params, H2_context_t* ctx, uint8_t* digest);
void H2_2_final(const params_t* params, H2_context_t* ctx, uint8_t* digest);
void H2_3_final(const params_t* params, H2_context_t* ctx, uint8_t* digest);
void H2_3_final_u32_le(const params_t* params, const H2_context_t* ctx, uint32_t ctr,
                       uint8_t* digest);
void H2_3_final_u32_le_batch4(const params_t* params, const H2_context_t* ctx,
                              uint32_t ctr_base, uint8_t* const digest[4],
                              size_t lane_count);

// implementation for H_3

typedef new_hash_context_t H3_context_t;

void H3_init(H3_context_t* ctx);
void H3_update(H3_context_t* ctx, const uint8_t* input, size_t bytes_len);
void H3_final(const params_t* params, H3_context_t* ctx, uint8_t* digest, uint8_t* iv);

// implementation for H_4

typedef new_hash_context_t H4_context_t;

void H4_init(H4_context_t* ctx);
void H4_update(H4_context_t* ctx, const uint8_t* pre_iv);
void H4_final(H4_context_t* ctx, uint8_t* iv);

#endif
