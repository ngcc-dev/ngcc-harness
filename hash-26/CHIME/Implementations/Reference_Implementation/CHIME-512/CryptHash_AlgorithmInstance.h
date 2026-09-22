/*
The software is provided by the Institute of Commercial Cryptography Standards
(ICCS), and is used for algorithm submissions in the Next-generation Commercial
Cryptographic Algorithms Program (NGCC).

ICCS doesn't represent or warrant that the operation of the software will be
uninterrupted or error-free in all cases. ICCS will take no responsibility for
the use of the software or the results thereof, if the software is used for any
other purposes.
*/

#ifndef CRYPTHASH_ALGORITHM_INSTANCE_H
#define CRYPTHASH_ALGORITHM_INSTANCE_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// Set "OUTPUT_BLANK_TEST_VECTORS" as 0 to generate test vector files
// Set "OUTPUT_BLANK_TEST_VECTORS" as 1 to generate blank template (default)
#define OUTPUT_BLANK_TEST_VECTORS 0

// Set "ALGORITHM_INSTANCE" as your algorithm instance name (no more than 64 bytes)
// Only letters, numbers, '-' or '_' are permitted
#define ALGORITHM_INSTANCE "CHIME-512"

// Set "DIGEST_BIT_LENGTH" as the message digest length of your algorithm instance
#define DIGEST_BIT_LENGTH 512

#ifdef __cplusplus
extern "C"
{
#endif

    /// @brief Input message to get message digests of specified lengths
    /// @param[in] digest_len_bits The total BITS of digest
    /// @param[in] msg The base address of message
    /// @param[in] msg_len_bits The total BITS of message
    /// @param[out] digest The base address of digest
    /// @return 0 for success, others for error
    int CryptHash(int digest_len_bits, const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

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
    int CryptHash512(const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);
 
    // ==================== Permutation M39F without AVX or SSE ====================
    /*
     * Stores the full permutation state as six 256-bit branches represented by 4x64-bit lanes.
     * 以 4x64 位 lane 形式存储六个 256 位分支的完整置换状态。
     */
    typedef struct {
        uint64_t A0[4];
        uint64_t A1[4];
        uint64_t B0[4];
        uint64_t B1[4];
        uint64_t C0[4];
        uint64_t C1[4];
    } p_state_t;

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
    void p_state_zero(p_state_t* s);

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
    void p_pn(p_state_t* s, int n);
    // ==================== END: Permutation without AVX or SSE ====================

    // =========================== CHIME-512 implementation ===========================

    /*
     * Defines the sponge rate in bits for CHIME-512.
     * 定义 CHIME-512 的海绵速率位宽。
     */
    #define CHIME_512_RATE_BITS     512u

    /*
     * Defines the sponge rate in bytes for CHIME-512.
     * 定义 CHIME-512 的海绵速率字节数。
     */
    #define CHIME_512_RATE_BYTES     64u

    /*
     * Defines the absorb block size in bytes for CHIME-512.
     * 定义 CHIME-512 的吸收块字节数。
     */
    #define CHIME_512_BLOCK_BYTES    64u

    /*
     * Defines the output digest size in bytes for CHIME-512.
     * 定义 CHIME-512 的摘要输出字节数。
     */
    #define CHIME_512_DIGEST_BYTES   64u

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
    );

    /*
     * Frees the padded block buffer allocated during block building.
     * Inputs:
     *     blocks: heap buffer to release
     *
     * 释放构建填充块时分配的块缓冲区。
     * 输入：
     *     blocks：待释放的堆缓冲区
     */
    void chime_512_free_blocks(uint8_t* blocks);

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
    );

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
    );

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
    );
    
    // =========================== END: CHIME-512 implementation ===========================

#ifdef __cplusplus
}
#endif
#endif