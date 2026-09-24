#include "params.h"
#include "gf_math.h"
#include <string.h>
#include "uvw_constants.h"


gf_elem_t gf_add(gf_elem_t a, gf_elem_t b) {
    // 1. 先做加法再减去 Q
    // 如果 a + b < UVW_Q，这里会发生无符号下溢出
    // 下溢出后，最高位（第 15 位）必然会变成 1
    uint16_t res = a + b - UVW_Q;
    
    // 2. 提取最高位 (如果下溢出，这里是 1；如果没下溢出，这里是 0)
    uint16_t sign_bit = res >> 15;
    
    // 3. 强行把 1 变成 0xFFFF，把 0 变成 0x0000
    // 无符号数中，-1 的补码就是全 1 (0xFFFF)
    uint16_t mask = (uint16_t)(-sign_bit);
    
    // 4. 如果发生了下溢出，mask 是 0xFFFF，加回 UVW_Q
    //    如果没发生下溢出，mask 是 0x0000，加 0
    res += mask & UVW_Q;
    
    return (gf_elem_t)res;
}

gf_elem_t gf_sub(gf_elem_t a, gf_elem_t b) {
    // 1. 直接减
    // 如果 a < b，发生下溢出，res 的最高位变成 1
    uint16_t res = a - b;
    
    // 2. 提取下溢出标志位
    uint16_t sign_bit = res >> 15;
    
    // 3. 膨胀成掩码 (1 -> 0xFFFF, 0 -> 0x0000)
    uint16_t mask = (uint16_t)(-sign_bit);
    
    // 4. 如果发生了下溢出，说明多减了，把 UVW_Q 补回来
    res += mask & UVW_Q;
    
    return (gf_elem_t)res;
}

// 3. 模乘法: (a * b) mod q
gf_elem_t gf_mul(gf_elem_t a, gf_elem_t b) {
    // 必须转成 uint32_t 防止乘法溢出
    return (gf_elem_t)((uint32_t)a * b % UVW_Q);
}

// 4. 模幂运算: a^exp mod q
gf_elem_t gf_pow(gf_elem_t a, uint32_t exp) {
    gf_elem_t res = 1;
    gf_elem_t base = a;
    while (exp > 0) {
        if (exp & 1) res = gf_mul(res, base);
        base = gf_mul(base, base);
        exp >>= 1;
    }
    return res;
}

// // 5. 模逆元: a^(-1) mod q
// // 根据费马小定理：a^(q-2) mod q 就是逆元
// gf_elem_t gf_inv(gf_elem_t a) {
//     if (a == 0) return 0; // 0 没有逆元，返回 0
//     return gf_pow(a, UVW_Q - 2);
// }

// // 针对 Q=433 (指数 431 = 0b110101111) 手动展开 ILP 逻辑
// gf_elem_t gf_inv(gf_elem_t a) {
//     gf_elem_t res = a;                      // bit 0 为 1
//     gf_elem_t base = gf_mul(a, a);          // a^2

//     res = gf_mul(res, base);                // bit 1 为 1
//     base = gf_mul(base, base);              // a^4

//     res = gf_mul(res, base);                // bit 2 为 1
//     base = gf_mul(base, base);              // a^8

//     res = gf_mul(res, base);                // bit 3 为 1
//     base = gf_mul(base, base);              // a^16

//     // bit 4 为 0，不更新 res，只平方 base
//     base = gf_mul(base, base);              // a^32

//     res = gf_mul(res, base);                // bit 5 为 1
//     base = gf_mul(base, base);              // a^64

//     // bit 6 为 0，不更新 res，只平方 base
//     base = gf_mul(base, base);              // a^128

//     res = gf_mul(res, base);                // bit 7 为 1
//     base = gf_mul(base, base);              // a^256

//     res = gf_mul(res, base);                // bit 8 为 1
    
//     return res;                             // 最终得到 a^431
// }

gf_elem_t gf_inv(gf_elem_t a) {
    return gf_inv_table[a];
}

// 向量点积: res = u . v
gf_elem_t gf_dot_product(const gf_elem_t *u, const gf_elem_t *v, int len) {
    gf_elem_t res = 0;
    for (int i = 0; i < len; i++) {
        gf_elem_t product = gf_mul(u[i], v[i]); // 乘
        res = gf_add(res, product);             // 加
    }
    return res;
}

// 矩阵向量乘法 (延迟取模)
// out = vec * G
// vec: 1 x k
// G:   k x n
// out: 1 x n
void gf_vec_mat_mul(gf_elem_t *out, const gf_elem_t *vec, const gf_elem_t *G, int k, int n) {
    // 在栈上开辟 32 位累加器
    uint32_t accum[n];
    memset(accum, 0, n * sizeof(uint32_t));

    for (int i = 0; i < k; i++) {
        uint32_t scalar = vec[i];
        if (scalar == 0) continue; 

        const gf_elem_t *row = &G[i * n];

        for (int j = 0; j < n; j++) {
            accum[j] += scalar * (uint32_t)row[j];
        }
    }

    // 将 k x n 次取模，压缩成了 n 次
    for (int j = 0; j < n; j++) {
        out[j] = (gf_elem_t)(accum[j] % UVW_Q);
    }
}