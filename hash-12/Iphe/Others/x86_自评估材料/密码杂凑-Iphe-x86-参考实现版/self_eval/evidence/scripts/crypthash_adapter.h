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
    int (*self_test)(void);
} HASH_METHOD;


void register_sm3(void);
void register_iphe_512(void);
void register_iphe_768(void);
void register_iphe_1024(void);
void register_iphe_512_perf(void);
void register_iphe_768_perf(void);
void register_iphe_1024_perf(void);
void register_iphe_512_res(void);
void register_iphe_768_res(void);
void register_iphe_1024_res(void);
#endif