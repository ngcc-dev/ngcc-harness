/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares NTT arithmetic routines and constants for the optimized POLARLAC-512 instance.
*/

#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "params.h"

#if defined(__GNUC__)
#define POLARLAC_NTT_MAYBE_UNUSED __attribute__((unused))
#else
#define POLARLAC_NTT_MAYBE_UNUSED
#endif
/**
 * @file ntt.h
 * @brief Number Theoretic Transform (NTT) table declarations and function
 *        prototypes used by the selected MLWE ARM arithmetic backend.
 *
 * Notes (consistent with README.txt):
 * - Implementations assume a little-endian byte order.
 * - The code is written for C99 or later.
 * - These arrays are defined as `static` in the header for compilation
 *   compatibility with the supplied implementation. Do not duplicate/include
 *   this header from multiple translation units that would cause duplicate
 *   definitions in a production build.
 */

// Root powers used by forward NTT (length RL_KEM_N). See implementation for
// interpretation of these constants.
static int16_t f[RL_KEM_N] POLARLAC_NTT_MAYBE_UNUSED ={
171,605,688,361,186,766,519,649,461,129,753,546,407,626,131,432,693,671,36,694,430,514,282,566,735,199,178,270,759,149,369,577,147,655,497,54,767,645,689,423,86,718,364,267,161,754,288,169,191,307,719,745,599,226,121,581,389,279,180,394,612,263,641,523,746,112,618,635,717,621,227,232,698,212,236,21,341,379,567,549,352,292,238,145,194,493,70,495,117,333,66,247,532,686,517,525,331,528,167,357,414,291,411,105,654,560,14,99,509,29,366,391,451,278,353,354,585,127,330,466,222,691,421,725,201,158,350,168};

// inverse root powers used by inverse NTT (length RL_KEM_N)
/**
 * @brief Inverse root powers used by inverse NTT (length RL_KEM_N).
 */
static int16_t fn[RL_KEM_N] POLARLAC_NTT_MAYBE_UNUSED ={
601,419,611,568,44,348,78,547,303,439,642,184,415,416,491,318,378,403,740,260,670,755,209,115,664,358,478,355,412,602,241,438,244,252,83,237,522,703,436,652,274,699,276,575,624,531,477,417,220,202,390,428,748,533,557,71,537,542,148,52,134,151,657,23,246,128,506,157,375,589,490,380,188,648,543,170,24,50,462,578,600,481,15,608,502,405,51,683,346,80,124,2,715,272,114,622,192,400,620,10,499,591,570,34,203,487,255,339,75,733,98,76,337,638,143,362,223,16,640,308,120,250,3,583,408,81,164,655};
#define QINV -767

/// @brief Montgomery reduction modulo RL_KEM_Q using R = 2^16.
/// @details For q = 769, R = 2^16 = 171 (mod q), so this returns
///          `a * R^{-1} mod q`. Use `rl_kem_mod_q` for canonical reduction.
/// @param[in] a Integer to be reduced.
/// @return A signed Montgomery-reduced representative modulo RL_KEM_Q.
static inline int16_t rl_kem_montgomery_reduce_i64(int64_t a)
{
    /* Defined low-half Montgomery arithmetic for q=769. */
    const uint64_t low = (uint64_t)a * (uint64_t)(uint16_t)QINV;
    const uint32_t u_bits = (uint32_t)low & UINT32_C(0xffff);
    const int32_t u = (int32_t)(u_bits ^ UINT32_C(0x8000)) - INT32_C(0x8000);
    const int64_t t = a - (int64_t)u * (int64_t)RL_KEM_Q;
    const uint32_t q_bits = (uint32_t)(((uint64_t)t >> 16) & UINT64_C(0xffff));
    const int32_t quotient = (int32_t)(q_bits ^ UINT32_C(0x8000)) - INT32_C(0x8000);
    return (int16_t)quotient;
}

static inline int16_t rl_kem_montgomery_reduce(int32_t a)
{
    return rl_kem_montgomery_reduce_i64((int64_t)a);
}

/// @brief Map a bounded arithmetic intermediate to [0, RL_KEM_Q).
static inline int16_t rl_kem_mod_q(int32_t a)
{
    int32_t t = (int32_t)rl_kem_montgomery_reduce_i64((int64_t)a * INT64_C(171));
    t += (int32_t)(((uint32_t)t >> 31) * (uint32_t)RL_KEM_Q);
    t -= RL_KEM_Q;
    t += (int32_t)(((uint32_t)t >> 31) * (uint32_t)RL_KEM_Q);
    return (int16_t)t;
}


/// @brief Forward Number Theoretic Transform (NTT) in-place.
/// @param[in,out] a Polynomial coefficient array of length RL_KEM_N (int16_t).
void mq_poly_ntt(int16_t *a);



/// @brief Inverse NTT in-place.
/// @param[in,out] a Polynomial coefficient array of length RL_KEM_N (int16_t).
void mq_poly_intt(int16_t *a);

/// @brief Pointwise multiplication in NTT domain (standard variant).
/// @param[out] r Result array of length RL_KEM_N.
/// @param[in]  a First operand (NTT domain), length RL_KEM_N.
/// @param[in]  b Second operand (NTT domain), length RL_KEM_N.
void mq_poly_pointwise_mul(int16_t *r, int16_t *a, int16_t *b);

/// Compute a0*b0 + a1*b1 directly in the incomplete-NTT domain.
void mq_poly_pointwise_mulacc2(int16_t *r, const int16_t *a0, const int16_t *b0,
                               const int16_t *a1, const int16_t *b1);

#ifdef POLARLAC_ARITH_TEST_HOOKS
void polarlac_test_ntt_stage(int16_t *a, unsigned int len, unsigned int *k);
void polarlac_test_intt_stage(int16_t *a, unsigned int len, unsigned int *k);
void polarlac_test_intt_final_scale(int16_t *a);
void polarlac_test_fqmul_vector(int16_t *out, const int16_t *x, const int16_t *y, unsigned int n);
int polarlac_test_backend_id(void);
#endif

/// @brief Forward conjugate NTT modulo CONJ_NTT_Q.
/// @param[in,out] a Polynomial coefficient array of length RL_KEM_N.
void con_poly_ntt(int16_t *a);

/// @brief Inverse conjugate NTT modulo CONJ_NTT_Q.
/// @param[in,out] a Polynomial coefficient array of length RL_KEM_N.
void con_poly_intt(int16_t *a);

/// @brief Pointwise multiplication in the conjugate NTT domain.
/// @param[out] r Result array of length RL_KEM_N.
/// @param[in]  a First operand, length RL_KEM_N.
/// @param[in]  b Second operand, length RL_KEM_N.
void con_poly_mul_ntt(int16_t *r, const int16_t *a, const int16_t *b);

/// @brief Compute the conjugate/adjoint polynomial in the conjugate NTT domain.
/// @param[out] r Result array of length RL_KEM_N.
/// @param[in]  a Input array of length RL_KEM_N.
void con_poly_adjoint_ntt(int16_t *r, const int16_t *a);

/// @brief Return whether the conjugate-NTT rejection score is within bound.
/// @param[in] a Input polynomial in coefficient domain.
/// @param[in] bound Inclusive threshold.
/// @return 1 when score <= bound, otherwise 0.
int con_poly_within_bound(const int16_t *a, int32_t bound);

/// @brief Compute the folded absolute-value score used by conjugate-NTT rejection.
/// @param[in] a Input polynomial in coefficient domain.
/// @return Sum of absolute signed inverse-transform coefficients.
int32_t con_poly_rejection_score(const int16_t *a);

#endif
