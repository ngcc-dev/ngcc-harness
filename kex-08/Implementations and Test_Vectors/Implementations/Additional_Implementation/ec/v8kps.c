// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from kps.c in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modifications made:
 *   - Changed the data structure to make it suitable for vectorization
 */

#include "v8isog.h"
#include "v8curve_extras.h"
#include <assert.h>

int v8sI, v8sJ, v8sK;	// Sizes of each current I, J, and K	

v8fp2_t v8I[sI_max][2],		// I plays also as the linear factors of the polynomial h_I(X)
			v8EJ_0[sJ_max][3], v8EJ_1[sJ_max][3];	// To be used in xisog y xeval

v8ec_point_t v8J[sJ_max], v8K[sK_max];		// Finite subsets of the kernel
v8fp2_t v8XZJ4[sJ_max],		// -4* (Xj * Zj) for each j in J, and x([j]P) = (Xj : Zj)
    v8rtree_A[(1 << (ceil_log_sI_max+2)) - 1],		// constant multiple of the reciprocal tree computation
    v8A0;			// constant multiple of the reciprocal R0

v8poly v8ptree_hI[(1 << (ceil_log_sI_max+2)) - 1],		// product tree of h_I(X)
     v8rtree_hI[(1 << (ceil_log_sI_max+2)) - 1],		// reciprocal tree of h_I(X)
     v8ptree_EJ[(1 << (ceil_log_sJ_max+2)) - 1];		// product tree of E_J(X)
     
v8fp2_t v8R0[2*sJ_max + 1];		// Reciprocal of h_I(X) required in the scaled remainder tree approach

int v8deg_ptree_hI[(1 << (ceil_log_sI_max+2)) - 1],	// degree of each noed in the product tree of h_I(X)
    v8deg_ptree_EJ[(1 << (ceil_log_sJ_max+2)) - 1];	// degree of each node in the product tree of E_J(X)

v8fp2_t v8leaves[sI_max];		// leaves of the remainder tree, which are required in the Resultant computation

// -----------------------------------------------------------
// -----------------------------------------------------------
// Traditional Kernel Point computation (KPs)


// Differential doubling in Twisted Edwards model
void v8ydbl(v8ec_point_t* Q, v8ec_point_t* const P, v8ec_point_t const* A)
{
	v8fp2_t t_0, t_1, X, Z;

	v8fp2_sqr(&t_0, &(P->x));
	v8fp2_sqr(&t_1, &(P->z));
	v8fp2_mul(&Z, &(A->z), &t_0);
	v8fp2_mul(&X, &Z, &t_1);
	v8fp2_sub(&t_1, &t_1, &t_0);
	v8fp2_mul(&t_0, &(A->x), &t_1);
	v8fp2_add(&Z, &Z, &t_0);
	v8fp2_mul(&Z, &Z, &t_1);

	v8fp2_sub(&(Q->x), &X, &Z);
	v8fp2_add(&(Q->z), &X, &Z);
}

// Differential addition in Twisted Edwards model
void v8yadd(v8ec_point_t* R, v8ec_point_t* const P, v8ec_point_t* const Q, v8ec_point_t* const PQ)
{
	v8fp2_t a, b, c, d, X, Z;

	v8fp2_mul(&a, &(P->z), &(Q->x));
	v8fp2_mul(&b, &(P->x), &(Q->z));
	v8fp2_add(&c, &a, &b);
	v8fp2_sub(&d, &a, &b);
	v8fp2_sqr(&c, &c);
	v8fp2_sqr(&d, &d);

	v8fp2_add(&a, &(PQ->z), &(PQ->x));
	v8fp2_sub(&b, &(PQ->z), &(PQ->x));
	v8fp2_mul(&X, &b, &c);
	v8fp2_mul(&Z, &a, &d);

	v8fp2_sub(&(R->x), &X, &Z);
	v8fp2_add(&(R->z), &X, &Z);
}

