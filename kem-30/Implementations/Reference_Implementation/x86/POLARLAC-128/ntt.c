/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Implements NTT arithmetic routines for the reference POLARLAC-128 instance.
*/

#include <stdint.h>
#include <stdio.h>
#include "params.h"
#include "ntt.h"

static const int16_t f[128] = {
1,241,64,4,249,128,2,225,136,137,223,30,197,189,15,17,81,246,44,67,123,88,162,235,222,46,73,117,23,146,187,92,9,113,62,36,185,124,18,226,196,205,208,13,231,159,135,153,215,158,139,89,79,21,173,59,199,157,143,25,207,29,141,57,3,209,192,12,233,127,6,161,151,154,155,90,77,53,45,51,243,224,132,201,112,7,229,191,152,138,219,94,69,181,47,19,27,82,186,108,41,115,54,164,74,101,110,39,179,220,148,202,131,217,160,10,237,63,5,177,83,214,172,75,107,87,166,171
};

static const int16_t fn[128] = {
86,91,170,150,182,85,43,174,80,252,194,20,247,97,40,126,55,109,37,78,218,147,156,183,93,203,142,216,149,71,175,230,238,210,76,188,163,38,119,105,66,28,250,145,56,125,33,14,206,212,204,180,167,102,103,106,96,251,130,24,245,65,48,254,200,116,228,50,232,114,100,58,198,84,236,178,168,118,99,42,104,122,98,26,244,49,52,61,31,239,133,72,221,195,144,248,165,70,111,234,140,184,211,35,22,95,169,134,190,213,11,176,240,242,68,60,227,34,120,121,32,255,129,8,253,193,16,255
};

static const int16_t con_ntt_f_769[RL_KEM_N] = {
171,605,688,361,186,766,519,649,461,129,753,546,407,626,131,432,
693,671,36,694,430,514,282,566,735,199,178,270,759,149,369,577,
147,655,497,54,767,645,689,423,86,718,364,267,161,754,288,169,
191,307,719,745,599,226,121,581,389,279,180,394,612,263,641,523,
746,112,618,635,717,621,227,232,698,212,236,21,341,379,567,549,
352,292,238,145,194,493,70,495,117,333,66,247,532,686,517,525,
331,528,167,357,414,291,411,105,654,560,14,99,509,29,366,391,
451,278,353,354,585,127,330,466,222,691,421,725,201,158,350,168
};

static const int16_t con_ntt_fn_769[RL_KEM_N] = {
601,419,611,568,44,348,78,547,303,439,642,184,415,416,491,318,
378,403,740,260,670,755,209,115,664,358,478,355,412,602,241,438,
244,252,83,237,522,703,436,652,274,699,276,575,624,531,477,417,
220,202,390,428,748,533,557,71,537,542,148,52,134,151,657,23,
246,128,506,157,375,589,490,380,188,648,543,170,24,50,462,578,
600,481,15,608,502,405,51,683,346,80,124,2,715,272,114,622,
192,400,620,10,499,591,570,34,203,487,255,339,75,733,98,76,
337,638,143,362,223,16,640,308,120,250,3,583,408,81,164,655
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


void mq_poly_ntt(int16_t *a) 
{
  unsigned int len, start, j, k;
  int16_t t, zeta;

  k = 1;
  for(len = RL_KEM_N >> 1; len >= 2; len >>= 1) 
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
  for(len = 2; len <= RL_KEM_N >> 1; len <<= 1)
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
  }
}

void base_mul(int16_t *r, int16_t *a, int16_t *b, int16_t zeta)
{
  int c0,  c1;
  c0 = fqmul(a[0], b[0]);
  c1 = fqmul(a[1], b[1]);


  r[0] = fqmul(c1, zeta);
  r[0] += c0;

  r[1] = fqmul(a[1], b[0]);
  r[1] += fqmul(a[0], b[1]);

}

/**
 * @brief Multiply polynomials in blocks (pointwise) in the NTT domain.
 * @param[out] r Result array (length RL_KEM_N).
 * @param[in]  a First operand (length RL_KEM_N).
 * @param[in]  b Second operand (length RL_KEM_N).
 */

void mq_poly_pointwise_mul(int16_t *r,  int16_t *a,  int16_t *b)
{
    int i;
    for(i = 0; i < RL_KEM_N / 4; i++) 
    {
        base_mul(r + 4 * i, a + 4 * i, b + 4 * i, f[64 + i]);
        base_mul(r + 4 * i + 2, a + 4 * i + 2, b + 4 * i + 2, -f[64 + i]);
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
    for (len = RL_KEM_N >> 1; len >= 2; len >>= 1) {
        for (start = 0; start < RL_KEM_N; start = j + len) {
            zeta = con_ntt_f_769[k++];
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
    for (len = 2; len <= RL_KEM_N >> 1; len <<= 1) {
        for (start = 0; start < RL_KEM_N; start = j + len) {
            zeta = con_ntt_fn_769[k++];
            for (j = start; j < start + len; j++) {
                t = a[j];
                a[j] = t + a[j + len];
                a[j + len] = t - a[j + len];
                a[j + len] = con_fqmul_769(zeta, a[j + len]);
            }
        }
    }

    for (j = 0; j < RL_KEM_N; j++) {
        a[j] = con_fqmul_769(a[j], con_ntt_fn_769[127]);
    }
}

static void con_basemul_769(int16_t *r, const int16_t *a, const int16_t *b, int16_t zeta)
{
    int c0, c1;

    c0 = con_fqmul_769(a[0], b[0]);
    c1 = con_fqmul_769(a[1], b[1]);
    
    r[0] = con_fqmul_769(c1, zeta);
    r[0] += c0;

    r[1] = con_fqmul_769(a[1], b[0]);
    r[1] += con_fqmul_769(a[0], b[1]);
}

void con_poly_mul_ntt(int16_t *r, const int16_t *a, const int16_t *b)
{
    for (int i = 0; i < RL_KEM_N / 4; i++) {
        con_basemul_769(r + 4 * i, a + 4 * i, b + 4 * i, con_ntt_f_769[64 + i]);
        con_basemul_769(r + 4 * i + 2, a + 4 * i + 2, b + 4 * i + 2,
            (int16_t)-con_ntt_f_769[64 + i]);
    }
}

static void con_base_adjoint_769(int16_t *r, const int16_t *a, int16_t zeta)
{
    r[0] = a[0];
    r[1] = con_fqmul_769(a[1], zeta);
}

void con_poly_adjoint_ntt(int16_t *r, const int16_t *a)
{
    for (int i = 0; i < RL_KEM_N / 4; i++) {
        con_base_adjoint_769(r + RL_KEM_N - 4 * (i + 1) + 2,
            a + 4 * i, con_ntt_f_769[64 + i]);
        con_base_adjoint_769(r + RL_KEM_N - 4 * (i + 1),
            a + 4 * i + 2, (int16_t)-con_ntt_f_769[64 + i]);
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
