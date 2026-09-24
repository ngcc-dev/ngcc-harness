#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"
#include "reduce.h"
#include "rounding.h"
#include "symmetric.h"

#ifdef DBENCH
#include "test/cpucycles.h"
extern const uint64_t timing_overhead;
extern uint64_t *tred, *tadd, *tmul, *tround, *tsample, *tpack;
#define DBENCH_START() uint64_t time = cpucycles()
#define DBENCH_STOP(t) t += cpucycles() - time - timing_overhead
#else
#define DBENCH_START()
#define DBENCH_STOP(t)
#endif

// =====================================================================
// W1 多项式打包
// 提取每个系数的低 POLYW1_BITS 位，紧凑拼接
// =====================================================================
void polyw1_pack(uint8_t *r, const poly *a) {
  unsigned int i;

#if POLYW1_BITS == 2
  /* 2 bit: 1 字节存 4 个系数 */
  for(i = 0; i < N/4; ++i) {
    r[i] = (a->coeffs[4*i+0] & 0x03) | 
          ((a->coeffs[4*i+1] & 0x03) << 2) | 
          ((a->coeffs[4*i+2] & 0x03) << 4) | 
          ((a->coeffs[4*i+3] & 0x03) << 6);
  }

#elif POLYW1_BITS == 3
  /* 3 bit: 3 字节存 8 个系数 */
  for(i = 0; i < N/8; ++i) {
    uint8_t t[8];
    for(int j=0; j<8; ++j) t[j] = a->coeffs[8*i+j] & 0x07;

    r[3*i+0] = t[0] | (t[1] << 3) | (t[2] << 6);
    r[3*i+1] = (t[2] >> 2) | (t[3] << 1) | (t[4] << 4) | (t[5] << 7);
    r[3*i+2] = (t[5] >> 1) | (t[6] << 2) | (t[7] << 5);
  }

#elif POLYW1_BITS == 4
  /* 4 bit: 1 字节存 2 个系数 */
  for(i = 0; i < N/2; ++i) {
    r[i] = (a->coeffs[2*i+0] & 0x0F) | ((a->coeffs[2*i+1] & 0x0F) << 4);
  }
#endif
}

// 向量级别封装


void polyeta_pack(uint8_t *r, const poly *a, int8_t eta) {
  unsigned int i;
  uint8_t t[8];

  if (eta == 1) {
    // ETA=1: 每个系数占 2-bit。4 个系数完美塞入 1 个字节
    for(i = 0; i < N/4; ++i) {
      t[0] = 1 - a->coeffs[4*i+0];
      t[1] = 1 - a->coeffs[4*i+1];
      t[2] = 1 - a->coeffs[4*i+2];
      t[3] = 1 - a->coeffs[4*i+3];
      
      r[i] = t[0] | (t[1] << 2) | (t[2] << 4) | (t[3] << 6);
    }
  } 
  else if (eta == 2) {
    // ETA=2: 每个系数占 3-bit。8 个系数完美塞入 3 个字节
    for(i = 0; i < N/8; ++i) {
      t[0] = 2 - a->coeffs[8*i+0];
      t[1] = 2 - a->coeffs[8*i+1];
      t[2] = 2 - a->coeffs[8*i+2];
      t[3] = 2 - a->coeffs[8*i+3];
      t[4] = 2 - a->coeffs[8*i+4];
      t[5] = 2 - a->coeffs[8*i+5];
      t[6] = 2 - a->coeffs[8*i+6];
      t[7] = 2 - a->coeffs[8*i+7];

      r[3*i+0]  = t[0]       | (t[1] << 3) | (t[2] << 6);
      r[3*i+1]  = (t[2] >> 2) | (t[3] << 1) | (t[4] << 4) | (t[5] << 7);
      r[3*i+2]  = (t[5] >> 1) | (t[6] << 2) | (t[7] << 5);
    }
  }
}

void polyeta_unpack(poly *r, const uint8_t *a, int8_t eta) {
  unsigned int i;

  if (eta == 1) {
    /* eta=1: 从1字节还原4个系数 */
    for(i = 0; i < N/4; ++i) {
      r->coeffs[4*i+0] = 1 - (a[i] & 0x03);
      r->coeffs[4*i+1] = 1 - ((a[i] >> 2) & 0x03);
      r->coeffs[4*i+2] = 1 - ((a[i] >> 4) & 0x03);
      r->coeffs[4*i+3] = 1 - ((a[i] >> 6) & 0x03);
    }
  } 
  else if (eta == 2) {
    /* eta=2: 从3字节还原8个系数 */
    for(i = 0; i < N/8; ++i) {
      r->coeffs[8*i+0] = 2 - (a[3*i+0] & 0x07);
      r->coeffs[8*i+1] = 2 - ((a[3*i+0] >> 3) & 0x07);
      r->coeffs[8*i+2] = 2 - ((a[3*i+0] >> 6) | ((a[3*i+1] << 2) & 0x07));
      r->coeffs[8*i+3] = 2 - ((a[3*i+1] >> 1) & 0x07);
      r->coeffs[8*i+4] = 2 - ((a[3*i+1] >> 4) & 0x07);
      r->coeffs[8*i+5] = 2 - ((a[3*i+1] >> 7) | ((a[3*i+2] << 1) & 0x07));
      r->coeffs[8*i+6] = 2 - ((a[3*i+2] >> 2) & 0x07);
      r->coeffs[8*i+7] = 2 - ((a[3*i+2] >> 5) & 0x07);
    }
  }
}

void polyz_pack(uint8_t *r, const poly *a) {
  unsigned int i;
  uint32_t t[8];

#if Z_BITS == 16
  /* 16 bit: 1个系数2字节 */
  for(i = 0; i < N; ++i) {
    t[0] = GAMMA1 - a->coeffs[i];
    r[2*i+0] = t[0];
    r[2*i+1] = t[0] >> 8;
  }

#elif Z_BITS == 18
  /* 18 bit: 4个系数9字节 */
  for(i = 0; i < N/4; ++i) {
    for(int j=0; j<4; ++j) t[j] = GAMMA1 - a->coeffs[4*i+j];

    r[9*i+0] = t[0];
    r[9*i+1] = t[0] >> 8;
    r[9*i+2] = (t[0] >> 16) | (t[1] << 2);
    r[9*i+3] = t[1] >> 6;
    r[9*i+4] = (t[1] >> 14) | (t[2] << 4);
    r[9*i+5] = t[2] >> 4;
    r[9*i+6] = (t[2] >> 12) | (t[3] << 6);
    r[9*i+7] = t[3] >> 2;
    r[9*i+8] = t[3] >> 10;
  }

#elif Z_BITS == 19
  /* 19 bit: 8个系数19字节 */
  for(i = 0; i < N/8; ++i) {
    for(int j=0; j<8; ++j) t[j] = GAMMA1 - a->coeffs[8*i+j];

    r[19*i+0]  = t[0];
    r[19*i+1]  = t[0] >> 8;
    r[19*i+2]  = (t[0] >> 16) | (t[1] << 3);
    r[19*i+3]  = t[1] >> 5;
    r[19*i+4]  = (t[1] >> 13) | (t[2] << 6);
    r[19*i+5]  = t[2] >> 2;
    r[19*i+6]  = t[2] >> 10;
    r[19*i+7]  = (t[2] >> 18) | (t[3] << 1);
    r[19*i+8]  = t[3] >> 7;
    r[19*i+9]  = (t[3] >> 15) | (t[4] << 4);
    r[19*i+10] = t[4] >> 4;
    r[19*i+11] = (t[4] >> 12) | (t[5] << 7);
    r[19*i+12] = t[5] >> 1;
    r[19*i+13] = t[5] >> 9;
    r[19*i+14] = (t[5] >> 17) | (t[6] << 2);
    r[19*i+15] = t[6] >> 6;
    r[19*i+16] = (t[6] >> 14) | (t[7] << 5);
    r[19*i+17] = t[7] >> 3;
    r[19*i+18] = t[7] >> 11;
  }

#elif Z_BITS == 20
  /* 20 bit: 2个系数5字节 */
  for(i = 0; i < N/2; ++i) {
    t[0] = GAMMA1 - a->coeffs[2*i+0];
    t[1] = GAMMA1 - a->coeffs[2*i+1];

    r[5*i+0] = t[0];
    r[5*i+1] = t[0] >> 8;
    r[5*i+2] = (t[0] >> 16) | (t[1] << 4);
    r[5*i+3] = t[1] >> 4;
    r[5*i+4] = t[1] >> 12;
  }
#endif
}

