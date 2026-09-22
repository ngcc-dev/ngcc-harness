#include <string.h>
#include "laurus_768.h"

#define rp 512
#define cp 512
#define NROUNDS 16
#define ROL(a, offset) ((a << offset) ^ (a >> (64 - offset)))
#define ROR(a, offset) ((a >> offset) ^ (a << (64 - offset)))

 /*************************************************
 * Description: Load 8 bytes into 64-bit integer in big-endian order
 **************************************************/
#define load64(y)	((uint64_t)((y)[0] & 0xFF) << 56) | ((uint64_t)((y)[1] & 0xFF) << 48) | \
					((uint64_t)((y)[2] & 0xFF) << 40) | ((uint64_t)((y)[3] & 0xFF) << 32) | \
					((uint64_t)((y)[4] & 0xFF) << 24) | ((uint64_t)((y)[5] & 0xFF) << 16) | \
					((uint64_t)((y)[6] & 0xFF) << 8) |  ((uint64_t)((y)[7] & 0xFF))


/*************************************************
* Description: Store a 64-bit integer to array of 8 bytes in big-endian order
**************************************************/
#define store64(y, x) { (y)[0] = ((x) >> 56) & 0xFF; (y)[1] = ((x) >> 48) & 0xFF;\
					    (y)[2] = ((x) >> 40) & 0xFF; (y)[3] = ((x) >> 32) & 0xFF;\
                        (y)[4] = ((x) >> 24) & 0xFF; (y)[5] = ((x) >> 16) & 0xFF;\
					    (y)[6] = ((x) >> 8) & 0xFF; (y)[7] = (x) & 0xFF; }


/* round constants */
static const uint64_t RC[NROUNDS] = 
{
    0xb7e151628aed2a6a,
    0xaf82a3cd15dad4d7,
    0x9f4546922bb529ad,
    0xfeca8c2c576ad359,
    0x3dd51950aed526b1,
    0x7baa32a15daa4d62,
    0xf7546542bb549ac4,
    0x2ee8cb8d76a9b58b,
    0x5dd1971aed536b16,
    0xbba32e35daa6d62c,
    0xb7065d63b54d2c5b,
    0xae4cbbcf6a9ad8b5,
    0x9cd97696d5353169,
    0xf9f2ec25aa6ae2d1,
    0x33a5d94354d545a1,
    0x674bb286a9aa8b42
};

/* linear transform params */
static const int theta[5][5] = {
    {31, 1, 4, 21, 22},
    {19, 12, 15, 34, 13},
    {28, 35, 58, 9, 23},
    {55, 18, 5, 27, 44},
    {14, 43, 53, 20, 25}
};

/*************************************************
* Name:        F1600_StatePermute
*
* Description: The F1600 Permutation
*
* Arguments:   - uint64_t *A: pointer to input/output state
**************************************************/
static void F1600_StatePermute(uint64_t A[25])
{
    int i, j, round;
    uint64_t B[25], P, Q;

    for (round = 0; round < NROUNDS; round++)
    {
        // add RoundConstant --> iota
        for (i = 0; i < 25; i++)
            B[i] = A[i];
        B[0] ^= RC[round];

        // non-linear transform --> chi
        for (i = 0; i < 5; i++)
        {
            A[i] = (B[i] & B[10 + i] & B[15 + i]) ^ (B[10 + i] & B[15 + i] & B[20 + i]) ^ (B[i] & B[20 + i]) ^ (B[15 + i] & B[20 + i]) ^ B[5 + i];
            A[5 + i] = (B[5 + i] & B[15 + i] & B[20 + i]) ^ (B[i] & B[5 + i]) ^ B[10 + i];
            A[10 + i] = (B[5 + i] & B[10 + i]) ^ B[15 + i];
            A[15 + i] = (B[10 + i] & B[15 + i]) ^ B[20 + i];
            A[20 + i] = (B[15 + i] & B[20 + i]) ^ B[i];
        }
        
        // linear transform --> theta
        for (i = 0; i < 5; i++)
        {
            P = 0; Q = 0;
            for (j = 0; j < 5; j++)
            {
                P ^= A[5 * i + j];
                Q ^= ROL(A[5 * i + j], theta[i][j]);
            }
            for (j = 0; j < 5; j++)
                B[5 * i + j] = A[5 * i + j] ^ ROR(P, theta[i][j]) ^ Q;
        }

        // with shifting transform --> pi
        for (i = 0; i < 5; i++)
            for (j = 0; j < 5; j++)
                A[5 * i + j] = B[((i + 2 * j) % 5) * 5 + ((3 * i + 3 * j) % 5)];
    }
}

// assistant function g0
void G0(uint64_t MSB[24], uint64_t LSB[24], const uint64_t M[], const uint64_t S[24], uint64_t r)
{
    int i;
    uint64_t x[40];

    for (i = 0; i < r / 64; i++)
        x[i] = M[i];
    for (i = 0; i < 24; i++)
        x[i + r / 64] = S[i];

    for (i = 0; i < 24; i++)
        MSB[i] = x[i];
    for (i = 0; i < r / 64; i++)
        LSB[i] = x[i + 24];
    for (; i < 24; i++)
        LSB[i] = x[i - r / 64];
}

