/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares the ZEN key-encapsulation mechanism layer for the optimized ZEN-256 instance.
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
    for(len = ZEN_N >> 1; len >= 8; len >>= 1) 
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
    for(len = ZEN_N >> 1; len >= 8; len >>= 1) 
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
    for(len = 8; len <= ZEN_N >> 1; len <<= 1)
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

/// @brief Multiply two degree-7 polynomial blocks in the NTT domain with a given twiddle factor
/// @param[out] r Base address of output coefficient array of length 8
/// @param[in] a Base address of first input coefficient array of length 8
/// @param[in] b Base address of second input coefficient array of length 8
/// @param[in] zeta Twiddle factor used in the block multiplication
/// @return None
static void base_mul(int16_t *r, int16_t *a, int16_t *b, int16_t zeta)
{
    int16_t c0, c1, c2, c3, c4, c5, c6, c7;
    int16_t s01, s02, s03, s04, s05, s06, s07;
    int16_t s12, s13, s14, s15, s16, s17;
    int16_t s23, s24, s25, s26, s27;
    int16_t s34, s35, s36, s37;
    int16_t s45, s46, s47;
    int16_t s56, s57;
    int16_t s67;
    int16_t k;

    c0 = fqmul(a[0], b[0]);
    c1 = fqmul(a[1], b[1]);
    c2 = fqmul(a[2], b[2]);
    c3 = fqmul(a[3], b[3]);
    c4 = fqmul(a[4], b[4]);
    c5 = fqmul(a[5], b[5]);
    c6 = fqmul(a[6], b[6]);
    c7 = fqmul(a[7], b[7]);

    s01 = fqmul(a[0] + a[1], b[0] + b[1]) - c0 - c1;
    s02 = fqmul(a[0] + a[2], b[0] + b[2]) - c0 - c2;
    s03 = fqmul(a[0] + a[3], b[0] + b[3]) - c0 - c3;
    s04 = fqmul(a[0] + a[4], b[0] + b[4]) - c0 - c4;
    s05 = fqmul(a[0] + a[5], b[0] + b[5]) - c0 - c5;
    s06 = fqmul(a[0] + a[6], b[0] + b[6]) - c0 - c6;
    s07 = fqmul(a[0] + a[7], b[0] + b[7]) - c0 - c7;

    s12 = fqmul(a[1] + a[2], b[1] + b[2]) - c1 - c2;
    s13 = fqmul(a[1] + a[3], b[1] + b[3]) - c1 - c3;
    s14 = fqmul(a[1] + a[4], b[1] + b[4]) - c1 - c4;
    s15 = fqmul(a[1] + a[5], b[1] + b[5]) - c1 - c5;
    s16 = fqmul(a[1] + a[6], b[1] + b[6]) - c1 - c6;
    s17 = fqmul(a[1] + a[7], b[1] + b[7]) - c1 - c7;

    s23 = fqmul(a[2] + a[3], b[2] + b[3]) - c2 - c3;
    s24 = fqmul(a[2] + a[4], b[2] + b[4]) - c2 - c4;
    s25 = fqmul(a[2] + a[5], b[2] + b[5]) - c2 - c5;
    s26 = fqmul(a[2] + a[6], b[2] + b[6]) - c2 - c6;
    s27 = fqmul(a[2] + a[7], b[2] + b[7]) - c2 - c7;

    s34 = fqmul(a[3] + a[4], b[3] + b[4]) - c3 - c4;
    s35 = fqmul(a[3] + a[5], b[3] + b[5]) - c3 - c5;
    s36 = fqmul(a[3] + a[6], b[3] + b[6]) - c3 - c6;
    s37 = fqmul(a[3] + a[7], b[3] + b[7]) - c3 - c7;

    s45 = fqmul(a[4] + a[5], b[4] + b[5]) - c4 - c5;
    s46 = fqmul(a[4] + a[6], b[4] + b[6]) - c4 - c6;
    s47 = fqmul(a[4] + a[7], b[4] + b[7]) - c4 - c7;

    s56 = fqmul(a[5] + a[6], b[5] + b[6]) - c5 - c6;
    s57 = fqmul(a[5] + a[7], b[5] + b[7]) - c5 - c7;

    s67 = fqmul(a[6] + a[7], b[6] + b[7]) - c6 - c7;

    k  = s17;
    k += s26;
    k += s35;
    k += c4;
    r[0] = c0 + fqmul(k, zeta);

    k  = s27;
    k += s36;
    k += s45;
    r[1] = s01 + fqmul(k, zeta);

    k  = s37;
    k += s46;
    k += c5;
    r[2] = s02 + c1 + fqmul(k, zeta);

    k  = s47;
    k += s56;
    r[3] = s03 + s12 + fqmul(k, zeta);

    k  = s57;
    k += c6;
    r[4] = s04 + s13 + c2 + fqmul(k, zeta);

    r[5] = s05 + s14 + s23 + fqmul(s67, zeta);

    r[6] = s06 + s15 + s24 + c3 + fqmul(c7, zeta);

    r[7] = s07 + s16 + s25 + s34;
}

