// SPDX-FileCopyrightText: 2026 The Project OSIDH-LD Authors
// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * Some functions related to elliptic curves come from SQIsign
 * (version 1.0) project (https://github.com/SQIsign/the-sqisign/tree/nist-v1)
 */

#ifndef PHELPER_H
#define PHELPER_H

#include <stdio.h>
#include "protocols.h"

static inline void AC_to_A24(ec_point_t *A24, ec_curve_t const *E)
{
    // A24 = (A+2C : 4C)
    fp2_add(&A24->z, &E->C, &E->C);
    fp2_add(&A24->x, &E->A, &A24->z);
    fp2_add(&A24->z, &A24->z, &A24->z);
}

static inline void A24_to_AC(ec_curve_t *E, ec_point_t const *A24)
{
    // (A:C) = ((A+2C)*2-4C : 4C)
    fp2_add(&E->A, &A24->x, &A24->x);
    fp2_sub(&E->A, &E->A, &A24->z);
    fp2_add(&E->A, &E->A, &E->A);
    fp2_copy(&E->C, &A24->z);
}

//If a > b, return 0xFF...FF, else return 0
static inline digit_t compare_gt(int a, int b)
{
    digit_t res = (uint64_t)(a > b) - 1;
    return res;
}

static inline digit_t bool_to_mask(bool a)
{
    digit_t res = (uint64_t)a - 1;
    return res;
}

static void print_cycle(const orient_cycle_t *A)
{
    printf("[");
    for(int i = 0; i < CYCLE_LENGTH; i++)
    {
        printf("[");
        ec_curve_t et;
        fp2_t t;
        A24_to_AC(&et, &((*A)[i].A24));
        fp2_copy(&t, &(et.C));
        fp2_inv(&t);
        fp2_mul(&t, &t, &(et.A));
        fp2_print(t);
        printf(",\n");
        fp2_copy(&t, &((*A)[i].Ps.z));
        fp2_inv(&t);
        fp2_mul(&t, &t, &((*A)[i].Ps.x));
        fp2_print(t);
        printf(",\n");
        fp2_copy(&t, &((*A)[i].Pt.z));
        fp2_inv(&t);
        fp2_mul(&t, &t, &((*A)[i].Pt.x));
        fp2_print(t);
        printf(",\n");
        fp2_copy(&t, &((*A)[i].Qs.z));
        fp2_inv(&t);
        fp2_mul(&t, &t, &((*A)[i].Qs.x));
        fp2_print(t);
        printf(",\n");
        fp2_copy(&t, &((*A)[i].Qt.z));
        fp2_inv(&t);
        fp2_mul(&t, &t, &((*A)[i].Qt.x));
        fp2_print(t);
        if(i == CYCLE_LENGTH - 1)
        {
            printf("]");
        }
        else
        {
            printf("],\n");
        }
    }
    printf("]");
}

static void print_point(const ec_point_t *A)
{
    ec_point_t B;
    copy_point(&B, A);
    ec_normalize(&B);
    fp2_print(B.x);
}

static void print_A24curve(const ec_point_t *A24)
{
    ec_curve_t C;
    A24_to_AC(&C, A24);
    ec_normalize(&C);
    fp2_print(C.A);
}

static void swap_vertices(vertex_t* A, vertex_t* B, const digit_t option)
{ // Swap vertices
  // If option = 0 then P <- P and Q <- Q, else if option = 0xFF...FF then P <- Q and Q <- P
    swap_points(&(A->A24), &(B->A24), option);
    swap_points(&(A->Ps), &(B->Ps), option);
    swap_points(&(A->Pt), &(B->Pt), option);
    swap_points(&(A->Qs), &(B->Qs), option);
    swap_points(&(A->Qt), &(B->Qt), option);
}

static void copy_vertices(vertex_t* A, const vertex_t* B)
{
    copy_point(&(A->A24), &(B->A24));
    copy_point(&(A->Ps), &(B->Ps));
    copy_point(&(A->Pt), &(B->Pt));
    copy_point(&(A->Qs), &(B->Qs));
    copy_point(&(A->Qt), &(B->Qt));
}

static void copy_vertices_swap_kernel(vertex_t* A, const vertex_t* B)
{
    copy_point(&(A->A24), &(B->A24));
    copy_point(&(A->Ps), &(B->Qs));
    copy_point(&(A->Pt), &(B->Qt));
    copy_point(&(A->Qs), &(B->Ps));
    copy_point(&(A->Qt), &(B->Pt));
}

static void copy_orient_cycle(orient_cycle_t* A, const orient_cycle_t* B)
{
    for(int i = 0; i < CYCLE_LENGTH; i++)
    {
        copy_vertices(&((*A)[i]), &((*B)[i]));
    }
}

static inline void swap_pointer(uintptr_t* a, uintptr_t* b, uintptr_t option)
{ // Swap vertices
  // If option = 0 then a <- a and b <- b, else if option = 0xFF...FF then a <- b and b <- a
    *a = ((*a) & (~option)) + ((*b) & option);
    *b = ((*a) & option) + ((*b) & (~option));
}

static inline int ct_min(int a, int b)
{
    int diff = a - b;
    int mask = diff >> (sizeof(int) * 8 - 1);
    return (b & ~mask) | (a & mask);
}

static void ec_isomorphism(ec_isom_t* isom, const ec_curve_t* from, const ec_curve_t* to){
    fp2_t t0, t1, t2, t3, t4;
    fp2_mul(&t0, &from->A, &to->C);
    fp2_sqr(&t0, &t0);                  //fromA^2toC^2
    fp2_mul(&t1, &to->A, &from->C);
    fp2_sqr(&t1, &t1);                  //toA^2fromC^2
    fp2_mul(&t2, &to->C, &from->C);
    fp2_sqr(&t2, &t2);                  //toC^2fromC^2
    fp2_add(&t3, &t2, &t2);
    fp2_add(&t2, &t3, &t2);             //3toC^2fromC^2
    fp2_sub(&t3, &t2, &t0);             //3toC^2fromC^2-fromA^2toC^2
    fp2_sub(&t4, &t2, &t1);             //3toC^2fromC^2-toA^2fromC^2
    fp2_inv(&t3);
    fp2_mul(&t4, &t4, &t3);
    fp2_sqrt(&t4);                      //lambda^2 constant for SW isomorphism
    fp2_sqr(&t3, &t4);
    fp2_mul(&t3, &t3, &t4);             //lambda^6

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
    fp2_mul(&t2, &t2, &t0);     //toC^3* [-9fromC^2fromA+2fromA^3]
    fp2_mul(&t3, &t3, &t2);             //lambda^6*(-9fromA+2fromA^3)*toC^3
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
    fp2_mul(&t2, &t2, &t0);     //fromC^3* [-9toC^2toA+2toA^3]
    // if(!fp2_is_equal(&t2, &t3))
    //     fp2_neg(&t4, &t4);
    fp2_t nt4;
    fp2_neg(&nt4, &t4);
    fp2_swap(&t4, &nt4, bool_to_mask(fp2_is_equal(&t2, &t3)));

    // Mont -> SW -> SW -> Mont
    fp_mont_setone(t0.re);
    fp_set(t0.im, 0);
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

static void ec_iso_eval(ec_point_t *P, ec_isom_t* isom){
    fp2_t tmp;
    fp2_mul(&P->x, &P->x, &isom->Nx);
    fp2_mul(&tmp, &P->z, &isom->Nz);
    fp2_sub(&P->x, &P->x, &tmp);
    fp2_mul(&P->z, &P->z, &isom->D);
}

#endif