#ifndef CRYPTHASH_ADAPTER_H
#define CRYPTHASH_ADAPTER_H

#include <stddef.h>
#include <stdint.h>
#include "common.h"

typedef struct
{
    size_t (*get_digest_len)(void);
    size_t (*get_block_len)(void);
    int (*do_hash)(int digest_len_bits, const uint8_t *msg, size_t msg_len_bits, uint8_t *digest);
} HASH_METHOD;

void register_taichi_512_ref(void);
void register_taichi_768_ref(void);
void register_taichi_1024_ref(void);

void register_taichi_512_op_per(void);
void register_taichi_768_op_per(void);
void register_taichi_1024_op_per(void);

void register_taichi_512_op_res(void);
void register_taichi_768_op_res(void);
void register_taichi_1024_op_res(void);

#endif