// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from kps.c in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modifications made:
 *   - Changed the data structure to SoA
 */

#include "bundle_isog.h"
#include "bundle_curve_extras.h"
#include <assert.h>

int bundlesI, bundlesJ, bundlesK;	// Sizes of each current I, J, and K	

bundlefp2_t bundleI[sI_max][2],		// I plays also as the linear factors of the polynomial h_I(X)
			bundleEJ_0[sJ_max][3], bundleEJ_1[sJ_max][3];	// To be used in xisog y xeval

bundleec_point_t bundleJ[sJ_max], bundleK[sK_max];		// Finite subsets of the kernel
bundlefp2_t bundleXZJ4[sJ_max],		// -4* (Xj * Zj) for each j in J, and x([j]P) = (Xj : Zj)
    bundlertree_A[(1 << (ceil_log_sI_max+2)) - 1],		// constant multiple of the reciprocal tree computation
    bundleA0;			// constant multiple of the reciprocal R0

bundlepoly bundleptree_hI[(1 << (ceil_log_sI_max+2)) - 1],		// product tree of h_I(X)
     bundlertree_hI[(1 << (ceil_log_sI_max+2)) - 1],		// reciprocal tree of h_I(X)
     bundleptree_EJ[(1 << (ceil_log_sJ_max+2)) - 1];		// product tree of E_J(X)
     
bundlefp2_t bundleR0[2*sJ_max + 1];		// Reciprocal of h_I(X) required in the scaled remainder tree approach

int bundledeg_ptree_hI[(1 << (ceil_log_sI_max+2)) - 1],	// degree of each noed in the product tree of h_I(X)
    bundledeg_ptree_EJ[(1 << (ceil_log_sJ_max+2)) - 1];	// degree of each node in the product tree of E_J(X)

bundlefp2_t bundleleaves[sI_max];		// leaves of the remainder tree, which are required in the Resultant computation

// -----------------------------------------------------------
// -----------------------------------------------------------
// Traditional Kernel Point computation (KPs)

// Kernel computation required in tye degree-4 isogeny evaluation
void bundlekps_4(bundleec_point_t const P, int len)
{
	bundlefp2_sub(&bundleK[1].x, &P.x, &P.z, len);
	bundlefp2_add(&bundleK[2].x, &P.x, &P.z, len);
	bundlefp2_sqr(&bundleK[0].x, &P.z, len);
	bundlefp2_add(&bundleK[0].z, &bundleK[0].x, &bundleK[0].x, len);
	bundlefp2_add(&bundleK[0].x, &bundleK[0].z, &bundleK[0].z, len);
	bundlefp2_copy(&bundleK[3].x, &P.x, len);
	bundlefp2_copy(&bundleK[3].z, &P.z, len);
}

// Differential doubling in Twisted Edwards model
void bundleydbl(bundleec_point_t* Q, bundleec_point_t* const P, bundleec_point_t const* A, int len)
{
	bundlefp2_t t_0, t_1, X, Z;

	bundlefp2_sqr(&t_0, &(P->x), len);
	bundlefp2_sqr(&t_1, &(P->z), len);
	bundlefp2_mul(&Z, &(A->z), &t_0, len);
	bundlefp2_mul(&X, &Z, &t_1, len);
	bundlefp2_sub(&t_1, &t_1, &t_0, len);
	bundlefp2_mul(&t_0, &(A->x), &t_1, len);
	bundlefp2_add(&Z, &Z, &t_0, len);
	bundlefp2_mul(&Z, &Z, &t_1, len);

	bundlefp2_sub(&(Q->x), &X, &Z, len);
	bundlefp2_add(&(Q->z), &X, &Z, len);
}

// Differential addition in Twisted Edwards model
void bundleyadd(bundleec_point_t* R, bundleec_point_t* const P, bundleec_point_t* const Q, bundleec_point_t* const PQ, int len)
{
	bundlefp2_t a, b, c, d, X, Z;

	bundlefp2_mul(&a, &(P->z), &(Q->x), len);
	bundlefp2_mul(&b, &(P->x), &(Q->z), len);
	bundlefp2_add(&c, &a, &b, len);
	bundlefp2_sub(&d, &a, &b, len);
	bundlefp2_sqr(&c, &c, len);
	bundlefp2_sqr(&d, &d, len);

	bundlefp2_add(&a, &(PQ->z), &(PQ->x), len);
	bundlefp2_sub(&b, &(PQ->z), &(PQ->x), len);
	bundlefp2_mul(&X, &b, &c, len);
	bundlefp2_mul(&Z, &a, &d, len);

	bundlefp2_sub(&(R->x), &X, &Z, len);
	bundlefp2_add(&(R->z), &X, &Z, len);
}

