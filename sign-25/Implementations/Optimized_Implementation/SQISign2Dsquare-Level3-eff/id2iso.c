#include <quaternion.h>
#include <ec.h>
#include <endomorphism_action.h>
#include <id2iso.h>
#include <inttypes.h>
#include <locale.h>
#include <bench.h>
#include <curve_extras.h>
#include <biextension.h>


void
ec_mul_ibz(ec_point_t *res, const ec_curve_t *curve, const ibz_t *scalarP, const ec_point_t *P)
{

    digit_t scalars[NWORDS_FIELD] = { 0 };
    if (ibz_cmp(scalarP, &ibz_const_one) == 0) {
        copy_point(res, P);
    } else {
        ibz_to_digit_array(scalars, scalarP);
        ec_mul(res, curve, scalars, P);
    }
}

void
ec_biscalar_mul_ibz(ec_point_t *res,
                    const ec_curve_t *curve,
                    const ibz_t *scalarP,
                    const ibz_t *scalarQ,
                    const ec_basis_t *PQ,
                    int f)
{

    digit_t scalars[2][NWORDS_ORDER_2] = { 0 };

    if (ibz_cmp(scalarP, &ibz_const_zero) == 0) {
        ec_mul_ibz(res, curve, scalarQ, &PQ->Q);
    } else if (ibz_cmp(scalarQ, &ibz_const_zero) == 0) {
        ec_mul_ibz(res, curve, scalarP, &PQ->P);
    } else {
        ibz_to_digit_array(scalars[0], scalarP);
        ibz_to_digit_array(scalars[1], scalarQ);
        ec_biscalar_mul_bounded(res, curve, scalars[0], scalars[1], PQ, f);
    }
}



// helper function to apply a matrix to a basis of E[2^f]
// works in place
void
matrix_application_even_basis(ec_basis_t *bas, const ec_curve_t *E, ibz_mat_2x2_t *mat, int f)
{
    digit_t scalars[2][NWORDS_ORDER_2] = { 0 };

    ibz_t tmp, pow_two;
    ibz_init(&tmp);
    ibz_init(&pow_two);
    ibz_pow(&pow_two, &ibz_const_two, f);

    ec_basis_t tmp_bas;
    copy_point(&tmp_bas.P, &bas->P);
    copy_point(&tmp_bas.Q, &bas->Q);
    copy_point(&tmp_bas.PmQ, &bas->PmQ);

    // reduction mod 2f
    ibz_mod(&(*mat)[0][0], &(*mat)[0][0], &pow_two);
    ibz_mod(&(*mat)[0][1], &(*mat)[0][1], &pow_two);
    ibz_mod(&(*mat)[1][0], &(*mat)[1][0], &pow_two);
    ibz_mod(&(*mat)[1][1], &(*mat)[1][1], &pow_two);

    // first basis element
    ibz_to_digit_array(scalars[0], &(*mat)[0][0]);
    // ibz_set(&mat[0][1],0);
    ibz_to_digit_array(scalars[1], &(*mat)[1][0]);
    ec_biscalar_mul_bounded(&bas->P, E, scalars[0], scalars[1], &tmp_bas, f);
    ibz_to_digit_array(scalars[0], &(*mat)[0][1]);
    ibz_to_digit_array(scalars[1], &(*mat)[1][1]);
    ec_biscalar_mul_bounded(&bas->Q, E, scalars[0], scalars[1], &tmp_bas, f);

    ibz_sub(&tmp, &(*mat)[0][0], &(*mat)[0][1]);
    ibz_mod(&tmp, &tmp, &pow_two);
    ibz_to_digit_array(scalars[0], &tmp);
    ibz_sub(&tmp, &(*mat)[1][0], &(*mat)[1][1]);
    ibz_mod(&tmp, &tmp, &pow_two);
    ibz_to_digit_array(scalars[1], &tmp);
    ec_biscalar_mul_bounded(&bas->PmQ, E, scalars[0], scalars[1], &tmp_bas, f);

    ibz_finalize(&tmp);
    ibz_finalize(&pow_two);
}

