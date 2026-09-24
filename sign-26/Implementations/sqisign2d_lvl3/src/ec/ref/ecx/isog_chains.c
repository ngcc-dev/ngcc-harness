#include "ec.h"
#include "isog.h"
#include "fp2.h"
#include <assert.h>

// since we use degree 4 isogeny steps, we need to handle the odd case with care
static void
ec_eval_even_strategy(ec_curve_t *image,
                      ec_point_t *points,
                      unsigned short points_len,
                      ec_point_t *A24,
                      const ec_point_t *kernel,
                      const int isog_len)
{

    ec_kps4_t kps;

    int log2_of_e, tmp;
    int e_half = (isog_len) >> 1;
    log2_of_e = 0;
    for (tmp = e_half, log2_of_e = 0; tmp > 0; tmp >>= 1, ++log2_of_e);
               

    log2_of_e *= 2; // In order to ensure each splits is at most size log2_of_e
    ec_point_t SPLITTING_POINTS[log2_of_e], K2;
    copy_point(&SPLITTING_POINTS[0], kernel);

    int strategy = 0, // Current element of the strategy to be used
        i, j;

    int BLOCK = 0,        // Keeps track of point order
        current = 0;      // Number of points being carried
    int XDBLs[log2_of_e]; // Number of doubles performed

    // If walk length is odd, we start with a 2-isogeny
    int is_odd = isog_len % 2;
    // if the length is long enough we normalize the first step
    // TODO: should we normalise throughout the chain too? This only
    // helps for the first step
    if (isog_len > 50) {
        ec_normalize_point(A24);
    }

    // Chain of 4-isogenies
    for (j = 0; j < (e_half - 1); j++) {
        // Get the next point of order 4
        while (BLOCK != (e_half - 1 - j)) {
            // A new split will be added
            current += 1;
            // We set the seed of the new split to be computed and saved
            copy_point(&SPLITTING_POINTS[current], &SPLITTING_POINTS[current - 1]);
                                 //   printf("*************************************1\n");
            // if we copied from the very first element, then we perform one additional doubling
            if (is_odd && current == 1) {
                if (j == 0) {
                    assert(fp2_is_one(&A24->z));
                                                      //  printf("*************************************2\n");

                    xDBL_A24_normalized(
                        &SPLITTING_POINTS[current], &SPLITTING_POINTS[current], A24);

                } else {
                    xDBL_A24(&SPLITTING_POINTS[current], &SPLITTING_POINTS[current], A24);

                }
            }
            for (i = 0; i < 2 * STRATEGY4[TORSION_PLUS_EVEN_POWER - isog_len][strategy]; i++)

                if (j == 0) {
                    assert(fp2_is_one(&A24->z));

                    xDBL_A24_normalized(
                        &SPLITTING_POINTS[current], &SPLITTING_POINTS[current], A24);

                } else {
                    xDBL_A24(&SPLITTING_POINTS[current], &SPLITTING_POINTS[current], A24);

                }
            XDBLs[current] = STRATEGY4[TORSION_PLUS_EVEN_POWER - isog_len]
                                      [strategy]; // The number of doublings performed is saved

            BLOCK += STRATEGY4[TORSION_PLUS_EVEN_POWER - isog_len]
                              [strategy]; // BLOCK is increased by the number of doublings performed

            strategy += 1;                // Next, we move to the next element of the strategy
        }

        if (j == 0) {

            assert(current > 0);

            ec_point_t T;
            assert(fp2_is_one(&A24->z));
            xDBL_A24_normalized(&T, &SPLITTING_POINTS[current], A24);
            if (fp2_is_zero(&T.x)) {
                xisog_4_singular(&kps, A24, SPLITTING_POINTS[current], *A24);
                xeval_4_singular(
                    SPLITTING_POINTS, SPLITTING_POINTS, current, SPLITTING_POINTS[current], &kps);

                // Evaluate points
                if (points_len)
                    xeval_4_singular(points, points, points_len, SPLITTING_POINTS[current], &kps);
            } else {
                xisog_4(&kps, A24, SPLITTING_POINTS[current]);
                xeval_4(SPLITTING_POINTS, SPLITTING_POINTS, current, &kps);

                // Evaluate points
                if (points_len)
                    xeval_4(points, points, points_len, &kps);
            }
        } else {
            if (is_odd && current == 0) {
                xDBL_A24(&SPLITTING_POINTS[current], &SPLITTING_POINTS[current], A24);
            }


#ifndef NDEBUG
            // printf("%d \n",current);
            assert(!fp2_is_zero(&SPLITTING_POINTS[current].z));
            ec_point_t test;
            copy_point(&test, &SPLITTING_POINTS[current]);
            xDBL_A24(&test, &test, A24);
            assert(!fp2_is_zero(&test.z));
            xDBL_A24(&test, &test, A24);
            assert(fp2_is_zero(&test.z));
#endif
            // Evaluate 4-isogeny
            xisog_4(&kps, A24, SPLITTING_POINTS[current]);
            xeval_4(SPLITTING_POINTS, SPLITTING_POINTS, current, &kps);
            if (points_len)
                xeval_4(points, points, points_len, &kps);
        }

        BLOCK -= XDBLs[current];
        XDBLs[current] = 0;
        current -= 1;
    }
    // Final 4-isogeny
    if (is_odd) {
        current = 1;
        copy_point(&SPLITTING_POINTS[1], &SPLITTING_POINTS[0]);
        xDBL_A24(&SPLITTING_POINTS[current], &SPLITTING_POINTS[current], A24);
    }
    xisog_4(&kps, A24, SPLITTING_POINTS[current]);
    if (points_len)
        xeval_4(points, points, points_len, &kps);

    // current-=1;
    // final 2-isogeny
    if (is_odd) {
        xeval_4(SPLITTING_POINTS, SPLITTING_POINTS, 1, &kps);

#ifndef NDEBUG
        assert(!fp2_is_zero(&SPLITTING_POINTS[0].z));
        ec_point_t test;
        copy_point(&test, &SPLITTING_POINTS[0]);
        xDBL_A24(&test, &test, A24);
        assert(fp2_is_zero(&test.z));

#endif

        ec_kps2_t kps;
        xisog_2(&kps, A24, SPLITTING_POINTS[0]);
        if (points_len)
            xeval_2(points, points, points_len, &kps);
    }

    // Output curve in the form (A:C)
    A24_to_AC(image, A24);

    // TODO:
    // The curve does not have A24 normalised though
    // should we normalise it here, or do it later?
    image->is_A24_computed_and_normalized = false;
}

