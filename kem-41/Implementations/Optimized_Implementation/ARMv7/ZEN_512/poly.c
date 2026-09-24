/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-512 instance.
*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "params.h"
#include "auxfunc.h"
#include "sample.h"
#include "symmetric.h"

int check_poly_inv_Zq(int16_t *a)
{
	unsigned int i;
    int32_t flag;
    uint32_t acc = 0;
    for(i = 0; i < ZEN_N; i += 16)
	{
		flag = a[i]      + a[i + 1]  + a[i + 2]  + a[i + 3] 
             + a[i + 4]  + a[i + 5]  + a[i + 6]  + a[i + 7]
             + a[i + 8]  + a[i + 9]  + a[i + 10] + a[i + 11]
             + a[i + 12] + a[i + 13] + a[i + 14] + a[i + 15];
		acc |= (((flag | (0u - flag)) >> 31) ^ 1u);
	}
	return (int)(acc & 1u);
}

int check_poly_inv_Z2(int16_t *a)
{
	unsigned int i;
    uint32_t acc = 0;
    for(i = 0; i < ZEN_N4; i++)
    {
        acc ^= a[i];
    }
	return (int)((((acc | (0u - acc)) >> 31) ^ 1u) & 1u);
}

#define MUL_R2_1024_STEP(AOFF)                                             \
    do {                                                                   \
        for (i = 0; i < 64; i++) {                                         \
            mask = 0ULL - ((uint64_t)((uint16_t)a[(AOFF) + i] & 1u));      \
                                                                           \
            r0  ^= b0  & mask;                                             \
            r1  ^= b1  & mask;                                             \
            r2  ^= b2  & mask;                                             \
            r3  ^= b3  & mask;                                             \
            r4  ^= b4  & mask;                                             \
            r5  ^= b5  & mask;                                             \
            r6  ^= b6  & mask;                                             \
            r7  ^= b7  & mask;                                             \
            r8  ^= b8  & mask;                                             \
            r9  ^= b9  & mask;                                             \
            r10 ^= b10 & mask;                                             \
            r11 ^= b11 & mask;                                             \
            r12 ^= b12 & mask;                                             \
            r13 ^= b13 & mask;                                             \
            r14 ^= b14 & mask;                                             \
            r15 ^= b15 & mask;                                             \
                                                                           \
            wrap = b15 >> 63;                                              \
            u0  = (b0  << 1) | wrap;                                       \
            u1  = (b1  << 1) | (b0  >> 63);                                \
            u2  = (b2  << 1) | (b1  >> 63);                                \
            u3  = (b3  << 1) | (b2  >> 63);                                \
            u4  = (b4  << 1) | (b3  >> 63);                                \
            u5  = (b5  << 1) | (b4  >> 63);                                \
            u6  = (b6  << 1) | (b5  >> 63);                                \
            u7  = (b7  << 1) | (b6  >> 63);                                \
            u8  = (b8  << 1) | (b7  >> 63);                                \
            u9  = (b9  << 1) | (b8  >> 63);                                \
            u10 = (b10 << 1) | (b9  >> 63);                                \
            u11 = (b11 << 1) | (b10 >> 63);                                \
            u12 = (b12 << 1) | (b11 >> 63);                                \
            u13 = (b13 << 1) | (b12 >> 63);                                \
            u14 = (b14 << 1) | (b13 >> 63);                                \
            u15 = (b15 << 1) | (b14 >> 63);                                \
                                                                           \
            b0  = u0;                                                      \
            b1  = u1;                                                      \
            b2  = u2;                                                      \
            b3  = u3;                                                      \
            b4  = u4;                                                      \
            b5  = u5;                                                      \
            b6  = u6;                                                      \
            b7  = u7;                                                      \
            b8  = u8;                                                      \
            b9  = u9;                                                      \
            b10 = u10;                                                     \
            b11 = u11;                                                     \
            b12 = u12;                                                     \
            b13 = u13;                                                     \
            b14 = u14;                                                     \
            b15 = u15;                                                     \
        }                                                                  \
    } while (0)

