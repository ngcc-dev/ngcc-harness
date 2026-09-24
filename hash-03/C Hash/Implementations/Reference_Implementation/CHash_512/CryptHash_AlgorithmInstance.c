/* C-Hash (C-Hash-512 / C-Hash-1024) 的普通 C 版本 (纯标量, 无 SIMD)
 *
 * 编译 (任意平台, 任意主流 C99/C11 编译器):
 *   gcc   -O2 -std=c11 CryptHash_AlgorithmInstance.c -o c_hash_plain
 *   clang -O2 -std=c11 CryptHash_AlgorithmInstance.c -o c_hash_plain
 *   cl    /O2 /std:c11 CryptHash_AlgorithmInstance.c
 *
 * 对外接口：
 *   int CryptHash(int digest_len_bits,
 *                 const unsigned char* msg,
 *                 unsigned long long msg_len_bits,
 *                 unsigned char* digest);
 */

#include "CryptHash_AlgorithmInstance.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>


#define ROWS          ((size_t)6)
#define NIBS_PER_ROW  ((size_t)64)
#define TOTAL_NIBS    (ROWS * NIBS_PER_ROW)


/* S 盒 */
static const uint8_t SBOX[16] = {
    0x1, 0x4, 0x0, 0xc, 0x3, 0x2, 0x5, 0xb,
    0xa, 0x8, 0x6, 0xf, 0x7, 0x9, 0xd, 0xe
};

/* 轮常数 */
static const uint32_t RC_U32[28] = {
    0x243F6A88u, 0x85A308D3u, 0x13198A2Eu, 0x03707344u,
    0xA4093822u, 0x299F31D0u, 0x082EFA98u, 0xEC4E6C89u,
    0x452821E6u, 0x38D01377u, 0xBE5466CFu, 0x34E90C6Cu,
    0xC0AC29B7u, 0xC97C50DDu, 0x3F84D5B5u, 0xB5470917u,
    0x9216D5D9u, 0x8979FB1Bu, 0xD1310BA6u, 0x98DFB5ACu,
    0x2FFD72DBu, 0xD01ADFB7u, 0xB8E1AFEDu, 0x6A267E96u,
    0xBA7C9045u, 0xF12C7F99u, 0x24A19947u, 0xB3916CF7u
};


/* 每行的列常量 */
static const uint8_t COL_CONST[6] = { 0xC, 0x9, 0x0, 0x8, 0xB, 0x2 };

/**
 * @brief 将 192 字节的状态转换为每行 64 个 nibble 的数组表示。
 *        每个字节拆分为高低两个 4 位 nibble。
 */
static void state_from_bytes(const uint8_t src_bytes[192], uint8_t nibs[TOTAL_NIBS]) {
    size_t row, k;
    for (row = 0; row < ROWS; ++row) {
        const uint8_t* p = src_bytes + row * 32;
        uint8_t* n = nibs + row * NIBS_PER_ROW;
        for (k = 0; k < 32; ++k) {
            n[2 * k + 0] = (uint8_t)((p[k] >> 4) & 0x0F);
            n[2 * k + 1] = (uint8_t)( p[k]       & 0x0F);
        }
    }
}

/**
 * @brief 将每行 64 个 nibble 的数组表示重新打包为 192 字节状态。
 */
static void state_to_bytes(const uint8_t nibs[TOTAL_NIBS], uint8_t dst_bytes[192]) {
    size_t row, k;
    for (row = 0; row < ROWS; ++row) {
        uint8_t* p = dst_bytes + row * 32;
        const uint8_t* n = nibs + row * NIBS_PER_ROW;
        for (k = 0; k < 32; ++k) {
            p[k] = (uint8_t)(((n[2 * k + 0] & 0x0F) << 4) | (n[2 * k + 1] & 0x0F));
        }
    }
}


/**
 * @brief 对 32 个 nibble 的数组执行向左循环移位 amount 位。
 *        用临时缓冲区模拟 std::rotate 的行为。
 */
static void rotate_left_nib(uint8_t* p, size_t amount) {
    uint8_t tmp[32];
    size_t i;
    amount %= 32;
    if (amount == 0) return;
    for (i = 0; i < 32; ++i) {
        tmp[i] = p[(i + amount) % 32];
    }
    memcpy(p, tmp, 32);
}


/**
 * @brief 对 32 个 nibble 的数组执行向右循环移位 1 位（即左移 31）。
 */
static void rotate_right1_nib(uint8_t* p) {
    rotate_left_nib(p, 31);
}

