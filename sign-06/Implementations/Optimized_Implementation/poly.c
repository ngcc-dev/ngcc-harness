#include <stdint.h>
#include "params.h"
#include "poly.h"
#include "ntt.h"
#include "ntt_avx.h"
#include "consts.h"
#include "reduce.h"
#include "rounding.h"
#include "symmetric.h"
#include "fips202x4.h"
#include "symmetric.h"
#include <immintrin.h>
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

#define POLY_UNIFORM_GAMMA1_NBLOCKS ((POLYZ_PACKEDBYTES + 135) / 136)

// #if Q == 2081281
// extern void ntt_avx(__m256i a[N/8], const qdata_t *qd);
// extern void invntt_avx(__m256i a[N/8], const qdata_t *qd);
// extern void pointwise_avx(__m256i c[N/8], const __m256i a[N/8], const __m256i b[N/8], const qdata_t *qd);
// extern void nttunpack_avx(__m256i a[N/8]);
// #endif

static unsigned int rej_eta(int32_t *a, unsigned int len, const uint8_t *buf, unsigned int buflen, int8_t eta) {
    unsigned int ctr = 0, pos = 0;
    uint32_t t;

    if (eta == 1) {
        while(ctr < len && pos + 1 < buflen) {
            t = buf[pos] | ((uint32_t)buf[pos+1] << 8);
            pos += 2;
            for(int i = 0; i < 8 && ctr < len; ++i) {
                uint32_t c = (t >> (2 * i)) & 0x03;
                if(c < 3) {
                    a[ctr++] = (int32_t)c - 1;
                }
            }
        }
    } else if (eta == 2) {
        while(ctr < len && pos + 1 < buflen) {
            t = buf[pos] | ((uint32_t)buf[pos+1] << 8);
            pos += 2;
            for(int i = 0; i < 5 && ctr < len; ++i) {
                uint32_t c = (t >> (3 * i)) & 0x07;
                if(c < 5) {
                    a[ctr++] = (int32_t)c - 2;
                }
            }
        }
    }
    return ctr;
}

void poly_uniform_eta_4x(poly *a0, poly *a1, poly *a2, poly *a3,
                         const uint8_t seed[CRHBYTES],
                         uint16_t nonce0, uint16_t nonce1, uint16_t nonce2, uint16_t nonce3,
                         int8_t eta) 
{
    unsigned int ctr0 = 0, ctr1 = 0, ctr2 = 0, ctr3 = 0;
    uint8_t buf0[SHAKE256_RATE], buf1[SHAKE256_RATE], buf2[SHAKE256_RATE], buf3[SHAKE256_RATE];
    uint8_t extseed0[CRHBYTES + 2], extseed1[CRHBYTES + 2];
    uint8_t extseed2[CRHBYTES + 2], extseed3[CRHBYTES + 2];
    keccakx4_state state;

    for (int i = 0; i < CRHBYTES; ++i) {
        extseed0[i] = extseed1[i] = extseed2[i] = extseed3[i] = seed[i];
    }

    extseed0[CRHBYTES] = (uint8_t)nonce0; extseed0[CRHBYTES+1] = (uint8_t)(nonce0 >> 8);
    extseed1[CRHBYTES] = (uint8_t)nonce1; extseed1[CRHBYTES+1] = (uint8_t)(nonce1 >> 8);
    extseed2[CRHBYTES] = (uint8_t)nonce2; extseed2[CRHBYTES+1] = (uint8_t)(nonce2 >> 8);
    extseed3[CRHBYTES] = (uint8_t)nonce3; extseed3[CRHBYTES+1] = (uint8_t)(nonce3 >> 8);

    shake256x4_absorb_once(&state, extseed0, extseed1, extseed2, extseed3, CRHBYTES + 2);

    // 只要有任何一个多项式还没达到 N 个系数，就继续挤出随机字节
    // 这里传入 N - ctr，确保底层采样精准截断，维持迭代稳定性
    while (ctr0 < N || ctr1 < N || ctr2 < N || ctr3 < N) {
        shake256x4_squeezeblocks(buf0, buf1, buf2, buf3, 1, &state);

        if (ctr0 < N) ctr0 += rej_eta(a0->coeffs + ctr0, N - ctr0, buf0, SHAKE256_RATE, eta);
        if (ctr1 < N) ctr1 += rej_eta(a1->coeffs + ctr1, N - ctr1, buf1, SHAKE256_RATE, eta);
        if (ctr2 < N) ctr2 += rej_eta(a2->coeffs + ctr2, N - ctr2, buf2, SHAKE256_RATE, eta);
        if (ctr3 < N) ctr3 += rej_eta(a3->coeffs + ctr3, N - ctr3, buf3, SHAKE256_RATE, eta);
    }
}

