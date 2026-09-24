/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-128 instance.
*/
#include <stdint.h>
#include <stdio.h>
#include "params.h"
#include "ntt.h"

int16_t montgomery_reduce(int32_t a)
{
    int32_t t;
    int16_t u;

    u = a * QINV;
    t = (int32_t)u * ZEN_Q;
    t = a - t;
    t >>= 16;
    return t;
}

/// @brief Multiply two coefficients in the finite field modulo ZEN_Q using Montgomery reduction
/// @param[in] a First input coefficient
/// @param[in] b Second input coefficient
/// @return Product of a and b reduced modulo ZEN_Q
static inline int16_t fqmul(int16_t a, int16_t b) 
{
  return montgomery_reduce((int32_t)a * b);
}

void poly_ntt(int16_t *a) 
{
    unsigned int len, start, j, k;
    int16_t t, zeta;

    k = 1;
    for(len = ZEN_N >> 1; len >= 4; len >>= 1) 
    {
        for(start = 0; start < ZEN_N; start = j + len) 
        {
            zeta = f[k++];
            for(j = start; j < start + len; j++) 
            {
                t = fqmul(zeta, a[j + len]);
                a[j + len] = a[j] - t;
                a[j] = a[j] + t;
            }
        }
    }
}

void poly_ntt_mq(int16_t *a) 
{
    unsigned int len, start, j, k;
    int16_t t, zeta;

    k = 1;
    for(len = ZEN_N >> 1; len >= 4; len >>= 1) 
    {
        for(start = 0; start < ZEN_N; start = j + len) 
        {
            zeta = f[k++];
            for(j = start; j < start + len; j++) 
            {
                t = fqmul(zeta, a[j + len]);
                a[j + len] = a[j] - t;
                a[j] = a[j] + t;
            }
        }
    }

    for(j = 0; j < ZEN_N; j++)
    {
        a[j] = fqmul(a[j], 171);
        a[j] += (a[j] >> 15) & ZEN_Q;
    }
}

void poly_intt(int16_t *a) 
{
    unsigned int start, len, j, k;
    int16_t t, zeta;

    k = 0;
    for(len = 4; len <= ZEN_N >> 1; len <<= 1)
    {
        for(start = 0; start < ZEN_N; start =j + len)
        {
            zeta = fn[k++];
            for(j = start; j < start + len; j++) 
            {
                t = a[j];
                a[j] = (t + a[j + len]);
                a[j + len] = t - a[j + len];
                a[j + len] = fqmul(zeta, a[j + len]);
            }
        }
    }

    for(j = 0; j < ZEN_N; j++)
    {
        a[j] = fqmul(a[j], fn[127]);
    }
}

/// @brief Multiply two degree-3 polynomial blocks in the NTT domain with a given twiddle factor
/// @param[out] r Base address of output coefficient array of length 4
/// @param[in] a Base address of first input coefficient array of length 4
/// @param[in] b Base address of second input coefficient array of length 4
/// @param[in] zeta Twiddle factor used in the block multiplication
/// @return None
static void base_mul(int16_t *r, int16_t *a, int16_t *b, int16_t zeta)
{
    int16_t c0,  c1,  c2,  c3;
    c0 = fqmul(a[0], b[0]);
    c1 = fqmul(a[1], b[1]);
    c2 = fqmul(a[2], b[2]);
    c3 = fqmul(a[3], b[3]);

    r[0] = fqmul((a[1] + a[3]), (b[1] + b[3]));
    r[0] -= c1;
    r[0] -= c3;
    r[0] += c2;
    r[0] = fqmul(r[0], zeta);
    r[0] += c0;

    r[1] = fqmul((a[2] + a[3]), (b[2] + b[3]));
    r[1] -= c2;
    r[1] -= c3;
    r[1] = fqmul(r[1], zeta);
    r[1] += fqmul((a[0] + a[1]), (b[0] + b[1]));
    r[1] -= c0;
    r[1] -= c1;

    r[2] = fqmul(c3, zeta);
    r[2] += c1;
    r[2] += fqmul((a[0] + a[2]), (b[0] + b[2]));
    r[2] -= c0;
    r[2] -= c2;

    r[3] = fqmul((a[0] + a[3]), (b[0] + b[3]));
    r[3] -= c0;
    r[3] -= c3;
    r[3] += fqmul((a[1] + a[2]), (b[1] + b[2]));
    r[3] -= c1;
    r[3] -= c2;
}

