#ifndef CHIME512_HPP
#define CHIME512_HPP

#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>
#include "P.hpp"

/*
 * Aliases the permutation state type for CHIME-512 optimized implementation.
 * 为 CHIME-512 优化实现提供置换状态类型别名。
 */
using State = S;

/*
 * Defines the absorb rate in bits for CHIME-512 byte-oriented interface.
 * 定义 CHIME-512 字节接口的吸收速率位宽。
 */
constexpr size_t CHIME_512_RATE_BITS  = 448;

/*
 * Defines the absorb rate in bytes derived from CHIME_512_RATE_BITS.
 * 定义由 CHIME_512_RATE_BITS 推导得到的吸收速率字节数。
 */
constexpr size_t CHIME_512_RATE_BYTES = CHIME_512_RATE_BITS / 8;   // 56

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
int chime_512_byte_padding(const uint8_t* msg, size_t len_bits, uint8_t* digest);

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
int chime_512_bit_padding(const uint8_t* msg, size_t len_bits, uint8_t* digest);

#endif