// tvelu formulae
void v8kps_t(uint64_t const i, v8ec_point_t const *P, v8ec_point_t const *A)
{
	int j;
	int d = ((int)TORSION_ODD_PRIMES[i] - 1) / 2;

	// Mapping the input point x(P), which belongs to a 
	// Montogmery curve model, into its Twisted Edwards 
	// representation y(P)
	v8fp2_sub(&(v8K[0].x), &(P->x), &(P->z));
	v8fp2_add(&(v8K[0].z), &(P->x), &(P->z));
	v8ydbl(&v8K[1], &v8K[0], A);				// y([2]P)

	for (j = 2; j < d; j++)
		v8yadd(&v8K[j], &v8K[j - 1], &v8K[0], &v8K[j - 2]);	// y([j+1]P)
}

// -----------------------------------------------------------
// -----------------------------------------------------------
// Kernel Point computation (KPs) used in velu SQRT
void v8kps_s(uint64_t const i, v8ec_point_t const *P, v8ec_point_t const *A)
{
	// =================================================================================
	assert(TORSION_ODD_PRIMES[i] > gap);	// Ensuring velusqrt is used for l_i > gap
	// The optimal bounds must corresponds to sI, sJ, and sK

	v8sI = sizeI[i];	// Size of I
	v8sJ = sizeJ[i];	// Size of J
	v8sK = sizeK[i];	// Size of K
	assert(v8sI >= v8sJ);	// Ensuring #I >= #J
	assert(v8sK >= 0);	// Recall, it must be that #K >= 0
	assert(v8sJ > 1);		// ensuring sI >= sJ > 1
	// =================================================================================
	
	// Now, we can proceed by the general case

	int j;

	// --------------------------------------------------
	// Computing [j]P for each j in {1, 3, ..., 2*sJ - 1}
	v8ec_point_t P2, P4;
	v8copy_point(&v8J[0], P);				//    x(P)
	// Next computations are required for allowing the use of the function get_A()
	v8fp2_mul(&v8XZJ4[0], &v8J[0].x, &v8J[0].z);					//   Xj*Zj
	v8fp2_add(&v8XZJ4[0], &v8XZJ4[0], &v8XZJ4[0]);					//  2Xj*Zj
	v8fp2_add(&v8XZJ4[0], &v8XZJ4[0], &v8XZJ4[0]);					//  4Xj*Zj
	v8fp2_neg(&v8XZJ4[0], &v8XZJ4[0]);					// -4Xj*Zj
	v8xDBLv2(&P2, P, A);					// x([2]P)
	v8xADD(&v8J[1], &P2, &v8J[0], &v8J[0]);			// x([3]P)
	// Next computations are required for allowing the use of the function get_A()
	v8fp2_mul(&v8XZJ4[1], &v8J[1].x, &v8J[1].z);					//   Xj*Zj
	v8fp2_add(&v8XZJ4[1], &v8XZJ4[1], &v8XZJ4[1]);					//  2Xj*Zj
	v8fp2_add(&v8XZJ4[1], &v8XZJ4[1], &v8XZJ4[1]);					//  4Xj*Zj
	v8fp2_neg(&v8XZJ4[1], &v8XZJ4[1]);					// -4Xj*Zj
	for (j = 2; j < v8sJ; j++)
	{
		v8xADD(&v8J[j], &v8J[j - 1], &P2, &v8J[j - 2]);	// x([2*j + 1]P)
		// Next computations are required for allowing the use of the function get_A()
		v8fp2_mul(&v8XZJ4[j], &v8J[j].x, &v8J[j].z);					//   Xj*Zj
		v8fp2_add(&v8XZJ4[j], &v8XZJ4[j], &v8XZJ4[j]);					//  2Xj*Zj
		v8fp2_add(&v8XZJ4[j], &v8XZJ4[j], &v8XZJ4[j]);					//  4Xj*Zj
		v8fp2_neg(&v8XZJ4[j], &v8XZJ4[j]);					// -4Xj*Zj
	};

	// ----------------------------------------------------------
	// Computing [i]P for i in { (2*sJ) * (2i + 1) : 0 <= i < sI}
	// and the linear factors of h_I(W)
	v8ec_point_t Q, Q2, tmp1, tmp2;
	int bhalf_floor= v8sJ >> 1;
	int bhalf_ceil = v8sJ - bhalf_floor;
	v8xDBLv2(&P4, &P2, A);								// x([4]P)
	v8swap_points(&P2, &P4, _mm512_set1_epi64(-(uint64_t)(v8sJ % 2)));								// x([4]P) <--- coditional swap ---> x([2]P)
	v8xADD(&Q, &v8J[bhalf_ceil], &v8J[bhalf_floor - 1], &P2);	// Q := [2b]P
	v8swap_points(&P2, &P4, _mm512_set1_epi64(-(uint64_t)(v8sJ % 2)));								// x([4]P) <--- coditional swap ---> x([2]P)

	// .............................................
	v8xDBLv2(&Q2, &Q, A);					// x([2]Q)
	v8xADD(&tmp1, &Q2, &Q, &Q);	// x([3]Q)
	v8fp2_neg(&v8I[0][0], &Q.x);
	v8fp2_copy(&v8I[0][1], &Q.z);
	v8fp2_neg(&v8I[1][0], &tmp1.x);
	v8fp2_copy(&v8I[1][1], &tmp1.z);
	v8copy_point(&tmp2, &Q);
	
	for (j = 2; j < v8sI; j++){
		v8xADD(&tmp2, &tmp1, &Q2, &tmp2);	// x([2*j + 1]Q)
		v8fp2_neg(&v8I[j][0], &tmp2.x);
		v8fp2_copy(&v8I[j][1], &tmp2.z);
		v8swap_points(&tmp1, &tmp2, _mm512_set1_epi64(-(uint64_t)1));
	}


	// ----------------------------------------------------------------
	// Computing [k]P for k in { 4*sJ*sI + 1, ..., l - 6, l - 4, l - 2}
	// In order to avoid BRANCHES we make allways copy in K[0] and K[1]
	// by assuming that these entries are only used when sK >= 1 and 
	// sK >= 2, respectively.

	//if (sK >= 1)
	v8copy_point(&v8K[0], &P2);				//       x([l - 2]P) = x([2]P)
	//if (sK >= 2)
	v8copy_point(&v8K[1], &P4);				//       x([l - 4]P) = x([4]P)
	
	for (j = 2; j < v8sK; j++)
		v8xADD(&v8K[j], &v8K[j - 1], &P2, &v8K[j - 2]);	// x([l - 2*(j+1)]P) = x([2 * (j+1)]P)

	// ----------------------------------------------------------------
	//                   ~~~~~~~~               ~~~~~~~~
	//                    |    |                 |    |
	// Computing h_I(W) = |    | (W - x([i]P)) = |    | (Zi * W - Xi) / Zi where x([i]P) = Xi/Zi
	//                    i in I                 i in I
	// In order to avoid costly inverse computations in fp, we are gonna work with projective coordinates

	v8product_tree_LENFeq2(v8ptree_hI, v8deg_ptree_hI, 0, v8I, v8sI);				// Product tree of hI
	if (!scaled)
	{
		// (unscaled) remainder tree approach
		v8reciprocal_tree(v8rtree_hI, v8rtree_A, 2*v8sJ + 1, v8ptree_hI, v8deg_ptree_hI, 0, v8sI);	// Reciprocal tree of hI
	}
	else
	{
		// scaled remainder tree approach
		v8fp2_t f_rev[sI_max + 1];
		for (j = 0; j < (v8sI + 1); j++)
			v8fp2_copy(&f_rev[j], &v8ptree_hI[0][v8sI - j]);

		if (v8sI > (2*v8sJ - v8sI + 1))
			v8reciprocal(v8R0, &v8A0, f_rev, v8sI + 1, v8sI);
		else
			v8reciprocal(v8R0, &v8A0, f_rev, v8sI + 1, 2*v8sJ - v8sI + 1);
	};
}

void v8kps_clear(int i){
		if (TORSION_ODD_PRIMES[i] > gap)
		{
			if (!scaled)
				v8clear_tree(v8rtree_hI, 0, sizeI[i]);
			v8clear_tree(v8ptree_hI, 0, sizeI[i]);
		}
}