/**
 * @brief 将 src_row 按 nibble 异或到 dst_row。
 */
static void row_xor_row(uint8_t s[TOTAL_NIBS], size_t dst_row, size_t src_row) {
    uint8_t* d = s + dst_row * NIBS_PER_ROW;
    const uint8_t* x = s + src_row * NIBS_PER_ROW;
    size_t k;
    for (k = 0; k < NIBS_PER_ROW; ++k) d[k] ^= x[k];
}

/**
 * @brief BigU-1536 单轮置换：S 盒、行混合、列常量、再 S 盒、双层混合旋转、轮常数注入。
 * @param s 当前状态（384 个 nibble）
 * @param r 轮号 0..27
 */
static void BigU_1536_Round_plain(uint8_t s[TOTAL_NIBS], int r) {
    size_t k, row;
    static const size_t P1_AMOUNTS[4] = { 15, 7, 23, 16 };
    uint8_t T1[32];
    uint32_t rc;
    uint8_t* x0;
    int sub;

    /* 第 1 次 S 盒替换 */
    for (k = 0; k < TOTAL_NIBS; ++k) {
        s[k] = SBOX[s[k] & 0x0F];
    }

    /* 行混合矩阵 */
    row_xor_row(s, 3, 2); row_xor_row(s, 4, 0); row_xor_row(s, 5, 1);
    row_xor_row(s, 0, 3); row_xor_row(s, 1, 4); row_xor_row(s, 2, 5);
    row_xor_row(s, 3, 1); row_xor_row(s, 4, 2); row_xor_row(s, 5, 0);

    /* 每行异或列常量 */
    for (row = 0; row < ROWS; ++row) {
        uint8_t c = COL_CONST[row];
        uint8_t* p = s + row * NIBS_PER_ROW;
        for (k = 0; k < NIBS_PER_ROW; ++k) p[k] ^= c;
    }

    /* 第 2 次 S 盒替换 */
    for (k = 0; k < TOTAL_NIBS; ++k) {
        s[k] = SBOX[s[k] & 0x0F];
    }

    /* 4 次 L/R 半行的线性变换 */
    for (row = 0; row < ROWS; ++row) {
        uint8_t* L = s + row * NIBS_PER_ROW;
        uint8_t* R = L + 32;

        for (sub = 0; sub < 4; ++sub) {
            /* T1 = L ^ R, 然后循环左移 P1_AMOUNTS[sub] */
            for (k = 0; k < 32; ++k) T1[k] = L[k] ^ R[k];
            rotate_left_nib(T1, P1_AMOUNTS[sub]);

            /* L ^= T1 */
            for (k = 0; k < 32; ++k) L[k] ^= T1[k];

            /* R ^= T1 后整体右移 1 */
            for (k = 0; k < 32; ++k) R[k] ^= T1[k];
            rotate_right1_nib(R);
        }
    }

    /* 轮常数注入到第 0 行的前 8 个 nibble */
    rc = RC_U32[r];
    x0 = s + 0 * NIBS_PER_ROW;
    for (k = 0; k < 8; ++k) {
        uint8_t nib = (uint8_t)((rc >> (28 - 4 * k)) & 0x0F);
        x0[k] ^= nib;
    }
}

/**
 * @brief 以 192 字节输入/输出方式调用 28 轮置换。
 */
static void C_Engine1536_Perm28_plain(uint8_t state_bytes[192]) {
    uint8_t s[TOTAL_NIBS];
    int r;
    memset(s, 0, TOTAL_NIBS);
    state_from_bytes(state_bytes, s);
    for (r = 0; r < 28; ++r) {
        BigU_1536_Round_plain(s, r);
    }
    state_to_bytes(s, state_bytes);
}

/* C-Hash 相关常量 */
#define CH_B_BITS       ((size_t)1536)
#define CH_W_BITS       ((size_t)64)
#define CH_N_BITS       ((size_t)1472)
#define CH_HALF_N_BITS  ((size_t)736)

#define CH_B_BYTES      (CH_B_BITS / 8)
#define CH_N_BYTES      (CH_N_BITS / 8)
#define CH_HALF_N_BYTES (CH_HALF_N_BITS / 8)
#define CH_W_BYTES      (CH_W_BITS / 8)

#define CH_LANE_INJECT  ((uint8_t)0x01)
#define CH_LANE_GEN     ((uint8_t)0x03)


/* 模式枚举 */
typedef enum CHashMode {
    CH_CTR_PERM = 0,
    CH_CTR_FUNC = 1
} CHashMode;