// function to apply a matrix to a basis of E[3^b]
void
matrix_application_three_basis(ec_basis_t* bas, const ec_curve_t* E, ibz_mat_2x2_t* mat)
{
	digit_t scalars[2][NWORDS_ORDER_3] = { 0 };
	ibz_t tmp;
	ibz_init(&tmp);

	ec_basis_t tmp_bas;
	copy_point(&tmp_bas.P, &bas->P);
	copy_point(&tmp_bas.Q, &bas->Q);
	copy_point(&tmp_bas.PmQ, &bas->PmQ);

	// reduction mod 3^b
	ibz_mod(&(*mat)[0][0], &(*mat)[0][0], &TORSION_PLUS_3POWER);
	ibz_mod(&(*mat)[0][1], &(*mat)[0][1], &TORSION_PLUS_3POWER);
	ibz_mod(&(*mat)[1][0], &(*mat)[1][0], &TORSION_PLUS_3POWER);
	ibz_mod(&(*mat)[1][1], &(*mat)[1][1], &TORSION_PLUS_3POWER);

	ibz_to_digit_array(scalars[0], &(*mat)[0][0]);
	ibz_to_digit_array(scalars[1], &(*mat)[1][0]);
	ec_biscalar_mul_3(&bas->P, E, scalars[0], scalars[1], &tmp_bas);
	ibz_to_digit_array(scalars[0], &(*mat)[0][1]);
	ibz_to_digit_array(scalars[1], &(*mat)[1][1]);
	ec_biscalar_mul_3(&bas->Q, E, scalars[0], scalars[1], &tmp_bas);

	ibz_sub(&tmp, &(*mat)[0][0], &(*mat)[0][1]);
	ibz_mod(&tmp, &tmp, &TORSION_PLUS_3POWER);
	ibz_to_digit_array(scalars[0], &tmp);
	ibz_sub(&tmp, &(*mat)[1][0], &(*mat)[1][1]);
	ibz_mod(&tmp, &tmp, &TORSION_PLUS_3POWER);
	ibz_to_digit_array(scalars[1], &tmp);
	ec_biscalar_mul_3(&bas->PmQ, E, scalars[0], scalars[1], &tmp_bas);


	ibz_finalize(&tmp);
}

// helper function to apply some endomorphism of E0 on the precomputed basis of E[2^f]
// works in place
void
endomorphism_application_even_basis(ec_basis_t *bas,
                                    const ec_curve_t *E,
                                    quat_alg_elem_t *theta,
                                    int f)
{
    ibz_t tmp;
    ibz_init(&tmp);
    ibz_vec_4_t coeffs;
    ibz_vec_4_init(&coeffs);
    ibz_mat_2x2_t mat;
    ibz_mat_2x2_init(&mat);

    ibz_t content;
    ibz_init(&content);

    // // decomposing theta on the basis
    quat_alg_make_primitive(&coeffs, &content, theta, &MAXORD_O0, &QUATALG_PINFTY);
    assert(ibz_get(&content) % 2 == 1);

    ibz_set(&mat[0][0], 0);
    ibz_set(&mat[0][1], 0);
    ibz_set(&mat[1][0], 0);
    ibz_set(&mat[1][1], 0);

    // computing the matrix
    for (unsigned i = 0; i < 2; ++i) {
        ibz_add(&mat[i][i], &mat[i][i], &coeffs[0]);
        for (unsigned j = 0; j < 2; ++j) {
            ibz_mul(&tmp, &ACTION_GEN2[i][j], &coeffs[1]);
            ibz_add(&mat[i][j], &mat[i][j], &tmp);
            ibz_mul(&tmp, &ACTION_GEN3[i][j], &coeffs[2]);
            ibz_add(&mat[i][j], &mat[i][j], &tmp);
            ibz_mul(&tmp, &ACTION_GEN4[i][j], &coeffs[3]);
            ibz_add(&mat[i][j], &mat[i][j], &tmp);
            ibz_mul(&mat[i][j], &mat[i][j], &content);
            // ibz_mod(&mat[i][j],&mat[i][j],&twopow);
        }
    }

    // and now we apply it
    matrix_application_even_basis(bas, E, &mat, f);

    ibz_vec_4_finalize(&coeffs);
    ibz_mat_2x2_finalize(&mat);
    ibz_finalize(&content);
}