void poly_uniform_gamma1_4x(poly *a0, poly *a1, poly *a2, poly *a3,
                            const uint8_t seed[CRHBYTES],
                            uint16_t nonce0, uint16_t nonce1, uint16_t nonce2, uint16_t nonce3) 
{
    unsigned int i;
    // CRHBYTES 通常为 64，加上 2 字节的 nonce = 66
    uint8_t extseed0[CRHBYTES + 2], extseed1[CRHBYTES + 2];
    uint8_t extseed2[CRHBYTES + 2], extseed3[CRHBYTES + 2];
    
    // 给 4 个多项式分配接收随机字节的缓冲区
    uint8_t buf0[POLY_UNIFORM_GAMMA1_NBLOCKS * SHAKE256_RATE];
    uint8_t buf1[POLY_UNIFORM_GAMMA1_NBLOCKS * SHAKE256_RATE];
    uint8_t buf2[POLY_UNIFORM_GAMMA1_NBLOCKS * SHAKE256_RATE];
    uint8_t buf3[POLY_UNIFORM_GAMMA1_NBLOCKS * SHAKE256_RATE];
    
    keccakx4_state state;

    // 拷贝 Seed
    for (i = 0; i < CRHBYTES; ++i) {
        extseed0[i] = extseed1[i] = extseed2[i] = extseed3[i] = seed[i];
    }

    // 拼接 Nonce (小端序)
    extseed0[CRHBYTES] = (uint8_t)nonce0; extseed0[CRHBYTES+1] = (uint8_t)(nonce0 >> 8);
    extseed1[CRHBYTES] = (uint8_t)nonce1; extseed1[CRHBYTES+1] = (uint8_t)(nonce1 >> 8);
    extseed2[CRHBYTES] = (uint8_t)nonce2; extseed2[CRHBYTES+1] = (uint8_t)(nonce2 >> 8);
    extseed3[CRHBYTES] = (uint8_t)nonce3; extseed3[CRHBYTES+1] = (uint8_t)(nonce3 >> 8);

    // 4 路 SHAKE256 吸收 (注意：这里使用的是 shake256x4)
    shake256x4_absorb_once(&state, extseed0, extseed1, extseed2, extseed3, CRHBYTES + 2);

    // 4 路并行挤出数据
    shake256x4_squeezeblocks(buf0, buf1, buf2, buf3, POLY_UNIFORM_GAMMA1_NBLOCKS, &state);

    // =========================================================
    // 【关键】：将字节流 Unpack 解析为 [-gamma1, gamma1] 的系数
    // 这里可以直接复用你现有的单路 poly_uniform_gamma1 函数内部的解析逻辑。
    // 在 COMPASS_SIG 标准实现中，y 的打包格式与 z 是相同的，
    // 因此这里可以直接使用解包 z 的函数（或你为其专门编写的 gamma1 解析宏）
    // =========================================================
    
    // 假设你参考了 COMPASS_SIG 使用了 polyz_unpack 的解包逻辑来处理 gamma1
    // (具体取决于你在 ref/poly.c 中单路 poly_uniform_gamma1 是怎么写的)
    
    // 示例伪代码（请替换为你的实际 unpack 调用）：
    polyz_unpack(a0, buf0);
    polyz_unpack(a1, buf1);
    polyz_unpack(a2, buf2);
    polyz_unpack(a3, buf3);
}

unsigned int rej_uniform_avx(int32_t *a, unsigned int len, const uint8_t *buf, unsigned int buflen) {
    unsigned int ctr, pos;
    uint32_t val;

    ctr = pos = 0;
    while (ctr < len && pos + 3 <= buflen) {
        // 读取 24 个比特（3 个字节）
        val  = buf[pos++];
        val |= (uint32_t)buf[pos++] << 8;
        val |= (uint32_t)buf[pos++] << 16;

        // 【关键区分点】：根据你当前的参数设置调整掩码
        // 针对 q = 2081281，使用 21 比特掩码：val &= 0x1FFFFF;
        // 针对 q = 8380417，使用 23 比特掩码：val &= 0x7FFFFF;
#if Q == 2081281
  val &= 0x1FFFFF; // 保留 21 位 (最大值 2,097,151)
                 // 接受率: 2081281 / 2097152 = 99.2% !!
#elif Q == 8380417
  val &= 0x7FFFFF; // 保留 23 位 (最大值 8,388,607)
                 // 接受率: 99.9%
#else
  #error "Missing mask for current Q"
#endif
        

        if (val < Q) {
            a[ctr++] = val;
        }
    }
    return ctr; // 返回本次成功采样的系数个数
}

