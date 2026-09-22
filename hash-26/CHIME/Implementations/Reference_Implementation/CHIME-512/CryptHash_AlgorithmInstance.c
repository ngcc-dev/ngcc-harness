/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#include <string.h>
#include "CryptHash_AlgorithmInstance.h"

int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
#if DIGEST_BIT_LENGTH == 512
    return CryptHash512(msg, msg_len_bits, digest);
#elif DIGEST_BIT_LENGTH == 1024
    return CryptHash1024(msg, msg_len_bits, digest);
#endif
}

/*
 * Computes a 512-bit CHIME digest for a bit-length message.
 * Inputs: 
 *     msg: message bytes
 *     msg_len_bits: message length in bit
 * Outputs:
 *     digest: 512-bit digest buffer
 *     return value: 0 on success, -1 on failure
 *
 * 计算按位长度消息的 512 位 CHIME 摘要。
 * 输入：
 *     msg：消息字节流
 *     msg_len_bits：消息位长度
 * 输出：
 *     digest：512 位摘要缓冲区
 *     返回值：成功返回 0，失败返回 -1
 */
int CryptHash512(const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest)
{
    return chime_512_hash_bits((const uint8_t*)msg, msg_len_bits, (uint8_t*)digest);
}

// ==================== Permutation M39F without AVX or SSE ====================

/*
 * Stores round constants used by the permutation rounds.
 * 存储置换轮中使用的轮常量。
 */
static const uint64_t ARC[20] = {
    0x409AC7567F4A7C15ULL,
    0xE2C34D9DFE94F82BULL,
    0x040BD3C37DDF743DULL,
    0xA670580AFD29F057ULL,
    0xC9B8DE707C746C6DULL,
    0x6BE164B7FBBEE87BULL,
    0x8D29EAFD7B096495ULL,
    0x2F167324FA53E0AFULL,
    0x515EF96A799E5CB5ULL,
    0xF0877FD1F8E8D8DBULL,
    0x12CF8417783354EDULL,
    0xB4340A5EF77DD0F7ULL,
    0xD67C908476C84D1DULL,
    0x79A516CBF612C92BULL,
    0x9BED9F31755D4535ULL,
    0x3DDA2578F4A7C15FULL,
    0x5F02ABBE73F23D75ULL,
    0xC14B31E5F33CB96BULL,
    0x60B3B62B7287359DULL,
    0x82F83C92F1D1B1B7ULL
};

/* -------------------------------------------------------------------------- */
/* Basic helpers                                                              */
/* -------------------------------------------------------------------------- */

/*
 * Rotates a 64-bit word to the left by k bits.
 * Inputs:
 *     x: source word
 *     k: rotation amount
 * Outputs:
 *     return value: rotated 64-bit word
 *
 * 将 64 位字按 k 位进行循环左移。
 * 输入：
 *     x：源字
 *     k：左移位数
 * 输出：
 *     返回值：循环左移后的 64 位字
 */
static uint64_t rol64(uint64_t x, unsigned k) {
    return (uint64_t)((x << k) | (x >> (64u - k)));
}

/*
 * Copies four 64-bit lanes from source to destination.
 * Inputs:
 *     src: source lane array
 * Outputs:
 *     dst: destination lane array overwritten with src content
 *
 * 将 4 个 64 位 lane 从源数组复制到目标数组。
 * 输入：
 *     src：源 lane 数组
 * 输出：
 *     dst：被 src 覆盖后的目标 lane 数组
 */
static void copy4(uint64_t dst[4], const uint64_t src[4]) {
    memcpy(dst, src, 4 * sizeof(uint64_t));
}

/* --------------------------------------------------------------------------
 * AVX-equivalent shuffle helpers
 * -------------------------------------------------------------------------- */

/*
 * Applies the 0x93 lane permutation equivalent to AVX permute4x64.
 * Inputs:
 *     src: input lane array
 * Outputs:
 *     dst: reordered lane array as [3,0,1,2]
 *
 * 执行与 AVX permute4x64(0x93) 等价的 lane 重排。
 * 输入：
 *     src：输入 lane 数组
 * 输出：
 *     dst：重排为 [3,0,1,2] 的 lane 数组
 */
static void pq_0x93(uint64_t dst[4], const uint64_t src[4]) {
    dst[0] = src[3];
    dst[1] = src[0];
    dst[2] = src[1];
    dst[3] = src[2];
}