void quat_to_isogeny_dlog_two(ibz_vec_2_t* vec,
	int f,
	const quat_alg_elem_t* alpha)
{
	ibz_t tmp, pow_2, res0, res1, content;
	ibz_vec_4_t coeffs;

	ibz_mat_2x2_t mat;
	ibz_init(&tmp);
	ibz_init(&pow_2);
	ibz_init(&res0);
	ibz_init(&res1);
	ibz_init(&content);
	ibz_vec_4_init(&coeffs);
	ibz_mat_2x2_init(&mat);

	ibz_pow(&pow_2, &ibz_const_two, f);

	quat_alg_make_primitive(&coeffs, &content, alpha, &MAXORD_O0, &QUATALG_PINFTY);
	assert(ibz_get(&content) % 2 == 1);

	ibz_set(&mat[0][0], 0);
	ibz_set(&mat[0][1], 0);
	ibz_set(&mat[1][0], 0);
	ibz_set(&mat[1][1], 0);

	for (unsigned i = 0; i < 2; ++i) {
		ibz_add(&mat[i][i], &mat[i][i], &coeffs[0]);
		for (unsigned j = 0; j < 2; ++j) {
			ibz_mul(&tmp, &ACTION_GEN2[i][j], &coeffs[1]);
			ibz_add(&mat[i][j], &mat[i][j], &tmp);
			ibz_mul(&tmp, &ACTION_GEN3[i][j], &coeffs[2]);
			ibz_add(&mat[i][j], &mat[i][j], &tmp);
			ibz_mul(&tmp, &ACTION_GEN4[i][j], &coeffs[3]);
			ibz_add(&mat[i][j], &mat[i][j], &tmp);
			ibz_mul(&mat[i][j], &mat[i][j], &content);
			ibz_mod(&mat[i][j], &mat[i][j], &pow_2);
		}
	}

	ibz_mod(&res0, &mat[0][0], &ibz_const_two);
	ibz_mod(&res1, &mat[1][0], &ibz_const_two);
	if (ibz_is_zero(&res0) && ibz_is_zero(&res1)) {
		ibz_copy(&((*vec)[0]), &mat[0][1]);
		ibz_copy(&((*vec)[1]), &mat[1][1]);
	}
	else {
		ibz_copy(&((*vec)[0]), &mat[0][0]);
		ibz_copy(&((*vec)[1]), &mat[1][0]);
	}

	ibz_pow(&pow_2, &ibz_const_two, TORSION_PLUS_EVEN_POWER - f);
	ibz_mul(&((*vec)[0]), &((*vec)[0]), &pow_2);
	ibz_mul(&((*vec)[1]), &((*vec)[1]), &pow_2);

#if _DEBUG
	assert(ibz_cmp(&((*vec)[0]), &TORSION_PLUS_2POWER) < 0);
	assert(ibz_cmp(&((*vec)[1]), &TORSION_PLUS_2POWER) < 0);
#endif

	ibz_finalize(&tmp);
	ibz_finalize(&pow_2);
	ibz_finalize(&res0);
	ibz_finalize(&res1);
	ibz_finalize(&content);
	ibz_vec_4_finalize(&coeffs);
	ibz_mat_2x2_finalize(&mat);
}

