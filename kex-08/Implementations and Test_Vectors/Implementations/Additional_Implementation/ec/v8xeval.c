// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from xeval.c in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modifications made:
 *   - Deleted some unused functions
 *   - Changed the data structure to make it suitable for vectorization
 */

#include "v8isog.h"
#include "v8ec.h"
#include <assert.h>

// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// Traditional isogeny evaluation (xEVAL)

// CrissCross procedure as described in Hisil and Costello paper
void v8CrissCross(v8fp2_t *r0, v8fp2_t *r1, v8fp2_t const alpha, v8fp2_t const beta, v8fp2_t const gamma, v8fp2_t const delta)
{
	v8fp2_t t_1, t_2;

	v8fp2_mul(&t_1, &alpha, &delta);
    	v8fp2_mul(&t_2, &beta, &gamma);
	v8fp2_add(&*r0, &t_1, &t_2);
	v8fp2_sub(&*r1, &t_1, &t_2);
}

// Isogeny evaluation on Montgomery curves
// Recall: K has been computed in Twisted Edwards model and none extra additions are required.
void v8xeval_t(v8ec_point_t* Q, uint64_t const i, v8ec_point_t const *P)
{
	int j;
	int d = ((int)TORSION_ODD_PRIMES[i] - 1) / 2;	// Here, l = 2d + 1

	v8fp2_t R0, R1, S0, S1, T0, T1;
	v8fp2_add(&S0, &(P->x), &(P->z));
	v8fp2_sub(&S1, &(P->x), &(P->z));

	v8CrissCross(&R0, &R1, v8K[0].z, v8K[0].x, S0, S1);
	for (j = 1; j < d; j++)
	{
		v8CrissCross(&T0, &T1, v8K[j].z, v8K[j].x, S0, S1);
		v8fp2_mul(&R0, &T0, &R0);
		v8fp2_mul(&R1, &T1, &R1);
	};

	v8fp2_sqr(&R0, &R0);
	v8fp2_sqr(&R1, &R1);

	v8fp2_mul(&(Q->x), &(P->x), &R0);
	v8fp2_mul(&(Q->z), &(P->z), &R1);
}

// -----------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------
// Isogeny evaluation (xEVAL) used in velu SQRT

