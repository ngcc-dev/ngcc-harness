#include <quaternion.h>
#include <ec.h>
#include <endomorphism_action.h>
#include <id2iso.h>
#include <inttypes.h>
#include <locale.h>
#include <bench.h>
#include <curve_extras.h>
#include <biextension.h>

static __inline__ uint64_t
rdtsc(void)
{
    return (uint64_t)cpucycles();
}

void
id2iso_long_two_isog_init(id2iso_long_two_isog_t *isog, const size_t length)
{
    isog->length = length;
    isog->chain = malloc(length * sizeof(*isog->chain));
}

void
id2iso_long_two_isog_finalize(id2iso_long_two_isog_t *isog)
{
    free(isog->chain);
}

void
id2iso_compressed_long_two_isog_init(id2iso_compressed_long_two_isog_t *zip, const size_t length)
{
    zip->length = length;
    zip->zip_chain = malloc(length * sizeof(*zip->zip_chain));
    for (size_t i = 0; i < length; ++i)
        ibz_init(&zip->zip_chain[i]);
}

void
id2iso_compressed_long_two_isog_finalize(id2iso_compressed_long_two_isog_t *zip)
{
    for (size_t i = 0; i < zip->length; ++i)
        ibz_finalize(&zip->zip_chain[i]);
    free(zip->zip_chain);
}

// untested
void
matrix_of_endomorphism_even(ibz_mat_2x2_t *mat, const quat_alg_elem_t *alpha)
{
    ibz_t tmp;
    ibz_init(&tmp);

    // construct the matrix of alpha on the 2^f-torsion
    ibz_vec_4_t coeffs;
    ibz_vec_4_init(&coeffs);
    from_1ijk_to_O0basis(&coeffs, alpha);

    ibz_mat_2x2_set(mat, 0, 0, 0, 0);

    for (unsigned i = 0; i < 2; ++i) {
        ibz_add(&((*mat)[i][i]), &((*mat)[i][i]), &coeffs[0]);
        ibz_mod(&((*mat)[i][i]), &((*mat)[i][i]), &TORSION_PLUS_2POWER);

        for (unsigned j = 0; j < 2; ++j) {
            ibz_mul(&tmp, &ACTION_GEN2[i][j], &coeffs[1]);
            ibz_add(&((*mat)[i][j]), &((*mat)[i][j]), &tmp);
            ibz_mod(&((*mat)[i][j]), &((*mat)[i][j]), &TORSION_PLUS_2POWER);

            ibz_mul(&tmp, &ACTION_GEN3[i][j], &coeffs[2]);
            ibz_add(&((*mat)[i][j]), &((*mat)[i][j]), &tmp);
            ibz_mod(&((*mat)[i][j]), &((*mat)[i][j]), &TORSION_PLUS_2POWER);

            ibz_mul(&tmp, &ACTION_GEN4[i][j], &coeffs[3]);
            ibz_add(&((*mat)[i][j]), &((*mat)[i][j]), &tmp);
            ibz_mod(&((*mat)[i][j]), &((*mat)[i][j]), &TORSION_PLUS_2POWER);
        }
    }

#ifndef NDEBUG
    // compate determinant and norm
    ibz_t det, norm;
    ibq_t norm_q;
    ibz_init(&det);
    ibz_init(&norm);
    ibq_init(&norm_q);

    ibz_mul(&det, &((*mat)[0][0]), &((*mat)[1][1]));
    ibz_mul(&tmp, &((*mat)[0][1]), &((*mat)[1][0]));
    ibz_sub(&det, &det, &tmp);
    ibz_mod(&det, &det, &TORSION_PLUS_2POWER);

    quat_alg_norm(&norm_q, alpha, &QUATALG_PINFTY);
    ibq_to_ibz(&norm, &norm_q);
    ibz_mod(&norm, &norm, &TORSION_PLUS_2POWER);

    assert(ibz_cmp(&det, &norm) == 0);
    ibz_finalize(&det);
    ibz_finalize(&norm);
    ibq_finalize(&norm_q);
#endif

    ibz_vec_4_finalize(&coeffs);

    ibz_finalize(&tmp);
}



void
matrix_of_endomorphism_odd(ibz_mat_2x2_t *mat, const quat_alg_elem_t *alpha)
{
    ibz_t tmp;
    ibz_init(&tmp);

    // construct the matrix of alpha on the 2^f-torsion
    ibz_vec_4_t coeffs;
    ibz_vec_4_init(&coeffs);
    from_1ijk_to_O0basis(&coeffs, alpha);

    ibz_mat_2x2_set(mat, 0, 0, 0, 0);

    for (unsigned i = 0; i < 2; ++i) {
        ibz_add(&((*mat)[i][i]), &((*mat)[i][i]), &coeffs[0]);
        ibz_mod(&((*mat)[i][i]), &((*mat)[i][i]), &TORSION_PLUS_3POWER);

        for (unsigned j = 0; j < 2; ++j) {
            ibz_mul(&tmp, &ACTION_GEN2[i][j], &coeffs[1]);
            ibz_add(&((*mat)[i][j]), &((*mat)[i][j]), &tmp);
            ibz_mod(&((*mat)[i][j]), &((*mat)[i][j]), &TORSION_PLUS_3POWER);

            ibz_mul(&tmp, &ACTION_GEN3[i][j], &coeffs[2]);
            ibz_add(&((*mat)[i][j]), &((*mat)[i][j]), &tmp);
            ibz_mod(&((*mat)[i][j]), &((*mat)[i][j]), &TORSION_PLUS_3POWER);

            ibz_mul(&tmp, &ACTION_GEN4[i][j], &coeffs[3]);
            ibz_add(&((*mat)[i][j]), &((*mat)[i][j]), &tmp);
            ibz_mod(&((*mat)[i][j]), &((*mat)[i][j]), &TORSION_PLUS_3POWER);
        }
    }

#ifndef NDEBUG
    // compate determinant and norm
    ibz_t det, norm;
    ibq_t norm_q;
    ibz_init(&det);
    ibz_init(&norm);
    ibq_init(&norm_q);

    ibz_mul(&det, &((*mat)[0][0]), &((*mat)[1][1]));
    ibz_mul(&tmp, &((*mat)[0][1]), &((*mat)[1][0]));
    ibz_sub(&det, &det, &tmp);
    ibz_mod(&det, &det, &TORSION_PLUS_3POWER);

    quat_alg_norm(&norm_q, alpha, &QUATALG_PINFTY);
    ibq_to_ibz(&norm, &norm_q);
    ibz_mod(&norm, &norm, &TORSION_PLUS_3POWER);

    assert(ibz_cmp(&det, &norm) == 0);
    ibz_finalize(&det);
    ibz_finalize(&norm);
    ibq_finalize(&norm_q);
#endif

    ibz_vec_4_finalize(&coeffs);

    ibz_finalize(&tmp);
}


