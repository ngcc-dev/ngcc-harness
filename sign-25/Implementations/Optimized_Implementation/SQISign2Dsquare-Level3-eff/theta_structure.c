#include "theta_structure.h"
#include <assert.h>

/**
 * @brief Perform the theta structure precomputation
 *
 * @param A Output: the theta_structure
 *
 * if A.null_point = (x,y,z,t)
 * if (xx,yy,zz,tt) = to_squared_theta(A.null_point)
 * Computes y0,z0,t0,Y0,Z0,T0 = x/y,x/z,x/t,XX/YY,XX/ZZ,XX/TT
 *
 */
void
theta_precomputation(theta_structure_t *A)
{

    if (!A->precomputation) {
        // temp = (xx,yy,zz,tt)
        theta_point_t temp;
        to_squared_theta(&temp, &A->null_point);
        // Computes t1,t[1],t[2],t[3],t[4],t[5] = 1/y,1/z,1/t,1/YY,1/ZZ,1/TT

        // fp2_t t[6];

        // t[0] = A->null_point.y;
        // t[1] = A->null_point.z;
        // t[2] = A->null_point.t;
        // t[3] = temp.y;
        // t[4] = temp.z;
        // t[5] = temp.t;
        // fp2_batched_inv(t,6);

        // y0,z0,t0,Y0,Z0,T0 = x/y,x/z,x/t,XX/YY,XX/ZZ,XX/TT
        // fp2_mul(&A->y0,&t[0],&A->null_point.x);
        // fp2_mul(&A->z0,&t[1],&A->null_point.x);
        // fp2_mul(&A->t0,&t[2],&A->null_point.x);
        // fp2_mul(&A->Y0,&t[3],&temp.x);
        // fp2_mul(&A->Z0,&t[4],&temp.x);
        // fp2_mul(&A->T0,&t[5],&temp.x);
        fp2_t t1, t2;
        fp2_mul(&t1, &temp.x, &temp.y);
        fp2_mul(&t2, &temp.z, &temp.t);
        fp2_mul(&A->XYZ0, &t1, &temp.z);
        fp2_mul(&A->XYT0, &t1, &temp.t);
        fp2_mul(&A->YZT0, &t2, &temp.y);
        fp2_mul(&A->XZT0, &t2, &temp.x);

        fp2_mul(&t1, &A->null_point.x, &A->null_point.y);
        fp2_mul(&t2, &A->null_point.z, &A->null_point.t);
        fp2_mul(&A->xyz0, &t1, &A->null_point.z);
        fp2_mul(&A->xyt0, &t1, &A->null_point.t);
        fp2_mul(&A->yzt0, &t2, &A->null_point.y);
        fp2_mul(&A->xzt0, &t2, &A->null_point.x);

        // fp2_mul(&A->XY0,&A->X0,&A->Y0);
        // fp2_mul(&A->ZT0,&A->Z0,&A->T0);
        // fp2_mul(&A->xy0,&A->null_point.x,&A->null_point.y);
        // fp2_mul(&A->zt0,&A->null_point.z,&A->null_point.t);

        A->precomputation = 1;
    }
}

/**
 * @brief Compute the double of the theta point in on the theta struc A
 *
 * @param out Output: the theta_point
 * @param A a theta structure
 * @param in a theta point in the theta structure A
 * in = (x,y,z,t)
 * out = [2] (x,y,z,t)
 * /!\ assumes that no coordinates is zero
 *
 */
void
double_point(theta_point_t *out, theta_structure_t *A, const theta_point_t *in)
{

    to_squared_theta(out, in);
    fp2_sqr(&out->x, &out->x);
    fp2_sqr(&out->y, &out->y);
    fp2_sqr(&out->z, &out->z);
    fp2_sqr(&out->t, &out->t);
    if (!A->precomputation) {
        theta_precomputation(A);
    }
    // fp2_mul(&out->x,&out->x,&A->X0);
    // fp2_mul(&out->y,&out->y,&A->Y0);
    // fp2_mul(&out->z,&out->z,&A->Z0);
    // fp2_mul(&out->t,&out->t,&A->T0);

    fp2_mul(&out->x, &out->x, &A->YZT0);
    fp2_mul(&out->y, &out->y, &A->XZT0);
    fp2_mul(&out->z, &out->z, &A->XYT0);
    fp2_mul(&out->t, &out->t, &A->XYZ0);

    // fp2_mul(&out->x,&out->x,&A->ZT0);
    // fp2_mul(&out->y,&out->y,&A->ZT0);
    // fp2_mul(&out->z,&out->z,&A->XY0);
    // fp2_mul(&out->t,&out->t,&A->XY0);

    hadamard(out, out);
    fp2_mul(&out->x, &out->x, &A->yzt0);
    fp2_mul(&out->y, &out->y, &A->xzt0);
    fp2_mul(&out->z, &out->z, &A->xyt0);
    fp2_mul(&out->t, &out->t, &A->xyz0);

    // fp2_mul(&out->x,&out->x,&A->zt0);
    // fp2_mul(&out->y,&out->y,&A->zt0);
    // fp2_mul(&out->z,&out->z,&A->xy0);
    // fp2_mul(&out->t,&out->t,&A->xy0);

    // fp2_mul(&out->y,&out->y,&A->y0);
    // fp2_mul(&out->z,&out->z,&A->z0);
    // fp2_mul(&out->t,&out->t,&A->t0);
}