void poly_basemul_ntt(int16_t *r,  int16_t *a,  int16_t *b)
{
    unsigned int i;
    for(i = 0; i < ZEN_N / 16; i++) 
    {
        base_mul(r + 16 * i, a + 16 * i, b + 16 * i, f[64 + i]);
        base_mul(r + 16 * i + 8, a + 16 * i + 8, b + 16 * i + 8, -f[64 + i]);
    }
}

/// @brief Multiply two degree-7 polynomial blocks in the NTT domain with a given twiddle factor and reduce coefficients modulo ZEN_Q
/// @param[out] r Base address of output coefficient array of length 8
/// @param[in] a Base address of first input coefficient array of length 8
/// @param[in] b Base address of second input coefficient array of length 8
/// @param[in] zeta Twiddle factor used in the block multiplication
/// @return None
static void base_mul_mq(int16_t *r, int16_t *a, int16_t *b, int16_t zeta)
{

    int16_t c0, c1, c2, c3, c4, c5, c6, c7;
    int16_t s01, s02, s03, s04, s05, s06, s07;
    int16_t s12, s13, s14, s15, s16, s17;
    int16_t s23, s24, s25, s26, s27;
    int16_t s34, s35, s36, s37;
    int16_t s45, s46, s47;
    int16_t s56, s57;
    int16_t s67;
    int16_t k;

    c0 = fqmul(a[0], b[0]);
    c1 = fqmul(a[1], b[1]);
    c2 = fqmul(a[2], b[2]);
    c3 = fqmul(a[3], b[3]);
    c4 = fqmul(a[4], b[4]);
    c5 = fqmul(a[5], b[5]);
    c6 = fqmul(a[6], b[6]);
    c7 = fqmul(a[7], b[7]);

    s01 = fqmul(a[0] + a[1], b[0] + b[1]) - c0 - c1;
    s02 = fqmul(a[0] + a[2], b[0] + b[2]) - c0 - c2;
    s03 = fqmul(a[0] + a[3], b[0] + b[3]) - c0 - c3;
    s04 = fqmul(a[0] + a[4], b[0] + b[4]) - c0 - c4;
    s05 = fqmul(a[0] + a[5], b[0] + b[5]) - c0 - c5;
    s06 = fqmul(a[0] + a[6], b[0] + b[6]) - c0 - c6;
    s07 = fqmul(a[0] + a[7], b[0] + b[7]) - c0 - c7;

    s12 = fqmul(a[1] + a[2], b[1] + b[2]) - c1 - c2;
    s13 = fqmul(a[1] + a[3], b[1] + b[3]) - c1 - c3;
    s14 = fqmul(a[1] + a[4], b[1] + b[4]) - c1 - c4;
    s15 = fqmul(a[1] + a[5], b[1] + b[5]) - c1 - c5;
    s16 = fqmul(a[1] + a[6], b[1] + b[6]) - c1 - c6;
    s17 = fqmul(a[1] + a[7], b[1] + b[7]) - c1 - c7;

    s23 = fqmul(a[2] + a[3], b[2] + b[3]) - c2 - c3;
    s24 = fqmul(a[2] + a[4], b[2] + b[4]) - c2 - c4;
    s25 = fqmul(a[2] + a[5], b[2] + b[5]) - c2 - c5;
    s26 = fqmul(a[2] + a[6], b[2] + b[6]) - c2 - c6;
    s27 = fqmul(a[2] + a[7], b[2] + b[7]) - c2 - c7;

    s34 = fqmul(a[3] + a[4], b[3] + b[4]) - c3 - c4;
    s35 = fqmul(a[3] + a[5], b[3] + b[5]) - c3 - c5;
    s36 = fqmul(a[3] + a[6], b[3] + b[6]) - c3 - c6;
    s37 = fqmul(a[3] + a[7], b[3] + b[7]) - c3 - c7;

    s45 = fqmul(a[4] + a[5], b[4] + b[5]) - c4 - c5;
    s46 = fqmul(a[4] + a[6], b[4] + b[6]) - c4 - c6;
    s47 = fqmul(a[4] + a[7], b[4] + b[7]) - c4 - c7;

    s56 = fqmul(a[5] + a[6], b[5] + b[6]) - c5 - c6;
    s57 = fqmul(a[5] + a[7], b[5] + b[7]) - c5 - c7;

    s67 = fqmul(a[6] + a[7], b[6] + b[7]) - c6 - c7;

    k  = s17;
    k += s26;
    k += s35;
    k += c4;
    r[0] = c0 + fqmul(k, zeta);
    r[0] = fqmul(r[0], 19);
    r[0] += (r[0] >> 15) & ZEN_Q;

    k  = s27;
    k += s36;
    k += s45;
    r[1] = s01 + fqmul(k, zeta);
    r[1] = fqmul(r[1], 19);
    r[1] += (r[1] >> 15) & ZEN_Q;

    k  = s37;
    k += s46;
    k += c5;
    r[2] = s02 + c1 + fqmul(k, zeta);
    r[2] = fqmul(r[2], 19);
    r[2] += (r[2] >> 15) & ZEN_Q;

    k  = s47;
    k += s56;
    r[3] = s03 + s12 + fqmul(k, zeta);
    r[3] = fqmul(r[3], 19);
    r[3] += (r[3] >> 15) & ZEN_Q;

    k  = s57;
    k += c6;
    r[4] = s04 + s13 + c2 + fqmul(k, zeta);
    r[4] = fqmul(r[4], 19);
    r[4] += (r[4] >> 15) & ZEN_Q;

    r[5] = s05 + s14 + s23 + fqmul(s67, zeta);
    r[5] = fqmul(r[5], 19);
    r[5] += (r[5] >> 15) & ZEN_Q;

    r[6] = s06 + s15 + s24 + c3 + fqmul(c7, zeta);
    r[6] = fqmul(r[6], 19);
    r[6] += (r[6] >> 15) & ZEN_Q;

    r[7] = s07 + s16 + s25 + s34;
    r[7] = fqmul(r[7], 19);
    r[7] += (r[7] >> 15) & ZEN_Q;
}