/*
 * Swaps the low/high 128-bit halves equivalent to AVX permute2x128(0x01).
 * Inputs:
 *     src: input lane array
 * Outputs:
 *     dst: lane array with swapped 128-bit halves
 *
 * 执行与 AVX permute2x128(0x01) 等价的高低 128 位半区交换。
 * 输入：
 *     src：输入 lane 数组
 * 输出：
 *     dst：交换 128 位半区后的 lane 数组
 */
static void p128_0x01(uint64_t dst[4], const uint64_t src[4]) {
    dst[0] = src[2];
    dst[1] = src[3];
    dst[2] = src[0];
    dst[3] = src[1];
}

/*
 * Splits one 128-bit value into four 32-bit words.
 * Inputs:
 *     lo64: lower 64 bits
 *     hi64: upper 64 bits
 * Outputs:
 *     w: 4-word array in lane-local order
 *
 * 将一个 128 位值拆分为 4 个 32 位字。
 * 输入：
 *     lo64：低 64 位
 *     hi64：高 64 位
 * 输出：
 *     w：按 lane 局部顺序排列的 4 字数组
 */
static void unpack_128_to_u32x4(uint64_t lo64, uint64_t hi64, uint32_t w[4]) {
    w[0] = (uint32_t)(lo64 & 0xFFFFFFFFu);
    w[1] = (uint32_t)(lo64 >> 32);
    w[2] = (uint32_t)(hi64 & 0xFFFFFFFFu);
    w[3] = (uint32_t)(hi64 >> 32);
}

/*
 * Packs four 32-bit words into one 128-bit value represented by two uint64 words.
 * Inputs:
 *     w: 4-word array
 * Outputs:
 *     lo64: lower 64 bits
 *     hi64: upper 64 bits
 *
 * 将 4 个 32 位字打包为一个 128 位值（由两个 uint64 表示）。
 * 输入：
 *     w：4 字数组
 * 输出：
 *     lo64：低 64 位
 *     hi64：高 64 位
 */
static void pack_u32x4_to_128(uint64_t* lo64, uint64_t* hi64, const uint32_t w[4]) {
    *lo64 = ((uint64_t)w[1] << 32) | (uint64_t)w[0];
    *hi64 = ((uint64_t)w[3] << 32) | (uint64_t)w[2];
}

/*
 * Applies the 0x39 32-bit shuffle on each 128-bit half.
 * Inputs:
 *     src: input lane array
 * Outputs:
 *     dst: shuffled lane array
 *
 * 在每个 128 位半区执行 0x39 的 32 位字重排。
 * 输入：
 *     src：输入 lane 数组
 * 输出：
 *     dst：重排后的 lane 数组
 */
static void sh_0x39(uint64_t dst[4], const uint64_t src[4]) {
    uint32_t inw[4], outw[4];

    /* lower 128 */
    unpack_128_to_u32x4(src[0], src[1], inw);
    outw[0] = inw[1];
    outw[1] = inw[2];
    outw[2] = inw[3];
    outw[3] = inw[0];
    pack_u32x4_to_128(&dst[0], &dst[1], outw);

    /* upper 128 */
    unpack_128_to_u32x4(src[2], src[3], inw);
    outw[0] = inw[1];
    outw[1] = inw[2];
    outw[2] = inw[3];
    outw[3] = inw[0];
    pack_u32x4_to_128(&dst[2], &dst[3], outw);
}

/*
 * Applies the 0x4E 32-bit shuffle on each 128-bit half.
 * Inputs:
 *     src: input lane array
 * Outputs:
 *     dst: shuffled lane array
 *
 * 在每个 128 位半区执行 0x4E 的 32 位字重排。
 * 输入：
 *     src：输入 lane 数组
 * 输出：
 *     dst：重排后的 lane 数组
 */
static void sh_0x4e(uint64_t dst[4], const uint64_t src[4]) {
    uint32_t inw[4], outw[4];

    /* lower 128 */
    unpack_128_to_u32x4(src[0], src[1], inw);
    outw[0] = inw[2];
    outw[1] = inw[3];
    outw[2] = inw[0];
    outw[3] = inw[1];
    pack_u32x4_to_128(&dst[0], &dst[1], outw);

    /* upper 128 */
    unpack_128_to_u32x4(src[2], src[3], inw);
    outw[0] = inw[2];
    outw[1] = inw[3];
    outw[2] = inw[0];
    outw[3] = inw[1];
    pack_u32x4_to_128(&dst[2], &dst[3], outw);
}