typedef struct CHashVariant {
    int       hashlen_bits;
    uint8_t   iv[CH_HALF_N_BYTES];
    size_t    h_bits;
    CHashMode mode;
    uint8_t   lane_inject;
    uint8_t   lane_gen;
    uint8_t   lane_ldrh;
} CHashVariant;


/**
 * @brief 构建 IV：后两个字节写入 hashlen_bits 的大端表示。
 */
static void build_IV(int hashlen_bits, uint8_t out[CH_HALF_N_BYTES]) {
    memset(out, 0, CH_HALF_N_BYTES);
    out[CH_HALF_N_BYTES - 2] = (uint8_t)((hashlen_bits >> 8) & 0xFF);
    out[CH_HALF_N_BYTES - 1] = (uint8_t)( hashlen_bits       & 0xFF);
}

/**
 * @brief 根据摘要长度选择变体参数（512 或 1024）。
 * @return 1 成功，0 失败
 */
static int make_variant(int hashlen_bits, CHashVariant* v) {
    if (hashlen_bits == 512) {
        v->hashlen_bits = 512;
        v->h_bits       = 736;
        v->mode         = CH_CTR_PERM;
        v->lane_inject  = 0x01;   /* VIL  */
        v->lane_gen     = 0x02;   /* FIL  */
        v->lane_ldrh    = 0x03;   /* LDRH */
    } else if (hashlen_bits == 1024) {
        v->hashlen_bits = 1024;
        v->h_bits       = 1472;
        v->mode         = CH_CTR_FUNC;
        v->lane_inject  = 0x81;   /* VIL  */
        v->lane_gen     = 0x82;   /* FIL  */
        v->lane_ldrh    = 0x83;   /* LDRH */
    } else {
        return 0;
    }
    build_IV(hashlen_bits, v->iv);
    return 1;
}

/**
 * @brief 将 laneID 与 56 位计数器拼接为 8 字节大端 CTR。
 */
static void make_CTR_bytes(uint8_t laneID, uint64_t i, uint8_t out[8]) {
    uint64_t ctr = ((uint64_t)laneID << 56) | (i & ((1ULL << 56) - 1));
    int k;
    for (k = 0; k < 8; ++k) {
        out[k] = (uint8_t)(ctr >> (56 - 8 * k));
    }
}


/**
 * @brief 把 x_n 和 ctr_w 拼接成 B 字节状态。
 */
static void pack_state(const uint8_t x_n[CH_N_BYTES],
                       const uint8_t ctr_w[CH_W_BYTES],
                       uint8_t state_out[CH_B_BYTES]) {
    memcpy(state_out,              x_n,   CH_N_BYTES);
    memcpy(state_out + CH_N_BYTES, ctr_w, CH_W_BYTES);
}

/**
 * @brief 就地按字节异或 src 到 dst。
 */
static void xor_inplace(uint8_t* dst, const uint8_t* src, size_t n) {
    size_t k;
    for (k = 0; k < n; ++k) dst[k] ^= src[k];
}

/**
 * @brief CTR-PERM 模式：直接取置换输出的前 N 字节作为 y。
 */
static void ctr_perm(const uint8_t x_n[CH_N_BYTES],
                     const uint8_t ctr_w[CH_W_BYTES],
                     uint8_t y_n[CH_N_BYTES]) {
    uint8_t S[CH_B_BYTES];
    pack_state(x_n, ctr_w, S);
    C_Engine1536_Perm28_plain(S);
    memcpy(y_n, S, CH_N_BYTES);
}


/**
 * @brief CTR-FUNC 模式：y = perm(x || ctr)[:N] ^ x
 */
static void ctr_func(const uint8_t x_n[CH_N_BYTES],
                     const uint8_t ctr_w[CH_W_BYTES],
                     uint8_t y_n[CH_N_BYTES]) {
    uint8_t S[CH_B_BYTES];
    pack_state(x_n, ctr_w, S);
    C_Engine1536_Perm28_plain(S);
    memcpy(y_n, S, CH_N_BYTES);
    xor_inplace(y_n, x_n, CH_N_BYTES);
}

/**
 * @brief 根据 variant 选择 ctr_perm 或 ctr_func 作用到输入。
 */
static void apply_P(const CHashVariant* v,
                    const uint8_t x_n[CH_N_BYTES],
                    const uint8_t ctr_w[CH_W_BYTES],
                    uint8_t y_n[CH_N_BYTES]) {
    if (v->mode == CH_CTR_PERM) ctr_perm(x_n, ctr_w, y_n);
    else                        ctr_func(x_n, ctr_w, y_n);
}