void
ec_eval_even(ec_curve_t *image, ec_isog_even_t *phi, ec_point_t *points, unsigned short length)
{
    ec_curve_normalize_A24(&phi->curve);
    ec_eval_even_strategy(image, points, length, &phi->curve.A24, &phi->kernel, phi->length);
}

// naive implementation
void
ec_eval_small_chain(ec_curve_t *image,
                    const ec_point_t *kernel,
                    int len,
                    ec_point_t *points,
                    int len_points)
{

    ec_point_t A24;
    AC_to_A24(&A24, image);

    ec_kps2_t kps;

    ec_point_t small_K, big_K;
    copy_point(&big_K, kernel);

    for (int i = 0; i < len; i++) {
        copy_point(&small_K, &big_K);
        // small_K = big_K;
        for (int j = 0; j < len - i - 1; j++) {
            xDBL_A24(&small_K, &small_K, &A24);
        }
        if (fp2_is_zero(&small_K.x)) {
            ec_point_t B24;
            xisog_2_singular(&kps, &B24, A24);
            xeval_2_singular(&big_K, &big_K, 1, &kps);
            xeval_2_singular(points, points, len_points, &kps);
            copy_point(&A24, &B24);
        } else {
            xisog_2(&kps, &A24, small_K);
            xeval_2(&big_K, &big_K, 1, &kps);
            xeval_2(points, points, len_points, &kps);
        }
    }
    A24_to_AC(image, &A24);

    // TODO:
    // The curve does not have A24 normalised though
    // should we normalise it here, or do it later?
    image->is_A24_computed_and_normalized = false;
}

