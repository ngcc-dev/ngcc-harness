#ifndef POLY_TEST_H
#define POLY_TEST_H
#include <stdio.h>
#include "api.h"
#include "parameters.h"
#include "gf2x.h"
#include <string.h>   // memset, memcmp
#include <stdlib.h>   // rand, srand
#include <time.h>     // time
#define N PARAM_N

#define NB ((N + 7) / 8)
// ===== bit 操作 =====
int get_bit(const uint8_t *a, int i) {
    return (a[i >> 3] >> (i & 7)) & 1;
}

void flip_bit(uint8_t *c, int i) {
    c[i >> 3] ^= (1 << (i & 7));
}

void ring_mul_naive(uint8_t *c,
                    const uint8_t *a,
                    const uint8_t *b)
{
    memset(c, 0, NB);

    for (int i = 0; i < N; i++) {
        if (!get_bit(a, i)) continue;

        for (int j = 0; j < N; j++) {
            if (!get_bit(b, j)) continue;

            int k = i + j;
            if (k >= N) k -= N;

            flip_bit(c, k);
        }
    }

    // ✅ 清理高位（LSB体系）
    int extra = NB * 8 - N;
    if (extra > 0) {
        c[NB - 1] &= (0xFF >> extra);
    }
}

// ===== 随机生成多项式 =====
void random_poly(uint8_t *a) {
    for (int i = 0; i < NB; i++) {
        a[i] = rand() & 0xFF;
    }
    // 清除多余bit
    int extra = NB * 8 - N;
    if (extra > 0) {
        a[NB - 1] &= (0xFF >> extra);
    }
}

// ===== 比较函数 =====
int poly_equal(const uint8_t *a, const uint8_t *b) {
    for (int i = 0; i < NB; i++) {
        if (a[i] != b[i]) {
            printf("❌ 第 %d 字节首次不同\n", i);
            printf("a[%d] = 0x%02X\n", i, a[i]);
            printf("b[%d] = 0x%02X\n", i, b[i]);
            return i; // 返回第一个不一样的下标
        }
    }

    // 全部一样
    printf("✅ 全部 %d 字节都相同\n", NB);
    printf("✅ 测试通过！\n");
    return -1;
    return memcmp(a, b, (size_t)NB) == 0;
}

// ===== 打印前若干位（调试用）=====
void print_poly(const uint8_t *a, int bits) {
    for (int i = 0; i < bits; i++) {
        printf("%d", get_bit(a, i));
    }
    printf("\n");
}

// 计算 (1 + f + f^2 + ...) mod x^n
// f: 指数数组（例如 {15, 255}）
// f_len: f的长度
// n: 模 x^n
// out: 输出数组（存非零项指数）
// 返回值: 非零项个数
int compute_series(const int *f, int f_len, int n, int *out) {
    // res[i] 表示 x^i 是否存在（0/1）
    int res[n];

    // 初始化
    for (int i = 0; i < n; i++) res[i] = 0;
    res[0] = 1;

    // 递推
    for (int i = 1; i < n; i++) {
        int val = 0;
        for (int j = 0; j < f_len; j++) {
            int e = f[j];
            if (i - e >= 0) {
                val ^= res[i - e];  // F2 加法
            }
        }
        res[i] = val;
    }

    // 收集非零项
    int cnt = 0;
    for (int i = 0; i < n; i++) {
        if (res[i]) {
            out[cnt++] = i;
        }
    }

    return cnt;
}


#include <stdint.h>
#include <string.h>

void compute_poly(int n, uint8_t *res) {

    uint8_t a = 0x02;
    uint8_t ai = 1;

    res[0] = 1;
    int deg = 0;

    for (int i = 1; i <= n; i++) {

        ai = gf_mul(ai, a);   // a^i in GF(2^8)

        uint8_t next[512];
        memset(next, 0, sizeof(next));

        for (int j = 0; j <= deg; j++) {
            uint8_t v = res[j];

            next[j + 1] ^= v;               // × x
            next[j]     ^= gf_mul(v, ai);   // × a^i
        }

        deg++;

        for (int j = 0; j <= deg; j++) {
            res[j] = next[j];
        }
    }
}

void print_poly_generate(uint8_t *res, int deg) {
    for (int i = 0; i <= deg; i++) {
        printf("%d, ", res[i]);
    }
    printf("\n");
    int tmp;
    for (int i = 0; i <= deg/4; i++) {
        printf("0x");
        for (int j = 3; j >=0; j--)
        {
            tmp=i*4+j;
            if (tmp<=deg) 
            {
                if(res[tmp]<10)printf("000%0x", res[tmp]);
                else printf("00%0x", res[tmp]);
            }
            else printf("0000");
        }
        printf(", ");
    }
    printf("\n");
}


#define GF_MOD 0x11D

static inline uint8_t gf_pow_alpha(int k) {
    uint8_t r = 1;
    uint8_t alpha = 0x02;

    for (int i = 0; i < k; i++) {
        r = gf_mul(r, alpha);
    }

    return r;
}


#define ROWS 255
#define COLS 255

static uint8_t alpha_ij_pow[ROWS][COLS];

void build_table() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {

            int exp = (i + 1) * (j + 1);

            // multiplicative group order = 255
            exp %= 255;

            if (exp == 0)
                alpha_ij_pow[i][j] = 1;
            else
                alpha_ij_pow[i][j] = gf_pow_alpha(exp);
        }
    }
}


void print_table() {
    printf("static const uint8_t alpha_ij_pow[255][255] = {\n");

    for (int i = 0; i < ROWS; i++) {
        printf("    {");

        for (int j = 0; j < COLS; j++) {
            printf("%3u", alpha_ij_pow[i][j]);
            if (j != COLS - 1) printf(", ");
        }

        printf("}");

        if (i != ROWS - 1)
            printf(",\n");
        else
            printf("\n");

    }

    printf("};\n");
}
#endif