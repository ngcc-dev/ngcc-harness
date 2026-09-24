#include "ec.h"
#include "fp2.h"
#include "isog.h"
#include "gf_constants.h"

static void
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

void
ec_curve_to_basis_2_to_hint(ec_basis_t *PQ2, ec_curve_t *curve, int f, int *hint)
{
    fp2_t x, t0, t1, t2;
    ec_point_t P, Q, Q2, P2;

    // normalize
    ec_curve_normalize_A24(curve);

    fp2_set_one(&x);

    int count = 0;

    // Find P
    while (1) {
        count++;
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
        } else
            continue;

        // Clear odd factors from the order
        xMULv2(&P, &P, p_cofactor_for_2f, P_COFACTOR_FOR_2F_BITLENGTH, &curve->A24);
        // clear the power of two
        for (int i = 0; i < POWER_OF_2 - f; i++) {
            xDBL_A24_normalized(&P, &P, &curve->A24);
        }

        // Check if point has order 2^f
        copy_point(&P2, &P);
        for (int i = 0; i < f - 1; i++)
            xDBL_A24_normalized(&P2, &P2, &curve->A24);
        if (ec_is_zero(&P2))
            continue;
        else
            break;
    }

    hint[0] = count;

    count = 0;
    // Find Q
    while (1) {
        count++;
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
        } else
            continue;

        // Clear odd factors from the order
        xMULv2(&Q, &Q, p_cofactor_for_2f, P_COFACTOR_FOR_2F_BITLENGTH, &curve->A24);
        // clear the power of two
        for (int i = 0; i < POWER_OF_2 - f; i++) {
            xDBL_A24_normalized(&Q, &Q, &curve->A24);
        }

        // Check if point has order 2^f
        copy_point(&Q2, &Q);
        for (int i = 0; i < f - 1; i++)
            xDBL_A24_normalized(&Q2, &Q2, &curve->A24);
        if (ec_is_zero(&Q2))
            continue;

        // Check if point is orthogonal to P
        if (is_point_equal(&P2, &Q2))
            continue;
        else
            break;
    }

    hint[1] = count;

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
    difference_point(&PQ2->PmQ, &P, &Q, &E);
    copy_point(&PQ2->P, &P);
    copy_point(&PQ2->Q, &Q);
}

void
ec_curve_to_basis_2_from_hint(ec_basis_t *PQ2, ec_curve_t *curve, int f, int *hint)
{
    fp2_t x, t0, t1, t2;
    ec_point_t P, Q;

    // normalize
    ec_curve_normalize_A24(curve);

    fp2_set_one(&x);

    int count = 0;

    for (int i = 0; i < hint[0]; i++) {
        fp_add(&(x.im), &(x.re), &(x.im));
    }
    fp2_copy(&P.x, &x);
    fp2_set_one(&P.z);

    // getting the actual point
    // Clear odd factors from the order
    xMULv2(&P, &P, p_cofactor_for_2f, P_COFACTOR_FOR_2F_BITLENGTH, &curve->A24);
    // clear the power of two
    for (int i = 0; i < POWER_OF_2 - f; i++) {
        xDBL_A24_normalized(&P, &P, &curve->A24);
    }
    // second point

    for (int i = 0; i < hint[1]; i++) {
        fp_add(&(x.im), &(x.re), &(x.im));
    }

    fp2_copy(&Q.x, &x);
    fp2_set_one(&Q.z);

    // Clear odd factors from the order
    xMULv2(&Q, &Q, p_cofactor_for_2f, P_COFACTOR_FOR_2F_BITLENGTH, &curve->A24);
    // clear the power of two
    for (int i = 0; i < POWER_OF_2 - f; i++) {
        xDBL_A24_normalized(&Q, &Q, &curve->A24);
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
    difference_point(&PQ2->PmQ, &P, &Q, &E);
    copy_point(&PQ2->P, &P);
    copy_point(&PQ2->Q, &Q);
}

void
ec_curve_to_basis_2(ec_basis_t *PQ2, ec_curve_t *curve, int f)
{
    fp2_t x, t0, t1, t2;
    ec_point_t P, Q, Q2, P2;

    // normalize
    ec_curve_normalize_A24(curve);

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
        } else
            continue;

        // Clear odd factors from the order
        xMULv2(&P, &P, p_cofactor_for_2f, P_COFACTOR_FOR_2F_BITLENGTH, &curve->A24);
        // clear the power of two
        for (int i = 0; i < POWER_OF_2 - f; i++) {
            xDBL_A24_normalized(&P, &P, &curve->A24);
        }

        // Check if point has order 2^f
        copy_point(&P2, &P);
        for (int i = 0; i < f - 1; i++)
            xDBL_A24_normalized(&P2, &P2, &curve->A24);
        if (ec_is_zero(&P2))
            continue;
        else
            break;
    }

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
        } else
            continue;

        // Clear odd factors from the order
        xMULv2(&Q, &Q, p_cofactor_for_2f, P_COFACTOR_FOR_2F_BITLENGTH, &curve->A24);
        // clear the power of two
        for (int i = 0; i < POWER_OF_2 - f; i++) {
            xDBL_A24_normalized(&Q, &Q, &curve->A24);
        }

        // Check if point has order 2^f
        copy_point(&Q2, &Q);
        for (int i = 0; i < f - 1; i++)
            xDBL_A24_normalized(&Q2, &Q2, &curve->A24);
        if (ec_is_zero(&Q2))
            continue;

        // Check if point is orthogonal to P
        if (is_point_equal(&P2, &Q2))
            continue;
        else
            break;
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
    difference_point(&PQ2->PmQ, &P, &Q, &E);
    copy_point(&PQ2->P, &P);
    copy_point(&PQ2->Q, &Q);
}