void mul_in_R2_1024(int16_t *a, int16_t *b, int16_t *res)
{
    unsigned int i;
    uint64_t b0, b1, b2, b3, b4, b5, b6, b7;
    uint64_t b8, b9, b10, b11, b12, b13, b14, b15;
    uint64_t r0, r1, r2, r3, r4, r5, r6, r7;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t u0, u1, u2, u3, u4, u5, u6, u7;
    uint64_t u8, u9, u10, u11, u12, u13, u14, u15;
    uint64_t mask, wrap;

    b0 = b1 = b2 = b3 = b4 = b5 = b6 = b7 = 0;
    b8 = b9 = b10 = b11 = b12 = b13 = b14 = b15 = 0;
    r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = 0;
    r8 = r9 = r10 = r11 = r12 = r13 = r14 = r15 = 0;

    for (i = 0; i < 64; i++) {
        b0  |= ((uint64_t)((uint16_t)b[i]       & 1u)) << i;
        b1  |= ((uint64_t)((uint16_t)b[i + 64]  & 1u)) << i;
        b2  |= ((uint64_t)((uint16_t)b[i + 128] & 1u)) << i;
        b3  |= ((uint64_t)((uint16_t)b[i + 192] & 1u)) << i;
        b4  |= ((uint64_t)((uint16_t)b[i + 256] & 1u)) << i;
        b5  |= ((uint64_t)((uint16_t)b[i + 320] & 1u)) << i;
        b6  |= ((uint64_t)((uint16_t)b[i + 384] & 1u)) << i;
        b7  |= ((uint64_t)((uint16_t)b[i + 448] & 1u)) << i;
        b8  |= ((uint64_t)((uint16_t)b[i + 512] & 1u)) << i;
        b9  |= ((uint64_t)((uint16_t)b[i + 576] & 1u)) << i;
        b10 |= ((uint64_t)((uint16_t)b[i + 640] & 1u)) << i;
        b11 |= ((uint64_t)((uint16_t)b[i + 704] & 1u)) << i;
        b12 |= ((uint64_t)((uint16_t)b[i + 768] & 1u)) << i;
        b13 |= ((uint64_t)((uint16_t)b[i + 832] & 1u)) << i;
        b14 |= ((uint64_t)((uint16_t)b[i + 896] & 1u)) << i;
        b15 |= ((uint64_t)((uint16_t)b[i + 960] & 1u)) << i;
    }

    MUL_R2_1024_STEP(0);
    MUL_R2_1024_STEP(64);
    MUL_R2_1024_STEP(128);
    MUL_R2_1024_STEP(192);
    MUL_R2_1024_STEP(256);
    MUL_R2_1024_STEP(320);
    MUL_R2_1024_STEP(384);
    MUL_R2_1024_STEP(448);
    MUL_R2_1024_STEP(512);
    MUL_R2_1024_STEP(576);
    MUL_R2_1024_STEP(640);
    MUL_R2_1024_STEP(704);
    MUL_R2_1024_STEP(768);
    MUL_R2_1024_STEP(832);
    MUL_R2_1024_STEP(896);
    MUL_R2_1024_STEP(960);

    for (i = 0; i < 64; i++) {
        res[i]       = (int16_t)((r0  >> i) & 1ULL);
        res[i + 64]  = (int16_t)((r1  >> i) & 1ULL);
        res[i + 128] = (int16_t)((r2  >> i) & 1ULL);
        res[i + 192] = (int16_t)((r3  >> i) & 1ULL);
        res[i + 256] = (int16_t)((r4  >> i) & 1ULL);
        res[i + 320] = (int16_t)((r5  >> i) & 1ULL);
        res[i + 384] = (int16_t)((r6  >> i) & 1ULL);
        res[i + 448] = (int16_t)((r7  >> i) & 1ULL);
        res[i + 512] = (int16_t)((r8  >> i) & 1ULL);
        res[i + 576] = (int16_t)((r9  >> i) & 1ULL);
        res[i + 640] = (int16_t)((r10 >> i) & 1ULL);
        res[i + 704] = (int16_t)((r11 >> i) & 1ULL);
        res[i + 768] = (int16_t)((r12 >> i) & 1ULL);
        res[i + 832] = (int16_t)((r13 >> i) & 1ULL);
        res[i + 896] = (int16_t)((r14 >> i) & 1ULL);
        res[i + 960] = (int16_t)((r15 >> i) & 1ULL);
    }
}