void poly_uniform_4x(poly *a0, poly *a1, poly *a2, poly *a3,
                     const uint8_t seed[SEEDBYTES],
                     uint16_t nonce0, uint16_t nonce1, uint16_t nonce2, uint16_t nonce3) 
{
    unsigned int i, ctr0 = 0, ctr1 = 0, ctr2 = 0, ctr3 = 0;
    // SHAKE128_RATE 通常为 168 字节
    uint8_t buf0[SHAKE128_RATE], buf1[SHAKE128_RATE], buf2[SHAKE128_RATE], buf3[SHAKE128_RATE];
    uint8_t extseed0[SEEDBYTES + 2], extseed1[SEEDBYTES + 2], extseed2[SEEDBYTES + 2], extseed3[SEEDBYTES + 2];
    keccakx4_state state;

    // 将 32 字节的 seed 复制到扩展缓冲区
    for (i = 0; i < SEEDBYTES; ++i) {
        extseed0[i] = extseed1[i] = extseed2[i] = extseed3[i] = seed[i];
    }

    // 拼接 2 字节的 nonce（小端序存储）
    extseed0[SEEDBYTES] = (uint8_t)nonce0; extseed0[SEEDBYTES + 1] = (uint8_t)(nonce0 >> 8);
    extseed1[SEEDBYTES] = (uint8_t)nonce1; extseed1[SEEDBYTES + 1] = (uint8_t)(nonce1 >> 8);
    extseed2[SEEDBYTES] = (uint8_t)nonce2; extseed2[SEEDBYTES + 1] = (uint8_t)(nonce2 >> 8);
    extseed3[SEEDBYTES] = (uint8_t)nonce3; extseed3[SEEDBYTES + 1] = (uint8_t)(nonce3 >> 8);

    // 4 路并行吸收
    shake128x4_absorb_once(&state, extseed0, extseed1, extseed2, extseed3, SEEDBYTES + 2);

    // 并行挤出随机字节，直至所有 4 个多项式的 N 个系数都被成功采样
    while (ctr0 < N || ctr1 < N || ctr2 < N || ctr3 < N) {
        shake128x4_squeezeblocks(buf0, buf1, buf2, buf3, 1, &state);

        if (ctr0 < N) ctr0 += rej_uniform_avx(a0->coeffs + ctr0, N - ctr0, buf0, SHAKE128_RATE);
        if (ctr1 < N) ctr1 += rej_uniform_avx(a1->coeffs + ctr1, N - ctr1, buf1, SHAKE128_RATE);
        if (ctr2 < N) ctr2 += rej_uniform_avx(a2->coeffs + ctr2, N - ctr2, buf2, SHAKE128_RATE);
        if (ctr3 < N) ctr3 += rej_uniform_avx(a3->coeffs + ctr3, N - ctr3, buf3, SHAKE128_RATE);
    }
}

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