void
ec_complete_basis_2(ec_basis_t *PQ2, const ec_curve_t *curve, const ec_point_t *P)
{

    fp2_t x, t0, t1, t2;
    ec_point_t Q, Q2, P2, A24;

    // Curve coefficient in the form A24 = (A+2C:4C)
    fp2_add(&A24.z, &curve->C, &curve->C);
    fp2_add(&A24.x, &curve->A, &A24.z);
    fp2_add(&A24.z, &A24.z, &A24.z);

    // Point of order 2 generated by P
    copy_point(&P2, P);
    for (int i = 0; i < POWER_OF_2 - 1; i++)
        xDBL_A24(&P2, &P2, &A24);

    // Find Q
    fp2_set_one(&x);
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
        } else
            continue;

        // Clear odd factors from the order
        xMULv2(&Q, &Q, p_cofactor_for_2f, (int)P_COFACTOR_FOR_2F_BITLENGTH, &A24);

        // Check if point has order 2^f
        copy_point(&Q2, &Q);
        for (int i = 0; i < POWER_OF_2 - 1; i++)
            xDBL_A24(&Q2, &Q2, &A24);
        if (ec_is_zero(&Q2))
            continue;

        // Check if point is orthogonal to P
        if (is_point_equal(&P2, &Q2))
            continue;
        else
            break;
    }

    // Normalize points
    ec_curve_t E;
    ec_curve_init(&E);

    ec_point_t PP;
    fp2_mul(&t0, &P->z, &Q.z);
    fp2_mul(&t1, &t0, &curve->C);
    fp2_inv(&t1);
    fp2_mul(&PP.x, &P->x, &t1);
    fp2_mul(&Q.x, &Q.x, &t1);
    fp2_mul(&E.A, &curve->A, &t1);
    fp2_mul(&PP.x, &PP.x, &Q.z);
    fp2_mul(&PP.x, &PP.x, &curve->C);
    fp2_mul(&Q.x, &Q.x, &P->z);
    fp2_mul(&Q.x, &Q.x, &curve->C);
    fp2_mul(&E.A, &E.A, &t0);
    fp2_set_one(&PP.z);
    fp2_copy(&Q.z, &PP.z);
    fp2_copy(&E.C, &PP.z);

    // Compute P-Q
    difference_point(&PQ2->PmQ, &PP, &Q, &E);
    copy_point(&PQ2->P, &PP);
    copy_point(&PQ2->Q, &Q);
}

