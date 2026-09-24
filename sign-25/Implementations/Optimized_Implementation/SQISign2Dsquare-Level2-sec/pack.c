#include "sqisigndim2.h"
#include "config.h"

/*
 * Encode an unsigned 64-bit digit array as bytes.
 * @param x Input unsigned 64-bit digit array.
 * @param nbytes Number of bytes to encode; requires nbytes <= Len(x) * sizeof(digit_t).
 * @param enc Output byte string.
 */
static void
encode_digits(unsigned char *enc, const digit_t *x, size_t nbytes)
{
	memcpy(enc, (const unsigned char *)x, nbytes);
}

/*
 * Decode a byte string into an unsigned 64-bit digit array.
 * @param enc Input byte string.
 * @param nbytes Number of bytes in enc.
 * @param ndigits Length of x; requires nbytes <= ndigits * sizeof(digit_t).
 * @param x Output unsigned 64-bit digit array.
 */
static void
decode_digits(digit_t *x, const unsigned char *enc, size_t nbytes, size_t ndigits)
{
	assert(nbytes <= ndigits * sizeof(digit_t));
	memcpy((unsigned char *)x, enc, nbytes);
	memset((unsigned char *)x + nbytes, 0, ndigits * sizeof(digit_t) - nbytes);
}

/*
 * Encode a nonnegative integer as a byte string.
 * @param x Input nonnegative integer.
 * @param nbytes Output byte length; requires x < 2^(8*nbytes).
 * @param enc Output byte string.
 */
static unsigned char *
ibz_to_bytes(unsigned char *enc, const ibz_t *x, size_t nbytes)
{
#if _DEBUG
{
	// Ensure that the input x satisfies 0 <= x < 2^(8*nbytes).
	ibz_t bnd;
	ibz_init(&bnd);
	ibz_pow(&bnd, &ibz_const_two, 8 * nbytes);
	assert((ibz_cmp(&x, &bnd) < 0) && (ibz_cmp(&x, &ibz_const_zero) >= 0));
	ibz_finalize(&bnd);
}
#endif
	const size_t digits = (nbytes + sizeof(digit_t) - 1) / sizeof(digit_t);
	digit_t* d = (digit_t*)malloc(digits*sizeof(digit_t));
	memset(d, 0, digits*sizeof(digit_t));
	ibz_to_digits(d, x);
	encode_digits(enc, d, nbytes);
	free(d);
	return enc + nbytes;
}

/*
 * Decode a byte string into an integer.
 * @param enc Input byte string.
 * @param nbytes Number of bytes in enc.
 * @param x Output nonnegative integer.
 */
static const unsigned char*
ibz_from_bytes(ibz_t* x, const unsigned char* enc, const size_t nbytes)
{
	assert(nbytes > 0);
	const size_t digits = (nbytes + sizeof(digit_t) - 1) / sizeof(digit_t);
	digit_t* d = (digit_t*)malloc(digits*sizeof(digit_t));
	memset(d, 0, digits*sizeof(digit_t));
	decode_digits(d, enc, nbytes, digits);
	ibz_copy_digits(x, d, digits);
	free(d);
	return enc + nbytes;
}

/**
 * Encode an fp2 element x as a byte string.
 * @param x Input fp2 element.
 * @param enc Output byte string.
 */
static unsigned char *
fp2_to_bytes(unsigned char *enc, const fp2_t *x)
{
	fp2_encode(enc, x);
	return enc + FP2_ENCODED_BYTES;
}

/**
 * Decode a byte string into an fp2 element x.
 * @param enc Input byte string.
 * @param x Output fp2 element.
 */
static const unsigned char *
fp2_from_bytes(fp2_t *x, const unsigned char *enc)
{
	fp2_decode(x, enc);
	return enc + FP2_ENCODED_BYTES;
}

/**
 * Encode a projective fp2 point (x:z) as a byte string.
 * @param x Input projective x-coordinate.
 * @param z Input projective z-coordinate.
 * @param enc Output byte string.
 */