void polyz_unpack(poly *r, const uint8_t *a) {
  unsigned int i;

#if Z_BITS == 16
  for(i = 0; i < N; ++i) {
    r->coeffs[i] = GAMMA1 - ((uint32_t)a[2*i+0] | ((uint32_t)a[2*i+1] << 8));
  }

#elif Z_BITS == 18
  for(i = 0; i < N/4; ++i) {
    r->coeffs[4*i+0] = GAMMA1 - (((uint32_t)a[9*i+0]) | ((uint32_t)a[9*i+1] << 8) | (((uint32_t)a[9*i+2] & 0x03) << 16));
    r->coeffs[4*i+1] = GAMMA1 - (((uint32_t)a[9*i+2] >> 2) | ((uint32_t)a[9*i+3] << 6) | (((uint32_t)a[9*i+4] & 0x0F) << 14));
    r->coeffs[4*i+2] = GAMMA1 - (((uint32_t)a[9*i+4] >> 4) | ((uint32_t)a[9*i+5] << 4) | (((uint32_t)a[9*i+6] & 0x3F) << 12));
    r->coeffs[4*i+3] = GAMMA1 - (((uint32_t)a[9*i+6] >> 6) | ((uint32_t)a[9*i+7] << 2) | ((uint32_t)a[9*i+8] << 10));
  }

#elif Z_BITS == 19
  for(i = 0; i < N/8; ++i) {
    r->coeffs[8*i+0] = GAMMA1 - (((uint32_t)a[19*i+0]) | ((uint32_t)a[19*i+1] << 8) | (((uint32_t)a[19*i+2] & 0x07) << 16));
    r->coeffs[8*i+1] = GAMMA1 - (((uint32_t)a[19*i+2] >> 3) | ((uint32_t)a[19*i+3] << 5) | (((uint32_t)a[19*i+4] & 0x3F) << 13));
    r->coeffs[8*i+2] = GAMMA1 - (((uint32_t)a[19*i+4] >> 6) | ((uint32_t)a[19*i+5] << 2) | ((uint32_t)a[19*i+6] << 10) | (((uint32_t)a[19*i+7] & 0x01) << 18));
    r->coeffs[8*i+3] = GAMMA1 - (((uint32_t)a[19*i+7] >> 1) | ((uint32_t)a[19*i+8] << 7) | (((uint32_t)a[19*i+9] & 0x0F) << 15));
    r->coeffs[8*i+4] = GAMMA1 - (((uint32_t)a[19*i+9] >> 4) | ((uint32_t)a[19*i+10] << 4) | (((uint32_t)a[19*i+11] & 0x7F) << 12));
    r->coeffs[8*i+5] = GAMMA1 - (((uint32_t)a[19*i+11] >> 7) | ((uint32_t)a[19*i+12] << 1) | ((uint32_t)a[19*i+13] << 9) | (((uint32_t)a[19*i+14] & 0x03) << 17));
    r->coeffs[8*i+6] = GAMMA1 - (((uint32_t)a[19*i+14] >> 2) | ((uint32_t)a[19*i+15] << 6) | (((uint32_t)a[19*i+16] & 0x1F) << 14));
    r->coeffs[8*i+7] = GAMMA1 - (((uint32_t)a[19*i+16] >> 5) | ((uint32_t)a[19*i+17] << 3) | ((uint32_t)a[19*i+18] << 11));
  }

#elif Z_BITS == 20
  for(i = 0; i < N/2; ++i) {
    r->coeffs[2*i+0] = GAMMA1 - (((uint32_t)a[5*i+0]) | ((uint32_t)a[5*i+1] << 8) | (((uint32_t)a[5*i+2] & 0x0F) << 16));
    r->coeffs[2*i+1] = GAMMA1 - (((uint32_t)a[5*i+2] >> 4) | ((uint32_t)a[5*i+3] << 4) | ((uint32_t)a[5*i+4] << 12));
  }
#endif
}

void polyt1_pack(uint8_t *r, const poly *a) {
  unsigned int i;

#if T1_BITS == 16
  /* 16 bit: 每个系数 2 字节，最快 */
  for(i = 0; i < N; ++i) {
    r[2*i+0] = a->coeffs[i];
    r[2*i+1] = a->coeffs[i] >> 8;
  }

#elif T1_BITS == 17
  /* 17 bit: 8 个系数占 17 字节 */
  uint32_t t[8];
  for(i = 0; i < N/8; ++i) {
    for(int j=0; j<8; ++j) t[j] = a->coeffs[8*i+j];
    r[17*i+0]  = t[0];
    r[17*i+1]  = t[0] >> 8;
    r[17*i+2]  = (t[0] >> 16) | (t[1] << 1);
    r[17*i+3]  = t[1] >> 7;
    r[17*i+4]  = t[1] >> 15 | (t[2] << 2);
    r[17*i+5]  = t[2] >> 6;
    r[17*i+6]  = t[2] >> 14 | (t[3] << 3);
    r[17*i+7]  = t[3] >> 5;
    r[17*i+8]  = t[3] >> 13 | (t[4] << 4);
    r[17*i+9]  = t[4] >> 4;
    r[17*i+10] = t[4] >> 12 | (t[5] << 5);
    r[17*i+11] = t[5] >> 3;
    r[17*i+12] = t[5] >> 11 | (t[6] << 6);
    r[17*i+13] = t[6] >> 2;
    r[17*i+14] = t[6] >> 10 | (t[7] << 7);
    r[17*i+15] = t[7] >> 1;
    r[17*i+16] = t[7] >> 9;
  }
// 在 polyt1_pack 函数中添加：
#elif T1_BITS == 18
  /* 18 bit: 4 个系数占 9 字节 */
  uint32_t t[4];
  for(i = 0; i < N/4; ++i) {
    for(int j=0; j<4; ++j) t[j] = a->coeffs[4*i+j];
    r[9*i+0] = t[0];
    r[9*i+1] = t[0] >> 8;
    r[9*i+2] = (t[0] >> 16) | (t[1] << 2);
    r[9*i+3] = t[1] >> 6;
    r[9*i+4] = (t[1] >> 14) | (t[2] << 4);
    r[9*i+5] = t[2] >> 4;
    r[9*i+6] = (t[2] >> 12) | (t[3] << 6);
    r[9*i+7] = t[3] >> 2;
    r[9*i+8] = t[3] >> 10;
  }
#endif
}

