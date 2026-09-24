#ifndef PARAMS_H
#define PARAMS_H

#include "config.h"

#define SEEDBYTES 32
#define CRHBYTES 64
#define TRBYTES 64
#define RNDBYTES 32
// #define N 256
// #define Q 8380417
// #define D 13
// #define ROOT_OF_UNITY 1753

#if COMPASS_SIG_MODE == 128
#define Q 2081281
#define N 256
#define K 3
#define L 4
#define ETA_S 1
#define ETA_E 1
#define D 4
#define GAMMA1 (1 << 15)
#define GAMMA2 ((Q-1)/16)
#define TAU 30
#define L2_BOUND_B 560
#define ROOT_OF_UNITY 143389  // 预先为您计算好的 NTT 域单位根(仅供参考)
#define CTILDEBYTES 32

// 打包位宽定义 (推导值)
#define Z_BITS      16        // [-2^15, 2^15] 需要 16 位
#define T1_BITS     17        // Q 约 21 位, 去除低 4 位 d, 剩下 17 位
#define T0_BITS     4         // 对应参数 d
#define ETA_S_BITS  2         // [-1, 1] 通常以 3 位存储(或根据打包算法取 2 位)
#define ETA_E_BITS  2

#define POLYW1_BITS 4
#define POLYW1_PACKEDBYTES 128

#elif COMPASS_SIG_MODE == 256
#define Q 2081281
#define N 256
#define K 7
#define L 7
#define ETA_S 1
#define ETA_E 1
#define D 5
#define GAMMA1 (1 << 17)
#define GAMMA2 ((Q-1)/4)
#define TAU 60
#define L2_BOUND_B 1120
#define ROOT_OF_UNITY 143389
#define CTILDEBYTES 48        // 根据安全性级别推算的哈希长度

// 打包位宽定义 (推导值)
#define Z_BITS      18        // [-2^17, 2^17] 需要 18 位
#define T1_BITS     16        // Q 约 21 位, 去除低 5 位 d, 剩下 16 位
#define T0_BITS     5         // 对应参数 d
#define ETA_S_BITS  2
#define ETA_E_BITS  2

#define POLYW1_BITS 2
#define POLYW1_PACKEDBYTES 64

#elif COMPASS_SIG_MODE == 384
#define Q 8380417
#define N 512                 // 注意：多项式长度为 512
#define K 5
#define L 6
#define ETA_S 1
#define ETA_E 1
#define D 5
#define GAMMA1 (1 << 18)
#define GAMMA2 ((Q-1)/8)
#define TAU 78
#define L2_BOUND_B 1760
#define ROOT_OF_UNITY 1718063 // Q=8380417, N=512时的单位根
#define CTILDEBYTES 48

// ==========================================
// 打包位宽定义 (严谨数学推导值)
// ==========================================
#define Z_BITS      19        // gamma1=2^18, z最大范围2*gamma1-1, 需 19 位
#define T1_BITS     18        // t1 最大值为 261888, 需要 18 位 (17位最大只有131071)
#define T0_BITS     5         // 严格对应参数 d
#define ETA_S_BITS  2         // eta=1, 范围{-1,0,1}, 需要 2 位
#define ETA_E_BITS  2         // eta=1, 范围{-1,0,1}, 需要 2 位

#define POLYW1_BITS 2         // gamma2=(Q-1)/8, w1只有{0,1,2,3}, 需要 2 位
#define POLYW1_PACKEDBYTES     ((N * POLYW1_BITS) / 8)  // 512 * 2 / 8 = 128

#elif COMPASS_SIG_MODE == 512
#define Q 8380417
#define N 512                 // 多项式长度为 512
#define K 7
#define L 7
#define ETA_S 1
#define ETA_E 1
#define D 6
#define GAMMA1 (1 << 19)
#define GAMMA2 ((Q-1)/4)
#define TAU 120
#define L2_BOUND_B 2240
#define ROOT_OF_UNITY 1718063
#define CTILDEBYTES 64

// 打包位宽定义 (推导值)
#define Z_BITS      20        // [-2^19, 2^19] 需要 20 位
#define T1_BITS     17        // Q 约 23 位, 去除低 6 位 d, 剩下 17 位
#define T0_BITS     6         // 对应参数 d
#define ETA_S_BITS  2
#define ETA_E_BITS  2

#define POLYW1_BITS 2
#define POLYW1_PACKEDBYTES 128
#endif

// 针对 z = y + c*s1 的边界
#define BETA_Z (TAU * ETA_S)

// 针对 w0' = w0 - c*s2 的边界
#define BETA_W (TAU * ETA_E)

#define POLYT1_PACKEDBYTES      ((N * T1_BITS) / 8)
#define POLYT0_PACKEDBYTES      ((N * T0_BITS) / 8)
#define POLYZ_PACKEDBYTES       ((N * Z_BITS) / 8)
#define POLY_ETA_S_PACKEDBYTES  ((N * ETA_S_BITS) / 8)
#define POLY_ETA_E_PACKEDBYTES  ((N * ETA_E_BITS) / 8)

// #define POLYT1_PACKEDBYTES  320
// #define POLYT0_PACKEDBYTES  416
// #define POLYVECH_PACKEDBYTES (OMEGA + K)

// #if GAMMA1 == (1 << 17)
// #define POLYZ_PACKEDBYTES   576
// #elif GAMMA1 == (1 << 19)
// #define POLYZ_PACKEDBYTES   640
// #endif

// #if GAMMA2 == (Q-1)/88
// #define POLYW1_PACKEDBYTES  192
// #elif GAMMA2 == (Q-1)/32
// #define POLYW1_PACKEDBYTES  128
// #endif

// #if ETA == 2
// #define POLYETA_PACKEDBYTES  96
// #elif ETA == 4
// #define POLYETA_PACKEDBYTES 128
// #endif

// #define CRYPTO_PUBLICKEYBYTES (SEEDBYTES + K*POLYT1_PACKEDBYTES)
// #define CRYPTO_SECRETKEYBYTES (2*SEEDBYTES + TRBYTES + L*POLYETA_PACKEDBYTES + K*POLYETA_PACKEDBYTES + K*POLYT0_PACKEDBYTES)
// #define CRYPTO_BYTES (CTILDEBYTES + L*POLYZ_PACKEDBYTES + POLYVECH_PACKEDBYTES)

// 最终公钥、私钥、签名尺寸
// =========================================================================

// 公钥：公钥种子 + t1 多项式向量
#define CRYPTO_PUBLICKEYBYTES (SEEDBYTES + K*POLYT1_PACKEDBYTES)

// 私钥：私钥种子 + 公钥种子 + 公钥哈希tr + s向量 + e向量 + t0向量
#define CRYPTO_SECRETKEYBYTES (2*SEEDBYTES \
                               + TRBYTES \
                               + L*POLY_ETA_S_PACKEDBYTES \
                               + K*POLY_ETA_E_PACKEDBYTES \
                               + K*POLYT0_PACKEDBYTES)

// 签名：哈希c + z多项式向量 (注意：移除了原算法中的提示向量 Hint/POLYVECH)
#define CRYPTO_BYTES (CTILDEBYTES + L*POLYZ_PACKEDBYTES)

#endif
