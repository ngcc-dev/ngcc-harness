#ifndef _HASH_GARNET_H_
#define _HASH_GARNET_H_

#include <stdint.h>
#include <string.h>
#include "garnet_config.h"

// 保持原有的类型定义
#ifndef custom_types_defined
#define custom_types_defined
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#endif

#ifndef UINT128_TYPE_DEFINED
#define UINT128_TYPE_DEFINED
typedef struct uint128_t {
    union {
        u64 v[2];
        u32 m[4];
        u8  n[16];
    } u;
} uint128_t;
#endif

// ---------------------------------------------------------
// Garnet 家族 48 位算法配置字 (Counter Magic Numbers)
// ---------------------------------------------------------
#define COUNTER_512_W512        0x02000e0a0e0aULL
#define COUNTER_512_W640        0x02800e0a0e0aULL
#define COUNTER_512_W768        0x03000e0a0e0aULL
#define COUNTER_512_W896        0x03800e0a0e0aULL
#define COUNTER_512_W1024       0x04000e0a0e0aULL
#define COUNTER_768_W512        0x02000f0b0f0bULL
#define COUNTER_1024_W1024      0x0400100c100cULL
#define COUNTER_1024_W1152      0x0480100c100cULL
#define COUNTER_1024_W2048_DM   0x8800100c100cULL
#define COUNTER_1024A_SP_DM     0x8380100c100cULL

#ifdef __cplusplus
extern "C" {
#endif

/* --- 自动映射层 --- */
#define ACTIVE_COUNTER_512  SUBMISSION_VARIANT_512
#define ACTIVE_COUNTER_768  SUBMISSION_VARIANT_768
#define ACTIVE_COUNTER_1024 SUBMISSION_VARIANT_1024

/* --- 函数声明 --- */
void initialize_state_st(struct uint128_t state[25], uint64_t digest_size, uint64_t counter);
void hash_garnet_universal_st(const u8* message, u64 mlength, u8* digest, struct uint128_t state[25], uint64_t counter);
// 32位机器上 4个32位异或远快于 2个64位异或
static inline struct uint128_t xor128(struct uint128_t x, struct uint128_t y) {
    struct uint128_t z;
    z.u.m[0] = x.u.m[0] ^ y.u.m[0];
    z.u.m[1] = x.u.m[1] ^ y.u.m[1];
    z.u.m[2] = x.u.m[2] ^ y.u.m[2];
    z.u.m[3] = x.u.m[3] ^ y.u.m[3];
    return z;
}

// 清零优化
static inline void zero128(struct uint128_t* x) {
    x->u.m[0] = 0;
    x->u.m[1] = 0;
    x->u.m[2] = 0;
    x->u.m[3] = 0;
}

/* 内部通用执行模板 (必须是 static inline 且放在头文件) */
static inline void garnet_variant_executor(int d_bits, uint64_t c_val, const u8* msg, u64 m_len_bits, u8* digest) {
    struct uint128_t state[25] __attribute__((aligned(16)));
    //memset(state, 0, sizeof(state));
    initialize_state_st(state, (uint32_t)d_bits, c_val);
    hash_garnet_universal_st(msg, m_len_bits, digest, state, c_val);
}

#ifdef __cplusplus
}
#endif

#endif // _HASH_GARNET_H_