void
ec_eval_three_rec(ec_point_t *A24,
                  unsigned int length_path,
                  ec_point_t *ker,
                  ec_point_t *stack,
                  unsigned short length_stack,
                  ec_point_t *points,
                  unsigned short length)
{

    if (length_path == 0)
        return;
    if (length_path == 1) {
        ec_point_t B24;
        kps(0, *ker, *A24);
        xisog(&B24, 0, *A24);
        for (int j = 0; j < length_stack; j++)
            xeval(&stack[j], 0, stack[j], *A24);
        for (int j = 0; j < length; j++)
            xeval(&points[j], 0, points[j], *A24);
        copy_point(A24, &B24);
        return;
    }

    long right = length_path / 1.5;
    long left = length_path - right;

    copy_point(&(stack[length_stack]), ker);

    for (int j = 0; j < left; j++)
        xMULv2(ker, ker, &(TORSION_ODD_PRIMES[0]), p_plus_minus_bitlength[0], A24);

    ec_eval_three_rec(A24, right, ker, stack, length_stack + 1, points, length);

    copy_point(ker, &(stack[length_stack]));

    ec_eval_three_rec(A24, left, ker, stack, length_stack, points, length);

    // ibz_finalize(&pow);
}

void
ec_eval_three(ec_curve_t *image,
              const ec_isog_odd_t *phi,
              ec_point_t *points,
              unsigned short length)
{

    ec_point_t ker, A24;
    ec_point_t stack[TORSION_PLUS_ODD_POWERS[0]]; // need much smaller stack but okay...
    int i, j, k;

    AC_to_A24(&A24, &phi->curve);
    for (i = 1; i < P_LEN + M_LEN; i++) {
        assert(phi->degree[i] == 0);
    }

    copy_point(&ker, &phi->ker_plus);
    ec_eval_three_rec(&A24, phi->degree[0], &ker, stack, 0, points, length);

    A24_to_AC(image, &A24);

    // TODO:
    // The curve does not have A24 normalised though
    // should we normalise it here, or do it later?
    image->is_A24_computed_and_normalized = 0;
}

void
ec_eval_odd(ec_curve_t *image, const ec_isog_odd_t *phi, ec_point_t *points, unsigned short length)
{

    ec_point_t ker_plus, ker_minus, P, K, A24, B24;
    int i, j, k;

    AC_to_A24(&A24, &phi->curve);

    // Isogenies with kernel in E[p+1]
    copy_point(&ker_plus, &phi->ker_plus);
    copy_point(&ker_minus, &phi->ker_minus);
    for (i = 0; i < P_LEN; i++) {
        copy_point(&P, &ker_plus);
        for (j = i + 1; j < P_LEN; j++) {
            for (k = 0; k < phi->degree[j]; k++)
                xMULv2(&P, &P, &(TORSION_ODD_PRIMES[j]), p_plus_minus_bitlength[j], &A24);
        }
        for (k = 0; k < phi->degree[i]; k++) {
            copy_point(&K, &P);
            for (j = 0; j < phi->degree[i] - k - 1; j++)
                xMULv2(&K, &K, &(TORSION_ODD_PRIMES[i]), p_plus_minus_bitlength[i], &A24);
            kps(i, K, A24);
            xisog(&B24, i, A24);
            xeval(&P, i, P, A24);
            xeval(&ker_plus, i, ker_plus, A24);
            xeval(&ker_minus, i, ker_minus, A24);
            for (j = 0; j < length; j++)
                xeval(&points[j], i, points[j], A24);
            copy_point(&A24, &B24);
        }
    }
    // Isogenies with kernel in E[p-1]
    for (i = P_LEN; i < P_LEN + M_LEN; i++) {
        copy_point(&P, &ker_minus);
        for (j = i + 1; j < P_LEN + M_LEN; j++) {
            for (k = 0; k < phi->degree[j]; k++)
                xMULv2(&P, &P, &(TORSION_ODD_PRIMES[j]), p_plus_minus_bitlength[j], &A24);
        }  
        for (k = 0; k < phi->degree[i]; k++) {
            copy_point(&K, &P);
            for (j = 0; j < phi->degree[i] - k - 1; j++)
                xMULv2(&K, &K, &(TORSION_ODD_PRIMES[i]), p_plus_minus_bitlength[i], &A24);
            kps(i, K, A24);
            xisog(&B24, i, A24);
            xeval(&P, i, P, A24);
            xeval(&ker_minus, i, ker_minus, A24);
            for (j = 0; j < length; j++)
                xeval(&points[j], i, points[j], A24);
            copy_point(&A24, &B24);
        }
    }

    A24_to_AC(image, &A24);

    // TODO:
    // The curve does not have A24 normalised though
    // should we normalise it here, or do it later?
    image->is_A24_computed_and_normalized = 0;
}