void quat_to_isogeny_dlog_three(ibz_vec_2_t* vec,
	int f,
	const quat_alg_elem_t* alpha)
{
	ibz_t tmp, pow_3, res0, res1, content;
	ibz_vec_4_t coeffs;

	ibz_mat_2x2_t mat;
	ibz_init(&tmp);
	ibz_init(&pow_3);
	ibz_init(&res0);
	ibz_init(&res1);
	ibz_init(&content);
	ibz_vec_4_init(&coeffs);
	ibz_mat_2x2_init(&mat);

	ibz_pow(&pow_3, &ibz_const_three, f);

	quat_alg_make_primitive(&coeffs, &content, alpha, &MAXORD_O0, &QUATALG_PINFTY);
	assert(ibz_get(&content) % 2 == 1);

	ibz_set(&mat[0][0], 0);
	ibz_set(&mat[0][1], 0);
	ibz_set(&mat[1][0], 0);
	ibz_set(&mat[1][1], 0);

	for (unsigned i = 0; i < 2; ++i) {
		ibz_add(&mat[i][i], &mat[i][i], &coeffs[0]);
		for (unsigned j = 0; j < 2; ++j) {
			ibz_mul(&tmp, &ACTION_GEN2[i][j], &coeffs[1]);
			ibz_add(&mat[i][j], &mat[i][j], &tmp);
			ibz_mul(&tmp, &ACTION_GEN3[i][j], &coeffs[2]);
			ibz_add(&mat[i][j], &mat[i][j], &tmp);
			ibz_mul(&tmp, &ACTION_GEN4[i][j], &coeffs[3]);
			ibz_add(&mat[i][j], &mat[i][j], &tmp);
			ibz_mul(&mat[i][j], &mat[i][j], &content);
			ibz_mod(&mat[i][j],&mat[i][j],&pow_3);
		}
	}

	// Only mat[0][0] and mat[1][0] are needed; some steps below may be removable.
	//for (unsigned i = 0; i < 2; ++i) {
	//    ibz_add(&mat[i][i], &mat[i][i], &((alpha->coord)[0]));
	//    for (unsigned j = 0; j < 2; ++j) {
	//        ibz_mul(&tmp, &ACTION_I[i][j], &((alpha->coord)[1]));
	//        ibz_sub(&mat[i][j], &mat[i][j], &tmp);
	//        ibz_mul(&tmp, &ACTION_J[i][j], &((alpha->coord)[2]));
	//        ibz_sub(&mat[i][j], &mat[i][j], &tmp);
	//        ibz_mul(&tmp, &ACTION_K[i][j], &((alpha->coord)[3]));
	//        ibz_sub(&mat[i][j], &mat[i][j], &tmp);
	//        //                ibz_mod(&mat[i][j], &mat[i][j], &TORSION_ODD);
	//    }
	//}
	ibz_mod(&res0, &mat[0][0], &ibz_const_three);
	ibz_mod(&res1, &mat[1][0], &ibz_const_three);
	if (ibz_is_zero(&res0) && ibz_is_zero(&res1)) {
		ibz_copy(&((*vec)[0]), &mat[0][1]);
		ibz_copy(&((*vec)[1]), &mat[1][1]);
	}
	else {
		ibz_copy(&((*vec)[0]), &mat[0][0]);
		ibz_copy(&((*vec)[1]), &mat[1][0]);
	}

    ibz_pow(&pow_3, &ibz_const_three, TORSION_PLUS_THREE_POWER - f);
	ibz_mul(&((*vec)[0]), &((*vec)[0]), &pow_3);
	ibz_mul(&((*vec)[1]), &((*vec)[1]), &pow_3);

#if _DEBUG
    assert(ibz_cmp(&((*vec)[0]), &TORSION_PLUS_3POWER) < 0);
    assert(ibz_cmp(&((*vec)[1]), &TORSION_PLUS_3POWER) < 0);
#endif

	ibz_finalize(&tmp);
	ibz_finalize(&pow_3);
	ibz_finalize(&res0);
	ibz_finalize(&res1);
	ibz_finalize(&content);
	ibz_vec_4_finalize(&coeffs);
	ibz_mat_2x2_finalize(&mat);
}


