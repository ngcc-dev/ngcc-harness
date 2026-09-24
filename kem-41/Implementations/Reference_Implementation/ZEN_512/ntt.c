/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-512 instance.
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

/// @brief Multiply two polynomials of degree less than 4 using one level of Karatsuba multiplication
/// @param[out] r Base address of output coefficient array of length 7, storing the product polynomial
/// @param[in] a Base address of first input polynomial coefficient array of length 4
/// @param[in] b Base address of second input polynomial coefficient array of length 4
/// @return None
static inline void karatsuba_mul4(int16_t *r, const int16_t *a, const int16_t *b)
{
    int16_t a01, a23, b01, b23;
    int16_t p0[3], p1[3], pm[3];

    /* low part: (a0 + a1*x)(b0 + b1*x) */
    p0[0] = fqmul(a[0], b[0]);
    p0[2] = fqmul(a[1], b[1]);
    p0[1] = fqmul(a[0] + a[1], b[0] + b[1]) - p0[0] - p0[2];

    /* high part: (a2 + a3*x)(b2 + b3*x) */
    p1[0] = fqmul(a[2], b[2]);
    p1[2] = fqmul(a[3], b[3]);
    p1[1] = fqmul(a[2] + a[3], b[2] + b[3]) - p1[0] - p1[2];

    /* middle product */
    a01 = a[0] + a[2];
    a23 = a[1] + a[3];
    b01 = b[0] + b[2];
    b23 = b[1] + b[3];

    pm[0] = fqmul(a01, b01);
    pm[2] = fqmul(a23, b23);
    pm[1] = fqmul(a01 + a23, b01 + b23) - pm[0] - pm[2];

    r[0] = p0[0];
    r[1] = p0[1];
    r[2] = p0[2] + pm[0] - p0[0] - p1[0];
    r[3] = pm[1] - p0[1] - p1[1];
    r[4] = p1[0] + pm[2] - p0[2] - p1[2];
    r[5] = p1[1];
    r[6] = p1[2];
}

/// @brief Multiply two polynomials of degree less than 8 using a recursive Karatsuba multiplication based on karatsuba_mul4
/// @param[out] r Base address of output coefficient array of length 15, storing the product polynomial
/// @param[in] a Base address of first input polynomial coefficient array of length 8
/// @param[in] b Base address of second input polynomial coefficient array of length 8
/// @return None
static inline void karatsuba_mul8(int16_t *r, const int16_t *a, const int16_t *b)
{
    unsigned int i;
    int16_t as[4], bs[4];
    int16_t p0[7], p1[7], pm[7];

    karatsuba_mul4(p0, a,     b);
    karatsuba_mul4(p1, a + 4, b + 4);

    for(i = 0; i < 4; i++)
    {
        as[i] = a[i] + a[i + 4];
        bs[i] = b[i] + b[i + 4];
    }

    karatsuba_mul4(pm, as, bs);

    r[0]  = p0[0];
    r[1]  = p0[1];
    r[2]  = p0[2];
    r[3]  = p0[3];
    r[4]  = p0[4] + pm[0] - p0[0] - p1[0];
    r[5]  = p0[5] + pm[1] - p0[1] - p1[1];
    r[6]  = p0[6] + pm[2] - p0[2] - p1[2];
    r[7]  =          pm[3] - p0[3] - p1[3];
    r[8]  = p1[0] + pm[4] - p0[4] - p1[4];
    r[9]  = p1[1] + pm[5] - p0[5] - p1[5];
    r[10] = p1[2] + pm[6] - p0[6] - p1[6];
    r[11] = p1[3];
    r[12] = p1[4];
    r[13] = p1[5];
    r[14] = p1[6];
}