void
id2iso_ideal_to_kernel_dlogs_even(ibz_vec_2_t *vec, const quat_left_ideal_t *lideal)
{
    ibz_t tmp;
    ibz_init(&tmp);

    ibz_mat_2x2_t mat;
    ibz_mat_2x2_init(&mat);

    // construct the matrix of the dual of alpha on the 2^f-torsion
    {
        quat_alg_elem_t alpha;
        quat_alg_elem_init(&alpha);

        int lideal_generator_ok;
        lideal_generator_ok = quat_lideal_generator(&alpha, lideal, &QUATALG_PINFTY, 0);
        assert(lideal_generator_ok);
        quat_alg_conj(&alpha, &alpha);

        ibz_vec_4_t coeffs;
        ibz_vec_4_init(&coeffs);
        from_1ijk_to_O0basis(&coeffs, &alpha);

        for (unsigned i = 0; i < 2; ++i) {
            ibz_add(&mat[i][i], &mat[i][i], &coeffs[0]);
            for (unsigned j = 0; j < 2; ++j) {
                ibz_mul(&tmp, &ACTION_GEN2[i][j], &coeffs[1]);
                ibz_add(&mat[i][j], &mat[i][j], &tmp);
                ibz_mul(&tmp, &ACTION_GEN3[i][j], &coeffs[2]);
                ibz_add(&mat[i][j], &mat[i][j], &tmp);
                ibz_mul(&tmp, &ACTION_GEN4[i][j], &coeffs[3]);
                ibz_add(&mat[i][j], &mat[i][j], &tmp);
            }
        }

        ibz_vec_4_finalize(&coeffs);
        quat_alg_elem_finalize(&alpha);
    }

    // find the kernel of alpha modulo the norm of the ideal
    {
        ibz_t const *const norm = &lideal->norm;

        ibz_mod(&(*vec)[0], &mat[0][0], norm);
        ibz_mod(&(*vec)[1], &mat[1][0], norm);
        ibz_gcd(&tmp, &(*vec)[0], &(*vec)[1]);
        if (!(ibz_get(&tmp) & 1)) {
            ibz_mod(&(*vec)[0], &mat[0][1], norm);
            ibz_mod(&(*vec)[1], &mat[1][1], norm);
        }
#ifndef NDEBUG
        ibz_gcd(&tmp, &(*vec)[0], norm);
        ibz_gcd(&tmp, &(*vec)[1], &tmp);
        assert(!ibz_cmp(&tmp, &ibz_const_one));
#endif
    }

    ibz_mat_2x2_finalize(&mat);
    ibz_finalize(&tmp);
}

void
id2iso_ideal_to_isogeny_even(ec_isog_even_t *isog, const quat_left_ideal_t *lideal_input)
{
    // compute length
    isog->length = 0;
    ibz_t norm;
    ibz_init(&norm);
    ibz_copy(&norm, &lideal_input->norm);
    while (!ibz_is_one(&norm)) {
        assert(!ibz_is_zero(&norm) && !(ibz_get(&norm) & 1));
        ibz_div_2exp(&norm, &norm, 1);
        ++isog->length;
    }
    ibz_finalize(&norm);
    assert(isog->length <= TORSION_PLUS_EVEN_POWER);

    digit_t scalars[2][NWORDS_FIELD];
    {
        ibz_vec_2_t vec;
        ibz_vec_2_init(&vec);

        id2iso_ideal_to_kernel_dlogs_even(&vec, lideal_input);

        // multiply out unnecessary cofactor from 2^f-torsion basis
        for (size_t i = isog->length; i < TORSION_PLUS_EVEN_POWER; ++i) {
            ibz_add(&vec[0], &vec[0], &vec[0]);
            ibz_add(&vec[1], &vec[1], &vec[1]);
        }

        ibz_to_digit_array(scalars[0], &vec[0]);
        ibz_to_digit_array(scalars[1], &vec[1]);

        ibz_vec_2_finalize(&vec);
    }

    isog->curve = CURVE_E0;
    ec_biscalar_mul(&isog->kernel, &isog->curve, scalars[0], scalars[1], &BASIS_EVEN);
}

void
ec_mul_ibz(ec_point_t *res, const ec_curve_t *curve, const ibz_t *scalarP, const ec_point_t *P)
{

    digit_t scalars[NWORDS_FIELD];
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

    digit_t scalars[2][NWORDS_FIELD];

    if (ibz_cmp(scalarP, &ibz_const_zero) == 0) {
        ec_mul_ibz(res, curve, scalarQ, &PQ->Q);
    } else if (ibz_cmp(scalarQ, &ibz_const_zero) == 0) {
        ec_mul_ibz(res, curve, scalarP, &PQ->P);
    } else {
        ibz_to_digit_array(scalars[0], scalarP);
        ibz_to_digit_array(scalars[1], scalarQ);
        //ec_biscalar_mul_bounded(res, curve, scalars[0], scalars[1], PQ, f);
        ec_biscalar_mul(res, curve, scalars[0], scalars[1], PQ);

    }
}

