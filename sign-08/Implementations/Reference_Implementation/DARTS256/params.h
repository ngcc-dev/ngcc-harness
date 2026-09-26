#ifndef SIGN_PARAMS_H
#define SIGN_PARAMS_H

#include "config.h"

#define CRHBYTES 64 

/*-----------------128 bits security parameters ----------------------*/    
#if DARTS_MODE == 128 

#define SEEDBYTES 32

#define N 512
#define Q 130817
#define HALF_Q 65409 // Q+1 / 2
#define ROOT_OF_UNITY 667 // 256-th primitive root of unity module q
#define DQ (Q << 1)  // 2Q
#define K 1
#define L 2

#define P 0.15  // probability of 1 (or -1) 
#define TAU 30
#define B0 5858.47  // 小球的半径（小）
#define B 6049.79   // 超球采样半径（大）
#define B1 6046.76  // 中球的半径（中）
#define B11 7500.00 // 验签时的半径
#define GAMMA 34.93
#define D 10
#define SQNM 39.19 // sqrt(n*(k+l))

#define BASE_ENC_HB_Z1  220
#define BASE_ENC_H 80

#define LN 16384
#define LNHALF 8192
#define LNBITS 14

#define B0SQ ((uint64_t)(B0*B0))
#define B1SQ ((uint64_t)(B1*B1))
#define B11SQ ((uint64_t)(B11*B11))
#define BSQ ((uint64_t)(B*B))

#define Q_BITS 17

#define POLY_Q_PACKEDBYTES 1088 // 17 * n / 8 bytes
#define POLY_S_PACKEDBYTES 128  // 2 * n / 8 bytes
#define POLY_C_PACKEDBYTES 64   // size of c (n bits)

#define POLY_HIGHBITS_PACKEDBYTES ((N * D) / 8)  // size of highbits of poly
#define POLY_LOWBITS_PACKEDBYTES ((N * 57) / 64) // size of lowbits of poly (BAT)

#define ENCODE_MAXSIZE 471 // 对 HighBits(z1) 和 h 的rANS编码的最大尺寸

#define POLYVECK_HIGHBITS_PACKEDBYTES (K * POLY_HIGHBITS_PACKEDBYTES) // 多项式向量高位封装尺寸

#define CRYPTO_PUBLICKEYBYTES (SEEDBYTES + K * POLY_Q_PACKEDBYTES)  // pk = (seed, A_0)
#define CRYPTO_SECRETKEYBYTES (CRYPTO_PUBLICKEYBYTES + SEEDBYTES + (K+L) * POLY_S_PACKEDBYTES)  // sk = (s, K, pk)

#define CRYPTO_SIGNATUREBYTES (POLY_C_PACKEDBYTES + L * POLY_LOWBITS_PACKEDBYTES + 2 + ENCODE_MAXSIZE) // 签名尺寸 = c + LowBits(z1) + rANS_length +Encode(HighBits(z1)) + Encode(h)

#endif


/*-----------------256 bits security parameters ----------------------*/
#if DARTS_MODE == 256

#define SEEDBYTES 32

#define N 512
#define Q 130817
#define HALF_Q 65409 // Q+1 / 2
#define ROOT_OF_UNITY 667 // 256-th primitive root of unity module q
#define DQ (Q << 1)  // 2Q
#define K 2
#define L 3
#define P 0.3125  // probability of 1 (or -1) 
#define TAU 59
#define B0 18429.40  // 小球的半径（小）
#define B 18906.24   // 超球采样半径（大）
#define B1 18900.22  // 中球的半径（中）
#define B11 23000.00 // 验签时的半径
#define GAMMA 62.08
#define D 9
#define SQNM 50.60 // sqrt(n*(k+l))

#define BASE_ENC_HB_Z1 400 
#define BASE_ENC_H 300

#define LN 16384
#define LNHALF 8192
#define LNBITS 14

#define B0SQ ((uint64_t)(B0*B0))
#define B1SQ ((uint64_t)(B1*B1))
#define B11SQ ((uint64_t)(B11*B11))
#define BSQ ((uint64_t)(B*B))

#define Q_BITS 17

