#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../poly.h"
#include "../cbd.h"
#include "../avx2_cbd.h"
#include "../randombytes.h"

void print_failure(const char* func_name, int iter, int idx, int16_t expected, int16_t got) {
    printf("\n[FAILED] %s at iteration %d\n", func_name, iter);
    printf("  Index: %d\n", idx);
    printf("  Expected: %d\n", expected);
    printf("  Got:      %d\n", got);
    exit(1);
}

void test_cbd1() {
    printf("Testing CBD1 (N=%d, Bytes=%d)... ", DTRU_N, DTRU_CBD1_BYTES);
    
    poly r_ref, r_avx;
    uint8_t buf[DTRU_CBD1_BYTES];
    int iterations = 10000;

    for (int iter = 0; iter < iterations; iter++) {
        // 1. 生成随机输入
        for (int i = 0; i < DTRU_CBD1_BYTES; i++) {
            buf[i] = rand() & 0xFF;
        }

        // 2. 清零输出内存 (防止旧数据干扰)
        memset(&r_ref, 0, sizeof(poly));
        memset(&r_avx, 0, sizeof(poly));

        // 3. 运行函数
        cbd1(&r_ref, buf);
        cbd1_avx2_fast(&r_avx, buf);

        // 4. 比较结果
        for (int i = 0; i < DTRU_N; i++) {
            if (r_ref.coeffs[i] != r_avx.coeffs[i]) {
                print_failure("cbd1", iter, i, r_ref.coeffs[i], r_avx.coeffs[i]);
            }
        }
    }
    printf("PASSED (%d iterations)\n", iterations);
}

void test_cbd2() {
    printf("Testing CBD2 (N=%d, Bytes=%d)... ", DTRU_N, DTRU_CBD2_BYTES);
    
    poly r_ref, r_avx;
    uint8_t buf[DTRU_CBD2_BYTES];
    int iterations = 10000;

    for (int iter = 0; iter < iterations; iter++) {
        // 1. 生成随机输入
        for (int i = 0; i < DTRU_CBD2_BYTES; i++) {
            buf[i] = rand() & 0xFF;
        }

        // 2. 清零
        memset(&r_ref, 0, sizeof(poly));
        memset(&r_avx, 0, sizeof(poly));

        // 3. 运行
        cbd2(&r_ref, buf);
        cbd2_avx2_fast(&r_avx, buf);

        // 4. 比较
        for (int i = 0; i < DTRU_N; i++) {
            if (r_ref.coeffs[i] != r_avx.coeffs[i]) {
                print_failure("cbd2", iter, i, r_ref.coeffs[i], r_avx.coeffs[i]);
            }
        }
    }
    printf("PASSED (%d iterations)\n", iterations);
}

int main() {
    srand(time(NULL));
    test_cbd1();
    test_cbd2();
    return 0;
}