// vec3[0] and vec3[1] must not both be divisible by 3.
// f <= b, and the norm of lideal is 3^f.
void
id2iso_kernel_dlogs_to_ideal_three_general(quat_left_ideal_t *lideal, 
	const ibz_vec_2_t *vec3,
	unsigned int f)
{
	assert(f > 0);

	ibz_t pow3;
	ibz_vec_2_t vec;
	ibz_init(&pow3);
	ibz_vec_2_init(&vec);

	ibz_pow(&pow3, &ibz_const_three, f);

	{
		ibz_mat_2x2_t mat;
		ibz_mat_2x2_init(&mat);

		ibz_copy(&mat[0][0], &(*vec3)[0]);
		ibz_copy(&mat[1][0], &(*vec3)[1]);

		ibz_mat_2x2_eval(&vec, &ACTION_I, vec3);
		ibz_copy(&mat[0][1], &vec[0]);
		ibz_copy(&mat[1][1], &vec[1]);
		ibz_mod(&mat[0][1], &mat[0][1], &pow3);
		ibz_mod(&mat[1][1], &mat[1][1], &pow3);

		ibz_mat_2x2_t inv;
		ibz_mat_2x2_init(&inv);
		{
			int inv_ok = ibz_2x2_inv_mod(&inv, &mat, &pow3);
			assert(inv_ok);
		}

		ibz_mat_2x2_eval(&vec, &ACTION_J, vec3);
		ibz_mat_2x2_eval(&vec, &inv, &vec);

		ibz_mat_2x2_finalize(&mat);
		ibz_mat_2x2_finalize(&inv);
	}

	// final result: a - j + b*i
	quat_alg_elem_t gen;
	quat_alg_elem_init(&gen);
	ibz_set(&gen.denom, 1);
	ibz_copy(&(gen.coord[0]), &vec[0]);
	ibz_copy(&(gen.coord[1]), &vec[1]);
	ibz_set(&(gen.coord[2]), -1);
	ibz_set(&(gen.coord[3]), 0);

#if _DEBUG
	{
		// check that vec3 is in the kernel of the matix of gen
		ibz_mat_2x2_t mat, mattmp;
		ibz_vec_2_t vectest;
		ibz_vec_2_init(&vectest);
		ibz_mat_2x2_init(&mat);
		ibz_mat_2x2_init(&mattmp);

		ibz_copy(&mat[0][0], &(gen.coord[0]));
		ibz_copy(&mat[1][1], &(gen.coord[0]));

		ibz_mat_2x2_copy(&mattmp, &ACTION_I);
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				ibz_mul(&(mattmp[i][j]), &(mattmp[i][j]), &(gen.coord[1]));
				ibz_mod(&(mattmp[i][j]), &(mattmp[i][j]), &pow3);
			}
		}
		ibz_mat_2x2_add(&mat, &mat, &mattmp);

		ibz_mat_2x2_copy(&mattmp, &ACTION_J);
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				ibz_mul(&(mattmp[i][j]), &(mattmp[i][j]), &(gen.coord[2]));
				ibz_mod(&(mattmp[i][j]), &(mattmp[i][j]), &pow3);
			}
		}
		ibz_mat_2x2_add(&mat, &mat, &mattmp);

		ibz_mat_2x2_copy(&mattmp, &ACTION_K);
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				ibz_mul(&(mattmp[i][j]), &(mattmp[i][j]), &(gen.coord[3]));
				ibz_mod(&(mattmp[i][j]), &(mattmp[i][j]), &pow3);
			}
		}
		ibz_mat_2x2_add(&mat, &mat, &mattmp);

		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {
				ibz_mod(&(mat[i][j]), &(mat[i][j]), &pow3);
				// ibz_printf("%Zd\n", &(mat[i][j]));
			}
		}

		ibz_mat_2x2_eval(&vectest, &mat, vec3);

		ibz_mod(&(vectest[0]), &(vectest[0]), &pow3);
		ibz_mod(&(vectest[1]), &(vectest[1]), &pow3);
		assert(ibz_is_zero(&(vectest[0])));
		assert(ibz_is_zero(&(vectest[1])));

		ibz_vec_2_finalize(&vectest);
		ibz_mat_2x2_finalize(&mat);
		ibz_mat_2x2_finalize(&mattmp);
	}
