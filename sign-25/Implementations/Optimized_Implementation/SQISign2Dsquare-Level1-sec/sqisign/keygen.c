#include <sqisigndim2.h>
#include <curve_extras.h>
#include <stdio.h>
#include <gmp.h>
#include <inttypes.h>
#include <ec_params.h>
#include <config.h>


void
public_key_init(public_key_t *pk)
{
    ec_curve_init(&pk->curve);
}

void
public_key_finalize(public_key_t *pk)
{

}

void
secret_key_init(secret_key_t *sk)
{
    quat_left_ideal_init(&(sk->secret_ideal));
	ibz_mat_2x2_init(&(sk->mat_B0_to_Bcan_two));
	ibz_mat_2x2_init(&(sk->mat_B0_to_Bcan_three));
    ec_curve_init(&sk->curve);
}

void
secret_key_finalize(secret_key_t *sk)
{
    quat_left_ideal_finalize(&(sk->secret_ideal));
	ibz_mat_2x2_finalize(&(sk->mat_B0_to_Bcan_two));
	ibz_mat_2x2_finalize(&(sk->mat_B0_to_Bcan_three));
}

void protocols_keygen_internal(public_key_t *pk, secret_key_t *sk)
{
	int found;
	ibz_t n, p_quarter;
	ec_basis_t basis_two, B_0_two, B_can_two;
	ec_basis_t basis_three, B_0_three, B_can_three;
	theta_chain_t isog;

	ibz_init(&n);
	ibz_init(&p_quarter);
	

	compute_p_quarter(&p_quarter, &QUATALG_PINFTY.p);
	do {
		generate_random_prime_kygen(&n, &p_quarter);
		if (mpz_legendre(ibz_const_three, n) != -1) {
			continue;
		}
		break;
	} while (1);

	found = fixed_small_degree_isogeny(&isog, &(sk->secret_ideal), &n, 1);
    (void)found;
	
	copy_point(&(basis_two.P), &BASIS_EVEN.P);
	copy_point(&(basis_two.Q), &BASIS_EVEN.Q);
	copy_point(&(basis_two.PmQ), &BASIS_EVEN.PmQ);
	copy_point(&(basis_three.P), &(BASIS_THREE.P));
	copy_point(&(basis_three.Q), &(BASIS_THREE.Q));
	copy_point(&(basis_three.PmQ), &(BASIS_THREE.PmQ));

	theta_couple_point_t V1, V2, V1m2;
	theta_couple_point_t P, Q, PmQ;
	theta_couple_curve_t E01;

	E01.E1 = isog.domain.E1;
	E01.E2 = isog.domain.E2;

	copy_point(&(P.P1), &(basis_two.P));
	copy_point(&(Q.P1), &(basis_two.Q));
	copy_point(&(PmQ.P1), &(basis_two.PmQ));
	ec_set_zero(&P.P2);
	ec_set_zero(&Q.P2);
	ec_set_zero(&PmQ.P2);

	theta_chain_eval_special_case(&V1, &isog, &P, &E01);
	theta_chain_eval_special_case(&V2, &isog, &Q, &E01);
	theta_chain_eval_special_case(&V1m2, &isog, &PmQ, &E01);

#if _DEBUG
	{
		fp2_t w0, w1, test_pow;
		ec_point_t AC, A24;
		ibz_t temp;
		ibz_init(&temp);
		fp2_copy(&AC.x, &(E01.E1.A));
		fp2_copy(&AC.z, &(E01.E1.C));
		A24_from_AC(&A24, &AC);
		weil(&w0, TORSION_PLUS_EVEN_POWER, &(basis_two.P), &(basis_two.Q), &(basis_two.PmQ), &A24);
		copy_point(&(basis_two.P), &V1.P1);
		copy_point(&(basis_two.Q), &V2.P1);
		copy_point(&(basis_two.PmQ), &V1m2.P1);
		fp2_copy(&AC.x, &(isog.codomain.E1.A));
		fp2_copy(&AC.z, &(isog.codomain.E1.C));
		A24_from_AC(&A24, &AC);
		weil(&w1, TORSION_PLUS_EVEN_POWER, &(basis_two.P), &(basis_two.Q), &(basis_two.PmQ), &A24);
		digit_t digit[NWORDS_ORDER_2] = { 0 };
		ibz_pow(&temp, &ibz_const_two, NONSMOOTH_PART);
		ibz_sub(&temp, &n, &temp);
		ibz_to_digit_array(digit, &temp);
		fp2_pow_vartime(&test_pow, &w0, digit, NWORDS_ORDER_2);
		assert(fp2_is_equal(&test_pow, &w1));
		ibz_finalize(&temp);
	}
#endif

	copy_curve(&(sk->curve), &isog.codomain.E2);
	copy_point(&(B_0_two.P), &V1.P2);
	copy_point(&(B_0_two.PmQ), &V1m2.P2);
	copy_point(&(B_0_two.Q), &V2.P2);
	copy_point(&(P.P1), &(basis_three.P));
	copy_point(&(Q.P1), &(basis_three.Q));
	copy_point(&(PmQ.P1), &(basis_three.PmQ));
	ec_set_zero(&P.P2);
	ec_set_zero(&Q.P2);
	ec_set_zero(&PmQ.P2);
	theta_chain_eval_special_case(&V1, &isog, &P, &E01);
	theta_chain_eval_special_case(&V2, &isog, &Q, &E01);
	theta_chain_eval_special_case(&V1m2, &isog, &PmQ, &E01);
	copy_point(&(B_0_three.P), &V1.P2);
	copy_point(&(B_0_three.PmQ), &V1m2.P2);
	copy_point(&(B_0_three.Q), &V2.P2);
		
	// computing the public key
	fp2_t temp;
	fp2_copy(&temp, &(sk->curve.C));
	fp2_inv(&temp);
	fp2_set_one(&(pk->curve.C));
	fp2_mul(&(pk->curve.A), &temp, &(sk->curve.A));
	sk->curve.is_A24_computed_and_normalized = false;
	pk->curve.is_A24_computed_and_normalized = false;

#if COMPRESSED == 0
	copy_point(&sk->basis_two.P, &B_0_two.P);
	copy_point(&sk->basis_two.Q, &B_0_two.Q);
	copy_point(&sk->basis_two.PmQ, &B_0_two.PmQ);

	copy_point(&sk->basis_three.P, &B_0_three.P);
	copy_point(&sk->basis_three.Q, &B_0_three.Q);
	copy_point(&sk->basis_three.PmQ, &B_0_three.PmQ);
#elif COMPRESSED == 1
	pk->hint_pk = ec_curve_to_basis_2f_to_hint(&B_can_two, &(pk->curve), TORSION_PLUS_EVEN_POWER);
#if _DEBUG
	assert(test_point_order_twof(&B_can_two.Q, &pk->curve, TORSION_PLUS_EVEN_POWER));
	assert(test_point_order_twof(&B_can_two.PmQ, &pk->curve, TORSION_PLUS_EVEN_POWER));
	assert(test_point_order_twof(&B_can_two.P, &pk->curve, TORSION_PLUS_EVEN_POWER));
#endif
	sk->hint_sk[0] = pk->hint_pk;
	/*sk->hint_sk[1] = pk->hint_pk[1];*/
	change_of_basis_matrix_two(&sk->mat_B0_to_Bcan_two, &B_0_two, &B_can_two, &(pk->curve), TORSION_PLUS_EVEN_POWER);

	ec_curve_to_basis_3f_to_hint(&B_can_three, &(pk->curve), sk->hint_sk + 1, TORSION_PLUS_THREE_POWER);
	change_of_basis_matrix_three(&sk->mat_B0_to_Bcan_three, &B_0_three, &B_can_three, &(pk->curve));
#endif

	ibz_finalize(&n);
	ibz_finalize(&p_quarter);
}

void protocols_keygen(unsigned char* state_pk, unsigned char* state_sk)
{
	public_key_t pk;
	secret_key_t sk;

	public_key_init(&pk);
	secret_key_init(&sk);

	protocols_keygen_internal(&pk, &sk);
	public_key_to_bytes(state_pk, &pk);
	secret_key_to_bytes(state_sk, &sk, &pk);

	public_key_finalize(&pk);
	secret_key_finalize(&sk);
}