void
ec_eval_odd_minus(ec_curve_t *image, const ec_isog_odd_t *phi, ec_point_t *points, unsigned short length)
{

    ec_point_t ker_minus, P, K, A24, B24;
    int i, j, k;

    AC_to_A24(&A24, &phi->curve);
    copy_point(&ker_minus, &phi->ker_minus);
    // Isogenies with kernel in E[p-1]
    for (i = P_LEN; i < P_LEN + M_LEN; i++) {
        copy_point(&P, &ker_minus);

        for (j = i + 1; j < P_LEN + M_LEN; j++) {
            for (k = 0; k < phi->degree[j]; k++)
                xMULv2(&P, &P, &(TORSION_ODD_PRIMES[j]), p_plus_minus_bitlength[j], &A24);
        }  

        for (k = 0; k < phi->degree[i]; k++) {
            copy_point(&K, &P);
            for (j = 0; j < phi->degree[i] - k - 1; j++)
                xMULv2(&K, &K, &(TORSION_ODD_PRIMES[i]), p_plus_minus_bitlength[i], &A24);
            kps(i, K, A24);
            xisog(&B24, i, A24);
            xeval(&P, i, P, A24);
            xeval(&ker_minus, i, ker_minus, A24);
            //point_print("xP0:=F4!",ker_minus);
            for (j = 0; j < length; j++)
                xeval(&points[j], i, points[j], A24);
            copy_point(&A24, &B24);
        }
        // point_print("xP0:=F4!",ker_minus);
        // A24_to_AC(image, &B24);
        // curve_print("a:=", *image);
    }

    A24_to_AC(image, &A24);

    // TODO:
    // The curve does not have A24 normalised though
    // should we normalise it here, or do it later?
    image->is_A24_computed_and_normalized = 0;
}

void
ec_isomorphism(ec_isom_t *isom, const ec_curve_t *from, const ec_curve_t *to)
{
    fp2_t t0, t1, t2, t3, t4;
    fp2_mul(&t0, &from->A, &to->C);
    fp2_sqr(&t0, &t0); // fromA^2toC^2
    fp2_mul(&t1, &to->A, &from->C);
    fp2_sqr(&t1, &t1); // toA^2fromC^2
    fp2_mul(&t2, &to->C, &from->C);
    fp2_sqr(&t2, &t2); // toC^2fromC^2
    fp2_add(&t3, &t2, &t2);
    fp2_add(&t2, &t3, &t2); // 3toC^2fromC^2
    fp2_sub(&t3, &t2, &t0); // 3toC^2fromC^2-fromA^2toC^2
    fp2_sub(&t4, &t2, &t1); // 3toC^2fromC^2-toA^2fromC^2
    fp2_inv(&t3);
    fp2_mul(&t4, &t4, &t3);
    fp2_sqrt(&t4); // lambda^2 constant for SW isomorphism
    fp2_sqr(&t3, &t4);
    fp2_mul(&t3, &t3, &t4); // lambda^6

    // Check sign of lambda^2, such that lambda^6 has the right sign
    fp2_sqr(&t0, &from->C);
    fp2_add(&t1, &t0, &t0);
    fp2_add(&t1, &t1, &t1);
    fp2_add(&t1, &t1, &t1);
    fp2_add(&t0, &t0, &t1); // 9fromC^2
    fp2_sqr(&t2, &from->A);
    fp2_add(&t2, &t2, &t2); // 2fromA^2
    fp2_sub(&t2, &t2, &t0);
    fp2_mul(&t2, &t2, &from->A); // -9fromC^2fromA+2fromA^3
    fp2_sqr(&t0, &to->C);
    fp2_mul(&t0, &t0, &to->C);
    fp2_mul(&t2, &t2, &t0); // toC^3* [-9fromC^2fromA+2fromA^3]
    fp2_mul(&t3, &t3, &t2); // lambda^6*(-9fromA+2fromA^3)*toC^3
    fp2_sqr(&t0, &to->C);
    fp2_add(&t1, &t0, &t0);
    fp2_add(&t1, &t1, &t1);
    fp2_add(&t1, &t1, &t1);
    fp2_add(&t0, &t0, &t1); // 9toC^2
    fp2_sqr(&t2, &to->A);
    fp2_add(&t2, &t2, &t2); // 2toA^2
    fp2_sub(&t2, &t2, &t0);
    fp2_mul(&t2, &t2, &to->A); // -9toC^2toA+2toA^3
    fp2_sqr(&t0, &from->C);
    fp2_mul(&t0, &t0, &from->C);
    fp2_mul(&t2, &t2, &t0); // fromC^3* [-9toC^2toA+2toA^3]
    if (!fp2_is_equal(&t2, &t3))
        fp2_neg(&t4, &t4);

    // Mont -> SW -> SW -> Mont
    fp2_set_one(&t0);
    fp2_add(&isom->D, &t0, &t0);
    fp2_add(&isom->D, &isom->D, &t0);
    fp2_mul(&isom->D, &isom->D, &from->C);
    fp2_mul(&isom->D, &isom->D, &to->C);
    fp2_mul(&isom->Nx, &isom->D, &t4);
    fp2_mul(&t4, &t4, &from->A);
    fp2_mul(&t4, &t4, &to->C);
    fp2_mul(&t0, &to->A, &from->C);
    fp2_sub(&isom->Nz, &t0, &t4);
}

