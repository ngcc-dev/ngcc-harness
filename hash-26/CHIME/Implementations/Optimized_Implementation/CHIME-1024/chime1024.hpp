#ifndef CHIME1024_HPP
#define CHIME1024_HPP

#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>
#include "P.hpp"

/*
 * Defines the absorb rate in bits for CHIME-1024.
 * 定义 CHIME-1024 的吸收速率位宽。
 */
constexpr size_t CHIME_1024_RATE_BITS   = 448;

/*
 * Defines the absorb rate in bytes for CHIME-1024.
 * 定义 CHIME-1024 的吸收速率字节数。
 */
constexpr size_t CHIME_1024_RATE_BYTES  = CHIME_1024_RATE_BITS / 8;   // 56

/*
 * Defines the digest size in bits for CHIME-1024.
 * 定义 CHIME-1024 的摘要位宽。
 */
constexpr size_t CHIME_1024_DIGEST_BITS = 1024;

/*
 * Defines the digest size in bytes for CHIME-1024.
 * 定义 CHIME-1024 的摘要字节数。
 */
constexpr size_t CHIME_1024_DIGEST_BYTES = CHIME_1024_DIGEST_BITS / 8; // 128

/*
 * Provides the canonical state alias used by the optimized CHIME-1024 code.
 * 提供 CHIME-1024 优化代码中使用的统一状态别名。
 */
using State = S;

/*
 * Computes a CHIME-1024 digest with bit-level padding.
 * Inputs:
 *     msg: message buffer
 *     len_bits: message length in bits
 *     digest: output digest buffer
 * Outputs:
 *     return value: 0 on success, negative value on failure
 *
 * 使用位级填充计算 CHIME-1024 摘要。
 * 输入：
 *     msg：消息缓冲区
 *     len_bits：消息位长度
 *     digest：输出摘要缓冲区
 * 输出：
 *     返回值：成功返回 0，失败返回负值
 */
int chime_1024_bit_padding(const uint8_t* msg, size_t len_bits, uint8_t* digest);

#endif