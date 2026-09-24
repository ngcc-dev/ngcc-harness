#include <inttypes.h>
#include "dim2id2iso_tests.h"
#include <tools.h>

int
dim2id2iso_test_fixed_degree_isogeny()
{

    ibz_t u, two_pow, adjust_u;
    ibz_t tmp;
    quat_left_ideal_t lideal;
    ibz_init(&u);
    ibz_init(&adjust_u);
    ibz_init(&tmp);
    ibz_init(&two_pow);
    quat_left_ideal_init(&lideal);

    // u is 3 times a random prime
    generate_random_prime(&u, 1, 110);
    ibz_mul(&u, &u, &ibz_const_three);

    theta_chain_t F;
    fixed_degree_isogeny(&F, &lideal, &u, &adjust_u, 0);

    // now we check that we get a correct codomain in the end
    // by evaluating some point and checking the pairing
    theta_couple_jac_point_t Teval1, Teval2, Teval3;
    jac_point_t temp;
    theta_couple_point_t Tev1, Tev2, Tev3;
    theta_couple_curve_t E00;
    ec_curve_t E0;
    E0 = CURVE_E0;
    E00.E1 = CURVE_E0;
    E00.E2 = CURVE_E0;

    ec_curve_init(&E0);
    ec_curve_init(&E00.E1);
    ec_curve_init(&E00.E2);

    ec_basis_t bas = BASIS_EVEN;
    lift_basis(&Teval1.P1, &Teval2.P1, &bas, &E0);
    jac_neg(&temp, &Teval2.P1);
    ADD(&Teval3.P1, &Teval1.P1, &temp, &E0);
    fp2_set_zero(&Teval1.P2.x);
    fp2_set_one(&Teval1.P2.y);
    fp2_set_zero(&Teval1.P2.z);
    fp2_set_zero(&Teval2.P2.x);
    fp2_set_one(&Teval2.P2.y);
    fp2_set_zero(&Teval2.P2.z);
    fp2_set_zero(&Teval3.P2.x);
    fp2_set_one(&Teval3.P2.y);
    fp2_set_zero(&Teval3.P2.z);
    theta_chain_eval_no_help(&Tev1, &F, &Teval1, &E00);
    theta_chain_eval_no_help(&Tev2, &F, &Teval2, &E00);
    theta_chain_eval_no_help(&Tev3, &F, &Teval3, &E00);

    assert(test_point_order_twof(&Tev1.P1, &F.codomain.E1, TORSION_PLUS_EVEN_POWER));
    assert(test_point_order_twof(&Tev1.P2, &F.codomain.E2, TORSION_PLUS_EVEN_POWER));
    assert(test_point_order_twof(&Tev2.P1, &F.codomain.E1, TORSION_PLUS_EVEN_POWER));
    assert(test_point_order_twof(&Tev2.P2, &F.codomain.E2, TORSION_PLUS_EVEN_POWER));
    assert(test_point_order_twof(&Tev3.P1, &F.codomain.E1, TORSION_PLUS_EVEN_POWER));
    assert(test_point_order_twof(&Tev3.P2, &F.codomain.E2, TORSION_PLUS_EVEN_POWER));

    fp2_t w0, w1, w2;
    ec_point_t AC, A24;
    copy_point(&AC, &CURVE_E0_A24); // Warning, this is AC, not A24!
    A24_from_AC(&A24, &AC);
    weil(&w0, TORSION_PLUS_EVEN_POWER, &bas.P, &bas.Q, &bas.PmQ, &A24);
    // Changing the AC
    fp2_copy(&AC.x, &F.codomain.E1.A);
    fp2_copy(&AC.z, &F.codomain.E1.C);
    A24_from_AC(&A24, &AC);
    weil(&w1, TORSION_PLUS_EVEN_POWER, &Tev1.P1, &Tev2.P1, &Tev3.P1, &A24);
    fp2_copy(&AC.x, &F.codomain.E2.A);
    fp2_copy(&AC.z, &F.codomain.E2.C);
    A24_from_AC(&A24, &AC);
    weil(&w2, TORSION_PLUS_EVEN_POWER, &Tev1.P2, &Tev2.P2, &Tev3.P2, &A24);
    ibz_pow(&two_pow, &ibz_const_two, F.length);
    ibz_mul(&tmp, &adjust_u, &adjust_u);
    ibz_mul(&tmp, &tmp, &u);
    ibz_sub(&two_pow, &two_pow, &tmp);
    // now we are checking that one of the two is equal to the correct value
    digit_t digit_u[NWORDS_ORDER] = { 0 };
    ibz_to_digit_array(digit_u, &tmp);
    fp2_t test_pow;
    fp2_pow_vartime(&test_pow, &w0, digit_u, NWORDS_ORDER);

    // it seems like we always get the second curve
    assert(fp2_is_equal(&test_pow, &w2));
    ibz_to_digit_array(digit_u, &two_pow);
    fp2_pow_vartime(&test_pow, &w0, digit_u, NWORDS_ORDER);
    assert(fp2_is_equal(&test_pow, &w1));

    if (fp2_is_equal(&test_pow, &w1)) {
        ibz_to_digit_array(digit_u, &two_pow);
        fp2_pow_vartime(&test_pow, &w0, digit_u, NWORDS_ORDER);
        fp2_is_equal(&test_pow, &w2);
    } else if (fp2_is_equal(&test_pow, &w2)) {
        assert(fp2_is_equal(&test_pow, &w2));
        ibz_to_digit_array(digit_u, &two_pow);
        fp2_pow_vartime(&test_pow, &w0, digit_u, NWORDS_ORDER);
        assert(fp2_is_equal(&test_pow, &w1));
    } else {
        assert(0);
    }

    ibz_finalize(&adjust_u);
    ibz_finalize(&tmp);
    ibz_finalize(&u);
    ibz_finalize(&two_pow);
    quat_left_ideal_finalize(&lideal);
    return 1;
}