static unsigned char *
proj_to_bytes(unsigned char *enc, const fp2_t *x, const fp2_t *z)
{
	assert(!fp2_is_zero(z));

	fp2_t tmp = *z;
	fp2_inv(&tmp);
	fp2_mul(&tmp, x, &tmp);
	enc = fp2_to_bytes(enc, &tmp);
	return enc;
}

/**
 * Decode a byte string into a projective fp2 point (x:z).
 * @param enc Input byte string.
 * @param x Output projective x-coordinate.
 * @param z Output projective z-coordinate.
 */
static const unsigned char *
proj_from_bytes(fp2_t *x, fp2_t *z, const unsigned char *enc)
{
	enc = fp2_from_bytes(x, enc);
	fp2_set_one(z);
	return enc;
}

/**
 * Encode a curve as a byte string.
 * @param curve Input curve.
 * @param enc Output byte string.
 */
static unsigned char *
ec_curve_to_bytes(unsigned char *enc, const ec_curve_t *curve)
{
	return proj_to_bytes(enc, &curve->A, &curve->C);
}


/**
 * Decode a byte string into a curve.
 * @param enc Input byte string.
 * @param curve Output curve.
 */
static const unsigned char *
ec_curve_from_bytes(ec_curve_t *curve, const unsigned char *enc)
{
	memset(curve, 0, sizeof(*curve));
	return proj_from_bytes(&curve->A, &curve->C, enc);
}


/**
 * Encode a public key as a byte string.
 * @param pk Input public key.
 * @param enc Output byte string.
 */
unsigned char *
public_key_to_bytes(unsigned char *enc, const public_key_t *pk)
{
#if _DEBUG
	const unsigned char *const start = enc;
#endif

	enc = ec_curve_to_bytes(enc, &pk->curve);

#if COMPRESSED
	*enc++ = (unsigned char)pk->hint_pk;
	//*enc++ = (unsigned char)pk->hint_pk[1];
#endif

#if _DEBUG
	assert(enc - start == PUBLICKEY_BYTES);
#endif
	return enc;
}

/**
 * Decode a byte string into a public key.
 * @param enc Input byte string.
 * @param pk Output public key.
 */
const unsigned char *
public_key_from_bytes(public_key_t *pk, const unsigned char *enc)
{
#if _DEBUG
	const unsigned char *const start = enc;
#endif

	enc = ec_curve_from_bytes(&pk->curve, enc);
	
#if COMPRESSED
	pk->hint_pk = (int)*enc++;
	//pk->hint_pk[1] = (int)*enc++;
#endif
#if _DEBUG
	assert(enc - start == PUBLICKEY_BYTES);
#endif
	return enc;
}

/**
 * Print an encoded public key byte string.
 */
void print_public_key(const unsigned char* state_pk)
{
	printf("Public Key:\n");
	for (int i = 0; i < PUBLICKEY_BYTES; i++) {
		printf("%02x", state_pk[i]);
	}
	printf("\n");
}

/**
 * Encode a signature as a byte string.
 * @param sig Input signature.
 * @param enc Output byte string.
 */