void
ec_curve_to_basis_3(ec_basis_t *PQ3, const ec_curve_t *curve)
{

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
        } else
            continue;

        // Clear non-3 factors from the order
        xMULv2(&P, &P, p_cofactor_for_3g, (int)P_COFACTOR_FOR_3G_BITLENGTH, &A24);

        // Check if point has order 3^g
        copy_point(&P3, &P);
        for (int i = 0; i < POWER_OF_3 - 1; i++)
            xTPL(&P3, &P3, &A3);
        if (ec_is_zero(&P3))
            continue;
        else
            break;
    }

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
        } else
            continue;

        // Clear non-3 factors from the order
        xMULv2(&Q, &Q, p_cofactor_for_3g, (int)P_COFACTOR_FOR_3G_BITLENGTH, &A24);

        // Check if point has order 3^g
        copy_point(&Q3, &Q);
        for (int i = 0; i < POWER_OF_3 - 1; i++)
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
ec_curve_to_basis_6(ec_basis_t *PQ6, const ec_curve_t *curve)
{

    fp2_t x, t0, t1, t2;
    ec_point_t P, Q, Q6, P6, R, T, A24, A3;

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
        } else
            continue;

        // Clear non-2 factors and non-3 factors from the order
        xMULv2(&P, &P, p_cofactor_for_6fg, (int)P_COFACTOR_FOR_6FG_BITLENGTH, &A24);

        // Check if point has order 2^f*3^g
        copy_point(&P6, &P);
        for (int i = 0; i < POWER_OF_2 - 1; i++)
            xDBL_A24(&P6, &P6, &A24);
        for (int i = 0; i < POWER_OF_3 - 1; i++)
            xTPL(&P6, &P6, &A3);
        if (ec_is_zero(&P6))
            continue;
        xDBL_A24(&T, &P6, &A24);
        if (ec_is_zero(&T))
            continue;
        xTPL(&T, &P6, &A3);
        if (ec_is_zero(&T))
            continue;
        break;
    }

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
        } else
            continue;

        // Clear non-6 factors from the order
        xMULv2(&Q, &Q, p_cofactor_for_6fg, (int)P_COFACTOR_FOR_6FG_BITLENGTH, &A24);

        // Check first if point has order 2^f*3^g
        copy_point(&Q6, &Q);
        for (int i = 0; i < POWER_OF_2 - 1; i++)
            xDBL_A24(&Q6, &Q6, &A24);
        for (int i = 0; i < POWER_OF_3 - 1; i++)
            xTPL(&Q6, &Q6, &A3);
        if (ec_is_zero(&Q6))
            continue;
        xDBL_A24(&T, &Q6, &A24);
        if (ec_is_zero(&T))
            continue;
        xTPL(&T, &Q6, &A3);
        if (ec_is_zero(&T))
            continue;

        // Check if point P is independent from point Q
        xTPL(&R, &P6, &A3);
        xTPL(&T, &Q6, &A3);
        if (is_point_equal(&R, &T))
            continue;
        xDBL_A24(&R, &P6, &A24);
        xDBL_A24(&T, &Q6, &A24);
        if (is_point_equal(&R, &T))
            continue;
        break;
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
    difference_point(&PQ6->PmQ, &P, &Q, &E);
    copy_point(&PQ6->P, &P);
    copy_point(&PQ6->Q, &Q);
}

// New methods for basis generation using Entangled / ApresSQI like
// methods. Finds a point of full order with square checking, then
// finds the point of desired order by clearing cofactors. Not need
// to double all the way down to ensure the correct order.
//
// This also allows a faster method for completing a torsion basis
// as if we know a point P is above (0 : 0) or not, we can directly
// compute the point Q orthogonal to this one using
// ec_curve_to_point_2f_above_montgomery or
// ec_curve_to_point_2f_not_above_montgomery

/// Finds a point of order k * 2^n where n is the largest power of two
/// dividing (p+1).
/// The x-coordinate is picked such that the point (0 : 0) is always
/// the point of order two below the point.