#define DO_FASTINV_LEVEL_64(N, MASKN)                                  \
    do {                                                               \
        for (i = 0; i < (N); i++) {                                    \
            acc = 0;                                                   \
            for (j = i; j < ZEN_N4; j += (N)) {                  \
                acc ^= k[j];                                           \
            }                                                          \
            tmp[i] = acc;                                              \
        }                                                              \
                                                                       \
        /*                                                            \
         * bp = f_inv[0..N-1] * tmp[0..N-1] in R2_N.                  \
         * The product is kept packed, so no b[] unpacking is needed. \
         */                                                           \
        ap = 0;                                                        \
        tp = 0;                                                        \
        for (i = 0; i < (N); i++) {                                    \
            ap |= ((uint64_t)((uint16_t)f_inv[i] & 1u)) << i;          \
            tp |= ((uint64_t)((uint16_t)tmp[i] & 1u)) << i;            \
        }                                                              \
                                                                       \
        B = tp & (MASKN);                                              \
        bp = 0;                                                        \
        for (i = 0; i < (N); i++) {                                    \
            mask = 0ULL - ((ap >> i) & 1ULL);                          \
            bp ^= B & mask;                                            \
                                                                       \
            wrap = (B >> ((N) - 1)) & 1ULL;                            \
            B = ((B << 1) | wrap) & (MASKN);                           \
        }                                                              \
        bp &= (MASKN);                                                 \
                                                                       \
        /*                                                            \
         * tmp = bp * f in R2_512.                                    \
         * Since bp has only N valid bits, only N rotations are needed.\
         */                                                           \
        g0 = f0;                                                       \
        g1 = f1;                                                       \
        g2 = f2;                                                       \
        g3 = f3;                                                       \
        g4 = f4;                                                       \
        g5 = f5;                                                       \
        g6 = f6;                                                       \
        g7 = f7;                                                       \
        r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = 0;                     \
                                                                       \
        for (i = 0; i < (N); i++) {                                    \
            mask = 0ULL - ((bp >> i) & 1ULL);                          \
            r0 ^= g0 & mask;                                           \
            r1 ^= g1 & mask;                                           \
            r2 ^= g2 & mask;                                           \
            r3 ^= g3 & mask;                                           \
            r4 ^= g4 & mask;                                           \
            r5 ^= g5 & mask;                                           \
            r6 ^= g6 & mask;                                           \
            r7 ^= g7 & mask;                                           \
                                                                       \
            wrap = g7 >> 63;                                           \
            u0 = (g0 << 1) | wrap;                                     \
            u1 = (g1 << 1) | (g0 >> 63);                               \
            u2 = (g2 << 1) | (g1 >> 63);                               \
            u3 = (g3 << 1) | (g2 >> 63);                               \
            u4 = (g4 << 1) | (g3 >> 63);                               \
            u5 = (g5 << 1) | (g4 >> 63);                               \
            u6 = (g6 << 1) | (g5 >> 63);                               \
            u7 = (g7 << 1) | (g6 >> 63);                               \
                                                                       \
            g0 = u0;                                                   \
            g1 = u1;                                                   \
            g2 = u2;                                                   \
            g3 = u3;                                                   \
            g4 = u4;                                                   \
            g5 = u5;                                                   \
            g6 = u6;                                                   \
            g7 = u7;                                                   \
        }                                                              \
                                                                       \
        for (i = 0; i < 64; i++) {                                     \
            k[i]       ^= (int16_t)((r0 >> i) & 1ULL);                 \
            k[i + 64]  ^= (int16_t)((r1 >> i) & 1ULL);                 \
            k[i + 128] ^= (int16_t)((r2 >> i) & 1ULL);                 \
            k[i + 192] ^= (int16_t)((r3 >> i) & 1ULL);                 \
            k[i + 256] ^= (int16_t)((r4 >> i) & 1ULL);                 \
            k[i + 320] ^= (int16_t)((r5 >> i) & 1ULL);                 \
            k[i + 384] ^= (int16_t)((r6 >> i) & 1ULL);                 \
            k[i + 448] ^= (int16_t)((r7 >> i) & 1ULL);                 \
        }                                                              \
                                                                       \
        for (i = (N); i < ZEN_N4; i += (N)) {                    \
            for (j = i; j < i + (N); j++) {                            \
                k[j] ^= k[j - (N)];                                    \
            }                                                          \
        }                                                              \
                                                                       \
        for (i = 0; i < (N); i++) {                                    \
            bit = (int16_t)((bp >> i) & 1ULL);                         \
            f_inv[i]       ^= bit;                                     \
            f_inv[i + (N)] ^= bit;                                     \
        }                                                              \
    } while (0)

#define MUL_BP256_STEP(APWORD)                                         \
    do {                                                              \
        for (i = 0; i < 64; i++) {                                    \
            mask = 0ULL - (((APWORD) >> i) & 1ULL);                   \
            bp0 ^= B0 & mask;                                         \
            bp1 ^= B1 & mask;                                         \
            bp2 ^= B2 & mask;                                         \
            bp3 ^= B3 & mask;                                         \
                                                                      \
            wrap = B3 >> 63;                                          \
            u0 = (B0 << 1) | wrap;                                    \
            u1 = (B1 << 1) | (B0 >> 63);                              \
            u2 = (B2 << 1) | (B1 >> 63);                              \
            u3 = (B3 << 1) | (B2 >> 63);                              \
            B0 = u0;                                                  \
            B1 = u1;                                                  \
            B2 = u2;                                                  \
            B3 = u3;                                                  \
        }                                                             \
    } while (0)