void
signature_to_bytes(unsigned char *enc, const signature_t *sig)
{
#if _DEBUG
	unsigned char *const start = enc;
#endif

#if COMPRESSED
	enc = ec_curve_to_bytes(enc, &sig->E_diag);
	encode_digits(enc, sig->chall, NWORDS_ORDER_3_BYTES);
	enc += NWORDS_ORDER_3_BYTES;

	digit_t mat_temp[2][2][NWORDS_ORDER_2];
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 2; j++)
			ibz_to_digit_array(mat_temp[i][j], &sig->mat_Bpk_can_to_B_pk[i][j]);
	encode_digits(enc, mat_temp[0][0], NWORDS_ORDER_2_BYTES);
	enc += NWORDS_ORDER_2_BYTES;
	encode_digits(enc, mat_temp[0][1], NWORDS_ORDER_2_BYTES);
	enc += NWORDS_ORDER_2_BYTES;
	encode_digits(enc, mat_temp[1][0], NWORDS_ORDER_2_BYTES);
	enc += NWORDS_ORDER_2_BYTES;
	encode_digits(enc, mat_temp[1][1], NWORDS_ORDER_2_BYTES);
	enc += NWORDS_ORDER_2_BYTES;

	encode_digits(enc, sig->scalar, NWORDS_ORDER_3_BYTES);
	enc += NWORDS_ORDER_3_BYTES;

	*enc++ = (unsigned char)(sig->hint_curve | (sig->ind << 1));

	*enc++ = (unsigned char)sig->hint_chall[0];
	*enc++ = (unsigned char)sig->hint_chall[1];
	*enc++ = (unsigned char)sig->hint_diag;
	//*enc++ = (unsigned char)sig->hint_diag[1];
#else
	enc = ec_curve_to_bytes(enc, &sig->E_aux);
	enc = ec_curve_to_bytes(enc, &sig->E_com);

	digit_t mat_temp[2][2][NWORDS_ORDER_2];
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 2; j++)
			ibz_to_digit_array(mat_temp[i][j], &sig->mat_Bchall_can_to_B_chall[i][j]);
	encode_digits(enc, mat_temp[0][0], NWORDS_ORDER_2_BYTES);
	enc += NWORDS_ORDER_2_BYTES;
	encode_digits(enc, mat_temp[0][1], NWORDS_ORDER_2_BYTES);
	enc += NWORDS_ORDER_2_BYTES;
	encode_digits(enc, mat_temp[1][0], NWORDS_ORDER_2_BYTES);
	enc += NWORDS_ORDER_2_BYTES;
	encode_digits(enc, mat_temp[1][1], NWORDS_ORDER_2_BYTES);
	enc += NWORDS_ORDER_2_BYTES;
	
	*enc++ = (unsigned char)sig->hint_aux;
	//*enc++ = (unsigned char)sig->hint_aux[1];
	*enc++ = (unsigned char)sig->hint_chall;
	//*enc++ = (unsigned char)sig->hint_chall[1];
	*enc++ = (unsigned char)sig->hint_com[0];
	*enc++ = (unsigned char)sig->hint_com[1];
#endif

#if _DEBUG
	assert(enc - start == SIGNATURE_BYTES);
#endif
}

/**
 * Decode a byte string into a signature.
 * @param enc Input byte string.
 * @param sig Output signature.
 */
void
signature_from_bytes(signature_t *sig, const unsigned char *enc)
{
#if _DEBUG
	const unsigned char* const start = enc;
#endif

#if COMPRESSED
	enc = ec_curve_from_bytes(&sig->E_diag, enc);

	decode_digits(sig->chall, enc, NWORDS_ORDER_3_BYTES, NWORDS_ORDER_3);
	enc += NWORDS_ORDER_3_BYTES;

	digit_t mat_temp[2][2][NWORDS_ORDER_2];
	decode_digits(mat_temp[0][0], enc, NWORDS_ORDER_2_BYTES, NWORDS_ORDER_2);
	enc += NWORDS_ORDER_2_BYTES;
	decode_digits(mat_temp[0][1], enc, NWORDS_ORDER_2_BYTES, NWORDS_ORDER_2);
	enc += NWORDS_ORDER_2_BYTES;
	decode_digits(mat_temp[1][0], enc, NWORDS_ORDER_2_BYTES, NWORDS_ORDER_2);
	enc += NWORDS_ORDER_2_BYTES;
	decode_digits(mat_temp[1][1], enc, NWORDS_ORDER_2_BYTES, NWORDS_ORDER_2);
	enc += NWORDS_ORDER_2_BYTES;
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 2; j++)
			ibz_copy_digit_array(&sig->mat_Bpk_can_to_B_pk[i][j], mat_temp[i][j]);

	decode_digits(sig->scalar, enc, NWORDS_ORDER_3_BYTES, NWORDS_ORDER_3);
	enc += NWORDS_ORDER_3_BYTES;

	sig->hint_curve = (int)(*enc & 1);
	sig->ind = (int)((*enc>>1) & 1);
	enc++;

	sig->hint_chall[0] = (int)*enc++;
	sig->hint_chall[1] = (int)*enc++;
	sig->hint_diag = (int)*enc++;
	//sig->hint_diag[1] = (int)*enc++;
