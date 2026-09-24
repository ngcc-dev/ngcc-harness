#include <ec.h>
#include <fp2.h>
#include "theta_structure.h"
#include <hd.h>

/** @file
 *
 * @authors Antonin Leroux
 *
 * @brief the theta isogeny header
 */

#ifndef THETA_ISOGENY_H
#define THETA_ISOGENY_H

/*************************** Functions *****************************/

/**
 * @brief Compute the gluing isogeny from an elliptic product
 *
 * @param out Output: the theta_gluing
 * @param E12 an elliptic curve couple E1 x E2
 * @param K1_8 a point in E1xE2[8]
 * @param K2_8 a point in E1xE2[8]
 *
 * out : E1xE2 -> A of kernel [4](K1_8,K2_8)
 *
 */
void gluing_comput(theta_gluing_t *out,
                   theta_couple_curve_t *E12,
                   // const theta_couple_point_t *K1_8,const theta_couple_point_t *K2_8, const
                   // theta_couple_point_t *K1m2_8
                   const theta_couple_jac_point_t *xyT1,
                   const theta_couple_jac_point_t *xyT2);

/**
 * @brief Evaluate a gluing isogeny from an elliptic product on a basis
 *
 * @param image1 Output: the theta_point of the image of the first couple of points
 * @param image2 Output : the theta point of the image of the second couple of points
 * @param P : a couple point in E12
 * @param Q : a couple point in E12
 * @param PmQ : a couple point in E12 corresponding to P-Q
 * @param a : ibz
 * @param b : ibz
 * @param E12 : an elliptic product
 * @param phi : a gluing isogeny E1 x E2 -> A
 *
 * The integers a,b are such that the phi.K1_4 = a P + b Q
 * out : phi( P ), Phi (Q)
 *
 */
void gluing_eval_basis(theta_point_t *image1,
                       theta_point_t *image2,
                       //  const theta_couple_point_t *P,const theta_couple_point_t *Q, const
                       //  theta_couple_point_t *PmQ,const ibz_t *a, const ibz_t *b,
                       const theta_couple_jac_point_t *xyT1,
                       const theta_couple_jac_point_t *xyT2,
                       theta_couple_curve_t *E12,
                       const theta_gluing_t *phi);

/**
 * @brief Compute  a (2,2) isogeny in dimension 2 in the theta_model
 *
 * @param out Output: the theta_isogeny
 * @param A a theta null point for the domain
 * @param T1_8 a point in A[8]
 * @param T2_8 a point in A[8]
 * @param bool1 a boolean
 * @param boo2 a boolean
 *
 * out : A -> B of kernel [4](T1_8,T2_8)
 * bool1 controls if the domain is in standard or dual coordinates
 * bool2 controls if the codomain is in standard or dual coordinates
 *
 */
void theta_isogeny_comput(theta_isogeny_t *out,
                          const theta_structure_t *A,
                          const theta_point_t *T1_8,
                          const theta_point_t *T2_8,
                          int bool1,
                          int bool2);

/**
 * @brief Evaluate a theta isogeny
 *
 * @param out Output: the evaluating point
 * @param phi a theta isogeny
 * @param P a point in the domain of phi
 *
 * out = phi(P)
 *
 */
void theta_isogeny_eval(theta_point_t *out, const theta_isogeny_t *phi, const theta_point_t *P);

/**
 * @brief Compute the splitting isomorphism from a theta structure to the product theta structure,
 * returns false if the given theta structure is not isomorphic to an elliptic product
 *
 * @return a boolean indicating if A is isomorphic to an elliptic product
 * @param out: the splitting isomorphism
 * @param A : the theta_structure in consideration
 *
 * out : A -> B where B is theta product associated to ExF an elliptic product
 */
int splitting_comput(theta_splitting_t *out, const theta_structure_t *A);

// /**
//  * @brief Compute  a (2,2) isogeny chain in dimension 2 between elliptic products in the
//  theta_model
//  *
//  * @param out Output: the theta_chain
//  * @param n : the length of the isogeny chain
//  * @param E12 an elliptic curve product
//  * @param T1 a couple point on E12[2^(n+2)]
//  * @param T2 a couple point on E12[2^(n+2)]
//  * @param T1m2 a couple point on E12[2^(n+2)] equal to T1-T2
//  *
//  * out : E1xE2 -> E3xE4 of kernel [4](T1,T2)
//  *
//    */
// void theta_chain_comput(theta_chain_t *out,int n,const theta_couple_curve_t *E12,const
// theta_couple_point_t *T1,const theta_couple_point_t *T2, const theta_couple_point_t *T1m2);

// /**
//  * @brief Evaluate a (2,2) isogeny chain in dimension 2 between elliptic products in the
//  theta_model
//  *
//  * @param out Output: the image point
//  * @param phi : the (2,2) isogeny chain of domain E12
//  * @param P12 a couple point on E12,
//  *
//  * phi : E1xE2 -> E3xE4 of kernel
//  * P12 in E1xE2
//  * out = phi(P12) in E3xE4
//  *
//    */
// void theta_chain_eval(theta_couple_point_t *out,theta_chain_t *phi,theta_couple_point_t *P12);

#endif