// tvelu formulae
void bundlekps_t(uint64_t const i, bundleec_point_t const P, bundleec_point_t const A, int len)
{
	if (TORSION_ODD_PRIMES[i] == 2)
	{
		bundlefp2_copy(&bundleK[1].x, &P.x, len);
		bundlefp2_copy(&bundleK[1].z, &P.z, len);
		return;
	}
	if (TORSION_ODD_PRIMES[i] == 4)
	{
		bundlekps_4(P, len);
		return;
	}
	int j;
	int d = ((int)TORSION_ODD_PRIMES[i] - 1) / 2;

	// Mapping the input point x(P), which belongs to a 
	// Montogmery curve model, into its Twisted Edwards 
	// representation y(P)
	bundlefp2_sub(&bundleK[0].x, &P.x, &P.z, len);
	bundlefp2_add(&bundleK[0].z, &P.x, &P.z, len);
	bundleydbl(&bundleK[1], &bundleK[0], &A, len);				// y([2]P)

	for (j = 2; j < d; j++)
		bundleyadd(&bundleK[j], &bundleK[j - 1], &bundleK[0], &bundleK[j - 2], len);	// y([j+1]P)
}

// -----------------------------------------------------------
// -----------------------------------------------------------
// Kernel Point computation (KPs) used in velu SQRT
void bundlekps_s(uint64_t const i, bundleec_point_t const P, bundleec_point_t const A, int len)
{
	// =================================================================================
	assert(TORSION_ODD_PRIMES[i] > gap);	// Ensuring velusqrt is used for l_i > gap
	// The optimal bounds must corresponds to sI, sJ, and sK

	bundlesI = sizeI[i];	// Size of I
	bundlesJ = sizeJ[i];	// Size of J
	bundlesK = sizeK[i];	// Size of K
	assert(bundlesI >= bundlesJ);	// Ensuring #I >= #J
	assert(bundlesK >= 0);	// Recall, it must be that #K >= 0
	assert(bundlesJ > 1);		// ensuring sI >= sJ > 1
	// =================================================================================
	
	// Now, we can proceed by the general case

	int j;

	// --------------------------------------------------
	// Computing [j]P for each j in {1, 3, ..., 2*sJ - 1}
	bundleec_point_t P2, P4;
	bundlecopy_point(&bundleJ[0], &P, len);				//    x(P)
	// Next computations are required for allowing the use of the function get_A()
	bundlefp2_mul(&bundleXZJ4[0], &bundleJ[0].x, &bundleJ[0].z, len);					//   Xj*Zj
	bundlefp2_add(&bundleXZJ4[0], &bundleXZJ4[0], &bundleXZJ4[0], len);					//  2Xj*Zj
	bundlefp2_add(&bundleXZJ4[0], &bundleXZJ4[0], &bundleXZJ4[0], len);					//  4Xj*Zj
	bundlefp2_neg(&bundleXZJ4[0], &bundleXZJ4[0], len);					// -4Xj*Zj
	bundlexDBLv2(&P2, &P, &A, len);					// x([2]P)
	bundlexADD(&bundleJ[1], &P2, &bundleJ[0], &bundleJ[0], len);			// x([3]P)
	// Next computations are required for allowing the use of the function get_A()
	bundlefp2_mul(&bundleXZJ4[1], &bundleJ[1].x, &bundleJ[1].z, len);					//   Xj*Zj
	bundlefp2_add(&bundleXZJ4[1], &bundleXZJ4[1], &bundleXZJ4[1], len);					//  2Xj*Zj
	bundlefp2_add(&bundleXZJ4[1], &bundleXZJ4[1], &bundleXZJ4[1], len);					//  4Xj*Zj
	bundlefp2_neg(&bundleXZJ4[1], &bundleXZJ4[1], len);					// -4Xj*Zj
	for (j = 2; j < bundlesJ; j++)
	{
		bundlexADD(&bundleJ[j], &bundleJ[j - 1], &P2, &bundleJ[j - 2], len);	// x([2*j + 1]P)
		// Next computations are required for allowing the use of the function get_A()
		bundlefp2_mul(&bundleXZJ4[j], &bundleJ[j].x, &bundleJ[j].z, len);					//   Xj*Zj
		bundlefp2_add(&bundleXZJ4[j], &bundleXZJ4[j], &bundleXZJ4[j], len);					//  2Xj*Zj
		bundlefp2_add(&bundleXZJ4[j], &bundleXZJ4[j], &bundleXZJ4[j], len);					//  4Xj*Zj
		bundlefp2_neg(&bundleXZJ4[j], &bundleXZJ4[j], len);					// -4Xj*Zj
	};

	// ----------------------------------------------------------
	// Computing [i]P for i in { (2*sJ) * (2i + 1) : 0 <= i < sI}
	// and the linear factors of h_I(W)
	bundleec_point_t Q, Q2, tmp1, tmp2;
	int bhalf_floor= bundlesJ >> 1;
	int bhalf_ceil = bundlesJ - bhalf_floor;
	bundlexDBLv2(&P4, &P2, &A, len);								// x([4]P)
	bundleswap_points(&P2, &P4, -(uint64_t)(bundlesJ % 2), len);								// x([4]P) <--- coditional swap ---> x([2]P)
	bundlexADD(&Q, &bundleJ[bhalf_ceil], &bundleJ[bhalf_floor - 1], &P2, len);	// Q := [2b]P
	bundleswap_points(&P2, &P4, -(uint64_t)(bundlesJ % 2), len);								// x([4]P) <--- coditional swap ---> x([2]P)

	// .............................................
	bundlexDBLv2(&Q2, &Q, &A, len);					// x([2]Q)
	bundlexADD(&tmp1, &Q2, &Q, &Q, len);	// x([3]Q)
	bundlefp2_neg(&bundleI[0][0], &Q.x, len);
	bundlefp2_copy(&bundleI[0][1], &Q.z, len);
	bundlefp2_neg(&bundleI[1][0], &tmp1.x, len);
	bundlefp2_copy(&bundleI[1][1], &tmp1.z, len);
	bundlecopy_point(&tmp2, &Q, len);
	
	for (j = 2; j < bundlesI; j++){
		bundlexADD(&tmp2, &tmp1, &Q2, &tmp2, len);	// x([2*j + 1]Q)
		bundlefp2_neg(&bundleI[j][0], &tmp2.x, len);
		bundlefp2_copy(&bundleI[j][1], &tmp2.z, len);
		bundleswap_points(&tmp1, &tmp2, -(uint64_t)1, len);
	}


	// ----------------------------------------------------------------
	// Computing [k]P for k in { 4*sJ*sI + 1, ..., l - 6, l - 4, l - 2}
	// In order to avoid BRANCHES we make allways copy in K[0] and K[1]
	// by assuming that these entries are only used when sK >= 1 and 
	// sK >= 2, respectively.

	//if (sK >= 1)
	bundlecopy_point(&bundleK[0], &P2, len);				//       x([l - 2]P) = x([2]P)
	//if (sK >= 2)
	bundlecopy_point(&bundleK[1], &P4, len);				//       x([l - 4]P) = x([4]P)
	
	for (j = 2; j < bundlesK; j++)
		bundlexADD(&bundleK[j], &bundleK[j - 1], &P2, &bundleK[j - 2], len);	// x([l - 2*(j+1)]P) = x([2 * (j+1)]P)

	// ----------------------------------------------------------------
	//                   ~~~~~~~~               ~~~~~~~~
	//                    |    |                 |    |
	// Computing h_I(W) = |    | (W - x([i]P)) = |    | (Zi * W - Xi) / Zi where x([i]P) = Xi/Zi
	//                    i in I                 i in I
	// In order to avoid costly inverse computations in fp, we are gonna work with projective coordinates

	bundleproduct_tree_LENFeq2(bundleptree_hI, bundledeg_ptree_hI, 0, bundleI, bundlesI, len);				// Product tree of hI
	if (!scaled)
	{
		// (unscaled) remainder tree approach
		bundlereciprocal_tree(bundlertree_hI, bundlertree_A, 2*bundlesJ + 1, bundleptree_hI, bundledeg_ptree_hI, 0, bundlesI, len);	// Reciprocal tree of hI
	}
	else
	{
		// scaled remainder tree approach
		bundlefp2_t f_rev[sI_max + 1];
		for (j = 0; j < (bundlesI + 1); j++)
			bundlefp2_copy(&f_rev[j], &bundleptree_hI[0][bundlesI - j], len);

		if (bundlesI > (2*bundlesJ - bundlesI + 1))
			bundlereciprocal(bundleR0, &bundleA0, f_rev, bundlesI + 1, bundlesI, len);
		else
			bundlereciprocal(bundleR0, &bundleA0, f_rev, bundlesI + 1, 2*bundlesJ - bundlesI + 1, len);
	};
}

void bundlekps_clear(int i){
		if (TORSION_ODD_PRIMES[i] > gap)
		{
			if (!scaled)
				bundleclear_tree(bundlertree_hI, 0, sizeI[i]);
			bundleclear_tree(bundleptree_hI, 0, sizeI[i]);
		}
}