void polyt1_unpack(poly *r, const uint8_t *a) {
  unsigned int i;

#if T1_BITS == 16
  /* 16 bit: 2 字节还原 1 个系数 */
  for(i = 0; i < N; ++i) {
    r->coeffs[i] = (uint32_t)a[2*i+0] | ((uint32_t)a[2*i+1] << 8);
  }

#elif T1_BITS == 17
  /* 17 bit: 17 字节还原 8 个系数 (最小对齐单元) */
  for(i = 0; i < N/8; ++i) {
    const uint8_t *ptr = a + 17*i;
    
    r->coeffs[8*i+0] = ((uint32_t)ptr[0])       | ((uint32_t)ptr[1] << 8)  | (((uint32_t)ptr[2] & 0x01) << 16);
    r->coeffs[8*i+1] = ((uint32_t)ptr[2] >> 1)  | ((uint32_t)ptr[3] << 7)  | (((uint32_t)ptr[4] & 0x03) << 15);
    r->coeffs[8*i+2] = ((uint32_t)ptr[4] >> 2)  | ((uint32_t)ptr[5] << 6)  | (((uint32_t)ptr[6] & 0x07) << 14);
    r->coeffs[8*i+3] = ((uint32_t)ptr[6] >> 3)  | ((uint32_t)ptr[7] << 5)  | (((uint32_t)ptr[8] & 0x0F) << 13);
    r->coeffs[8*i+4] = ((uint32_t)ptr[8] >> 4)  | ((uint32_t)ptr[9] << 4)  | (((uint32_t)ptr[10] & 0x1F) << 12);
    r->coeffs[8*i+5] = ((uint32_t)ptr[10] >> 5) | ((uint32_t)ptr[11] << 3) | (((uint32_t)ptr[12] & 0x3F) << 11);
    r->coeffs[8*i+6] = ((uint32_t)ptr[12] >> 6) | ((uint32_t)ptr[13] << 2) | (((uint32_t)ptr[14] & 0x7F) << 10);
    r->coeffs[8*i+7] = ((uint32_t)ptr[14] >> 7) | ((uint32_t)ptr[15] << 1) | (((uint32_t)ptr[16]) << 9);
  }
// 在 polyt1_unpack 函数中添加：
#elif T1_BITS == 18
  /* 18 bit: 9 字节还原 4 个系数 */
  for(i = 0; i < N/4; ++i) {
    const uint8_t *ptr = a + 9*i;
    r->coeffs[4*i+0] = ((uint32_t)ptr[0])       | ((uint32_t)ptr[1] << 8) | (((uint32_t)ptr[2] & 0x03) << 16);
    r->coeffs[4*i+1] = ((uint32_t)ptr[2] >> 2)  | ((uint32_t)ptr[3] << 6) | (((uint32_t)ptr[4] & 0x0F) << 14);
    r->coeffs[4*i+2] = ((uint32_t)ptr[4] >> 4)  | ((uint32_t)ptr[5] << 4) | (((uint32_t)ptr[6] & 0x3F) << 12);
    r->coeffs[4*i+3] = ((uint32_t)ptr[6] >> 6)  | ((uint32_t)ptr[7] << 2) | (((uint32_t)ptr[8]) << 10);
  }
#endif
}

void polyt0_pack(uint8_t *r, const poly *a) {
  unsigned int i;
  uint32_t t[8];

#if D == 4
  /* d=4: 每个系数占 4 bits，1字节存2个系数 */
  for(i = 0; i < N/2; ++i) {
    t[0] = (1 << 3) - a->coeffs[2*i+0];
    t[1] = (1 << 3) - a->coeffs[2*i+1];
    r[i] = t[0] | (t[1] << 4);
  }

#elif D == 5
  /* d=5: 每个系数占 5 bits，5字节存8个系数 */
  for(i = 0; i < N/8; ++i) {
    for(int j = 0; j < 8; ++j) t[j] = (1 << 4) - a->coeffs[8*i+j];

    r[5*i+0] = t[0] | (t[1] << 5);
    r[5*i+1] = (t[1] >> 3) | (t[2] << 2) | (t[3] << 7);
    r[5*i+2] = (t[3] >> 1) | (t[4] << 4);
    r[5*i+3] = (t[4] >> 4) | (t[5] << 1) | (t[6] << 6);
    r[5*i+4] = (t[6] >> 2) | (t[7] << 3);
  }

#elif D == 6
  /* d=6: 每个系数占 6 bits，3字节存4个系数 */
  for(i = 0; i < N/4; ++i) {
    for(int j = 0; j < 4; ++j) t[j] = (1 << 5) - a->coeffs[4*i+j];

    r[3*i+0] = t[0] | (t[1] << 6);
    r[3*i+1] = (t[1] >> 2) | (t[2] << 4);
    r[3*i+2] = (t[2] >> 4) | (t[3] << 2);
  }
#endif
}

void polyt0_unpack(poly *r, const uint8_t *a) {
  unsigned int i;

#if D == 4
  /* d=4: 1字节还原2个系数 */
  for(i = 0; i < N/2; ++i) {
    r->coeffs[2*i+0] = (1 << 3) - (a[i] & 0x0F);
    r->coeffs[2*i+1] = (1 << 3) - (a[i] >> 4);
  }

#elif D == 5
  /* d=5: 5字节还原8个系数 */
  for(i = 0; i < N/8; ++i) {
    r->coeffs[8*i+0] = (1 << 4) - (a[5*i+0] & 0x1F);
    r->coeffs[8*i+1] = (1 << 4) - ((a[5*i+0] >> 5) | ((a[5*i+1] & 0x03) << 3));
    r->coeffs[8*i+2] = (1 << 4) - ((a[5*i+1] >> 2) & 0x1F);
    r->coeffs[8*i+3] = (1 << 4) - ((a[5*i+1] >> 7) | ((a[5*i+2] & 0x0F) << 1));
    r->coeffs[8*i+4] = (1 << 4) - ((a[5*i+2] >> 4) | ((a[5*i+3] & 0x01) << 4));
    r->coeffs[8*i+5] = (1 << 4) - ((a[5*i+3] >> 1) & 0x1F);
    r->coeffs[8*i+6] = (1 << 4) - ((a[5*i+3] >> 6) | ((a[5*i+4] & 0x07) << 2));
    r->coeffs[8*i+7] = (1 << 4) - (a[5*i+4] >> 3);
  }

#elif D == 6
  /* d=6: 3字节还原4个系数 */
  for(i = 0; i < N/4; ++i) {
    r->coeffs[4*i+0] = (1 << 5) - (a[3*i+0] & 0x3F);
    r->coeffs[4*i+1] = (1 << 5) - ((a[3*i+0] >> 6) | ((a[3*i+1] & 0x0F) << 2));
    r->coeffs[4*i+2] = (1 << 5) - ((a[3*i+1] >> 4) | ((a[3*i+2] & 0x03) << 4));
    r->coeffs[4*i+3] = (1 << 5) - (a[3*i+2] >> 2);
  }
#endif
}

