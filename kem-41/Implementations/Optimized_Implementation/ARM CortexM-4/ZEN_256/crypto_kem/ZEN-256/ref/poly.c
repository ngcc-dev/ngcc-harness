/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-256 instance.
*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "params.h"
#include "auxfunc.h"
#include "sample.h"
#include "symmetric.h"

static const uint64_t pack_table[] = 
{
    1, 769, 591361, 454756609, 349707832321
};

int check_poly_inv_Zq(int16_t *a)
{
	unsigned int i;
    int32_t flag;
    uint32_t acc = 0;
    for(i = 0; i < ZEN_N; i += 8)
	{
		flag = a[i] + a[i + 1] + a[i + 2] + a[i + 3] + a[i + 4] + a[i + 5] + a[i + 6] + a[i + 7];
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

#define DEFINE_MUL_IN_R2_SMALL(N, MASKN)                              \
void mul_in_R2_##N(int16_t *a, int16_t *b, int16_t *res)               \
{                                                                      \
    unsigned int i;                                                    \
    uint64_t B;                                                        \
    uint64_t R;                                                        \
    uint64_t mask;                                                     \
    uint64_t wrap;                                                     \
                                                                       \
    B = 0;                                                             \
    R = 0;                                                             \
                                                                       \
    for (i = 0; i < (N); i++) {                                        \
        B |= ((uint64_t)((uint16_t)b[i] & 1u)) << i;                   \
    }                                                                  \
                                                                       \
    B &= (MASKN);                                                      \
                                                                       \
    for (i = 0; i < (N); i++) {                                        \
        mask = 0ULL - ((uint64_t)((uint16_t)a[i] & 1u));               \
        R ^= B & mask;                                                 \
                                                                       \
        wrap = (B >> ((N) - 1)) & 1ULL;                                \
        B = ((B << 1) | wrap) & (MASKN);                               \
    }                                                                  \
                                                                       \
    for (i = 0; i < (N); i++) {                                        \
        res[i] = (int16_t)((R >> i) & 1ULL);                           \
    }                                                                  \
}
DEFINE_MUL_IN_R2_SMALL(2,  0x0000000000000003ULL)
DEFINE_MUL_IN_R2_SMALL(4,  0x000000000000000FULL)
DEFINE_MUL_IN_R2_SMALL(8,  0x00000000000000FFULL)
DEFINE_MUL_IN_R2_SMALL(16, 0x000000000000FFFFULL)
DEFINE_MUL_IN_R2_SMALL(32, 0x00000000FFFFFFFFULL)
DEFINE_MUL_IN_R2_SMALL(64, UINT64_MAX)

#define MUL_R2_512_STEP(AOFF)                                             \
    do {                                                                  \
        for (i = 0; i < 64; i++) {                                        \
            mask = 0ULL - ((uint64_t)((uint16_t)a[(AOFF) + i] & 1u));     \
                                                                          \
            r0 ^= b0 & mask;                                              \
            r1 ^= b1 & mask;                                              \
            r2 ^= b2 & mask;                                              \
            r3 ^= b3 & mask;                                              \
            r4 ^= b4 & mask;                                              \
            r5 ^= b5 & mask;                                              \
            r6 ^= b6 & mask;                                              \
            r7 ^= b7 & mask;                                              \
                                                                          \
            wrap = b7 >> 63;                                              \
            u0 = (b0 << 1) | wrap;                                        \
            u1 = (b1 << 1) | (b0 >> 63);                                  \
            u2 = (b2 << 1) | (b1 >> 63);                                  \
            u3 = (b3 << 1) | (b2 >> 63);                                  \
            u4 = (b4 << 1) | (b3 >> 63);                                  \
            u5 = (b5 << 1) | (b4 >> 63);                                  \
            u6 = (b6 << 1) | (b5 >> 63);                                  \
            u7 = (b7 << 1) | (b6 >> 63);                                  \
                                                                          \
            b0 = u0;                                                      \
            b1 = u1;                                                      \
            b2 = u2;                                                      \
            b3 = u3;                                                      \
            b4 = u4;                                                      \
            b5 = u5;                                                      \
            b6 = u6;                                                      \
            b7 = u7;                                                      \
        }                                                                 \
    } while (0)

static inline void mul_in_R2_128(int16_t *a, int16_t *b, int16_t *res)
{
    unsigned int i;
    uint64_t b0, b1;
    uint64_t r0, r1;
    uint64_t u0, u1;
    uint64_t mask;
    uint64_t wrap;

    b0 = b1 = 0;
    r0 = r1 = 0;

    for (i = 0; i < 64; i++) {
        b0 |= ((uint64_t)((uint16_t)b[i] & 1u)) << i;
        b1 |= ((uint64_t)((uint16_t)b[i + 64] & 1u)) << i;
    }

    for (i = 0; i < 64; i++) {
        mask = 0ULL - ((uint64_t)((uint16_t)a[i] & 1u));

        r0 ^= b0 & mask;
        r1 ^= b1 & mask;

        wrap = b1 >> 63;
        u0 = (b0 << 1) | wrap;
        u1 = (b1 << 1) | (b0 >> 63);

        b0 = u0;
        b1 = u1;
    }

    for (i = 0; i < 64; i++) {
        mask = 0ULL - ((uint64_t)((uint16_t)a[i + 64] & 1u));

        r0 ^= b0 & mask;
        r1 ^= b1 & mask;

        wrap = b1 >> 63;
        u0 = (b0 << 1) | wrap;
        u1 = (b1 << 1) | (b0 >> 63);

        b0 = u0;
        b1 = u1;
    }

    for (i = 0; i < 64; i++) {
        res[i] = (int16_t)((r0 >> i) & 1ULL);
        res[i + 64] = (int16_t)((r1 >> i) & 1ULL);
    }
}

void mul_in_R2_256(int16_t *a, int16_t *b, int16_t *res)
{
    unsigned int i;
    uint64_t b0, b1, b2, b3;
    uint64_t r0, r1, r2, r3;
    uint64_t u0, u1, u2, u3;
    uint64_t mask;
    uint64_t wrap;

    b0 = b1 = b2 = b3 = 0;
    r0 = r1 = r2 = r3 = 0;

    for (i = 0; i < 64; i++) {
        b0 |= ((uint64_t)((uint16_t)b[i] & 1u)) << i;
        b1 |= ((uint64_t)((uint16_t)b[i + 64] & 1u)) << i;
        b2 |= ((uint64_t)((uint16_t)b[i + 128] & 1u)) << i;
        b3 |= ((uint64_t)((uint16_t)b[i + 192] & 1u)) << i;
    }

    for (i = 0; i < 64; i++) {
        mask = 0ULL - ((uint64_t)((uint16_t)a[i] & 1u));

        r0 ^= b0 & mask;
        r1 ^= b1 & mask;
        r2 ^= b2 & mask;
        r3 ^= b3 & mask;

        wrap = b3 >> 63;
        u0 = (b0 << 1) | wrap;
        u1 = (b1 << 1) | (b0 >> 63);
        u2 = (b2 << 1) | (b1 >> 63);
        u3 = (b3 << 1) | (b2 >> 63);

        b0 = u0;
        b1 = u1;
        b2 = u2;
        b3 = u3;
    }

    for (i = 0; i < 64; i++) {
        mask = 0ULL - ((uint64_t)((uint16_t)a[i + 64] & 1u));

        r0 ^= b0 & mask;
        r1 ^= b1 & mask;
        r2 ^= b2 & mask;
        r3 ^= b3 & mask;

        wrap = b3 >> 63;
        u0 = (b0 << 1) | wrap;
        u1 = (b1 << 1) | (b0 >> 63);
        u2 = (b2 << 1) | (b1 >> 63);
        u3 = (b3 << 1) | (b2 >> 63);

        b0 = u0;
        b1 = u1;
        b2 = u2;
        b3 = u3;
    }

    for (i = 0; i < 64; i++) {
        mask = 0ULL - ((uint64_t)((uint16_t)a[i + 128] & 1u));

        r0 ^= b0 & mask;
        r1 ^= b1 & mask;
        r2 ^= b2 & mask;
        r3 ^= b3 & mask;

        wrap = b3 >> 63;
        u0 = (b0 << 1) | wrap;
        u1 = (b1 << 1) | (b0 >> 63);
        u2 = (b2 << 1) | (b1 >> 63);
        u3 = (b3 << 1) | (b2 >> 63);

        b0 = u0;
        b1 = u1;
        b2 = u2;
        b3 = u3;
    }

    for (i = 0; i < 64; i++) {
        mask = 0ULL - ((uint64_t)((uint16_t)a[i + 192] & 1u));

        r0 ^= b0 & mask;
        r1 ^= b1 & mask;
        r2 ^= b2 & mask;
        r3 ^= b3 & mask;

        wrap = b3 >> 63;
        u0 = (b0 << 1) | wrap;
        u1 = (b1 << 1) | (b0 >> 63);
        u2 = (b2 << 1) | (b1 >> 63);
        u3 = (b3 << 1) | (b2 >> 63);

        b0 = u0;
        b1 = u1;
        b2 = u2;
        b3 = u3;
    }

    for (i = 0; i < 64; i++) {
        res[i] = (int16_t)((r0 >> i) & 1ULL);
        res[i + 64] = (int16_t)((r1 >> i) & 1ULL);
        res[i + 128] = (int16_t)((r2 >> i) & 1ULL);
        res[i + 192] = (int16_t)((r3 >> i) & 1ULL);
    }
}

void mul_in_R2_512(int16_t *a, int16_t *b, int16_t *res)
{
    unsigned int i;
    uint64_t b0, b1, b2, b3, b4, b5, b6, b7;
    uint64_t r0, r1, r2, r3, r4, r5, r6, r7;
    uint64_t u0, u1, u2, u3, u4, u5, u6, u7;
    uint64_t mask, wrap;

    b0 = b1 = b2 = b3 = b4 = b5 = b6 = b7 = 0;
    r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = 0;

    for (i = 0; i < 64; i++) {
        b0 |= ((uint64_t)((uint16_t)b[i] & 1u)) << i;
        b1 |= ((uint64_t)((uint16_t)b[i + 64] & 1u)) << i;
        b2 |= ((uint64_t)((uint16_t)b[i + 128] & 1u)) << i;
        b3 |= ((uint64_t)((uint16_t)b[i + 192] & 1u)) << i;
        b4 |= ((uint64_t)((uint16_t)b[i + 256] & 1u)) << i;
        b5 |= ((uint64_t)((uint16_t)b[i + 320] & 1u)) << i;
        b6 |= ((uint64_t)((uint16_t)b[i + 384] & 1u)) << i;
        b7 |= ((uint64_t)((uint16_t)b[i + 448] & 1u)) << i;
    }

    MUL_R2_512_STEP(0);
    MUL_R2_512_STEP(64);
    MUL_R2_512_STEP(128);
    MUL_R2_512_STEP(192);
    MUL_R2_512_STEP(256);
    MUL_R2_512_STEP(320);
    MUL_R2_512_STEP(384);
    MUL_R2_512_STEP(448);

    for (i = 0; i < 64; i++) {
        res[i] = (int16_t)((r0 >> i) & 1ULL);
        res[i + 64] = (int16_t)((r1 >> i) & 1ULL);
        res[i + 128] = (int16_t)((r2 >> i) & 1ULL);
        res[i + 192] = (int16_t)((r3 >> i) & 1ULL);
        res[i + 256] = (int16_t)((r4 >> i) & 1ULL);
        res[i + 320] = (int16_t)((r5 >> i) & 1ULL);
        res[i + 384] = (int16_t)((r6 >> i) & 1ULL);
        res[i + 448] = (int16_t)((r7 >> i) & 1ULL);
    }
}


void FastInversion(int16_t *f_inv, int16_t *f)
{
    unsigned int i, j;
    int16_t k[ZEN_N4];
    int16_t b[ZEN_N4] = {0};
    int16_t tmp_f[2 * ZEN_N4];
    int16_t tmp[ZEN_N4];
    const size_t coeff_bytes = ZEN_N4 * sizeof(int16_t);

    memcpy(tmp_f, f, coeff_bytes);
    memcpy(tmp_f + ZEN_N4, f, coeff_bytes);

    k[0] = f[0];
    for (i = 1; i < ZEN_N4; i++) 
    {
        k[i] = f[i] ^ k[i - 1];
    }

    memset(f_inv, 0, ZEN_N4 * sizeof(int16_t));
    f_inv[0] = 1;

    b[0] = 0;
    for (i = 0; i < ZEN_N4; i++) 
    {
        b[0] ^= k[i];
    }

    for (i = 0; i < ZEN_N4; i++) 
    {
        k[i] ^= (b[0] * f[i]);
    }

    for (i = 1; i < ZEN_N4; i++) 
    {
        k[i] = k[i] ^ k[i - 1];
    }

    f_inv[0] = !b[0];
    f_inv[1] = b[0];

    /*
     * Level n = 2
     */
    memset(tmp, 0, 2 * sizeof(int16_t));
    for (i = 0; i < 2; i++) 
    {
        for (j = i; j < ZEN_N4; j += 2) 
        {
            tmp[i] ^= k[j];
        }
    }

    mul_in_R2_2(f_inv, tmp, b);
    mul_in_R2_256(b, tmp_f, tmp);

    for (j = 0; j < ZEN_N4; j++) 
    {
        k[j] = k[j] ^ tmp[j];
    }

    for (i = 2; i < ZEN_N4; i += 2) 
    {
        for (j = i; j < i + 2; j++) 
        {
            k[j] = k[j] ^ k[j - 2];
        }
    }

    for (i = 0; i < 2; i++) 
    {
        tmp[i] = tmp[i + 2] = b[i];
    }

    for (i = 0; i < 4; i++) 
    {
        f_inv[i] ^= tmp[i];
    }

    /*
     * Level n = 4
     */
    memset(tmp, 0, 4 * sizeof(int16_t));
    for (i = 0; i < 4; i++) 
    {
        for (j = i; j < ZEN_N4; j += 4) 
        {
            tmp[i] ^= k[j];
        }
    }

    mul_in_R2_4(f_inv, tmp, b);
    mul_in_R2_256(b, tmp_f, tmp);

    for (j = 0; j < ZEN_N4; j++) 
    {
        k[j] = k[j] ^ tmp[j];
    }

    for (i = 4; i < ZEN_N4; i += 4) 
    {
        for (j = i; j < i + 4; j++) 
        {
            k[j] = k[j] ^ k[j - 4];
        }
    }

    for (i = 0; i < 4; i++) 
    {
        tmp[i] = tmp[i + 4] = b[i];
    }

    for (i = 0; i < 8; i++) 
    {
        f_inv[i] ^= tmp[i];
    }

    /*
     * Level n = 8
     */
    memset(tmp, 0, 8 * sizeof(int16_t));
    for (i = 0; i < 8; i++) 
    {
        for (j = i; j < ZEN_N4; j += 8) 
        {
            tmp[i] ^= k[j];
        }
    }

    mul_in_R2_8(f_inv, tmp, b);
    mul_in_R2_256(b, tmp_f, tmp);

    for (j = 0; j < ZEN_N4; j++) 
    {
        k[j] = k[j] ^ tmp[j];
    }

    for (i = 8; i < ZEN_N4; i += 8) 
    {
        for (j = i; j < i + 8; j++) 
        {
            k[j] = k[j] ^ k[j - 8];
        }
    }

    for (i = 0; i < 8; i++) 
    {
        tmp[i] = tmp[i + 8] = b[i];
    }

    for (i = 0; i < 16; i++) 
    {
        f_inv[i] ^= tmp[i];
    }

    /*
     * Level n = 16
     */
    memset(tmp, 0, 16 * sizeof(int16_t));
    for (i = 0; i < 16; i++) 
    {
        for (j = i; j < ZEN_N4; j += 16) 
        {
            tmp[i] ^= k[j];
        }
    }

    mul_in_R2_16(f_inv, tmp, b);
    mul_in_R2_256(b, tmp_f, tmp);

    for (j = 0; j < ZEN_N4; j++) 
    {
        k[j] = k[j] ^ tmp[j];
    }

    for (i = 16; i < ZEN_N4; i += 16) 
    {
        for (j = i; j < i + 16; j++) 
        {
            k[j] = k[j] ^ k[j - 16];
        }
    }

    for (i = 0; i < 16; i++) 
    {
        tmp[i] = tmp[i + 16] = b[i];
    }

    for (i = 0; i < 32; i++) 
    {
        f_inv[i] ^= tmp[i];
    }

    /*
     * Level n = 32
     */
    memset(tmp, 0, 32 * sizeof(int16_t));
    for (i = 0; i < 32; i++) 
    {
        for (j = i; j < ZEN_N4; j += 32) 
        {
            tmp[i] ^= k[j];
        }
    }

    mul_in_R2_32(f_inv, tmp, b);
    mul_in_R2_256(b, tmp_f, tmp);

    for (j = 0; j < ZEN_N4; j++) 
    {
        k[j] = k[j] ^ tmp[j];
    }

    for (i = 32; i < ZEN_N4; i += 32) 
    {
        for (j = i; j < i + 32; j++) 
        {
            k[j] = k[j] ^ k[j - 32];
        }
    }

    for (i = 0; i < 32; i++) 
    {
        tmp[i] = tmp[i + 32] = b[i];
    }

    for (i = 0; i < 64; i++) 
    {
        f_inv[i] ^= tmp[i];
    }

    /*
     * Level n = 64
     */
    memset(tmp, 0, 64 * sizeof(int16_t));
    for (i = 0; i < 64; i++) 
    {
        for (j = i; j < ZEN_N4; j += 64) 
        {
            tmp[i] ^= k[j];
        }
    }

    mul_in_R2_64(f_inv, tmp, b);
    mul_in_R2_256(b, tmp_f, tmp);

    for (j = 0; j < ZEN_N4; j++) 
    {
        k[j] = k[j] ^ tmp[j];
    }

    for (i = 64; i < ZEN_N4; i += 64) 
    {
        for (j = i; j < i + 64; j++) 
        {
            k[j] = k[j] ^ k[j - 64];
        }
    }

    for (i = 0; i < 64; i++) 
    {
        tmp[i] = tmp[i + 64] = b[i];
    }

    for (i = 0; i < 128; i++) 
    {
        f_inv[i] ^= tmp[i];
    }

    /*
     * Level n = 128
     */
    memset(tmp, 0, 128 * sizeof(int16_t));
    for (i = 0; i < 128; i++) 
    {
        for (j = i; j < ZEN_N4; j += 128) 
        {
            tmp[i] ^= k[j];
        }
    }

    mul_in_R2_128(f_inv, tmp, b);
    mul_in_R2_256(b, tmp_f, tmp);

    for (j = 0; j < ZEN_N4; j++) 
    {
        k[j] = k[j] ^ tmp[j];
    }

    for (i = 128; i < ZEN_N4; i += 128) 
    {
        for (j = i; j < i + 128; j++) 
        {
            k[j] = k[j] ^ k[j - 128];
        }
    }

    for (i = 0; i < 128; i++) 
    {
        tmp[i] = tmp[i + 128] = b[i];
    }

    for (i = 0; i < 256; i++) 
    {
        f_inv[i] ^= tmp[i];
    }
}

void poly_generate_g(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    uint8_t buf[ZEN_N_LEN_BYTES*4];

    ZEN_pseudoXOF(ZEN_N*4, seed, SEED_LEN_BYTES*8, buf, nonce);
    tenary3_16(a, buf);
}

void poly_generate_f(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    uint8_t buf[ZEN_N_LEN_BYTES*3];

    ZEN_pseudoXOF(ZEN_N*3, seed, SEED_LEN_BYTES*8, buf, nonce);
    tenary1_8(a, buf);
}

void poly_generate_se(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    uint8_t buf[ZEN_N_LEN_BYTES*2];

    ZEN_pseudoXOF(ZEN_N*2, seed, SEED_LEN_BYTES*8, buf, nonce);
    cbd1(a, buf);
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

void poly_publickey_pack(uint8_t *pa, const int16_t *a)
{
    int i, idx;
    uint64_t tmp[205] = {0};
    uint64_t res[154] = {0};

    idx = 0;
    for(i = 0; i < ZEN_N - 4; i += 5)
    {
        tmp[idx] =
              (uint64_t)(uint16_t)a[i]
            + (uint64_t)(uint16_t)a[i + 1] * pack_table[1]
            + (uint64_t)(uint16_t)a[i + 2] * pack_table[2]
            + (uint64_t)(uint16_t)a[i + 3] * pack_table[3]
            + (uint64_t)(uint16_t)a[i + 4] * pack_table[4];
        idx++;
    }

    tmp[204] =
          (uint64_t)(uint16_t)a[ZEN_N - 4]
        + (uint64_t)(uint16_t)a[ZEN_N - 3] * pack_table[1]
        + (uint64_t)(uint16_t)a[ZEN_N - 2] * pack_table[2]
        + (uint64_t)(uint16_t)a[ZEN_N - 1] * pack_table[3];

    idx = 0;
    for(i = 0; i < 204; i += 4)
    {
        uint64_t x0 = tmp[i];
        uint64_t x1 = tmp[i + 1];
        uint64_t x2 = tmp[i + 2];
        uint64_t x3 = tmp[i + 3];

        res[idx++] = x0 | ((x3 & 0xFFFFULL) << 48);
        res[idx++] = x1 | (((x3 >> 16) & 0xFFFFULL) << 48);
        res[idx++] = x2 | (((x3 >> 32) & 0xFFFFULL) << 48);
    }

    res[idx++] = tmp[204];

    memcpy(pa, (const uint8_t *)res, ZEN_INDCPA_PUBLICKEY_LEN_BYTES);
}

static inline uint32_t div769_u26(uint32_t x, uint32_t *r)
{
    uint32_t q = ((uint64_t)x * 44681065u) >> 35;

    *r = x - q * 769u;
    return q;
}

static inline uint16_t divmod769_u48(uint64_t *x)
{
    uint64_t v = *x;
    uint32_t r = 0;
    uint32_t q2, q1, q0;

    q2 = div769_u26((uint32_t)(v >> 32), &r);
    q1 = div769_u26((r << 16) | ((uint32_t)(v >> 16) & 0xffffu), &r);
    q0 = div769_u26((r << 16) | ((uint32_t)v & 0xffffu), &r);

    *x = ((uint64_t)q2 << 32) | ((uint64_t)q1 << 16) | q0;
    return (uint16_t)r;
}

void poly_publickey_unpack(int16_t *a, const uint8_t *pa)
{
    int i, idx;
    uint64_t res[154] = {0};
    uint64_t tmp[205] = {0};
    const uint64_t MASK48 = 0x0000FFFFFFFFFFFFULL;
    const uint64_t MASK39 = 0x0000007FFFFFFFFFULL;

    memcpy((uint8_t *)res, pa, ZEN_INDCPA_PUBLICKEY_LEN_BYTES);

    idx = 153;

    /* last packed block: 4 coefficients packed into 39 bits */
    tmp[204] = res[idx--] & MASK39;

    for (i = 203; i > 0; i -= 4)
    {
        tmp[i - 1] = res[idx] & MASK48;
        tmp[i] = ((res[idx--] >> 48) & 0xFFFFULL) << 32;

        tmp[i - 2] = res[idx] & MASK48;
        tmp[i] |= ((res[idx--] >> 48) & 0xFFFFULL) << 16;

        tmp[i - 3] = res[idx] & MASK48;
        tmp[i] |= ((res[idx--] >> 48) & 0xFFFFULL);
    }

    for (i = 0; i < 4; i++)
    {
        a[ZEN_N - 4 + i] = divmod769_u48(&tmp[204]);
    }

    idx = 0;
    for (i = 0; i < ZEN_N - 4; i += 5)
    {
        for (int j = 0; j < 5; j++)
        {
            a[i + j] = divmod769_u48(&tmp[idx]);
        }

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
        d *= 2727;
        d >>= 21;
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