/**
 * @brief 处理短消息(< N 位)的 LDRH 模式：对同一 x 用计数器 1、2 各做一次
 *        CTR-Perm，输出相异或后截断为摘要。
 *        x   = M || 1 || 0^(N-1-|M|)
 *        y1  = LMB(N, P(x || [LaneID||1]))
 *        y2  = LMB(N, P(x || [LaneID||2]))
 *        y   = y1 XOR y2
 * @return 输出摘要字节数
 */
static size_t C_HASH_LDRH(const CHashVariant* v,
                          const uint8_t* msg, uint64_t msg_len_bits,
                          uint8_t* digest_out) {
    uint8_t x_1472[CH_N_BYTES];
    uint8_t ctr1[CH_W_BYTES], ctr2[CH_W_BYTES];
    uint8_t y1[CH_N_BYTES], y2[CH_N_BYTES];
    uint64_t full_bytes;
    uint32_t tail_bits;
    size_t hashlen_bytes;

    memset(x_1472, 0, CH_N_BYTES);

    full_bytes = msg_len_bits / 8;
    tail_bits  = (uint32_t)(msg_len_bits % 8);
    if (full_bytes > 0) {
        memcpy(x_1472, msg, (size_t)full_bytes);
    }
    if (tail_bits > 0) {
        uint8_t mask = (uint8_t)(0xFFu << (8 - tail_bits));
        x_1472[(size_t)full_bytes] = (uint8_t)(msg[full_bytes] & mask);
    }

    /* 写入 '1' 比特作为填充起点 */
    x_1472[(size_t)(msg_len_bits / 8)] |= (uint8_t)(0x80u >> (msg_len_bits % 8));

    /* 两个计数器：[LaneID||1] 与 [LaneID||2] */
    make_CTR_bytes(v->lane_ldrh, 1, ctr1);
    make_CTR_bytes(v->lane_ldrh, 2, ctr2);

    /* 两次 CTR-Perm 输出相异或 */
    ctr_perm(x_1472, ctr1, y1);
    ctr_perm(x_1472, ctr2, y2);
    xor_inplace(y1, y2, CH_N_BYTES);

    hashlen_bytes = (size_t)(v->hashlen_bits / 8);
    memcpy(digest_out, y1, hashlen_bytes);
    return hashlen_bytes;
}

/* ===================================================================== *
 *  LDXOF —— 短消息可扩展输出模式（参考实现，独立函数）
 *
 *  注意：本函数仅作为 C-Hash “短消息可扩展输出模式 (XOF)” 的参考实现，
 *        供需要 XOF 能力的使用者参考调用；它【不】被标准 CryptHash 流程
 *        使用，也不参与 VIL 注入 / FIL 收尾。LDXOF 始终采用 CTR-Perm
 *        形式——即便是 C-Hash-1024 也不使用 CTR-Func、不做 XOR x。
 *
 *  仅处理短消息：要求 |M| < 1472 bit（即 < 184 字节）。
 *  LaneID：C-Hash-512 → 0x04，C-Hash-1024 → 0x84。
 *
 *  64 位计数器布局（大端序列化）：
 *      [ LaneID(8) | OutputByteLen(24) | Reserved(8)=0 | Index(24) ]
 *
 *  流程：
 *      x   = M || 1 || 0^(1471-|M|)                  （1472-bit 块）
 *      y0  = LMB(1472, P(x || ctr0))                 （i=0 基线）
 *      ri  = y0 XOR LMB(1472, P(x || ctri))          （i=1..block_count）
 *      R   = r1 || r2 || ... || r_block_count
 *      out = LMB(OutputByteLen*8, R)
 * ===================================================================== */

/**
 * @brief 构建 LDXOF 的 64 位计数器并以 8 字节大端写出。
 * @param lane_id         LDXOF 通道号（0x04 或 0x84）
 * @param output_byte_len 用户请求的输出字节数（24 bit）
 * @param index           当前置换实例编号 i（24 bit）
 * @param out             输出的 8 字节大端计数器
 */
static void make_ldxof_counter(uint8_t lane_id, uint32_t output_byte_len,
                               uint32_t index, uint8_t out[CH_W_BYTES]) {
    uint64_t ctr = ((uint64_t)lane_id << 56)
                 | ((uint64_t)(output_byte_len & 0xFFFFFFu) << 32)
                 | ((uint64_t)0x00u << 24)
                 | ((uint64_t)(index & 0xFFFFFFu));
    int k;
    for (k = 0; k < 8; ++k) {
        out[k] = (uint8_t)(ctr >> (56 - 8 * k));
    }
}

