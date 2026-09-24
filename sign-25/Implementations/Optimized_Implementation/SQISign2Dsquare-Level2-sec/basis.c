#include "ec.h"
#include "fp2.h"
#include "isog.h"
#include "gf_constants.h"
#include <assert.h>

void
xTPL(ec_point_t *Q, const ec_point_t *P, const ec_point_t *A3)
{
    /* ----------------------------------------------------------------------------- *
     * Differential point tripling given the montgomery coefficient A3 = (A+2C:A-2C)
     * ----------------------------------------------------------------------------- */

    fp2_t t0, t1, t2, t3, t4;
    fp2_sub(&t0, &P->x, &P->z);
    fp2_sqr(&t2, &t0);
    fp2_add(&t1, &P->x, &P->z);
    fp2_sqr(&t3, &t1);
    fp2_add(&t4, &t1, &t0);
    fp2_sub(&t0, &t1, &t0);
    fp2_sqr(&t1, &t4);
    fp2_sub(&t1, &t1, &t3);
    fp2_sub(&t1, &t1, &t2);
    fp2_mul(&Q->x, &t3, &A3->x);
    fp2_mul(&t3, &Q->x, &t3);
    fp2_mul(&Q->z, &t2, &A3->z);
    fp2_mul(&t2, &t2, &Q->z);
    fp2_sub(&t3, &t2, &t3);
    fp2_sub(&t2, &Q->x, &Q->z);
    fp2_mul(&t1, &t2, &t1);
    fp2_add(&t2, &t3, &t1);
    fp2_sqr(&t2, &t2);
    fp2_mul(&Q->x, &t2, &t4);
    fp2_sub(&t1, &t3, &t1);
    fp2_sqr(&t1, &t1);
    fp2_mul(&Q->z, &t1, &t0);
}

int
ec_is_on_curve(const ec_curve_t *curve, const ec_point_t *P)
{
	if (fp2_is_zero(&P->z))
		return 1;

    fp2_t t0, t1, t2;

    // Check if xz*(C^2x^2+zACx+z^2C^2) is a square
    fp2_mul(&t0, &curve->C, &P->x);
    fp2_mul(&t1, &t0, &P->z);
    fp2_mul(&t1, &t1, &curve->A);
    fp2_mul(&t2, &curve->C, &P->z);
    fp2_sqr(&t0, &t0);
    fp2_sqr(&t2, &t2);
    fp2_add(&t0, &t0, &t1);
    fp2_add(&t0, &t0, &t2);
    fp2_mul(&t0, &t0, &P->x);
    fp2_mul(&t0, &t0, &P->z);
    return fp2_is_square(&t0);
}