uint64_t poly_sqnorm(const poly *a) {
  unsigned int i;
  uint64_t sum_sq = 0;
  for(i = 0; i < N; ++i) {
    sum_sq += (uint64_t)a->coeffs[i] * a->coeffs[i];
  }
  return sum_sq;
}

/*************************************************
* Name:        poly_reduce
*
* Description: Inplace reduction of all coefficients of polynomial to
*              representative in [-6283008,6283008].
*
* Arguments:   - poly *a: pointer to input/output polynomial
**************************************************/
void poly_reduce(poly *a) {
  unsigned int i;
  DBENCH_START();

  for(i = 0; i < N; ++i)
    a->coeffs[i] = reduce32(a->coeffs[i]);

  DBENCH_STOP(*tred);
}

/*************************************************
* Name:        poly_caddq
*
* Description: For all coefficients of in/out polynomial add Q if
*              coefficient is negative.
*
* Arguments:   - poly *a: pointer to input/output polynomial
**************************************************/
void poly_caddq(poly *a) {
  unsigned int i;
  DBENCH_START();

  for(i = 0; i < N; ++i)
    a->coeffs[i] = caddq(a->coeffs[i]);

  DBENCH_STOP(*tred);
}

/*************************************************
* Name:        poly_add
*
* Description: Add polynomials. No modular reduction is performed.
*
* Arguments:   - poly *c: pointer to output polynomial
*              - const poly *a: pointer to first summand
*              - const poly *b: pointer to second summand
**************************************************/
void poly_add(poly *c, const poly *a, const poly *b)  {
  unsigned int i;
  DBENCH_START();

  for(i = 0; i < N; ++i)
    c->coeffs[i] = a->coeffs[i] + b->coeffs[i];

  DBENCH_STOP(*tadd);
}

/*************************************************
* Name:        poly_sub
*
* Description: Subtract polynomials. No modular reduction is
*              performed.
*
* Arguments:   - poly *c: pointer to output polynomial
*              - const poly *a: pointer to first input polynomial
*              - const poly *b: pointer to second input polynomial to be
*                               subtraced from first input polynomial
**************************************************/
void poly_sub(poly *c, const poly *a, const poly *b) {
  unsigned int i;
  DBENCH_START();

  for(i = 0; i < N; ++i)
    c->coeffs[i] = a->coeffs[i] - b->coeffs[i];

  DBENCH_STOP(*tadd);
}

/*************************************************
* Name:        poly_shiftl
*
* Description: Multiply polynomial by 2^D without modular reduction. Assumes
*              input coefficients to be less than 2^{31-D} in absolute value.
*
* Arguments:   - poly *a: pointer to input/output polynomial
**************************************************/
void poly_shiftl(poly *a) {
  unsigned int i;
  DBENCH_START();

  for(i = 0; i < N; ++i)
    a->coeffs[i] <<= D;

  DBENCH_STOP(*tmul);
}

/*************************************************
* Name:        poly_ntt
*
* Description: Inplace forward NTT. Coefficients can grow by
*              8*Q in absolute value.
*
* Arguments:   - poly *a: pointer to input/output polynomial
**************************************************/
void poly_ntt(poly *a) {
  DBENCH_START();

  ntt(a->coeffs);

  DBENCH_STOP(*tmul);
}

/*************************************************
* Name:        poly_invntt_tomont
*
* Description: Inplace inverse NTT and multiplication by 2^{32}.
*              Input coefficients need to be less than Q in absolute
*              value and output coefficients are again bounded by Q.
*
* Arguments:   - poly *a: pointer to input/output polynomial
**************************************************/
void poly_invntt_tomont(poly *a) {
  DBENCH_START();

  invntt_tomont(a->coeffs);

  DBENCH_STOP(*tmul);
}

/*************************************************
* Name:        poly_pointwise_montgomery
*
* Description: Pointwise multiplication of polynomials in NTT domain
*              representation and multiplication of resulting polynomial
*              by 2^{-32}.
*
* Arguments:   - poly *c: pointer to output polynomial
*              - const poly *a: pointer to first input polynomial
*              - const poly *b: pointer to second input polynomial
**************************************************/
void poly_pointwise_montgomery(poly *c, const poly *a, const poly *b) {
  unsigned int i;
  DBENCH_START();

  for(i = 0; i < N; ++i)
    c->coeffs[i] = montgomery_reduce((int64_t)a->coeffs[i] * b->coeffs[i]);

  DBENCH_STOP(*tmul);
}

/*************************************************
* Name:        poly_power2round
*
* Description: For all coefficients c of the input polynomial,
*              compute c0, c1 such that c mod Q = c1*2^D + c0
*              with -2^{D-1} < c0 <= 2^{D-1}. Assumes coefficients to be
*              standard representatives.
*
* Arguments:   - poly *a1: pointer to output polynomial with coefficients c1
*              - poly *a0: pointer to output polynomial with coefficients c0
*              - const poly *a: pointer to input polynomial
**************************************************/
void poly_power2round(poly *a1, poly *a0, const poly *a) {
  unsigned int i;
  DBENCH_START();

  for(i = 0; i < N; ++i)
    a1->coeffs[i] = power2round(&a0->coeffs[i], a->coeffs[i]);

  DBENCH_STOP(*tround);
}

/*************************************************
* Name:        poly_decompose
*
* Description: For all coefficients c of the input polynomial,
*              compute high and low bits c0, c1 such c mod Q = c1*ALPHA + c0
*              with -ALPHA/2 < c0 <= ALPHA/2 except c1 = (Q-1)/ALPHA where we
*              set c1 = 0 and -ALPHA/2 <= c0 = c mod Q - Q < 0.
*              Assumes coefficients to be standard representatives.
*
* Arguments:   - poly *a1: pointer to output polynomial with coefficients c1
*              - poly *a0: pointer to output polynomial with coefficients c0
*              - const poly *a: pointer to input polynomial
**************************************************/
void poly_decompose(poly *a1, poly *a0, const poly *a) {
  unsigned int i;
  DBENCH_START();

  for(i = 0; i < N; ++i)
    a1->coeffs[i] = decompose(&a0->coeffs[i], a->coeffs[i]);

  DBENCH_STOP(*tround);
}

/*************************************************
* Name:        poly_chknorm
*
* Description: Check infinity norm of polynomial against given bound.
*              Assumes input coefficients were reduced by reduce32().
*
* Arguments:   - const poly *a: pointer to polynomial
*              - int32_t B: norm bound
*
* Returns 0 if norm is strictly smaller than B <= (Q-1)/8 and 1 otherwise.
**************************************************/
int poly_chknorm(const poly *a, int32_t B) {
  unsigned int i;
  int32_t t;
  DBENCH_START();

  // if(B > (Q-1)/8)
  //   return 1;

  /* It is ok to leak which coefficient violates the bound since
     the probability for each coefficient is independent of secret
     data but we must not leak the sign of the centralized representative. */
  for(i = 0; i < N; ++i) {
    /* Absolute value */
    t = a->coeffs[i] >> 31;
    t = a->coeffs[i] - (t & 2*a->coeffs[i]);

    if(t >= B) {
      DBENCH_STOP(*tsample);
      return 1;
    }
  }

  DBENCH_STOP(*tsample);
  return 0;
}

