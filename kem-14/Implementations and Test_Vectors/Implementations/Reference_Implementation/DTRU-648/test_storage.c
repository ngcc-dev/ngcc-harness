#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "KEM_AlgorithmInstance.h"
#include "params.h"
#include "drng.h"

DRNG_ctx drng_algorithm;


#ifdef __APPLE__

#include <mach/mach.h>

/* macOS: 获取当前进程常驻内存（Bytes） */
static inline size_t get_resident_memory(void)
{
  struct mach_task_basic_info info;
  mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                (task_info_t)&info, &count) == KERN_SUCCESS)
    return info.resident_size;
  return 0;
}

/* macOS: 获取进程启动以来的常驻内存峰值（Bytes） */
static inline size_t get_peak_memory(void)
{
  struct mach_task_basic_info info;
  mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                (task_info_t)&info, &count) == KERN_SUCCESS)
    return info.resident_size_max;
  return 0;
}

#else

#include <string.h>

/* Linux: 从 /proc/self/status 解析指定字段（单位：kB） */
static size_t parse_status_kb(const char *key)
{
  FILE *fp = fopen("/proc/self/status", "r");
  if (!fp) return 0;
  size_t val = 0;
  char line[256];
  size_t keylen = strlen(key);
  while (fgets(line, sizeof(line), fp))
  {
    if (strncmp(line, key, keylen) == 0)
    {
      sscanf(line + keylen, ": %zu", &val);
      break;
    }
  }
  fclose(fp);
  return val * 1024;
}

/* Linux: 获取当前进程常驻内存（Bytes） */
static inline size_t get_resident_memory(void)
{
  return parse_status_kb("VmRSS");
}

/* Linux: 获取进程启动以来的常驻内存峰值（Bytes） */
static inline size_t get_peak_memory(void)
{
  return parse_status_kb("VmHWM");
}

#endif


#define NTESTS 2000

void test_storage(void)
{
  printf("\n");
  printf("DTRU-%d-%d-KEM Memory Usage\n\n", DTRU_N, DTRU_Q);

  unsigned int i;
  unsigned char k1[DTRU_SHAREDKEYBYTES], k2[DTRU_SHAREDKEYBYTES];
  unsigned char pk[DTRU_KEM_PUBLICKEYBYTES], sk[DTRU_KEM_SECRETKEYBYTES];
  unsigned char ct[DTRU_KEM_CIPHERTEXTBYTES];
  unsigned long long ss_byts1, ss_byts2, ct_byts, pk_byts, sk_byts;
  size_t base_mem, peak_mem;

  /* 1. 算法加载后未执行运算时的基础内存占用（代码段、常量、全局变量等） */
  base_mem = get_resident_memory();
  printf("Base memory (static: code/const/global/bss): %zu Bytes\n", base_mem);

  /* 2. 运行全部算法流程，系统自动跟踪内存峰值 */
  for (i = 0; i < NTESTS; i++)
  {
    kem_keygen(pk, &pk_byts, sk, &sk_byts);
    kem_enc(pk, pk_byts, k1, &ss_byts1, ct, &ct_byts);
    kem_dec(sk, sk_byts, ct, ct_byts, k2, &ss_byts2);
  }

  peak_mem = get_peak_memory();
  printf("Peak memory (runtime + static):            %zu Bytes\n", peak_mem);
  printf("Memory overhead (peak - base):             %zu Bytes\n",
         peak_mem > base_mem ? peak_mem - base_mem : 0);
  printf("\n");
}

int main(void)
{
  test_storage();
  return 0;
}
