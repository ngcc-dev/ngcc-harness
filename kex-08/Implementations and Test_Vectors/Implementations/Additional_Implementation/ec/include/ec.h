// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from ec.h in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modifications made:
 *   - Deleted some unused functions
 *   - Added some functions
 */

/** @file
 *
 * @authors Luca De Feo, Francisco RH
 *
 * @brief Elliptic curve stuff
*/

#ifndef EC_H
#define EC_H

#include <fp2.h>
#include <ec_params.h>


/** @defgroup ec Elliptic curves
 * @{
*/

/** @defgroup ec_t Data structures
 * @{
*/

/** @brief Projective point
 *
 * @typedef ec_point_t
 *
 * @struct ec_point_t
 *
 * A projective point in (X:Z) or (X:Y:Z) coordinates (tbd).
*/
typedef struct ec_point_t {
    fp2_t x;
    fp2_t z;
} ec_point_t;

/** @brief A basis of a torsion subgroup
 *
 * @typedef ec_basis_t
 *
 * @struct ec_basis_t
 *
 * A pair of points (or a triplet, tbd) forming a basis of a torsion subgroup.
*/
typedef struct ec_basis_t {
    ec_point_t P;
    ec_point_t Q;
    ec_point_t PmQ;
} ec_basis_t;

/** @brief An elliptic curve
 *
 * @typedef ec_curve_t
 *
 * @struct ec_curve_t
 *
 * An elliptic curve in projective Montgomery form
*/
typedef struct ec_curve_t {
    fp2_t A;
    fp2_t C; ///< cannot be 0
} ec_curve_t;

/** @brief An isogeny of degree a power of 2
 *
 * @typedef ec_isog_even_t
 *
 * @struct ec_isog_even_t
*/
typedef struct ec_isog_even_t {
    ec_curve_t curve;      ///< The domain curve
    ec_point_t kernel;     ///< A kernel generator
    unsigned short length; ///< The length as a 2-isogeny walk
} ec_isog_even_t;


/** @brief An odd divisor of p² - 1
 *
 * @typedef ec_isog_odd_t
 *
 * Given that the list of divisors of p² - 1 is known, this is
 * represented as a fixed-length vector of integer exponents.
*/

typedef uint8_t ec_degree_odd_t[P_LEN + M_LEN];

/** @brief An isogeny of odd degree dividing p² - 1
 *
 * @typedef ec_isog_odd_t
 *
 * @struct ec_isog_odd_t
*/
typedef struct ec_isog_odd_t {
    ec_curve_t curve;
    ec_point_t ker_plus;    ///< A generator of E[p+1] ∩ ker(φ)
    ec_point_t ker_minus;   ///< A generator of E[p-1] ∩ ker(φ)
    ec_degree_odd_t degree; ///< The degree of the isogeny
} ec_isog_odd_t;

/** @brief Isomorphism of Montgomery curves
 *
 * @typedef ec_isom_t
 *
 * @struct ec_isom_t
 *
 * The isomorphism is given by the map maps (X:Z) ↦ ( (Nx X - Nz Z) : (D Z) )
*/
typedef struct ec_isom_t {
    fp2_t Nx;
    fp2_t Nz;
    fp2_t D;
} ec_isom_t;

// end ec_t
/** @}
*/


/** @defgroup ec_curve_t Curves and isomorphisms
 * @{
*/

/**
 * @brief j-invariant.
 *
 * @param j_inv computed j_invariant
 * @param curve input curve
 */
void ec_j_inv(fp2_t* j_inv, const ec_curve_t* curve);

/**
 * @brief Point multiplication by a scalar of limited length
 *
 * @param res computed scalar * P
 * @param curve the curve
 * @param scalar an unsigned multi-precision integer
 * @param kbits the bit size of scalar
 * @param P a point
 */
void xMULv2(ec_point_t* Q, ec_point_t const* P, digit_t const* k, const int kbits, ec_point_t const* A24);
void ec_ker_mul(fp2_t* pi_X, fp2_t* pi_Z, const ec_point_t* ker, const ec_point_t* A24, int ell);


#endif
