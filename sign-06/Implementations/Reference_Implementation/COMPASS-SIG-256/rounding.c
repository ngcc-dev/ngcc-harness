#include <stdint.h>
#include "params.h"
#include "rounding.h"

/*************************************************
* Name:        power2round
*
* Description: For finite field element a, compute a0, a1 such that
*              a mod^+ Q = a1*2^D + a0 with -2^{D-1} < a0 <= 2^{D-1}.
*              Assumes a to be standard representative.
*
* Arguments:   - int32_t a: input element
*              - int32_t *a0: pointer to output element a0
*
* Returns a1.
**************************************************/
int32_t power2round(int32_t *a0, int32_t a)  {
  int32_t a1;

  a1 = (a + (1 << (D-1)) - 1) >> D;
  *a0 = a - (a1 << D);
  return a1;
}

/*************************************************
* Name:        decompose
*
* Description: For finite field element a, compute high and low bits a0, a1 such
*              that a mod^+ Q = a1*ALPHA + a0 with -ALPHA/2 < a0 <= ALPHA/2 except
*              if a1 = (Q-1)/ALPHA where we set a1 = 0 and
*              -ALPHA/2 <= a0 = a mod^+ Q - Q < 0. Assumes a to be standard
*              representative.
*
* Arguments:   - int32_t a: input element
*              - int32_t *a0: pointer to output element a0
*
* Returns a1.
**************************************************/
// int32_t decompose(int32_t *a0, int32_t a) {
//   int32_t a1;

//   a1  = (a + 127) >> 7;
// #if GAMMA2 == (Q-1)/32
//   a1  = (a1*1025 + (1 << 21)) >> 22;
//   a1 &= 15;
// #elif GAMMA2 == (Q-1)/88
//   a1  = (a1*11275 + (1 << 23)) >> 24;
//   a1 ^= ((43 - a1) >> 31) & a1;
// #endif

//   *a0  = a - a1*2*GAMMA2;
//   *a0 -= (((Q-1)/2 - *a0) >> 31) & Q;
//   return a1;
// }

int32_t decompose(int32_t *a0, int32_t a) {
  int32_t a1;
  int32_t x,mask;

  // 1. 将 a 加上 GAMMA2，等效于为后续的求商过程做四舍五入
  x = a + GAMMA2;

  // 2. 核心魔法除法：计算 x / (2 * GAMMA2)
  // 注意：x * M 的最大值大约是 1000万 * 100万 = 10^13，超出了 32 位整数的范围，
  // 因此必须将 x 强制转换为 int64_t 进行乘法！
  a1 = ((int64_t)x * DECOMPOSE_M + DECOMPOSE_A) >> DECOMPOSE_S;

  // 3. 计算低位 a0
  *a0 = a - a1 * 2 * GAMMA2;

  // 4. 处理模 q 环的溢出回绕边界
  // 如果 a1 达到了允许的最大值，使其回绕到 0
  // if (a1 == (Q - 1) / (2 * GAMMA2)) {
  //   a1 = 0;
  //   *a0 = a - Q;
  // }
  int32_t max_a1 = (Q - 1) / (2 * GAMMA2);
  
  // 构造掩码：当 a1 == max_a1 时，mask 为全 1；否则为全 0
  mask = (max_a1 - 1 - a1) >> 31;

  // 应用掩码：
  // 若 mask 为 0 (不回绕)：~mask 为全 1，a1 不变；mask & Q 为 0，*a0 不变。
  // 若 mask 为全 1 (回绕)：~mask 为 0，a1 被清零；mask & Q 为 Q，*a0 减去 Q。
  a1 = a1 & (~mask);      
  int32_t sub_term = (Q & mask) | ((a1 * 2 * GAMMA2) & ~mask);

  // 统一计算 *a0
  *a0 = a - sub_term;
  return a1;
}