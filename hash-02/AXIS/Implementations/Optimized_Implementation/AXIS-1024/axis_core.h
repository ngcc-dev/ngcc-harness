#ifndef AXIS_CORE_H
#define AXIS_CORE_H

#include <stdint.h>

#include "def.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum axis_variant_t {
    AXIS_VARIANT_512 = 0,
    AXIS_VARIANT_768 = 1,
    AXIS_VARIANT_1024 = 2
} axis_variant_t;

typedef struct axis_variant_config_t {
    axis_variant_t variant;
    uint8_t register_count;
    uint16_t result_bits;
    uint16_t blank_rounds;
    uint8_t length_bits;
    uint64_t init_words[AXIS_MAX_REGISTERS][AXIS_REGISTER_WORDS];
    uint16_t exl[6];
    uint16_t exl_prev;
    uint16_t upl[6];
} axis_variant_config_t;

typedef struct axis_core_t {
    const axis_variant_config_t* config;
    uint64_t regs[AXIS_MAX_REGISTERS][AXIS_REGISTER_WORDS];
    uint8_t pending_bits[2];
    uint8_t pending_len;
    uint64_t total_message_bits;
    uint64_t padded_message_bits;
    uint64_t message_capacity_bits;
    uint8_t* message;
    uint8_t message_owned;
    uint8_t allocation_failed;
} axis_core_t;

const axis_variant_config_t* axis_default_config(axis_variant_t variant);
uint32_t axis_result_bits(axis_variant_t variant);
void axis_core_init(axis_core_t* ctx, axis_variant_t variant);
void axis_core_init_with_config(axis_core_t* ctx, const axis_variant_config_t* config);
void axis_core_update(axis_core_t* ctx, const void* msg, uint64_t msg_len);
void axis_core_update_bits(axis_core_t* ctx, const void* msg, uint64_t msg_bit_len);
void axis_core_final(axis_core_t* ctx, void* out);
void axis_core_hash_bits(axis_variant_t variant, const void* msg, uint64_t msg_bit_len, void* out);

#ifdef __cplusplus
}
#endif

#endif
