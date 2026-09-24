#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <immintrin.h>
#include "poly.h"
#include "cbd.h"
#include "randombytes.h"

void print_failure(const char* func_name, int iter, int idx, int16_t expected, int16_t got) {
    printf("\n[FAILED] %s at iteration %d\n", func_name, iter);
    printf("  Index: %d\n", idx);
    printf("  Expected: %d\n", expected);
    printf("  Got:      %d\n", got);
    // exit(1);
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
        cbd1_avx2(&r_avx, buf);

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
        cbd2_avx2(&r_avx, buf);

        // 4. 比较
        for (int i = 0; i < DTRU_N; i++) {
            if (r_ref.coeffs[i] != r_avx.coeffs[i]) {
                print_failure("cbd2", iter, i, r_ref.coeffs[i], r_avx.coeffs[i]);
            }
        }
    }
    printf("PASSED (%d iterations)\n", iterations);
}

void test_cbd5() {
    printf("Testing CBD5 (N=%d, Bytes=%d)... ", DTRU_N, DTRU_CBD5_BYTES);
    
    poly r_ref, r_avx;
    uint8_t buf[DTRU_CBD5_BYTES];
    int iterations = 1;
    int cnt = 0;

    for (int iter = 0; iter < iterations; iter++) {
        // 1. 生成随机输入
        for (int i = 0; i < DTRU_CBD5_BYTES; i++) {
            buf[i] = rand() & 0xFF;
        }

        // 2. 清零输出内存 (防止旧数据干扰)
        memset(&r_ref, 0, sizeof(poly));
        memset(&r_avx, 0, sizeof(poly));

        // 3. 运行函数
        cbd5(&r_ref, buf);
        cbd5_avx2(&r_avx, buf);

        // 4. 比较结果
        for (int i = 0; i < DTRU_N; i++) {
            if (r_ref.coeffs[i] != r_avx.coeffs[i]) {
                print_failure("cbd5", iter, i, r_ref.coeffs[i], r_avx.coeffs[i]);
                cnt++;
            }
        }
    }
    if (cnt == 0)
        printf("PASSED (%d iterations)\n", iterations);
    else
        printf("FAILED (%d discrepancies found)\n", cnt);
}

void test_intrinsic() {
    __m256i a = _mm256_set_epi8(31,30,29,28,27,26,25,24,
                                 23,22,21,20,19,18,17,16,
                                 15,14,13,12,11,10,9,8,
                                 7,6,5,4,3,2,1,0);
    __m256i f0 = _mm256_permute4x64_epi64(a, 0x94);
    const __m256i shufbidx = _mm256_set_epi8(11, 10, 10, 9, 9, 8, 8, 7,
                                              6, 5, 5, 4, 4, 3, 3, 2,
                                              9, 8, 8, 7, 7, 6, 6, 5,
                                              4, 3, 3, 2, 2, 1, 1, 0);
    uint8_t in[32], out[32];
    _mm256_storeu_si256((__m256i *)in, a);
    _mm256_storeu_si256((__m256i *)out, f0);
    printf("ref:\n");
    for (int i = 0; i < 32; i++) {
        printf("%d ", in[i]);
    }
    printf("\n");
    printf("permute output:\n");
    for (int i = 0; i < 32; i++) {
        printf("%d ", out[i]);
    }
    printf("\n");

    f0 = _mm256_shuffle_epi8(f0, shufbidx);
    _mm256_storeu_si256((__m256i *)out, f0);
    printf("shuffle output:\n");
    for (int i = 0; i < 32; i++) {
        printf("%d ", out[i]);
    }
    printf("\n");
}

int main() {
    srand(time(NULL));
    // test_cbd1();
    // test_cbd2();
    // test_cbd5();
    test_intrinsic();
    return 0;
}
