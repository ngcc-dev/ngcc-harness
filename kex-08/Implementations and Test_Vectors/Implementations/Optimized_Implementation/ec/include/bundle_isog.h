// SPDX-FileCopyrightText: Copyright 2023 the SQIsign team. All rights reserved.
// SPDX-License-Identifier: Apache-2.0

/*
 * This file is derived from isog.h in the SQIsign (version 1.0) 
 * project (https://github.com/SQIsign/the-sqisign/tree/nist-v1),
 * which is licensed under the Apache-2.0 license.
 *
 * Modifications made:
 *   - Deleted some unused functions
 *   - Changed the data structure to SoA
 */

#ifndef _BUNDLE_ISOG_H_
#define _BUNDLE_ISOG_H_

#include "bundle_curve_extras.h"
#include "bundle_poly.h"

extern int bundlesI, bundlesJ, bundlesK;	// Sizes of each current I, J, and K	

extern bundlefp2_t bundleI[sI_max][2],		// I plays also as the linear factors of the polynomial h_I(X)
			bundleEJ_0[sJ_max][3], bundleEJ_1[sJ_max][3];	// To be used in xisog y xeval

extern bundleec_point_t bundleJ[sJ_max], bundleK[sK_max];		// Finite subsets of the kernel
extern bundlefp2_t bundleXZJ4[sJ_max],		// -4* (Xj * Zj) for each j in J, and x([j]P) = (Xj : Zj)
    bundlertree_A[(1 << (ceil_log_sI_max+2)) - 1],		// constant multiple of the reciprocal tree computation
    bundleA0;			// constant multiple of the reciprocal R0

extern bundlepoly bundleptree_hI[(1 << (ceil_log_sI_max+2)) - 1],		// product tree of h_I(X)
     bundlertree_hI[(1 << (ceil_log_sI_max+2)) - 1],		// reciprocal tree of h_I(X)
     bundleptree_EJ[(1 << (ceil_log_sJ_max+2)) - 1];		// product tree of E_J(X)
     
extern bundlefp2_t bundleR0[2*sJ_max + 1];		// Reciprocal of h_I(X) required in the scaled remainder tree approach

extern int bundledeg_ptree_hI[(1 << (ceil_log_sI_max+2)) - 1],	// degree of each noed in the product tree of h_I(X)
    bundledeg_ptree_EJ[(1 << (ceil_log_sJ_max+2)) - 1];	// degree of each node in the product tree of E_J(X)

extern bundlefp2_t bundleleaves[sI_max];		// leaves of the remainder tree, which are required in the Resultant computation


// void eds2mont(ec_point_t* P);						// mapping from Twisted edwards into Montogmery
void bundleyadd(bundleec_point_t* R, bundleec_point_t* const P, bundleec_point_t* const Q, bundleec_point_t* const PQ, int len);	// differential addition on Twisted edwards model
void bundleCrissCross(bundlefp2_t *r0, bundlefp2_t *r1, bundlefp2_t const alpha, bundlefp2_t const beta, bundlefp2_t const gamma, bundlefp2_t const delta, int len);

void bundlekps_t(uint64_t const i, bundleec_point_t const P, bundleec_point_t const A, int len);	// tvelu formulae
void bundlekps_s(uint64_t const i, bundleec_point_t const P, bundleec_point_t const A, int len);	// svelu formulae

void bundlexisog_t(bundleec_point_t* B, uint64_t const i, bundleec_point_t const A, int len);	// tvelu formulae
void bundlexisog_s(bundleec_point_t* B, uint64_t const i, bundleec_point_t const A, int len);	// svelu formulae

void bundlexeval_t(bundleec_point_t* Q, uint64_t const i, bundleec_point_t const P, int len);			// tvelu formulae
void bundlexeval_s(bundleec_point_t* Q, uint64_t const i, bundleec_point_t const P, bundleec_point_t const A, int len);	// svelu formulae

void bundlekps_clear(int i);	// Clear memory assigned by KPS


// hybrid velu formulae
static inline void bundlekps(uint64_t const i, bundleec_point_t const P, bundleec_point_t const A, int len)	
{
	// Next branch only depends on a fixed public bound (named gap)
	if (TORSION_ODD_PRIMES[i] <= gap)
		bundlekps_t(i, P, A, len);
	else
		bundlekps_s(i, P, A, len);
}

static inline void bundlexisog(bundleec_point_t* B, uint64_t const i, bundleec_point_t const A, int len)
{
	// Next branch only depends on a fixed public bound (named gap)
	if (TORSION_ODD_PRIMES[i] <= gap)
		bundlexisog_t(B, i, A, len);
	else
		bundlexisog_s(B, i, A, len);
}

static inline void bundlexeval(bundleec_point_t* Q, uint64_t const i, bundleec_point_t const P, bundleec_point_t const A, int len)
{
	// Next branch only depends on a fixed public bound (named gap)
	if (TORSION_ODD_PRIMES[i] <= gap)
		bundlexeval_t(Q, i, P, len);
	else
		bundlexeval_s(Q, i, P, A, len);
}


#endif
