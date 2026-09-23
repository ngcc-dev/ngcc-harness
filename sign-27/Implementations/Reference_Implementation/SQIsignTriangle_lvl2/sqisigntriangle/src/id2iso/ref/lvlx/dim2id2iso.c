#include <ec.h>
#include <endomorphism_action.h>
#include <hd.h>
#include <id2iso.h>
#include <inttypes.h>
#include <locale.h>
#include <quaternion.h>
#include <stdio.h>
#include <tools.h>
#include <torsion_constants.h>


static int
_fixed_degree_isogeny_impl(quat_left_ideal_t *lideal,
                           const ibz_t *u,
                           bool small,
                           theta_couple_curve_t *E34,
                           theta_couple_point_t *P12,
                           size_t numP,
                           const int index_alternate_order)
{

    // var declaration
    int ret;
    ibz_t two_pow, tmp;
    quat_alg_elem_t theta;

    ec_curve_t E0;
    copy_curve(&E0, &CURVES_WITH_ENDOMORPHISMS[index_alternate_order].curve);
    ec_curve_normalize_A24(&E0);

    unsigned length;

    int u_bitsize = ibz_bitsize(u);

    // deciding the power of 2 of the dim2 isogeny we use for this
    // the smaller the faster, but if it set too low there is a risk that
    // RepresentInteger will fail
    if (!small) {
        // in that case, we just set it to be the biggest value possible
        length = TORSION_EVEN_POWER - HD_extra_torsion;
    } else {
        length = ibz_bitsize(&QUATALG_PINFTY.p) + QUAT_repres_bound_input - u_bitsize;
        assert(u_bitsize < (int)length);
        assert(length < TORSION_EVEN_POWER - HD_extra_torsion);
    }
    assert(length);

    // var init
    ibz_init(&two_pow);
    ibz_init(&tmp);
    quat_alg_elem_init(&theta);

    ibz_pow(&two_pow, &ibz_const_two, length);
    ibz_copy(&tmp, u);
    assert(ibz_cmp(&two_pow, &tmp) > 0);
    assert(!ibz_is_even(&tmp));

    // computing the endomorphism theta of norm u * (2^(length) - u)
    ibz_sub(&tmp, &two_pow, &tmp);
    ibz_mul(&tmp, &tmp, u);
    assert(!ibz_is_even(&tmp));

    // setting-up the quat_represent_integer_params
    quat_represent_integer_params_t ri_params;
    ri_params.primality_test_iterations = QUAT_represent_integer_params.primality_test_iterations;

    quat_p_extremal_maximal_order_t order_hnf;
    quat_alg_elem_init(&order_hnf.z);
    quat_alg_elem_copy(&order_hnf.z, &EXTREMAL_ORDERS[index_alternate_order].z);
    quat_alg_elem_init(&order_hnf.t);
    quat_alg_elem_copy(&order_hnf.t, &EXTREMAL_ORDERS[index_alternate_order].t);
    quat_lattice_init(&order_hnf.order);
    ibz_copy(&order_hnf.order.denom, &EXTREMAL_ORDERS[index_alternate_order].order.denom);
    ibz_mat_4x4_copy(&order_hnf.order.basis, &EXTREMAL_ORDERS[index_alternate_order].order.basis);
    order_hnf.q = EXTREMAL_ORDERS[index_alternate_order].q;
    ri_params.order = &order_hnf;
    ri_params.algebra = &QUATALG_PINFTY;

#ifndef NDEBUG
    assert(quat_lattice_contains(NULL, &ri_params.order->order, &ri_params.order->z));
    assert(quat_lattice_contains(NULL, &ri_params.order->order, &ri_params.order->t));
#endif

    ret = quat_represent_integer(&theta, &tmp, 1, &ri_params);

    assert(!ibz_is_even(&tmp));

    if (!ret) {
        printf("represent integer failed for the alternate order number %d and for "
               "a target of "
               "size %d for a u of size %d with length = "
               "%u \n",
               index_alternate_order,
               ibz_bitsize(&tmp),
               ibz_bitsize(u),
               length);
        goto cleanup;
    }
    quat_lideal_create(lideal, &theta, u, &order_hnf.order, &QUATALG_PINFTY);

    quat_alg_elem_finalize(&order_hnf.z);
    quat_alg_elem_finalize(&order_hnf.t);
    quat_lattice_finalize(&order_hnf.order);

#ifndef NDEBUG
    ibz_t test_norm, test_denom;
    ibz_init(&test_denom);
    ibz_init(&test_norm);
    quat_alg_norm(&test_norm, &test_denom, &theta, &QUATALG_PINFTY);
    assert(ibz_is_one(&test_denom));
    assert(ibz_cmp(&test_norm, &tmp) == 0);
    assert(!ibz_is_even(&tmp));
    assert(quat_lattice_contains(NULL, &EXTREMAL_ORDERS[index_alternate_order].order, &theta));
    ibz_finalize(&test_norm);
    ibz_finalize(&test_denom);
#endif

    ec_basis_t B0_two;
    // copying the basis
    copy_basis(&B0_two, &CURVES_WITH_ENDOMORPHISMS[index_alternate_order].basis_even);
    assert(test_basis_order_twof(&B0_two, &E0, TORSION_EVEN_POWER));
    ec_dbl_iter_basis(&B0_two, TORSION_EVEN_POWER - length - HD_extra_torsion, &B0_two, &E0);

    assert(test_basis_order_twof(&B0_two, &E0, length + HD_extra_torsion));

    // now we set-up the kernel
    theta_couple_point_t T1;
    theta_couple_point_t T2, T1m2;

    copy_point(&T1.P1, &B0_two.P);
    copy_point(&T2.P1, &B0_two.Q);
    copy_point(&T1m2.P1, &B0_two.PmQ);

    // multiplication of theta by (u)^-1 mod 2^(length+2)
    ibz_mul(&two_pow, &two_pow, &ibz_const_two);
    ibz_mul(&two_pow, &two_pow, &ibz_const_two);
    ibz_copy(&tmp, u);
    ibz_invmod(&tmp, &tmp, &two_pow);
    assert(!ibz_is_even(&tmp));

    ibz_mul(&theta.coord[0], &theta.coord[0], &tmp);
    ibz_mul(&theta.coord[1], &theta.coord[1], &tmp);
    ibz_mul(&theta.coord[2], &theta.coord[2], &tmp);
    ibz_mul(&theta.coord[3], &theta.coord[3], &tmp);

    // applying theta to the basis
    ec_basis_t B0_two_theta;
    copy_basis(&B0_two_theta, &B0_two);
    endomorphism_application_even_basis(&B0_two_theta, index_alternate_order, &E0, &theta, length + HD_extra_torsion);

    // Ensure the basis we're using has the expected order
    assert(test_basis_order_twof(&B0_two_theta, &E0, length + HD_extra_torsion));

    // Set-up the domain E0 x E0
    theta_couple_curve_t E00;
    E00.E1 = E0;
    E00.E2 = E0;

    // Set-up the kernel from the bases
    theta_kernel_couple_points_t dim_two_ker;
    copy_bases_to_kernel(&dim_two_ker, &B0_two, &B0_two_theta);

    ret = theta_chain_compute_and_eval(length, &E00, &dim_two_ker, true, E34, P12, numP);
    if (!ret)
        goto cleanup;

    assert(length);
    ret = (int)length;

cleanup:
    // var finalize
    ibz_finalize(&two_pow);
    ibz_finalize(&tmp);
    quat_alg_elem_finalize(&theta);

    return ret;
}

