#include <immintrin.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <iostream>
#include "chime1024.hpp"

/*
 * Loads one 448-bit rate block into A0 and A1 according to the CHIME-1024 mapping.
 * Inputs:
 *     blk: 64-byte input block
 *     mA0: output first 256-bit lane
 *     mA1: output second 256-bit lane
 * Outputs:
 *     mA0: loaded first rate lane
 *     mA1: loaded second rate lane with low 64 bits kept zero
 *
 * 按 CHIME-1024 映射将一个 448 位速率块装载到 A0 和 A1。
 * 输入：
 *     blk：64 字节输入块
 *     mA0：输出的第一个 256 位 lane
 *     mA1：输出的第二个 256 位 lane
 * 输出：
 *     mA0：装载后的第一个速率 lane
 *     mA1：低 64 位保持为 0 的第二个速率 lane
 */
static inline void load_rate448_to_A0_A1(
    const uint8_t blk[64],
    __m256i& mA0,
    __m256i& mA1
) {
    // First 256 bits go directly into A0
    mA0 = _mm256_loadu_si256((const __m256i*)(blk + 0));

    // Remaining 192 bits go into the HIGH 192 bits of A1.
    // So bytes:
    //   A1 bytes [8..31] = blk bytes [32..55]
    //   A1 bytes [0..7]  = 0
    alignas(32) uint8_t tmp[32];
    std::memset(tmp, 0, sizeof(tmp));
    std::memcpy(tmp + 8, blk + 32, 24);
    mA1 = _mm256_load_si256((const __m256i*)tmp);
}

/*
 * Applies feed-forward to the 1088-bit capacity/inner part only.
 * Inputs:
 *     old_s: state before the permutation round
 *     new_s: state after the permutation round
 * Outputs:
 *     new_s: updated in place with capacity feed-forward
 *
 * 仅对 1088 位容量/内层部分执行前馈。
 * 输入：
 *     old_s：轮前状态
 *     new_s：轮后状态
 * 输出：
 *     new_s：经过容量前馈后的状态
 */
static inline void feedforward_inner_1088(const State& old_s, State& new_s) {
    // Keep only lane0 = low 64 bits of A1
    const __m256i lo64_mask = _mm256_set_epi64x(
         0LL,  // lane3
         0LL,  // lane2
         0LL,  // lane1
        -1LL   // lane0
    );

    __m256i old_a1_lo = _mm256_and_si256(old_s.A1, lo64_mask);
    new_s.A1 = _mm256_xor_si256(new_s.A1, old_a1_lo);

    new_s.B0 = _mm256_xor_si256(new_s.B0, old_s.B0);
    new_s.B1 = _mm256_xor_si256(new_s.B1, old_s.B1);
    new_s.C0 = _mm256_xor_si256(new_s.C0, old_s.C0);
    new_s.C1 = _mm256_xor_si256(new_s.C1, old_s.C1);
}

/*
 * Computes a CHIME-1024 digest with bit-level padding and absorption.
 * Inputs:
 *     msg: message buffer
 *     len_bits: message length in bits
 *     digest: output digest buffer
 * Outputs:
 *     return value: 0 on success, negative value on error
 *
 * 使用位级填充与吸收流程计算 CHIME-1024 摘要。
 * 输入：
 *     msg：消息缓冲区
 *     len_bits：消息位长度
 *     digest：输出摘要缓冲区
 * 输出：
 *     返回值：成功返回 0，失败返回负值
 */
int chime_1024_bit_padding(const uint8_t* msg, size_t len_bits, uint8_t* digest) {
    constexpr size_t CHIME_1024_RATE_BITS  = 448;
    constexpr size_t CHIME_1024_RATE_BYTES = 56;
    init_arc();
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


    const size_t full_blocks = len_bits / CHIME_1024_RATE_BITS;
    const size_t rem_bits    = len_bits % CHIME_1024_RATE_BITS;

    auto absorb_one_block = [&](const uint8_t blk[64], bool is_last, const char* tag) {

        if (is_last) {
            // spongef final tweak before the true last absorb
            s.C1 = _mm256_xor_si256(s.C1, _mm256_set_epi64x(0, 0, 0, 1));
        }

        S old_s = s;

        __m256i mA0, mA1;
        load_rate448_to_A0_A1(blk, mA0, mA1);

        s.A0 = _mm256_xor_si256(s.A0, mA0);
        s.A1 = _mm256_xor_si256(s.A1, mA1);


        P_20(s);
        feedforward_inner_1088(old_s, s);

    };

    // 1) absorb all full 56-byte message blocks directly
    for (size_t i = 0; i < full_blocks; ++i) {
        alignas(32) uint8_t blk[64];
        std::memset(blk, 0, sizeof(blk));
        std::memcpy(blk, msg + i * CHIME_1024_RATE_BYTES, CHIME_1024_RATE_BYTES);

        // Full message blocks are never the true last block,
        // because pad10*1 always contributes at least one padded block.
        absorb_one_block(blk, false, "full block");
    }

    // 2) build final padded block(s) in MSB-first convention
    alignas(32) uint8_t blk0[64];
    alignas(32) uint8_t blk1[64];
    std::memset(blk0, 0, sizeof(blk0));
    std::memset(blk1, 0, sizeof(blk1));

    const uint8_t* tail = msg + full_blocks * CHIME_1024_RATE_BYTES;

    const size_t rem_full_bytes = rem_bits >> 3;
    const size_t rem_tail_bits  = rem_bits & 7u;

    // copy remaining whole bytes directly
    if (rem_full_bytes > 0) {
        std::memcpy(blk0, tail, rem_full_bytes);
    }

    // copy remaining partial byte prefix, MSB-first
    if (rem_tail_bits != 0) {
        const uint8_t mask = (uint8_t)(0xFFu << (8 - rem_tail_bits));
        blk0[rem_full_bytes] = (uint8_t)(tail[rem_full_bytes] & mask);
    }

    if (rem_bits <= 446 && rem_bits != 0) {
        // first padding '1' at bit position rem_bits (MSB-first in byte)
        const size_t byte_index = rem_bits >> 3;
        const size_t bit_in_byte = rem_bits & 7u;
        blk0[byte_index] |= (uint8_t)(1u << (7 - bit_in_byte));

        // final '1' at bit 447 => byte 55, bit 0
        blk0[55] |= 0x01u;

        absorb_one_block(blk0, true, "final padded block");
    }
    else if (rem_bits == 447) {
        // first '1' lands exactly at the last bit of blk0
        blk0[55] |= 0x01u;

        // closing '1' goes to the last bit of blk1
        blk1[55] |= 0x01u;

        absorb_one_block(blk0, false, "final padded block 0");
        absorb_one_block(blk1, true,  "final padded block 1");
    }
    else {
        // rem_bits == 0
        // ALWAYS need one extra padding-only block
        blk0[0]  |= 0x80u;  // first bit of block
        blk0[55] |= 0x01u;  // last bit of rate portion

        absorb_one_block(blk0, true, "padding-only block");
    }

    // digest = B0 || B1 || C0 || C1
    _mm256_storeu_si256((__m256i*)(digest +   0), s.B0);
    _mm256_storeu_si256((__m256i*)(digest +  32), s.B1);
    _mm256_storeu_si256((__m256i*)(digest +  64), s.C0);
    _mm256_storeu_si256((__m256i*)(digest +  96), s.C1);

    return 0;
}