/**
 * @brief LDXOF 短消息可扩展输出（参考实现，标准 CryptHash 流程不调用）。
 * @param variant         512 或 1024
 * @param msg             短消息指针
 * @param msg_len_bits    消息位长，要求 < 1472
 * @param output_byte_len 期望输出字节数，要求 < 2^24
 * @param out             输出缓冲区（至少 output_byte_len 字节）
 * @return 实际写出的字节数；参数非法返回 (size_t)-1
 */
size_t C_HASH_LDXOF(int variant,
                    const uint8_t* msg, uint64_t msg_len_bits,
                    uint32_t output_byte_len,
                    uint8_t* out) {
    uint8_t  lane_id;
    uint8_t  x_1472[CH_N_BYTES];
    uint8_t  ctr[CH_W_BYTES];
    uint8_t  y0[CH_N_BYTES], yi[CH_N_BYTES];
    uint64_t full_bytes;
    uint32_t tail_bits;
    size_t   block_count, idx, written;

    /* 仅处理短消息，且输出长度需落在 24 位范围内 */
    if (msg_len_bits >= CH_N_BITS)     return (size_t)-1;
    if (output_byte_len >= (1u << 24)) return (size_t)-1;
    if (variant == 512)       lane_id = 0x04;
    else if (variant == 1024) lane_id = 0x84;
    else                      return (size_t)-1;

    /* 短消息 padding：x = M || 1 || 0^(1471-|M|) */
    memset(x_1472, 0, CH_N_BYTES);
    full_bytes = msg_len_bits / 8;
    tail_bits  = (uint32_t)(msg_len_bits % 8);
    if (full_bytes > 0) memcpy(x_1472, msg, (size_t)full_bytes);
    if (tail_bits > 0) {
        uint8_t mask = (uint8_t)(0xFFu << (8 - tail_bits));
        x_1472[(size_t)full_bytes] = (uint8_t)(msg[full_bytes] & mask);
    }
    x_1472[(size_t)(msg_len_bits / 8)] |= (uint8_t)(0x80u >> (msg_len_bits % 8));

    /* y0：i=0  */
    make_ldxof_counter(lane_id, output_byte_len, 0, ctr);
    ctr_perm(x_1472, ctr, y0);

    /* block_count = ceil(OutputByteLen*8 / 1472) = ceil(OutputByteLen / 184) */
    block_count = (size_t)((output_byte_len + CH_N_BYTES - 1) / CH_N_BYTES);

    /* 逐块：ri = y0 XOR LMB(N, P(x||ctri))，拼接后截断为 OutputByteLen */
    written = 0;
    for (idx = 1; idx <= block_count; ++idx) {
        size_t copy_len;
        make_ldxof_counter(lane_id, output_byte_len, (uint32_t)idx, ctr);
        ctr_perm(x_1472, ctr, yi);
        xor_inplace(yi, y0, CH_N_BYTES);   /* ri = y0 XOR yi */

        copy_len = CH_N_BYTES;
        if (written + copy_len > (size_t)output_byte_len) {
            copy_len = (size_t)output_byte_len - written;
        }
        memcpy(out + written, yi, copy_len);
        written += copy_len;
    }
    return written;
}

/**
 * @brief 对消息进行填充：'1' 比特 + K 个 '0' 比特，使总长是 HALF_N 位的整数倍。
 * @param[out] blocks 填充后的缓冲区（调用者通过 *blocks_out 释放）
 * @return 填充后的块数
 */
static size_t pad_message(const uint8_t* msg, uint64_t msg_len_bits,
                          uint8_t** blocks_out) {
    uint64_t rem        = msg_len_bits % CH_HALF_N_BITS;
    uint64_t K          = (CH_HALF_N_BITS - 1 - rem) % CH_HALF_N_BITS;
    uint64_t total_bits = msg_len_bits + 1 + K;
    size_t   l          = (size_t)(total_bits / CH_HALF_N_BITS);
    size_t   total_bytes = l * CH_HALF_N_BYTES;
    uint8_t* blocks;
    uint64_t full_bytes;
    uint32_t tail_bits;
    size_t   idx;
    uint32_t bit_in_byte;

    blocks = (uint8_t*)calloc(total_bytes ? total_bytes : 1, 1);
    *blocks_out = blocks;

    full_bytes = msg_len_bits / 8;
    tail_bits  = (uint32_t)(msg_len_bits % 8);
    if (full_bytes) memcpy(blocks, msg, (size_t)full_bytes);
    if (tail_bits > 0) {
        uint8_t mask = (uint8_t)(0xFFu << (8 - tail_bits));
        blocks[(size_t)full_bytes] = (uint8_t)(msg[full_bytes] & mask);
    }

    idx = (size_t)(msg_len_bits / 8);
    bit_in_byte = (uint32_t)(msg_len_bits % 8);
    blocks[idx] |= (uint8_t)(0x80u >> bit_in_byte);

    return l;
}