void poly_basemul_ntt(int16_t *r,  int16_t *a,  int16_t *b)
{
    unsigned int i;
    for(i = 0; i < ZEN_N / 8; i++) 
    {
        base_mul(r + 8 * i, a + 8 * i, b + 8 * i, f[64 + i]);
        base_mul(r + 8 * i + 4, a + 8 * i + 4, b + 8 * i + 4, -f[64 + i]);
    }
}

/// @brief Multiply two degree-3 polynomial blocks in the NTT domain with a given twiddle factor and reduce coefficients modulo ZEN_Q
/// @param[out] r Base address of output coefficient array of length 4
/// @param[in] a Base address of first input coefficient array of length 4
/// @param[in] b Base address of second input coefficient array of length 4
/// @param[in] zeta Twiddle factor used in the block multiplication
/// @return None
static void base_mul_mq(int16_t *r, int16_t *a, int16_t *b, int16_t zeta)
{
    int16_t c0,  c1,  c2,  c3;
    c0 = fqmul(a[0], b[0]);
    c1 = fqmul(a[1], b[1]);
    c2 = fqmul(a[2], b[2]);
    c3 = fqmul(a[3], b[3]);

    r[0] = fqmul((a[1] + a[3]), (b[1] + b[3]));
    r[0] -= c1;
    r[0] -= c3;
    r[0] += c2;
    r[0] = fqmul(r[0], zeta);
    r[0] += c0;
    r[0] = fqmul(r[0], 19);
    r[0] += (r[0] >> 15) & ZEN_Q;

    r[1] = fqmul((a[2] + a[3]), (b[2] + b[3]));
    r[1] -= c2;
    r[1] -= c3;
    r[1] = fqmul(r[1], zeta);
    r[1] += fqmul((a[0] + a[1]), (b[0] + b[1]));
    r[1] -= c0;
    r[1] -= c1;
    r[1] = fqmul(r[1], 19);
    r[1] += (r[1] >> 15) & ZEN_Q;

    r[2] = fqmul(c3, zeta);
    r[2] += c1;
    r[2] += fqmul((a[0] + a[2]), (b[0] + b[2]));
    r[2] -= c0;
    r[2] -= c2;
    r[2] = fqmul(r[2], 19);
    r[2] += (r[2] >> 15) & ZEN_Q;

    r[3] = fqmul((a[0] + a[3]), (b[0] + b[3]));
    r[3] -= c0;
    r[3] -= c3;
    r[3] += fqmul((a[1] + a[2]), (b[1] + b[2]));
    r[3] -= c1;
    r[3] -= c2;
    r[3] = fqmul(r[3], 19);
    r[3] += (r[3] >> 15) & ZEN_Q;
}

void poly_basemul_ntt_mq(int16_t *r,  int16_t *a,  int16_t *b)
{
    unsigned int i;
    for(i = 0; i < ZEN_N / 8; i++) 
    {
        base_mul_mq(r + 8 * i, a + 8 * i, b + 8 * i, f[64 + i]);
        base_mul_mq(r + 8 * i + 4, a + 8 * i + 4, b + 8 * i + 4, -f[64 + i]);
    }
}

