// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from xisog.c in the SQIsign (version 1.0) 
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

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

// xISOG procedure, which is a hybrid between Montgomery and Twisted Edwards
// This tradition fomulae corresponds with the Twisted Edwards formulae but 
// mapping the output into Montgomery form
void v8xisog_t(v8ec_point_t* B, uint64_t const i, v8ec_point_t const *A)
{
	int j;
	int d = ((int)TORSION_ODD_PRIMES[i] - 1) / 2;	// Here, l = 2d + 1

	v8fp2_t By, Bz, constant_d_edwards, tmp_a, tmp_d;

	v8fp2_copy(&By, &v8K[0].x);
	v8fp2_copy(&Bz, &v8K[0].z);

	for (j = 1; j < d; j++)
	{
		v8fp2_mul(&By, &By, &v8K[j].x);
		v8fp2_mul(&Bz, &Bz, &v8K[j].z);
	};

	// Mapping Montgomery curve coefficients into Twisted Edwards form
	v8fp2_sub(&constant_d_edwards, &(A->x), &(A->z));
	v8fp2_copy(&tmp_a, &(A->x));
	v8fp2_copy(&tmp_d, &constant_d_edwards);

	// left-to-right method for computing a^l and d^l
	for (j = 1; j < (int)p_plus_minus_bitlength[i]; j++)
	{
		v8fp2_sqr(&tmp_a, &tmp_a);
		v8fp2_sqr(&tmp_d, &tmp_d);
		if( ( ((int)TORSION_ODD_PRIMES[i] >> ((int)p_plus_minus_bitlength[i] - j - 1)) & 1 ) != 0 )
		{
			v8fp2_mul(&tmp_a, &tmp_a, &(A->x));
			v8fp2_mul(&tmp_d, &tmp_d, &constant_d_edwards);
		};
	};

	// raising to 8-th power
	for (j = 0; j < 3; j++)
	{
		v8fp2_sqr(&By, &By);
		v8fp2_sqr(&Bz, &Bz);
	};

	// Mapping Twisted Edwards curve coefficients into Montgomery form
	v8fp2_mul(&(B->x), &tmp_a, &Bz);
	v8fp2_mul(&(B->z), &tmp_d, &By);
	v8fp2_sub(&(B->z), &(B->x), &(B->z));
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
//  Isogeny construction (xISOG) used in velu SQRT

void v8xisog_s(v8ec_point_t* B, uint64_t const i, v8ec_point_t const *A)
{
	// =================================================================================
	assert(TORSION_ODD_PRIMES[i] > gap);     // Ensuring velusqrt is used for l_i > gap
	v8sI = sizeI[i];          // size of I
	v8sJ = sizeJ[i];          // size of J
	v8sK = sizeK[i];          // size of K

	assert(v8sI >= v8sJ);       // Ensuring #I >= #J
	assert(v8sK >= 0);         // Recall, L is a prime and therefore it must be that #K > 0
	assert(v8sJ > 1);         // ensuring sI >= sJ > 1
	// =================================================================================
	
	// We require the curve coefficient A = A'/C ... well, a multiple of these ones
	v8fp2_t Ap;
	v8fp2_add(&Ap, &(A->x), &(A->x));	// 2A' + 4C
	v8fp2_sub(&Ap, &Ap, &(A->z));	// 2A'
	v8fp2_add(&Ap, &Ap, &Ap);	// 4A'

	v8fp2_t ADD_SQUARED[sJ_max],	// (Xj + Zj)^2
	   SUB_SQUARED[sJ_max];	// (Xj - Zj)^2

	int j;
	// Next loop precompute some variables to be used in the reaminder of xisog
	for (j = 0; j < v8sJ; j++)
	{
		v8fp2_sub(&SUB_SQUARED[j], &v8J[j].x, &v8J[j].z);		// (Xj - Zj)
		v8fp2_sqr(&SUB_SQUARED[j], &SUB_SQUARED[j]);		// (Xj - Zj)^2
		v8fp2_sub(&ADD_SQUARED[j], &SUB_SQUARED[j], &v8XZJ4[j]);	// (Xj + Zj)^2
	};

	//  --------------------------------------------------------------------------------------------------
	//                   ~~~~~~~~
	//                    |    | 
	// Computing E_J(W) = |    | [ F0(W, x([j]P)) * alpha^2 + F1(W, x([j]P)) * alpha + F2(W, x([j]P)) ]
	//                    j in J 
	// In order to avoid costly inverse computations in fp, we are gonna work with projective coordinates
	// In particular, for a degree-l isogeny construction, we need alpha = 1 and alpha = -1

	//fp2_t EJ_0[sJ][3],	// quadratic factors of one polynomial to be used in a resultant 
	//   EJ_1[sJ][3];	// quadratic factors of one polynomial to be used in a resultant

	// Next loop computes all the quadratic factors of EJ_0 and EJ_1
	v8fp2_t t1;
	for (j = 0; j < v8sJ; j++)
	{
		// Each SUB_SQUARED[j] and ADD_SQUARED[j] should be multiplied by C
		v8fp2_mul(&v8EJ_1[j][0], &ADD_SQUARED[j], &(A->z));
		v8fp2_mul(&v8EJ_0[j][0], &SUB_SQUARED[j], &(A->z));
		// We require the double of tadd and tsub
		v8fp2_add(&v8EJ_0[j][1], &v8EJ_1[j][0], &v8EJ_1[j][0]);
		v8fp2_add(&v8EJ_1[j][1], &v8EJ_0[j][0], &v8EJ_0[j][0]);

		v8fp2_mul(&t1, &v8XZJ4[j], &Ap);			// A' *(-4*Xj*Zj)

		// Case alpha = 1
		v8fp2_sub(&v8EJ_0[j][1], &t1, &v8EJ_0[j][1]);
		v8fp2_copy(&v8EJ_0[j][2], &v8EJ_0[j][0]);		// E_[0,j} is a palindrome
		
		// Case alpha = -1
		v8fp2_sub(&v8EJ_1[j][1], &v8EJ_1[j][1], &t1);
		v8fp2_copy(&v8EJ_1[j][2], &v8EJ_1[j][0]);		// E_{1,j} is a palindrome
	};

	// ---------------------------------------------------------------------
	// The faster way for multiplying is using a divide-and-conquer approach
	
	// selfreciprocal product tree of EJ_0 (we only require the root)
	v8product_tree_selfreciprocal_LENFeq3(v8ptree_EJ, v8deg_ptree_EJ, 0, v8EJ_0, v8sJ);
	assert( v8deg_ptree_EJ[0] == (2*v8sJ) );
	if (!scaled)
	{
		// (unscaled) remainder tree approach
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
	v8fp2_t r0;
	v8product(&r0, (const v8fp2_t*)v8leaves, v8sI);

	// selfreciprocal product tree of EJ_1 (we only require the root)
	v8product_tree_selfreciprocal_LENFeq3(v8ptree_EJ, v8deg_ptree_EJ, 0, v8EJ_1, v8sJ);
	assert( v8deg_ptree_EJ[0] == (2*v8sJ) );
	if (!scaled)
	{
		// (unscaled) remainder tree approach
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
		v8fp2_sub(&hK_0[j], &v8K[j].z, &v8K[j].x);
		v8fp2_add(&hK_1[j], &v8K[j].z, &v8K[j].x);
	};

	// hk_0 <- use product to mulitiply all the elements in hK_0
	v8product(&hk_0, (const v8fp2_t*)hK_0, v8sK);
	// hk_1 <- use product to mulitiply all the elements in hK_1
	v8product(&hk_1, (const v8fp2_t*)hK_1, v8sK);
	
	// --------------------------------------------------------------
	// Now, we have all the ingredients for computing the image curve
	v8fp2_t A24, A24m,
	   t24, t24m;	// <---- JORGE creo que podemos omitir estas variables, se usan cuando ya no se requiren los valores de la entrada A (podemos cambiar estos t's por B[0] y B[1]

	v8fp2_copy(&A24, &(A->x));			// A' + 2C
	v8fp2_sub(&A24m, &(A->x), &(A->z));		// A' - 2C
	v8fp2_copy(&Ap, &A24m);

	// left-to-right method for computing (A' + 2C)^l and (A' - 2C)^l
	for (j = 1; j < (int)p_plus_minus_bitlength[i]; j++)
	{
		v8fp2_sqr(&A24, &A24);
		v8fp2_sqr(&A24m, &A24m);
		if( ( ((int)TORSION_ODD_PRIMES[i] >> ((int)p_plus_minus_bitlength[i] - j - 1)) & 1 ) != 0 )
		{
			v8fp2_mul(&A24, &A24, &(A->x));
			v8fp2_mul(&A24m, &A24m, &Ap);
		};
	};

	v8fp2_mul(&t24m, &hk_1, &r1);			// output of algorithm 2 with alpha =-1 and without the demoninator
	v8fp2_sqr(&t24m, &t24m);			// raised at 2
	v8fp2_sqr(&t24m, &t24m);			// raised at 4
	v8fp2_sqr(&t24m, &t24m);			// raised at 8

	v8fp2_mul(&t24, &hk_0, &r0);			// output of algorithm 2 with alpha = 1 and without the demoninator 
	v8fp2_sqr(&t24, &t24);			// raised at 2
	v8fp2_sqr(&t24, &t24);			// raised at 4
	v8fp2_sqr(&t24, &t24);			// raised at 8

	v8fp2_mul(&A24, &A24, &t24m);
	v8fp2_mul(&A24m, &A24m, &t24);

	// Now, we have d = (A24m / A24) where the image Montgomery cuve coefficient is
	//      B'   2*(1 + d)   2*(A24 + A24m)
	// B = ---- = --------- = --------------
	//      C      (1 - d)     (A24 - A24m)
	// However, we required B' + 2C = 4*A24 and 4C = 4 * (A24 - A24m)

	v8fp2_sub(&t24m, &A24, &A24m);		//   (A24 - A24m)
	v8fp2_add(&t24m, &t24m, &t24m);		// 2*(A24 - A24m)
	v8fp2_add(&t24m, &t24m, &t24m);		// 4*(A24 - A24m)

	v8fp2_add(&t24, &A24, &A24);			// 2 * A24
	v8fp2_add(&t24, &t24, &t24);			// 4 * A24

	v8fp2_copy(&(B->x), &t24);
	v8fp2_copy(&(B->z), &t24m);
}