static int
ec_curve_to_point_2f_above_montgomery(ec_point_t *P, const ec_curve_t *curve)
{
    fp_t one;
    fp_set_one(&one);

    // Compute a root of x^2 + Ax + 1
    fp2_t t0, x, four;
    fp2_t a, d, alpha;

    // TODO: do I need to compute A/C here?
    // a = A / C
    fp2_copy(&a, &curve->C);
    fp2_inv(&a);
    fp2_mul(&a, &a, &curve->A);

    // d = sqrt(A^2 - 4)
    fp2_set_small(&four, 4);
    fp2_sqr(&d, &a);
    fp2_sub(&d, &d, &four);
    fp2_sqrt(&d);

    // alpha = (-A + d) / 2
    fp2_sub(&alpha, &d, &a);
    fp2_half(&alpha, &alpha);

    int hint = 0;
    fp2_t z1, z2;
    for (;;) {
        // collect z2-value from table, we have 20 chances
        // and expect to be correct 50% of the time.
        if (hint < 20) {
            z2 = Z_NQR_TABLE[hint];
        }
        // Fallback method for when we're unlucky
        else {
            if (hint == 20) {
                fp_set_one(&z1.im);
                fp_set_one(&z2.im);
                fp_set_small(&z1.re, hint - 2);
                fp_set_small(&z2.re, hint - 1);
            }

            // Look for z2 = i + hint with z2 a square and
            // z2 - 1 not a square.
            for (;;) {
                // Set z2 = i + hint and z1 = z2 - 1
                // TODO: we could swap z1 and z2 on failure
                // and save one addition
                fp_add(&z1.re, &z1.re, &one);
                fp_add(&z2.re, &z2.re, &one);

                // Now check whether z2 is a square and z1 is not
                if (fp2_is_square(&z2) && !fp2_is_square(&z1)) {
                    break;
                } else {
                    hint += 1;
                }
            }
        }

        // Compute x-coordinate
        fp2_mul(&x, &z2, &alpha);

        // Find a point on curve with x a NQR
        fp2_add(&t0, &x, &a);         // x + (A/C)
        fp2_mul(&t0, &t0, &x);        // x^2 + (A/C)*x
        fp_add(&t0.re, &t0.re, &one); // x^2 + (A/C)*x + 1
        fp2_mul(&t0, &t0, &x);        // x^3 + (A/C)*x^2 + x

        if (fp2_is_square(&t0)) {
            fp2_copy(&P->x, &x);
            fp2_set_one(&P->z);
            break;
        } else {
            hint += 1;
        }
    }

    return hint;
}

/// Finds a point of order k * 2^n where n is the largest power of two
/// dividing (p+1) using a hint such that z2 = i + hint above the point
/// (0 : 0).
static void
ec_curve_to_point_2f_above_montgomery_from_hint(ec_point_t *P, const ec_curve_t *curve, int hint)
{
    fp2_t x, four;
    fp2_t a, d, alpha;

    // TODO: do I need to compute A/C here?
    // a = A / C
    fp2_copy(&a, &curve->C);
    fp2_inv(&a);
    fp2_mul(&a, &a, &curve->A);

    // d = sqrt(A^2 - 4)
    fp2_set_small(&four, 4);
    fp2_sqr(&d, &a);
    fp2_sub(&d, &d, &four);
    fp2_sqrt(&d);

    // alpha = (-A + d) / 2
    fp2_sub(&alpha, &d, &a);
    fp2_half(&alpha, &alpha);

    // Compute the x coordinate from the hint and alpha
    // With 1/2^20 chance we can use the table look up
    fp2_t z1, z2;
    if (hint < 20) {
        z2 = Z_NQR_TABLE[hint];
    }
    // Otherwise we create this using the form i + hint
    else {
        fp_set_small(&z2.re, hint);
        fp_set_one(&z2.im);
    }

    // fp_set_small(&z2.re, hint);
    // fp_set_one(&z2.im);
    fp2_mul(&x, &z2, &alpha);

    // Set the point
    fp2_copy(&P->x, &x);
    fp2_set_one(&P->z);
}

