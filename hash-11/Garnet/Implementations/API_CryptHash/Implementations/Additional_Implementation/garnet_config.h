#ifndef GARNET_CONFIG_H
#define GARNET_CONFIG_H
// ========================================================
// =========================================================

// 选择 Garnet-512 的具体速率变体 (只能开启一个)
#define SUBMISSION_VARIANT_512  COUNTER_512_W1024 

// 选择 Garnet-768 的具体速率变体
#define SUBMISSION_VARIANT_768  COUNTER_768_W512

// 选择 Garnet-1024 的具体速率变体 (例如选择 5x5 矩阵版本)
#define SUBMISSION_VARIANT_1024 COUNTER_1024_W2048_DM


// ============================================================================
// 【算法裁剪配置区】 (Algorithm Trimming Configuration)
// 测试阶段默认开启全部变体
// ============================================================================

// --- 4x4 架构: Garnet-512 家族 ---
#ifndef ENABLE_GARNET_512_W512
#define ENABLE_GARNET_512_W512      // counter_0: 0x02000e0a0e0a
#endif
#ifndef ENABLE_GARNET_512_W640
#define ENABLE_GARNET_512_W640      // counter_1: 0x02800e0a0e0a
#endif
#ifndef ENABLE_GARNET_512_W768
#define ENABLE_GARNET_512_W768      // counter_2: 0x03000e0a0e0a
#endif
#ifndef ENABLE_GARNET_512_W896
#define ENABLE_GARNET_512_W896      // counter_3: 0x03800e0a0e0a
#endif
#ifndef ENABLE_GARNET_512_W1024
#define ENABLE_GARNET_512_W1024     // counter_4: 0x04000e0a0e0a
#endif

// --- 4x4 架构: Garnet-768 家族 ---
#ifndef ENABLE_GARNET_768_W512
#define ENABLE_GARNET_768_W512      // counter_5: 0x02000f0b0f0b
#endif

// --- 5x5 架构: Garnet-1024 核心版 ---
#ifndef ENABLE_GARNET_1024_W1024
#define ENABLE_GARNET_1024_W1024    // counter_6: 0x0400100c100c
#endif
#ifndef ENABLE_GARNET_1024_W1152
#define ENABLE_GARNET_1024_W1152    // counter_7: 0x0480100c100c
#endif

// --- 4x4 架构: Garnet-1024a (Sponge-DM 特殊构造) ---
#ifndef ENABLE_GARNET_1024A_SP_DM
#define ENABLE_GARNET_1024A_SP_DM   // counter_8: 0x8380100c100c (MSB=1, Rate=896)
#endif

// --- 5x5 架构: Garnet-1024 超宽速率版 ---
#ifndef ENABLE_GARNET_1024_W2048_DM
#define ENABLE_GARNET_1024_W2048_DM  // counter_x: 0x8800100c100cULL 
#endif


/* --- Debug Toggle --- */
#ifndef GARNET_DEBUG
#define GARNET_DEBUG 0 /* 默认关闭：0 为关，1 为开 */
#endif


/* 
 * 1. 函数加速宏：GARNET_FAST_FUNC
 * 目标：将代码放入单周期访问的 RAM 中 (STM32 的 CCMRAM 或 ESP32 的 IRAM)
 */
#if defined(CONFIG_SOC_SERIES_ESP32C6) || defined(__ESP32__)
    // ESP32-C6: 放入 IRAM (Instruction RAM)
    #define GARNET_FAST_FUNC __attribute__((section(".iram1")))
#elif defined(STM32F4) || defined(ENABLE_CCM)
    // STM32F4: 放入 CCMRAM 代码段
    #define GARNET_FAST_FUNC __attribute__((section(".ccmram_text")))
#else
    // 默认：保持在 Flash 中
    #define GARNET_FAST_FUNC 
#endif


/* 
 * 2. 查找表加速宏：GARNET_FAST_TABLE
 * 目标：将只读数据放入单周期访问的 RAM 中 (STM32 的 CCMRAM 或 ESP32 的 DRAM)
 */
#if defined(CONFIG_SOC_SERIES_ESP32C6) || defined(__ESP32__)
    // ESP32-C6: 必须放入 DRAM (Data RAM) 才能安全进行字节访问
    #define GARNET_FAST_TABLE __attribute__((section(".dram1")))
#elif defined(STM32F4) || defined(ENABLE_CCM)
    // STM32F4: 放入 CCMRAM 数据段
    #define GARNET_FAST_TABLE __attribute__((section(".ccmram_tables")))
#else
    // 默认：保持在 Flash (const) 中
    #define GARNET_FAST_TABLE 
#endif


/* 
 * 3. 内存对齐宏：GARNET_ALIGN
 * 目标：确保 32 位访问效率，防止 RISC-V/ARM 非对齐访问性能下降
 */
#if defined(_MSC_VER)
    #define GARNET_ALIGN(n) __declspec(align(n))
#else
    #define GARNET_ALIGN(n) __attribute__((aligned(n)))
#endif


#endif // GARNET_CONFIG_H