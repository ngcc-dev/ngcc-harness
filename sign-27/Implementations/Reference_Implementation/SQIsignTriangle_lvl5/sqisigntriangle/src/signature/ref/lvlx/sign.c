#include <signature.h>
#include <tools.h>
#include <hd.h>
#include <quaternion_data.h>
#include <id2iso.h>
#include <torsion_constants.h>
#include <encoded_sizes.h>
#include <string.h>

enum {
    NEW_SIGN_RESPONSE_RETRY_LIMIT = 64,
    NEW_SIGN_AUX_RETRY_LIMIT = 64,
};

// compute the commitment with ideal to isogeny clapotis
// and apply it to the basis of E0 (together with the multiplication by some scalar u)
static bool
commit(ec_curve_t *E_com, ec_basis_t *basis_even_com, quat_left_ideal_t *lideal_com)
{

    bool found = false;

    found = quat_sampling_random_ideal_O0_given_norm(lideal_com, &COM_DEGREE, 1, &QUAT_represent_integer_params, NULL);
    // replacing it with a shorter prime norm equivalent ideal
    found = found && quat_lideal_prime_norm_reduced_equivalent(
                         lideal_com, &QUATALG_PINFTY, QUAT_primality_num_iter, QUAT_equiv_bound_coeff);
    // ideal to isogeny clapotis
    found = found && dim2id2iso_arbitrary_isogeny_evaluation(basis_even_com, E_com, lideal_com);
    return found;
}

int
new_protocols_sign(new_signature_t *sig, const public_key_t *pk, secret_key_t *sk, const unsigned char *m, size_t l)
{
    int ret = 0;

    ibz_t c1, c2, k, aux_norm, pow_two, gcd;
    ibz_init(&c1);
    ibz_init(&c2);
    ibz_init(&k);
    ibz_init(&aux_norm);
    ibz_init(&pow_two);
    ibz_init(&gcd);

    quat_left_ideal_t lideal_commit, lideal_sk_conj, lideal_com_rsp1, lideal_com_rsp;
    quat_left_ideal_t lideal_aux, lideal_sk_rsp, lideal_cra;
    quat_left_ideal_init(&lideal_commit);
    quat_left_ideal_init(&lideal_sk_conj);
    quat_left_ideal_init(&lideal_com_rsp1);
    quat_left_ideal_init(&lideal_com_rsp);
    quat_left_ideal_init(&lideal_aux);
    quat_left_ideal_init(&lideal_sk_rsp);
    quat_left_ideal_init(&lideal_cra);

    quat_lattice_t temp_parent;
    quat_lattice_init(&temp_parent);

    ibz_mat_4x4_t temp1, temp2;
    ibz_mat_4x4_init(&temp1);
    ibz_mat_4x4_init(&temp2);

    ec_curve_t E_com, E_aux;
    ec_basis_t B_com;
    ec_curve_init(&E_com);
    ec_curve_init(&E_aux);

    fp2_t pk_j_inv;
    ec_j_inv(&pk_j_inv, &pk->curve);

    quat_lideal_conjugate_without_hnf(&lideal_sk_conj, &temp_parent, &sk->secret_ideal, &QUATALG_PINFTY);

    while (!ret) {
        if (!commit(&E_com, &B_com, &lideal_commit)) {
            continue;
        }

        hash_to_challenge_prime_with_pk_j(&c1, &c2, &pk_j_inv, &E_com, m, l);

        quat_lideal_lideal_mul_reduced(&lideal_com_rsp1, &temp1, &lideal_sk_conj, &lideal_commit, &QUATALG_PINFTY);

        for (int rsp_attempt = 0; !ret && rsp_attempt < NEW_SIGN_RESPONSE_RETRY_LIMIT; ++rsp_attempt) {
            int e_rsp;

            if (!quat_equivalent_special_norm_ideal(&lideal_com_rsp, &k, &lideal_com_rsp1, &c1, &c2, &QUATALG_PINFTY)) {
                continue;
            }

            e_rsp = mpz_sizeinbase(lideal_com_rsp.norm, 2);
            if (e_rsp > TORSION_EVEN_POWER || e_rsp > 8 * SQISIGNTRIANGLE_Q_BYTES ||
                !ibz_is_odd(&lideal_com_rsp.norm)) {
                continue;
            }

            ibz_pow(&pow_two, &ibz_const_two, e_rsp);
            ibz_sub(&aux_norm, &pow_two, &lideal_com_rsp.norm);
            ibz_gcd(&gcd, &lideal_com_rsp.norm, &aux_norm);
            if (!ibz_is_one(&gcd)) {
                continue;
            }

            quat_lideal_lideal_mul_reduced(&lideal_sk_rsp, &temp2, &sk->secret_ideal, &lideal_com_rsp, &QUATALG_PINFTY);

            for (int aux_attempt = 0; !ret && aux_attempt < NEW_SIGN_AUX_RETRY_LIMIT; ++aux_attempt) {
                if (!quat_sampling_random_ideal_O0_given_norm(
                        &lideal_aux, &aux_norm, 0, &QUAT_represent_integer_params, &QUAT_prime_cofactor)) {
                    continue;
                }

                quat_lideal_inter(&lideal_cra, &lideal_sk_rsp, &lideal_aux, &QUATALG_PINFTY);

                if (!dim2id2iso_arbitrary_isogeny_evaluation(&sig->B_aux, &E_aux, &lideal_cra)) {
                    continue;
                }

                {
                    ibz_mat_2x2_t mat;
                    ibz_mat_2x2_init(&mat);
                    ibz_mat_2x2_copy(&mat, &sk->mat_BAcan_to_BA0_two);
                    ret = matrix_application_even_basis(&sig->B_aux, &E_aux, &mat, TORSION_EVEN_POWER);
                    ibz_mat_2x2_finalize(&mat);
                    if (!ret) {
                        continue;
                    }
                }

                ec_dbl_iter_basis(&sig->B_aux, TORSION_EVEN_POWER - e_rsp, &sig->B_aux, &E_aux);
                ec_normalize_curve(&E_aux);
                fp2_copy(&sig->E_aux_A, &E_aux.A);
                memset(sig->q, 0, sizeof(sig->q));
                ibz_to_digit_array(sig->q, &lideal_com_rsp.norm);
            }
        }
    }

    quat_left_ideal_finalize(&lideal_commit);
    quat_left_ideal_finalize(&lideal_sk_conj);
    quat_left_ideal_finalize(&lideal_com_rsp1);
    quat_left_ideal_finalize(&lideal_com_rsp);
    quat_left_ideal_finalize(&lideal_aux);
    quat_left_ideal_finalize(&lideal_sk_rsp);
    quat_left_ideal_finalize(&lideal_cra);

    quat_lattice_finalize(&temp_parent);
    ibz_mat_4x4_finalize(&temp1);
    ibz_mat_4x4_finalize(&temp2);

    ibz_finalize(&c1);
    ibz_finalize(&c2);
    ibz_finalize(&k);
    ibz_finalize(&aux_norm);
    ibz_finalize(&pow_two);
    ibz_finalize(&gcd);

    return ret;
}
