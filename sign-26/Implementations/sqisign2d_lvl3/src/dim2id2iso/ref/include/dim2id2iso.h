/** @file
 *
 * @authors Antonin Leroux
 *
 * @brief The dim2_id2iso algorithms
 */

#ifndef DIM2ID2ISO_H
#define DIM2ID2ISO_H

#include <intbig.h>
#include <quaternion.h>
#include <klpt.h>
#include <ec.h>
#include <hd.h>
#include <id2iso.h>
#include <biextension.h>
#include <klpt_constants.h>

/*************************** Functions *****************************/

int find_uv(ibz_t *u,
            ibz_t *v,
            ibz_vec_4_t *coeffs,
            quat_alg_elem_t *beta1,
            quat_alg_elem_t *beta2,
            ibz_t *d1,
            ibz_t *d2,
            const ibz_t *target,
            int number_sum_square,
            const quat_left_ideal_t *lideal,
            const quat_alg_t *Bpoo,
            int num_rerun);

/**
 * @brief Computes an arbitrary isogeny of fixed degree starting from E0
 *
 * @param isog Output : a dim2 isogeny encoding an isogeny of degree u
 * @param lideal Output : an ideal of norm u
 * @param u : integer
 * @param adjust : adjusting factor
 * @param small : bit indicating if we the value of u is "small" meaning that we expect it to be
 * around sqrt{p}, in that case we use a length slightly above
 * @returns a bit indicating if the computation succeeded
 *
 * F is an isogeny encoding an isogeny [adjust]*phi : E0 -> Eu of degree u * adjust^2
 * note that the codomain of F can be either Eu x Eu' or Eu' x Eu for some curve Eu'
 */
int fixed_degree_isogeny(theta_chain_t *isog,
                         quat_left_ideal_t *lideal,
                         ibz_t *u,
                         ibz_t *adjust,
                         int small);

/** @defgroup dim2id2iso_others Other functions needed for id2iso
 * @{
 */

/**
 * @brief Translating an ideal into a representation of the corresponding isogeny
 *
 * @param isog Output : dim 2 isogeny
 * @param beta1 Output : quaternion element
 * @param beta2 Output : quaternion element
 * @param u Output : integer
 * @param v Output : integer
 * @param coeffs Output : integer vector
 * @param phiv Output : dim2 isogeny representation of an isogeny of degree v
 * @param d1 Output : integer
 * @param d2 Output : integer
 * @param codomain the curve d1-isogenous to E0
 * @param basis Output : evaluation of the canonical basis of E0 through the ideal corresponding to
 * lideal
 * @param lideal : ideal in input
 * @param Bpoo : the quaternion algebra
 * @returns a bit indicating if the computation succeeded
 *
 * beta1 and beta2 are elements in lideal of norm n(lideal)d1 and n(lideal) d2 respectively
 * u,v are integers such that 2^e = d1 u + d2 v, and it may be that u = coeffs[0]^2 + coeffs[1]^2, v
 * = coeffs[2]^2 + coeffs[3]^2 phiu,phiv are dim 2 isogeny representing isogenies of degree u,v : E0
 * -> Eu,Ev F is a dim2 2^e - isogeny between Eu x Ev -> E_I x E that encodes an isogeny E0 -> E_I
 * corresponding to the ideal lideal given in input
 */
int dim2id2iso_ideal_to_isogeny_clapotis(theta_chain_t *isog,
                                         quat_alg_elem_t *beta1,
                                         quat_alg_elem_t *beta2,
                                         ibz_t *u,
                                         ibz_t *v,
                                         ibz_vec_4_t *coeffs,
                                         theta_chain_t *phiu,
                                         theta_chain_t *phiv,
                                         ibz_t *d1,
                                         ibz_t *d2,
                                         ec_curve_t *codomain,
                                         ec_basis_t *basis,
                                         const quat_left_ideal_t *lideal,
                                         const quat_alg_t *Bpoo);

/**
 * @brief Translating an ideal into a representation of the corresponding isogeny
 *
 * @param basis Output : evaluation of the canonical basis of E0 through the ideal corresponding to
 * lideal
 * @param lideal : ideal in input
 *
 * This is a wrapper around the ideal to isogeny clapotis function
 */
int dim2id2iso_arbitrary_isogeny_evaluation(ec_basis_t *basis,
                                            ec_curve_t *codomain,
                                            const quat_left_ideal_t *lideal);

#endif
