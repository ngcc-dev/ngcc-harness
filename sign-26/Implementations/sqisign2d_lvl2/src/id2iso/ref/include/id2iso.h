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

/** @defgroup id2iso_id2iso Ideal to isogeny conversion
 * @{
 */

/** @defgroup id2iso_iso_types Types for isogenies needed for the id2iso
 * @{
 */

/** @brief Type for long chain of two isogenies
 *
 * @typedef id2iso_long_two_isog
 *
 * Represented as a vector ec_isog_even_t
 */
typedef struct id2iso_long_two_isog
{
    unsigned short length; ///< the number of smaller two isogeny chains
    ec_isog_even_t *chain; ///< the chain of two isogeny
} id2iso_long_two_isog_t;

/** @brief Type for compressed long chain of two isogenies
 *
 * @typedef id2iso_compressed_long_two_isog
 *
 * Represented as a vector of intbig
 */
typedef struct id2iso_compressed_long_two_isog
{
    unsigned short length;        ///< the number of smaller two isogeny chains
    ibz_t *zip_chain;             ///< the chain of two isogeny, compressed
    unsigned char bit_first_step; ///< the bit for the first step
} id2iso_compressed_long_two_isog_t;

/** @}
 */

/*************************** Functions *****************************/

/** @defgroup id2iso Constructors and Destructors
 * @{
 */
void id2iso_long_two_isog_init(id2iso_long_two_isog_t *isog, const size_t length);
void id2iso_long_two_isog_finalize(id2iso_long_two_isog_t *isog);

void id2iso_compressed_long_two_isog_init(id2iso_compressed_long_two_isog_t *zip,
                                          const size_t length);
void id2iso_compressed_long_two_isog_finalize(id2iso_compressed_long_two_isog_t *zip);

/** @}
 */

/** @defgroup id2iso_others Other functions needed for id2iso
 * @{
 */

/**
 * @brief Translating an ideal of odd norm dividing p²-1 into the corresponding isogeny
 *
 * @param isog Output : the output isogeny
 * @param basis_minus : a basis of ec points
 * @param basis_plus : a basis of ec points
 * @param domain : an elliptic curve
 * @param lideal_input : O0-ideal corresponding to the ideal to be translated
 *
 * compute  the isogeny starting from domain corresponding to ideal_input
 * the coefficients extracted from the ideal are to be applied to basis_minus and basis_plus to
 * compute the kernel of the isogeny.
 *
 */

void id2iso_ideal_to_isogeny_odd(ec_isog_odd_t *isog,
                                 const ec_curve_t *domain,
                                 const ec_basis_t *basis_plus,
                                 const ec_basis_t *basis_minus,
                                 const quat_left_ideal_t *lideal_input);
void id2iso_ideal_to_isogeny_odd_plus(ec_isog_odd_t *isog,
                                      ibz_vec_2_t *ker_dlog,
                                      const ec_curve_t *domain,
                                      const ec_basis_t *basis_plus,
                                      const quat_left_ideal_t *lideal_input);
void
id2iso_ideal_to_isogeny_odd_minus(ec_isog_odd_t *isog,
                                 ibz_vec_2_t *ker_dlog,
                                 const ec_curve_t *domain,
                                 const ec_basis_t *basis_minus,
                                 const quat_left_ideal_t *lideal_input);

/**
 * @brief Translating an ideal of norm a power of two dividing p²-1 into the corresponding isogeny
 *
 * @param isog Output : the output isogeny
 * @param lideal_input : O0-ideal corresponding to the ideal to be translated
 *
 */

void id2iso_ideal_to_isogeny_even(ec_isog_even_t *isog, const quat_left_ideal_t *lideal_input);
void id2iso_ideal_to_isogeny_even_dlogs(ec_isog_even_t *isog,
                                        ibz_vec_2_t *ker_dlog,
                                        const quat_left_ideal_t *lideal_input);

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
// helper function to apply some endomorphism of E on a basis of E[2^TORSION_PLUS_EVEN_POWER]
// works in place
void endomorphism_application_even_basis(ec_basis_t *P,
                                         const ec_curve_t *E,
                                         quat_alg_elem_t *theta,
                                         int f);
// same as above but for a basis given in x,y coordinates
void endomorphism_application_even_jac_basis(jac_point_t *P,
                                             jac_point_t *Q,
                                             quat_alg_elem_t *theta,
                                             int f);

/**
 * @brief Translating a kernel on the curve E0, represented as two vectors with respect to the
 * precomputed 2^f- and 3^e-torsion bases, into the corresponding O0-ideal
 *
 * @param lideal Output : the output O0-ideal
 * @param vec2 : length-2 vector giving the 2-power part of the kernel with respect to the
 * precomputed TORSION_PLUS_2POWER basis
 * @param vec3 : length-2 vector giving the 3-power part of the kernel with respect to the
 * precomputed TORSION_PLUS_3POWER basis
 *
 */
void id2iso_kernel_dlogs_to_ideal(quat_left_ideal_t *lideal,
                                  const ibz_vec_2_t *vec2,
                                  const ibz_vec_2_t *vec3);
void id2iso_kernel_dlogs_to_ideal_two(quat_left_ideal_t *lideal, const ibz_vec_2_t *vec2, int f);
void id2iso_kernel_dlogs_to_ideal_three(quat_left_ideal_t *lideal, const ibz_vec_2_t *vec3);

/** @}
 */
/** @}
 */

void matrix_of_endomorphism_even(ibz_mat_2x2_t *mat, const quat_alg_elem_t *alpha);
void matrix_of_endomorphism_odd(ibz_mat_2x2_t *mat, const quat_alg_elem_t *alpha);

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

// function to sample a random left O0-ideal of given norm
// the boolean is_prime indicates if the intput norm is known to be prime
// if it is the case, then the algorithm is significantly faster
void sampling_random_ideal_O0(quat_left_ideal_t *lideal, ibz_t *norm, int is_prime);
void matrix_application_even_basis_basic(ec_basis_t *bas, const ec_curve_t *E, ibz_mat_2x2_t *mat);
void ec_iso_inv(ec_isom_t* isom);
#endif