// assistant function g1
void G1(uint64_t MSB[24], uint64_t LSB[25], const uint64_t M[], const uint64_t S[24])
{
    int i;
    uint64_t x[32];

    for (i = 0; i < 24; i++)
        x[i] = S[i];
    for (i = 0; i < rp / 64; i++)
        x[i + 24] = M[i];

    for (i = 0; i < 24; i++)
        MSB[i] = x[i + rp / 64];
    for (i = 0; i < 17; i++)
        LSB[i] = x[i + 15];
}

/*************************************************
* Name:        laurus
*
* Description: Top laurus function with permutation
*
* Arguments:   - uint8_t *out:      pointer to output
*              - uint64_t outlen:   length of output in bits
*              - const uint8_t *in: pointer to input
*              - uint64_t inlen:    length of input in bits
*              - uint64_t c:        length of security parameter in bits
*              - uint64_t fid:      id of hash function, input 1 if laurus_xof, otherwise 0
**************************************************/
static void laurus(uint8_t* out, uint64_t outlen, const uint8_t* in, uint64_t inlen, uint64_t c, uint64_t fid)
{
    uint8_t tmp[136] = { 0 };
    uint64_t A[25], S[24] = { 0 }, M[16], MSB[24], LSB[24], d;
    uint64_t i, j, r = 1536 - c, l = inlen / r, res = inlen % r, pos;

    l -= (r == rp && res == 0 && inlen != 0);

    S[23] = (fid << 63) + c;
    for (i = 0; i < l; i++)
    {
        for (j = 0; j < r / 64; j++)
            M[j] = load64(in + 8 * j);
        A[0] = i;
        G0(A + 1, LSB, M, S, r);
        F1600_StatePermute(A);
        for (j = 0; j < 24; j++)
            S[j] = A[j + 1] ^ LSB[j];
        in += r / 8;
    }

    d = (res == 0 && r == rp && inlen != 0) || res == rp;
    if (d == 1) {
        for (j = 0; j < rp / 64; j++)
            M[j] = load64(in + 8 * j);
    }
    else {
        for (j = 0; j < res / 64; j++)
            M[j] = load64(in + 8 * j);
        in += (res / 64) * 8;
        if (res % 64) {
            pos = res % 64;

            memcpy(tmp, in, pos / 8);
            if (pos % 8) {
                tmp[pos / 8] = in[pos / 8] & ((uint8_t)0xFF << (8 - (pos % 8)));
                tmp[pos / 8] |= 1 << (7 - (pos % 8));
            }
            else
                tmp[pos / 8] = 0x80;
            M[j++] = load64(tmp);
        }
        else
            M[j++] = 0x8000000000000000ULL;
        if (res < rp) {
            for (; j < rp / 64; j++)
                M[j] = 0;
        }
        else {
            for (; j < r / 64; j++)
                M[j] = 0;
            A[0] = i;
            G0(A + 1, LSB, M, S, r);
            F1600_StatePermute(A);
            for (j = 0; j < 24; j++)
                S[j] = A[j + 1] ^ LSB[j];
            for (j = 0; j < rp / 64; j++)
                M[j] = 0;
        }
    }

    G1(MSB, LSB, M, S);
    for (i = 0; i < outlen / 1088; i++)
    {
        A[0] = ((2 + d) << 62) + i;
        for (j = 0; j < 24; j++)
            A[j + 1] = MSB[j];
        F1600_StatePermute(A);
        for (j = 0; j < 17; j++)
        {
            A[j + 8] ^= LSB[j];
            store64(out + 8 * j, A[j + 8]);
        }
        out += 136;
    }
    if (outlen % 1088)
    {
        res = outlen % 1088;

        A[0] = ((2 + d) << 62) + i;
        for (j = 0; j < 24; j++)
            A[j + 1] = MSB[j];
        F1600_StatePermute(A);
        for (j = 0; j < 17; j++)
        {
            A[j + 8] ^= LSB[j];
            store64(tmp + 8 * j, A[j + 8]);
        }
        memcpy(out, tmp, res / 8);
        if (res % 8)
            out[res / 8] = tmp[res / 8] & ((uint8_t)0xFF << (8 - (res % 8)));
    }
}

/*************************************************
* Name:        laurus_768
*
* Description: laurus-768 with non-incremental API
*
* Arguments:   - uint8_t *h:        pointer to output (96 bytes)
*              - const uint8_t *in: pointer to input
*              - uint64_t inlen:    length of input in bytes
**************************************************/
void laurus_768(uint8_t h[96], const uint8_t* in, uint64_t inlen)
{
    laurus(h, 768, in, inlen, 768, 0);
}