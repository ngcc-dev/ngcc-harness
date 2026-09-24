/// @file gf256.c
/// @brief GF(2^8) 单元素及向量基本运算。
///
/// 本文件的所有实现均直接使用 DOVE/2/ref 目录下 UOV 参考实现中的
/// 静态内联原语（uov/gf16.h、uov/blas_u32.h），并在文件内提供
/// 供其它翻译单元调用的非 inline 版本（与 UOV 的数学结果完全一致）。
///
/// 不可约多项式：x^8 + x^4 + x^3 + x + 1 (0x11b)
///

#include "gf256.h"

// 引入 UOV 参考实现的 32 位向量原语
// 提供 static inline: _gf256v_add_u32 / _gf256v_madd_u32 /
//                    _gf256v_mul_scalar_u32 / _gf256v_conditional_add_u32
#include "uov/blas_u32.h"

// -----------------------------------------------------------------
// 单元素运算
// -----------------------------------------------------------------

uint8_t gf256_add(uint8_t a, uint8_t b) {
    return a ^ b;
}

// 注：gf256_mul / gf256_inv 由 uov/gf16.h 以 static inline 形式提供，
// 任何包含 gf256.h 的翻译单元都可直接使用。

// -----------------------------------------------------------------
// 向量点积：sum(a[i] * b[i]) over GF(256)
// -----------------------------------------------------------------

uint8_t gf256_dot(const uint8_t *a, const uint8_t *b, size_t len) {
    uint8_t res = 0;
    for (size_t i = 0; i < len; i++) {
        res ^= gf256_mul(a[i], b[i]);
    }
    return res;
}

// -----------------------------------------------------------------
// 向量基本运算（与 UOV 等价）
// -----------------------------------------------------------------

void gf256v_add(uint8_t *b, const uint8_t *a, unsigned _num_byte) {
    _gf256v_add_u32(b, a, _num_byte);
}

void gf256v_madd(uint8_t *accu_c, const uint8_t *a, uint8_t gf256_b, unsigned _num_byte) {
    _gf256v_madd_u32(accu_c, a, gf256_b, _num_byte);
}

void gf256v_mul_scalar(uint8_t *a, uint8_t b, unsigned _num_byte) {
    _gf256v_mul_scalar_u32(a, b, _num_byte);
}

void gf256v_set_zero(uint8_t *a, unsigned _num_byte) {
    // UOV 的 gf256v_set_zero 使用 b ^= b 并依赖 Valgrind 标记；
    // 此处直接清零，行为等价（XOR(0,0) 与赋值 0 相同）。
    for (unsigned i = 0; i < _num_byte; i++) {
        a[i] = 0;
    }
}

void gf256v_conditional_add(uint8_t *accu_b, uint8_t condition, const uint8_t *a, unsigned _num_byte) {
    _gf256v_conditional_add_u32(accu_b, condition, a, _num_byte);
}