/* -------------------------------------------------------------------------- */
/* chi                                                                        */
/* -------------------------------------------------------------------------- */

/*
 * Applies chi non-linear mixing across three 4-lane vectors in place.
 * Inputs:
 *     A: first state vector before update
 *     B: second state vector before update
 *     C: third state vector before update
 * Outputs:
 *     A: first state vector after chi transform
 *     B: second state vector after chi transform
 *     C: third state vector after chi transform
 *
 * 对三个 4-lane 向量执行 chi 非线性混合并原地更新。
 * 输入：
 *     A：更新前的第一组状态向量
 *     B：更新前的第二组状态向量
 *     C：更新前的第三组状态向量
 * 输出：
 *     A：chi 变换后的第一组状态向量
 *     B：chi 变换后的第二组状态向量
 *     C：chi 变换后的第三组状态向量
 */
static void chi3(uint64_t A[4], uint64_t B[4], uint64_t C[4]) {
    uint64_t a[4], b[4];
    int i;

    copy4(a, A);
    copy4(b, B);

    for (i = 0; i < 4; i++) {
        A[i] = a[i] ^ ((~b[i]) & C[i]);
        B[i] = b[i] ^ ((~C[i]) & a[i]);
        C[i] = C[i] ^ ((~a[i]) & b[i]);
    }
}

/*
 * Runs the even-round chi schedule on the permutation state.
 * Inputs:
 *     s: state containing A0/A1/B0/B1/C0/C1
 * Outputs:
 *     s: state updated in place
 *
 * 在置换状态上执行偶数轮的 chi 运算。
 * 输入：
 *     s：包含 A0/A1/B0/B1/C0/C1 的状态
 * 输出：
 *     s：原地更新后的状态
 */
static void chi_e(p_state_t* s) {
    chi3(s->A0, s->B0, s->C0);
    chi3(s->A1, s->B1, s->C1);
}

/*
 * Runs the odd-round chi schedule on the permutation state.
 * Inputs:
 *     s: state containing A0/A1/B0/B1/C0/C1
 * Outputs:
 *     s: state updated in place
 *
 * 在置换状态上执行奇数轮的 chi 运算。
 * 输入：
 *     s：包含 A0/A1/B0/B1/C0/C1 的状态
 * 输出：
 *     s：原地更新后的状态
 */
static void chi_o(p_state_t* s) {
    chi3(s->A0, s->B1, s->C1);
    chi3(s->A1, s->B0, s->C0);
}

/* -------------------------------------------------------------------------- */
/* Linear layer                                                           */
/* -------------------------------------------------------------------------- */

/*
 * Executes the linear diffusion layer on the state.
 * Inputs:
 *     s: state before linear layer
 * Outputs:
 *     s: state after in-place linear transformation
 *
 * 在状态上执行线性扩散层。
 * 输入：
 *     s：线性层执行前的状态
 * 输出：
 *     s：线性变换后原地更新的状态
 */
static void lin_m39(p_state_t* s) {
    uint64_t tmp[4];
    uint64_t t[4];
    int i;

    /* s.B1 ^= SH(s.C1, 0x39) */
    sh_0x39(tmp, s->C1);
    for (i = 0; i < 4; i++) s->B1[i] ^= tmp[i];

    /* s.B0 ^= s.A1 */
    for (i = 0; i < 4; i++) s->B0[i] ^= s->A1[i];

    /* s.C0 ^= ROL64<29>(s.A0) */
    for (i = 0; i < 4; i++) s->C0[i] ^= rol64(s->A0[i], 29);

    /* s.B1 ^= ROL64<31>(s.A1) */
    for (i = 0; i < 4; i++) s->B1[i] ^= rol64(s->A1[i], 31);

    /* s.C1 ^= ROL64<13>(s.A0) */
    for (i = 0; i < 4; i++) s->C1[i] ^= rol64(s->A0[i], 13);

    /* s.C0 ^= s.B1 */
    for (i = 0; i < 4; i++) s->C0[i] ^= s->B1[i];

    /* s.A0 = PQ(s.A0, 0x93) */
    pq_0x93(tmp, s->A0);
    copy4(s->A0, tmp);

    /* T = SH(s.C0, 0x4E) */
    sh_0x4e(t, s->C0);

    /* s.C1 ^= SH(s.B1, 0x39) */
    sh_0x39(tmp, s->B1);
    for (i = 0; i < 4; i++) s->C1[i] ^= tmp[i];

    /* s.A1 = P128(s.A1, 0x01) */
    p128_0x01(tmp, s->A1);
    copy4(s->A1, tmp);

    /* s.A0 ^= T */
    for (i = 0; i < 4; i++) s->A0[i] ^= t[i];

    /* s.A1 ^= SH(s.B0, 0x39) */
    sh_0x39(tmp, s->B0);
    for (i = 0; i < 4; i++) s->A1[i] ^= tmp[i];

    /* s.B1 ^= (s.A0 << 7) per 64-bit lane */
    for (i = 0; i < 4; i++) s->B1[i] ^= (s->A0[i] << 7);

    /* s.C1 ^= (s.A1 >> 5) per 64-bit lane */
    for (i = 0; i < 4; i++) s->C1[i] ^= (s->A1[i] >> 5);
}