int
fixed_degree_isogeny_and_eval(quat_left_ideal_t *lideal,
                              const ibz_t *u,
                              bool small,
                              theta_couple_curve_t *E34,
                              theta_couple_point_t *P12,
                              size_t numP,
                              const int index_alternate_order)
{
    return _fixed_degree_isogeny_impl(lideal, u, small, E34, P12, numP, index_alternate_order);
}

#define ID2ISO_DEBUG_PRINT 0

void
weil_basis(fp2_t *out, int e, const ec_basis_t *basis, const ec_curve_t *curve)
{
    ibz_vec_2_t tmp;
    ec_point_t wi;
    ec_curve_t ec;
    ec_point_init(&wi);
    ec_curve_init(&ec);
    ibz_vec_2_init(&tmp);
    ibz_set(&(tmp[0]), 1);
    ibz_set(&(tmp[1]), 1);
    ec_biscalar_mul_ibz_vec(&wi, &tmp, e, basis, curve);
    copy_curve(&ec, curve);
    weil(out, e, &basis->P, &basis->Q, &wi, &ec);
    ibz_vec_2_finalize(&tmp);
}

void
ec_basis_scalar_mul_ibz(ec_basis_t *out, int e, const ibz_t *n, const ec_basis_t *input, const ec_curve_t *curve)
{
#ifndef NDEBUG
    fp2_t prod, r1, r2;
    digit_t array[NWORDS_ORDER];
    weil_basis(&r1, e, input, curve);
#endif
    ibz_vec_2_t tmp;
    ec_basis_t used;
    copy_basis(&used, input);
    ibz_vec_2_init(&tmp);
    ibz_copy(&(tmp[0]), n);
    ibz_set(&(tmp[1]), 0);
    ec_biscalar_mul_ibz_vec(&out->P, &tmp, e, &used, curve);
    ibz_swap((&tmp[0]), &(tmp[1]));
    ec_biscalar_mul_ibz_vec(&out->Q, &tmp, e, &used, curve);
    ibz_copy(&(tmp[0]), &(tmp[1]));
    ibz_t power_two;
    ibz_init(&power_two);
    ibz_pow(&power_two, &ibz_const_two, e);
    ibz_sub(&(tmp[1]), &power_two, &(tmp[0]));
    ibz_finalize(&power_two);
    ec_biscalar_mul_ibz_vec(&out->PmQ, &tmp, e, &used, curve);
#ifndef NDEBUG
    if (ibz_is_odd(n)) {
        assert(test_basis_order_twof(out, curve, e));
        weil_basis(&r2, e, out, curve);
        if (ibz_bitsize(n) < (int)(sizeof(digit_t) * 8 * NWORDS_ORDER)) {
#if (ID2ISO_DEBUG_PRINT > 1)
            printf("pairing test for scalar mul runs\n");
#endif
            memset(array, 0, NWORDS_ORDER * sizeof(digit_t));
            ibz_to_digit_array(array, n);
            fp2_pow_vartime(&prod, &r1, &array[0], NWORDS_ORDER);
            fp2_pow_vartime(&prod, &prod, &array[0], NWORDS_ORDER);
#if (ID2ISO_DEBUG_PRINT > 2)
            fp2_print("       r2 ", &r2);
            fp2_print("     prod ", &prod);
            fp_neg(&r1.im, &prod.im);
            fp_copy(&r1.re, &prod.re);
            fp2_print("conj prod ", &r1);
#endif
            assert(fp2_is_equal(&prod, &r2));
        }
    }
#endif
    ibz_vec_2_finalize(&tmp);
}

