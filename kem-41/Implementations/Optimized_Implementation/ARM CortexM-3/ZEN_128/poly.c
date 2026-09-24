/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-128 instance.
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
    for(i = 0; i < ZEN_N; i += 4)
	{
		flag = a[i] + a[i + 1] + a[i + 2] + a[i + 3];
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

#define DO_FASTINV_LEVEL(N, MASKN)                                      \
    do {                                                                \
        /*                                                            */ \
        /* tmp[i] = k[i] ^ k[i+N] ^ k[i+2N] ^ ...                    */ \
        /*                                                            */ \
        for (i = 0; i < (N); i++) {                                     \
            acc = 0;                                                    \
            for (j = i; j < ZEN_N4; j += (N)) {                   \
                acc ^= k[j];                                            \
            }                                                           \
            tmp[i] = acc;                                               \
        }                                                               \
                                                                        \
        /*                                                            */ \
        /* Compute bp = f_inv[0..N-1] * tmp[0..N-1] in R2_N.          */ \
        /* Keep the product packed instead of unpacking into b[].      */ \
        /*                                                            */ \
        ap = 0;                                                         \
        tp = 0;                                                         \
        for (i = 0; i < (N); i++) {                                     \
            ap |= ((uint64_t)((uint16_t)f_inv[i] & 1u)) << i;           \
            tp |= ((uint64_t)((uint16_t)tmp[i] & 1u)) << i;             \
        }                                                               \
                                                                        \
        B = tp & (MASKN);                                               \
        bp = 0;                                                         \
        for (i = 0; i < (N); i++) {                                     \
            mask = 0ULL - ((ap >> i) & 1ULL);                           \
            bp ^= B & mask;                                             \
                                                                        \
            wrap = (B >> ((N) - 1)) & 1ULL;                             \
            B = ((B << 1) | wrap) & (MASKN);                            \
        }                                                               \
        bp &= (MASKN);                                                  \
                                                                        \
        /*                                                            */ \
        /* Compute tmp = bp * f in R2_128.                            */ \
        /* Since bp has only N valid bits, only N rotations are needed.*/ \
        /* The old mul_in_R2_128 scanned all 128 coefficients.         */ \
        /*                                                            */ \
        g0 = f0;                                                        \
        g1 = f1;                                                        \
        r0 = 0;                                                         \
        r1 = 0;                                                         \
                                                                        \
        for (i = 0; i < (N); i++) {                                     \
            mask = 0ULL - ((bp >> i) & 1ULL);                           \
            r0 ^= g0 & mask;                                            \
            r1 ^= g1 & mask;                                            \
                                                                        \
            wrap = g1 >> 63;                                            \
            u0 = (g0 << 1) | wrap;                                      \
            u1 = (g1 << 1) | (g0 >> 63);                                \
            g0 = u0;                                                    \
            g1 = u1;                                                    \
        }                                                               \
                                                                        \
        /*                                                            */ \
        /* k ^= tmp, but tmp is still packed as r0,r1.                 */ \
        /* Avoid unpacking tmp[0..127] first.                          */ \
        /*                                                            */ \
        for (i = 0; i < 64; i++) {                                      \
            k[i]      ^= (int16_t)((r0 >> i) & 1ULL);                   \
            k[i + 64] ^= (int16_t)((r1 >> i) & 1ULL);                   \
        }                                                               \
                                                                        \
        /*                                                            */ \
        /* Original prefix update over blocks of size N.               */ \
        /*                                                            */ \
        for (i = (N); i < ZEN_N4; i += (N)) {                     \
            for (j = i; j < i + (N); j++) {                             \
                k[j] ^= k[j - (N)];                                     \
            }                                                           \
        }                                                               \
                                                                        \
        /*                                                            */ \
        /* Original: tmp[i] = tmp[i+N] = b[i]; f_inv ^= tmp.           */ \
        /* Here b is packed in bp, so update f_inv directly.           */ \
        /*                                                            */ \
        for (i = 0; i < (N); i++) {                                     \
            bit = (int16_t)((bp >> i) & 1ULL);                          \
            f_inv[i]       ^= bit;                                      \
            f_inv[i + (N)] ^= bit;                                      \
        }                                                               \
    } while (0)

void FastInversion(int16_t *f_inv, int16_t *f)
{
    unsigned int i, j;
    int16_t k[ZEN_N4];
    int16_t tmp[ZEN_N4];

    int16_t acc;
    int16_t bit;

    uint64_t f0, f1;
    uint64_t ap, tp, bp;
    uint64_t B;
    uint64_t mask;
    uint64_t wrap;
    uint64_t g0, g1;
    uint64_t r0, r1;
    uint64_t u0, u1;

    /*
     * Pre-pack f once.
     * Original code re-packed tmp_f inside mul_in_R2_128 at every level.
     */
    f0 = 0;
    f1 = 0;
    for (i = 0; i < 64; i++) {
        f0 |= ((uint64_t)((uint16_t)f[i] & 1u)) << i;
        f1 |= ((uint64_t)((uint16_t)f[i + 64] & 1u)) << i;
    }

    /*
     * Original prefix computation.
     */
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

    DO_FASTINV_LEVEL(2,  0x0000000000000003ULL);
    DO_FASTINV_LEVEL(4,  0x000000000000000FULL);
    DO_FASTINV_LEVEL(8,  0x00000000000000FFULL);
    DO_FASTINV_LEVEL(16, 0x000000000000FFFFULL);
    DO_FASTINV_LEVEL(32, 0x00000000FFFFFFFFULL);
    DO_FASTINV_LEVEL(64, UINT64_MAX);

}

void poly_generate_g(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    unsigned int i;
    uint8_t buf[ZEN_N_LEN_BYTES*5];
    int16_t t[ZEN_N*2];
    zen_pseudoXOF(ZEN_N*5, seed, SEED_LEN_BYTES*8, buf, nonce);
    cbd1(t, buf);
    tenary1_8(t+ZEN_N, buf+ZEN_N_LEN_BYTES*2);
    for(i = 0; i < ZEN_N; i++)
    {
        a[i] = t[i] + t[i+ZEN_N];
    }
}

void poly_generate_f(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    unsigned int i;
    uint8_t buf[ZEN_N_LEN_BYTES*2];

    zen_pseudoXOF(ZEN_N*2, seed, SEED_LEN_BYTES*8, buf, nonce);
    cbd1(a, buf);
}

void poly_generate_s(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    unsigned int i;
    uint8_t buf[ZEN_N_LEN_BYTES*7];
    int16_t t[ZEN_N*2];
    zen_pseudoXOF(ZEN_N*7, seed, SEED_LEN_BYTES*8, buf, nonce);
    cbd1(t, buf);
    tenary3_32(t+ZEN_N, buf+ZEN_N_LEN_BYTES*2);
    for(i = 0; i < ZEN_N; i++)
    {
        a[i] = t[i] + t[i+ZEN_N];
    }
}

void poly_generate_e(int16_t *a, const uint8_t *seed, uint8_t nonce)
{
    unsigned int i;
    uint8_t buf[ZEN_N_LEN_BYTES*4];

    zen_pseudoXOF(ZEN_N*4, seed, SEED_LEN_BYTES*8, buf, nonce);
    cbd2(a, buf);
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
    uint64_t tmp[103] = {0};
    uint64_t res[77]  = {0};

    idx = 0;
    for(i = 0; i < ZEN_N - 2; i += 5)
    {
        tmp[idx] =
              (uint64_t)(uint16_t)a[i]
            + (uint64_t)(uint16_t)a[i + 1] * pack_table[1]
            + (uint64_t)(uint16_t)a[i + 2] * pack_table[2]
            + (uint64_t)(uint16_t)a[i + 3] * pack_table[3]
            + (uint64_t)(uint16_t)a[i + 4] * pack_table[4];
        idx++;
    }

    tmp[102] =
          (uint64_t)(uint16_t)a[ZEN_N - 2]
        + (uint64_t)(uint16_t)a[ZEN_N - 1] * pack_table[1];

    idx = 0;
    for(i = 0; i < 100; i += 4)
    {
        uint64_t x0 = tmp[i];
        uint64_t x1 = tmp[i + 1];
        uint64_t x2 = tmp[i + 2];
        uint64_t x3 = tmp[i + 3];

        res[idx++] = x0 | ((x3 & 0xFFFFULL) << 48);
        res[idx++] = x1 | (((x3 >> 16) & 0xFFFFULL) << 48);
        res[idx++] = x2 | (((x3 >> 32) & 0xFFFFULL) << 48);
    }

    res[idx++] = tmp[100] | ((tmp[102] & 0xFFFFULL) << 48);
    res[idx++] = tmp[101] | (((tmp[102] >> 16) & 0xFULL) << 48);

    memcpy(pa, (const uint8_t *)res, ZEN_INDCPA_PUBLICKEY_LEN_BYTES);
}

extern uint64_t umull64_shr58(uint64_t a, uint64_t b);
void poly_publickey_unpack(int16_t *a, const uint8_t *pa)
{
    int i, idx;
    uint64_t res[77]  = {0};
    uint64_t tmp[103] = {0};
    const uint64_t MASK48   = 0x0000FFFFFFFFFFFFULL;
    const uint64_t DIV769_M = 374811932576999ULL;

    memcpy((uint8_t *)res, pa, ZEN_INDCPA_PUBLICKEY_LEN_BYTES);

    idx = 76;

    tmp[101] = res[idx] & MASK48;
    tmp[102] = ((res[idx--] >> 48) & 0xFULL) << 16;

    tmp[100] = res[idx] & MASK48;
    tmp[102] |= ((res[idx--] >> 48) & 0xFFFFULL);

    for(i = 99; i > 0; i -= 4)
    {
        tmp[i - 1] = res[idx] & MASK48;
        tmp[i]     = ((res[idx--] >> 48) & 0xFFFFULL) << 32;

        tmp[i - 2] = res[idx] & MASK48;
        tmp[i]    |= ((res[idx--] >> 48) & 0xFFFFULL) << 16;

        tmp[i - 3] = res[idx] & MASK48;
        tmp[i]    |= ((res[idx--] >> 48) & 0xFFFFULL);
    }

    for(i = 0; i < 2; i++)
    {
        uint64_t x = tmp[102];
        uint64_t q = umull64_shr58(x, DIV769_M);
        uint64_t r = x - q * 769ULL;

        a[ZEN_N - 2 + i] = (int16_t)r;
        tmp[102] = q;
    }

    idx = 0;
    for(i = 0; i < ZEN_N - 2; i += 5)
    {
        uint64_t x, q, r;

        x = tmp[idx];

        q = umull64_shr58(x, DIV769_M);
        r = x - q * 769ULL;
        a[i] = (int16_t)r;
        x = q;

        q = umull64_shr58(x, DIV769_M);
        r = x - q * 769ULL;
        a[i + 1] = (int16_t)r;
        x = q;

        q = umull64_shr58(x, DIV769_M);
        r = x - q * 769ULL;
        a[i + 2] = (int16_t)r;
        x = q;

        q = umull64_shr58(x, DIV769_M);
        r = x - q * 769ULL;
        a[i + 3] = (int16_t)r;
        x = q;

        q = umull64_shr58(x, DIV769_M);
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