void poly_ntt(int16_t *a) 
{
    unsigned int len, start, j, k;
    int16_t t, zeta;

    k = 1;
    for(len = ZEN_N >> 1; len >= 16; len >>= 1) 
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
    for(len = ZEN_N >> 1; len >= 16; len >>= 1) 
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
    for(len = 16; len <= ZEN_N >> 1; len <<= 1)
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

/// @brief Multiply two degree-15 polynomial blocks in the NTT domain with a given twiddle factor
/// @param[out] r Base address of output coefficient array of length 16
/// @param[in] a Base address of first input coefficient array of length 16
/// @param[in] b Base address of second input coefficient array of length 16
/// @param[in] zeta Twiddle factor used in the block multiplication
/// @return None
static void base_mul(int16_t *r, int16_t *a, int16_t *b, int16_t zeta)
{
    unsigned int i;
    int16_t as[8], bs[8];
    int16_t p0[15], p1[15], pm[15];
    int16_t cross[15];

    /* split:
     * a(x) = a0(x) + x^8 a1(x)
     * b(x) = b0(x) + x^8 b1(x)
     */
    karatsuba_mul8(p0, a,     b);
    karatsuba_mul8(p1, a + 8, b + 8);

    for(i = 0; i < 8; i++)
    {
        as[i] = a[i] + a[i + 8];
        bs[i] = b[i] + b[i + 8];
    }

    karatsuba_mul8(pm, as, bs);

    for(i = 0; i < 15; i++)
    {
        cross[i] = pm[i] - p0[i] - p1[i];
    }

    /* modulus: x^16 = zeta  (same convention as the original code) */

    /* coefficients 0..6:
     * r[i] = p0[i] + zeta*p1[i] + zeta*cross[i+8]
     */
    for(i = 0; i < 7; i++)
    {
        r[i] = p0[i];
        r[i] += fqmul(p1[i], zeta);
        r[i] += fqmul(cross[i + 8], zeta);
        r[i] = fqmul(r[i], MONT);
    }

    /* coefficient 7:
     * no cross[15] term
     */
    r[7] = p0[7];
    r[7] += fqmul(p1[7], zeta);
    r[7] = fqmul(r[7], MONT);

    /* coefficients 8..14:
     * r[i] = p0[i] + zeta*p1[i] + cross[i-8]
     */
    for(i = 8; i < 15; i++)
    {
        r[i] = p0[i];
        r[i] += fqmul(p1[i], zeta);
        r[i] += cross[i - 8];
        r[i] = fqmul(r[i], MONT);
    }

    /* coefficient 15:
     * only cross[7]
     */
    r[15] = cross[7];
    r[15] = fqmul(r[15], MONT);
}

void poly_basemul_ntt(int16_t *r,  int16_t *a,  int16_t *b)
{
    unsigned int i;
    for(i = 0; i < ZEN_N / 32; i++) 
    {
        base_mul(r + 32 * i, a + 32 * i, b + 32 * i, f[64 + i]);
        base_mul(r + 32 * i + 16, a + 32 * i + 16, b + 32 * i + 16, -f[64 + i]);
    }
}

/// @brief Multiply two degree-15 polynomial blocks in the NTT domain with a given twiddle factor and reduce coefficients modulo ZEN_Q
/// @param[out] r Base address of output coefficient array of length 16
/// @param[in] a Base address of first input coefficient array of length 16
/// @param[in] b Base address of second input coefficient array of length 16
/// @param[in] zeta Twiddle factor used in the block multiplication
/// @return None
static void base_mul_mq(int16_t *r, int16_t *a, int16_t *b, int16_t zeta)
{

    unsigned int i;
    int16_t as[8], bs[8];
    int16_t p0[15], p1[15], pm[15];
    int16_t cross[15];

    /* split:
     * a(x) = a0(x) + x^8 a1(x)
     * b(x) = b0(x) + x^8 b1(x)
     */
    karatsuba_mul8(p0, a,     b);
    karatsuba_mul8(p1, a + 8, b + 8);

    for(i = 0; i < 8; i++)
    {
        as[i] = a[i] + a[i + 8];
        bs[i] = b[i] + b[i + 8];
    }

    karatsuba_mul8(pm, as, bs);

    for(i = 0; i < 15; i++)
    {
        cross[i] = pm[i] - p0[i] - p1[i];
    }

    /* modulus: x^16 = zeta  (same convention as the original code) */

    /* coefficients 0..6:
     * r[i] = p0[i] + zeta*p1[i] + zeta*cross[i+8]
     */
    for(i = 0; i < 7; i++)
    {
        r[i] = p0[i];
        r[i] += fqmul(p1[i], zeta);
        r[i] += fqmul(cross[i + 8], zeta);
        r[i] = fqmul(r[i], 19);
        r[i] += (r[i] >> 15) & ZEN_Q;
    }

    /* coefficient 7:
     * no cross[15] term
     */
    r[7] = p0[7];
    r[7] += fqmul(p1[7], zeta);
    r[7] = fqmul(r[7], 19);
    r[7] += (r[7] >> 15) & ZEN_Q;

    /* coefficients 8..14:
     * r[i] = p0[i] + zeta*p1[i] + cross[i-8]
     */
    for(i = 8; i < 15; i++)
    {
        r[i] = p0[i];
        r[i] += fqmul(p1[i], zeta);
        r[i] += cross[i - 8];
        r[i] = fqmul(r[i], 19);
        r[i] += (r[i] >> 15) & ZEN_Q;
    }

    /* coefficient 15:
     * only cross[7]
     */
    r[15] = cross[7];
    r[15] = fqmul(r[15], 19);
    r[15] += (r[15] >> 15) & ZEN_Q;
}

void poly_basemul_ntt_mq(int16_t *r,  int16_t *a,  int16_t *b)
{
    unsigned int i;
    for(i = 0; i < ZEN_N / 32; i++) 
    {
        base_mul_mq(r + 32 * i, a + 32 * i, b + 32 * i, f[64 + i]);
        base_mul_mq(r + 32 * i + 16, a + 32 * i + 16, b + 32 * i + 16, -f[64 + i]);
    }
}

static void base_inv(int16_t *r, int16_t *a, int16_t zeta)
{
    unsigned int i;
    int16_t t;

    int16_t h[8], b[4], g[8];
    int16_t ae[8], ao[8];
    int16_t he[4], ho[4];
    int16_t pe15[15], po15[15];
    int16_t pe7[7],  po7[7];

    int16_t c0, c1, e;
    int16_t f0, f1, f2, f3;

    /* split a into even / odd parts */
    for(i = 0; i < 8; i++)
    {
        ae[i] = a[2 * i];
        ao[i] = a[2 * i + 1];
    }

    /*
     * h = ae^2 - x * ao^2 mod (x^8 + zeta)
     */
    karatsuba_mul8(pe15, ae, ae);
    karatsuba_mul8(po15, ao, ao);

    h[0] = pe15[0] - fqmul(pe15[8] - po15[7], zeta);
    h[1] = pe15[1] - po15[0] - fqmul(pe15[9]  - po15[8],  zeta);
    h[2] = pe15[2] - po15[1] - fqmul(pe15[10] - po15[9],  zeta);
    h[3] = pe15[3] - po15[2] - fqmul(pe15[11] - po15[10], zeta);
    h[4] = pe15[4] - po15[3] - fqmul(pe15[12] - po15[11], zeta);
    h[5] = pe15[5] - po15[4] - fqmul(pe15[13] - po15[12], zeta);
    h[6] = pe15[6] - po15[5] - fqmul(pe15[14] - po15[13], zeta);
    h[7] = pe15[7] - po15[6] + fqmul(po15[14], zeta);

    /*
     * split h into even / odd parts
     */
    he[0] = h[0];
    he[1] = h[2];
    he[2] = h[4];
    he[3] = h[6];

    ho[0] = h[1];
    ho[1] = h[3];
    ho[2] = h[5];
    ho[3] = h[7];

    /*
     * b = he^2 - x * ho^2 mod (x^4 + zeta)
     */
    karatsuba_mul4(pe7, he, he);
    karatsuba_mul4(po7, ho, ho);

    b[0] = pe7[0] - fqmul(pe7[4] - po7[3], zeta);
    b[1] = pe7[1] - po7[0] - fqmul(pe7[5] - po7[4], zeta);
    b[2] = pe7[2] - po7[1] - fqmul(pe7[6] - po7[5], zeta);
    b[3] = pe7[3] - po7[2] + fqmul(po7[6], zeta);

    /*
     * c = (b0 + b2*x)^2 - x * (b1 + b3*x)^2 mod (x^2 + zeta)
     */
    {
        int16_t q0e, q1e, q2e;
        int16_t q0o, q1o, q2o;

        q0e = fqmul(b[0], b[0]);
        q2e = fqmul(b[2], b[2]);
        q1e = fqmul(fqmul(b[0], b[2]), 342);

        q0o = fqmul(b[1], b[1]);
        q2o = fqmul(b[3], b[3]);
        q1o = fqmul(fqmul(b[1], b[3]), 342);

        c0 = q0e - fqmul(q2e - q1o, zeta);
        c1 = q1e - q0o + fqmul(q2o, zeta);
    }

    /*
     * e = 1 / (c0^2 + zeta*c1^2)
     */
    e  = fqmul(c1, c1);
    e  = fqmul(e, zeta);
    t  = fqmul(c0, c0);
    e += t;
    e += (e >> 15) & ZEN_Q;
    e  = qinv[e];

    c0 = fqmul(e, c0);
    c1 = fqmul(e, c1);
    c1 = fqmul(c1, -171);

    /*
     * f = (c0 - x*c1) * (b0 - x*b1 + x^2*b2 - x^3*b3) mod (x^4 + zeta)
     */
    f0 = fqmul(c1, b[2]);
    f0 = fqmul(f0, zeta);
    t  = fqmul(c0, b[0]);
    f0 = t - f0;

    f1 = fqmul(c1, b[3]);
    f1 = fqmul(f1, zeta);
    t  = fqmul(c0, b[1]);
    f1 = f1 - t;

    f2 = fqmul(c0, b[2]);
    t  = fqmul(c1, b[0]);
    f2 += t;

    f3 = fqmul(c0, b[3]);
    t  = fqmul(c1, b[1]);
    f3 += t;
    f3 = fqmul(f3, -171);

    /*
     * g = (f0 + f1*x + f2*x^2 + f3*x^3) * (h0 - x*h1 + x^2*h2 - ... + x^6*h6 - x^7*h7)
     *   = f(x^2) * (he(x^2) - x*ho(x^2)) mod (x^8 + zeta)
     */
    {
        int16_t fvec[4];
        fvec[0] = f0;
        fvec[1] = f1;
        fvec[2] = f2;
        fvec[3] = f3;

        karatsuba_mul4(pe7, fvec, he);
        karatsuba_mul4(po7, fvec, ho);
    }

    g[0] = pe7[0] - fqmul(pe7[4], zeta);
    g[1] = fqmul(po7[4], zeta) - po7[0];
    g[2] = pe7[1] - fqmul(pe7[5], zeta);
    g[3] = fqmul(po7[5], zeta) - po7[1];
    g[4] = pe7[2] - fqmul(pe7[6], zeta);
    g[5] = fqmul(po7[6], zeta) - po7[2];
    g[6] = pe7[3];
    g[7] = fqmul(po7[3], -171);

    /*
     * r = g(x^2) * (ae(x^2) - x*ao(x^2)) mod (x^16 + zeta)
     */
    karatsuba_mul8(pe15, g, ae);
    karatsuba_mul8(po15, g, ao);

    r[0]  = pe15[0] - fqmul(pe15[8],  zeta);
    r[1]  = fqmul(po15[8],  zeta) - po15[0];
    r[2]  = pe15[1] - fqmul(pe15[9],  zeta);
    r[3]  = fqmul(po15[9],  zeta) - po15[1];
    r[4]  = pe15[2] - fqmul(pe15[10], zeta);
    r[5]  = fqmul(po15[10], zeta) - po15[2];
    r[6]  = pe15[3] - fqmul(pe15[11], zeta);
    r[7]  = fqmul(po15[11], zeta) - po15[3];
    r[8]  = pe15[4] - fqmul(pe15[12], zeta);
    r[9]  = fqmul(po15[12], zeta) - po15[4];
    r[10] = pe15[5] - fqmul(pe15[13], zeta);
    r[11] = fqmul(po15[13], zeta) - po15[5];
    r[12] = pe15[6] - fqmul(pe15[14], zeta);
    r[13] = fqmul(po15[14], zeta) - po15[6];
    r[14] = pe15[7];
    r[15] = fqmul(po15[7], -171);
}

void poly_baseinv_ntt(int16_t *r, int16_t *a)
{
    unsigned int i;
    for(i = 0; i < ZEN_N / 32; i++) 
    {
        base_inv(r + 32 * i, a + 32 * i, -f[64 + i]);
        base_inv(r + 32 * i + 16, a + 32 * i + 16, f[64 + i]);
    }
}