/*************************************************
* Name:        rej_uniform
*
* Description: Sample uniformly random coefficients in [0, Q-1] by
*              performing rejection sampling on array of random bytes.
*
* Arguments:   - int32_t *a: pointer to output array (allocated)
*              - unsigned int len: number of coefficients to be sampled
*              - const uint8_t *buf: array of random bytes
*              - unsigned int buflen: length of array of random bytes
*
* Returns number of sampled coefficients. Can be smaller than len if not enough
* random bytes were given.
**************************************************/
static unsigned int rej_uniform(int32_t *a,
                                unsigned int len,
                                const uint8_t *buf,
                                unsigned int buflen)
{
  unsigned int ctr, pos;
  uint32_t t;
  DBENCH_START();

  ctr = pos = 0;
  while(ctr < len && pos + 3 <= buflen) {
    t  = buf[pos++];
    t |= (uint32_t)buf[pos++] << 8;
    t |= (uint32_t)buf[pos++] << 16;
#if Q == 2081281
  t &= 0x1FFFFF; // 保留 21 位 (最大值 2,097,151)
                 // 接受率: 2081281 / 2097152 = 99.2% !!
#elif Q == 8380417
  t &= 0x7FFFFF; // 保留 23 位 (最大值 8,388,607)
                 // 接受率: 99.9%
#else
  #error "Missing mask for current Q"
#endif

    if(t < Q)
      a[ctr++] = t;
  }

  DBENCH_STOP(*tsample);
  return ctr;
}

/*************************************************
* Name:        poly_uniform
*
* Description: Sample polynomial with uniformly random coefficients
*              in [0,Q-1] by performing rejection sampling on the
*              output stream of SHAKE128(seed|nonce)
*
* Arguments:   - poly *a: pointer to output polynomial
*              - const uint8_t seed[]: byte array with seed of length SEEDBYTES
*              - uint16_t nonce: 2-byte nonce
**************************************************/
#define POLY_UNIFORM_NBLOCKS ((768 + STREAM128_BLOCKBYTES - 1)/STREAM128_BLOCKBYTES)
void poly_uniform(poly *a,
                  const uint8_t seed[SEEDBYTES],
                  uint16_t nonce)
{
  unsigned int i, ctr, off;
  unsigned int buflen = POLY_UNIFORM_NBLOCKS*STREAM128_BLOCKBYTES;
  uint8_t buf[POLY_UNIFORM_NBLOCKS*STREAM128_BLOCKBYTES + 2];
  stream128_state state;

  stream128_init(&state, seed, nonce);
  stream128_squeezeblocks(buf, POLY_UNIFORM_NBLOCKS, &state);

  ctr = rej_uniform(a->coeffs, N, buf, buflen);

  while(ctr < N) {
    off = buflen % 3;
    for(i = 0; i < off; ++i)
      buf[i] = buf[buflen - off + i];

    stream128_squeezeblocks(buf + off, 1, &state);
    buflen = STREAM128_BLOCKBYTES + off;
    ctr += rej_uniform(a->coeffs + ctr, N - ctr, buf, buflen);
  }
}

/*************************************************
* Name:        rej_eta
*
* Description: Sample uniformly random coefficients in [-ETA, ETA] by
*              performing rejection sampling on array of random bytes.
*
* Arguments:   - int32_t *a: pointer to output array (allocated)
*              - unsigned int len: number of coefficients to be sampled
*              - const uint8_t *buf: array of random bytes
*              - unsigned int buflen: length of array of random bytes
*
* Returns number of sampled coefficients. Can be smaller than len if not enough
* random bytes were given.
**************************************************/
// static unsigned int rej_eta(int32_t *a,
//                             unsigned int len,
//                             const uint8_t *buf,
//                             unsigned int buflen)
// {
//   unsigned int ctr, pos;
//   uint32_t t0, t1;
//   DBENCH_START();

//   ctr = pos = 0;
//   while(ctr < len && pos < buflen) {
//     t0 = buf[pos] & 0x0F;
//     t1 = buf[pos++] >> 4;

// #if ETA == 2
//     if(t0 < 15) {
//       t0 = t0 - (205*t0 >> 10)*5;
//       a[ctr++] = 2 - t0;
//     }
//     if(t1 < 15 && ctr < len) {
//       t1 = t1 - (205*t1 >> 10)*5;
//       a[ctr++] = 2 - t1;
//     }
// #elif ETA == 4
//     if(t0 < 9)
//       a[ctr++] = 4 - t0;
//     if(t1 < 9 && ctr < len)
//       a[ctr++] = 4 - t1;
// #endif
//   }

//   DBENCH_STOP(*tsample);
//   return ctr;
// }

/*************************************************
* Name:        poly_uniform_eta
*
* Description: Sample polynomial with uniformly random coefficients
*              in [-ETA,ETA] by performing rejection sampling on the
*              output stream from SHAKE256(seed|nonce)
*
* Arguments:   - poly *a: pointer to output polynomial
*              - const uint8_t seed[]: byte array with seed of length CRHBYTES
*              - uint16_t nonce: 2-byte nonce
**************************************************/
// #if ETA == 2
// #define POLY_UNIFORM_ETA_NBLOCKS ((136 + STREAM256_BLOCKBYTES - 1)/STREAM256_BLOCKBYTES)
// #elif ETA == 4
// #define POLY_UNIFORM_ETA_NBLOCKS ((227 + STREAM256_BLOCKBYTES - 1)/STREAM256_BLOCKBYTES)
// #endif
void poly_uniform_eta(poly *a,
                      const uint8_t seed[CRHBYTES],
                      uint16_t nonce,
                      int8_t eta)
{
  unsigned int ctr = 0;
  unsigned int pos = 0;
  uint8_t buf[SHAKE256_RATE];
  keccak_state state;
  uint32_t t;

  // 初始化 SHAKE256 并吸收种子和 nonce (这部分保留原样)
  shake256_init(&state);
  shake256_absorb(&state, seed, CRHBYTES);
  shake256_absorb(&state, (uint8_t *)&nonce, 2);
  shake256_finalize(&state);

  while(ctr < N) {
    // 挤出随机字节流
    shake256_squeezeblocks(buf, 1, &state);
    pos = 0;

    // ==========================================
    // 处理 ETA = 1 的采样逻辑
    // ==========================================
    if (eta == 1) {
      while(ctr < N && pos + 1 < SHAKE256_RATE) {
        t = buf[pos] | ((uint32_t)buf[pos+1] << 8);
        pos += 2;
        
        // 16 bit 分为 8 组 2-bit
        for(int i = 0; i < 8 && ctr < N; ++i) {
          uint32_t c = (t >> (2 * i)) & 0x03;
          if(c < 3) {
            a->coeffs[ctr++] = (int32_t)c - 1;
          }
        }
      }
    }
    
    // ==========================================
    // 处理 ETA = 2 的采样逻辑
    // ==========================================
    else if (eta == 2) {
      while(ctr < N && pos + 1 < SHAKE256_RATE) {
        t = buf[pos] | ((uint32_t)buf[pos+1] << 8);
        pos += 2;

        // 16 bit 中提取 5 组 3-bit
        for(int i = 0; i < 5 && ctr < N; ++i) {
          uint32_t c = (t >> (3 * i)) & 0x07;
          if(c < 5) {
            a->coeffs[ctr++] = (int32_t)c - 2;
          }
        }
        // 剩余的 1 bit 被自然舍弃，符合“16bit利用其中15bit”的方案
      }
    }
  }
}

