#ifndef GARNET_APP_H
#define GARNET_APP_H
/* 1. FreeRTOS 核心配置 (必须最先包含) */
#include "FreeRTOS.h"

/* 2. FreeRTOS 组件 (必须在 FreeRTOS.h 之后) */
#include "task.h"
#include "semphr.h"  // <--- 核心修复：必须包含它

/* 3. 项目基础头文件 */
#include "main.h"
#include "safe_printf.h"
#include "hash_garnet.h"
#include "garnet_config.h"
#include <string.h>
/*
 * C++ 兼容性包装：确保符号在 C 和 C++ 环境中链接一致
 */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 硬件周期计数器初始化 (针对 Cortex-M4)
 * 必须在 garnet_app.c 中实现，且不能标记为 static 或 inline
 */
void DWT_Init(void);

/**
 * @brief 获取当前 CPU 周期数
 * 用于 FreeRTOS 运行时间统计 (configGEN_RUN_TIME_STATS)
 */
uint32_t DWT_GetCycles(void);

/**
 * @brief Garnet 算法 FreeRTOS 任务入口
 */
void Garnet_Algorithm_Test(void *pvParameters);

/**
 * @brief 辅助打印：十六进制格式输出
 * 解决 C++ 中调用 C 打印函数的链接问题
 */
void Print_Full_Digest(const uint8_t* digest, int bits);

#ifdef __cplusplus
}
#endif

#endif /* GARNET_APP_H */