#define MUL_BPF512_STEP(BPWORD)                                        \
    do {                                                              \
        for (i = 0; i < 64; i++) {                                    \
            mask = 0ULL - (((BPWORD) >> i) & 1ULL);                   \
            r0 ^= g0 & mask;                                          \
            r1 ^= g1 & mask;                                          \
            r2 ^= g2 & mask;                                          \
            r3 ^= g3 & mask;                                          \
            r4 ^= g4 & mask;                                          \
            r5 ^= g5 & mask;                                          \
            r6 ^= g6 & mask;                                          \
            r7 ^= g7 & mask;                                          \
                                                                      \
            wrap = g7 >> 63;                                          \
            u0 = (g0 << 1) | wrap;                                    \
            u1 = (g1 << 1) | (g0 >> 63);                              \
            u2 = (g2 << 1) | (g1 >> 63);                              \
            u3 = (g3 << 1) | (g2 >> 63);                              \
            u4 = (g4 << 1) | (g3 >> 63);                              \
            u5 = (g5 << 1) | (g4 >> 63);                              \
            u6 = (g6 << 1) | (g5 >> 63);                              \
            u7 = (g7 << 1) | (g6 >> 63);                              \
            g0 = u0;                                                  \
            g1 = u1;                                                  \
            g2 = u2;                                                  \
            g3 = u3;                                                  \
            g4 = u4;                                                  \
            g5 = u5;                                                  \
            g6 = u6;                                                  \
            g7 = u7;                                                  \
        }                                                             \
    } while (0)

