#include "garnet_app.h"

   // 关键：通知下一个对比任务开始
   extern SemaphoreHandle_t xSha3StartSemaphore;
/* 性能参数配置 */
#define BENCH_ITERATIONS  10    // 每个样本循环次数，取平均值
#define TEST_SIZE_SMALL   64    // 测试短消息 (网络包)
#define TEST_SIZE_MED     1024  // 测试中等消息 (1KB)
#define TEST_SIZE_LARGE   4096  // 测试长消息 (4KB 吞吐)

/* 算法变体元数据结构 */
typedef struct {
    const char* name;
    uint64_t    counter;
    int         digest_bits;
    int         is_enabled;
} garnet_variant_t;

// 初始化 DWT 硬件计数器
void DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

uint32_t DWT_GetCycles(void) {
    return DWT->CYCCNT;
}

/**
 * @brief 核心 Benchmark 执行器
 * 严格使用 garnet_variant_executor 模板，包含 state 分配和 IV 初始化开销
 */
void Execute_Benchmark(const garnet_variant_t* var, uint32_t len_bytes) {
    if (!var->is_enabled) return;

    uint8_t digest[128];
    uint32_t start, end, total_cycles = 0;
    uint64_t m_bits = (uint64_t)len_bytes * 8;

    uint8_t *msg_buf = (uint8_t *)pvPortMalloc(len_bytes + 32);
    if (!msg_buf) return;
    memset(msg_buf, 0x61, len_bytes); // 填充 'a'

    // 1. 预热 (Warm-up)：填充 I-Cache
    garnet_variant_executor(var->digest_bits, var->counter, msg_buf, m_bits, digest);

    // 2. 正式测量循环
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        DWT->CYCCNT = 0;
        start = DWT_GetCycles();
        
        // 调用指定的标准执行器
        garnet_variant_executor(var->digest_bits, var->counter, msg_buf, m_bits, digest);
        
        end = DWT_GetCycles();
        total_cycles += (end - start);
    }

    uint32_t avg_c = total_cycles / BENCH_ITERATIONS;
    float freq_mhz = (float)SystemCoreClock / 1000000.0f;
    float time_us  = (float)avg_c / freq_mhz;
    float cpb      = (len_bytes == 0) ? 0 : (float)avg_c / len_bytes;
    float mib      = (len_bytes == 0) ? 0 : ((float)len_bytes / 1048576.0f) / (time_us / 1000000.0f);

    Safe_Printf("| %-18s | %5lu | %9lu | %9.1f | %8.2f | %8.3f |\n", 
                var->name, (unsigned long)len_bytes, (unsigned long)avg_c, time_us, cpb, mib);

    vPortFree(msg_buf);
}

void Garnet_Algorithm_Test(void *pvParameters) {
    DWT_Init();

    /* 1. 定义全变体测试矩阵 */
    garnet_variant_t variants[] = {
        // --- 512 家族 ---
        {"G512-W512",   COUNTER_512_W512,   512, 1},
        {"G512-W640",   COUNTER_512_W640,   512, 1},
        {"G512-W768",   COUNTER_512_W768,   512, 1},
        {"G512-W896",   COUNTER_512_W896,   512, 1},
        {"G512-W1024",  COUNTER_512_W1024,  512, 1},
        // --- 768 家族 ---
        {"G768-W512",   COUNTER_768_W512,   768, 1},
        // --- 1024 家族 ---
        {"G1024-W1024", COUNTER_1024_W1024, 1024, 1},
        {"G1024-W1152", COUNTER_1024_W1152, 1024, 1},
        {"G1024a-DM",   COUNTER_1024A_SP_DM,1024, 1},
        {"G1024-W2048-DM", COUNTER_1024_W2048_DM, 1024, 1}
    };
    int num_variants = sizeof(variants) / sizeof(variants[0]);
    uint32_t sizes[] = {16, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 12288, 16384};

    Safe_Printf("\n\n======================================================================\n");
    Safe_Printf("       Garnet Universal Hash Family - Full Performance Matrix\n");
    Safe_Printf("======================================================================\n");
    Safe_Printf("System: STM32F407 | Core: ARM Cortex-M4F | Frequency: %lu MHz\n", SystemCoreClock / 1000000);
    Safe_Printf("Config: CCMRAM Data Accelerated | Optimization: -O3 | Averaging: %d\n", BENCH_ITERATIONS);
    Safe_Printf("----------------------------------------------------------------------\n");
    Safe_Printf("| Variant            | Size  |   Cycles  |  Time(us) |   CPB    | MiB/s  |\n");
    Safe_Printf("|--------------------|-------|-----------|-----------|----------|--------|\n");

    /* 2. 执行全量迭代测试 */
    for (int v = 0; v < num_variants; v++) {
        for (int s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++) {
            Execute_Benchmark(&variants[v], sizes[s]);
        }
        Safe_Printf("|--------------------|-------|-----------|-----------|----------|--------|\n");
        vTaskDelay(pdMS_TO_TICKS(20)); // 给串口喘息时间
    }

    /* 3. KAT 正确性抽检 */
    uint8_t kat_d[128], kat_m[64] = {0x01};
    garnet_variant_executor(512, COUNTER_512_W1024, kat_m, 477, kat_d);
    Safe_Printf("\n[KAT Result Check] G512-W1024 (477-bit):\nDigest: ");
    for(int k=0; k<16; k++) Safe_Printf("%02x", kat_d[k]); 
    Safe_Printf("...\n");

    /* 4. RTOS 系统诊断 */
    Safe_Printf("\n--- RTOS System Diagnosis ---\n");
    char *taskBuffer = (char *)pvPortMalloc(1024);
    if (taskBuffer) {
        vTaskList(taskBuffer);
        Safe_Printf("Task          State  Prio  Stack  Num\n%s", taskBuffer);
        vTaskGetRunTimeStats(taskBuffer);
        Safe_Printf("\nTask          AbsTime       Usage\n%s", taskBuffer);
        vPortFree(taskBuffer);
    }

    Safe_Printf("\nHeap Info: Free:%u, MinFree:%u\n", 
                xPortGetFreeHeapSize(), xPortGetMinimumEverFreeHeapSize());
    Safe_Printf("======================================================================\n");
    Safe_Printf("\r\nGarnet Benchmark Suite Finished.\r\n");
    
 
    if (xSha3StartSemaphore != NULL) {
        xSemaphoreGive(xSha3StartSemaphore);
    }
    vTaskSuspend(NULL);
}