/* -------------------------------------------------------------------------- */
/* Round permutation RP_fwd / RP_rev                                          */
/* -------------------------------------------------------------------------- */

/*
 * Rotates branch registers forward among A/B/C groups.
 * Inputs:
 *     s: state before forward branch permutation
 * Outputs:
 *     s: state after forward branch permutation
 *
 * 在 A/B/C 分支组之间执行前向寄存器轮转。
 * 输入：
 *     s：前向分支置换前的状态
 * 输出：
 *     s：前向分支置换后的状态
 */
static void rp_fwd(p_state_t* s) {
    uint64_t t[4];

    copy4(t,     s->A0);
    copy4(s->A0, s->C1);
    copy4(s->C1, s->C0);
    copy4(s->C0, s->B1);
    copy4(s->B1, s->B0);
    copy4(s->B0, s->A1);
    copy4(s->A1, t);
}

/*
 * Rotates branch registers backward among A/B/C groups.
 * Inputs:
 *     s: state before reverse branch permutation
 * Outputs:
 *     s: state after reverse branch permutation
 *
 * 在 A/B/C 分支组之间执行反向寄存器轮转。
 * 输入：
 *     s：反向分支置换前的状态
 * 输出：
 *     s：反向分支置换后的状态
 */
static void rp_rev(p_state_t* s) {
    uint64_t t[4];

    copy4(t,     s->A0);
    copy4(s->A0, s->A1);
    copy4(s->A1, s->B0);
    copy4(s->B0, s->B1);
    copy4(s->B1, s->C0);
    copy4(s->C0, s->C1);
    copy4(s->C1, t);
}

/* -------------------------------------------------------------------------- */
/* Round and permutation                                                      */
/* -------------------------------------------------------------------------- */

/*
 * Executes one permutation round: linear layer, round constant XOR, and chi step.
 * Inputs:
 *     s: state for this round
 *     r: round index
 * Outputs:
 *     s: state updated in place after one round
 *
 * 执行一轮置换：线性层、轮常量异或、以及 chi 步骤。
 * 输入：
 *     s：本轮状态
 *     r：轮索引
 * 输出：
 *     s：完成一轮后原地更新的状态
 */