int
dim2id2iso_ideal_to_isogeny_clapotis(quat_alg_elem_t *beta1,
                                     quat_alg_elem_t *beta2,
                                     ibz_t *u,
                                     ibz_t *v,
                                     ibz_t *d1,
                                     ibz_t *d2,
                                     ec_curve_t *codomain,
                                     ec_basis_t *basis,
                                     const quat_left_ideal_t *lideal,
                                     const quat_alg_t *Bpoo)
{
    quat_alg_elem_t mu1, mu2, small, theta, elem, apply_to_basis;
    ibz_t tmp;
    ec_basis_t E0_basis, B2, B1, to_be_pushed;
    fp2_t r1, r2, prod;
    digit_t array[NWORDS_ORDER];
    struct theta_kernel_couple_points ker;
    theta_couple_curve_t E0E0;
    theta_couple_curve_t theta_codomain;
    ibz_copy(u, &ibz_const_one);
    ibz_copy(v, &ibz_const_one);
    ibz_t n, d, two_c;
    ibz_init(&n);
    ibz_init(&d);
    ibz_init(&two_c);
    quat_alg_elem_init(&theta);
    quat_alg_elem_init(&mu1);
    quat_alg_elem_init(&mu2);
    quat_alg_elem_init(&small);
    quat_alg_elem_init(&elem);
    ibz_init(&tmp);
    quat_alg_elem_init(&apply_to_basis);

    int ret;
    int used_torsion = QUAT_qlapoti_used_power_of_two;
    int c;

    ret = quat_qlapoti(&mu1,
                       &mu2,
                       &theta,
                       &small,
                       lideal,
                       Bpoo,
                       QUAT_qlapoti_iteration_bound,
                       QUAT_qlapoti_gen_bound_bits,
                       used_torsion,
                       &QUAT_cornacchia_extended_params);

    if (!ret) {
        ibz_printf("failed qlapoti (SHOULDNT HAPPEN!)\n");
    } else {
        quat_alg_normalize(&theta);
#if (ID2ISO_DEBUG_PRINT > 2)
        quat_alg_elem_print(&theta);
#endif
    }
    if (ret) {
        used_torsion = used_torsion - (0 != (ret & 2));
        c = TORSION_EVEN_POWER - used_torsion;
        ibz_pow(&two_c, &ibz_const_two, c);

#if (ID2ISO_DEBUG_PRINT > 2)
        ibz_printf("used_torsion depending on theta: %d\n", used_torsion);
#endif
        quat_alg_conj(&theta, &theta);
        {
            // norm d1
            quat_alg_norm(d1, &d, &mu1, Bpoo);
            assert(ibz_is_one(&d));
            ibz_mul(d1, d1, &lideal->norm);
            quat_alg_norm(&n, &d, &small, Bpoo);
            assert(ibz_is_one(&d));
            ibz_div(d1, &d, d1, &n);
            assert(ibz_is_zero(&d));

            // beta1
            quat_alg_mul(beta1, &mu1, &small, Bpoo);
            quat_alg_norm(&n, &d, &small, Bpoo);
            ibz_div(&n, &d, &n, &lideal->norm);
            assert(ibz_is_zero(&d));
            ibz_mul(&beta1->denom, &beta1->denom, &n);
            quat_alg_normalize(beta1);

#ifndef NDEBUG
            quat_left_ideal_t I1;
            quat_left_ideal_t betatest;
            quat_alg_elem_t gen;
            quat_left_ideal_init(&I1);
            quat_left_ideal_init(&betatest);
            quat_alg_elem_init(&gen);
            assert(quat_lattice_contains(NULL, &lideal->lattice, &small));
            quat_alg_conj(&gen, &small);
            ibz_mul(&gen.denom, &gen.denom, &lideal->norm);
            quat_lideal_mul(&I1, lideal, &gen, Bpoo);

            assert(quat_lattice_contains(NULL, &I1.lattice, &mu1));
            quat_alg_conj(&gen, &mu1);
            ibz_mul(&gen.denom, &gen.denom, &I1.norm);
            quat_lideal_mul(&I1, &I1, &gen, Bpoo);

            assert(ibz_cmp(d1, &I1.norm) == 0);

            // test beta1
            quat_alg_conj(&gen, beta1);
            ibz_mul(&gen.denom, &gen.denom, &lideal->norm);
            quat_lideal_mul(&betatest, lideal, &gen, Bpoo);
            assert(quat_lideal_equals(&betatest, &I1, Bpoo));

            quat_alg_elem_finalize(&gen);
            quat_left_ideal_finalize(&I1);
            quat_left_ideal_finalize(&betatest);
#endif
        }
        {
            // norm d2
            quat_alg_norm(d2, &d, &mu2, Bpoo);
            assert(ibz_is_one(&d));
            ibz_mul(d2, d2, &lideal->norm);
            quat_alg_norm(&n, &d, &small, Bpoo);
            assert(ibz_is_one(&d));
            ibz_div(d2, &d, d2, &n);
            assert(ibz_is_zero(&d));

            // beta2
            quat_alg_mul(beta2, &mu2, &small, Bpoo);
            quat_alg_norm(&n, &d, &small, Bpoo);
            ibz_div(&n, &d, &n, &lideal->norm);
            assert(ibz_is_zero(&d));
            ibz_mul(&beta2->denom, &beta2->denom, &n);
            quat_alg_normalize(beta2);
#ifndef NDEBUG
            quat_left_ideal_t I2;
            quat_left_ideal_t betatest;
            quat_alg_elem_t gen;
            quat_left_ideal_init(&I2);
            quat_left_ideal_init(&betatest);
            quat_alg_elem_init(&gen);
            assert(quat_lattice_contains(NULL, &lideal->lattice, &small));
            quat_alg_conj(&gen, &small);
            ibz_mul(&gen.denom, &gen.denom, &lideal->norm);
            quat_lideal_mul(&I2, lideal, &gen, Bpoo);

            assert(quat_lattice_contains(NULL, &I2.lattice, &mu2));
            quat_alg_conj(&gen, &mu2);
            ibz_mul(&gen.denom, &gen.denom, &I2.norm);
            quat_lideal_mul(&I2, &I2, &gen, Bpoo);

            assert(ibz_cmp(d2, &I2.norm) == 0);

            // test beta1
            quat_alg_conj(&gen, beta2);
            ibz_mul(&gen.denom, &gen.denom, &lideal->norm);
            quat_lideal_mul(&betatest, lideal, &gen, Bpoo);
            assert(quat_lideal_equals(&betatest, &I2, Bpoo));

            quat_alg_elem_finalize(&gen);
            quat_left_ideal_finalize(&I2);
            quat_left_ideal_finalize(&betatest);
#endif
        }
        assert(ibz_is_odd(d1));
        ibz_copy(&(tmp), d1);
        // mul by 2^c if lower order
        ibz_mul(&(tmp), d1, &two_c);
#if (ID2ISO_DEBUG_PRINT > 2)
        ibz_printf("d1 %Zd\n", d1);
#endif

        ec_basis_scalar_mul_ibz(&E0_basis, TORSION_EVEN_POWER, &two_c, &BASIS_EVEN, &CURVE_E0);
        copy_basis(&B2, &E0_basis);
        ibz_pow(&two_c, &ibz_const_two, TORSION_EVEN_POWER);
        UNUSED int invertible = ibz_invmod(&(tmp), d1, &two_c);

        quat_alg_elem_set(&apply_to_basis, 1, 0, 0, 0, 0);

        ibz_mul(&apply_to_basis.coord[0], &theta.coord[0], &(tmp));
        ibz_mul(&apply_to_basis.coord[1], &theta.coord[1], &(tmp));
        ibz_mul(&apply_to_basis.coord[2], &theta.coord[2], &(tmp));
        ibz_mul(&apply_to_basis.coord[3], &theta.coord[3], &(tmp));
        ibz_copy(&apply_to_basis.denom, &theta.denom);

        assert(test_basis_order_twof(&E0_basis, &CURVE_E0, TORSION_EVEN_POWER - c));
        endomorphism_application_even_basis(&E0_basis, 0, &CURVE_E0, &apply_to_basis, TORSION_EVEN_POWER - c);
        assert(test_basis_order_twof(&E0_basis, &CURVE_E0, TORSION_EVEN_POWER - c));

        // Both are normalized to avoid extra scalar mults
        // B1 = 2^c E0_basis
        // B2 = norm(I1) B1
        copy_basis(&B1, &E0_basis);

        copy_bases_to_kernel(&ker, &B1, &B2);
        copy_curve(&E0E0.E1, &CURVE_E0);
        copy_curve(&E0E0.E2, &CURVE_E0);

        copy_basis(&to_be_pushed, &BASIS_EVEN);
#ifndef NDEBUG
        weil_basis(&r1, TORSION_EVEN_POWER, &to_be_pushed, &CURVE_E0);
#endif
        assert(test_basis_order_twof(&BASIS_EVEN, &CURVE_E0, TORSION_EVEN_POWER));
        // compute to be pushed
        {
            quat_alg_elem_copy(&elem, beta1);
#if (ID2ISO_DEBUG_PRINT > 2)
            quat_alg_elem_print(&elem);
#endif
            quat_alg_norm(&n, &d, &elem, Bpoo);
            assert(ibz_is_one(&d));
            ibz_mul(&d, &lideal->norm, d1);
            assert(0 == ibz_cmp(&d, &n));
#ifndef NDEBUG
            quat_alg_norm(&n, &d, &elem, Bpoo);
            assert(ibz_is_one(&d));
            assert(ibz_is_odd(&n));
#endif
            assert(test_basis_order_twof(&to_be_pushed, &CURVE_E0, TORSION_EVEN_POWER));

            // Apply elem * d1^-1

            // scalar mul by d1 mod 2^Torsion

            ibz_pow(&two_c, &ibz_const_two, TORSION_EVEN_POWER);
            UNUSED int invertible = ibz_invmod(&(tmp), d1, &two_c);
            assert(invertible);

            assert(ibz_bitsize(&two_c) == TORSION_EVEN_POWER + 1);
            ibz_pow(&two_c, &ibz_const_two, c);
            ibz_copy(&d, &(tmp));

            quat_alg_elem_set(&apply_to_basis, 1, 0, 0, 0, 0);
            ibz_mul(&apply_to_basis.coord[0], &elem.coord[0], &(tmp));
            ibz_mul(&apply_to_basis.coord[1], &elem.coord[1], &(tmp));
            ibz_mul(&apply_to_basis.coord[2], &elem.coord[2], &(tmp));
            ibz_mul(&apply_to_basis.coord[3], &elem.coord[3], &(tmp));
            ibz_copy(&apply_to_basis.denom, &elem.denom);

            endomorphism_application_even_basis(&to_be_pushed, 0, &CURVE_E0, &apply_to_basis, TORSION_EVEN_POWER);
            assert(test_basis_order_twof(&to_be_pushed, &CURVE_E0, TORSION_EVEN_POWER));
        }
        weil_basis(&r1, TORSION_EVEN_POWER, &to_be_pushed, &CURVE_E0);
        theta_couple_point_t push[3];
        copy_point(&push[0].P1, &to_be_pushed.P);
        copy_point(&push[1].P1, &to_be_pushed.Q);
        copy_point(&push[2].P1, &to_be_pushed.PmQ);
        ec_point_init(&push[0].P2);
        ec_point_init(&push[1].P2);
        ec_point_init(&push[2].P2);

        ret = theta_chain_compute_and_eval_randomized(
            used_torsion, &E0E0, &ker, false, &theta_codomain, push, sizeof(push) / sizeof(*push));
        if (!ret) {
#if (ID2ISO_DEBUG_PRINT > 1)
            ibz_printf("chain failed\n");
#endif
            goto cleanup;
        }

        // sometimes it is E2 not E1
        copy_point(&basis->P, &push[0].P1);
        copy_point(&basis->Q, &push[1].P1);
        copy_point(&basis->PmQ, &push[2].P1);
        copy_curve(codomain, &theta_codomain.E1);
        memset(array, 0, NWORDS_ORDER * sizeof(digit_t));
        ibz_to_digit_array(array, d1);
        fp2_pow_vartime(&prod, &r1, &array[0], NWORDS_ORDER);
        weil_basis(&r2, TORSION_EVEN_POWER, basis, codomain);
        if (!fp2_is_equal(&prod, &r2)) {
            copy_point(&basis->P, &push[0].P2);
            copy_point(&basis->Q, &push[1].P2);
            copy_point(&basis->PmQ, &push[2].P2);
            copy_curve(codomain, &theta_codomain.E2);
#ifndef NDEBUG
            if (ID2ISO_DEBUG_PRINT > 1)
                printf("not d1\n");
            memset(array, 0, NWORDS_ORDER * sizeof(digit_t));
            weil_basis(&r2, TORSION_EVEN_POWER, basis, codomain);
            assert(fp2_is_equal(&r2, &prod));
            if (ID2ISO_DEBUG_PRINT > 1)
                printf("but d2\n");
#endif
        }
#ifndef NDEBUG
        if (ibz_is_odd(&lideal->norm)) {
            assert(test_point_order_twof(&basis->P, codomain, TORSION_EVEN_POWER));
            assert(test_point_order_twof(&basis->Q, codomain, TORSION_EVEN_POWER));
            assert(test_point_order_twof(&basis->PmQ, codomain, TORSION_EVEN_POWER));
            assert(test_basis_order_twof(basis, codomain, TORSION_EVEN_POWER));
        } else {
            printf("even order ideal\n");

            ibz_add(&d, d1, d2);
            ibz_pow(&n, &ibz_const_two, used_torsion);
            assert(0 == ibz_cmp(&n, &d));
        }
#endif
    }

cleanup:
    ibz_finalize(&n);
    ibz_finalize(&d);
    ibz_finalize(&two_c);
    quat_alg_elem_finalize(&theta);
    quat_alg_elem_finalize(&mu1);
    quat_alg_elem_finalize(&mu2);
    quat_alg_elem_finalize(&small);
    quat_alg_elem_finalize(&elem);
    quat_alg_elem_finalize(&apply_to_basis);
    ibz_finalize(&tmp);
    return ret;
}

int
dim2id2iso_arbitrary_isogeny_evaluation(ec_basis_t *basis, ec_curve_t *codomain, const quat_left_ideal_t *lideal)
{
    int ret;

    quat_alg_elem_t beta1, beta2;
    ibz_t u, v, d1, d2;

    quat_alg_elem_init(&beta1);
    quat_alg_elem_init(&beta2);

    ibz_init(&u);
    ibz_init(&v);
    ibz_init(&d1);
    ibz_init(&d2);

    ret = dim2id2iso_ideal_to_isogeny_clapotis(
        &beta1, &beta2, &u, &v, &d1, &d2, codomain, basis, lideal, &QUATALG_PINFTY);

    quat_alg_elem_finalize(&beta1);
    quat_alg_elem_finalize(&beta2);

    ibz_finalize(&u);
    ibz_finalize(&v);
    ibz_finalize(&d1);
    ibz_finalize(&d2);

    return ret;
}