#else
	enc = ec_curve_from_bytes(&sig->E_aux, enc);
	enc = ec_curve_from_bytes(&sig->E_com, enc);

	digit_t mat_temp[2][2][NWORDS_ORDER_2];
	decode_digits(mat_temp[0][0], enc, NWORDS_ORDER_2_BYTES, NWORDS_ORDER_2);
	enc += NWORDS_ORDER_2_BYTES;
	decode_digits(mat_temp[0][1], enc, NWORDS_ORDER_2_BYTES, NWORDS_ORDER_2);
	enc += NWORDS_ORDER_2_BYTES;
	decode_digits(mat_temp[1][0], enc, NWORDS_ORDER_2_BYTES, NWORDS_ORDER_2);
	enc += NWORDS_ORDER_2_BYTES;
	decode_digits(mat_temp[1][1], enc, NWORDS_ORDER_2_BYTES, NWORDS_ORDER_2);
	enc += NWORDS_ORDER_2_BYTES;
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 2; j++)
			ibz_copy_digit_array(&sig->mat_Bchall_can_to_B_chall[i][j], mat_temp[i][j]);

	sig->hint_aux = (int)*enc++;
	//sig->hint_aux[1] = (int)*enc++;
	sig->hint_chall = (int)*enc++;
	//sig->hint_chall[1] = (int)*enc++;
	sig->hint_com[0] = (int)*enc++;
	sig->hint_com[1] = (int)*enc++;
#endif

#if _DEBUG
	assert(enc - start == SIGNATURE_BYTES);
#endif
}

/**
 * Print an encoded signature byte string.
 */
void print_signature(const unsigned char* state_sig)
{
	printf("Signature:\n");
	for (int i = 0; i < SIGNATURE_BYTES; i++) {
		printf("%02x", state_sig[i]);
	}
	printf("\n");
}


/**
 * Encode a secret key as a byte string.
 * The encoded secret key includes the following contents.
 * @param sk Input secret key.
 * @param pk Input public key.
 * @param enc Output byte string.
 */