void v8xeval_s(v8ec_point_t* Q, uint64_t const i, v8ec_point_t const *P, v8ec_point_t const *A)
{
	// =================================================================================
	assert(TORSION_ODD_PRIMES[i] > gap);     // Ensuring velusqrt is used for l_i > gap
	v8sI = sizeI[i];          // size of I
	v8sJ = sizeJ[i];          // size of J
	v8sK = sizeK[i];          // size of K

	assert(v8sI >= v8sJ);       // Ensuring #I >= #J
	assert(v8sK >= 0);        // Recall, it must be that #K >= 0
	assert(v8sJ > 1);         // ensuring sI >= sJ > 1
	// =================================================================================

	// We require the curve coefficient A = A'/C ... well, a multiple of these ones
	v8fp2_t Ap;
	v8fp2_add(&Ap, &(A->x), &(A->x)); // 2A' + 4C
	v8fp2_sub(&Ap, &Ap, &(A->z));   // 2A'
	v8fp2_add(&Ap, &Ap, &Ap);     // 4A'

	//  --------------------------------------------------------------------------------------------------
	//                   ~~~~~~~~
	//                    |    | 
	// Computing E_J(W) = |    | [ F0(W, x([j]P)) * alpha^2 + F1(W, x([j]P)) * alpha + F2(W, x([j]P)) ]
	//                    j in J 
	// In order to avoid costly inverse computations in fp, we are gonna work with projective coordinates
	// In particular, for a degree-l isogeny construction, we need alpha = X/Z and alpha = Z/X (i.e., 1/alpha)

	//fp2_t EJ_0[sJ][3]; // EJ_0[j][2] factors of one polynomial to be used in a resultant 

	v8fp2_t XZ_add, XZj_add,
	   XZ_sub, XZj_sub,
	   AXZ2,
	   CXZ2,
	   CX2Z2,
	   t1, t2;

	v8fp2_add(&XZ_add, &(P->x), &(P->z));	// X + Z
	v8fp2_sub(&XZ_sub, &(P->x), &(P->z));	// X - Z

	v8fp2_mul(&AXZ2, &(P->x), &(P->z));	// X * Z
	v8fp2_sqr(&t1, &(P->x));		// X ^ 2
	v8fp2_sqr(&t2, &(P->z));		// Z ^ 2

	v8fp2_add(&CX2Z2, &t1, &t2);		//      X^2 + Z^2
	v8fp2_mul(&CX2Z2, &CX2Z2, &(A->z));	// C * (X^2 + Z^2)

	v8fp2_add(&AXZ2, &AXZ2, &AXZ2);	//       2 * (X * Z)
	v8fp2_mul(&CXZ2, &AXZ2, &(A->z));	// C  * [2 * (X * Z)]
	v8fp2_mul(&AXZ2, &AXZ2, &Ap);		// A' * [2 * (X * Z)]

	int j;
	for (j = 0; j < v8sJ; j++)
	{
		v8fp2_add(&XZj_add, &v8J[j].x, &v8J[j].z);		// Xj + Zj
		v8fp2_sub(&XZj_sub, &v8J[j].x, &v8J[j].z);		// Xj - Zj

		v8fp2_mul(&t1, &XZ_sub, &XZj_add);			// (X - Z) * (Xj + Zj)
		v8fp2_mul(&t2, &XZ_add, &XZj_sub);			// (X + Z) * (Xj - Zj)

		// ...................................
		// Computing the quadratic coefficient
		v8fp2_sub(&v8EJ_0[j][2], &t1, &t2);			//       2 * [(X*Zj) - (Z*Xj)]
		v8fp2_sqr(&v8EJ_0[j][2], &v8EJ_0[j][2]);			//     ( 2 * [(X*Zj) - (Z*Xj)] )^2
		v8fp2_mul(&v8EJ_0[j][2], &(A->z), &v8EJ_0[j][2]);		// C * ( 2 * [(X*Zj) - (Z*Xj)] )^2

		// ..................................
		// Computing the constant coefficient
		v8fp2_add(&v8EJ_0[j][0], &t1, &t2);			//       2 * [(X*Xj) - (Z*Zj)]
		v8fp2_sqr(&v8EJ_0[j][0], &v8EJ_0[j][0]);			//     ( 2 * [(X*Xj) - (Z*Zj)] )^2
		v8fp2_mul(&v8EJ_0[j][0], &(A->z), &v8EJ_0[j][0]);		// C * ( 2 * [(X*Xj) - (Z*Zj)] )^2

		// ................................
		// Computing the linear coefficient
	
		// C * [ (-2*Xj*Zj)*(alpha^2 + 1) + (-2*alpha)*(Xj^2 + Zj^2)] + [A' * (-2*Xj*Zj) * (2*X*Z)] where alpha = X/Z
		v8fp2_add(&t1, &v8J[j].x, &v8J[j].z);			//      (Xj + Zj)
		v8fp2_sqr(&t1, &t1);					//      (Xj + Zj)^2
		v8fp2_add(&t1, &t1, &t1);				//  2 * (Xj + Zj)^2
		v8fp2_add(&t1, &t1, &v8XZJ4[j]);			//  2 * (Xj + Zj)^2 - (4*Xj*Zj) := 2 * (Xj^2 + Zj^2)
		v8fp2_mul(&t1, &t1, &CXZ2);				// [2 * (Xj^2 + Zj^2)] * (2 * [ C * (X * Z)])

		v8fp2_mul(&t2, &CX2Z2, &v8XZJ4[j]);			// [C * (X^2 + Z^2)] * (-4 * Xj * Zj)
		v8fp2_sub(&t1, &t2, &t1);				// [C * (X^2 + Z^2)] * (-4 * Xj * Zj) - [2 * (Xj^2 + Zj^2)] * (2 * [ C * (X * Z)])

		v8fp2_mul(&t2, &AXZ2, &v8XZJ4[j]);			// (2 * [A' * (X * Z)]) * (-4 * Xj * Zj)
		v8fp2_add(&v8EJ_0[j][1], &t1, &t2);			// This is our desired equation but multiplied by 2
		v8fp2_add(&v8EJ_0[j][1], &v8EJ_0[j][1], &v8EJ_0[j][1]);	// This is our desired equation but multiplied by 4
	};

        // ---------------------------------------------------------------------
        // The faster way for multiplying is using a divide-and-conquer approach

	// product tree of EJ_0 (we only require the root)
	v8product_tree_LENFeq3(v8ptree_EJ, v8deg_ptree_EJ, 0, v8EJ_0, v8sJ);
	assert( v8deg_ptree_EJ[0] == (2*v8sJ) );
	if (!scaled)
	{
		// unscaled remainder tree approach
		v8multieval_unscaled(v8leaves, v8ptree_EJ[0], 2*v8sJ + 1, v8rtree_hI, (const v8fp2_t*)v8rtree_A, v8ptree_hI, v8deg_ptree_hI, 0, v8sI);
	}
	else
	{
		// scaled remainder tree approach
		v8fp2_t G[sI_max], G_rev[sI_max];
		v8poly_redc(G, v8ptree_EJ[0], 2*v8sJ + 1, v8ptree_hI[0], v8sI + 1, v8R0, v8A0);
		for (j = 0; j < v8sI; j++)
			v8fp2_copy(&G_rev[j], &G[v8sI - 1 - j]);

		v8poly_mul_middle(G_rev, G_rev, v8sI, v8R0, v8sI);
		for (j = 0; j < v8sI; j++)
			v8fp2_copy(&G[j], &G_rev[v8sI - 1 - j]);

		v8multieval_scaled(v8leaves, G, v8ptree_hI, v8deg_ptree_hI, 0, v8sI);
        };

	// Finally, we must multiply the leaves of the outpur of remainders
	v8fp2_t r0;
	v8product(&r0, (const v8fp2_t*)v8leaves, v8sI);
	// EJ_1 is just reverting the ordering in the coefficients of EJ_0
	for (j = 0; j < v8sJ; j++){
		v8fp2_copy(&t1, &v8ptree_EJ[0][j]);
		v8fp2_copy(&v8ptree_EJ[0][j], &v8ptree_EJ[0][2*v8sJ - j]);
		v8fp2_copy(&v8ptree_EJ[0][2*v8sJ - j], &t1);
	}

	if (!scaled)
	{
		// unscaled remainder tree approach
		v8multieval_unscaled(v8leaves, v8ptree_EJ[0], 2*v8sJ + 1, v8rtree_hI, (const v8fp2_t*)v8rtree_A, v8ptree_hI, v8deg_ptree_hI, 0, v8sI);
	}
	else
	{
		// scaled remainder tree approach
		v8fp2_t G[sI_max], G_rev[sI_max];
		v8poly_redc(G, v8ptree_EJ[0], 2*v8sJ + 1, v8ptree_hI[0], v8sI + 1, v8R0, v8A0);
		for (j = 0; j < v8sI; j++)
			v8fp2_copy(&G_rev[j], &G[v8sI - 1 - j]);

		v8poly_mul_middle(G_rev, G_rev, v8sI, v8R0, v8sI);
		for (j = 0; j < v8sI; j++)
			v8fp2_copy(&G[j], &G_rev[v8sI - 1 - j]);

		v8multieval_scaled(v8leaves, G, v8ptree_hI, v8deg_ptree_hI, 0, v8sI);
        };
	v8clear_tree(v8ptree_EJ, 0, v8sJ);
	// Finally, we must multiply the leaves of the outpur of remainders
	v8fp2_t r1;
	v8product(&r1, (const v8fp2_t*)v8leaves, v8sI);

	// -------------------------------
	// Sometimes the public value sK is equal to zero,
	// Thus for avoing runtime error we add one when sK =0
	v8fp2_t hK_0[sK_max + 1], hK_1[sK_max + 1], hk_0, hk_1;
	for (j = 0; j < v8sK; j++)
	{
		v8fp2_add(&XZj_add, &v8K[j].x, &v8K[j].z);	// Xk + Zk
		v8fp2_sub(&XZj_sub, &v8K[j].x, &v8K[j].z);	// Xk - Zk
		v8fp2_mul(&t1, &XZ_sub, &XZj_add);		// (X - Z) * (Xk + Zk)
		v8fp2_mul(&t2, &XZ_add, &XZj_sub);		// (X + Z) * (Xk - Zk)

		// Case alpha = X/Z
		v8fp2_sub(&hK_0[j], &t1, &t2);		// 2 * [(X*Zk) - (Z*Xk)]

		// Case 1/alpha = Z/X
		v8fp2_add(&hK_1[j], &t1, &t2);		// 2 * [(X*Xk) - (Z*Zk)]
	};

	// hk_0 <- use product to mulitiply all the elements in hK_0
	v8product(&hk_0, (const v8fp2_t*)hK_0, v8sK);
	// hk_1 <- use product to mulitiply all the elements in hK_1
	v8product(&hk_1, (const v8fp2_t*)hK_1, v8sK);

	// ---------------------------------------------------------------------------------
	// Now, unifying all the computations
	v8fp2_mul(&t1, &hk_1, &r1);				// output of algorithm 2 with 1/alpha = Z/X and without the demoninator
	v8fp2_sqr(&t1, &t1);
	v8fp2_mul(&(Q->x), &t1, &(P->x));

	v8fp2_mul(&t2, &hk_0, &r0);				// output of algorithm 2 with alpha = X/Z and without the demoninator
	v8fp2_sqr(&t2, &t2);
	v8fp2_mul(&(Q->z), &t2, &(P->z));
}
