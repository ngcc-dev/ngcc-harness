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

typedef struct signature
{
    ec_curve_t E_aux; /// the montgomery A coefficient for the auxilliary curve
	ec_curve_t E_com; /// the montgomery A coefficient for the commitment curve
	ec_curve_t E_diag; /// the montgomery A coefficient for the diagonal curve
	ec_curve_t E_chall;
	digit_t chall[NWORDS_ORDER_3]; 
	ibz_mat_2x2_t mat_Bchall_can_to_B_chall;
	ibz_mat_2x2_t mat_Bpk_can_to_B_pk;
	int hint_curve;   /// 1 means E_chall has the larger j-invariant; 0 means it has the smaller one.
	int ind;
	digit_t scalar[NWORDS_ORDER_3];  /// ind and scalar encode the dual of phi_chall
	uint8_t hint_aux;
	int hint_com[2];
#if COMPRESSED
	int hint_chall[2];
#else
	uint8_t hint_chall;
#endif
	uint8_t hint_diag;
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
	uint8_t hint_pk;
} public_key_t;

/** @brief Type for the secret keys
 *
 * @typedef secret_key_t
 *
 * @struct secret_key
 *
 */
typedef struct secret_key
{
	ec_basis_t basis_two;     // the image of tau on the canonical 2pow basis of E0
	ec_basis_t basis_three;   // the image of the nonsmooth part of tau on the canonical 3pow basis of E0
	ec_curve_t curve;           /// the public curve, but with little precomputations
    quat_left_ideal_t secret_ideal;    // the ideal corresponding to the nonsmooth part of tau
	
	ibz_mat_2x2_t mat_B0_to_Bcan_two; /// mat_Bcan_to_B0_two*Bcan_two = B0_two, where Bcan_two is the
										/// canonical basis of EA[2^e], and B0_two the image of the
										/// basis of E0[2^e] through the secret isogeny

	ibz_mat_2x2_t mat_B0_to_Bcan_three; /// mat_B0_to_Bcan*Bcan_three = B0_three, where Bcan_three is the
									  /// canonical basis of EA[2^e], and B0_three the image of the
									  /// basis of E0[2^e] through the secret isogeny
	int hint_sk[3];
} secret_key_t;

/** @}
 */

/*************************** Functions *****************************/
void protocols_keygen(unsigned char* state_pk, unsigned char* state_sk);
int protocols_sign(unsigned char* state_sig,
	const unsigned char* state_sk,
	const unsigned char* m,
	size_t l);
int protocols_verif(unsigned char* state_sig, const unsigned char* state_pk, const unsigned char *m, size_t l);
void protocols_keygen_internal(public_key_t *pk, secret_key_t *sk);
int protocols_sign_internal(signature_t* sig,
	const public_key_t* pk,
	secret_key_t* sk,
	const unsigned char* m,
	size_t l);
int protocols_verif_internal(signature_t *sig, const public_key_t *pk, const unsigned char *m, size_t l);

void public_key_init(public_key_t *pk);
void public_key_finalize(public_key_t *pk);

void secret_key_init(secret_key_t *sk);
void secret_key_finalize(secret_key_t *sk);
void secret_sig_init(signature_t *sig);
void secret_sig_finalize(signature_t *sig);

void print_signature(const unsigned char* state_sig);
void print_public_key(const unsigned char* state_pk);
void print_secret_key(const unsigned char* state_sk);
void print_message(const unsigned char* msg, size_t l);

/**
* @brief Encodes a secret key as a byte array
*
* @param enc : Byte array to encode the secret key (including public key) in
* @param sk : Secret key to encode
* @param pk : Public key to encode
*/
void secret_key_to_bytes(unsigned char *enc, const secret_key_t *sk, const public_key_t *pk);

/**
* @brief Decodes a secret key (and public key) from a byte array
*
* @param sk : Structure to decode the secret key in
* @param pk : Structure to decode the public key in
* @param enc : Byte array to decode
*/
void secret_key_from_bytes(secret_key_t *sk, public_key_t *pk, const unsigned char *enc);

/**
* @brief Encodes a public key as a byte array
*
* @param enc : Byte array to encode the public key in
* @param pk : Public key to encode
*/
unsigned char *public_key_to_bytes(unsigned char *enc, const public_key_t *pk);

/**
* @brief Decodes a public key from a byte array
*
* @param pk : Structure to decode the public key in
* @param enc : Byte array to decode
*/
const unsigned char *public_key_from_bytes(public_key_t *pk, const unsigned char *enc);

/**
* @brief Encodes a signature as a byte array
*
* @param enc : Byte array to encode the signature in
* @param sig : Signature to encode
*/
void signature_to_bytes(unsigned char *enc, const signature_t *sig);

/**
* @brief Decodes a signature from a byte array
*
* @param sig : Structure to decode the signature in
* @param enc : Byte array to decode
*/
void signature_from_bytes(signature_t *sig, const unsigned char *enc);

/** @defgroup signature The signature protocol
 * @{
 */

/** @}
 */

/** @}
 */

#endif