static void
difference_point(ec_point_t *PQ, const ec_point_t *P, const ec_point_t *Q, const ec_curve_t *curve)
{
	// Given P,Q in projective x-only, computes a deterministic choice for (P-Q)
	// Based on Proposition 3 of https://eprint.iacr.org/2017/518.pdf

	fp2_t Bxx, Bxz, Bzz, t0, t1;

	fp2_mul(&t0, &P->x, &Q->x);
	fp2_mul(&t1, &P->z, &Q->z);
	fp2_sub(&Bxx, &t0, &t1);
	fp2_sqr(&Bxx, &Bxx);
	fp2_mul(&Bxx, &Bxx, &curve->C); // C*(P.x*Q.x-P.z*Q.z)^2
	fp2_add(&Bxz, &t0, &t1);
	fp2_mul(&t0, &P->x, &Q->z);
	fp2_mul(&t1, &P->z, &Q->x);
	fp2_add(&Bzz, &t0, &t1);
	fp2_mul(&Bxz, &Bxz, &Bzz); // (P.x*Q.x+P.z*Q.z)(P.x*Q.z+P.z*Q.x)
	fp2_sub(&Bzz, &t0, &t1);
	fp2_sqr(&Bzz, &Bzz);
	fp2_mul(&Bzz, &Bzz, &curve->C); // C*(P.x*Q.z-P.z*Q.x)^2
	fp2_mul(&Bxz, &Bxz, &curve->C); // C*(P.x*Q.x+P.z*Q.z)(P.x*Q.z+P.z*Q.x)
	fp2_mul(&t0, &t0, &t1);
	fp2_mul(&t0, &t0, &curve->A);
	fp2_add(&t0, &t0, &t0);
	fp2_add(&Bxz, &Bxz, &t0); // C*(P.x*Q.x+P.z*Q.z)(P.x*Q.z+P.z*Q.x) + 2*A*P.x*Q.z*P.z*Q.x

							  // To ensure that the denominator is a fourth power in Fp, we normalize by
							  // C*C_bar^2*(P.z)_bar^2*(Q.z)_bar^2
	fp_copy(&t0.re, &curve->C.re);
	fp_neg(&t0.im, &curve->C.im);
	fp2_sqr(&t0, &t0);
	fp2_mul(&t0, &t0, &curve->C);
	fp_copy(&t1.re, &P->z.re);
	fp_neg(&t1.im, &P->z.im);
	fp2_sqr(&t1, &t1);
	fp2_mul(&t0, &t0, &t1);
	fp_copy(&t1.re, &Q->z.re);
	fp_neg(&t1.im, &Q->z.im);
	fp2_sqr(&t1, &t1);
	fp2_mul(&t0, &t0, &t1);
	fp2_mul(&Bxx, &Bxx, &t0);
	fp2_mul(&Bxz, &Bxz, &t0);
	fp2_mul(&Bzz, &Bzz, &t0);

	// Solving quadratic equation
	fp2_sqr(&t0, &Bxz);
	fp2_mul(&t1, &Bxx, &Bzz);
	fp2_sub(&t0, &t0, &t1);
	// No need to check if t0 is square, as per the entangled basis algorithm.
	fp2_sqrt(&t0);
	fp2_add(&PQ->x, &Bxz, &t0);
	fp2_copy(&PQ->z, &Bzz);
}

static void
difference_point_normalized(ec_point_t *PQ, const ec_point_t *P, const ec_point_t *Q, const ec_curve_t *curve)
{
    // Given P,Q in affine x-only, computes a deterministic choice for (P-Q)
    // The points must be normalized to z=1 and the curve to C=1

    fp2_t t0, t1, t2, t3;

    fp2_sub(&PQ->z, &P->x, &Q->x); // P - Q
    fp2_mul(&t2, &P->x, &Q->x);    // P*Q
    fp2_set_one(&t1);
    fp2_sub(&t3, &t2, &t1);       // P*Q-1
    fp2_mul(&t0, &PQ->z, &t3);    // (P-Q)*(P*Q-1)
    fp2_sqr(&PQ->z, &PQ->z);      // (P-Q)^2
    fp2_sqr(&t0, &t0);            // (P-Q)^2*(P*Q-1)^2
    fp2_add(&t1, &t2, &t1);       // P*Q+1
    fp2_add(&t3, &P->x, &Q->x);   // P+Q
    fp2_mul(&t1, &t1, &t3);       // (P+Q)*(P*Q+1)
    fp2_mul(&t2, &t2, &curve->A); // A*P*Q
    fp2_add(&t2, &t2, &t2);       // 2*A*P*Q
    fp2_add(&t1, &t1, &t2);       // (P+Q)*(P*Q+1) + 2*A*P*Q
    fp2_sqr(&t2, &t1);            // ((P+Q)*(P*Q+1) + 2*A*P*Q)^2
    fp2_sub(&t0, &t2, &t0);       // ((P+Q)*(P*Q+1) + 2*A*P*Q)^2 - (P-Q)^2*(P*Q-1)^2
    fp2_sqrt(&t0);
    fp2_add(&PQ->x, &t0, &t1);
}


