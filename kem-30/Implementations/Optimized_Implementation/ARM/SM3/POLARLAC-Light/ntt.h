/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares NTT arithmetic routines and constants for the optimized POLARLAC-Light instance.
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
1,241,64,4,249,128,2,225,136,137,223,30,197,189,15,17,81,246,44,67,123,88,162,235,222,46,73,117,23,146,187,92,9,113,62,36,185,124,18,226,196,205,208,13,231,159,135,153,215,158,139,89,79,21,173,59,199,157,143,25,207,29,141,57,3,209,192,12,233,127,6,161,151,154,155,90,77,53,45,51,243,224,132,201,112,7,229,191,152,138,219,94,69,181,47,19,27,82,186,108,41,115,54,164,74,101,110,39,179,220,148,202,131,217,160,10,237,63,5,177,83,214,172,75,107,87,166,171};

// inverse root powers used by inverse NTT (length RL_KEM_N)
/**
 * @brief Inverse root powers used by inverse NTT (length RL_KEM_N).
 */
static int16_t fn[RL_KEM_N] POLARLAC_NTT_MAYBE_UNUSED ={
86,91,170,150,182,85,43,174,80,252,194,20,247,97,40,126,55,109,37,78,218,147,156,183,93,203,142,216,149,71,175,230,238,210,76,188,163,38,119,105,66,28,250,145,56,125,33,14,206,212,204,180,167,102,103,106,96,251,130,24,245,65,48,254,200,116,228,50,232,114,100,58,198,84,236,178,168,118,99,42,104,122,98,26,244,49,52,61,31,239,133,72,221,195,144,248,165,70,111,234,140,184,211,35,22,95,169,134,190,213,11,176,240,242,68,60,227,34,120,121,32,255,129,8,253,193,16,255};
#define QINV -255

/// @brief Montgomery reduction modulo RL_KEM_Q using R = 2^16.
/// @details Since 2^16 = 1 (mod 257), this reduction can be used directly as
///          a standard modular reduction without any extra domain conversion.
/// @param[in] a Integer to be reduced.
/// @return A signed representative congruent to a modulo RL_KEM_Q.
static inline int16_t rl_kem_montgomery_reduce(int32_t a)
{
    /* Defined low-half Montgomery arithmetic; no signed overflow or
       implementation-defined negative right shift. */
    const uint32_t low = (uint32_t)a * (uint32_t)(uint16_t)QINV;
    const uint32_t u_bits = low & UINT32_C(0xffff);
    const int32_t u = (int32_t)(u_bits ^ UINT32_C(0x8000)) - INT32_C(0x8000);
    const int64_t t = (int64_t)a - (int64_t)u * (int64_t)RL_KEM_Q;
    const uint32_t q_bits = (uint32_t)(((uint64_t)t >> 16) & UINT64_C(0xffff));
    const int32_t quotient = (int32_t)(q_bits ^ UINT32_C(0x8000)) - INT32_C(0x8000);
    return (int16_t)quotient;
}

/// @brief Map a bounded arithmetic intermediate to [0, RL_KEM_Q).
static inline int16_t rl_kem_mod_q(int32_t a)
{
    int32_t t = (int32_t)rl_kem_montgomery_reduce(a);
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
/// @param[in] a First operand in conjugate NTT domain.
/// @param[in] b Second operand in conjugate NTT domain.
void con_poly_mul_ntt(int16_t *r, const int16_t *a, const int16_t *b);

/// @brief Compute the conjugate-adjoint value in the conjugate NTT domain.
/// @param[out] r Output array of length RL_KEM_N.
/// @param[in] a Input array in conjugate NTT domain.
void con_poly_adjoint_ntt(int16_t *r, const int16_t *a);

/// @brief Check the conjugate-NTT rejection bound for a coefficient-domain polynomial.
/// @param[in] a Input coefficient-domain polynomial.
/// @param[in] bound Maximum allowed folded absolute-value sum.
/// @return 1 if within the bound; otherwise 0.
int con_poly_within_bound(const int16_t *a, int32_t bound);

/// @brief Compute the folded absolute-value score used by conjugate-NTT rejection.
/// @param[in] a Input coefficient-domain polynomial.
/// @return Folded score using direct absolute values of inverse conjugate-NTT coefficients.
int32_t con_poly_rejection_score(const int16_t *a);

#endif