void FastInversion(int16_t *f_inv, int16_t *f)
{
    unsigned int i, j;
    int16_t k[ZEN_N4];
    int16_t tmp[ZEN_N4];
    int16_t acc;
    int16_t bit;

    uint64_t f0, f1, f2, f3, f4, f5, f6, f7;
    uint64_t ap, tp, bp;
    uint64_t ap0, ap1, tp0, tp1, bp0, bp1;
    uint64_t ap2, ap3, tp2, tp3, bp2, bp3;
    uint64_t B, B0, B1, B2, B3;
    uint64_t mask;
    uint64_t wrap;
    uint64_t g0, g1, g2, g3, g4, g5, g6, g7;
    uint64_t r0, r1, r2, r3, r4, r5, r6, r7;
    uint64_t u0, u1, u2, u3, u4, u5, u6, u7;

    /*
     * Pack f[0..511] once.
     * The original code copied f into tmp_f and then mul_in_R2_512
     * repacked tmp_f at every level.
     */
    f0 = f1 = f2 = f3 = f4 = f5 = f6 = f7 = 0;
    for (i = 0; i < 64; i++) {
        f0 |= ((uint64_t)((uint16_t)f[i]       & 1u)) << i;
        f1 |= ((uint64_t)((uint16_t)f[i + 64]  & 1u)) << i;
        f2 |= ((uint64_t)((uint16_t)f[i + 128] & 1u)) << i;
        f3 |= ((uint64_t)((uint16_t)f[i + 192] & 1u)) << i;
        f4 |= ((uint64_t)((uint16_t)f[i + 256] & 1u)) << i;
        f5 |= ((uint64_t)((uint16_t)f[i + 320] & 1u)) << i;
        f6 |= ((uint64_t)((uint16_t)f[i + 384] & 1u)) << i;
        f7 |= ((uint64_t)((uint16_t)f[i + 448] & 1u)) << i;
    }

    k[0] = f[0];
    for (i = 1; i < ZEN_N4; i++) {
        k[i] = f[i] ^ k[i - 1];
    }

    memset(f_inv, 0, ZEN_N4 * sizeof(int16_t));
    f_inv[0] = 1;

    acc = 0;
    for (i = 0; i < ZEN_N4; i++) {
        acc ^= k[i];
    }

    for (i = 0; i < ZEN_N4; i++) {
        k[i] ^= (acc * f[i]);
    }

    for (i = 1; i < ZEN_N4; i++) {
        k[i] ^= k[i - 1];
    }

    f_inv[0] = !acc;
    f_inv[1] = acc;

    DO_FASTINV_LEVEL_64(2,  0x0000000000000003ULL);
    DO_FASTINV_LEVEL_64(4,  0x000000000000000FULL);
    DO_FASTINV_LEVEL_64(8,  0x00000000000000FFULL);
    DO_FASTINV_LEVEL_64(16, 0x000000000000FFFFULL);
    DO_FASTINV_LEVEL_64(32, 0x00000000FFFFFFFFULL);
    DO_FASTINV_LEVEL_64(64, UINT64_MAX);

    /*
     * Level n = 128.
     */
    for (i = 0; i < 128; i++) {
        acc = 0;
        for (j = i; j < ZEN_N4; j += 128) {
            acc ^= k[j];
        }
        tmp[i] = acc;
    }

    ap0 = ap1 = tp0 = tp1 = 0;
    for (i = 0; i < 64; i++) {
        ap0 |= ((uint64_t)((uint16_t)f_inv[i]      & 1u)) << i;
        ap1 |= ((uint64_t)((uint16_t)f_inv[i + 64] & 1u)) << i;
        tp0 |= ((uint64_t)((uint16_t)tmp[i]        & 1u)) << i;
        tp1 |= ((uint64_t)((uint16_t)tmp[i + 64]   & 1u)) << i;
    }

    /*
     * bp = f_inv[0..127] * tmp[0..127] in R2_128.
     */
    B0 = tp0;
    B1 = tp1;
    bp0 = bp1 = 0;

    for (i = 0; i < 64; i++) {
        mask = 0ULL - ((ap0 >> i) & 1ULL);
        bp0 ^= B0 & mask;
        bp1 ^= B1 & mask;

        wrap = B1 >> 63;
        u0 = (B0 << 1) | wrap;
        u1 = (B1 << 1) | (B0 >> 63);
        B0 = u0;
        B1 = u1;
    }

    for (i = 0; i < 64; i++) {
        mask = 0ULL - ((ap1 >> i) & 1ULL);
        bp0 ^= B0 & mask;
        bp1 ^= B1 & mask;

        wrap = B1 >> 63;
        u0 = (B0 << 1) | wrap;
        u1 = (B1 << 1) | (B0 >> 63);
        B0 = u0;
        B1 = u1;
    }

    /*
     * tmp = bp * f in R2_512.
     * bp has 128 valid bits, so only 128 rotations are needed.
     */
    g0 = f0;
    g1 = f1;
    g2 = f2;
    g3 = f3;
    g4 = f4;
    g5 = f5;
    g6 = f6;
    g7 = f7;
    r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = 0;

    for (i = 0; i < 64; i++) {
        mask = 0ULL - ((bp0 >> i) & 1ULL);
        r0 ^= g0 & mask;
        r1 ^= g1 & mask;
        r2 ^= g2 & mask;
        r3 ^= g3 & mask;
        r4 ^= g4 & mask;
        r5 ^= g5 & mask;
        r6 ^= g6 & mask;
        r7 ^= g7 & mask;

        wrap = g7 >> 63;
        u0 = (g0 << 1) | wrap;
        u1 = (g1 << 1) | (g0 >> 63);
        u2 = (g2 << 1) | (g1 >> 63);
        u3 = (g3 << 1) | (g2 >> 63);
        u4 = (g4 << 1) | (g3 >> 63);
        u5 = (g5 << 1) | (g4 >> 63);
        u6 = (g6 << 1) | (g5 >> 63);
        u7 = (g7 << 1) | (g6 >> 63);

        g0 = u0;
        g1 = u1;
        g2 = u2;
        g3 = u3;
        g4 = u4;
        g5 = u5;
        g6 = u6;
        g7 = u7;
    }

    for (i = 0; i < 64; i++) {
        mask = 0ULL - ((bp1 >> i) & 1ULL);
        r0 ^= g0 & mask;
        r1 ^= g1 & mask;
        r2 ^= g2 & mask;
        r3 ^= g3 & mask;
        r4 ^= g4 & mask;
        r5 ^= g5 & mask;
        r6 ^= g6 & mask;
        r7 ^= g7 & mask;

        wrap = g7 >> 63;
        u0 = (g0 << 1) | wrap;
        u1 = (g1 << 1) | (g0 >> 63);
        u2 = (g2 << 1) | (g1 >> 63);
        u3 = (g3 << 1) | (g2 >> 63);
        u4 = (g4 << 1) | (g3 >> 63);
        u5 = (g5 << 1) | (g4 >> 63);
        u6 = (g6 << 1) | (g5 >> 63);
        u7 = (g7 << 1) | (g6 >> 63);

        g0 = u0;
        g1 = u1;
        g2 = u2;
        g3 = u3;
        g4 = u4;
        g5 = u5;
        g6 = u6;
        g7 = u7;
    }

    for (i = 0; i < 64; i++) {
        k[i]       ^= (int16_t)((r0 >> i) & 1ULL);
        k[i + 64]  ^= (int16_t)((r1 >> i) & 1ULL);
        k[i + 128] ^= (int16_t)((r2 >> i) & 1ULL);
        k[i + 192] ^= (int16_t)((r3 >> i) & 1ULL);
        k[i + 256] ^= (int16_t)((r4 >> i) & 1ULL);
        k[i + 320] ^= (int16_t)((r5 >> i) & 1ULL);
        k[i + 384] ^= (int16_t)((r6 >> i) & 1ULL);
        k[i + 448] ^= (int16_t)((r7 >> i) & 1ULL);
    }

    for (i = 128; i < ZEN_N4; i += 128) {
        for (j = i; j < i + 128; j++) {
            k[j] ^= k[j - 128];
        }
    }

    for (i = 0; i < 64; i++) {
        bit = (int16_t)((bp0 >> i) & 1ULL);
        f_inv[i]       ^= bit;
        f_inv[i + 128] ^= bit;

        bit = (int16_t)((bp1 >> i) & 1ULL);
        f_inv[i + 64]  ^= bit;
        f_inv[i + 192] ^= bit;
    }

    /*
     * Level n = 256.
     */
    for (i = 0; i < 256; i++) {
        acc = 0;
        for (j = i; j < ZEN_N4; j += 256) {
            acc ^= k[j];
        }
        tmp[i] = acc;
    }

    ap0 = ap1 = ap2 = ap3 = 0;
    tp0 = tp1 = tp2 = tp3 = 0;
    for (i = 0; i < 64; i++) {
        ap0 |= ((uint64_t)((uint16_t)f_inv[i]       & 1u)) << i;
        ap1 |= ((uint64_t)((uint16_t)f_inv[i + 64]  & 1u)) << i;
        ap2 |= ((uint64_t)((uint16_t)f_inv[i + 128] & 1u)) << i;
        ap3 |= ((uint64_t)((uint16_t)f_inv[i + 192] & 1u)) << i;

        tp0 |= ((uint64_t)((uint16_t)tmp[i]       & 1u)) << i;
        tp1 |= ((uint64_t)((uint16_t)tmp[i + 64]  & 1u)) << i;
        tp2 |= ((uint64_t)((uint16_t)tmp[i + 128] & 1u)) << i;
        tp3 |= ((uint64_t)((uint16_t)tmp[i + 192] & 1u)) << i;
    }

    /*
     * bp = f_inv[0..255] * tmp[0..255] in R2_256.
     */
    B0 = tp0;
    B1 = tp1;
    B2 = tp2;
    B3 = tp3;
    bp0 = bp1 = bp2 = bp3 = 0;

    MUL_BP256_STEP(ap0);
    MUL_BP256_STEP(ap1);
    MUL_BP256_STEP(ap2);
    MUL_BP256_STEP(ap3);

    /*
     * tmp = bp * f in R2_512.
     * bp has 256 valid bits, so only 256 rotations are needed.
     */
    g0 = f0;
    g1 = f1;
    g2 = f2;
    g3 = f3;
    g4 = f4;
    g5 = f5;
    g6 = f6;
    g7 = f7;
    r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = 0;

    MUL_BPF512_STEP(bp0);
    MUL_BPF512_STEP(bp1);
    MUL_BPF512_STEP(bp2);
    MUL_BPF512_STEP(bp3);

    for (i = 0; i < 64; i++) {
        k[i]       ^= (int16_t)((r0 >> i) & 1ULL);
        k[i + 64]  ^= (int16_t)((r1 >> i) & 1ULL);
        k[i + 128] ^= (int16_t)((r2 >> i) & 1ULL);
        k[i + 192] ^= (int16_t)((r3 >> i) & 1ULL);
        k[i + 256] ^= (int16_t)((r4 >> i) & 1ULL);
        k[i + 320] ^= (int16_t)((r5 >> i) & 1ULL);
        k[i + 384] ^= (int16_t)((r6 >> i) & 1ULL);
        k[i + 448] ^= (int16_t)((r7 >> i) & 1ULL);
    }

    for (j = 256; j < 512; j++) {
        k[j] ^= k[j - 256];
    }

    for (i = 0; i < 64; i++) {
        bit = (int16_t)((bp0 >> i) & 1ULL);
        f_inv[i]       ^= bit;
        f_inv[i + 256] ^= bit;

        bit = (int16_t)((bp1 >> i) & 1ULL);
        f_inv[i + 64]  ^= bit;
        f_inv[i + 320] ^= bit;

        bit = (int16_t)((bp2 >> i) & 1ULL);
        f_inv[i + 128] ^= bit;
        f_inv[i + 384] ^= bit;

        bit = (int16_t)((bp3 >> i) & 1ULL);
        f_inv[i + 192] ^= bit;
        f_inv[i + 448] ^= bit;
    }
}