void
id2iso_ideal_to_isogeny_even_dlogs(ec_isog_even_t *isog,
                                   ibz_vec_2_t *ker_dlog,
                                   const quat_left_ideal_t *lideal_input)
{
    // compute length
    isog->length = 0;
    ibz_t norm;
    ibz_init(&norm);
    ibz_copy(&norm, &lideal_input->norm);
    while (!ibz_is_one(&norm)) {
        assert(!ibz_is_zero(&norm) && !(ibz_get(&norm) & 1));
        ibz_div_2exp(&norm, &norm, 1);
        ++isog->length;
    }
    ibz_finalize(&norm);
    assert(isog->length <= TORSION_PLUS_EVEN_POWER);

    digit_t scalars[2][NWORDS_FIELD];
    {
        ibz_vec_2_t vec;
        ibz_vec_2_init(&vec);

        id2iso_ideal_to_kernel_dlogs_even(&vec, lideal_input);

        // multiply out unnecessary cofactor from 2^f-torsion basis
        for (size_t i = isog->length; i < TORSION_PLUS_EVEN_POWER; ++i) {
            ibz_add(&vec[0], &vec[0], &vec[0]);
            ibz_add(&vec[1], &vec[1], &vec[1]);
        }

        ibz_copy(&(*ker_dlog)[0], &vec[0]);
        ibz_copy(&(*ker_dlog)[1], &vec[1]);

        ibz_to_digit_array(scalars[0], &vec[0]);
        ibz_to_digit_array(scalars[1], &vec[1]);

        ibz_vec_2_finalize(&vec);
    }

    isog->curve = CURVE_E0;
    ec_biscalar_mul(&isog->kernel, &isog->curve, scalars[0], scalars[1], &BASIS_EVEN);
}

// helper function to apply a matrix to a basis of E[2^f]
// works in place
void
matrix_application_even_basis(ec_basis_t *bas, const ec_curve_t *E, ibz_mat_2x2_t *mat, int f)
{
    digit_t scalars[2][NWORDS_FIELD] = { 0 };

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

// helper function to apply some endomorphism of E0 on the precomputed basis of E0[2^f]
// works in place
void
endomorphism_application_even_jac_basis(jac_point_t *P,
                                        jac_point_t *Q,
                                        quat_alg_elem_t *theta,
                                        int f)
{
    ibz_t tmp;
    ibz_init(&tmp);
    ibz_vec_4_t coeffs;
    ibz_vec_4_init(&coeffs);
    ibz_mat_2x2_t mat;
    ibz_mat_2x2_init(&mat);
    ibz_t twopow;
    ibz_init(&twopow);
    ibz_pow(&twopow, &ibz_const_two, f);

    ibz_t content;
    ibz_init(&content);

    digit_t scalars[2][NWORDS_ORDER] = { 0 };

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
            ibz_mod(&mat[i][j], &mat[i][j], &twopow);
        }
    }

    // and now we apply it
    jac_point_t tmp1, tmp2, tmp3;
    copy_jac_point(&tmp1, P);
    copy_jac_point(&tmp2, Q);

    // first basis element
    ibz_to_digit_array(scalars[0], &mat[0][0]);
    ibz_to_digit_array(scalars[1], &mat[1][0]);
    DBLMUL_generic(P, &tmp1, scalars[0], &tmp2, scalars[1], &CURVE_E0, NWORDS_ORDER);
    // ec_biscalar_mul(&bas->P,&CURVE_E0,scalars[0],scalars[1],&tmp_bas);

    ibz_to_digit_array(scalars[0], &mat[0][1]);
    ibz_to_digit_array(scalars[1], &mat[1][1]);
    DBLMUL_generic(Q, &tmp1, scalars[0], &tmp2, scalars[1], &CURVE_E0, NWORDS_ORDER);
    // ec_biscalar_mul(&bas->Q,&CURVE_E0,scalars[0],scalars[1],&tmp_bas);

    // ibz_sub(&tmp,&mat[0][0],&mat[0][1]);
    // ibz_mod(&tmp,&tmp,&twopow);
    // ibz_to_digit_array(scalars[0],&tmp);
    // ibz_sub(&tmp,&mat[1][0],&mat[1][1]);
    // ibz_mod(&tmp,&tmp,&twopow);
    // ibz_to_digit_array(scalars[1],&tmp);
    // ec_biscalar_mul(&bas->PmQ,&CURVE_E0,scalars[0],scalars[1],&tmp_bas);

    ibz_finalize(&tmp);
    ibz_vec_4_finalize(&coeffs);
    ibz_mat_2x2_finalize(&mat);
    ibz_finalize(&twopow);
    ibz_finalize(&content);
}