/*************************************************
* Name:        poly_uniform_gamma1m1
*
* Description: Sample polynomial with uniformly random coefficients
*              in [-(GAMMA1 - 1), GAMMA1] by unpacking output stream
*              of SHAKE256(seed|nonce)
*
* Arguments:   - poly *a: pointer to output polynomial
*              - const uint8_t seed[]: byte array with seed of length CRHBYTES
*              - uint16_t nonce: 16-bit nonce
**************************************************/
#define POLY_UNIFORM_GAMMA1_NBLOCKS ((POLYZ_PACKEDBYTES + STREAM256_BLOCKBYTES - 1)/STREAM256_BLOCKBYTES)
void poly_uniform_gamma1(poly *a,
                         const uint8_t seed[CRHBYTES],
                         uint16_t nonce)
{
  uint8_t buf[POLY_UNIFORM_GAMMA1_NBLOCKS*STREAM256_BLOCKBYTES];
  stream256_state state;

  stream256_init(&state, seed, nonce);
  stream256_squeezeblocks(buf, POLY_UNIFORM_GAMMA1_NBLOCKS, &state);
  polyz_unpack(a, buf);
}

/*************************************************
* Name:        challenge
*
* Description: Implementation of H. Samples polynomial with TAU nonzero
*              coefficients in {-1,1} using the output stream of
*              SHAKE256(seed).
*
* Arguments:   - poly *c: pointer to output polynomial
*              - const uint8_t mu[]: byte array containing seed of length CTILDEBYTES
**************************************************/
void poly_challenge(poly *c, const uint8_t seed[CTILDEBYTES]) {
  unsigned int i, b, pos;
  uint64_t signs;
  uint8_t buf[SHAKE256_RATE];
  keccak_state state;

  shake256_init(&state);
  shake256_absorb(&state, seed, CTILDEBYTES);
  shake256_finalize(&state);
  shake256_squeezeblocks(buf, 1, &state);

  signs = 0;
  for(i = 0; i < 8; ++i)
    signs |= (uint64_t)buf[i] << 8*i;
  pos = 8;

  for(i = 0; i < N; ++i)
    c->coeffs[i] = 0;
  for(i = N-TAU; i < N; ++i) {
    do {
      if(pos >= SHAKE256_RATE) {
        shake256_squeezeblocks(buf, 1, &state);
        pos = 0;
      }

      b = buf[pos++];
    } while(b > i);

    c->coeffs[i] = c->coeffs[b];
    c->coeffs[b] = 1 - 2*(signs & 1);
    signs >>= 1;
  }
}

/*************************************************
* Name:        polyeta_pack
*
* Description: Bit-pack polynomial with coefficients in [-ETA,ETA].
*
* Arguments:   - uint8_t *r: pointer to output byte array with at least
*                            POLYETA_PACKEDBYTES bytes
*              - const poly *a: pointer to input polynomial
**************************************************/
// void polyeta_pack(uint8_t *r, const poly *a) {
//   unsigned int i;
//   uint8_t t[8];
//   DBENCH_START();

// #if ETA == 2
//   for(i = 0; i < N/8; ++i) {
//     t[0] = ETA - a->coeffs[8*i+0];
//     t[1] = ETA - a->coeffs[8*i+1];
//     t[2] = ETA - a->coeffs[8*i+2];
//     t[3] = ETA - a->coeffs[8*i+3];
//     t[4] = ETA - a->coeffs[8*i+4];
//     t[5] = ETA - a->coeffs[8*i+5];
//     t[6] = ETA - a->coeffs[8*i+6];
//     t[7] = ETA - a->coeffs[8*i+7];

//     r[3*i+0]  = (t[0] >> 0) | (t[1] << 3) | (t[2] << 6);
//     r[3*i+1]  = (t[2] >> 2) | (t[3] << 1) | (t[4] << 4) | (t[5] << 7);
//     r[3*i+2]  = (t[5] >> 1) | (t[6] << 2) | (t[7] << 5);
//   }
// #elif ETA == 4
//   for(i = 0; i < N/2; ++i) {
//     t[0] = ETA - a->coeffs[2*i+0];
//     t[1] = ETA - a->coeffs[2*i+1];
//     r[i] = t[0] | (t[1] << 4);
//   }
// #endif

//   DBENCH_STOP(*tpack);
// }

/*************************************************
* Name:        polyeta_unpack
*
* Description: Unpack polynomial with coefficients in [-ETA,ETA].
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *a: byte array with bit-packed polynomial
**************************************************/
// void polyeta_unpack(poly *r, const uint8_t *a) {
//   unsigned int i;
//   DBENCH_START();

// #if ETA == 2
//   for(i = 0; i < N/8; ++i) {
//     r->coeffs[8*i+0] =  (a[3*i+0] >> 0) & 7;
//     r->coeffs[8*i+1] =  (a[3*i+0] >> 3) & 7;
//     r->coeffs[8*i+2] = ((a[3*i+0] >> 6) | (a[3*i+1] << 2)) & 7;
//     r->coeffs[8*i+3] =  (a[3*i+1] >> 1) & 7;
//     r->coeffs[8*i+4] =  (a[3*i+1] >> 4) & 7;
//     r->coeffs[8*i+5] = ((a[3*i+1] >> 7) | (a[3*i+2] << 1)) & 7;
//     r->coeffs[8*i+6] =  (a[3*i+2] >> 2) & 7;
//     r->coeffs[8*i+7] =  (a[3*i+2] >> 5) & 7;

//     r->coeffs[8*i+0] = ETA - r->coeffs[8*i+0];
//     r->coeffs[8*i+1] = ETA - r->coeffs[8*i+1];
//     r->coeffs[8*i+2] = ETA - r->coeffs[8*i+2];
//     r->coeffs[8*i+3] = ETA - r->coeffs[8*i+3];
//     r->coeffs[8*i+4] = ETA - r->coeffs[8*i+4];
//     r->coeffs[8*i+5] = ETA - r->coeffs[8*i+5];
//     r->coeffs[8*i+6] = ETA - r->coeffs[8*i+6];
//     r->coeffs[8*i+7] = ETA - r->coeffs[8*i+7];
//   }
// #elif ETA == 4
//   for(i = 0; i < N/2; ++i) {
//     r->coeffs[2*i+0] = a[i] & 0x0F;
//     r->coeffs[2*i+1] = a[i] >> 4;
//     r->coeffs[2*i+0] = ETA - r->coeffs[2*i+0];
//     r->coeffs[2*i+1] = ETA - r->coeffs[2*i+1];
//   }
// #endif

//   DBENCH_STOP(*tpack);
// }

/*************************************************
* Name:        polyt1_pack
*
* Description: Bit-pack polynomial t1 with coefficients fitting in 10 bits.
*              Input coefficients are assumed to be standard representatives.
*
* Arguments:   - uint8_t *r: pointer to output byte array with at least
*                            POLYT1_PACKEDBYTES bytes
*              - const poly *a: pointer to input polynomial
**************************************************/
// void polyt1_pack(uint8_t *r, const poly *a) {
//   unsigned int i;
//   DBENCH_START();