static inline __m256i reduce32_avx(__m256i a) {
#if Q == 8380417
    __m256i v_q = _mm256_set1_epi32(8380417);
    __m256i v_add = _mm256_set1_epi32(1 << 22);
    
    // t = (a + (1 << 22)) >> 23;
    __m256i t = _mm256_add_epi32(a, v_add);
    t = _mm256_srai_epi32(t, 23); // 算术右移 23 位
    
    // t = a - t * Q;
    __m256i tQ = _mm256_mullo_epi32(t, v_q);
    return _mm256_sub_epi32(a, tQ);
    
#elif Q == 2081281
    __m256i v_q = _mm256_set1_epi32(2081281);
    __m256i v_mu = _mm256_set1_epi64x(BARRETT_MU);
    __m256i v_round = _mm256_set1_epi64x(1LL << 31);

    // --- 处理偶数索引位置 (0, 2, 4, 6) 的 32x32 -> 64 乘法 ---
    // _mm256_mul_epi32 自动取每个 64-bit 块的低 32-bit 进行乘法，结果为 64-bit
    __m256i m0 = _mm256_mul_epi32(a, v_mu);
    m0 = _mm256_add_epi64(m0, v_round); // 加上 1LL << 31
    m0 = _mm256_srli_epi64(m0, 32);     // 右移 32 位，此时高 32 位结果落在了偶数通道

    // --- 处理奇数索引位置 (1, 3, 5, 7) 的 32x32 -> 64 乘法 ---
    __m256i a_odd = _mm256_srli_epi64(a, 32); // 将奇数通道的数据移到偶数位置
    __m256i m1 = _mm256_mul_epi32(a_odd, v_mu);
    m1 = _mm256_add_epi64(m1, v_round);
    m1 = _mm256_srli_epi64(m1, 32); 
    m1 = _mm256_slli_epi64(m1, 32); // 乘完后左移，归位到奇数通道

    // --- 缝合结果 ---
    // 按位掩码 0xAA (10101010) 将奇偶结果拼在一起形成完整的 q_val
    __m256i q_val = _mm256_blend_epi32(m0, m1, 0xAA); 

    // t = a - q_val * Q;
    __m256i qQ = _mm256_mullo_epi32(q_val, v_q);
    return _mm256_sub_epi32(a, qQ);
#endif
}
static inline __m256i caddq_avx(__m256i a) {
    __m256i v_q = _mm256_set1_epi32(Q);
    
    // a >> 31：算术右移，如果元素为负，全 1 (即 0xFFFFFFFF)；如果元素为正，全 0
    __m256i mask = _mm256_srai_epi32(a, 31);
    
    // mask & Q：如果是负数得到 Q，如果是正数得到 0
    __m256i add_val = _mm256_and_si256(mask, v_q);
    
    // a += (mask & Q)
    return _mm256_add_epi32(a, add_val);
}
void poly_reduce(poly *a) {
    unsigned int i;
    DBENCH_START();

    // 每次并行处理 8 个 32-bit 系数
    for(i = 0; i < N; i += 8) {
        __m256i f = _mm256_loadu_si256((__m256i *)&a->coeffs[i]);
        f = reduce32_avx(f);
        _mm256_storeu_si256((__m256i *)&a->coeffs[i], f);
    }

    DBENCH_STOP(*tred);
}