void
id2iso_ideal_to_kernel_dlogs_odd(ibz_vec_2_t *vec,
                                 ec_degree_odd_t *deg,
                                 const quat_left_ideal_t *lideal)
{
    ibz_t tmp;
    ibz_init(&tmp);

    ibz_mat_2x2_t mat;
    ibz_mat_2x2_init(&mat);

    // construct the matrix of the dual of alpha on the T-torsion
    {
        quat_alg_elem_t alpha;
        quat_alg_elem_init(&alpha);

        int lideal_generator_ok;
        lideal_generator_ok = quat_lideal_generator(&alpha, lideal, &QUATALG_PINFTY, 0);
        assert(lideal_generator_ok);
        assert(
            ibz_divides(&ibz_const_two, &alpha.denom)); // denominator is invertible mod T, ignore

        for (unsigned i = 0; i < 2; ++i) {
            ibz_add(&mat[i][i], &mat[i][i], &alpha.coord[0]);
            for (unsigned j = 0; j < 2; ++j) {
                ibz_mul(&tmp, &ACTION_I[i][j], &alpha.coord[1]);
                ibz_sub(&mat[i][j], &mat[i][j], &tmp);
                ibz_mul(&tmp, &ACTION_J[i][j], &alpha.coord[2]);
                ibz_sub(&mat[i][j], &mat[i][j], &tmp);
                ibz_mul(&tmp, &ACTION_K[i][j], &alpha.coord[3]);
                ibz_sub(&mat[i][j], &mat[i][j], &tmp);
                //                ibz_mod(&mat[i][j], &mat[i][j], &TORSION_ODD);
            }
        }

        quat_alg_elem_finalize(&alpha);
    }

    // determine prime powers in the norm of the ideal
    size_t numpp = 0;
#define NUMPP (sizeof(TORSION_ODD_PRIMEPOWERS) / sizeof(*TORSION_ODD_PRIMEPOWERS))
    ibz_t pps[NUMPP];
    ibz_vec_2_t vs[NUMPP];
    {
        ibz_t const *const norm = &lideal->norm;

        for (size_t i = 0; i < NUMPP; ++i) {
            ibz_gcd(&tmp, norm, &TORSION_ODD_PRIMEPOWERS[i]);
            (*deg)[i] = 0;
            if (!ibz_is_one(&tmp)) {
                ibz_init(&pps[numpp]);
                ibz_copy(&pps[numpp], &tmp);
                ibz_vec_2_init(&vs[numpp]);
                ++numpp;

                // compute valuation
                ibz_t l, r;
                ibz_init(&l);
                ibz_init(&r);
                ibz_set(&l, TORSION_ODD_PRIMES[i]);
                do {
                    ++(*deg)[i];
                    ibz_div(&tmp, &r, &tmp, &l);
                    assert(!ibz_is_zero(&tmp) && ibz_is_zero(&r));
                } while (!ibz_is_one(&tmp));
                ibz_finalize(&r);
                ibz_finalize(&l);
            }
        }
    }
#undef NUMPP

    // find the kernel of alpha modulo each prime power
    {
        for (size_t i = 0; i < numpp; ++i) {
            ibz_mod(&vs[i][0], &mat[0][0], &pps[i]);
            ibz_mod(&vs[i][1], &mat[1][0], &pps[i]);
            ibz_gcd(&tmp, &vs[i][0], &pps[i]);
            ibz_gcd(&tmp, &vs[i][1], &tmp);
            if (ibz_cmp(&tmp, &ibz_const_one)) {
                ibz_mod(&vs[i][0], &mat[0][1], &pps[i]);
                ibz_mod(&vs[i][1], &mat[1][1], &pps[i]);
            }
#ifndef NDEBUG
            ibz_gcd(&tmp, &vs[i][0], &pps[i]);
            ibz_gcd(&tmp, &vs[i][1], &tmp);
            assert(!ibz_cmp(&tmp, &ibz_const_one));
#endif
        }
    }

    // now CRT them together
    {
        // TODO use a product tree instead
        ibz_t mod;
        ibz_init(&mod);
        ibz_set(&mod, 1);
        ibz_set(&(*vec)[0], 0);
        ibz_set(&(*vec)[1], 0);
        for (size_t i = 0; i < numpp; ++i) {
            // TODO use vector CRT
            ibz_crt(&(*vec)[0], &(*vec)[0], &vs[i][0], &mod, &pps[i]);
            ibz_crt(&(*vec)[1], &(*vec)[1], &vs[i][1], &mod, &pps[i]);
            // TODO optionally return lcm from CRT and use it
            ibz_mul(&mod, &mod, &pps[i]);
            ibz_finalize(&pps[i]);
        }
        ibz_finalize(&mod);
    }

    for (size_t i = 0; i < numpp; ++i)
        ibz_vec_2_finalize(&vs[i]);

    ibz_mat_2x2_finalize(&mat);

    ibz_finalize(&tmp);
}

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

void
id2iso_ideal_to_isogeny_odd(ec_isog_odd_t *isog,
                            const ec_curve_t *domain,
                            const ec_basis_t *basis_plus,
                            const ec_basis_t *basis_minus,
                            const quat_left_ideal_t *lideal_input)
{
    digit_t scalars_plus[2][NWORDS_ORDER] = { 0 }, scalars_minus[2][NWORDS_ORDER] = { 0 };
    {
        ibz_vec_2_t vec;
        ibz_vec_2_init(&vec);

        id2iso_ideal_to_kernel_dlogs_odd(&vec, &isog->degree, lideal_input);

        ibz_t tmp;
        ibz_init(&tmp);

        // multiply out unnecessary cofactor from T-torsion basis
        assert(sizeof(isog->degree) / sizeof(*isog->degree) ==
               sizeof(TORSION_ODD_PRIMEPOWERS) / sizeof(*TORSION_ODD_PRIMEPOWERS));
        for (size_t i = 0; i < sizeof(isog->degree) / sizeof(*isog->degree); ++i) {
            assert(isog->degree[i] <= TORSION_ODD_POWERS[i]);
            if (isog->degree[i] == TORSION_ODD_POWERS[i])
                continue;
            ibz_set(&tmp, TORSION_ODD_PRIMES[i]);
            ibz_pow(&tmp, &tmp, TORSION_ODD_POWERS[i] - isog->degree[i]);
            ibz_mul(&vec[0], &vec[0], &tmp);
            ibz_mul(&vec[1], &vec[1], &tmp);
        }

        ibz_mod(&tmp, &vec[0], &TORSION_ODD_PLUS);
        ibz_to_digit_array(scalars_plus[0], &tmp);
        ibz_mod(&tmp, &vec[1], &TORSION_ODD_PLUS);
        ibz_to_digit_array(scalars_plus[1], &tmp);
        ibz_mod(&tmp, &vec[0], &TORSION_ODD_MINUS);
        // ibz_printf("> %Zx\n", &tmp);
        ibz_to_digit_array(scalars_minus[0], &tmp);
        ibz_mod(&tmp, &vec[1], &TORSION_ODD_MINUS);
        // ibz_printf("> %Zx\n", &tmp);
        ibz_to_digit_array(scalars_minus[1], &tmp);

        ibz_finalize(&tmp);

        ibz_vec_2_finalize(&vec);
    }

    isog->curve = *domain;
    ec_biscalar_mul(&isog->ker_plus, domain, scalars_plus[0], scalars_plus[1], basis_plus);

    ec_biscalar_mul(&isog->ker_minus, domain, scalars_minus[0], scalars_minus[1], basis_minus);
    // TODO: the case scalars_minus = (0,0) seems to bug
}

