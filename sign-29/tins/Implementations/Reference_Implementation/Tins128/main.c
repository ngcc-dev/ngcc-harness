#include "SIG_TINS128.h"
#include "drng.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  
#include "ff_arith.h"
#include "params.h"

#define SEED_LEN_BYTES 64
#define KAT_SIG_SUCCESS 0
#define KAT_ALGORITHM_INSTANCE_NAME_INVALID -1
#define KAT_FILE_OPERATE_FAILED -2
#define KAT_SIG_CRYPTO_FAILURE -3
#define KAT_MEMORY_ALLOCATION_FAILED -4

#define TEST_RUNS 1 // 运行次数

DRNG_ctx drng_algorithm;

// ==========================================
// 使用 GCC 内联汇编获取 CPU Cycles (针对 x86/x86_64 架构)
// ==========================================
static inline unsigned long long rdtsc() {
    unsigned int lo, hi;
    __asm__ volatile ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((unsigned long long)hi << 32) | lo;
}

int main() {
    unsigned char *nonce1;
    unsigned char *nonce2;
    
    // DRNG_ctx for generating seed
    DRNG_ctx drng_seed;
    // DRNG_ctx for generating message
    DRNG_ctx drng_msg;
    
    unsigned char *seed, *m, *sn, *pk, *sk;
    unsigned long long pk_len_bytes, sk_len_bytes, sn_len_bytes;
    int m_len_bytes = 56;
    int rtn;

    // ================= 1. 初始化 DRNG 环境 (只需执行一次) =================
    nonce1 = (unsigned char *)calloc(SEED_LEN_BYTES, sizeof(unsigned char));
    for (int i = 0; i < SEED_LEN_BYTES / 4; i++) {
        memcpy(nonce1 + 4 * i, "sood", 4);
    }
    init_random_number(&drng_seed, nonce1, SEED_LEN_BYTES);

    nonce2 = (unsigned char *)calloc(SEED_LEN_BYTES, sizeof(unsigned char));
    for (int i = 0; i < SEED_LEN_BYTES / 3; i++) {
        memcpy(nonce2 + 3 * i, "mrg", 3);
    }
    memcpy(nonce2 + SEED_LEN_BYTES - 1, "m", 1);
    init_random_number(&drng_msg, nonce2, SEED_LEN_BYTES);

    // ================= 2. 获取参数并分配内存 (只需执行一次) =================
    pk_len_bytes = sig_get_pk_len_bytes();
    sk_len_bytes = sig_get_sk_len_bytes();
    sn_len_bytes = sig_get_sn_len_bytes();

    pk = (unsigned char *)calloc(pk_len_bytes, sizeof(unsigned char));
    sk = (unsigned char *)calloc(sk_len_bytes, sizeof(unsigned char));
    sn = (unsigned char *)calloc(sn_len_bytes, sizeof(unsigned char));
    seed = (unsigned char *)calloc(SEED_LEN_BYTES, sizeof(unsigned char));
    m = (unsigned char *)calloc(128, sizeof(unsigned char));

    // 用于累加 CPU Cycles 的变量
    unsigned long long total_cycles_keygen = 0;
    unsigned long long total_cycles_sign = 0;
    unsigned long long total_cycles_verify = 0;
    
    unsigned long long t_start, t_end;

    // ================= 3. 执行 50 次测试循环 =================
    for (int i = 0; i < TEST_RUNS; i++) {
        // 每次循环生成不同的 seed 和 message
        get_random_number(&drng_seed, seed, SEED_LEN_BYTES * 8);
        get_random_number(&drng_msg, m, m_len_bytes * 8);

        // 注意：这步在外面，会导致每次 keygen 用同一个随机状态，必须移到循环内！
        init_random_number(&drng_algorithm, seed, SEED_LEN_BYTES);

        // --- 测量 sig_keygen ---
        t_start = rdtsc();
        rtn = sig_keygen(pk, &pk_len_bytes, sk, &sk_len_bytes);
        t_end = rdtsc();
        total_cycles_keygen += (t_end - t_start);
        printf("rtn_keygen= %d\n", rtn);

        // --- 测量 sig_sign ---
        t_start = rdtsc();
        rtn = sig_sign(sk, sk_len_bytes, m, m_len_bytes, sn, &sn_len_bytes);
        t_end = rdtsc();
        total_cycles_sign += (t_end - t_start);
       printf("rtn_sign= %d\n", rtn);

        // --- 测量 sig_verify ---
        t_start = rdtsc();
        rtn = sig_verify(pk, pk_len_bytes, sn, sn_len_bytes, m, m_len_bytes);
        t_end = rdtsc();
        total_cycles_verify += (t_end - t_start);
       printf("rtn_verify= %d\n", rtn);
    }

    // ================= 4. 计算平均值并输出 (单位: Millions of CPU Cycles) =================
    double avg_keygen = (double)total_cycles_keygen / TEST_RUNS / 1000000.0;
    double avg_sign   = (double)total_cycles_sign   / TEST_RUNS / 1000000.0;
    double avg_verify = (double)total_cycles_verify / TEST_RUNS / 1000000.0;

    printf("=== Performance Statistics (Averaged over %d runs) ===\n", TEST_RUNS);
    printf("sig_keygen : %8.3f Millions of CPU Cycles\n", avg_keygen);
    printf("sig_sign   : %8.3f Millions of CPU Cycles\n", avg_sign);
    printf("sig_verify : %8.3f Millions of CPU Cycles\n", avg_verify);

    // ================= 5. 释放内存 =================
    free(m);
    free(seed);
    free(sn);
    free(sk);
    free(pk);
    free(nonce2);
    free(nonce1);

    return 0;
}