int
ec_curve_to_basis_3f_to_hint(ec_basis_t* PQ3, const ec_curve_t* curve, int* hints, int f)
{
    int hint;
    fp2_t x, t0, t1, t2;
    ec_point_t P, Q, Q3, P3, A24, A3;

    // Curve coefficient in the form A24 = (A+2C:4C)
    fp2_add(&A24.z, &curve->C, &curve->C);
    fp2_add(&A24.x, &curve->A, &A24.z);
    fp2_add(&A24.z, &A24.z, &A24.z);

    // Curve coefficient in the form A3 = (A+2C:A-2C)
    fp2_sub(&A3.z, &A24.x, &A24.z);
    fp2_copy(&A3.x, &A24.x);

    fp2_set_one(&x);
    hint = 0;

    // Find P
    while (hint < 256) {
        hint++;
        fp_set_small(&x.im, hint);

        // Check if point is rational
        fp2_sqr(&t0, &curve->C);
        fp2_mul(&t1, &t0, &x);
        fp2_mul(&t2, &curve->A, &curve->C);
        fp2_add(&t1, &t1, &t2);
        fp2_mul(&t1, &t1, &x);
        fp2_add(&t1, &t1, &t0);
        fp2_mul(&t1, &t1, &x);
        if (fp2_is_square(&t1)) {
            fp2_copy(&P.x, &x);
            fp2_set_one(&P.z);
        }
        else
            continue;

        // Clear non-3 factors from the order
        xMULv2(&P, &P, p_cofactor_for_3g, (int)P_COFACTOR_FOR_3G_BITLENGTH, &A24);

        // Check if point has order 3^g
        for (int i = 0; i < POWER_OF_3 - f; i++) {
            xTPL(&P, &P, &A3);
        }
        copy_point(&P3, &P);
        for (int i = 0; i < f - 1; i++)
            xTPL(&P3, &P3, &A3);
        if (ec_is_zero(&P3))
            continue;
        else
            break;
    }
    if (hint == 256)
        return 0;
    else
        hints[0] = hint;

    fp_set_small(&x.im, 1);
    hint = 1;

    // Find Q
    while (hint < 256) {
        hint++;
        fp_set_small(&x.re, hint);

        // Check if point is rational
        fp2_sqr(&t0, &curve->C);
        fp2_mul(&t1, &t0, &x);
        fp2_mul(&t2, &curve->A, &curve->C);
        fp2_add(&t1, &t1, &t2);
        fp2_mul(&t1, &t1, &x);
        fp2_add(&t1, &t1, &t0);
        fp2_mul(&t1, &t1, &x);
        if (fp2_is_square(&t1)) {
            fp2_copy(&Q.x, &x);
            fp2_set_one(&Q.z);
        }
        else
            continue;

        // Clear non-3 factors from the order
        xMULv2(&Q, &Q, p_cofactor_for_3g, (int)P_COFACTOR_FOR_3G_BITLENGTH, &A24);

        // Check if point has order 3^g
        for (int i = 0; i < POWER_OF_3 - f; i++) {
            xTPL(&Q, &Q, &A3);
        }
        copy_point(&Q3, &Q);
        for (int i = 0; i < f - 1; i++)
            xTPL(&Q3, &Q3, &A3);
        if (ec_is_zero(&Q3))
            continue;

        // Check if point is orthogonal to P
        if (is_point_equal(&P3, &Q3))
            continue;
        xDBL_A24(&P3, &P3, &A24);
        if (is_point_equal(&P3, &Q3))
            continue;
        else
            break;
    }
    if (hint == 256)
        return 0;
    else
        hints[1] = hint;

    // Normalize points
    ec_curve_t E;
    ec_curve_init(&E);

    fp2_mul(&t0, &P.z, &Q.z);
    fp2_mul(&t1, &t0, &curve->C);
    fp2_inv(&t1);
    fp2_mul(&P.x, &P.x, &t1);
    fp2_mul(&Q.x, &Q.x, &t1);
    fp2_mul(&E.A, &curve->A, &t1);
    fp2_mul(&P.x, &P.x, &Q.z);
    fp2_mul(&P.x, &P.x, &curve->C);
    fp2_mul(&Q.x, &Q.x, &P.z);
    fp2_mul(&Q.x, &Q.x, &curve->C);
    fp2_mul(&E.A, &E.A, &t0);
    fp2_set_one(&P.z);
    fp2_copy(&Q.z, &P.z);
    fp2_copy(&E.C, &P.z);

    // Compute P-Q
    difference_point(&PQ3->PmQ, &P, &Q, &E);
    copy_point(&PQ3->P, &P);
    copy_point(&PQ3->Q, &Q);

    return 1;
}