void
id2iso_ideal_to_isogeny_odd_plus(ec_isog_odd_t *isog,
                                 ibz_vec_2_t *ker_dlog,
                                 const ec_curve_t *domain,
                                 const ec_basis_t *basis_plus,
                                 const quat_left_ideal_t *lideal_input)
{
    digit_t scalars_plus[2][NWORDS_ORDER] = { 0 };
    {
        ibz_vec_2_t vec;
        ibz_vec_2_init(&vec);

        id2iso_ideal_to_kernel_dlogs_odd(&vec, &isog->degree, lideal_input);

        ibz_t tmp;
        ibz_init(&tmp);

        // multiply out unnecessary cofactor from T-torsion basis
        assert(sizeof(isog->degree) / sizeof(*isog->degree) ==
               sizeof(TORSION_ODD_PRIMEPOWERS) / sizeof(*TORSION_ODD_PRIMEPOWERS));
        for (size_t i = 0; i < sizeof(isog->degree) / sizeof(*isog->degree); ++i) {
            assert(isog->degree[i] <= TORSION_ODD_POWERS[i]);
            if (isog->degree[i] == TORSION_ODD_POWERS[i])
                continue;
            ibz_set(&tmp, TORSION_ODD_PRIMES[i]);
            ibz_pow(&tmp, &tmp, TORSION_ODD_POWERS[i] - isog->degree[i]);
            ibz_mul(&vec[0], &vec[0], &tmp);
            ibz_mul(&vec[1], &vec[1], &tmp);
        }

        ibz_mod(&tmp, &vec[0], &TORSION_ODD_PLUS);
        ibz_copy(&((*ker_dlog)[0]), &tmp);
        ibz_to_digit_array(scalars_plus[0], &tmp);
        ibz_mod(&tmp, &vec[1], &TORSION_ODD_PLUS);
        ibz_copy(&((*ker_dlog)[1]), &tmp);
        ibz_to_digit_array(scalars_plus[1], &tmp);

#ifndef NDEBUG
        ibz_mod(&tmp, &vec[0], &TORSION_ODD_MINUS);
        assert(ibz_is_zero(&tmp));
        ibz_mod(&tmp, &vec[1], &TORSION_ODD_MINUS);
        assert(ibz_is_zero(&tmp));
#endif

        ibz_finalize(&tmp);

        ibz_vec_2_finalize(&vec);
    }

    isog->curve = *domain;
    ec_biscalar_mul(&isog->ker_plus, domain, scalars_plus[0], scalars_plus[1], basis_plus);
    ec_set_zero(&(isog->ker_minus));
}


void
id2iso_ideal_to_isogeny_odd_minus(ec_isog_odd_t *isog,
                                 ibz_vec_2_t *ker_dlog,
                                 const ec_curve_t *domain,
                                 const ec_basis_t *basis_minus,
                                 const quat_left_ideal_t *lideal_input)
{
    digit_t scalars_minus[2][NWORDS_ORDER] = { 0 };
    {
        ibz_vec_2_t vec;
        ibz_vec_2_init(&vec);
 
        id2iso_ideal_to_kernel_dlogs_odd(&vec, &isog->degree, lideal_input);

        ibz_t tmp;
        ibz_init(&tmp);

        // multiply out unnecessary cofactor from T-torsion basis
        assert(sizeof(isog->degree) / sizeof(*isog->degree) ==
               sizeof(TORSION_ODD_PRIMEPOWERS) / sizeof(*TORSION_ODD_PRIMEPOWERS));
        for (size_t i = 0; i < sizeof(isog->degree) / sizeof(*isog->degree); ++i) {
            assert(isog->degree[i] <= TORSION_ODD_POWERS[i]);
            if (isog->degree[i] == TORSION_ODD_POWERS[i])
                continue;
            ibz_set(&tmp, TORSION_ODD_PRIMES[i]);
            ibz_pow(&tmp, &tmp, TORSION_ODD_POWERS[i] - isog->degree[i]);
            ibz_mul(&vec[0], &vec[0], &tmp);
            ibz_mul(&vec[1], &vec[1], &tmp);
        }

        ibz_mod(&tmp, &vec[0], &TORSION_ODD_MINUS);
        ibz_copy(&((*ker_dlog)[0]), &tmp);
        ibz_to_digit_array(scalars_minus[0], &tmp);
        ibz_mod(&tmp, &vec[1], &TORSION_ODD_MINUS);
        ibz_copy(&((*ker_dlog)[1]), &tmp);
        ibz_to_digit_array(scalars_minus[1], &tmp);

#ifndef NDEBUG
        ibz_mod(&tmp, &vec[0], &TORSION_ODD_PLUS);
        assert(ibz_is_zero(&tmp));
        ibz_mod(&tmp, &vec[1], &TORSION_ODD_PLUS);
        assert(ibz_is_zero(&tmp));
#endif

        ibz_finalize(&tmp);

        ibz_vec_2_finalize(&vec);
    }

    isog->curve = *domain;
    ec_biscalar_mul(&isog->ker_minus, domain, scalars_minus[0], scalars_minus[1], basis_minus);
    ec_set_zero(&(isog->ker_plus));
}


