#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>

// 引入乐鑫 SoC 适配头文件
#include <esp_cpu.h> 
#include <esp_clk.h>

#include "garnet_app_zephyr.h"

K_SEM_DEFINE(xSha3StartSemaphore, 0, 1);

#define BENCH_ITERATIONS  10    

typedef struct {
    const char* name;
    uint64_t    counter;
    int         digest_bits;
    int         is_enabled;
} garnet_variant_t;

/**
 * @brief 执行 Benchmark
 */
void Execute_Benchmark(const garnet_variant_t* var, uint32_t len_bytes) {
    if (!var->is_enabled) return;

    uint8_t digest[128];
    uint32_t start, end;
    uint64_t total_cycles = 0;
    uint64_t m_bits = (uint64_t)len_bytes * 8;

    uint8_t *msg_buf = k_malloc(len_bytes + 32);
    if (!msg_buf) return;
    memset(msg_buf, 0x61, len_bytes); 

    garnet_variant_executor(var->digest_bits, var->counter, msg_buf, m_bits, digest);

    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        // 使用乐鑫官方 HAL API 获取真正的 CPU 周期，这在 C6 上是合法的
        start = esp_cpu_get_cycle_count(); 
        
        garnet_variant_executor(var->digest_bits, var->counter, msg_buf, m_bits, digest);
        
        end = esp_cpu_get_cycle_count();
        
        total_cycles += (uint32_t)(end - start);
    }

    uint32_t avg_c = (uint32_t)(total_cycles / BENCH_ITERATIONS);
    
    // 直接使用头文件里的函数，不需要手动 extern
    uint32_t cpu_freq = (uint32_t)esp_clk_cpu_freq();
    
    float freq_mhz = (float)cpu_freq / 1000000.0f;
    float time_us  = (float)avg_c / freq_mhz;
    float cpb      = (len_bytes == 0) ? 0 : (float)avg_c / (float)len_bytes;
    float mib      = (len_bytes == 0) ? 0 : ((float)len_bytes / 1048576.0f) / (time_us / 1000000.0f);

    printf("| %-18s | %5u | %9u | %9.1f | %8.2f | %8.3f |\n", 
                var->name, len_bytes, avg_c, (double)time_us, (double)cpb, (double)mib);

    k_free(msg_buf);
}

void Garnet_Algorithm_Test(void *p1, void *p2, void *p3) {
    // 等待串口准备就绪
    k_msleep(2000); 
    
    uint32_t actual_cpu_freq = (uint32_t)esp_clk_cpu_freq();

    printf("\n\n======================================================================\n");
    printf("       Garnet Universal Hash Family - Zephyr ESP32-C6 Full Matrix\n");
    printf("======================================================================\n");
    printf("System: ESP32-C6 | Core: RISC-V RV32IMAC | CPU Freq: %u MHz\n", actual_cpu_freq / 1000000);
    printf("Cycle Counter: Espressif SOC HAL (True CPU Cycles)\n");
    printf("Config: Internal SRAM Accelerated | Optimization: -O3 | Averaging: %d\n", BENCH_ITERATIONS);
    printf("----------------------------------------------------------------------\n");
    printf("| Variant            | Size  |   Cycles  |  Time(us) |   CPB    | MiB/s  |\n");
    printf("|--------------------|-------|-----------|-----------|----------|--------|\n");

    garnet_variant_t variants[] = {
        {"G512-W512",   COUNTER_512_W512,   512, 1},
        {"G512-W640",   COUNTER_512_W640,   512, 1},
        {"G512-W768",   COUNTER_512_W768,   512, 1},
        {"G512-W896",   COUNTER_512_W896,   512, 1},
        {"G512-W1024",  COUNTER_512_W1024,  512, 1},
        {"G768-W512",   COUNTER_768_W512,   768, 1},
        {"G1024-W1024", COUNTER_1024_W1024, 1024, 1},
        {"G1024-W1152", COUNTER_1024_W1152, 1024, 1},
        {"G1024a-DM",   COUNTER_1024A_SP_DM,1024, 1},
        {"G1024-W2048-DM", COUNTER_1024_W2048_DM, 1024, 1}
    };

    uint32_t sizes[] = {16, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 12288, 16384,32768};

    for (int v = 0; v < ARRAY_SIZE(variants); v++) {
        for (int s = 0; s < ARRAY_SIZE(sizes); s++) {
            Execute_Benchmark(&variants[v], sizes[s]);
        }
        printf("|--------------------|-------|-----------|-----------|----------|--------|\n");
        k_msleep(10); 
    }

    // 释放信号量通知后续任务
    k_sem_give(&xSha3StartSemaphore);
}

K_THREAD_DEFINE(garnet_test_id, 8192, Garnet_Algorithm_Test, NULL, NULL, NULL, 5, 0, 0);