void
ec_curve_to_basis_3f_from_hint(ec_basis_t* PQ3, const ec_curve_t* curve, int* hints, int f)
{
    fp2_t x, t0, t1;
    ec_point_t P, Q, A24, A3;

    // Curve coefficient in the form A24 = (A+2C:4C)
    fp2_add(&A24.z, &curve->C, &curve->C);
    fp2_add(&A24.x, &curve->A, &A24.z);
    fp2_add(&A24.z, &A24.z, &A24.z);

    // Curve coefficient in the form A3 = (A+2C:A-2C)
    fp2_sub(&A3.z, &A24.x, &A24.z);
    fp2_copy(&A3.x, &A24.x);

    // set for P
    fp_set_small(&x.re, 1);
    fp_set_small(&x.im, hints[0]);
    fp2_copy(&P.x, &x);
    fp2_set_one(&P.z);
    xMULv2(&P, &P, p_cofactor_for_3g, (int)P_COFACTOR_FOR_3G_BITLENGTH, &A24);
    for (int i = 0; i < POWER_OF_3 - f; i++) {
        xTPL(&P, &P, &A3);
    }
    // set for Q
    fp_set_small(&x.re, hints[1]);
    fp_set_small(&x.im, 1);
    fp2_copy(&Q.x, &x);
    fp2_set_one(&Q.z);
    xMULv2(&Q, &Q, p_cofactor_for_3g, (int)P_COFACTOR_FOR_3G_BITLENGTH, &A24);
    for (int i = 0; i < POWER_OF_3 - f; i++) {
        xTPL(&Q, &Q, &A3);
    }
    // Normalize points
    ec_curve_t E;
    ec_curve_init(&E);

    fp2_mul(&t0, &P.z, &Q.z);
    fp2_mul(&t1, &t0, &curve->C);
    fp2_inv(&t1);
    fp2_mul(&P.x, &P.x, &t1);
    fp2_mul(&Q.x, &Q.x, &t1);
    fp2_mul(&E.A, &curve->A, &t1);
    fp2_mul(&P.x, &P.x, &Q.z);
    fp2_mul(&P.x, &P.x, &curve->C);
    fp2_mul(&Q.x, &Q.x, &P.z);
    fp2_mul(&Q.x, &Q.x, &curve->C);
    fp2_mul(&E.A, &E.A, &t0);
    fp2_set_one(&P.z);
    fp2_copy(&Q.z, &P.z);
    fp2_copy(&E.C, &P.z);

    // Compute P-Q
    difference_point(&PQ3->PmQ, &P, &Q, &E);
    copy_point(&PQ3->P, &P);
    copy_point(&PQ3->Q, &Q);
}