void
ec_iso_eval(ec_point_t *P, ec_isom_t *isom)
{
    fp2_t tmp;
    fp2_mul(&P->x, &P->x, &isom->Nx);
    fp2_mul(&tmp, &P->z, &isom->Nz);
    fp2_sub(&P->x, &P->x, &tmp);
    fp2_mul(&P->z, &P->z, &isom->D);
}


void ec_curve_normalize(ec_curve_t *new, ec_isom_t *isom, const ec_curve_t *old){
    fp2_t t0, t1, t2, t3, t4, t5;
    // Compute the other solutions:
    // A'^2 = [ sqrt(A^2-4C^2)*(9C^2-A^2) +- (A^3-3AC^2) ] / [ 2C^2*sqrt(A^2-4C^2) ]
    fp2_sqr(&t0, &old->C);      //C^2
    fp2_add(&t1, &t0, &t0);     //2C^2
    fp2_add(&t2, &t1, &t1);     //4C^2
    fp2_sqr(&t3, &old->A);      //A^2
    fp2_sub(&t2, &t3, &t2);     //A^2-4C^2
    fp2_sqrt(&t2);              //sqrt(A^2-4C^2)
    fp2_add(&t0, &t0, &t1);     //3C^2
    fp2_mul(&t1, &t2, &t1);     //2C^2*sqrt(A^2-4C^2)
    fp2_sub(&t5, &t3, &t0);     //A^2-3C^2
    fp2_mul(&t5, &t5, &old->A);     //A^3-3AC^2
    fp2_add(&t4, &t0, &t0);     //6C^2
    fp2_add(&t0, &t4, &t0);     //9C^2
    fp2_sub(&t0, &t0, &t3);     //9C^2-A^2
    fp2_add(&t3, &t3, &t3);     //2A^2
    fp2_mul(&t3, &t3, &t2);     //2A^2*sqrt(A^2-4C^2)
    fp2_mul(&t2, &t2, &t0);     //sqrt(A^2-4C^2)*(9C^2-A^2)
    fp2_add(&t0, &t2, &t5);     //sqrt(A^2-4C^2)*(9C^2-A^2) + (A^3-3AC^2)
    fp2_sub(&t2, &t2, &t5);     //sqrt(A^2-4C^2)*(9C^2-A^2) - (A^3-3AC^2)
    fp2_inv(&t1);               //1/2C^2*sqrt(A^2-4C^2)
    fp2_mul(&t0, &t0, &t1);     // First solution
    fp2_mul(&t2, &t2, &t1);     // Second solution
    fp2_mul(&t1, &t3, &t1);     // Original solution

    // Chose the lexicographically first solution
    if(fp2_cmp(&t0, &t1)==1)
        fp2_copy(&t0, &t1);
    if(fp2_cmp(&t0, &t2)==1)
        fp2_copy(&t0, &t2);

    // Copy the solution
    fp2_sqrt(&t0);
    ec_curve_t E;
    fp2_copy(&E.A, &t0);
    fp_mont_setone(E.C.re);
    fp_set(E.C.im, 0);
    ec_isomorphism(isom, old, &E);
    fp2_copy(&new->A, &E.A);
    fp2_copy(&new->C, &E.C);
}