void poly_basemul_ntt_mq(int16_t *r,  int16_t *a,  int16_t *b)
{
    unsigned int i;
    for(i = 0; i < ZEN_N / 16; i++) 
    {
        base_mul_mq(r + 16 * i, a + 16 * i, b + 16 * i, f[64 + i]);
        base_mul_mq(r + 16 * i + 8, a + 16 * i + 8, b + 16 * i + 8, -f[64 + i]);
    }
}

/// @brief Compute the inverse of a degree-7 polynomial block in the NTT domain with a given twiddle factor
/// @param[out] r Base address of output coefficient array of length 8
/// @param[in] a Base address of input coefficient array of length 8
/// @param[in] zeta Twiddle factor associated with the block
/// @return None
static void base_inv(int16_t *r, int16_t *a, int16_t zeta)
{
    int16_t a0, a1, a2, a3, a4, a5, a6, a7;
    int16_t b0, b1, b2, b3;
    int16_t c0, c1;
    int16_t f0, f1, f2, f3;
    int16_t e, t;

    int16_t pe[7], po[7];
    int16_t p0, p1, p2, q0, q1, q2, m0, m1, m2;
    int16_t sx0, sx1, sy0, sy1;

    a0 = a[0]; a1 = a[1];
    a2 = a[2]; a3 = a[3];
    a4 = a[4]; a5 = a[5];
    a6 = a[6]; a7 = a[7];

    /*
     * ------------------------------------------------------------
     * b = (a0+a2*x+a4*x^2+a6*x^3)^2
     *   - x*(a1+a3*x+a5*x^2+a7*x^3)^2   mod (x^4 + zeta)
     *
     * Square-specific Karatsuba
     * ------------------------------------------------------------
     */

    /* pe = (a0 + a2*x + a4*x^2 + a6*x^3)^2 */
    p0 = fqmul(a0, a0);
    p1 = fqmul(fqmul(a0, a2), 342);
    p2 = fqmul(a2, a2);

    q0 = fqmul(a4, a4);
    q1 = fqmul(fqmul(a4, a6), 342);
    q2 = fqmul(a6, a6);

    sx0 = a0 + a4;
    sx1 = a2 + a6;
    m0  = fqmul(sx0, sx0);
    m1  = fqmul(fqmul(sx0, sx1), 342);
    m2  = fqmul(sx1, sx1);

    pe[0] = p0;
    pe[1] = p1;
    pe[2] = p2 + m0 - p0 - q0;
    pe[3] = m1 - p1 - q1;
    pe[4] = q0 + m2 - p2 - q2;
    pe[5] = q1;
    pe[6] = q2;

    /* po = (a1 + a3*x + a5*x^2 + a7*x^3)^2 */
    p0 = fqmul(a1, a1);
    p1 = fqmul(fqmul(a1, a3), 342);
    p2 = fqmul(a3, a3);

    q0 = fqmul(a5, a5);
    q1 = fqmul(fqmul(a5, a7), 342);
    q2 = fqmul(a7, a7);

    sx0 = a1 + a5;
    sx1 = a3 + a7;
    m0  = fqmul(sx0, sx0);
    m1  = fqmul(fqmul(sx0, sx1), 342);
    m2  = fqmul(sx1, sx1);

    po[0] = p0;
    po[1] = p1;
    po[2] = p2 + m0 - p0 - q0;
    po[3] = m1 - p1 - q1;
    po[4] = q0 + m2 - p2 - q2;
    po[5] = q1;
    po[6] = q2;

    b0 = pe[0] - fqmul(pe[4] - po[3], zeta);
    b1 = pe[1] - po[0] - fqmul(pe[5] - po[4], zeta);
    b2 = pe[2] - po[1] - fqmul(pe[6] - po[5], zeta);
    b3 = pe[3] - po[2] + fqmul(po[6], zeta);

    /*
     * ------------------------------------------------------------
     * c = (b0 + b2*x)^2 - x*(b1 + b3*x)^2   mod (x^2 + zeta)
     *
     * Square-specific Karatsuba
     * ------------------------------------------------------------
     */
    p0 = fqmul(b0, b0);
    p1 = fqmul(fqmul(b0, b2), 342);
    p2 = fqmul(b2, b2);

    q0 = fqmul(b1, b1);
    q1 = fqmul(fqmul(b1, b3), 342);
    q2 = fqmul(b3, b3);

    c0 = p0 - fqmul(p2 - q1, zeta);
    c1 = p1 - q0 + fqmul(q2, zeta);

    /*
     * ------------------------------------------------------------
     * e = 1 / (c0^2 + zeta*c1^2)
     * ------------------------------------------------------------
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
     * ------------------------------------------------------------
     * f = (c0 + c1*x^2) * (b0 - b1*x + b2*x^2 - b3*x^3) mod (x^4 + zeta)
     *
     * Two 2x2 Karatsuba multiplications
     * ------------------------------------------------------------
     */

    /* pe = (c0 + c1*x) * (b0 + b2*x) */
    p0 = fqmul(c0, b0);
    p2 = fqmul(c1, b2);
    p1 = fqmul(c0 + c1, b0 + b2) - p0 - p2;

    /* po = (c0 + c1*x) * (b1 + b3*x) */
    q0 = fqmul(c0, b1);
    q2 = fqmul(c1, b3);
    q1 = fqmul(c0 + c1, b1 + b3) - q0 - q2;

    f0 = p0 - fqmul(p2, zeta);
    f1 = fqmul(q2, zeta) - q0;
    f2 = p1;
    f3 = fqmul(q1, -171);

    /*
     * ------------------------------------------------------------
     * r = (f0 + f1*x^2 + f2*x^4 + f3*x^6)
     *   * (a0 - a1*x + a2*x^2 - a3*x^3 + a4*x^4 - a5*x^5 + a6*x^6 - a7*x^7)
     *   mod (x^8 + zeta)
     *
     * Two 4x4 Karatsuba multiplications
     * ------------------------------------------------------------
     */

    /* pe = (f0 + f1*x + f2*x^2 + f3*x^3) * (a0 + a2*x + a4*x^2 + a6*x^3) */

    p0 = fqmul(f0, a0);
    p2 = fqmul(f1, a2);
    p1 = fqmul(f0 + f1, a0 + a2) - p0 - p2;

    q0 = fqmul(f2, a4);
    q2 = fqmul(f3, a6);
    q1 = fqmul(f2 + f3, a4 + a6) - q0 - q2;

    sx0 = f0 + f2;
    sx1 = f1 + f3;
    sy0 = a0 + a4;
    sy1 = a2 + a6;
    m0  = fqmul(sx0, sy0);
    m2  = fqmul(sx1, sy1);
    m1  = fqmul(sx0 + sx1, sy0 + sy1) - m0 - m2;

    pe[0] = p0;
    pe[1] = p1;
    pe[2] = p2 + m0 - p0 - q0;
    pe[3] = m1 - p1 - q1;
    pe[4] = q0 + m2 - p2 - q2;
    pe[5] = q1;
    pe[6] = q2;

    /* po = (f0 + f1*x + f2*x^2 + f3*x^3) * (a1 + a3*x + a5*x^2 + a7*x^3) */

    p0 = fqmul(f0, a1);
    p2 = fqmul(f1, a3);
    p1 = fqmul(f0 + f1, a1 + a3) - p0 - p2;

    q0 = fqmul(f2, a5);
    q2 = fqmul(f3, a7);
    q1 = fqmul(f2 + f3, a5 + a7) - q0 - q2;

    sx0 = f0 + f2;
    sx1 = f1 + f3;
    sy0 = a1 + a5;
    sy1 = a3 + a7;
    m0  = fqmul(sx0, sy0);
    m2  = fqmul(sx1, sy1);
    m1  = fqmul(sx0 + sx1, sy0 + sy1) - m0 - m2;

    po[0] = p0;
    po[1] = p1;
    po[2] = p2 + m0 - p0 - q0;
    po[3] = m1 - p1 - q1;
    po[4] = q0 + m2 - p2 - q2;
    po[5] = q1;
    po[6] = q2;

    r[0] = pe[0] - fqmul(pe[4], zeta);
    r[1] = fqmul(po[4], zeta) - po[0];
    r[2] = pe[1] - fqmul(pe[5], zeta);
    r[3] = fqmul(po[5], zeta) - po[1];
    r[4] = pe[2] - fqmul(pe[6], zeta);
    r[5] = fqmul(po[6], zeta) - po[2];
    r[6] = pe[3];
    r[7] = fqmul(po[3], -171);
}

void poly_baseinv_ntt(int16_t *r, int16_t *a)
{
    unsigned int i;
    for(i = 0; i < ZEN_N / 16; i++) 
    {
        base_inv(r + 16 * i, a + 16 * i, -f[64 + i]);
        base_inv(r + 16 * i + 8, a + 16 * i + 8, f[64 + i]);
    }
}
