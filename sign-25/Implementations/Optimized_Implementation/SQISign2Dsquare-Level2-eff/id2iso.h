/** @file
 *
 * @authors Antonin Leroux
 *
 * @brief The id2iso algorithms
 */

#ifndef ID2ISO_H
#define ID2ISO_H

#include <intbig.h>
#include <quaternion.h>
#include <klpt.h>
#include <ec.h>


/*************************** Functions *****************************/



/** @defgroup id2iso_others Other functions needed for id2iso
 * @{
 */

 /** @brief Convert the quaternion alpha to an isogeny starting from E0.
 * @param alpha Input quaternion with 2^f || nrd(alpha).
 *
 * @param vec Output: ker = vec[0]*P0+vec[1]*Q0 generates alpha*E0[2^a],
 *           where (P0,Q0) is the canonical basis of E0[2^a]
 */
void quat_to_isogeny_dlog_two(ibz_vec_2_t* vec,
	int f,
	const quat_alg_elem_t* alpha);

 /** @brief Convert the quaternion alpha to an isogeny starting from E0.
 * @param alpha Input quaternion with 3^f || nrd(alpha).
 *
 * @param vec Output: ker = vec[0]*R0+vec[1]*S0 generates alpha*E0[3^b],
 *           where (R0,S0) is the canonical basis of E0[3^b]
 */
void quat_to_isogeny_dlog_three(ibz_vec_2_t* vec,
	int f,
	const quat_alg_elem_t* alpha);

void ec_biscalar_mul_ibz(ec_point_t *res,
                         const ec_curve_t *curve,
                         const ibz_t *scalarP,
                         const ibz_t *scalarQ,
                         const ec_basis_t *PQ,
                         int f);

void ec_mul_ibz(ec_point_t *res,
                const ec_curve_t *curve,
                const ibz_t *scalarP,
                const ec_point_t *P);

// helper function to apply some 2x2 matrix on a basis of E[2^TORSION_PLUS_EVEN_POWER]
// works in place
void matrix_application_even_basis(ec_basis_t *P, const ec_curve_t *E, ibz_mat_2x2_t *mat, int f);
// helper function to apply some 2x2 matrix on a basis of E[3^TORSION_PLUS_THREE_POWER]
void matrix_application_three_basis(ec_basis_t* bas, const ec_curve_t* E, ibz_mat_2x2_t* mat);
// helper function to apply some endomorphism of E on a basis of E[2^TORSION_PLUS_EVEN_POWER]
// works in place
void endomorphism_application_even_basis(ec_basis_t *P,
                                         const ec_curve_t *E,
                                         quat_alg_elem_t *theta,
                                         int f);


void
id2iso_kernel_dlogs_to_ideal_three_general(quat_left_ideal_t *lideal,
	const ibz_vec_2_t *vec3,
	unsigned int f);
void id2iso_kernel_dlogs_to_ideal_three(quat_left_ideal_t *lideal, const ibz_vec_2_t *vec3);

/** @}
 */
/** @}
 */



/**
 * @brief Change of basis matrix
 * Finds mat such that:
 * (mat*v).B2 = v.B1
 * where "." is the dot product, defined as (v1,v2).(P,Q) = v1*P + v2*Q
 *
 * @param mat the computed change of basis matrix
 * @param B1 the source basis
 * @param B2 the target basis
 * @param E the elliptic curve
 *
 * mat encodes the coordinates of the points of B1 in the basis B2
 */
void change_of_basis_matrix_two(ibz_mat_2x2_t *mat,
                                ec_basis_t *B1,
                                ec_basis_t *B2,
                                ec_curve_t *E,
                                int f);

void change_of_basis_matrix_three(ibz_mat_2x2_t *mat,
                                  const ec_basis_t *B1,
                                  const ec_basis_t *B2,
                                  const ec_curve_t *E);

#endif
