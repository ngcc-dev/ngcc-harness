/*
Copyright (c) 2026 Yu Zhang.
Organization: State Key Laboratory of Cyberspace Security Defense,Institute of Information Engineering, CAS
              School of Cyber Security, University of Chinese Academy of Sciences  
File Description: Declares NTT arithmetic routines and constants for the reference POLARLAC-256 instance.
*/

#ifndef NTT_H
#define NTT_H

#include <stdint.h>
#include "params.h"
/**
 * @file ntt.h
 * @brief Number Theoretic Transform (NTT) function prototypes and inline
 *        reduction helpers used by the RL-KEM reference implementation.
 *
 * Notes (consistent with README.txt):
 * - Implementations assume a little-endian byte order.
 * - The code is written for C99 or later.
 * - Root-power tables are defined once in ntt.c.
 */

/* Root-power tables are private to ntt.c. */

/// @brief Montgomery reduction modulo RL_KEM_Q using R = 2^16.
/// @details Since 2^16 = 1 (mod 257), this reduction can be used directly as
///          a standard modular reduction without any extra domain conversion.
/// @param[in] a Integer to be reduced.
/// @return A signed representative congruent to a modulo RL_KEM_Q.
static inline int16_t rl_kem_montgomery_reduce(int32_t a)
{
    int32_t t;
    int16_t u;

    u = (int16_t)(a * QINV);
    t = a - (int32_t)u * RL_KEM_Q;
    return (int16_t)(t >> 16);
}

/// @brief Map an integer to the canonical interval [0, RL_KEM_Q).
/// @param[in] a Integer to be reduced modulo RL_KEM_Q.
/// @return Canonical representative of a modulo RL_KEM_Q.
static inline int16_t rl_kem_mod_q(int32_t a)
{
    int16_t t = rl_kem_montgomery_reduce(a);

    t += (int16_t)((t >> 15) & RL_KEM_Q);
    return t;
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