//   for(i = 0; i < N/4; ++i) {
//     r[5*i+0] = (a->coeffs[4*i+0] >> 0);
//     r[5*i+1] = (a->coeffs[4*i+0] >> 8) | (a->coeffs[4*i+1] << 2);
//     r[5*i+2] = (a->coeffs[4*i+1] >> 6) | (a->coeffs[4*i+2] << 4);
//     r[5*i+3] = (a->coeffs[4*i+2] >> 4) | (a->coeffs[4*i+3] << 6);
//     r[5*i+4] = (a->coeffs[4*i+3] >> 2);
//   }

//   DBENCH_STOP(*tpack);
// }

/*************************************************
* Name:        polyt1_unpack
*
* Description: Unpack polynomial t1 with 10-bit coefficients.
*              Output coefficients are standard representatives.
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *a: byte array with bit-packed polynomial
**************************************************/
// void polyt1_unpack(poly *r, const uint8_t *a) {
//   unsigned int i;
//   DBENCH_START();

//   for(i = 0; i < N/4; ++i) {
//     r->coeffs[4*i+0] = ((a[5*i+0] >> 0) | ((uint32_t)a[5*i+1] << 8)) & 0x3FF;
//     r->coeffs[4*i+1] = ((a[5*i+1] >> 2) | ((uint32_t)a[5*i+2] << 6)) & 0x3FF;
//     r->coeffs[4*i+2] = ((a[5*i+2] >> 4) | ((uint32_t)a[5*i+3] << 4)) & 0x3FF;
//     r->coeffs[4*i+3] = ((a[5*i+3] >> 6) | ((uint32_t)a[5*i+4] << 2)) & 0x3FF;
//   }

//   DBENCH_STOP(*tpack);
// }

/*************************************************
* Name:        polyt0_pack
*
* Description: Bit-pack polynomial t0 with coefficients in ]-2^{D-1}, 2^{D-1}].
*
* Arguments:   - uint8_t *r: pointer to output byte array with at least
*                            POLYT0_PACKEDBYTES bytes
*              - const poly *a: pointer to input polynomial
**************************************************/
// void polyt0_pack(uint8_t *r, const poly *a) {
//   unsigned int i;
//   uint32_t t[8];
//   DBENCH_START();

//   for(i = 0; i < N/8; ++i) {
//     t[0] = (1 << (D-1)) - a->coeffs[8*i+0];
//     t[1] = (1 << (D-1)) - a->coeffs[8*i+1];
//     t[2] = (1 << (D-1)) - a->coeffs[8*i+2];
//     t[3] = (1 << (D-1)) - a->coeffs[8*i+3];
//     t[4] = (1 << (D-1)) - a->coeffs[8*i+4];
//     t[5] = (1 << (D-1)) - a->coeffs[8*i+5];
//     t[6] = (1 << (D-1)) - a->coeffs[8*i+6];
//     t[7] = (1 << (D-1)) - a->coeffs[8*i+7];

//     r[13*i+ 0]  =  t[0];
//     r[13*i+ 1]  =  t[0] >>  8;
//     r[13*i+ 1] |=  t[1] <<  5;
//     r[13*i+ 2]  =  t[1] >>  3;
//     r[13*i+ 3]  =  t[1] >> 11;
//     r[13*i+ 3] |=  t[2] <<  2;
//     r[13*i+ 4]  =  t[2] >>  6;
//     r[13*i+ 4] |=  t[3] <<  7;
//     r[13*i+ 5]  =  t[3] >>  1;
//     r[13*i+ 6]  =  t[3] >>  9;
//     r[13*i+ 6] |=  t[4] <<  4;
//     r[13*i+ 7]  =  t[4] >>  4;
//     r[13*i+ 8]  =  t[4] >> 12;
//     r[13*i+ 8] |=  t[5] <<  1;
//     r[13*i+ 9]  =  t[5] >>  7;
//     r[13*i+ 9] |=  t[6] <<  6;
//     r[13*i+10]  =  t[6] >>  2;
//     r[13*i+11]  =  t[6] >> 10;
//     r[13*i+11] |=  t[7] <<  3;
//     r[13*i+12]  =  t[7] >>  5;
//   }

//   DBENCH_STOP(*tpack);
// }

/*************************************************
* Name:        polyt0_unpack
*
* Description: Unpack polynomial t0 with coefficients in ]-2^{D-1}, 2^{D-1}].
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *a: byte array with bit-packed polynomial
**************************************************/
// void polyt0_unpack(poly *r, const uint8_t *a) {
//   unsigned int i;
//   DBENCH_START();

//   for(i = 0; i < N/8; ++i) {
//     r->coeffs[8*i+0]  = a[13*i+0];
//     r->coeffs[8*i+0] |= (uint32_t)a[13*i+1] << 8;
//     r->coeffs[8*i+0] &= 0x1FFF;

//     r->coeffs[8*i+1]  = a[13*i+1] >> 5;
//     r->coeffs[8*i+1] |= (uint32_t)a[13*i+2] << 3;
//     r->coeffs[8*i+1] |= (uint32_t)a[13*i+3] << 11;
//     r->coeffs[8*i+1] &= 0x1FFF;

//     r->coeffs[8*i+2]  = a[13*i+3] >> 2;
//     r->coeffs[8*i+2] |= (uint32_t)a[13*i+4] << 6;
//     r->coeffs[8*i+2] &= 0x1FFF;

//     r->coeffs[8*i+3]  = a[13*i+4] >> 7;
//     r->coeffs[8*i+3] |= (uint32_t)a[13*i+5] << 1;
//     r->coeffs[8*i+3] |= (uint32_t)a[13*i+6] << 9;
//     r->coeffs[8*i+3] &= 0x1FFF;

//     r->coeffs[8*i+4]  = a[13*i+6] >> 4;
//     r->coeffs[8*i+4] |= (uint32_t)a[13*i+7] << 4;
//     r->coeffs[8*i+4] |= (uint32_t)a[13*i+8] << 12;
//     r->coeffs[8*i+4] &= 0x1FFF;

//     r->coeffs[8*i+5]  = a[13*i+8] >> 1;
//     r->coeffs[8*i+5] |= (uint32_t)a[13*i+9] << 7;
//     r->coeffs[8*i+5] &= 0x1FFF;

//     r->coeffs[8*i+6]  = a[13*i+9] >> 6;
//     r->coeffs[8*i+6] |= (uint32_t)a[13*i+10] << 2;
//     r->coeffs[8*i+6] |= (uint32_t)a[13*i+11] << 10;
//     r->coeffs[8*i+6] &= 0x1FFF;

//     r->coeffs[8*i+7]  = a[13*i+11] >> 3;
//     r->coeffs[8*i+7] |= (uint32_t)a[13*i+12] << 5;
//     r->coeffs[8*i+7] &= 0x1FFF;