static void rp(p_state_t* s, int r) {
    int i;

    lin_m39(s);

    for (i = 0; i < 4; i++) {
        s->A0[i] ^= ARC[r];
    }

    if ((r & 1) == 0) chi_e(s);
    else              chi_o(s);
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

/*
 * Resets the permutation state to all zeros.
 * Inputs:
 *     s: state pointer to initialize
 * Outputs:
 *     s: all fields set to zero
 *
 * 将置换状态重置为全零。
 * 输入：
 *     s：待初始化的状态指针
 * 输出：
 *     s：所有字段被置零
 */
void p_state_zero(p_state_t* s) {
    memset(s, 0, sizeof(*s));
}

/*
 * Runs n rounds of the permutation with scheduled forward/reverse branch swaps.
 * Inputs:
 *     s: state to process
 *     n: number of rounds
 * Outputs:
 *     s: state after n rounds
 *
 * 执行 n 轮置换，并按调度进行前向/反向分支交换。
 * 输入：
 *     s：待处理状态
 *     n：轮数
 * 输出：
 *     s：执行 n 轮后的状态
 */
void p_pn(p_state_t* s, int n) {
    int r;
    for (r = 0; r < n; r++) {
        rp(s, r);
        if (((r / 3) & 1) == 0) rp_fwd(s);
        else                    rp_rev(s);
    }
}

// ==================== END: Permutation ====================

// =========================== CHIME-512 implementation ===========================

/*
 * Loads 8 little-endian bytes into one 64-bit word.
 * Inputs:
 *     src: 8-byte little-endian array
 * Outputs:
 *     return value: decoded 64-bit word
 *
 * 将 8 字节小端数据加载为一个 64 位字。
 * 输入：
 *     src：8 字节小端数组
 * 输出：
 *     返回值：解码后的 64 位字
 */
static uint64_t load64_le(const uint8_t src[8]) {
    return ((uint64_t)src[0]      ) |
           ((uint64_t)src[1] <<  8) |
           ((uint64_t)src[2] << 16) |
           ((uint64_t)src[3] << 24) |
           ((uint64_t)src[4] << 32) |
           ((uint64_t)src[5] << 40) |
           ((uint64_t)src[6] << 48) |
           ((uint64_t)src[7] << 56);
}

/*
 * Stores one 64-bit word into 8 bytes in little-endian format.
 * Inputs:
 *     x: source 64-bit word
 * Outputs:
 *     dst: 8-byte little-endian array
 *
 * 将一个 64 位字按小端格式写入 8 字节数组。
 * 输入：
 *     x：源 64 位字
 * 输出：
 *     dst：写入后的 8 字节小端数组
 */
static void store64_le(uint8_t dst[8], uint64_t x) {
    dst[0] = (uint8_t)(x      );
    dst[1] = (uint8_t)(x >>  8);
    dst[2] = (uint8_t)(x >> 16);
    dst[3] = (uint8_t)(x >> 24);
    dst[4] = (uint8_t)(x >> 32);
    dst[5] = (uint8_t)(x >> 40);
    dst[6] = (uint8_t)(x >> 48);
    dst[7] = (uint8_t)(x >> 56);
}

/*
 * XORs four 64-bit source lanes into destination lanes.
 * Inputs:
 *     src: source lane array
 * Outputs:
 *     dst: updated in place with dst[i] ^= src[i]
 *
 * 将 4 个 64 位源 lane 异或到目标 lane。
 * 输入：
 *     src：源 lane 数组
 * 输出：
 *     dst：按 dst[i] ^= src[i] 原地更新
 */
static void xor64x4(uint64_t dst[4], const uint64_t src[4]) {
    int i;
    for (i = 0; i < 4; i++) dst[i] ^= src[i];
}

/*
 * Copies the first n bits from source to destination using MSB-first semantics in the tail byte.
 * Inputs:
 *     src: source byte buffer
 *     nbits: number of bits to copy
 * Outputs:
 *     dst: destination buffer containing copied prefix bits
 *
 * 按尾字节高位优先语义将源数据前 n 位复制到目标。
 * 输入：
 *     src：源字节缓冲区
 *     nbits：要复制的位数
 * 输出：
 *     dst：写入前缀比特后的目标缓冲区
 */
static void copy_bits_msb_prefix(
    uint8_t* dst,
    const uint8_t* src,
    unsigned long long nbits
) {
    unsigned long long full_bytes = nbits >> 3;
    unsigned rem_bits = (unsigned)(nbits & 7u);

    if (full_bytes > 0) {
        memcpy(dst, src, (size_t)full_bytes);
    }

    if (rem_bits != 0u) {
        uint8_t mask = (uint8_t)(0xFFu << (8u - rem_bits));
        dst[full_bytes] = (uint8_t)(src[full_bytes] & mask);
    }
}

/*
 * Builds padded CHIME-512 blocks with bit-level pad10*1 rule.
 * Inputs:
 *     msg: message bitstream
 *     len_bits: message bit length
 * Outputs:
 *     blocks_out: allocated block buffer
 *     num_blocks_out: block count
 *     return value: status code, 0 on success, -1 on failure
 *
 * 按位级 pad10*1 规则构建 CHIME-512 填充块。
 * 输入：
 *     msg：消息比特流
 *     len_bits：消息位长度
 * 输出：
 *     blocks_out：分配得到的块缓冲区
 *     num_blocks_out：块数量
 *     返回值：状态码，成功返回 0，失败返回 -1
 */
int chime_512_build_padded_blocks_bit_level(
    const uint8_t* msg,
    unsigned long long len_bits,
    uint8_t** blocks_out,
    size_t* num_blocks_out
) {
    unsigned long long full_blocks_ull, rem_bits_ull, num_blocks_ull;
    size_t num_blocks_sz;
    uint8_t* blocks;

    if (blocks_out == NULL || num_blocks_out == NULL) return -1;
    if (msg == NULL && len_bits != 0ULL) return -1;

    *blocks_out = NULL;
    *num_blocks_out = 0;

    full_blocks_ull = len_bits / CHIME_512_RATE_BITS;
    rem_bits_ull    = len_bits % CHIME_512_RATE_BITS;

    if (rem_bits_ull <= CHIME_512_RATE_BITS - 2u) {
        num_blocks_ull = full_blocks_ull + 1u;
    } else if (rem_bits_ull == CHIME_512_RATE_BITS - 1u) {
        num_blocks_ull = full_blocks_ull + 2u;
    } else {
        /* rem_bits == 0 */
        num_blocks_ull = full_blocks_ull + 1u;
    }

    if (num_blocks_ull > (unsigned long long)SIZE_MAX) return -1;
    num_blocks_sz = (size_t)num_blocks_ull;

    if (num_blocks_sz > SIZE_MAX / CHIME_512_BLOCK_BYTES) return -1;

    blocks = (uint8_t*)calloc(num_blocks_sz, CHIME_512_BLOCK_BYTES);
    if (blocks == NULL) return -1;

    /* Copy all full blocks directly */
    for (size_t i = 0; i < (size_t)full_blocks_ull; i++) {
        memcpy(blocks + i * CHIME_512_BLOCK_BYTES,
               msg + i * CHIME_512_BLOCK_BYTES,
               CHIME_512_BLOCK_BYTES);
    }

    /* Final block(s) with pad10*1 */
    if (rem_bits_ull <= CHIME_512_RATE_BITS - 2u) {
        uint8_t* blk = blocks + (size_t)full_blocks_ull * CHIME_512_BLOCK_BYTES;
        unsigned long long full_bytes = rem_bits_ull >> 3;
        unsigned rem_bits = (unsigned)(rem_bits_ull & 7u);

        copy_bits_msb_prefix(blk, msg + (size_t)full_blocks_ull * CHIME_512_BLOCK_BYTES, rem_bits_ull);

        /* first '1' */
        if (rem_bits == 0u) {
            blk[full_bytes] |= 0x80u;
        } else {
            blk[full_bytes] |= (uint8_t)(1u << (7u - rem_bits));
        }

        /* final '1' = very last bit of block */
        blk[CHIME_512_BLOCK_BYTES - 1] |= 0x01u;
    }
    else if (rem_bits_ull == CHIME_512_RATE_BITS - 1u) {
        uint8_t* blk0 = blocks + (size_t)full_blocks_ull * CHIME_512_BLOCK_BYTES;
        uint8_t* blk1 = blocks + ((size_t)full_blocks_ull + 1u) * CHIME_512_BLOCK_BYTES;

        copy_bits_msb_prefix(blk0, msg + (size_t)full_blocks_ull * CHIME_512_BLOCK_BYTES, rem_bits_ull);

        /* first '1' lands in the last bit of blk0 */
        blk0[CHIME_512_BLOCK_BYTES - 1] |= 0x01u;

        /* closing final '1' in blk1 */
        blk1[CHIME_512_BLOCK_BYTES - 1] |= 0x01u;
    }
    else {
        /* rem_bits == 0 */
        uint8_t* blk = blocks + (size_t)full_blocks_ull * CHIME_512_BLOCK_BYTES;
        blk[0] |= 0x80u;                         /* first '1' */
        blk[CHIME_512_BLOCK_BYTES - 1] |= 0x01u;   /* final '1' */
    }

    *blocks_out = blocks;
    *num_blocks_out = num_blocks_sz;
    return 0;
}

/*
 * Frees the padded block buffer allocated during block building.
 * Inputs:
 *     blocks: heap buffer to release
 *
 * 释放构建填充块时分配的块缓冲区。
 * 输入：
 *     blocks：待释放的堆缓冲区
 */
void chime_512_free_blocks(uint8_t* blocks) {
    free(blocks);
}

/*
 * Decodes one 64-byte rate block into A0 and A1 lane vectors.
 * Inputs:
 *     block64: 64-byte block data
 * Outputs:
 *     A0: first 4 lanes
 *     A1: second 4 lanes
 *
 * 将一个 64 字节速率块解码为 A0 和 A1 lane 向量。
 * 输入：
 *     block64：64 字节块数据
 * 输出：
 *     A0：前 4 个 lane
 *     A1：后 4 个 lane
 */
static void load_rate512_to_A0_A1(
    const uint8_t block64[64],
    uint64_t A0[4],
    uint64_t A1[4]
) {
    /* rate = A0 || A1 */
    A0[0] = load64_le(block64 +  0);
    A0[1] = load64_le(block64 +  8);
    A0[2] = load64_le(block64 + 16);
    A0[3] = load64_le(block64 + 24);

    A1[0] = load64_le(block64 + 32);
    A1[1] = load64_le(block64 + 40);
    A1[2] = load64_le(block64 + 48);
    A1[3] = load64_le(block64 + 56);
}

/*
 * Absorbs all input blocks into the state and applies 15 rounds per block.
 * Inputs:
 *     blocks: contiguous block buffer
 *     num_blocks: number of blocks
 * Outputs:
 *     out_state: final state after absorption
 *
 * 将所有输入块吸收到状态中，并对每个块执行 15 轮置换。
 * 输入：
 *     blocks：连续块缓冲区
 *     num_blocks：块数量
 * 输出：
 *     out_state：吸收完成后的最终状态
 */
void chime_512_p_512_1024_blocks_ref(
    const uint8_t* blocks,
    unsigned long long num_blocks,
    p_state_t* out_state
) {
    unsigned long long i;
    p_state_t s;

    p_state_zero(&s);


    for (i = 0; i < num_blocks; i++) {
        const uint8_t* blk = blocks + (size_t)i * CHIME_512_BLOCK_BYTES;
        uint64_t mA0[4], mA1[4];

        load_rate512_to_A0_A1(blk, mA0, mA1);

        xor64x4(s.A0, mA0);
        xor64x4(s.A1, mA1);


        p_pn(&s, 15);

    }

    *out_state = s;
}

/*
 * Extracts a 512-bit digest from state lanes A0 and A1.
 * Inputs:
 *     s: state to read digest from
 * Outputs:
 *     digest: 64-byte output digest
 *
 * 从状态的 A0 和 A1 lane 中提取 512 位摘要。
 * 输入：
 *     s：用于读取摘要的状态
 * 输出：
 *     digest：64 字节输出摘要
 */
void chime_512_extract_digest512(
    const p_state_t* s,
    uint8_t digest[CHIME_512_DIGEST_BYTES]
) {
    /* low 256 bits = A0 */
    store64_le(digest +  0, s->A0[0]);
    store64_le(digest +  8, s->A0[1]);
    store64_le(digest + 16, s->A0[2]);
    store64_le(digest + 24, s->A0[3]);

    /* high 256 bits = A1 */
    store64_le(digest + 32, s->A1[0]);
    store64_le(digest + 40, s->A1[1]);
    store64_le(digest + 48, s->A1[2]);
    store64_le(digest + 56, s->A1[3]);
}

/*
 * Performs complete CHIME-512 hashing: padding, absorption, and digest extraction.
 * Inputs:
 *     msg: message bitstream
 *     len_bits: message bit length
 * Outputs:
 *     digest: final 512-bit digest
 *     return value: 0 on success, -1 on failure
 *
 * 执行完整 CHIME-512 哈希流程：填充、吸收、摘要提取。
 * 输入：
 *     msg：消息比特流
 *     len_bits：消息位长度
 * 输出：
 *     digest：最终 512 位摘要
 *     返回值：成功返回 0，失败返回 -1
 */
int chime_512_hash_bits(
    const uint8_t* msg,
    unsigned long long len_bits,
    uint8_t digest[CHIME_512_DIGEST_BYTES]
) {
    uint8_t* blocks = NULL;
    size_t num_blocks = 0;
    p_state_t s;
    int rc;

    rc = chime_512_build_padded_blocks_bit_level(msg, len_bits, &blocks, &num_blocks);
    if (rc != 0) return -1;

    chime_512_p_512_1024_blocks_ref(blocks, (unsigned long long)num_blocks, &s);
    chime_512_extract_digest512(&s, digest);

    chime_512_free_blocks(blocks);
    return 0;
}

// =========================== END: CHIME-512 implementation ===========================