/// @brief Compute the inverse of a degree-3 polynomial block in the NTT domain with a given twiddle factor
/// @param[out] r Base address of output coefficient array of length 4
/// @param[in] a Base address of input coefficient array of length 4
/// @param[in] zeta Twiddle factor associated with the block
/// @return None
static void base_inv(int16_t *r, int16_t *a, int16_t zeta)
{
    unsigned int t, k, det;
    int16_t zeta2 = fqmul(zeta, zeta);

    r[0] = fqmul(a[2], a[2]);
    t = fqmul(a[1], a[3]);
    t = fqmul(t, 342);
    r[0] += t;
    r[0] = fqmul(r[0], a[0]);
    t = fqmul(a[1], a[1]);
    t = fqmul(t, a[2]);
    r[0] -= t;
    r[0] = fqmul(r[0], zeta);
    t = fqmul(a[3], a[3]);
    t = fqmul(t, a[2]);
    t = fqmul(t, zeta2);
    r[0] -= t;
    t = fqmul(a[0], a[0]);
    t = fqmul(t, a[0]);
    r[0] -= t;
    r[0] = fqmul(r[0], 173);

    r[1] = fqmul(a[1], a[2]);
    t = fqmul(a[0], a[3]);
    t = fqmul(t, 342);
    r[1] -= t;
    r[1] = fqmul(r[1], a[2]);
    t = fqmul(a[1], a[1]);
    t = fqmul(t, a[3]);
    r[1] -= t;
    r[1] = fqmul(r[1], zeta);
    t = fqmul(a[3], a[3]);
    t = fqmul(t, a[3]);
    t = fqmul(t, zeta2);
    r[1] += t;
    t = fqmul(a[0], a[0]);
    t = fqmul(t, a[1]);
    r[1] += t;
    r[1] = fqmul(r[1], 173);

    r[2] = fqmul(a[1], a[3]);
    r[2] = fqmul(r[2], 342);
    t = fqmul(a[2], a[2]);
    r[2] -= t;
    r[2] = fqmul(r[2], a[2]);
    t = fqmul(a[3], a[3]);
    t = fqmul(t, a[0]);
    r[2] -= t;
    r[2] = fqmul(r[2], zeta);
    t = fqmul(a[0], a[0]);
    t = fqmul(t, a[2]);
    r[2] += t;
    t = fqmul(a[1], a[1]);
    t = fqmul(t, a[0]);
    r[2] -= t;
    r[2] = fqmul(r[2], 173);

    r[3] = fqmul(a[2], a[2]);
    t = fqmul(a[1], a[3]);
    r[3] -= t;
    r[3] = fqmul(r[3], a[3]);
    r[3] = fqmul(r[3], zeta);
    t = fqmul(a[1], a[1]);
    t = fqmul(t, a[1]);
    r[3] += t;
    t = fqmul(a[0], a[2]);
    t = fqmul(t, a[1]);
    t = fqmul(t, 342);
    r[3] -= t;
    t = fqmul(a[0], a[0]);
    t = fqmul(t, a[3]);
    r[3] += t;
    r[3] = fqmul(r[3], 173);

    det = fqmul(a[2], a[2]);
    t = fqmul(a[1], a[3]);
    t = fqmul(t, 684);
    det -= t;
    det = fqmul(det, a[2]);
    det = fqmul(det, a[2]);
    t = fqmul(a[0], a[2]);
    t = fqmul(t, 342);
    t += fqmul(a[1], a[1]);
    t = fqmul(t, a[3]);
    t = fqmul(t, a[3]);
    t = fqmul(t, 342);
    det += t;
    det = fqmul(det, zeta2);
    t = fqmul(a[3], a[3]);
    t = fqmul(t, a[3]);
    t = fqmul(t, a[3]);
    t = fqmul(t, zeta2);
    t = fqmul(t, zeta);
    det = t-det;
    t = fqmul(a[0], a[0]);
    t = fqmul(t, a[0]);
    t = fqmul(t, a[0]);
    det -= t;
    t = fqmul(a[1], a[3]);
    t = fqmul(t, 342);
    t += fqmul(a[2], a[2]);
    t = fqmul(t, 342);
    t = fqmul(t, a[0]);
    t = fqmul(t, a[0]);
    k = fqmul(a[0], a[2]);
    k = fqmul(k, -684);
    k += fqmul(a[1], a[1]);
    k = fqmul(k, a[1]);
    k = fqmul(k, a[1]);
    t += k;
    t = fqmul(t, zeta);
    det += t;
    det = fqmul(det, 361);
    det += (det >> 15) & ZEN_Q;
    det = qinv[det];

    r[0] = fqmul(r[0], det);
    r[0] = fqmul(r[0], 19);
    r[1] = fqmul(r[1], det);
    r[1] = fqmul(r[1], 19);
    r[2] = fqmul(r[2], det);
    r[2] = fqmul(r[2], 19);
    r[3] = fqmul(r[3], det);
    r[3] = fqmul(r[3], 19);
}

void poly_baseinv_ntt(int16_t *r, int16_t *a)
{
    unsigned int i;
    for(i = 0; i < ZEN_N / 8; i++) 
    {
        base_inv(r + 8 * i, a + 8 * i, f[64 + i]);
        base_inv(r + 8 * i + 4, a + 8 * i + 4, -f[64 + i]);
    }
}
