#ifndef GARNET_APP_ZEPHYR_H
#define GARNET_APP_ZEPHYR_H

/* 1. Zephyr 核心头文件 */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

/* 2. 项目基础算法头文件 */
#include "hash_garnet.h"
#include "garnet_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Garnet 算法 Zephyr 线程入口
 * 
 * 符合 Zephyr K_THREAD_DEFINE 的标准原型：
 * @param p1 线程参数1 (通常为 NULL)
 * @param p2 线程参数2 (通常为 NULL)
 * @param p3 线程参数3 (通常为 NULL)
 */
void Garnet_Algorithm_Test(void *p1, void *p2, void *p3);

/**
 * @brief 性能计时适配说明 (不推荐在头文件重新声明 DWT)
 * 
 * 在 ESP32-C6/Zephyr 中，请直接使用以下系统 API：
 * - 获取周期：k_cycle_get_32()
 * - 获取频率：sys_clock_hw_cycles_per_sec()
 */

/**
 * @brief 辅助打印：十六进制格式输出摘要
 * @param digest 摘要缓冲区指针
 * @param bits   摘要长度 (512/768/1024)
 */
void Print_Full_Digest(const uint8_t* digest, int bits);

#ifdef __cplusplus
}
#endif

#endif /* GARNET_APP_ZEPHYR_H */