/**
 * @brief Compute the iterated double of the theta point in on the theta struc A
 *
 * @param out Output: the theta_point
 * @param A a theta structure
 * @param in a theta point in the theta structure A
 * @param exp the exponent
 * in = (x,y,z,t)
 * out = [2^2] (x,y,z,t)
 * /!\ assumes that no coordinates is zero and that the precomputation of A has been done
 *
 */
void
double_iter(theta_point_t *out, theta_structure_t *A, const theta_point_t *in, int exp)
{
    if (exp == 0) {
        fp2_copy(&out->x, &in->x);
        fp2_copy(&out->y, &in->y);
        fp2_copy(&out->z, &in->z);
        fp2_copy(&out->t, &in->t);
    } else {
        double_point(out, A, in);
        for (int i = 1; i < exp; i++) {
            double_point(out, A, out);
        }
    }
}

/**
 * @brief Compute the differential addition of two theta points in the theta struc A
 *
 * @param R Output: the theta_point
 * @param A a theta structure
 * @param P a theta point in the theta structure A
 * @param Q a theta point in the theta structure A
 * @param PQ the theta point P-Q in the theta structure A
 * R = P+Q
 * /!\ assumes that no coordinates is zero and that the precomputation of A has been done
 *
 */
void
diff_add(theta_point_t *R,
         const theta_structure_t *A,
         const theta_point_t *P,
         const theta_point_t *Q,
         const theta_point_t *PQ)
{

    assert(0);
    // not sure this is correct!

    // theta_point_t tP,tQ;
    // // tP = H(P), tQ = H(Q)
    // hadamard(&tP,P);
    // hadamard(&tQ,Q);

    // fp2_mul(&R->x,&tP.x,&tQ.x);
    // fp2_mul(&R->y,&tP.y,&tQ.y);
    // fp2_mul(&R->y,&R->y,&A->Y0);
    // fp2_mul(&R->z,&tP.z,&tQ.z);
    // fp2_mul(&R->z,&R->z,&A->Z0);
    // fp2_mul(&R->t,&tP.t,&tQ.t);
    // fp2_mul(&R->t,&R->t,&A->T0);

    // fp2_t t1,t2;
    // fp2_mul(&t1,&PQ->x,&PQ->y);
    // fp2_mul(&t2,&PQ->z,&PQ->t);

    // hadamard(R,R);
    // fp2_mul(&R->x,&R->x,&t2);
    // fp2_mul(&R->x,&R->x,&PQ->y);
    // fp2_mul(&R->y,&R->y,&t2);
    // fp2_mul(&R->y,&R->y,&PQ->x);
    // fp2_mul(&R->z,&R->z,&t1);
    // fp2_mul(&R->z,&R->z,&PQ->t);
    // fp2_mul(&R->t,&R->t,&t1);
    // fp2_mul(&R->t,&R->t,&PQ->z);
}

/// TODO : see if this needed, and if yes, finish to implement correctly
// /**
//  * @brief Compute the scalar multiplication of a point in the theta struc A
//  *
//  * @param R Output: the theta_point
//  * @param A a theta structure
//  * @param P a theta point in the theta structure A
//  * @param k a scalar
//  * R = [k] P
//  * /!\ assumes that no coordinates is zero and that the precomputation of A has been done
//  *
//    */
// void mul(theta_point_t* R, theta_structure_t const* A, digit_t const* k, theta_point_t const* P)
// {
//     theta_point_t R0, R1, R2;
//     digit_t mask;
//     unsigned int bit = 0, prevbit = 0, swap;

//     // R0 <- P, R1 <- P, R2 <- [2]P
//     fp2_copy(&R1.x, &P->x);
//     fp2_copy(&R1.z, &P->z);
//     fp2_copy(&R1.y, &P->y);
//     fp2_copy(&R1.t, &P->t);
//     fp2_copy(&R0.x, &P->x);
//     fp2_copy(&R0.z, &P->z);
//     fp2_copy(&R0.y, &P->y);
//     fp2_copy(&R0.t, &P->t);

//     double_point(&R2,&A,P);

//     // Main loop
//     for (int i = BITS-1; i >= 0; i--) {
//         bit = (k[i >> LOG2RADIX] >> (i & (RADIX-1))) & 1;
//         swap = bit ^ prevbit;
//         prevbit = bit;
//         mask = 0 - (digit_t)swap;

//         swap_points(&R0, &R1, mask);
//         xDBLADD(&R0, &R1, &R0, &R1, P, &A24);
//     }
//     swap = 0 ^ prevbit;
//     mask = 0 - (digit_t)swap;
//     swap_points(&R0, &R1, mask);

//     fp2_copy(&Q->x, &R0.x);
//     fp2_copy(&Q->z, &R0.z);
// }