void
id2iso_kernel_dlogs_to_ideal(quat_left_ideal_t *lideal,
                             const ibz_vec_2_t *vec2,
                             const ibz_vec_2_t *vec3)
{
    ibz_vec_2_t ker;
    ibz_vec_2_init(&ker);
    ibz_crt(&ker[0], &(*vec2)[0], &(*vec3)[0], &TORSION_PLUS_2POWER, &TORSION_PLUS_3POWER);
    ibz_crt(&ker[1], &(*vec2)[1], &(*vec3)[1], &TORSION_PLUS_2POWER, &TORSION_PLUS_3POWER);

    // algorithm: apply endomorphisms 1 and j+(1+k)/2 to the kernel point,
    // the result should form a basis of the respective torsion subgroup.
    // then apply i to the kernel point and decompose over said basis.
    // hence we have an equation a*P + b*[j+(1+k)/2]P == [i]P, which will
    // easily reveal an endomorphism that kills P.

    ibz_vec_2_t vec;
    ibz_vec_2_init(&vec);

    {
        ibz_mat_2x2_t mat;
        ibz_mat_2x2_init(&mat);

        ibz_copy(&mat[0][0], &ker[0]);
        ibz_copy(&mat[1][0], &ker[1]);

        ibz_mat_2x2_eval(&vec, &ACTION_J, &ker);
        ibz_copy(&mat[0][1], &vec[0]);
        ibz_copy(&mat[1][1], &vec[1]);
        ibz_mat_2x2_eval(&vec, &ACTION_GEN4, &ker);
        ibz_add(&mat[0][1], &mat[0][1], &vec[0]);
        ibz_add(&mat[1][1], &mat[1][1], &vec[1]);
        ibz_mod(&mat[0][1], &mat[0][1], &TORSION_PLUS_23POWER);
        ibz_mod(&mat[1][1], &mat[1][1], &TORSION_PLUS_23POWER);

        ibz_mat_2x2_t inv;
        ibz_mat_2x2_init(&inv);
        {
            int inv_ok = ibz_2x2_inv_mod(&inv, &mat, &TORSION_PLUS_23POWER);
            assert(inv_ok);
        }
        ibz_mat_2x2_finalize(&mat);

        ibz_mat_2x2_eval(&vec, &ACTION_I, &ker);
        ibz_mat_2x2_eval(&vec, &inv, &vec);

        ibz_mat_2x2_finalize(&inv);
    }

    ibz_vec_2_finalize(&ker);

    // final result: a - i + b*(j+(1+k)/2)
    quat_alg_elem_t gen;
    quat_alg_elem_init(&gen);
    ibz_set(&gen.denom, 2);
    ibz_add(&gen.coord[0], &vec[0], &vec[0]);
    ibz_set(&gen.coord[1], -2);
    ibz_add(&gen.coord[2], &vec[1], &vec[1]);
    ibz_copy(&gen.coord[3], &vec[1]);
    ibz_add(&gen.coord[0], &gen.coord[0], &vec[1]);
    ibz_vec_2_finalize(&vec);

    quat_lideal_create_from_primitive(
        lideal, &gen, &TORSION_PLUS_23POWER, &MAXORD_O0, &QUATALG_PINFTY);

    assert(0 == ibz_cmp(&lideal->norm, &TORSION_PLUS_23POWER));

    quat_alg_elem_finalize(&gen);
}

