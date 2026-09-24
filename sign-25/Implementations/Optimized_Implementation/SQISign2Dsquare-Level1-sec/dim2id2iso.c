#include <quaternion.h>
#include <ec.h>
#include <endomorphism_action.h>
#include <dim2id2iso.h>
#include <inttypes.h>
#include <locale.h>
#include <bench.h>
#include <curve_extras.h>
#include <id2iso.h>
#include <tools.h>
#include <ec_params.h>

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
int
fixed_degree_isogeny(theta_chain_t *isog,
	quat_left_ideal_t *lideal,
	ibz_t *u,
	int extra_info)
{
    int found, length;
    ec_basis_t basis_two, basis_three;
	ibz_t norm_alpha, temp, pow_two;
    ibz_vec_2_t vec;
    quat_alg_elem_t alpha;
    theta_couple_curve_t E01;
    theta_couple_point_t T1, T2, T1m2;

	ibz_init(&norm_alpha);
    ibz_init(&temp);
	ibz_init(&pow_two);
    ibz_vec_2_init(&vec);
    quat_alg_elem_init(&alpha);

	if (extra_info) {
		// 2^(n+2)-torsion points
		length = TORSION_PLUS_EVEN_POWER - 2;
		ibz_pow(&pow_two, &ibz_const_two, length);
	}
	else {
		// 2^n-torsion points
		length = TORSION_PLUS_EVEN_POWER;
		ibz_copy(&pow_two, &TORSION_PLUS_2POWER);
	}

	// Compute norm_alpha = u*(2^a-u)*3^b
	assert(ibz_cmp(u, &pow_two) < 0);
	ibz_sub(&norm_alpha, &pow_two, u);
	ibz_mul(&norm_alpha, &norm_alpha, u);
	ibz_mul(&norm_alpha, &norm_alpha, &TORSION_PLUS_3POWER);

    // Compute alpha
    found = represent_integer(&alpha, &norm_alpha, &QUATALG_PINFTY);
    assert(found);
    (void)found;

    // lideal = O_0<alpha,u>
    quat_lideal_create_from_primitive(lideal, &alpha, u, &MAXORD_O0, &QUATALG_PINFTY);
    
    // decomposing alpha as a composition of two isogenies of degree 3^b and u*(2^a-u)
    // first compute the kernel of the 3^b-degree isogeny
    quat_to_isogeny_dlog_three(&vec, TORSION_PLUS_THREE_POWER, &alpha);
    ec_curve_t E0 = CURVE_E0;
    ec_curve_init(&E0);
    ec_point_t kernel;
    digit_t scalars[2][NWORDS_ORDER_3];
    ibz_to_digit_array(scalars[0], &(vec[0]));
    ibz_to_digit_array(scalars[1], &(vec[1]));
    copy_point(&(basis_three.P), &(BASIS_THREE.P));
    copy_point(&(basis_three.Q), &(BASIS_THREE.Q));
    copy_point(&(basis_three.PmQ), &(BASIS_THREE.PmQ));
    ec_biscalar_mul_3(&kernel, &E0, scalars[0], scalars[1], &basis_three);

    // then compute the torsion 2power points carried by the 3^b-degree isogeny
    copy_point(&(basis_two.P), &(BASIS_EVEN.P));
    copy_point(&(basis_two.Q), &(BASIS_EVEN.Q));
    copy_point(&(basis_two.PmQ), &(BASIS_EVEN.PmQ));

    // multiple alpha by (u*3^b)^-1 mod 2^a
    ibz_mul(&temp, u, &TORSION_PLUS_3POWER);
    ibz_invmod(&temp, &temp, &TORSION_PLUS_2POWER);
    assert(!ibz_is_even(&temp));
    ibz_mul(&(alpha.coord[0]), &(alpha.coord[0]), &temp);
    ibz_mul(&(alpha.coord[1]), &(alpha.coord[1]), &temp);
    ibz_mul(&(alpha.coord[2]), &(alpha.coord[2]), &temp);
    ibz_mul(&(alpha.coord[3]), &(alpha.coord[3]), &temp);

    // apply alpha to basis_two
    endomorphism_application_even_basis(&basis_two, &E0, &alpha, TORSION_PLUS_EVEN_POWER);

    // compute the 3^b-degree isogeny
    ec_isog_three_t phi;
    phi.curve = E0;
    phi.kernel = kernel;
    phi.length = TORSION_PLUS_THREE_POWER;
    ec_point_t basis_two_points[3];
    copy_point(&basis_two_points[0], &basis_two.P);
    copy_point(&basis_two_points[1], &basis_two.Q);
    copy_point(&basis_two_points[2], &basis_two.PmQ);
    ec_eval_three(&(E01.E2), &phi, basis_two_points, 3, NULL);
    copy_point(&basis_two.P, &basis_two_points[0]);
    copy_point(&basis_two.Q, &basis_two_points[1]);
    copy_point(&basis_two.PmQ, &basis_two_points[2]);

    // now set up for the dim2 isogeny
    E01.E1 = E0;
    copy_point(&(T1.P1), &(BASIS_EVEN.P));
    copy_point(&(T2.P1), &(BASIS_EVEN.Q));
    copy_point(&(T1m2.P1), &(BASIS_EVEN.PmQ));
    copy_point(&(T1.P2), &(basis_two.P));
    copy_point(&(T2.P2), &(basis_two.Q));
    copy_point(&(T1m2.P2), &(basis_two.PmQ));

    theta_chain_comput_strategy(isog,
        length,
        &E01,
        &T1,
        &T2,
        &T1m2,
        strategies[0],
        extra_info);

	ibz_finalize(&norm_alpha);
    ibz_finalize(&temp);
	ibz_finalize(&pow_two);
    ibz_vec_2_finalize(&vec);
    quat_alg_elem_finalize(&alpha);

    return 1;
}