#endif

	quat_lideal_create_from_primitive(
		lideal, &gen, &pow3, &MAXORD_O0, &QUATALG_PINFTY);

	assert(0 == ibz_cmp(&lideal->norm, &pow3));

	ibz_finalize(&pow3);
	ibz_vec_2_finalize(&vec);
	quat_alg_elem_finalize(&gen);
}


void
id2iso_kernel_dlogs_to_ideal_three(quat_left_ideal_t *lideal, const ibz_vec_2_t *vec3)
{

    // algorithm: apply endomorphisms 1 and i to the kernel point,
    // the result should form a basis of the respective torsion subgroup.
    // then apply j to the kernel point and decompose over said basis.
    // hence we have an equation a*P + b*[j]P == [i]P, which will
    // easily reveal an endomorphism that kills P.

    ibz_vec_2_t vec;
    ibz_vec_2_init(&vec);

    {
        ibz_mat_2x2_t mat;
        ibz_mat_2x2_init(&mat);

        ibz_copy(&mat[0][0], &(*vec3)[0]);
        ibz_copy(&mat[1][0], &(*vec3)[1]);

        ibz_mat_2x2_eval(&vec, &ACTION_I, vec3);
        ibz_copy(&mat[0][1], &vec[0]);
        ibz_copy(&mat[1][1], &vec[1]);
        ibz_mod(&mat[0][1], &mat[0][1], &TORSION_PLUS_3POWER);
        ibz_mod(&mat[1][1], &mat[1][1], &TORSION_PLUS_3POWER);

        ibz_mat_2x2_t inv;
        ibz_mat_2x2_init(&inv);
        {
            int inv_ok = ibz_2x2_inv_mod(&inv, &mat, &TORSION_PLUS_3POWER);
            assert(inv_ok);
        }

        ibz_mat_2x2_eval(&vec, &ACTION_J, vec3);
        ibz_mat_2x2_eval(&vec, &inv, &vec);

        ibz_mat_2x2_finalize(&mat);
        ibz_mat_2x2_finalize(&inv);
    }

    // final result: a - j + b*i
    quat_alg_elem_t gen;
    quat_alg_elem_init(&gen);
    ibz_set(&gen.denom, 1);
    ibz_copy(&(gen.coord[0]), &vec[0]);
    ibz_copy(&(gen.coord[1]), &vec[1]);
    ibz_set(&(gen.coord[2]), -1);
    ibz_set(&(gen.coord[3]), 0);

#if _DEBUG
    {
        // check that vec3 is in the kernel of the matix of gen
        ibz_mat_2x2_t mat, mattmp;
        ibz_vec_2_t vectest;
        ibz_vec_2_init(&vectest);
        ibz_mat_2x2_init(&mat);
        ibz_mat_2x2_init(&mattmp);

        ibz_copy(&mat[0][0], &(gen.coord[0]));
        ibz_copy(&mat[1][1], &(gen.coord[0]));

        ibz_mat_2x2_copy(&mattmp, &ACTION_I);
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                ibz_mul(&(mattmp[i][j]), &(mattmp[i][j]), &(gen.coord[1]));
                ibz_mod(&(mattmp[i][j]), &(mattmp[i][j]), &TORSION_PLUS_3POWER);
            }
        }
        ibz_mat_2x2_add(&mat, &mat, &mattmp);

        ibz_mat_2x2_copy(&mattmp, &ACTION_J);
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                ibz_mul(&(mattmp[i][j]), &(mattmp[i][j]), &(gen.coord[2]));
                ibz_mod(&(mattmp[i][j]), &(mattmp[i][j]), &TORSION_PLUS_3POWER);
            }
        }
        ibz_mat_2x2_add(&mat, &mat, &mattmp);

        ibz_mat_2x2_copy(&mattmp, &ACTION_K);
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                ibz_mul(&(mattmp[i][j]), &(mattmp[i][j]), &(gen.coord[3]));
                ibz_mod(&(mattmp[i][j]), &(mattmp[i][j]), &TORSION_PLUS_3POWER);
            }
        }
        ibz_mat_2x2_add(&mat, &mat, &mattmp);

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                ibz_mod(&(mat[i][j]), &(mat[i][j]), &TORSION_PLUS_3POWER);
                // ibz_printf("%Zd\n", &(mat[i][j]));
            }
        }

        ibz_mat_2x2_eval(&vectest, &mat, vec3);

        ibz_mod(&(vectest[0]), &(vectest[0]), &TORSION_PLUS_3POWER);
        ibz_mod(&(vectest[1]), &(vectest[1]), &TORSION_PLUS_3POWER);
        assert(ibz_is_zero(&(vectest[0])));
        assert(ibz_is_zero(&(vectest[1])));

        ibz_vec_2_finalize(&vectest);
        ibz_mat_2x2_finalize(&mat);
        ibz_mat_2x2_finalize(&mattmp);
    }