void poly_generate_gf(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    uint8_t buf[ZEN_N_LEN_BYTES*5];

    zen_pseudoXOF(ZEN_N*5, seed, SEED_LEN_BYTES*8, buf, nonce);
    tenary3_32(a, buf);
}

void poly_generate_se(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    uint8_t buf[ZEN_N_LEN_BYTES*3];

    zen_pseudoXOF(ZEN_N*3, seed, SEED_LEN_BYTES*8, buf, nonce);
    tenary1_8(a, buf);
}

void poly_bit2byte_pack(uint8_t *pa, const int16_t *a, const unsigned int n)
{
    unsigned int i;
    const int16_t *src = a;

    for(i = 0; i < n / 8; i++)
    {
        pa[i] = (uint8_t)(
              ((uint16_t)src[0] & 1u)
            | (((uint16_t)src[1] & 1u) << 1)
            | (((uint16_t)src[2] & 1u) << 2)
            | (((uint16_t)src[3] & 1u) << 3)
            | (((uint16_t)src[4] & 1u) << 4)
            | (((uint16_t)src[5] & 1u) << 5)
            | (((uint16_t)src[6] & 1u) << 6)
            | (((uint16_t)src[7] & 1u) << 7));

        src += 8;
    }
}

void poly_byte2bit_unpack(int16_t *a, const uint8_t *pa, const unsigned int n)
{
    unsigned int i;
    int16_t *dst = a;

    for(i = 0; i < n / 8; i++)
    {
        uint8_t byte = pa[i];

        dst[0] = (int16_t)((byte >> 0) & 1u);
        dst[1] = (int16_t)((byte >> 1) & 1u);
        dst[2] = (int16_t)((byte >> 2) & 1u);
        dst[3] = (int16_t)((byte >> 3) & 1u);
        dst[4] = (int16_t)((byte >> 4) & 1u);
        dst[5] = (int16_t)((byte >> 5) & 1u);
        dst[6] = (int16_t)((byte >> 6) & 1u);
        dst[7] = (int16_t)((byte >> 7) & 1u);

        dst += 8;
    }
}

