/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements NTT arithmetic routines for the reference POLARLAC-512-Star instance.
*/

#include <stdint.h>
#include <stdio.h>
#include "params.h"
#include "ntt.h"

static const int16_t f[128] = {
171,605,688,361,186,766,519,649,461,129,753,546,407,626,131,432,693,671,36,694,430,514,282,566,735,199,178,270,759,149,369,577,147,655,497,54,767,645,689,423,86,718,364,267,161,754,288,169,191,307,719,745,599,226,121,581,389,279,180,394,612,263,641,523,746,112,618,635,717,621,227,232,698,212,236,21,341,379,567,549,352,292,238,145,194,493,70,495,117,333,66,247,532,686,517,525,331,528,167,357,414,291,411,105,654,560,14,99,509,29,366,391,451,278,353,354,585,127,330,466,222,691,421,725,201,158,350,168
};

static const int16_t fn[128] = {
601,419,611,568,44,348,78,547,303,439,642,184,415,416,491,318,378,403,740,260,670,755,209,115,664,358,478,355,412,602,241,438,244,252,83,237,522,703,436,652,274,699,276,575,624,531,477,417,220,202,390,428,748,533,557,71,537,542,148,52,134,151,657,23,246,128,506,157,375,589,490,380,188,648,543,170,24,50,462,578,600,481,15,608,502,405,51,683,346,80,124,2,715,272,114,622,192,400,620,10,499,591,570,34,203,487,255,339,75,733,98,76,337,638,143,362,223,16,640,308,120,250,3,583,408,81,164,655
};


#define CONJ_NTT_QINV (-767)

/**
 * @file ntt.c
 * @brief Implementations of NTT-related routines used by the RL-KEM
 *        reference implementation.
 *
 * Functions include Montgomery reduction, forward/inverse NTT transforms,
 * pointwise multiplication, and small-block base multiplications and
 * inverses. Comments follow the @brief/@param style used elsewhere in the
 * project (see KEM_AlgorithmInstance.h).
 */

/**
 * @brief Montgomery reduction: reduce a 32-bit integer modulo RL_KEM_Q using
 *        Montgomery constant QINV. Result is a 16-bit representative.
 * @param[in] a Integer to be reduced (int32_t).
 * @return Reduced value (int16_t) congruent to a * R^{-1} mod RL_KEM_Q where R=2^16.
 */
static inline int16_t montgomery_reduce(int32_t a)
{
    int32_t t;
    int16_t u;

    u = a * QINV;
    t = (int32_t)u * RL_KEM_Q;
    t = a - t;
    t >>= 16;
    return t;
}

/**
 * @brief Fixed-modulus multiplication in Montgomery domain.
 * @param[in] a Operand a (int16_t).
 * @param[in] b Operand b (int16_t).
 * @return montgomery_reduce(a * b).
 */
static inline int16_t fqmul(int16_t a, int16_t b)
{
    return montgomery_reduce((int32_t)a * b);
}



/**
 * @brief Forward NTT (in-place) without final normalization.
 * @param[in,out] a Polynomial coefficients array (length RL_KEM_N).
 */