void
ec_curve_to_basis_3(ec_basis_t* PQ3, const ec_curve_t* curve)
{
    int flag = 0;
    fp2_t x, t0, t1, t2;
    ec_point_t P, Q, Q3, P3, A24, A3;

    // Curve coefficient in the form A24 = (A+2C:4C)
    fp2_add(&A24.z, &curve->C, &curve->C);
    fp2_add(&A24.x, &curve->A, &A24.z);
    fp2_add(&A24.z, &A24.z, &A24.z);

    // Curve coefficient in the form A3 = (A+2C:A-2C)
    fp2_sub(&A3.z, &A24.x, &A24.z);
    fp2_copy(&A3.x, &A24.x);

    fp2_set_one(&x);

    // Find P
    while (1) {
        fp_add(&(x.im), &(x.re), &(x.im));

        // Check if point is rational
        fp2_sqr(&t0, &curve->C);
        fp2_mul(&t1, &t0, &x);
        fp2_mul(&t2, &curve->A, &curve->C);
        fp2_add(&t1, &t1, &t2);
        fp2_mul(&t1, &t1, &x);
        fp2_add(&t1, &t1, &t0);
        fp2_mul(&t1, &t1, &x);
        if (fp2_is_square(&t1)) {
            fp2_copy(&P.x, &x);
            fp2_set_one(&P.z);
        }
        else
            continue;

        // Clear non-3 factors from the order
        xMULv2(&P, &P, p_cofactor_for_3g, (int)P_COFACTOR_FOR_3G_BITLENGTH, &A24);
        if (ec_is_zero(&P))
            continue;
        for (int i = 0; i < POWER_OF_3 ; i++) {
            xTPL(&P3, &P, &A3);
            if (ec_is_zero(&P3)) {
                flag = 1;
                break;
            }
            else {
                copy_point(&P, &P3);
            }
        }
        if (flag)
            break;
    }

    flag = 0;

    // Find Q
    while (1) {
        fp_add(&(x.im), &(x.re), &(x.im));

        // Check if point is rational
        fp2_sqr(&t0, &curve->C);
        fp2_mul(&t1, &t0, &x);
        fp2_mul(&t2, &curve->A, &curve->C);
        fp2_add(&t1, &t1, &t2);
        fp2_mul(&t1, &t1, &x);
        fp2_add(&t1, &t1, &t0);
        fp2_mul(&t1, &t1, &x);
        if (fp2_is_square(&t1)) {
            fp2_copy(&Q.x, &x);
            fp2_set_one(&Q.z);
        }
        else
            continue;
		assert(ec_is_on_curve(curve, &Q));
        // Clear non-3 factors from the order
        xMULv2(&Q, &Q, p_cofactor_for_3g, (int)P_COFACTOR_FOR_3G_BITLENGTH, &A24);
        if (ec_is_zero(&Q))
            continue;
        for (int i = 0; i < POWER_OF_3; i++) {
            xTPL(&Q3, &Q, &A3);
            if (ec_is_zero(&Q3)) {
                flag = 1;
                break;
            }
            else {
                copy_point(&Q, &Q3);
            }
        }
        if (!flag)
            continue;
        if (ec_is_equal(&P, &Q))
            continue;
        xDBL_A24(&P3, &P, &A24);
        if (ec_is_equal(&P3, &Q))
            continue;
        else
            break;
    }

    // Compute P-Q
    difference_point_normalized(&PQ3->PmQ, &P, &Q, curve);
    copy_point(&PQ3->P, &P);
    copy_point(&PQ3->Q, &Q);

#if _DEBUG
	assert(ec_is_on_curve(curve, &PQ3->P));
	assert(ec_is_on_curve(curve, &PQ3->Q));
	assert(ec_is_on_curve(curve, &PQ3->PmQ));
#endif
}

// Helper function which given a point of order k*2^n with n maximal
// and k odd, computes a point of order 2^f
static void
clear_cofactor_for_maximal_even_order(ec_point_t *P, const ec_curve_t *curve, int f)
{
    // clear out the odd cofactor to get a point of order 2^n
    xMULv2(P, P, p_cofactor_for_2f, P_COFACTOR_FOR_2F_BITLENGTH, &curve->A24);

    // clear the power of two to get a point of order 2^f
    for (int i = 0; i < POWER_OF_2 - f; i++) {
        xDBL_A24_normalized(P, P, &curve->A24);
    }
}

// Given an x-coordinate, determines if this is a valid
// point on the curve. Assumes C=1.
static uint32_t
is_on_curve(const fp2_t *x, const ec_curve_t *curve)
{
	assert(fp2_is_one(&curve->C));
	fp2_t t0, ONE;
	fp2_set_one(&ONE);

	fp2_add(&t0, x, &curve->A); // x + (A/C)
	fp2_mul(&t0, &t0, x);       // x^2 + (A/C)*x
	fp2_add(&t0, &t0, &ONE);
	fp2_mul(&t0, &t0, x);       // x^3 + (A/C)*x^2 + x

	return fp2_is_square(&t0);
}