void poly_secretkey_pack(uint8_t *ss, const int16_t *a)
{
    unsigned int i, j, k;

    for(i = 0; i < ZEN_N; i += 32)
    {
        const int16_t *src = a + i;
        uint8_t *dst = ss + (i / 32) * 40;

        for(k = 0; k < 10; k++)
        {
            uint32_t w = 0;

            for(j = 0; j < 32; j++)
            {
                w |= (uint32_t)((((uint16_t)src[j] >> k) & 1u) << j);
            }

            dst[4 * k + 0] = (uint8_t)(w);
            dst[4 * k + 1] = (uint8_t)(w >> 8);
            dst[4 * k + 2] = (uint8_t)(w >> 16);
            dst[4 * k + 3] = (uint8_t)(w >> 24);
        }
    }
}

void poly_secretkey_unpack(int16_t *a, const uint8_t *ss)
{
    unsigned int i, j, k;

    for(i = 0; i < ZEN_N; i += 32)
    {
        int16_t *dst = a + i;
        const uint8_t *src = ss + (i / 32) * 40;

        for(j = 0; j < 32; j++)
        {
            dst[j] = 0;
        }

        for(k = 0; k < 10; k++)
        {
            uint32_t w;

            w  = (uint32_t)src[4 * k + 0];
            w |= (uint32_t)src[4 * k + 1] << 8;
            w |= (uint32_t)src[4 * k + 2] << 16;
            w |= (uint32_t)src[4 * k + 3] << 24;

            for(j = 0; j < 32; j++)
            {
                dst[j] |= (int16_t)(((w >> j) & 1u) << k);
            }
        }
    }
}

static const uint64_t pack_table[] = 
{
    1, 769, 591361, 454756609, 349707832321
};

void poly_publickey_pack(uint8_t *pa, const int16_t *a)
{
    int i, idx;
    uint64_t tmp[410] = {0};   /* 409 packed 48-bit blocks + 1 packed 29-bit block */
    uint64_t res[308] = {0};   /* ceil((409*48 + 29)/64) = 308 */

    idx = 0;
    for(i = 0; i < ZEN_N - 3; i += 5)
    {
        tmp[idx] =
              (uint64_t)(uint16_t)a[i]
            + (uint64_t)(uint16_t)a[i + 1] * pack_table[1]
            + (uint64_t)(uint16_t)a[i + 2] * pack_table[2]
            + (uint64_t)(uint16_t)a[i + 3] * pack_table[3]
            + (uint64_t)(uint16_t)a[i + 4] * pack_table[4];
        idx++;
    }

    /* last 3 coefficients -> 29 bits */
    tmp[409] =
          (uint64_t)(uint16_t)a[ZEN_N - 3]
        + (uint64_t)(uint16_t)a[ZEN_N - 2] * pack_table[1]
        + (uint64_t)(uint16_t)a[ZEN_N - 1] * pack_table[2];

    idx = 0;

    /* first 408 blocks = 102 groups of 4 blocks -> 306 uint64 words */
    for(i = 0; i < 408; i += 4)
    {
        uint64_t x0 = tmp[i];
        uint64_t x1 = tmp[i + 1];
        uint64_t x2 = tmp[i + 2];
        uint64_t x3 = tmp[i + 3];

        res[idx++] = x0 | ((x3 & 0xFFFFULL) << 48);
        res[idx++] = x1 | (((x3 >> 16) & 0xFFFFULL) << 48);
        res[idx++] = x2 | (((x3 >> 32) & 0xFFFFULL) << 48);
    }

    /* remaining one 48-bit block tmp[408] + one 29-bit block tmp[409] */
    res[idx++] = tmp[408] | ((tmp[409] & 0xFFFFULL) << 48);
    res[idx++] = (tmp[409] >> 16) & 0x1FFFULL;

    memcpy(pa, (const uint8_t *)res, ZEN_INDCPA_PUBLICKEY_LEN_BYTES);
}

/**
 * @brief 64位÷769快速除法 — 32位平台兼容替代 __uint128_t
 *
 * 等价于: (uint64_t)(((__uint128_t)x * 374811932576999ULL) >> 58)
 * 算法:  将64位操作数拆分为32位半字, 4次32×32→64乘法构建128位乘积
 *         然后右移58位得到商。对 x < 769^5 精确。
 */
