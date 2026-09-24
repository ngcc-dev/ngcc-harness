// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from isog.h in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modifications made:
 *   - Deleted some unused functions
 *   - Changed the data structure to make it suitable for vectorization
 */

#ifndef _V8_ISOG_H_
#define _V8_ISOG_H_

#include "v8curve_extras.h"
#include "v8poly.h"

extern int v8sI, v8sJ, v8sK;	// Sizes of each current I, J, and K	

extern v8fp2_t v8I[sI_max][2],		// I plays also as the linear factors of the polynomial h_I(X)
			v8EJ_0[sJ_max][3], v8EJ_1[sJ_max][3];	// To be used in xisog y xeval

extern v8ec_point_t v8J[sJ_max], v8K[sK_max];		// Finite subsets of the kernel
extern v8fp2_t v8XZJ4[sJ_max],		// -4* (Xj * Zj) for each j in J, and x([j]P) = (Xj : Zj)
    v8rtree_A[(1 << (ceil_log_sI_max+2)) - 1],		// constant multiple of the reciprocal tree computation
    v8A0;			// constant multiple of the reciprocal R0

extern v8poly v8ptree_hI[(1 << (ceil_log_sI_max+2)) - 1],		// product tree of h_I(X)
     v8rtree_hI[(1 << (ceil_log_sI_max+2)) - 1],		// reciprocal tree of h_I(X)
     v8ptree_EJ[(1 << (ceil_log_sJ_max+2)) - 1];		// product tree of E_J(X)
     
extern v8fp2_t v8R0[2*sJ_max + 1];		// Reciprocal of h_I(X) required in the scaled remainder tree approach

extern int v8deg_ptree_hI[(1 << (ceil_log_sI_max+2)) - 1],	// degree of each noed in the product tree of h_I(X)
    v8deg_ptree_EJ[(1 << (ceil_log_sJ_max+2)) - 1];	// degree of each node in the product tree of E_J(X)

extern v8fp2_t v8leaves[sI_max];		// leaves of the remainder tree, which are required in the Resultant computation


// void eds2mont(ec_point_t* P);						// mapping from Twisted edwards into Montogmery
void v8yadd(v8ec_point_t* R, v8ec_point_t* const P, v8ec_point_t* const Q, v8ec_point_t* const PQ);	// differential addition on Twisted edwards model
void v8CrissCross(v8fp2_t *r0, v8fp2_t *r1, v8fp2_t const alpha, v8fp2_t const beta, v8fp2_t const gamma, v8fp2_t const delta);

void v8kps_t(uint64_t const i, v8ec_point_t const *P, v8ec_point_t const *A);	// tvelu formulae
void v8kps_s(uint64_t const i, v8ec_point_t const *P, v8ec_point_t const *A);	// svelu formulae

void v8xisog_t(v8ec_point_t* B, uint64_t const i, v8ec_point_t const *A);	// tvelu formulae
void v8xisog_s(v8ec_point_t* B, uint64_t const i, v8ec_point_t const *A);	// svelu formulae

void v8xeval_t(v8ec_point_t* Q, uint64_t const i, v8ec_point_t const *P);			// tvelu formulae
void v8xeval_s(v8ec_point_t* Q, uint64_t const i, v8ec_point_t const *P, v8ec_point_t const *A);	// svelu formulae

// Strategy-based 4-isogeny chain
// static void ec_eval_even_strategy(ec_curve_t* image, ec_point_t* points, unsigned short points_len,
//     ec_point_t* A24, const ec_point_t *kernel, const int isog_len);

void v8kps_clear(int i);	// Clear memory assigned by KPS

// hybrid velu formulae
static inline void v8kps(uint64_t const i, v8ec_point_t const *P, v8ec_point_t const *A)	
{
	// Next branch only depends on a fixed public bound (named gap)
	if (TORSION_ODD_PRIMES[i] <= gap)
		v8kps_t(i, P, A);
	else
		v8kps_s(i, P, A);
}

static inline void v8xisog(v8ec_point_t* B, uint64_t const i, v8ec_point_t const *A)
{
	// Next branch only depends on a fixed public bound (named gap)
	if (TORSION_ODD_PRIMES[i] <= gap)
		v8xisog_t(B, i, A);
	else
		v8xisog_s(B, i, A);
}

static inline void v8xeval(v8ec_point_t* Q, uint64_t const i, v8ec_point_t const *P, v8ec_point_t const *A)
{
	// Next branch only depends on a fixed public bound (named gap)
	if (TORSION_ODD_PRIMES[i] <= gap)
		v8xeval_t(Q, i, P);
	else
		v8xeval_s(Q, i, P, A);
}


#endif