// compute the ideal whose kernel is generated by vec2[0]*BO[0] + vec2[1]*B0[1] where B0 is the
// canonical basis of E0
void
id2iso_kernel_dlogs_to_ideal_two(quat_left_ideal_t *lideal, const ibz_vec_2_t *vec2, int f)
{

    // algorithm: apply endomorphisms 1 and j+(1+k)/2 to the kernel point,
    // the result should form a basis of the respective torsion subgroup.
    // then apply i to the kernel point and decompose over said basis.
    // hence we have an equation a*P + b*[j+(1+k)/2]P == [i]P, which will
    // easily reveal an endomorphism that kills P.

    ibz_t two_pow;
    ibz_vec_2_t vec;
    ibz_vec_2_init(&vec);

    ibz_init(&two_pow);

    if (f == TORSION_PLUS_EVEN_POWER) {
        ibz_copy(&two_pow, &TORSION_PLUS_2POWER);
    } else {
        ibz_pow(&two_pow, &ibz_const_two, f);
    }

    {
        ibz_mat_2x2_t mat;
        ibz_mat_2x2_init(&mat);

        ibz_copy(&mat[0][0], &(*vec2)[0]);
        ibz_copy(&mat[1][0], &(*vec2)[1]);

        ibz_mat_2x2_eval(&vec, &ACTION_J, vec2);
        ibz_copy(&mat[0][1], &vec[0]);
        ibz_copy(&mat[1][1], &vec[1]);
        ibz_mat_2x2_eval(&vec, &ACTION_GEN4, vec2);
        ibz_add(&mat[0][1], &mat[0][1], &vec[0]);
        ibz_add(&mat[1][1], &mat[1][1], &vec[1]);
        ibz_mod(&mat[0][1], &mat[0][1], &two_pow);
        ibz_mod(&mat[1][1], &mat[1][1], &two_pow);

        ibz_mat_2x2_t inv;
        ibz_mat_2x2_init(&inv);
        {
            int inv_ok = ibz_2x2_inv_mod(&inv, &mat, &two_pow);
            assert(inv_ok);
        }
        ibz_mat_2x2_finalize(&mat);

        ibz_mat_2x2_eval(&vec, &ACTION_I, vec2);
        ibz_mat_2x2_eval(&vec, &inv, &vec);

        ibz_mat_2x2_finalize(&inv);
    }

    // final result: a - i + b*(j+(1+k)/2)
    quat_alg_elem_t gen;
    quat_alg_elem_init(&gen);
    ibz_set(&gen.denom, 2);
    ibz_add(&gen.coord[0], &vec[0], &vec[0]);
    ibz_set(&gen.coord[1], -2);
    ibz_add(&gen.coord[2], &vec[1], &vec[1]);
    ibz_copy(&gen.coord[3], &vec[1]);
    ibz_add(&gen.coord[0], &gen.coord[0], &vec[1]);
    ibz_vec_2_finalize(&vec);

    quat_lideal_create_from_primitive(lideal, &gen, &two_pow, &MAXORD_O0, &QUATALG_PINFTY);

    assert(0 == ibz_cmp(&lideal->norm, &two_pow));

    quat_alg_elem_finalize(&gen);
    ibz_init(&two_pow);
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

#ifndef NDEBUG
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
    digit_t x1[NWORDS_ORDER] = { 0 }, x2[NWORDS_ORDER] = { 0 }, x3[NWORDS_ORDER] = { 0 },
            x4[NWORDS_ORDER] = { 0 };
    digit_t x5[NWORDS_ORDER] = { 0 }, x6[NWORDS_ORDER] = { 0 };
    ibz_t i1, i2, i3, i4, i5, i6;
    ibz_init(&i1);
    ibz_init(&i2);
    ibz_init(&i3);
    ibz_init(&i4);
    ibz_init(&i5);
    ibz_init(&i6);

    assert(test_point_order_twof(&(B2->PmQ), E, f));

    ec_dlog_2_weil(x1, x2, x3, x4, B2, B1, E, f);

    // copying the digits
    ibz_copy_digit_array(&i1, x1);
    ibz_copy_digit_array(&i2, x2);
    ibz_copy_digit_array(&i3, x3);
    ibz_copy_digit_array(&i4, x4);

    ec_point_t test;
#ifndef NDEBUG
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

        ibz_sub(&i5, &i1, &i3);
        ibz_mod(&i5, &i5, &TORSION_PLUS_2POWER);
        ibz_sub(&i6, &i2, &i4);
        ibz_mod(&i6, &i6, &TORSION_PLUS_2POWER);

        ibz_to_digits(x5, &i5);
        ibz_to_digits(x6, &i6);

#ifndef NDEBUG
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
    digit_t x1[NWORDS_ORDER] = { 0 }, x2[NWORDS_ORDER] = { 0 }, x3[NWORDS_ORDER] = { 0 },
            x4[NWORDS_ORDER] = { 0 };
    digit_t x5[NWORDS_ORDER] = { 0 }, x6[NWORDS_ORDER] = { 0 };
    ibz_t i1, i2, i3, i4, i5, i6;
    ibz_init(&i1);
    ibz_init(&i2);
    ibz_init(&i3);
    ibz_init(&i4);
    ibz_init(&i5);
    ibz_init(&i6);

    assert(test_point_order_threef(&(B1->P), E, TORSION_PLUS_ODD_POWERS[0]));
    assert(test_point_order_threef(&(B1->Q), E, TORSION_PLUS_ODD_POWERS[0]));
    assert(test_point_order_threef(&(B1->PmQ), E, TORSION_PLUS_ODD_POWERS[0]));
    assert(test_point_order_threef(&(B2->P), E, TORSION_PLUS_ODD_POWERS[0]));
    assert(test_point_order_threef(&(B2->Q), E, TORSION_PLUS_ODD_POWERS[0]));
    assert(test_point_order_threef(&(B2->PmQ), E, TORSION_PLUS_ODD_POWERS[0]));

    ec_dlog_3(x1, x2, B2, &(B1->P), E);
    ec_dlog_3(x3, x4, B2, &(B1->Q), E);
    ibz_copy_digit_array(&i1, x1);
    ibz_copy_digit_array(&i2, x2);
    ibz_copy_digit_array(&i3, x3);
    ibz_copy_digit_array(&i4, x4);

    ec_point_t test;
#ifndef NDEBUG
    ec_biscalar_mul(&test, E, x1, x2, B2);
    assert(ec_is_equal(&test, &(B1->P)));
    ec_biscalar_mul(&test, E, x3, x4, B2);
    assert(ec_is_equal(&test, &(B1->Q)));
#endif

    ibz_sub(&i5, &i1, &i3);
    ibz_mod(&i5, &i5, &TORSION_PLUS_3POWER);
    ibz_sub(&i6, &i2, &i4);
    ibz_mod(&i6, &i6, &TORSION_PLUS_3POWER);

    ibz_to_digits(x5, &i5);
    ibz_to_digits(x6, &i6);

    ec_biscalar_mul(&test, E, x5, x6, B2);
    if (!(ec_is_equal(&test, &(B1->PmQ)))) {
        ibz_neg(&i3, &i3);
        ibz_neg(&i4, &i4);

        ibz_sub(&i5, &i1, &i3);
        ibz_mod(&i5, &i5, &TORSION_PLUS_3POWER);
        ibz_sub(&i6, &i2, &i4);
        ibz_mod(&i6, &i6, &TORSION_PLUS_3POWER);

        ibz_to_digits(x5, &i5);
        ibz_to_digits(x6, &i6);

#ifndef NDEBUG
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

// function to sample a random left O0-ideal of given norm
void
heuristic_sampling_random_ideal_O0(quat_left_ideal_t *lideal, ibz_t *norm)
{

    ibz_t n_temp;
    quat_alg_elem_t gen;
    int found = 1;
    ibz_init(&n_temp);
    quat_alg_elem_init(&gen);

    // TODO make a cleaner provable version of this
    generate_random_prime(&n_temp, 1, ibz_bitsize(&QUATALG_PINFTY.p));
    ibz_mul(&n_temp, norm, &n_temp);
    found = found && represent_integer(&gen, &n_temp, &QUATALG_PINFTY);
    assert(found);
    quat_lideal_create_from_primitive(lideal, &gen, norm, &MAXORD_O0, &QUATALG_PINFTY);

    ibz_finalize(&n_temp);
    quat_alg_elem_finalize(&gen);
}

// function to sample a random left O0-ideal of given norm
void
sampling_random_ideal_O0(quat_left_ideal_t *lideal, ibz_t *norm, int is_prime)
{

    ibz_t n_temp;
    ibz_t disc;
    quat_alg_elem_t gen, gen_rerand;
    int found = 0;
    ibz_init(&n_temp);
    ibz_init(&disc);
    quat_alg_elem_init(&gen);
    quat_alg_elem_init(&gen_rerand);
    ibq_t q_norm;
    ibq_init(&q_norm);
    int m = 10;

    ibz_set(&gen.coord[0], 0);
    ibz_set(&gen_rerand.coord[2], 0);
    ibz_set(&gen_rerand.coord[3], 0);

    // when the norm is prime we can be quite efficient
    // by avoiding to run represent integer

    if (is_prime) {
        // we find an quaternion element of norm divided by norm
        while (!found) {

            // generating 3 coefficients at random
            for (int i = 1; i < 4; i++) {
                ibz_rand_interval_minm_m(&gen.coord[i], m);
            }

            // first, we compute the norm of the gen
            quat_alg_norm(&q_norm, &gen, &QUATALG_PINFTY);
            ibq_to_ibz(&n_temp, &q_norm);

            // and finally the negation mod norm
            ibz_neg(&disc, &n_temp);
            ibz_mod(&disc, &disc, norm);
            // now we check that -n is a square mod norm
            // and if the squareroot exists we compute it
            found = ibz_sqrt_mod_p(&gen.coord[0], &disc, norm);
        }
    } else {
        // if it is not prime or we don't know if it is prime, we may just use represent integer
        if (ibz_bitsize(norm) > ibz_bitsize(&QUATALG_PINFTY.p) + 20 ) {
            ibz_set(&n_temp,1);
        }
        else {
            generate_random_prime(&n_temp, 1, ibz_bitsize(&QUATALG_PINFTY.p) + 40 - ibz_bitsize(norm));
        }
        ibz_mul(&n_temp, &n_temp, norm);
        found = represent_integer(&gen, &n_temp, &QUATALG_PINFTY);
    }

#ifndef NDEBUG
    assert(found);
    // first, we compute the norm of the gen
    quat_alg_norm(&q_norm, &gen, &QUATALG_PINFTY);
    ibq_to_ibz(&n_temp, &q_norm);
    ibz_mod(&n_temp, &n_temp, norm);
    assert(ibz_cmp(&n_temp, &ibz_const_zero) == 0);
#endif

    // now we just have to rerandomize the class of the ideal generated by gen by multiplying by
    // element of a+ib where (a:b) is uniformly random in P1(ZZ/normZZ) first sample a bit to
    // determine what coordinate is non-zero
    found = 0;
    while (!found) {
        for (int i = 0; i < 4; i++) {
            ibz_rand_interval(&gen_rerand.coord[i], &ibz_const_one, norm);
        }
        quat_alg_norm(&q_norm, &gen_rerand, &QUATALG_PINFTY);
        ibq_to_ibz(&n_temp, &q_norm);
        ibz_gcd(&disc, &n_temp, norm);
        found = (ibz_cmp(&disc, &ibz_const_one) == 0);
    }

    quat_alg_mul(&gen, &gen, &gen_rerand, &QUATALG_PINFTY);
    quat_lideal_create_from_primitive(lideal, &gen, norm, &MAXORD_O0, &QUATALG_PINFTY);

    ibz_finalize(&n_temp);
    quat_alg_elem_finalize(&gen);
    quat_alg_elem_finalize(&gen_rerand);
    ibq_finalize(&q_norm);
}

void
matrix_application_even_basis_basic(ec_basis_t *bas, const ec_curve_t *E, ibz_mat_2x2_t *mat)
{
    digit_t scalars[2][NWORDS_FIELD] = { 0 };

    ibz_t tmp, pow_two;
    ibz_init(&tmp);
    ibz_init(&pow_two);
    int f = POWER_OF_2;
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
    ec_biscalar_mul(&bas->P, E, scalars[0], scalars[1], &tmp_bas);
    ibz_to_digit_array(scalars[0], &(*mat)[0][1]);
    ibz_to_digit_array(scalars[1], &(*mat)[1][1]);
    ec_biscalar_mul(&bas->Q, E, scalars[0], scalars[1], &tmp_bas);

    ibz_sub(&tmp, &(*mat)[0][0], &(*mat)[0][1]);
    ibz_mod(&tmp, &tmp, &pow_two);
    ibz_to_digit_array(scalars[0], &tmp);
    ibz_sub(&tmp, &(*mat)[1][0], &(*mat)[1][1]);
    ibz_mod(&tmp, &tmp, &pow_two);
    ibz_to_digit_array(scalars[1], &tmp);
    ec_biscalar_mul(&bas->PmQ, E, scalars[0], scalars[1], &tmp_bas);

    ibz_finalize(&tmp);
    ibz_finalize(&pow_two);
}

void ec_iso_inv(ec_isom_t* isom){
    fp2_t tmp;
    fp2_copy(&tmp, &isom->D);
    fp2_copy(&isom->D, &isom->Nx);
    fp2_copy(&isom->Nx, &tmp);
    fp2_neg(&isom->Nz, &isom->Nz);
}
