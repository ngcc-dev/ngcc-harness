/** @file
 *
 * @authors Luca De Feo, Francisco RH
 *
 * @brief Elliptic curve stuff
 */

#ifndef EC_H
#define EC_H

#include <fp2.h>
#include "ec_params.h"
#include <tools.h>
#include <stdio.h>

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
typedef struct ec_point_t
{
    fp2_t x;
    fp2_t z;
} ec_point_t;

/** @brief Projective point
 *
 * @typedef jac_point_t
 *
 * @struct jac_point_t
 *
 * A projective point in(X:Y:Z) coordinates
 */
typedef struct jac_point_t
{
    fp2_t x;
    fp2_t y;
    fp2_t z;
} jac_point_t;

/** @brief A basis of a torsion subgroup
 *
 * @typedef ec_basis_t
 *
 * @struct ec_basis_t
 *
 * A pair of points (or a triplet, tbd) forming a basis of a torsion subgroup.
 */
typedef struct ec_basis_t
{
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
typedef struct ec_curve_t
{
    fp2_t A;
    fp2_t C;                             ///< cannot be 0
    ec_point_t A24;                      // the point (A+2C : 4C)
    bool is_A24_computed_and_normalized; // says if A24 has been computed and normalized
} ec_curve_t;

/** @brief An isogeny of degree a power of 2
 *
 * @typedef ec_isog_even_t
 *
 * @struct ec_isog_even_t
 */
typedef struct ec_isog_even_t
{
    ec_curve_t curve;      ///< The domain curve
    ec_point_t kernel;     ///< A kernel generator
    unsigned short length; ///< The length as a 2-isogeny walk
} ec_isog_even_t;

typedef struct ec_isog_three_t
{
    ec_curve_t curve;      ///< The domain curve
    ec_point_t kernel;     ///< A kernel generator
    unsigned short length; ///< The length as a 3-isogeny walk
} ec_isog_three_t;


/** @brief Isomorphism of Montgomery curves
 *
 * @typedef ec_isom_t
 *
 * @struct ec_isom_t
 *
 * The isomorphism is given by the map maps (X:Z) ↦ ( (Nx X - Nz Z) : (D Z) )
 */
typedef struct ec_isom_t
{
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

void ec_curve_init(ec_curve_t *E);
void ec_curve_normalize_A24(ec_curve_t *E);
void ec_normalize_curve_and_A24(ec_curve_t *E);
/**
 * @brief j-invariant.
 *
 * @param j_inv computed j_invariant
 * @param curve input curve
 */
void ec_j_inv(fp2_t *j_inv, const ec_curve_t *curve);

/**
* @brief Test whether two curves are isomorphic.
*
* @return 0xFFFFFFFF if the curves are isomorphic, and 0x00000000 otherwise.
*/
uint32_t ec_is_isomorphic(ec_curve_t *E1, ec_curve_t *E2);

/**
 * @brief Isomorphism of elliptic curve
 *
 * @param isom computed isomorphism
 * @param from domain curve
 * @param to image curve
 */
void ec_isomorphism(ec_isom_t *isom, const ec_curve_t *from, const ec_curve_t *to);

/**
 * @brief In-place inversion of an isomorphism
 *
 * @param isom an isomorphism
 */
void ec_iso_inv(ec_isom_t *isom);

/**
* @brief Canonical representative of a curve.
*
* @param curve Input curve.
* @param std Output canonical representative of curve.
* @param isom Output isomorphism from curve to std.
*/
void ec_curve_standardized(ec_isom_t *isom, ec_curve_t *std, ec_curve_t *curve);

/**
 * @brief In-place evaluation of an isomorphism
 *
 * @param P a point
 * @param isom an isomorphism
 */
void ec_iso_eval(ec_point_t *P, ec_isom_t *isom);

/** @}
 */
/** @defgroup ec_point_t Point operations
 * @{
 */

/**
 * @brief Point equality
 *
 * @param P a point
 * @param Q a point
 * @return 1 if equal
 */
bool ec_is_equal(const ec_point_t *P, const ec_point_t *Q);

/**
 * @brief Reduce Z-coordinate of point in place
 *
 * @param P a point
 */
void ec_normalize_point(ec_point_t *P);

/**
 * @brief Test whether a point is on a curve
 *
 * @param curve a curve
 * @param P a point
 * @return 1 if P is on the curve
 */
int ec_is_on_curve(const ec_curve_t *curve, const ec_point_t *P);

/**
 * @brief Point negation
 *
 * @param res computed opposite of P
 * @param P a point
 */
void ec_neg(ec_point_t *res, const ec_point_t *P);

/**
 * @brief Point addition
 *
 * @param res computed sum of P and Q
 * @param P a point
 * @param Q a point
 * @param PQ the difference P-Q
 */
void ec_add(ec_point_t *res, const ec_point_t *P, const ec_point_t *Q, const ec_point_t *PQ);

/**
 * @brief Point doubling
 *
 * @param res computed double of P
 * @param P a point
 */
void ec_dbl(ec_point_t *res, const ec_curve_t *curve, const ec_point_t *P);

/**
 * @brief Point iterated doubling
 *
 * @param res computed double of P
 * @param P a point
 * @param n the number of double
 */
void ec_dbl_iter(ec_point_t *res, int n, ec_curve_t *curve, const ec_point_t *P);


void xTPL(ec_point_t* Q, const ec_point_t* P, const ec_point_t* A3);

/**
 * @brief Point tripling
 *
 * @param res computed triple of P
 * @param P a point
 */
void ec_tpl(ec_point_t* res, const ec_curve_t* curve, const ec_point_t* P);

/**
 * @brief Point iterated doubling
 *
 * @param res computed double of P
 * @param P a point
 * @param n the number of double
 */
void ec_dbl_iter(ec_point_t* res, int n, ec_curve_t* curve, const ec_point_t* P);

/**
* @brief Point iterated tripling
*
* @param res computed triple of P
* @param P a point
* @param n the number of triple
*/
void ec_tpl_iter(ec_point_t* res, int n, ec_curve_t* curve, const ec_point_t* P);

/**
 * @brief Point multiplication
 *
 * @param res computed scalar * P
 * @param curve the curve
 * @param scalar an unsigned multi-precision integer
 * @param P a point
 */
void ec_mul(ec_point_t *res, const ec_curve_t *curve, const digit_t *scalar, const ec_point_t *P);

/**
 * @brief Point multiplication by a scalar of limited length
 *
 * @param res computed scalar * P
 * @param curve the curve
 * @param scalar an unsigned multi-precision integer
 * @param kbits the bit size of scalar
 * @param P a point
 */
void xMULv2(ec_point_t *Q,
            ec_point_t const *P,
            digit_t const *k,
            const int kbits,
            ec_point_t const *A24);

/**
 * @brief Combination P+m*Q
 *
 * @param R computed P + m * Q
 * @param curve the curve
 * @param m an unsigned multi-precision integer
 * @param P a point
 * @param Q a point
 * @param PQ the difference P-Q
 */
void ec_ladder3pt(ec_point_t *R,
                  digit_t *const m,
                  ec_point_t const *P,
                  ec_point_t const *Q,
                  ec_point_t const *PQ,
                  ec_curve_t const *A);

/**
 * @brief Linear combination of points of a basis
 *
 * @param res computed scalarP * P + scalarQ * Q
 * @param curve the curve
 * @param scalarP an unsigned multi-precision integer
 * @param scalarQ an unsigned multi-precision integer
 * @param PQ a torsion basis consisting of points P and Q
 */
void ec_biscalar_mul(ec_point_t *res,
                     const ec_curve_t *curve,
                     const digit_t *scalarP,
                     const digit_t *scalarQ,
                     const ec_basis_t *PQ);
void ec_biscalar_mul_3(ec_point_t* res,
    const ec_curve_t* curve,
    const digit_t* scalarP,
    const digit_t* scalarQ,
    const ec_basis_t* PQ);

// same as above but points are expected to have order 2^f
void ec_biscalar_mul_bounded(ec_point_t *res,
                             const ec_curve_t *curve,
                             const digit_t *scalarP,
                             const digit_t *scalarQ,
                             const ec_basis_t *PQ,
                             int f);
/** @}
 */

/** @defgroup ec_dlog_t Discrete logs and bases
 * @{
 */

/**
 * @brief Generate a Montgomery curve and a 2^f-torsion basis
 *
 * The algorithm is deterministc
 *
 * @param PQ2 computed basis of the 2^f-torsion
 * @param curve the computed curve
 */
uint8_t ec_curve_to_basis_2f_to_hint(ec_basis_t *PQ2, ec_curve_t *curve, int f);
int ec_curve_to_basis_2f_from_hint(ec_basis_t *PQ2, ec_curve_t *curve, int f, const uint8_t hint);
/**
 * @brief Generate a 3^e-torsion basis
 *
 * The algorithm is deterministic
 *
 * @param PQ3 the computed 3^e-torsion basis
 * @param curve a curve
 */
int ec_curve_to_basis_3f_to_hint(ec_basis_t* PQ3, const ec_curve_t* curve, int* hints, int f);

void ec_curve_to_basis_3f_from_hint(ec_basis_t* PQ3, const ec_curve_t* curve, int* hints, int f);

void ec_curve_to_basis_3(ec_basis_t* PQ3, const ec_curve_t* curve);
/** @}
 */

/** @defgroup ec_isog_t Isogenies
 * @{
 */

/**
 * @brief Evaluate isogeny of even degree on list of points
 *
 * @param image computed image curve
 * @param phi isogeny
 * @param points a list of points to evaluate the isogeny on, modified in place
 * @param length of the list points
 */
void ec_eval_even(ec_curve_t *image,
                  ec_isog_even_t *phi,
                  ec_point_t *points,
                  unsigned short length);
void ec_eval_even_test(ec_curve_t *image, ec_isog_even_t *phi, ec_point_t *points, unsigned short length);

/**
 * @brief Evaluate isogeny of even degree on list of torsion bases
 *
 * @param image computed image curve
 * @param phi isogeny
 * @param points a list of bases to evaluate the isogeny on, modified in place
 * @param length of the list bases
 */
static void ec_eval_even_basis(ec_curve_t *image,
                   ec_isog_even_t *phi,
                   ec_basis_t *points,
                   unsigned short length)
{
    ec_eval_even(
        image, phi, (ec_point_t *)points, sizeof(ec_basis_t) / sizeof(ec_point_t) * length);
}


// Compute a 3-isogeny chain.
// @param phi Input isogeny, including the domain curve, kernel, and chain length.
// @param points Input/output list of points to evaluate.
// @param length Number of points in points.
// @param image Output image curve.
// @param dual If non-NULL, output a 3-torsion point generating ker(phi_dual).
void ec_eval_three(ec_curve_t *image,
                   const ec_isog_three_t* phi,
                   ec_point_t *points,
                   unsigned short length,
				   ec_point_t* dual);
void
ec_eval_three_test(ec_curve_t* image,
	const ec_isog_three_t* phi,
	ec_point_t* points,
	unsigned short length,
	ec_point_t* dual);
// Generate a 3-isogeny path of length e from domain.
// This naive method is suitable for short chains.
void
random_isog_chain_3(ec_curve_t* codomain,
	const ec_curve_t* domain,
	const unsigned int e,
	ec_point_t* points,
	const unsigned int points_len);

void copy_jac_point(jac_point_t *P, jac_point_t const *Q);
bool is_jac_equal(const jac_point_t *P, const jac_point_t *Q);
void jac_neg(jac_point_t *Q, jac_point_t const *P);
void jac_to_xz(ec_point_t *P, const jac_point_t *xyP);
bool is_jac_xz_equal(const jac_point_t *P, const ec_point_t *Q);
void ADD(jac_point_t *R, jac_point_t const *P, jac_point_t const *Q, ec_curve_t const *AC);
void DBL(jac_point_t *Q, jac_point_t const *P, ec_curve_t const *AC);
void DBLMUL_generic(jac_point_t *R,
                    const jac_point_t *P,
                    const digit_t *k,
                    const jac_point_t *Q,
                    const digit_t *l,
                    const ec_curve_t *curve,
                    int size);
void lift_point(jac_point_t *P, ec_point_t *Q, ec_curve_t *E);
void lift_basis(jac_point_t *P, jac_point_t *Q, ec_basis_t *B, ec_curve_t *E);

static int
test_point_order_twof(const ec_point_t *P, const ec_curve_t *E, int t)
{
    ec_point_t test;
    test = *P;
    // copy_point(&test,P);
    if (fp2_is_zero(&test.z))
        return 0;
    for (int i = 0; i < t - 1; i++) {
        ec_dbl(&test, E, &test);
    }
    if (fp2_is_zero(&test.z))
        return 0;
    ec_dbl(&test, E, &test);
    return (fp2_is_zero(&test.z));
}

static int
test_jac_order_twof(const jac_point_t *P, const ec_curve_t *E, int t)
{
    jac_point_t test;
    test = *P;
    if (fp2_is_zero(&test.z))
        return 0;
    for (int i = 0; i < t - 1; i++) {
        DBL(&test, &test, E);
    }
    if (fp2_is_zero(&test.z))
        return 0;
    DBL(&test, &test, E);
    return (fp2_is_zero(&test.z));
}

static int
test_point_order_threef(const ec_point_t *P, const ec_curve_t *E, int t)
{
    ec_point_t test, temp;
    test = *P;
    if (fp2_is_zero(&test.z))
        return 0;
    for (int i = 0; i < t - 1; i++) {
	    ec_dbl(&temp, E, &test);
	    ec_add(&test, &temp, &test, &test);
    }
    if (fp2_is_zero(&test.z)) {
        return 0;
    }
    ec_dbl(&temp, E, &test);
    ec_add(&test, &temp, &test, &test);
    return (fp2_is_zero(&test.z));
}

static void
point_print(char *name, ec_point_t P)
{
    fp2_t a;
    if (fp2_is_zero(&P.z)) {
        printf("%s = INF\n", name);
    } else {
        fp2_copy(&a, &P.z);
        fp2_inv(&a);
        fp2_mul(&a, &a, &P.x);
        fp2_print(name, &a);
    }
}

static void
curve_print(char *name, ec_curve_t E)
{
    fp2_t a;
    fp2_copy(&a, &E.C);
    fp2_inv(&a);
    fp2_mul(&a, &a, &E.A);
    fp2_print(name, &a);
}

static void
AC_to_A24(ec_point_t *A24, ec_curve_t const *E)
{
    // A24 = (A+2C : 4C)
    fp2_add(&A24->z, &E->C, &E->C);
    fp2_add(&A24->x, &E->A, &A24->z);
    fp2_add(&A24->z, &A24->z, &A24->z);
}

static void
A24_to_AC(ec_curve_t *E, ec_point_t const *A24)
{
    // (A:C) = ((A+2C)*2-4C : 4C)
    fp2_add(&E->A, &A24->x, &A24->x);
    fp2_sub(&E->A, &E->A, &A24->z);
    fp2_add(&E->A, &E->A, &E->A);
    fp2_copy(&E->C, &A24->z);
}

static void
A3_to_AC(ec_curve_t* E, ec_point_t const* A3)
{
    // (A:C) = (2*(A+2C+A-2C):(A+2C-(A-2C)))
    fp2_add(&E->A, &A3->x, &A3->z);
    fp2_add(&E->A, &E->A, &E->A);
    fp2_sub(&E->C, &A3->x, &A3->z);
}

static void
AC_to_A3(ec_point_t* A3, ec_curve_t const* E)
{
    // A3 = (A+2C:A-2C) 
    fp2_add(&A3->z, &E->C, &E->C);
    fp2_add(&A3->x, &E->A, &A3->z);
    fp2_sub(&A3->z, &E->A, &A3->z);
}

void xDBL_A24_normalized(ec_point_t *Q, ec_point_t const *P, ec_point_t const *A24);

/** @}
 */

// end ec
/** @}
 */

#endif