int
dim2id2iso_test_find_uv()
{
    // var dec
    int found = 1;
    ibz_t temp, remainder, n1, n2;
    ibq_t ibq_norm;
    quat_alg_elem_t gen;

    quat_left_ideal_t lideal_small;
    quat_order_t right_order;
    ibz_mat_4x4_t reduced, gram;
    ibz_vec_4_t coeffs;
    quat_alg_elem_t beta1, beta2;
    ibz_t u, v, d1, d2, target;
    // var init
    ibz_init(&target);
    ibq_init(&ibq_norm);
    ibz_init(&temp);
    ibz_init(&remainder);
    ibz_init(&n1);
    ibz_init(&n2);
    quat_alg_elem_init(&gen);
    quat_left_ideal_init(&lideal_small);
    quat_order_init(&right_order);
    ibz_mat_4x4_init(&reduced);
    ibz_mat_4x4_init(&gram);
    ibz_vec_4_init(&coeffs);

    quat_alg_elem_init(&beta1);
    quat_alg_elem_init(&beta2);

    ibz_init(&u);
    ibz_init(&v);
    ibz_init(&d1);
    ibz_init(&d2);

    // computation of lideal_small
    generate_random_prime(&n1, 1, ibz_bitsize(&QUATALG_PINFTY.p) / 2);
    generate_random_prime(&n2, 1, ibz_bitsize(&QUATALG_PINFTY.p));
    ibz_mul(&temp, &n1, &n2);
    found = found && represent_integer(&gen, &temp, &QUATALG_PINFTY);
    assert(found);
    quat_lideal_create_from_primitive(
        &lideal_small, &gen, &n1, &STANDARD_EXTREMAL_ORDER.order, &QUATALG_PINFTY);

    int exp = TORSION_PLUS_EVEN_POWER;
    ibz_pow(&target, &ibz_const_two, exp);

    found = 0;
    int num_rerun = 0;
    while (!found && num_rerun < 3) {
        found = find_uv(&u,
                        &v,
                        &coeffs,
                        &beta1,
                        &beta2,
                        &d1,
                        &d2,
                        &target,
                        0,
                        &lideal_small,
                        &QUATALG_PINFTY,
                        num_rerun);
        num_rerun++;
    }
    // assert(found);
    if (!found) {
        printf("not found \n");
        ibz_printf("%Zd %Zd %Zd %Zd \n", u, v, d1, d2);
    }

    quat_lattice_contains(&coeffs, &lideal_small.lattice, &beta1, &QUATALG_PINFTY);
    quat_alg_norm(&ibq_norm, &beta1, &QUATALG_PINFTY);
    ibq_to_ibz(&n1, &ibq_norm);
    ibz_div(&n1, &remainder, &n1, &lideal_small.norm);
    assert(ibz_cmp(&remainder, &ibz_const_zero) == 0);
    assert(ibz_cmp(&n1, &d1) == 0);

    quat_lattice_contains(&coeffs, &lideal_small.lattice, &beta2, &QUATALG_PINFTY);
    quat_alg_norm(&ibq_norm, &beta2, &QUATALG_PINFTY);
    ibq_to_ibz(&n2, &ibq_norm);
    ibz_div(&n2, &remainder, &n2, &lideal_small.norm);
    assert(ibz_cmp(&remainder, &ibz_const_zero) == 0);
    assert(ibz_cmp(&n2, &d2) == 0);

    ibz_pow(&remainder, &ibz_const_two, TORSION_PLUS_EVEN_POWER);

    ibz_mul(&n1, &d1, &u);
    ibz_mul(&temp, &d2, &v);
    ibz_add(&n1, &temp, &n1);
    assert(ibz_cmp(&remainder, &n1) == 0);

    ibq_finalize(&ibq_norm);
    ibz_finalize(&temp);
    ibz_finalize(&remainder);
    ibz_finalize(&n1);
    ibz_finalize(&n2);
    quat_alg_elem_finalize(&gen);
    quat_left_ideal_finalize(&lideal_small);
    quat_order_finalize(&right_order);
    ibz_mat_4x4_finalize(&reduced);
    ibz_mat_4x4_finalize(&gram);
    ibz_finalize(&u);
    ibz_finalize(&v);
    ibz_finalize(&d1);
    ibz_finalize(&d2);
    ibz_vec_4_finalize(&coeffs);
    ibz_finalize(&target);

    quat_alg_elem_finalize(&beta1);
    quat_alg_elem_finalize(&beta2);

    return found;
}

