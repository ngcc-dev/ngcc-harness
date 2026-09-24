/** @file
 *
 * @authors Antonin Leroux
 *
 * @brief The protocols
 */

#ifndef SQISIGNDIM2_H
#define SQISIGNDIM2_H

#include <intbig.h>
#include <quaternion.h>
#include <klpt_constants.h>
#include <quaternion_data.h>
#include <torsion_constants.h>
#include <rng.h>
#include <endomorphism_action.h>
#include <encoded_sizes.h>
#include <fp_constants.h>

#include <stdio.h>
#include <klpt.h>
#include <id2iso.h>
#include <hd.h>
#include <dim2id2iso.h>

/** @defgroup sqisigndim2_sqisigndim2 SQIsignHD protocols
 * @{
 */
/** @defgroup sqisigndim2_t Types for SQIsignHD protocols
 * @{
 */

/** @brief Type for the signature
 *
 * @typedef signature_t
 *
 * @struct signature
 *
 */

typedef struct signature {
    ec_curve_t E_aux; /// commitment curve
    ibz_mat_2x2_t mat_sigma_phichall; /// the matrix of sigma o phi_chall from canonical basis EA to canonical basis of E_com
    int nrsp;
    int size_d;
    ibz_t chl;
} signature_t;

/** @brief Type for the public keys
 *
 * @typedef public_key_t
 *
 * @struct public_key
 *
 */
typedef struct public_key
{
    ec_curve_t curve; /// the normalized A coefficient of the Montgomery curve
} public_key_t;

typedef struct secret_key_compact {
    fp2_t fp2_part[5];
    ibz_t two_part[6];
    ibz_t three_part[6];
} secret_key_compact_t;

/** @brief Type for the secret keys
 *
 * @typedef secret_key_t
 *
 * @struct secret_key
 *
 */


/** @brief Type for the secret keys
 * 
 * @typedef secret_key_t
 * 
 * @struct secret_key
 * 
*/

typedef struct secret_key {
	ec_curve_t curve; /// the public curve
    quat_left_ideal_t secret_ideal_two;
    quat_alg_elem_t two_to_three_transporter;
    ibz_mat_2x2_t mat_BAcan_to_BA0_two; /// mat_BA0_to_BAcan*BA0 = BAcan, where BAcan is the canonical basis of EA[2^e], and BA0 the image of the basis of E0[2^e] through the secret odd isogeny
    ibz_mat_2x2_t mat_BAcan_to_BA0_three; /// mat_BA0_to_BAcan*BA0 = BAcan, where BAcan is the canonical basis of EA[2^e], and BA0 the image of the basis of E0[2^e] through the secret odd isogeny
    secret_key_compact_t compact;
} secret_key_t;

// typedef struct secret_key
// {
//     ec_basis_t canonical_basis; // the canonical basis of the public key curve
//     ec_curve_t curve;           /// the public curve, but with little precomputations
//     quat_left_ideal_t secret_ideal;
//     ibz_mat_2x2_t mat_BAcan_to_BA0_two; /// mat_BA0_to_BAcan*BA0 = BAcan, where BAcan is the
//                                         /// canonical basis of EA[2^e], and BA0 the image of the
//                                         /// basis of E0[2^e] through the secret isogeny
//     ibz_mat_2x2_t mat_BAcan_to_BA0_three
// } secret_key_t;



/** @}
 */

/*************************** Functions *****************************/

void doublepath(quat_alg_elem_t *gamma, quat_left_ideal_t *lideal_even, quat_left_ideal_t *lideal_odd, 
    ec_basis_t *basis_three,
    ec_basis_t *basis_two,
    ec_curve_t *E_target,
    int verbose);
    
void doublepath_com(quat_alg_elem_t *gamma,  quat_left_ideal_t *lideal_even, quat_left_ideal_t *lideal_odd, 
    ec_basis_t *basis_three_image, 
    ec_basis_t *basis_two_mid, 
    ec_basis_t *basis_two_image, 
    ec_curve_t *E_mid,
    ec_point_t * k0,
    ec_point_t * km,
    ec_curve_t *E_target,
    ibz_t *a3,
    ibz_t *b3,
    int verbose);

void protocols_keygen(public_key_t *pk, secret_key_t *sk);
// int protocols_sign(signature_t *sig,
//                    const public_key_t *pk,
//                    secret_key_t *sk,
//                    const unsigned char *m,
//                    size_t l,
//                    int verbose);
// int protocols_sign(signature_t *sig, const public_key_t *pk, const secret_key_t *sk, const unsigned char* m, size_t l, int verbose);

int protocols_sign(signature_t *sig, const public_key_t *pk, const secret_key_t *sk, const unsigned char* m, size_t l, int verbose);

int protocols_verif(signature_t *sig, const public_key_t *pk, const unsigned char *m, size_t l);


void public_key_init(public_key_t *pk);
void public_key_finalize(public_key_t *pk);

void secret_key_init(secret_key_t *sk);
void secret_key_finalize(secret_key_t *sk);
void secret_sig_init(signature_t *sig);
void secret_sig_finalize(signature_t *sig);

void public_key_encode(unsigned char out[PUBLICKEY_BYTES], const public_key_t *pk);
void public_key_decode(public_key_t *pk, const unsigned char in[PUBLICKEY_BYTES]);
void signature_encode(unsigned char out[SIGNATURE_LEN], const signature_t *sig);
void signature_decode(signature_t *sig, const unsigned char in[SIGNATURE_LEN]);

void print_signature(const signature_t *sig);
void print_public_key(const public_key_t *pk);
void isog_init_three(ec_isog_odd_t *isog, const ec_curve_t *curve, const ec_point_t *ker, int length);
void quat_to_isog_power_of_three(ec_isog_odd_t *isog, ibz_vec_2_t *ker_dlog, const quat_alg_elem_t *gamma);
void quat_to_isog_power_of_odd_minus(ec_isog_odd_t *isog, ibz_vec_2_t *ker_dlog, const quat_alg_elem_t *gamma);

/** @defgroup signature The signature protocol
 * @{
 */

/** @}
 */

/** @}
 */

#endif