void
secret_key_to_bytes(unsigned char *enc, const secret_key_t *sk, const public_key_t *pk)
{
#if _DEBUG
	unsigned char *const start = enc;
#endif

	enc = public_key_to_bytes(enc, pk);

	enc = ibz_to_bytes(enc, &sk->secret_ideal.norm, SECRET_NORM_BOUND_BYTES);
	{
		quat_alg_elem_t gen;
		quat_alg_elem_init(&gen);
		int ret = quat_lideal_generator(&gen, &sk->secret_ideal, &QUATALG_PINFTY, 0);
		assert(ret);
#if _DEBUG
		{
			// let's make sure that the denominator is indeed coprime to the norm of the ideal
			ibz_t gcd;
			ibz_init(&gcd);
			ibz_gcd(&gcd, &gen.denom, &sk->secret_ideal.norm);
			assert(!ibz_cmp(&gcd, &ibz_const_one));
			ibz_finalize(&gcd);
		}
#endif
		enc = ibz_to_bytes(enc, &gen.coord[0], SECRET_NORM_BOUND_BYTES);
		enc = ibz_to_bytes(enc, &gen.coord[1], SECRET_NORM_BOUND_BYTES);
		enc = ibz_to_bytes(enc, &gen.coord[2], SECRET_NORM_BOUND_BYTES);
		enc = ibz_to_bytes(enc, &gen.coord[3], SECRET_NORM_BOUND_BYTES);
		quat_alg_elem_finalize(&gen);
	}
#if COMPRESSED
	digit_t mat_temp1[2][2][NWORDS_ORDER_2], mat_temp2[2][2][NWORDS_ORDER_3];
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 2; j++)
			ibz_to_digit_array(mat_temp1[i][j], &sk->mat_B0_to_Bcan_two[i][j]);
	encode_digits(enc, mat_temp1[0][0], NWORDS_ORDER_2_BYTES);
	enc += NWORDS_ORDER_2_BYTES;
	encode_digits(enc, mat_temp1[0][1], NWORDS_ORDER_2_BYTES);
	enc += NWORDS_ORDER_2_BYTES;
	encode_digits(enc, mat_temp1[1][0], NWORDS_ORDER_2_BYTES);
	enc += NWORDS_ORDER_2_BYTES;
	encode_digits(enc, mat_temp1[1][1], NWORDS_ORDER_2_BYTES);
	enc += NWORDS_ORDER_2_BYTES;

	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 2; j++)
			ibz_to_digit_array(mat_temp2[i][j], &sk->mat_B0_to_Bcan_three[i][j]);
	encode_digits(enc, mat_temp2[0][0], NWORDS_ORDER_3_BYTES);
	enc += NWORDS_ORDER_3_BYTES;
	encode_digits(enc, mat_temp2[0][1], NWORDS_ORDER_3_BYTES);
	enc += NWORDS_ORDER_3_BYTES;
	encode_digits(enc, mat_temp2[1][0], NWORDS_ORDER_3_BYTES);
	enc += NWORDS_ORDER_3_BYTES;
	encode_digits(enc, mat_temp2[1][1], NWORDS_ORDER_3_BYTES);
	enc += NWORDS_ORDER_3_BYTES;

	*enc++ = (unsigned char)sk->hint_sk[0];
	*enc++ = (unsigned char)sk->hint_sk[1];
	*enc++ = (unsigned char)sk->hint_sk[2];
	//*enc++ = (unsigned char)sk->hint_sk[3];
#else
	enc = proj_to_bytes(enc, &(sk->basis_two.P.x), &(sk->basis_two.P.z));
	enc = proj_to_bytes(enc, &(sk->basis_two.Q.x), &(sk->basis_two.Q.z));
	enc = proj_to_bytes(enc, &(sk->basis_two.PmQ.x), &(sk->basis_two.PmQ.z));
	enc = proj_to_bytes(enc, &(sk->basis_three.P.x), &(sk->basis_three.P.z));
	enc = proj_to_bytes(enc, &(sk->basis_three.Q.x), &(sk->basis_three.Q.z));
	enc = proj_to_bytes(enc, &(sk->basis_three.PmQ.x), &(sk->basis_three.PmQ.z));
#endif

#if _DEBUG
	assert(enc - start == SECRETKEY_BYTES);
#endif
}

/**
 * Decode a byte string into a secret key.
 * @param enc Input byte string.
 * @param sk Output secret key.
 * @param pk Output public key.
 */