int
dim2id2iso_test_dimid2iso()
{

    // var dec
    int found = 1;
    ibz_t temp, remainder, n1, n2;
    ibq_t ibq_norm;
    quat_alg_elem_t gen;

    quat_left_ideal_t lideal_small;
    quat_order_t right_order;
    ibz_mat_4x4_t reduced, gram;
    ibz_vec_4_t coeffs;
    quat_alg_elem_t beta1, beta2;
    ibz_t u, v, d1, d2;

    // theta stuff
    theta_chain_t Phi;
    theta_chain_t phiu, phiv;

    // var init
    ibq_init(&ibq_norm);
    ibz_init(&temp);
    ibz_init(&remainder);
    ibz_init(&n1);
    ibz_init(&n2);
    quat_alg_elem_init(&gen);
    quat_left_ideal_init(&lideal_small);
    quat_order_init(&right_order);
    ibz_mat_4x4_init(&reduced);
    ibz_mat_4x4_init(&gram);
    ibz_vec_4_init(&coeffs);

    quat_alg_elem_init(&beta1);
    quat_alg_elem_init(&beta2);

    ibz_init(&u);
    ibz_init(&v);
    ibz_init(&d1);
    ibz_init(&d2);

    // computation of lideal_small
    generate_random_prime(&n1, 1, ibz_bitsize(&QUATALG_PINFTY.p) / 2);
    generate_random_prime(&n2, 1, ibz_bitsize(&QUATALG_PINFTY.p));
    ibz_mul(&temp, &n1, &n2);
    found = found && represent_integer(&gen, &temp, &QUATALG_PINFTY);
    assert(found);
    quat_lideal_create_from_primitive(
        &lideal_small, &gen, &n1, &STANDARD_EXTREMAL_ORDER.order, &QUATALG_PINFTY);
    ec_basis_t bas = BASIS_EVEN;
    ec_basis_t bas_end;
    ec_curve_t codom;
    ec_curve_init(&codom);

    clock_t tt = tic();
    found = dim2id2iso_ideal_to_isogeny_clapotis(&Phi,
                                                 &beta1,
                                                 &beta2,
                                                 &u,
                                                 &v,
                                                 &coeffs,
                                                 &phiu,
                                                 &phiv,
                                                 &d1,
                                                 &d2,
                                                 &codom,
                                                 &bas_end,
                                                 &lideal_small,
                                                 &QUATALG_PINFTY);

    TOC(tt, "total time for ideal to isogeny clapotis");

    ibq_finalize(&ibq_norm);
    ibz_finalize(&temp);
    ibz_finalize(&remainder);
    ibz_finalize(&n1);
    ibz_finalize(&n2);
    quat_alg_elem_finalize(&gen);
    quat_left_ideal_finalize(&lideal_small);
    quat_order_finalize(&right_order);
    ibz_mat_4x4_finalize(&reduced);
    ibz_mat_4x4_finalize(&gram);
    ibz_finalize(&u);
    ibz_finalize(&v);
    ibz_finalize(&d1);
    ibz_finalize(&d2);
    ibz_vec_4_finalize(&coeffs);

    quat_alg_elem_finalize(&beta1);
    quat_alg_elem_finalize(&beta2);

    return found;
}

int
main()
{

    int res = 1;

    randombytes_init((unsigned char *)"some", (unsigned char *)"string", 128);

    printf("\nRunning dim2id2iso module unit tests\n");

    printf("Running find uv tests \n");
    for (int i = 0; i < 100; i++) {
        res = res & dim2id2iso_test_find_uv();
    }

    printf("\nRunning fixed degree tests\n");

    res = res & dim2id2iso_test_fixed_degree_isogeny();

    printf("\nRunning id2iso_clapotis tests\n");

    res = res & dim2id2iso_test_dimid2iso();

    if (!res) {
        printf("\nSome tests failed!\n");
    } else {
        printf("\nAll tests passed!\n");
    }
    return (!res);
}