#endif

    quat_lideal_create_from_primitive(
        lideal, &gen, &TORSION_PLUS_3POWER, &MAXORD_O0, &QUATALG_PINFTY);

    assert(0 == ibz_cmp(&lideal->norm, &TORSION_PLUS_3POWER));

    ibz_vec_2_finalize(&vec);
    quat_alg_elem_finalize(&gen);
}

// finds mat such that:
// (mat*v).B2 = v.B1
// where "." is the dot product, defined as (v1,v2).(P,Q) = v1*P + v2*Q
// mat encodes the coordinates of the points of B1 in the basis B2
void
change_of_basis_matrix_two(ibz_mat_2x2_t *mat, ec_basis_t *B1, ec_basis_t *B2, ec_curve_t *E, int f)
{
    // TODO
    digit_t x1[NWORDS_ORDER_2] = { 0 }, x2[NWORDS_ORDER_2] = { 0 }, x3[NWORDS_ORDER_2] = { 0 },
            x4[NWORDS_ORDER_2] = { 0 };
    digit_t x5[NWORDS_ORDER_2] = { 0 }, x6[NWORDS_ORDER_2] = { 0 };
    ibz_t i1, i2, i3, i4, i5, i6;
    ibz_init(&i1);
    ibz_init(&i2);
    ibz_init(&i3);
    ibz_init(&i4);
    ibz_init(&i5);
    ibz_init(&i6);

#if _DEBUG
    assert(test_point_order_twof(&(B2->PmQ), E, f));
#endif

    ec_dlog_2_weil(x1, x2, x3, x4, B2, B1, E, f);

    // copying the digits
    ibz_copy_digit_array(&i1, x1);
    ibz_copy_digit_array(&i2, x2);
    ibz_copy_digit_array(&i3, x3);
    ibz_copy_digit_array(&i4, x4);

    ec_point_t test;
#if _DEBUG
    ec_biscalar_mul(&test, E, x1, x2, B2);
    assert(ec_is_equal(&test, &(B1->P)));
    ec_biscalar_mul(&test, E, x3, x4, B2);
    assert(ec_is_equal(&test, &(B1->Q)));
#endif

    ibz_sub(&i5, &i1, &i3);
    ibz_mod(&i5, &i5, &TORSION_PLUS_2POWER);
    ibz_sub(&i6, &i2, &i4);
    ibz_mod(&i6, &i6, &TORSION_PLUS_2POWER);

    ibz_to_digits(x5, &i5);
    ibz_to_digits(x6, &i6);

    ec_biscalar_mul(&test, E, x5, x6, B2);
    if (!(ec_is_equal(&test, &(B1->PmQ)))) {
        ibz_neg(&i3, &i3);
        ibz_neg(&i4, &i4);

#if _DEBUG
        ibz_sub(&i5, &i1, &i3);
        ibz_mod(&i5, &i5, &TORSION_PLUS_2POWER);
        ibz_sub(&i6, &i2, &i4);
        ibz_mod(&i6, &i6, &TORSION_PLUS_2POWER);

        ibz_to_digits(x5, &i5);
        ibz_to_digits(x6, &i6);

        ec_biscalar_mul(&test, E, x5, x6, B2);
        assert(ec_is_equal(&test, &(B1->PmQ)));
#endif
    }

    ibz_copy(&((*mat)[0][0]), &i1);
    ibz_copy(&((*mat)[1][0]), &i2);
    ibz_copy(&((*mat)[0][1]), &i3);
    ibz_copy(&((*mat)[1][1]), &i4);

    ibz_finalize(&i1);
    ibz_finalize(&i2);
    ibz_finalize(&i3);
    ibz_finalize(&i4);
    ibz_finalize(&i5);
    ibz_finalize(&i6);

    return;
}