// Helper function which finds a point x(P) = n * A
static uint8_t
find_nA_x_coord(fp2_t *x, ec_curve_t *curve)
{
	assert(!fp2_is_square(&curve->A)); // Only to be called when A is a NQR

									   // when A is NQR we allow x(P) to be a multiple n*A of A
	uint8_t n = 1;
	fp2_copy(x, &curve->A);

	while (!is_on_curve(x, curve)) {
		fp2_add(x, x, &curve->A);
		n++;
	}

	/*
	* With very low probability (1/2^128), n will not fit in 7 bits.
	* In this case, we set hint = 0 which signals failure and the need
	* to generate a value on the fly during verification
	*/
	uint8_t hint = n < 128 ? n : 0;
	return hint;
}

// Helper function which finds an NQR -1 / (1 + i*b) for entangled basis generation
static uint8_t
find_nqr_factor(fp2_t *x, ec_curve_t *curve)
{
	// factor = -1/(1 + i*b) for b in Fp will be NQR whenever 1 + b^2 is NQR
	// in Fp, so we find one of these and then invert (1 + i*b). We store b
	// as a u8 hint to save time in verification.

	// We return the hint as a u8, but use (uint16_t)n to give 2^16 - 1
	// to make failure cryptographically negligible, with a fallback when
	// n > 128 is required.
	uint8_t hint;
	uint32_t found = 0;
	uint16_t n = 1;

	bool qr_b = 1;
	fp_t b, tmp;
	fp2_t z, t0, t1;

	do {
		while (qr_b) {
			// find b with 1 + b^2 a non-quadratic residue
			fp_set_small(&tmp, (uint32_t)n * n + 1);
			qr_b = fp_is_square(&tmp);
			n++; // keeps track of b = n - 1
		}

		// for Px := -A/(1 + i*b) to be on the curve
		// is equivalent to A^2*(z-1) - z^2 NQR for z = 1 + i*b
		// thus prevents unnecessary inversion pre-check

		// t0 = z - 1 = i*b
		// t1 = z = 1 + i*b
		fp_set_small(&b, (uint32_t)n - 1);
		fp2_set_zero(&t0);
		fp2_set_one(&z);
		fp_copy(&z.im, &b);
		fp_copy(&t0.im, &b);

		// A^2*(z-1) - z^2
		fp2_sqr(&t1, &curve->A);
		fp2_mul(&t0, &t0, &t1); // A^2 * (z - 1)
		fp2_sqr(&t1, &z);
		fp2_sub(&t0, &t0, &t1); // A^2 * (z - 1) - z^2
		found = !fp2_is_square(&t0);

		qr_b = 1;
	} while (!found);

	// set Px to -A/(1 + i*b)
	fp2_copy(x, &z);
	fp2_inv(x);
	fp2_mul(x, x, &curve->A);
	fp2_neg(x, x);

	/*
	* With very low probability n will not fit in 7 bits.
	* We set hint = 0 which signals failure and the need
	* to generate a value on the fly during verification
	*/
	hint = n <= 128 ? n - 1 : 0;

	return hint;
}

