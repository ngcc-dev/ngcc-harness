#include <verification.h>
#include <hd.h>
#include <encoded_sizes.h>

int
new_protocols_verify(new_signature_t *sig, const public_key_t *pk, const unsigned char *m, size_t l)
{
    int verify = 0;

    ibz_t q, c1, c2, c3, c4, rem;
    ibz_init(&q);
    ibz_init(&c1);
    ibz_init(&c2);
    ibz_init(&c3);
    ibz_init(&c4);
    ibz_init(&rem);

    ibz_copy_digit_array(&q, sig->q);
    if (ibz_cmp(&q, &ibz_const_zero) <= 0 || !ibz_is_odd(&q))
        goto cleanup;

    int e_rsp = ibz_bitsize(&q);
    if (e_rsp < 3 || e_rsp > TORSION_EVEN_POWER)
        goto cleanup;

    if (!ec_curve_verify_A(&(pk->curve).A))
        goto cleanup;

    ec_curve_t E_pk, E_aux;
    copy_curve(&E_pk, &pk->curve);
    if (!ec_curve_init_from_A(&E_aux, &sig->E_aux_A))
        goto cleanup;

    fp2_t pk_j_inv;
    ec_j_inv(&pk_j_inv, &pk->curve);

    ec_basis_t B_pk;
    if (!ec_curve_to_basis_2f_from_hint(&B_pk, &E_pk, TORSION_EVEN_POWER, pk->hint_pk))
        goto cleanup;

    ec_mul(&B_pk.P, sig->q, e_rsp, &B_pk.P, &E_pk);
    ec_mul(&B_pk.Q, sig->q, e_rsp, &B_pk.Q, &E_pk);
    ec_mul(&B_pk.PmQ, sig->q, e_rsp, &B_pk.PmQ, &E_pk);
    ec_dbl_iter_basis(&B_pk, TORSION_EVEN_POWER - e_rsp, &B_pk, &E_pk);

    if (!test_basis_order_twof(&B_pk, &E_pk, e_rsp))
        goto cleanup;
    if (!test_basis_order_twof(&sig->B_aux, &E_aux, e_rsp))
        goto cleanup;

    theta_couple_curve_t EpkxEaux;
    copy_curve(&EpkxEaux.E1, &E_pk);
    copy_curve(&EpkxEaux.E2, &E_aux);

    theta_kernel_couple_points_t dim_two_ker;
    copy_bases_to_kernel(&dim_two_ker, &B_pk, &sig->B_aux);

    theta_couple_curve_t codomain;
    ec_curve_init(&codomain.E1);
    ec_curve_init(&codomain.E2);
    if (!theta_chain_compute_and_eval_verify(e_rsp, &EpkxEaux, &dim_two_ker, false, &codomain, NULL, 0))
        goto cleanup;

    hash_to_challenge_prime_with_pk_j(&c1, &c2, &pk_j_inv, &codomain.E1, m, l);
    ibz_sub(&rem, &q, &c2);
    ibz_mod(&rem, &rem, &c1);
    if (ibz_is_zero(&rem)) {
        verify = 1;
        goto cleanup;
    }

    hash_to_challenge_prime_with_pk_j(&c3, &c4, &pk_j_inv, &codomain.E2, m, l);
    ibz_sub(&rem, &q, &c4);
    ibz_mod(&rem, &rem, &c3);
    verify = ibz_is_zero(&rem);

cleanup:
    ibz_finalize(&q);
    ibz_finalize(&c1);
    ibz_finalize(&c2);
    ibz_finalize(&c3);
    ibz_finalize(&c4);
    ibz_finalize(&rem);

    return verify;
}