#define POLY_Q_PACKEDBYTES 1088 // 17 * n / 8 bytes
#define POLY_S_PACKEDBYTES 128  // 2 * n / 8 bytes
#define POLY_C_PACKEDBYTES 64   // size of c (n bits)

#define POLY_HIGHBITS_PACKEDBYTES ((N * D) / 8)  // size of highbits of poly
#define POLY_LOWBITS_PACKEDBYTES ((N * 65) / 64) // size of lowbits of poly (BAT)

#define ENCODE_MAXSIZE 863 // 对 HighBits(z1) 和 h 的rANS编码的最大尺寸

#define POLYVECK_HIGHBITS_PACKEDBYTES (K * POLY_HIGHBITS_PACKEDBYTES) // 多项式向量高位封装尺寸

#define CRYPTO_PUBLICKEYBYTES (SEEDBYTES + K * POLY_Q_PACKEDBYTES)  // pk = (seed, A_0)
#define CRYPTO_SECRETKEYBYTES (CRYPTO_PUBLICKEYBYTES + SEEDBYTES + (K+L) * POLY_S_PACKEDBYTES)  // sk = (s, K, pk)

#define CRYPTO_SIGNATUREBYTES (POLY_C_PACKEDBYTES + L * POLY_LOWBITS_PACKEDBYTES + 2 + ENCODE_MAXSIZE) // 签名尺寸 = c + LowBits(z1) + rANS_length +Encode(HighBits(z1)) + Encode(h)

#endif

/*-----------------512 bits security parameters ----------------------*/
#if DARTS_MODE == 512

#define SEEDBYTES 64

#define N 1024
#define Q 260609
#define HALF_Q 130305 // Q+1 / 2
#define ROOT_OF_UNITY 1092 // 512-th primitive root of unity module q
#define DQ (Q << 1)  // 2Q
#define K 2
#define L 3
#define P 0.2  // probability of 1 (or -1) 
#define TAU 115
#define B0 73426.15  // 小球的半径（小）
#define B 74206.14   // 超球采样半径（大）
#define B1 74202.04  // 中球的半径（中）
#define B11 80000.00 // 验签时的半径
#define GAMMA 72.73
#define D 10
#define SQNM 72.75 // sqrt(n*(k+l))

#define BASE_ENC_HB_Z1 1500
#define BASE_ENC_H 1000

#define LN 16384
#define LNHALF 8192
#define LNBITS 14

#define B0SQ ((uint64_t)(B0*B0))
#define B1SQ ((uint64_t)(B1*B1))
#define B11SQ ((uint64_t)(B11*B11))
#define BSQ ((uint64_t)(B*B))

#define Q_BITS 18

#define POLY_Q_PACKEDBYTES 2304 // 18 * n / 8 bytes
#define POLY_S_PACKEDBYTES 256  // 2 * n / 8 bytes
#define POLY_C_PACKEDBYTES 128   // size of c (n bits)

#define POLY_HIGHBITS_PACKEDBYTES ((N * D) / 8)  // size of highbits of poly
#define POLY_LOWBITS_PACKEDBYTES (N) // size of lowbits of poly (BAT)

#define ENCODE_MAXSIZE 2649  // 对 HighBits(z1) 和 h 的rANS编码的最大尺寸

#define POLYVECK_HIGHBITS_PACKEDBYTES (K * POLY_HIGHBITS_PACKEDBYTES) // 多项式向量高位封装尺寸

#define CRYPTO_PUBLICKEYBYTES (SEEDBYTES + K * POLY_Q_PACKEDBYTES)  // pk = (seed, A_0)
#define CRYPTO_SECRETKEYBYTES (CRYPTO_PUBLICKEYBYTES + SEEDBYTES + (K+L) * POLY_S_PACKEDBYTES)  // sk = (s, K, pk)

#define CRYPTO_SIGNATUREBYTES (POLY_C_PACKEDBYTES + L * POLY_LOWBITS_PACKEDBYTES + 2 + ENCODE_MAXSIZE) // 签名尺寸 = c + LowBits(z1) + rANS_length +Encode(HighBits(z1)) + Encode(h)

#endif


#endif