void mq_poly_ntt(int16_t *a) 
{
  unsigned int len, start, j, k;
  int16_t t, zeta;

  k = 1;
  for(len = RL_KEM_N >> 1; len >= 8; len >>= 1) 
  {
    for(start = 0; start < RL_KEM_N; start = j + len) 
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

//   for(j = 0; j < RL_KEM_N; j++)
//   {
//     a[j] = fqmul(a[j], 171);
//     a[j] += (a[j] >> 15) & RL_KEM_Q;
//   }

}



/**
 * @brief Inverse NTT (in-place). After this call, coefficients are
 *        transformed back into the standard coefficient domain.
 * @param[in,out] a Polynomial coefficients array (length RL_KEM_N).
 */

void mq_poly_intt(int16_t *a) 
{
  unsigned int start, len, j, k;
  int16_t t, zeta;

  k = 0;
  for(len = 8; len <= RL_KEM_N >> 1; len <<= 1)
  {
    for(start = 0; start < RL_KEM_N; start =j + len)
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

  for(j = 0; j < RL_KEM_N; j++)
  {
    a[j] = fqmul(a[j], fn[127]);
    // a[j] += (a[j] >> 15) & RL_KEM_Q;
  }
}


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

void mq_poly_pointwise_mul(int16_t *r, int16_t *a, int16_t *b)
{
    int i;
    for(i = 0; i < RL_KEM_N / 16; i++) 
    {
        base_mul(r + 16 * i, a + 16 * i, b + 16 * i, f[64 + i]);
        base_mul(r + 16 * i + 8, a + 16 * i + 8, b + 16 * i + 8, -f[64 + i]);
    }
}

static inline int16_t con_montgomery_reduce_769(int32_t a)
{
    int32_t t;
    int16_t u;

    u = (int16_t)(a * CONJ_NTT_QINV);
    t = a - (int32_t)u * CONJ_NTT_Q;
    return (int16_t)(t >> 16);
}

static inline int16_t con_fqmul_769(int16_t a, int16_t b)
{
    return con_montgomery_reduce_769((int32_t)a * b);
}

void con_poly_ntt(int16_t *a)
{
    unsigned int len, start, j, k;
    int16_t t, zeta;

    k = 1;
    for (len = RL_KEM_N >> 1; len >= 8; len >>= 1) {
        for (start = 0; start < RL_KEM_N; start = j + len) {
            zeta = f[k++];
            for (j = start; j < start + len; j++) {
                t = con_fqmul_769(zeta, a[j + len]);
                a[j + len] = a[j] - t;
                a[j] = a[j] + t;
            }
        }
    }
}

void con_poly_intt(int16_t *a)
{
    unsigned int start, len, j, k;
    int16_t t, zeta;

    k = 0;
    for (len = 8; len <= RL_KEM_N >> 1; len <<= 1) {
        for (start = 0; start < RL_KEM_N; start = j + len) {
            zeta = fn[k++];
            for (j = start; j < start + len; j++) {
                t = a[j];
                a[j] = t + a[j + len];
                a[j + len] = t - a[j + len];
                a[j + len] = con_fqmul_769(zeta, a[j + len]);
            }
        }
    }

    for (j = 0; j < RL_KEM_N; j++) {
        a[j] = con_fqmul_769(a[j], fn[127]);
    }
}

static void con_basemul_769(int16_t *r, const int16_t *a, const int16_t *b, int16_t zeta)
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

    c0 = con_fqmul_769(a[0], b[0]);
    c1 = con_fqmul_769(a[1], b[1]);
    c2 = con_fqmul_769(a[2], b[2]);
    c3 = con_fqmul_769(a[3], b[3]);
    c4 = con_fqmul_769(a[4], b[4]);
    c5 = con_fqmul_769(a[5], b[5]);
    c6 = con_fqmul_769(a[6], b[6]);
    c7 = con_fqmul_769(a[7], b[7]);

    s01 = con_fqmul_769(a[0] + a[1], b[0] + b[1]) - c0 - c1;
    s02 = con_fqmul_769(a[0] + a[2], b[0] + b[2]) - c0 - c2;
    s03 = con_fqmul_769(a[0] + a[3], b[0] + b[3]) - c0 - c3;
    s04 = con_fqmul_769(a[0] + a[4], b[0] + b[4]) - c0 - c4;
    s05 = con_fqmul_769(a[0] + a[5], b[0] + b[5]) - c0 - c5;
    s06 = con_fqmul_769(a[0] + a[6], b[0] + b[6]) - c0 - c6;
    s07 = con_fqmul_769(a[0] + a[7], b[0] + b[7]) - c0 - c7;

    s12 = con_fqmul_769(a[1] + a[2], b[1] + b[2]) - c1 - c2;
    s13 = con_fqmul_769(a[1] + a[3], b[1] + b[3]) - c1 - c3;
    s14 = con_fqmul_769(a[1] + a[4], b[1] + b[4]) - c1 - c4;
    s15 = con_fqmul_769(a[1] + a[5], b[1] + b[5]) - c1 - c5;
    s16 = con_fqmul_769(a[1] + a[6], b[1] + b[6]) - c1 - c6;
    s17 = con_fqmul_769(a[1] + a[7], b[1] + b[7]) - c1 - c7;

    s23 = con_fqmul_769(a[2] + a[3], b[2] + b[3]) - c2 - c3;
    s24 = con_fqmul_769(a[2] + a[4], b[2] + b[4]) - c2 - c4;
    s25 = con_fqmul_769(a[2] + a[5], b[2] + b[5]) - c2 - c5;
    s26 = con_fqmul_769(a[2] + a[6], b[2] + b[6]) - c2 - c6;
    s27 = con_fqmul_769(a[2] + a[7], b[2] + b[7]) - c2 - c7;

    s34 = con_fqmul_769(a[3] + a[4], b[3] + b[4]) - c3 - c4;
    s35 = con_fqmul_769(a[3] + a[5], b[3] + b[5]) - c3 - c5;
    s36 = con_fqmul_769(a[3] + a[6], b[3] + b[6]) - c3 - c6;
    s37 = con_fqmul_769(a[3] + a[7], b[3] + b[7]) - c3 - c7;

    s45 = con_fqmul_769(a[4] + a[5], b[4] + b[5]) - c4 - c5;
    s46 = con_fqmul_769(a[4] + a[6], b[4] + b[6]) - c4 - c6;
    s47 = con_fqmul_769(a[4] + a[7], b[4] + b[7]) - c4 - c7;

    s56 = con_fqmul_769(a[5] + a[6], b[5] + b[6]) - c5 - c6;
    s57 = con_fqmul_769(a[5] + a[7], b[5] + b[7]) - c5 - c7;
    s67 = con_fqmul_769(a[6] + a[7], b[6] + b[7]) - c6 - c7;

    k = s17;
    k += s26;
    k += s35;
    k += c4;
    r[0] = c0 + con_fqmul_769(k, zeta);

    k = s27;
    k += s36;
    k += s45;
    r[1] = s01 + con_fqmul_769(k, zeta);

    k = s37;
    k += s46;
    k += c5;
    r[2] = s02 + c1 + con_fqmul_769(k, zeta);

    k = s47;
    k += s56;
    r[3] = s03 + s12 + con_fqmul_769(k, zeta);

    k = s57;
    k += c6;
    r[4] = s04 + s13 + c2 + con_fqmul_769(k, zeta);

    r[5] = s05 + s14 + s23 + con_fqmul_769(s67, zeta);
    r[6] = s06 + s15 + s24 + c3 + con_fqmul_769(c7, zeta);
    r[7] = s07 + s16 + s25 + s34;
}

void con_poly_mul_ntt(int16_t *r, const int16_t *a, const int16_t *b)
{
    for (int i = 0; i < RL_KEM_N / 16; i++) {
        con_basemul_769(r + 16 * i, a + 16 * i, b + 16 * i, f[64 + i]);
        con_basemul_769(r + 16 * i + 8, a + 16 * i + 8, b + 16 * i + 8,
            (int16_t)-f[64 + i]);
    }
}

static void con_base_adjoint_769(int16_t *r, const int16_t *a, int16_t zeta)
{
    r[0] = a[0];
    r[1] = con_fqmul_769(a[7], zeta);
    r[2] = con_fqmul_769(a[6], zeta);
    r[3] = con_fqmul_769(a[5], zeta);
    r[4] = con_fqmul_769(a[4], zeta);
    r[5] = con_fqmul_769(a[3], zeta);
    r[6] = con_fqmul_769(a[2], zeta);
    r[7] = con_fqmul_769(a[1], zeta);
}

void con_poly_adjoint_ntt(int16_t *r, const int16_t *a)
{
    for (int i = 0; i < RL_KEM_N / 16; i++) {
        con_base_adjoint_769(r + RL_KEM_N - 16 * (i + 1) + 8,
            a + 16 * i, f[64 + i]);
        con_base_adjoint_769(r + RL_KEM_N - 16 * (i + 1),
            a + 16 * i + 8, (int16_t)- f[64 + i]);
    }
}

static inline int32_t con_abs_i16(int16_t x)
{
    return x < 0 ? -(int32_t)x : (int32_t)x;
}

int32_t con_poly_rejection_score(const int16_t *a)
{
    int16_t a_ntt[RL_KEM_N];
    int16_t con_a[RL_KEM_N];
    int16_t mul_b[RL_KEM_N];
    int32_t acc;

    for (int i = 0; i < RL_KEM_N; i++) {
        a_ntt[i] = a[i];
    }

    con_poly_ntt(a_ntt);
    con_poly_adjoint_ntt(con_a, a_ntt);
    con_poly_mul_ntt(mul_b, con_a, a_ntt);
    con_poly_intt(mul_b);

    acc = con_abs_i16(mul_b[0]) + con_abs_i16(mul_b[RL_KEM_N / 2]);
    for (int i = 1; i < RL_KEM_N / 2; i++) {
        acc += 2 * con_abs_i16(mul_b[i]);
    }

    return acc;
}

int con_poly_within_bound(const int16_t *a, int32_t bound)
{
    return con_poly_rejection_score(a) <= bound;
}