/// Finds a point of order k * 2^n where n is the largest power of two
/// dividing (p+1).
/// The x-coordinate is picked such that the point (0 : 0) is never the
/// point of order two.
static int
ec_curve_to_point_2f_not_above_montgomery(ec_point_t *P, const ec_curve_t *curve)
{
    int hint = 0;
    fp_t one;
    fp2_t x, t, t0, t1;

    for (;;) {
        // For each guess of an x, we expect it to be a point 1/2
        // the time, so our table look up will work with failure 2^20
        if (hint < 20) {
            x = NQR_TABLE[hint];
        }

        // Fallback method in case we do not find a value!
        // For the cases where we are unlucky, we try points of the form
        // x = hint + i
        else {
            // When we first hit this loop, set x to be i + (hint - 1)
            // NOTE: we do hint -1 as we add one before checking a square
            //       in the below loop
            if (hint == 20) {
                fp_set_one(&one);
                fp_set_one(&x.im);
                fp_set_small(&x.re, hint - 1);
            }

            // Now we find a t which is a NQR of the form i + hint
            for (;;) {
                // Increase the real part by one until a NQR is found
                // TODO: could be made faster by adding one rather
                // than setting each time, but this is OK for now.
                fp_add(&x.re, &x.re, &one);
                if (!fp2_is_square(&x)) {
                    break;
                } else {
                    hint += 1;
                }
            }
        }

        // Now we have x which is a NQR -- is it on the curve?
        // Note: the below method saves two multiplications compared
        // to old method
        fp2_mul(&t0, &x, &curve->C);  // t0 = x*C
        fp2_add(&t1, &t0, &curve->A); // C*x + A
        fp2_mul(&t1, &t1, &x);        // C*x^2 + A*x
        fp2_add(&t1, &t1, &curve->C); // C*x^2 + A*x + C
        fp2_mul(&t1, &t1, &t0);       // C^2*x^3 + A*C*x^2 + C^2*x = C^2*y^2

        if (fp2_is_square(&t1)) {
            fp2_copy(&P->x, &x);
            fp2_set_one(&P->z);
            break;
        } else {
            hint += 1;
        }
    }

    return hint;
}

/// Finds a point of order k * 2^n where n is the largest power of two
/// dividing (p+1) using a hint such that z2 = i + hint not above
/// the point (0 : 0).
static void
ec_curve_to_point_2f_not_above_montgomery_from_hint(ec_point_t *P,
                                                    const ec_curve_t *curve,
                                                    int hint)
{
    fp2_t x;

    // If we got lucky (1/2^20) then we just grab an x-value
    // from the table
    if (hint < 20) {
        x = NQR_TABLE[hint];
    }
    // Otherwise, we find points of the form
    // i + hint
    else {
        fp_set_small(&x.re, hint);
        fp_set_one(&x.im);
    }

    fp2_copy(&P->x, &x);
    fp2_set_one(&P->z);
}

// Helper function to construct normalised basis given E[N] = <P, Q>
static inline void
normalise_points_for_basis(ec_basis_t *PQ2, const ec_curve_t *curve, ec_point_t *P, ec_point_t *Q)
{
    // Normalize points
    fp2_t t0, t1;
    ec_curve_t E;
    ec_curve_init(&E);

    fp2_mul(&t0, &P->z, &Q->z);
    fp2_mul(&t1, &t0, &curve->C);
    fp2_inv(&t1);
    fp2_mul(&P->x, &P->x, &t1);
    fp2_mul(&Q->x, &Q->x, &t1);
    fp2_mul(&E.A, &curve->A, &t1);
    fp2_mul(&P->x, &P->x, &Q->z);
    fp2_mul(&P->x, &P->x, &curve->C);
    fp2_mul(&Q->x, &Q->x, &P->z);
    fp2_mul(&Q->x, &Q->x, &curve->C);
    fp2_mul(&E.A, &E.A, &t0);
    fp2_set_one(&P->z);
    fp2_copy(&Q->z, &P->z);
    fp2_copy(&E.C, &P->z);

    // Compute P-Q
    difference_point(&PQ2->PmQ, P, Q, &E);
    copy_point(&PQ2->P, P);
    copy_point(&PQ2->Q, Q);
}

// Helper function which given a point of order k*2^n with n maximal
// and k odd, computes a point of order 2^f
static inline void
clear_cofactor_for_maximal_even_order(ec_point_t *P, const ec_curve_t *curve, int f)
{
    // clear out the odd cofactor to get a point of order 2^n
    xMULv2(P, P, p_cofactor_for_2f, P_COFACTOR_FOR_2F_BITLENGTH, &curve->A24);

    // clear the power of two to get a point of order 2^f
    for (int i = 0; i < POWER_OF_2 - f; i++) {
        xDBL_A24_normalized(P, P, &curve->A24);
    }
}