// u < 2^NONSMOOTH_PART and u is prime
int
fixed_small_degree_isogeny(theta_chain_t *isog,
	quat_left_ideal_t *lideal,
	ibz_t *u,
	int extra_info)
{
	int found, length;
	ec_basis_t basis_two, basis_three;
	ibz_t norm_alpha, temp, nonsmooth_bound;
	ibz_t q, r;
	ibz_vec_2_t vec;
	quat_alg_elem_t alpha;
	theta_couple_curve_t E01;
	theta_couple_point_t T1, T2, T1m2;

	ibz_init(&norm_alpha);
	ibz_init(&temp);
	ibz_init(&q);
	ibz_init(&r);
	ibz_init(&nonsmooth_bound);
	ibz_vec_2_init(&vec);
	quat_alg_elem_init(&alpha);

	if (extra_info)
		length = TORSION_PLUS_EVEN_POWER - 2;
	else
		length = TORSION_PLUS_EVEN_POWER;

	// Compute norm_alpha = u*(2^NONSMOOTH_PART-u)*2^(TORSION_PLUS_EVEN_POWER-NONSMOOTH_PART-2)*3^b
	ibz_pow(&nonsmooth_bound, &ibz_const_two, NONSMOOTH_PART);
	assert(ibz_cmp(u, &nonsmooth_bound) < 0);
	ibz_sub(&norm_alpha, &nonsmooth_bound, u);
	ibz_mul(&norm_alpha, &norm_alpha, u);
	ibz_mul(&norm_alpha, &norm_alpha, &TORSION_PLUS_3POWER);
	ibz_pow(&temp, &ibz_const_two, length - NONSMOOTH_PART);
	ibz_mul(&norm_alpha, &norm_alpha, &temp);
	

	// Compute alpha
	found = represent_integer_exact(&alpha, &norm_alpha, &temp, &QUATALG_PINFTY);
	assert(found);
    (void)found;

	int val_2 = length - NONSMOOTH_PART;
	{	
		ibz_div(&q, &r, &temp, &ibz_const_two);
		while (ibz_is_zero(&r)) {
			val_2 -= 2;
			ibz_copy(&temp, &q);
			ibz_div(&q, &r, &temp, &ibz_const_two);
		}
#if _DEBUG
		ibz_pow(&q, &ibz_const_two, val_2);
		ibz_pow(&r, &ibz_const_three, TORSION_PLUS_THREE_POWER);
		ibz_mul(&q, &q, &r);
		ibz_sub(&r, &nonsmooth_bound, u);
		ibz_mul(&r, &r, u);
		ibz_mul(&q, &q, &r);
		ibz_div(&temp, &r, &norm_alpha, &q);
		assert(ibz_is_zero(&r));
		assert(!(ibz_is_even(&temp)));
		ibz_div(&q, &r, &temp, &ibz_const_three);
		assert(!(ibz_is_zero(&r)));
#endif
	}

	// lideal = O_0<alpha,u>
	quat_lideal_create_from_primitive(lideal, &alpha, u, &MAXORD_O0, &QUATALG_PINFTY);

	/* decomposing alpha as a composition of 3 isogenies of degree 3^b, 2^val_2 and u*(2^NONSMOOTH_PART-u) */
	// first compute the kernel of the 3^b-degree isogeny
	quat_to_isogeny_dlog_three(&vec, TORSION_PLUS_THREE_POWER, &alpha);
	ec_curve_t E0 = CURVE_E0;
	ec_curve_init(&E0);
	ec_point_t kernel_3, kernel_2;
	digit_t scalars_3[2][NWORDS_ORDER_3], scalars_2[2][NWORDS_ORDER_2];
	ibz_to_digit_array(scalars_3[0], &(vec[0]));
	ibz_to_digit_array(scalars_3[1], &(vec[1]));
	copy_point(&(basis_three.P), &(BASIS_THREE.P));
	copy_point(&(basis_three.Q), &(BASIS_THREE.Q));
	copy_point(&(basis_three.PmQ), &(BASIS_THREE.PmQ));
	ec_biscalar_mul_3(&kernel_3, &E0, scalars_3[0], scalars_3[1], &basis_three);
	
	assert(test_point_order_threef(&kernel_3, &E0, TORSION_PLUS_THREE_POWER));

	// then compute the kernel of the 2^val_2-degree isogeny
	quat_to_isogeny_dlog_two(&vec, val_2, &alpha);
	ibz_to_digit_array(scalars_2[0], &(vec[0]));
	ibz_to_digit_array(scalars_2[1], &(vec[1]));
	copy_point(&basis_two.P, &BASIS_EVEN.P);
	copy_point(&basis_two.Q, &BASIS_EVEN.Q);
	copy_point(&basis_two.PmQ, &BASIS_EVEN.PmQ);
	ec_biscalar_mul(&kernel_2, &E0, scalars_2[0], scalars_2[1], &basis_two);
	assert(test_point_order_twof(&kernel_2, &E0, val_2));

	// then compute the torsion 2power points carried by the 3^b-degree isogeny

	// multiple alpha by (u*3^b)^-1 mod 2^a
	ibz_pow(&temp, &ibz_const_three, TORSION_PLUS_THREE_POWER);
	ibz_mul(&temp, u, &temp);
	ibz_invmod(&temp, &temp, &TORSION_PLUS_2POWER);
	assert(!ibz_is_even(&temp));
	ibz_mul(&(alpha.coord[0]), &(alpha.coord[0]), &temp);
	ibz_mul(&(alpha.coord[1]), &(alpha.coord[1]), &temp);
	ibz_mul(&(alpha.coord[2]), &(alpha.coord[2]), &temp);
	ibz_mul(&(alpha.coord[3]), &(alpha.coord[3]), &temp);

	// apply alpha to basis_two
	endomorphism_application_even_basis(&basis_two, &E0, &alpha, TORSION_PLUS_EVEN_POWER);

	// compute the 3^b-degree isogeny
	ec_isog_three_t phi;
	phi.curve = E0;
	phi.kernel = kernel_3;
	phi.length = TORSION_PLUS_THREE_POWER;
	ec_point_t points[4];
	copy_point(&(points[0]), &basis_two.P);
	copy_point(&(points[1]), &basis_two.Q);
	copy_point(&(points[2]), &basis_two.PmQ);
	copy_point(&(points[3]), &kernel_2);
	ec_eval_three(&(E01.E2), &phi, points, 4, NULL);

	// compute the 2^val_2-degree isogeny
	ec_isog_even_t phi_even;
	phi_even.curve = E01.E2;
	phi_even.kernel = points[3];
	phi_even.length = val_2;
	copy_point(&basis_two.P, &(points[0]));
	copy_point(&basis_two.Q, &(points[1]));
	copy_point(&basis_two.PmQ, &(points[2]));
	ec_point_t basis_two_points_even[3];
	copy_point(&basis_two_points_even[0], &basis_two.P);
	copy_point(&basis_two_points_even[1], &basis_two.Q);
	copy_point(&basis_two_points_even[2], &basis_two.PmQ);
	ec_eval_even(&(E01.E2), &phi_even, basis_two_points_even, 3);
	copy_point(&basis_two.P, &basis_two_points_even[0]);
	copy_point(&basis_two.Q, &basis_two_points_even[1]);
	copy_point(&basis_two.PmQ, &basis_two_points_even[2]);

	// now set up for the dim2 isogeny
	E01.E1 = E0;
	ec_dbl_iter(&(T1.P1), length - NONSMOOTH_PART, &E0, &(BASIS_EVEN.P));
	ec_dbl_iter(&(T2.P1), length - NONSMOOTH_PART, &E0, &(BASIS_EVEN.Q));
	ec_dbl_iter(&(T1m2.P1), length - NONSMOOTH_PART, &E0, &(BASIS_EVEN.PmQ));
	ec_dbl_iter(&(basis_two.P), length - NONSMOOTH_PART - val_2, &E01.E2, &(basis_two.P));
	ec_dbl_iter(&(basis_two.Q), length - NONSMOOTH_PART - val_2, &E01.E2, &(basis_two.Q));
	ec_dbl_iter(&(basis_two.PmQ), length - NONSMOOTH_PART - val_2, &E01.E2, &(basis_two.PmQ));
	copy_point(&(T1.P2), &(basis_two.P));
	copy_point(&(T2.P2), &(basis_two.Q));
	copy_point(&(T1m2.P2), &(basis_two.PmQ));
	assert(test_point_order_twof(&T1.P1, &E01.E1, NONSMOOTH_PART + 2 * extra_info));
	assert(test_point_order_twof(&T2.P1, &E01.E1, NONSMOOTH_PART + 2 * extra_info));
	assert(test_point_order_twof(&T1m2.P1, &E01.E1, NONSMOOTH_PART + 2 * extra_info));
	assert(test_point_order_twof(&T1.P2, &E01.E2, NONSMOOTH_PART + 2 * extra_info));
	assert(test_point_order_twof(&T2.P2, &E01.E2, NONSMOOTH_PART + 2 * extra_info));
	assert(test_point_order_twof(&T1m2.P2, &E01.E2, NONSMOOTH_PART + 2 * extra_info));

	theta_chain_comput_strategy(isog,
		NONSMOOTH_PART,
		&E01,
		&T1,
		&T2,
		&T1m2,
		strategies[1],
		extra_info);

	ibz_finalize(&norm_alpha);
	ibz_finalize(&temp);
	ibz_finalize(&nonsmooth_bound);
	ibz_finalize(&q);
	ibz_finalize(&r);
	ibz_vec_2_finalize(&vec);
	quat_alg_elem_finalize(&alpha);

	return 1;
}