void
secret_key_from_bytes(secret_key_t *sk, public_key_t *pk, const unsigned char *enc)
{
#if _DEBUG
	const unsigned char *const start = enc;
#endif

	enc = public_key_from_bytes(pk, enc);

	{
		ibz_t norm;
		ibz_init(&norm);
		quat_alg_elem_t gen;
		quat_alg_elem_init(&gen);
		enc = ibz_from_bytes(&norm, enc, SECRET_NORM_BOUND_BYTES);
		enc = ibz_from_bytes(&gen.coord[0], enc, SECRET_NORM_BOUND_BYTES);
		enc = ibz_from_bytes(&gen.coord[1], enc, SECRET_NORM_BOUND_BYTES);
		enc = ibz_from_bytes(&gen.coord[2], enc, SECRET_NORM_BOUND_BYTES);
		enc = ibz_from_bytes(&gen.coord[3], enc, SECRET_NORM_BOUND_BYTES);
		quat_lideal_create_from_primitive(&sk->secret_ideal, &gen, &norm, &MAXORD_O0, &QUATALG_PINFTY);
		quat_alg_elem_finalize(&gen);
	}

#if COMPRESSED
	digit_t mat_temp1[2][2][NWORDS_ORDER_2], mat_temp2[2][2][NWORDS_ORDER_3];
	decode_digits(mat_temp1[0][0], enc, NWORDS_ORDER_2_BYTES, NWORDS_ORDER_2);
	enc += NWORDS_ORDER_2_BYTES;
	decode_digits(mat_temp1[0][1], enc, NWORDS_ORDER_2_BYTES, NWORDS_ORDER_2);
	enc += NWORDS_ORDER_2_BYTES;
	decode_digits(mat_temp1[1][0], enc, NWORDS_ORDER_2_BYTES, NWORDS_ORDER_2);
	enc += NWORDS_ORDER_2_BYTES;
	decode_digits(mat_temp1[1][1], enc, NWORDS_ORDER_2_BYTES, NWORDS_ORDER_2);
	enc += NWORDS_ORDER_2_BYTES;
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 2; j++)
			ibz_copy_digit_array(&sk->mat_B0_to_Bcan_two[i][j], mat_temp1[i][j]);

	decode_digits(mat_temp2[0][0], enc, NWORDS_ORDER_3_BYTES, NWORDS_ORDER_3);
	enc += NWORDS_ORDER_3_BYTES;
	decode_digits(mat_temp2[0][1], enc, NWORDS_ORDER_3_BYTES, NWORDS_ORDER_3);
	enc += NWORDS_ORDER_3_BYTES;
	decode_digits(mat_temp2[1][0], enc, NWORDS_ORDER_3_BYTES, NWORDS_ORDER_3);
	enc += NWORDS_ORDER_3_BYTES;
	decode_digits(mat_temp2[1][1], enc, NWORDS_ORDER_3_BYTES, NWORDS_ORDER_3);
	enc += NWORDS_ORDER_3_BYTES;
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 2; j++)
			ibz_copy_digit_array(&sk->mat_B0_to_Bcan_three[i][j], mat_temp2[i][j]);

	sk->hint_sk[0] = (int)*enc++;
	sk->hint_sk[1] = (int)*enc++;
	sk->hint_sk[2] = (int)*enc++;
	//sk->hint_sk[3] = (int)*enc++;
#else
	enc = proj_from_bytes(&(sk->basis_two.P.x), &(sk->basis_two.P.z), enc);
	enc = proj_from_bytes(&(sk->basis_two.Q.x), &(sk->basis_two.Q.z), enc);
	enc = proj_from_bytes(&(sk->basis_two.PmQ.x), &(sk->basis_two.PmQ.z), enc);
	enc = proj_from_bytes(&(sk->basis_three.P.x), &(sk->basis_three.P.z), enc);
	enc = proj_from_bytes(&(sk->basis_three.Q.x), &(sk->basis_three.Q.z), enc);
	enc = proj_from_bytes(&(sk->basis_three.PmQ.x), &(sk->basis_three.PmQ.z), enc);
#endif

#if _DEBUG
	assert(enc - start == SECRETKEY_BYTES);
#endif

	sk->curve = pk->curve;
}

/**
 * Print an encoded secret key byte string.
 */
void print_secret_key(const unsigned char* state_sig)
{
	printf("Secret Key:\n");
	for (int i = 0; i < SECRETKEY_BYTES; i++) {
		printf("%02x", state_sig[i]);
	}
	printf("\n");
}

/**
 * Print a message.
 */
void print_message(const unsigned char* msg, size_t l)
{
	printf("Message:\n");
	for (int i = 0; i < l; i++) {
		printf("%02x", msg[i]);
	}
	printf("\n");
}