void poly_caddq(poly *a) {
    unsigned int i;
    DBENCH_START();

    // 每次并行处理 8 个 32-bit 系数
    for(i = 0; i < N; i += 8) {
        __m256i f = _mm256_loadu_si256((__m256i *)&a->coeffs[i]);
        f = caddq_avx(f);
        _mm256_storeu_si256((__m256i *)&a->coeffs[i], f);
    }

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
/*************************************************
* Name:        poly_add
*
* Description: Add polynomials. No modular reduction is performed.
*
* Arguments:   - poly *c: pointer to output polynomial
* - const poly *a: pointer to first input polynomial
* - const poly *b: pointer to second input polynomial
**************************************************/
void poly_add(poly *c, const poly *a, const poly *b)  {
  unsigned int i;
  __m256i f0, f1;
  DBENCH_START();

  // 每次处理 8 个 32-bit 系数
  for(i = 0; i < N; i += 8) {
    // 从内存加载数据到 AVX2 寄存器
    f0 = _mm256_loadu_si256((__m256i *)&a->coeffs[i]);
    f1 = _mm256_loadu_si256((__m256i *)&b->coeffs[i]);
    
    // 执行 8 路 32-bit 并行加法: f0 = f0 + f1
    f0 = _mm256_add_epi32(f0, f1);
    
    // 将结果存回内存
    _mm256_storeu_si256((__m256i *)&c->coeffs[i], f0);
  }

  DBENCH_STOP(*tadd);
}

/*************************************************
* Name:        poly_sub
*
* Description: Subtract polynomials. No modular reduction is
* performed.
*
* Arguments:   - poly *c: pointer to output polynomial
* - const poly *a: pointer to first input polynomial
* - const poly *b: pointer to second input polynomial to be
* subtracted from first input polynomial
**************************************************/
void poly_sub(poly *c, const poly *a, const poly *b) {
  unsigned int i;
  __m256i f0, f1;
  DBENCH_START();

  // 每次处理 8 个 32-bit 系数
  for(i = 0; i < N; i += 8) {
    f0 = _mm256_loadu_si256((__m256i *)&a->coeffs[i]);
    f1 = _mm256_loadu_si256((__m256i *)&b->coeffs[i]);
    
    // 执行 8 路 32-bit 并行减法: f0 = f0 - f1
    f0 = _mm256_sub_epi32(f0, f1);
    
    _mm256_storeu_si256((__m256i *)&c->coeffs[i], f0);
  }

  DBENCH_STOP(*tadd);
}

/*************************************************
* Name:        poly_shiftl
*
* Description: Multiply polynomial by 2^D without modular reduction. Assumes
* input coefficients to be less than 2^{31-D} in absolute value.
*
* Arguments:   - poly *a: pointer to input/output polynomial
**************************************************/
void poly_shiftl(poly *a) {
  unsigned int i;
  __m256i f0;
  DBENCH_START();

  // 每次处理 8 个 32-bit 系数
  for(i = 0; i < N; i += 8) {
    f0 = _mm256_loadu_si256((__m256i *)&a->coeffs[i]);
    
    // 执行 8 路 32-bit 并行逻辑左移: f0 = f0 << D
    f0 = _mm256_slli_epi32(f0, D);
    
    _mm256_storeu_si256((__m256i *)&a->coeffs[i], f0);
  }

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
// void poly_nttunpack(poly *a) {
// #if Q == 2081281
//   nttunpack_avx(a->coeffs);
// #else
//   // 标准 C 语言版本不需要 unpack 操作，保持为空即可
//   (void)a;
// #endif
// }

void poly_nttunpack(poly *a) {

  // 标准 C 语言版本不需要 unpack 操作，保持为空即可
  (void)a;

}

/*************************************************
* Name:        poly_ntt
*
* Description: Inplace forward NTT.
**************************************************/
// void poly_ntt(poly *a) {
//   DBENCH_START();

// #if Q == 2081281
//   ntt_avx(a->coeffs, &qdata);
// #else
//   ntt(a->coeffs); // 回退到 ref 版本的 C 函数
// #endif

//   DBENCH_STOP(*tmul);
// }

void poly_ntt(poly *a) {

  ntt(a->coeffs); // 回退到 ref 版本的 C 函数


  DBENCH_STOP(*tmul);
}
/*************************************************
* Name:        poly_invntt_tomont
*
* Description: Inplace inverse NTT and multiplication by 2^{32}.
**************************************************/
// void poly_invntt_tomont(poly *a) {
//   DBENCH_START();

// #if Q == 2081281
//   invntt_avx(a->coeffs, &qdata);
// #else
//   invntt_tomont(a->coeffs); // 回退到 ref 版本的 C 函数
// #endif

//   DBENCH_STOP(*tmul);
// }

void poly_invntt_tomont(poly *a) {
  DBENCH_START();


  invntt_tomont(a->coeffs); // 回退到 ref 版本的 C 函数


  DBENCH_STOP(*tmul);
}

/*************************************************
* Name:        poly_pointwise_montgomery
*
* Description: Pointwise multiplication of polynomials in NTT domain
**************************************************/
static inline __m256i montgomery_mul_avx(__m256i a, __m256i b) {
    // 预加载常量，注意要强转为 64 位以适配 _mm256_set1_epi64x
    __m256i vQ = _mm256_set1_epi64x((int64_t)Q);
    __m256i vQINV = _mm256_set1_epi64x((int64_t)QINV);

    // =======================================================
    // 1. 处理偶数通道 (位置 0, 2, 4, 6) 的 32x32 -> 64 乘法
    // =======================================================
    __m256i prod0 = _mm256_mul_epi32(a, b);
    
    // t = (int32_t)a * QINV
    // _mm256_mul_epi32 自动取 prod0 的低 32 位并进行有符号乘法，
    // 这完美等价于 C 语言中的 (int64_t)(int32_t)a * QINV
    __m256i t0 = _mm256_mul_epi32(prod0, vQINV);
    
    // t * Q
    __m256i t0_Q = _mm256_mul_epi32(t0, vQ);
    
    // a - t * Q
    __m256i res0 = _mm256_sub_epi64(prod0, t0_Q);
    
    // 逻辑右移 32 位，将高 32 位结果移到每个 64-bit 块的低 32 位中
    res0 = _mm256_srli_epi64(res0, 32); 

    // =======================================================
    // 2. 处理奇数通道 (位置 1, 3, 5, 7) 的 32x32 -> 64 乘法
    // =======================================================
    // 将奇数通道的数据逻辑右移到偶数位置，以喂给 mul_epi32
    __m256i a1 = _mm256_srli_epi64(a, 32);
    __m256i b1 = _mm256_srli_epi64(b, 32);
    
    __m256i prod1 = _mm256_mul_epi32(a1, b1);
    __m256i t1 = _mm256_mul_epi32(prod1, vQINV);
    __m256i t1_Q = _mm256_mul_epi32(t1, vQ);
    __m256i res1 = _mm256_sub_epi64(prod1, t1_Q);
    
    // 注意：奇数通道的结果此时正稳稳地停留在每个 64-bit 块的高 32 位中，
    // 我们不需要移动它，直接保留原位即可。

    // =======================================================
    // 3. 缝合结果 (Blend)
    // =======================================================
    // 0xAA 掩码即二进制 10101010，表示：
    // 第 0, 2, 4, 6 个 32-bit 位置从 res0 取 (正好是我们计算的偶数结果)
    // 第 1, 3, 5, 7 个 32-bit 位置从 res1 取 (正好是我们计算的奇数结果)
    return _mm256_blend_epi32(res0, res1, 0xAA);
}

/*************************************************
* Name:        poly_pointwise_montgomery
**************************************************/
// void poly_pointwise_montgomery(poly *c, const poly *a, const poly *b) {
//   DBENCH_START();

// #if Q == 2081281
//   pointwise_avx(c->coeffs, a->coeffs, b->coeffs, &qdata);
// #else
//   unsigned int i;
  
//   // 每次循环处理 8 个系数
//   for(i = 0; i < N; i += 8) {
//     __m256i va = _mm256_loadu_si256((__m256i *)&a->coeffs[i]);
//     __m256i vb = _mm256_loadu_si256((__m256i *)&b->coeffs[i]);
    
//     __m256i vc = montgomery_mul_avx(va, vb);
    
//     _mm256_storeu_si256((__m256i *)&c->coeffs[i], vc);
//   }
// #endif

//   DBENCH_STOP(*tmul);
// }

void poly_pointwise_montgomery(poly *c, const poly *a, const poly *b) {
  DBENCH_START();

  unsigned int i;
  
  // 每次循环处理 8 个系数
  for(i = 0; i < N; i += 8) {
    __m256i va = _mm256_loadu_si256((__m256i *)&a->coeffs[i]);
    __m256i vb = _mm256_loadu_si256((__m256i *)&b->coeffs[i]);
    
    __m256i vc = montgomery_mul_avx(va, vb);
    
    _mm256_storeu_si256((__m256i *)&c->coeffs[i], vc);
  }


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
static inline __m256i power2round_avx(__m256i *a0, __m256i a) {
    // 预计算常数偏移量: (1 << (D-1)) - 1
    // 现代编译器极其聪明，会在编译期计算出这个值，并将其自动提升到循环外部，
    // 因此在实际执行时，这句代码消耗的 CPU 时钟周期严格为 0。
    __m256i v_offset = _mm256_set1_epi32((1 << (D - 1)) - 1);
    
    // a1 = a + (1 << (D-1)) - 1
    __m256i a1 = _mm256_add_epi32(a, v_offset);
    
    // a1 = a1 >> D (注意：这里必须使用算术右移 srai 来保持符号位)
    a1 = _mm256_srai_epi32(a1, D);
    
    // a1_shifted = a1 << D (左移补充 0，slli 即可)
    __m256i a1_shifted = _mm256_slli_epi32(a1, D);
    
    // *a0 = a - (a1 << D)
    *a0 = _mm256_sub_epi32(a, a1_shifted);
    
    return a1;
}

/*************************************************
* Name:        poly_power2round
* Description: For all coefficients of the input polynomial,
* compute high and low bits.
* Arguments:   - poly *a1: pointer to output polynomial with high bits
* - poly *a0: pointer to output polynomial with low bits
* - const poly *a: pointer to input polynomial
**************************************************/
void poly_power2round(poly *a1, poly *a0, const poly *a) {
    unsigned int i;
    DBENCH_START();

    // 每次处理 8 个 32-bit 系数
    for(i = 0; i < N; i += 8) {
        // 加载输入
        __m256i va = _mm256_loadu_si256((__m256i *)&a->coeffs[i]);
        __m256i va0;
        
        // 计算拆分
        __m256i va1 = power2round_avx(&va0, va);
        
        // 分别存储高位和低位
        _mm256_storeu_si256((__m256i *)&a1->coeffs[i], va1);
        _mm256_storeu_si256((__m256i *)&a0->coeffs[i], va0);
    }

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
#undef POLY_UNIFORM_GAMMA1_NBLOCKS
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