// Computes a basis E[2^f] = <P, Q> where the point Q is above (0 : 0)
void
ec_curve_to_basis_2f(ec_basis_t *PQ2, ec_curve_t *curve, int f)
{
    // TODO: is this fastest for this case?
    // normalize the curve
    ec_curve_normalize_A24(curve);

    // Compute the points P, Q
    ec_point_t P, Q;
    ec_curve_to_point_2f_not_above_montgomery(&P, curve);
    ec_curve_to_point_2f_above_montgomery(&Q, curve);

    // clear out the odd cofactor to get a point of order 2^f
    clear_cofactor_for_maximal_even_order(&P, curve, f);
    clear_cofactor_for_maximal_even_order(&Q, curve, f);

    // Normalise and compute the basis P, Q and P - Q
    normalise_points_for_basis(PQ2, curve, &P, &Q);
}

// Computes a basis E[2^f] = <P, Q> where the point Q is above (0 : 0)
// and stores hints as an array for faster recomputation at a later point
void
ec_curve_to_basis_2f_to_hint(ec_basis_t *PQ2, ec_curve_t *curve, int f, int *hint)
{
    // TODO: is this fastest for this case?
    // normalize the curve
    ec_curve_normalize_A24(curve);

    // Compute the points P, Q
    ec_point_t P, Q;
    hint[0] = ec_curve_to_point_2f_not_above_montgomery(&P, curve);
    hint[1] = ec_curve_to_point_2f_above_montgomery(&Q, curve);

    // clear out the odd cofactor to get a point of order 2^f
    clear_cofactor_for_maximal_even_order(&P, curve, f);
    clear_cofactor_for_maximal_even_order(&Q, curve, f);

    // Normalise and compute the basis P, Q and P - Q
    normalise_points_for_basis(PQ2, curve, &P, &Q);
}

void
ec_curve_to_basis_2f_from_hint(ec_basis_t *PQ2, ec_curve_t *curve, int f, int *hint)
{
    // TODO: is this fastest for this case?
    // normalize the curve
    ec_curve_normalize_A24(curve);

    // Compute the points P, Q
    ec_point_t P, Q;
    ec_curve_to_point_2f_not_above_montgomery_from_hint(&P, curve, hint[0]);
    ec_curve_to_point_2f_above_montgomery_from_hint(&Q, curve, hint[1]);

    // clear out the odd cofactor to get a point of order 2^f
    clear_cofactor_for_maximal_even_order(&P, curve, f);
    clear_cofactor_for_maximal_even_order(&Q, curve, f);

    // Normalise and compute the basis P, Q and P - Q
    normalise_points_for_basis(PQ2, curve, &P, &Q);
}

/// Given a point R in E[2^f] compute E[2^f] = <P, Q> with Q above (0 : 0)
void
ec_complete_basis_2f(ec_basis_t *PQ2, ec_curve_t *curve, const ec_point_t *R, int f)
{
    ec_point_t R2, P, Q;

    // TODO: is this fastest for this case?
    // normalize the curve
    ec_curve_normalize_A24(curve);

    // Compute the point of order two beneath R
    copy_point(&R2, R);
    for (int i = 0; i < f - 1; i++) {
        xDBL_A24(&R2, &R2, &curve->A24);
    }

    // If R2 = (0 : 0) then we ensure P is not above the Montgomery point
    // and set R = Q
    if (fp2_is_zero(&R2.x)) {
        // Compute point of order k*2^n, not above (0 : 0)
        ec_curve_to_point_2f_not_above_montgomery(&P, curve);

        // clear out the odd cofactor to get a point of order 2^f
        clear_cofactor_for_maximal_even_order(&P, curve, f);

        copy_point(&Q, R);
    }
    // Otherwise, we set P = R and find Q above (0 : 0)
    else {
        copy_point(&P, R);

        // Set Q to be the point above (0 : 0)
        ec_curve_to_point_2f_above_montgomery(&Q, curve);

        // clear out the odd cofactor to get a point of order 2^f
        clear_cofactor_for_maximal_even_order(&Q, curve, f);
    }

    normalise_points_for_basis(PQ2, curve, &P, &Q);
}