// finds mat such that:
// (mat*v).B2 = v.B1
// where "." is the dot product, defined as (v1,v2).(P,Q) = v1*P + v2*Q
void
change_of_basis_matrix_three(ibz_mat_2x2_t *mat,
                             const ec_basis_t *B1,
                             const ec_basis_t *B2,
                             const ec_curve_t *E)
{
    // TODO
    digit_t x1[NWORDS_ORDER_3] = { 0 }, x2[NWORDS_ORDER_3] = { 0 }, x3[NWORDS_ORDER_3] = { 0 },
            x4[NWORDS_ORDER_3] = { 0 };
    digit_t x5[NWORDS_ORDER_3] = { 0 }, x6[NWORDS_ORDER_3] = { 0 };
    ibz_t i1, i2, i3, i4, i5, i6;
    ibz_init(&i1);
    ibz_init(&i2);
    ibz_init(&i3);
    ibz_init(&i4);
    ibz_init(&i5);
    ibz_init(&i6);

    
	ec_basis_t B1_copy = *B1;
	ec_basis_t B2_copy = *B2;
	ec_curve_t E_copy = *E;
	ec_basis_dlog_3_tate(x1, x2, x3, x4, &B2_copy, &B1_copy, &E_copy);

    // copying the digits
    ibz_copy_digit_array(&i1, x1);
    ibz_copy_digit_array(&i2, x2);
    ibz_copy_digit_array(&i3, x3);
    ibz_copy_digit_array(&i4, x4);

    ec_point_t test;
#if _DEBUG
    ec_biscalar_mul_3(&test, E, x1, x2, B2);
    assert(ec_is_equal(&test, &(B1->P)));
    ec_biscalar_mul_3(&test, E, x3, x4, B2);
    assert(ec_is_equal(&test, &(B1->Q)));
#endif

    ibz_sub(&i5, &i1, &i3);
    ibz_mod(&i5, &i5, &TORSION_PLUS_3POWER);
    ibz_sub(&i6, &i2, &i4);
    ibz_mod(&i6, &i6, &TORSION_PLUS_3POWER);

    ibz_to_digits(x5, &i5);
    ibz_to_digits(x6, &i6);

    ec_biscalar_mul_3(&test, E, x5, x6, B2);
    if (!(ec_is_equal(&test, &(B1->PmQ)))) {
        ibz_neg(&i3, &i3);
        ibz_neg(&i4, &i4);

#if _DEBUG
        ibz_sub(&i5, &i1, &i3);
        ibz_mod(&i5, &i5, &TORSION_PLUS_3POWER);
        ibz_sub(&i6, &i2, &i4);
        ibz_mod(&i6, &i6, &TORSION_PLUS_3POWER);

        ibz_to_digits(x5, &i5);
        ibz_to_digits(x6, &i6);

        ec_biscalar_mul_3(&test, E, x5, x6, B2);
        assert(ec_is_equal(&test, &(B1->PmQ)));
#endif
    }

    ibz_copy(&((*mat)[0][0]), &i1);
    ibz_copy(&((*mat)[1][0]), &i2);
    ibz_copy(&((*mat)[0][1]), &i3);
    ibz_copy(&((*mat)[1][1]), &i4);

    ibz_finalize(&i1);
    ibz_finalize(&i2);
    ibz_finalize(&i3);
    ibz_finalize(&i4);
    ibz_finalize(&i5);
    ibz_finalize(&i6);

    return;
}