//     r->coeffs[8*i+0] = (1 << (D-1)) - r->coeffs[8*i+0];
//     r->coeffs[8*i+1] = (1 << (D-1)) - r->coeffs[8*i+1];
//     r->coeffs[8*i+2] = (1 << (D-1)) - r->coeffs[8*i+2];
//     r->coeffs[8*i+3] = (1 << (D-1)) - r->coeffs[8*i+3];
//     r->coeffs[8*i+4] = (1 << (D-1)) - r->coeffs[8*i+4];
//     r->coeffs[8*i+5] = (1 << (D-1)) - r->coeffs[8*i+5];
//     r->coeffs[8*i+6] = (1 << (D-1)) - r->coeffs[8*i+6];
//     r->coeffs[8*i+7] = (1 << (D-1)) - r->coeffs[8*i+7];
//   }

//   DBENCH_STOP(*tpack);
// }

/*************************************************
* Name:        polyz_pack
*
* Description: Bit-pack polynomial with coefficients
*              in [-(GAMMA1 - 1), GAMMA1].
*
* Arguments:   - uint8_t *r: pointer to output byte array with at least
*                            POLYZ_PACKEDBYTES bytes
*              - const poly *a: pointer to input polynomial
**************************************************/
// void polyz_pack(uint8_t *r, const poly *a) {
//   unsigned int i;
//   uint32_t t[4];
//   DBENCH_START();

// #if GAMMA1 == (1 << 17)
//   for(i = 0; i < N/4; ++i) {
//     t[0] = GAMMA1 - a->coeffs[4*i+0];
//     t[1] = GAMMA1 - a->coeffs[4*i+1];
//     t[2] = GAMMA1 - a->coeffs[4*i+2];
//     t[3] = GAMMA1 - a->coeffs[4*i+3];

//     r[9*i+0]  = t[0];
//     r[9*i+1]  = t[0] >> 8;
//     r[9*i+2]  = t[0] >> 16;
//     r[9*i+2] |= t[1] << 2;
//     r[9*i+3]  = t[1] >> 6;
//     r[9*i+4]  = t[1] >> 14;
//     r[9*i+4] |= t[2] << 4;
//     r[9*i+5]  = t[2] >> 4;
//     r[9*i+6]  = t[2] >> 12;
//     r[9*i+6] |= t[3] << 6;
//     r[9*i+7]  = t[3] >> 2;
//     r[9*i+8]  = t[3] >> 10;
//   }
// #elif GAMMA1 == (1 << 19)
//   for(i = 0; i < N/2; ++i) {
//     t[0] = GAMMA1 - a->coeffs[2*i+0];
//     t[1] = GAMMA1 - a->coeffs[2*i+1];

//     r[5*i+0]  = t[0];
//     r[5*i+1]  = t[0] >> 8;
//     r[5*i+2]  = t[0] >> 16;
//     r[5*i+2] |= t[1] << 4;
//     r[5*i+3]  = t[1] >> 4;
//     r[5*i+4]  = t[1] >> 12;
//   }
// #endif

//   DBENCH_STOP(*tpack);
// }

/*************************************************
* Name:        polyz_unpack
*
* Description: Unpack polynomial z with coefficients
*              in [-(GAMMA1 - 1), GAMMA1].
*
* Arguments:   - poly *r: pointer to output polynomial
*              - const uint8_t *a: byte array with bit-packed polynomial
**************************************************/
// void polyz_unpack(poly *r, const uint8_t *a) {
//   unsigned int i;
//   DBENCH_START();

// #if GAMMA1 == (1 << 17)
//   for(i = 0; i < N/4; ++i) {
//     r->coeffs[4*i+0]  = a[9*i+0];
//     r->coeffs[4*i+0] |= (uint32_t)a[9*i+1] << 8;
//     r->coeffs[4*i+0] |= (uint32_t)a[9*i+2] << 16;
//     r->coeffs[4*i+0] &= 0x3FFFF;

//     r->coeffs[4*i+1]  = a[9*i+2] >> 2;
//     r->coeffs[4*i+1] |= (uint32_t)a[9*i+3] << 6;
//     r->coeffs[4*i+1] |= (uint32_t)a[9*i+4] << 14;
//     r->coeffs[4*i+1] &= 0x3FFFF;

//     r->coeffs[4*i+2]  = a[9*i+4] >> 4;
//     r->coeffs[4*i+2] |= (uint32_t)a[9*i+5] << 4;
//     r->coeffs[4*i+2] |= (uint32_t)a[9*i+6] << 12;
//     r->coeffs[4*i+2] &= 0x3FFFF;

//     r->coeffs[4*i+3]  = a[9*i+6] >> 6;
//     r->coeffs[4*i+3] |= (uint32_t)a[9*i+7] << 2;
//     r->coeffs[4*i+3] |= (uint32_t)a[9*i+8] << 10;
//     r->coeffs[4*i+3] &= 0x3FFFF;

//     r->coeffs[4*i+0] = GAMMA1 - r->coeffs[4*i+0];
//     r->coeffs[4*i+1] = GAMMA1 - r->coeffs[4*i+1];
//     r->coeffs[4*i+2] = GAMMA1 - r->coeffs[4*i+2];
//     r->coeffs[4*i+3] = GAMMA1 - r->coeffs[4*i+3];
//   }
// #elif GAMMA1 == (1 << 19)
//   for(i = 0; i < N/2; ++i) {
//     r->coeffs[2*i+0]  = a[5*i+0];
//     r->coeffs[2*i+0] |= (uint32_t)a[5*i+1] << 8;
//     r->coeffs[2*i+0] |= (uint32_t)a[5*i+2] << 16;
//     r->coeffs[2*i+0] &= 0xFFFFF;

//     r->coeffs[2*i+1]  = a[5*i+2] >> 4;
//     r->coeffs[2*i+1] |= (uint32_t)a[5*i+3] << 4;
//     r->coeffs[2*i+1] |= (uint32_t)a[5*i+4] << 12;
//     /* r->coeffs[2*i+1] &= 0xFFFFF; */ /* No effect, since we're anyway at 20 bits */

//     r->coeffs[2*i+0] = GAMMA1 - r->coeffs[2*i+0];
//     r->coeffs[2*i+1] = GAMMA1 - r->coeffs[2*i+1];
//   }
// #endif

//   DBENCH_STOP(*tpack);
// }

/*************************************************
* Name:        polyw1_pack
*
* Description: Bit-pack polynomial w1 with coefficients in [0,15] or [0,43].
*              Input coefficients are assumed to be standard representatives.
*
* Arguments:   - uint8_t *r: pointer to output byte array with at least
*                            POLYW1_PACKEDBYTES bytes
*              - const poly *a: pointer to input polynomial
**************************************************/
// void polyw1_pack(uint8_t *r, const poly *a) {
//   unsigned int i;
//   DBENCH_START();

// #if GAMMA2 == (Q-1)/88
//   for(i = 0; i < N/4; ++i) {
//     r[3*i+0]  = a->coeffs[4*i+0];
//     r[3*i+0] |= a->coeffs[4*i+1] << 6;
//     r[3*i+1]  = a->coeffs[4*i+1] >> 2;
//     r[3*i+1] |= a->coeffs[4*i+2] << 4;
//     r[3*i+2]  = a->coeffs[4*i+2] >> 4;
//     r[3*i+2] |= a->coeffs[4*i+3] << 2;
//   }
// #elif GAMMA2 == (Q-1)/32
//   for(i = 0; i < N/2; ++i)
//     r[i] = a->coeffs[2*i+0] | (a->coeffs[2*i+1] << 4);
// #endif

//   DBENCH_STOP(*tpack);
// }
