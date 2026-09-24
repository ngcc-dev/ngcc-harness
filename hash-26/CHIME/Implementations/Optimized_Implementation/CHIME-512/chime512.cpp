#include <immintrin.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <iostream>
#include "chime512.hpp"

/*
 * Extracts digest bytes from state as A0 || A1.
 * Inputs:
 *     s: permutation state after final round
 * Outputs:
 *     digest: 64-byte digest buffer written in raw lane order
 *
 * 将状态按 A0 || A1 顺序提取为摘要字节。
 * 输入：
 *     s：最终轮后的置换状态
 * 输出：
 *     digest：按原始 lane 顺序写入的 64 字节摘要缓冲区
 */
static inline void extract_digest512_bytes(const S& s, uint8_t* digest) {
    // raw digest bytes = A0 || A1
    _mm256_storeu_si256((__m256i*)(digest +  0), s.A0);
    _mm256_storeu_si256((__m256i*)(digest + 32), s.A1);
}

/*
 * Computes CHIME-512 digest with bit-level message padding flow.
 * Inputs:
 *     msg: input message buffer
 *     len_bits: message length in bits
 * Outputs:
 *     digest: output 512-bit digest buffer
 *     return value: 0 on success
 *                   -1 if digest is nullptr
 *                   -2 if msg is nullptr while len_bits is non-zero
 *
 * 使用位级消息填充流程计算 CHIME-512 摘要。
 * 输入：
 *     msg：输入消息缓冲区
 *     len_bits：消息位长度
 * 输出：
 *     digest：输出的 512 位摘要缓冲区
 *     返回值：成功返回 0
 *            当 digest 为 nullptr 时返回 -1
 *            当 msg 为 nullptr 且 len_bits 非 0 时返回 -2
 */
int chime_512_bit_padding(const uint8_t* msg, size_t len_bits, uint8_t* digest) {
    init_arc();
    constexpr size_t CHIME_512_RATE_BITS  = 512;
    constexpr size_t CHIME_512_RATE_BYTES = 64;

    if (digest == nullptr) {
        return -1;
    }
    if (msg == nullptr && len_bits != 0) {
        return -2;
    }

    S s;
    s.A0 = _mm256_setzero_si256();
    s.A1 = _mm256_setzero_si256();
    s.B0 = _mm256_setzero_si256();
    s.B1 = _mm256_setzero_si256();
    s.C0 = _mm256_setzero_si256();
    s.C1 = _mm256_setzero_si256();


    const size_t full_blocks = len_bits / CHIME_512_RATE_BITS;
    const size_t rem_bits    = len_bits % CHIME_512_RATE_BITS;

    // --------------------------------------------------------
    // 1) absorb all full 512-bit message blocks directly
    // --------------------------------------------------------
    for (size_t i = 0; i < full_blocks; ++i) {
        const uint8_t* blk = msg + i * CHIME_512_RATE_BYTES;


        __m256i m0 = _mm256_loadu_si256((const __m256i*)(blk + 0));
        __m256i m1 = _mm256_loadu_si256((const __m256i*)(blk + 32));

        s.A0 = _mm256_xor_si256(s.A0, m0);
        s.A1 = _mm256_xor_si256(s.A1, m1);


        P_15(s);

    }

    // --------------------------------------------------------
    // 2) handle final block(s) with bit-level pad10*1
    //    Message convention: partial byte is MSB-first
    // --------------------------------------------------------
    auto absorb_one_block = [&](const uint8_t blk[64], const char* tag) {

        __m256i m0 = _mm256_loadu_si256((const __m256i*)(blk + 0));
        __m256i m1 = _mm256_loadu_si256((const __m256i*)(blk + 32));

        s.A0 = _mm256_xor_si256(s.A0, m0);
        s.A1 = _mm256_xor_si256(s.A1, m1);


        P_15(s);

    };

    alignas(32) uint8_t blk0[64];
    alignas(32) uint8_t blk1[64];
    std::memset(blk0, 0, sizeof(blk0));
    std::memset(blk1, 0, sizeof(blk1));

    const uint8_t* tail = msg + full_blocks * CHIME_512_RATE_BYTES;

    // Copy remaining message bits into blk0:
    // full bytes directly, last partial byte MSB-first prefix only.
    const size_t rem_full_bytes = rem_bits >> 3;
    const size_t rem_tail_bits  = rem_bits & 7u;

    if (rem_full_bytes > 0) {
        std::memcpy(blk0, tail, rem_full_bytes);
    }
    if (rem_tail_bits != 0) {
        const uint8_t mask = (uint8_t)(0xFFu << (8 - rem_tail_bits));
        blk0[rem_full_bytes] = (uint8_t)(tail[rem_full_bytes] & mask);
    }

    if (rem_bits <= 510) {
        // first '1' and last '1' fit in same block
        const size_t first_pad_bit = rem_bits;
        blk0[first_pad_bit >> 3] |= (uint8_t)(1u << (7 - (first_pad_bit & 7u)));

        // final '1' at bit 511 => byte 63 bit 0
        blk0[63] |= 0x01u;

        absorb_one_block(blk0, "final padded block");
    }
    else if (rem_bits == 511) {
        // first '1' lands at the last bit of blk0
        blk0[63] |= 0x01u;

        // second block contains only the final closing '1'
        blk1[63] |= 0x01u;

        absorb_one_block(blk0, "final padded block 0");
        absorb_one_block(blk1, "final padded block 1");
    }
    else {
        // rem_bits == 0
        // need one extra block containing 1 || 0* || 1
        blk0[0]  |= 0x80u;
        blk0[63] |= 0x01u;

        absorb_one_block(blk0, "padding-only block");
    }

    // --------------------------------------------------------
    // 3) extract digest = A0 || A1
    // --------------------------------------------------------
    _mm256_storeu_si256((__m256i*)(digest +  0), s.A0);
    _mm256_storeu_si256((__m256i*)(digest + 32), s.A1);
    
    return 0;
}