static inline uint64_t div769_mul_shift(uint64_t x)
{
    const uint64_t M = 374811932576999ULL;
    uint32_t x0 = (uint32_t)x;
    uint32_t x1 = (uint32_t)(x >> 32);
    uint32_t m0 = (uint32_t)M;
    uint32_t m1 = (uint32_t)(M >> 32);

    uint64_t w0 = (uint64_t)x0 * m0;
    uint64_t w1 = (uint64_t)x0 * m1;
    uint64_t w2 = (uint64_t)x1 * m0;
    uint64_t w3 = (uint64_t)x1 * m1;

    /* 128位乘积: w3*2^64 + w1*2^32 + w2*2^32 + w0 */
    uint64_t sum1 = (w0 >> 32) + (w1 & 0xFFFFFFFFULL) + (w2 & 0xFFFFFFFFULL);
    uint64_t P_lo = ((sum1 & 0xFFFFFFFFULL) << 32) | (w0 & 0xFFFFFFFFULL);
    uint64_t P_hi = w3 + (w1 >> 32) + (w2 >> 32) + (sum1 >> 32);

    /* 右移58位 */
    return (P_hi << 6) | (P_lo >> 58);
}

void poly_publickey_unpack(int16_t *a, const uint8_t *pa)
{
    int i, idx;
    uint64_t res[308] = {0};
    uint64_t tmp[410] = {0};
    const uint64_t MASK48   = 0x0000FFFFFFFFFFFFULL;
    const uint64_t MASK13   = 0x1FFFULL;

    memcpy((uint8_t *)res, pa, 2458);

    idx = 307;

    /* last packed 29-bit block */
    tmp[409] = (res[idx--] & MASK13) << 16;

    /* remaining one 48-bit block + low 16 bits of tmp[409] */
    tmp[408] = res[idx] & MASK48;
    tmp[409] |= ((res[idx--] >> 48) & 0xFFFFULL);

    /* recover first 408 packed 48-bit blocks */
    for(i = 407; i > 0; i -= 4)
    {
        tmp[i - 1] = res[idx] & MASK48;
        tmp[i]     = ((res[idx--] >> 48) & 0xFFFFULL) << 32;

        tmp[i - 2] = res[idx] & MASK48;
        tmp[i]    |= ((res[idx--] >> 48) & 0xFFFFULL) << 16;

        tmp[i - 3] = res[idx] & MASK48;
        tmp[i]    |= ((res[idx--] >> 48) & 0xFFFFULL);
    }

    /* decode last 3 coefficients from 29-bit block */
    for(i = 0; i < 3; i++)
    {
        uint64_t x = tmp[409];
        uint64_t q = div769_mul_shift(x);
        uint64_t r = x - q * 769ULL;

        a[ZEN_N - 3 + i] = (int16_t)r;
        tmp[409] = q;
    }

    idx = 0;
    for(i = 0; i < ZEN_N - 3; i += 5)
    {
        uint64_t x, q, r;

        x = tmp[idx];

        q = div769_mul_shift(x);
        r = x - q * 769ULL;
        a[i] = (int16_t)r;
        x = q;

        q = div769_mul_shift(x);
        r = x - q * 769ULL;
        a[i + 1] = (int16_t)r;
        x = q;

        q = div769_mul_shift(x);
        r = x - q * 769ULL;
        a[i + 2] = (int16_t)r;
        x = q;

        q = div769_mul_shift(x);
        r = x - q * 769ULL;
        a[i + 3] = (int16_t)r;
        x = q;

        q = div769_mul_shift(x);
        r = x - q * 769ULL;
        a[i + 4] = (int16_t)r;

        idx++;
    }
}

void poly_ciphertext_pack(uint8_t *pa, const int16_t *a)
{
    unsigned int i;

    for(i = 0; i < ZEN_N; i++)
    {
        pa[i] = (uint8_t)(a[i] & 0xFF);
    }
}

void poly_ciphertext_unpack(int16_t *a, const uint8_t *pa)
{
    unsigned int i;

    for(i = 0; i < ZEN_N; i++)
    {
        a[i] = (int16_t)pa[i];
    }
}

void poly_compress(int16_t *a)
{
    unsigned int i;
    uint32_t d;
    for(i = 0; i < ZEN_N; i++)
    {
        // a[i] = ((((uint32_t)a[i] << 8) + ZEN_Q/2) / ZEN_Q) & 255;
        d = a[i] << 8;
        d += 384;
        d *= 10908;
        d >>= 23;
        a[i] = d & 255;
    }
}

void poly_decompress(int16_t *a)
{
    unsigned int i;
    for(i = 0; i < ZEN_N; i++)
    {
        a[i] = ((((uint32_t)a[i] * ZEN_Q) + 128) >> 8);
    }
}