// Computes a basis E[2^f] = <P, Q> where the point Q is above (0 : 0)
// and stores hints as an array for faster recomputation at a later point
uint8_t
ec_curve_to_basis_2f_to_hint(ec_basis_t *PQ2, ec_curve_t *curve, int f)
{
	assert(!fp2_is_zero(&curve->A));

	// Normalise (A/C : 1) and ((A + 2)/4 : 1)
	ec_normalize_curve_and_A24(curve);

	uint8_t hint;
	bool hint_A = fp2_is_square(&curve->A);

	// Compute the points P, Q
	ec_point_t P, Q;

	if (!hint_A) {
		// when A is NQR we allow x(P) to be a multiple n*A of A
		hint = find_nA_x_coord(&P.x, curve);
	}
	else {
		// when A is QR we instead have to find (1 + b^2) a NQR
		// such that x(P) = -A / (1 + i*b)
		hint = find_nqr_factor(&P.x, curve);
	}

	fp2_set_one(&P.z);
	fp2_add(&Q.x, &curve->A, &P.x);
	fp2_neg(&Q.x, &Q.x);
	fp2_set_one(&Q.z);

	// clear out the odd cofactor to get a point of order 2^f
	clear_cofactor_for_maximal_even_order(&P, curve, f);
	clear_cofactor_for_maximal_even_order(&Q, curve, f);

	assert(test_point_order_twof(&P, curve, TORSION_PLUS_EVEN_POWER));
	assert(test_point_order_twof(&Q, curve, TORSION_PLUS_EVEN_POWER));

	// compute PmQ, set PmQ to Q to ensure Q above (0,0)
	difference_point(&PQ2->Q, &P, &Q, curve);
	copy_point(&PQ2->P, &P);
	copy_point(&PQ2->PmQ, &Q);

	// Finally, we compress hint_A and hint into a single bytes.
	// We choose to set the LSB of hint to hint_A
	assert(hint < 128); // We expect hint to be 7-bits in size
	return (hint << 1) | hint_A;
}

// Computes a basis E[2^f] = <P, Q> where the point Q is above (0 : 0)
// given the hints as an array for faster basis computation
int
ec_curve_to_basis_2f_from_hint(ec_basis_t *PQ2, ec_curve_t *curve, int f, const uint8_t hint)
{
	assert(!fp2_is_zero(&curve->A));

	// Normalise (A/C : 1) and ((A + 2)/4 : 1)
	ec_normalize_curve_and_A24(curve);

	// The LSB of hint encodes whether A is a QR
	// The remaining 7-bits are used to find a valid x(P)
	bool hint_A = hint & 1;
	uint8_t hint_P = hint >> 1;

	// Compute the points P, Q
	ec_point_t P, Q;

	if (!hint_P) {
		return 0;
	}
	else {
		// Otherwise we use the hint to directly find x(P) based on hint_A
		if (!hint_A) {
			// when A is NQR, we have found n such that x(P) = n*A
			fp2_t temp;
			fp2_set_small(&temp, hint_P);
			fp2_mul(&P.x, &curve->A, &temp);
		}
		else {
			// when A is QR we have found b such that (1 + b^2) is a NQR in
			// Fp, so we must compute x(P) = -A / (1 + i*b)
			fp_t b, denom;
			fp_set_small(&b, hint_P);
			fp_sqr(&denom, &b);
			fp_add(&denom, &denom, &ONE);
			fp_inv(&denom);
			fp_copy(&P.x.re, &denom);
			fp_mul(&P.x.im, &b, &denom);
			fp_neg(&P.x.im, &P.x.im);
			fp2_mul(&P.x, &P.x, &curve->A);
			fp2_neg(&P.x, &P.x);
		}
	}
	fp2_set_one(&P.z);

#ifndef NDEBUG
	int passed = 1;
	passed = is_on_curve(&P.x, curve);
	passed &= !fp2_is_square(&P.x);

	if (!passed)
		return 0;
#endif

	// set xQ to -xP - A
	fp2_add(&Q.x, &curve->A, &P.x);
	fp2_neg(&Q.x, &Q.x);
	fp2_set_one(&Q.z);

	// clear out the odd cofactor to get a point of order 2^f
	clear_cofactor_for_maximal_even_order(&P, curve, f);
	clear_cofactor_for_maximal_even_order(&Q, curve, f);

	// compute PmQ, set PmQ to Q to ensure Q above (0,0)
	difference_point(&PQ2->Q, &P, &Q, curve);
	copy_point(&PQ2->P, &P);
	copy_point(&PQ2->PmQ, &Q);

#ifndef NDEBUG
	passed &= test_point_order_twof(&PQ2->P, curve, f);
	passed &= test_point_order_twof(&PQ2->Q, curve, f);
	passed &= test_point_order_twof(&PQ2->PmQ, curve, f);

	if (!passed)
		return 0;
#endif

	return 1;
}