/*
 * Computes CHIME-512 digest with byte-aligned message padding flow.
 * Inputs:
 *     msg: input message buffer
 *     len_bits: message length in bits, must be byte-aligned
 * Outputs:
 *     digest: output 512-bit digest buffer
 *     return value: 0 on success
 *                   -1 if digest is nullptr
 *                   -2 if msg is nullptr while len_bits is non-zero
 *                   -3 if len_bits is not byte-aligned
 *
 * 使用字节对齐消息填充流程计算 CHIME-512 摘要。
 * 输入：
 *     msg：输入消息缓冲区
 *     len_bits：消息位长度，必须按字节对齐
 * 输出：
 *     digest：输出的 512 位摘要缓冲区
 *     返回值：成功返回 0
 *            当 digest 为 nullptr 时返回 -1
 *            当 msg 为 nullptr 且 len_bits 非 0 时返回 -2
 *            当 len_bits 非字节对齐时返回 -3
 */
int chime_512_byte_padding(const uint8_t* msg, size_t len_bits, uint8_t* digest) {
    init_arc();
    constexpr size_t CHIME_512_RATE_BITS  = 512;
    constexpr size_t CHIME_512_RATE_BYTES = 64;

    if (digest == nullptr) {
        return -1;
    }
    if (msg == nullptr && len_bits != 0) {
        return -2;
    }
    if ((len_bits & 7u) != 0u) {
        return -3;
    }


    S s;
    s.A0 = _mm256_setzero_si256();
    s.A1 = _mm256_setzero_si256();
    s.B0 = _mm256_setzero_si256();
    s.B1 = _mm256_setzero_si256();
    s.C0 = _mm256_setzero_si256();
    s.C1 = _mm256_setzero_si256();

    const size_t len_bytes        = len_bits / 8;
    const size_t full_blocks      = len_bytes / CHIME_512_RATE_BYTES;
    const size_t last_block_bytes = len_bytes % CHIME_512_RATE_BYTES;

    auto absorb_one_block = [&](const uint8_t blk[64], const char* tag) {
        __m256i m0 = _mm256_loadu_si256((const __m256i*)(blk + 0));
        __m256i m1 = _mm256_loadu_si256((const __m256i*)(blk + 32));

        s.A0 = _mm256_xor_si256(s.A0, m0);
        s.A1 = _mm256_xor_si256(s.A1, m1);

        P_15(s);
    };

    // --------------------------------------------------------
    // 1) absorb all full 64-byte blocks directly
    // --------------------------------------------------------
    for (size_t i = 0; i < full_blocks; ++i) {
        absorb_one_block(msg + i * CHIME_512_RATE_BYTES, "full block");
    }

    // --------------------------------------------------------
    // 2) construct and absorb final padded block
    // --------------------------------------------------------
    alignas(32) uint8_t blk[64];
    std::memset(blk, 0, sizeof(blk));

    if (last_block_bytes == 0) {
        // no remaining message bytes -> add a pure padding block
        blk[0]  = 0x80;
        blk[63] = 0x01;
    } else {
        std::memcpy(blk, msg + full_blocks * CHIME_512_RATE_BYTES, last_block_bytes);

        if (last_block_bytes == 64) {
            // unreachable, because last_block_bytes = len_bytes % 64
        }

        if (last_block_bytes == 63) {
            // same byte gets both the first '1' and the final trailing '1'
            blk[63] = 0x81;
        } else {
            blk[last_block_bytes] = 0x80;
            blk[63] |= 0x01;
        }
    }

    absorb_one_block(blk, "final padded block");

    extract_digest512_bytes(s, digest);

    return 0;
}