/**
 * @brief C-Hash 主流程：对长消息做分块压缩，最后做生成阶段输出摘要。
 * @return 输出摘要字节数
 */
static size_t C_HASH(const CHashVariant* v,
                     const uint8_t* msg, uint64_t msg_len_bits,
                     uint8_t* digest_out) {
    uint8_t x_1472[CH_N_BYTES];
    uint8_t ctr[CH_W_BYTES];
    uint8_t t_1472[CH_N_BYTES];
    uint8_t h1[CH_HALF_N_BYTES], h2[CH_HALF_N_BYTES];
    uint8_t x_buf[CH_N_BYTES], y_buf[CH_N_BYTES];
    uint8_t* blocks = NULL;
    size_t l;
    size_t i;
    size_t hashlen_bytes;

    if (msg_len_bits < CH_N_BITS) {
        return C_HASH_LDRH(v, msg, msg_len_bits, digest_out);
    }

    /* 初始状态：左半 = IV, 右半 = 0 */
    memcpy(x_1472,                   v->iv, CH_HALF_N_BYTES);
    memset(x_1472 + CH_HALF_N_BYTES, 0,     CH_HALF_N_BYTES);

    make_CTR_bytes(v->lane_inject, 0, ctr);

    apply_P(v, x_1472, ctr, t_1472);

    /* 将 IV 再次异或到右半 */
    xor_inplace(t_1472 + CH_HALF_N_BYTES, v->iv, CH_HALF_N_BYTES);

    memcpy(h1, t_1472,                   CH_HALF_N_BYTES);
    memcpy(h2, t_1472 + CH_HALF_N_BYTES, CH_HALF_N_BYTES);

    /* 填充消息 */
    l = pad_message(msg, msg_len_bits, &blocks);

    for (i = 1; i <= l; ++i) {
        const uint8_t* m_i = blocks + (i - 1) * CH_HALF_N_BYTES;

        /* 左半 = h1 ^ m_i, 右半 = h2 */
        memcpy(x_buf,                   h1, CH_HALF_N_BYTES);
        xor_inplace(x_buf, m_i, CH_HALF_N_BYTES);
        memcpy(x_buf + CH_HALF_N_BYTES, h2, CH_HALF_N_BYTES);

        make_CTR_bytes(v->lane_inject, (uint64_t)i, ctr);
        apply_P(v, x_buf, ctr, y_buf);

        /* 更新 h1 / h2, h2 再异或 m_i */
        memcpy(h1, y_buf,                   CH_HALF_N_BYTES);
        memcpy(h2, y_buf + CH_HALF_N_BYTES, CH_HALF_N_BYTES);
        xor_inplace(h2, m_i, CH_HALF_N_BYTES);
    }

    /* 生成阶段 */
    memcpy(x_buf,                   h1, CH_HALF_N_BYTES);
    memcpy(x_buf + CH_HALF_N_BYTES, h2, CH_HALF_N_BYTES);
    make_CTR_bytes(v->lane_gen, 1, ctr);
    apply_P(v, x_buf, ctr, y_buf);

    hashlen_bytes = (size_t)(v->hashlen_bits / 8);
    memcpy(digest_out, y_buf, hashlen_bytes);

    free(blocks);
    return hashlen_bytes;
}


/**
 * @brief 对外导出的 CryptHash 接口（纯 C 实现）。
 * @return 0 成功, 负值为错误码
 */
int CryptHash(int digest_len_bits,
              const unsigned char* msg,
              unsigned long long msg_len_bits,
              unsigned char* digest) {
    CHashVariant v;
    if (digest == NULL) return -2;
    if (msg == NULL && msg_len_bits > 0) return -3;

    if (!make_variant(digest_len_bits, &v)) return -1;

    C_HASH(&v, (const uint8_t*)msg, (uint64_t)msg_len_bits, (uint8_t*)digest);
    return 0;
}
