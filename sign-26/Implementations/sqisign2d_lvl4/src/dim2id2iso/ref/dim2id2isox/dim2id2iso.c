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

void
swap(ibz_t *a, ibz_t *b, ibz_vec_4_t *va, ibz_vec_4_t *vb)
{
    ibz_t temp;
    ibz_vec_4_t vtemp;
    ibz_init(&temp);
    ibz_vec_4_init(&vtemp);

    ibz_copy(&temp, a);
    ibz_copy(&vtemp[0], &(*va)[0]);
    ibz_copy(&vtemp[1], &(*va)[1]);
    ibz_copy(&vtemp[2], &(*va)[2]);
    ibz_copy(&vtemp[3], &(*va)[3]);
    ibz_copy(a, b);
    ibz_copy(&(*va)[0], &(*vb)[0]);
    ibz_copy(&(*va)[1], &(*vb)[1]);
    ibz_copy(&(*va)[2], &(*vb)[2]);
    ibz_copy(&(*va)[3], &(*vb)[3]);
    ibz_copy(b, &temp);
    ibz_copy(&(*vb)[0], &vtemp[0]);
    ibz_copy(&(*vb)[1], &vtemp[1]);
    ibz_copy(&(*vb)[2], &vtemp[2]);
    ibz_copy(&(*vb)[3], &vtemp[3]);

    ibz_finalize(&temp);
    ibz_vec_4_finalize(&vtemp);
}

int
partition(ibz_t arr[], ibz_vec_4_t varr[], int low, int high)
{
    ibz_t pivot;
    ibz_init(&pivot);
    ibz_copy(&pivot, &arr[high]);

    int i = low - 1;

    for (int j = low; j <= high - 1; j++) {
        if (ibz_cmp(&arr[j], &pivot) < 0) {
            i++;
            swap(&arr[i], &arr[j], &varr[i], &varr[j]);
        }
    }

    swap(&arr[i + 1], &arr[high], &varr[i + 1], &varr[high]);

    ibz_finalize(&pivot);
    return i + 1;
}

void
quicksort(ibz_t arr[], ibz_vec_4_t varr[], int low, int high)
{
    if (low < high) {
        int pi = partition(arr, varr, low, high);

        quicksort(arr, varr, low, pi - 1);
        quicksort(arr, varr, pi + 1, high);
    }
}

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
                     ibz_t *adjust,
                     int small)
{

    // var declaration
    int found;
    ibz_t two_pow, tmp;
    quat_alg_elem_t theta;
    ec_curve_t E0 = CURVE_E0;
    ec_curve_init(&E0);

    int length;
    int extra_info = 1;

    int u_bitsize = ibz_bitsize(u);

    if (!small) {
        length = TORSION_PLUS_EVEN_POWER - 2;
    } else {
        // TODO This is a constant that should be set-up at a cleaner place
        length = SQIsign2D_small_fixed_deg_exp;
        assert(u_bitsize < length);
        length = ibz_bitsize(&QUATALG_PINFTY.p) + 15 - ibz_bitsize(u);
    }

    // var init
    ibz_init(&two_pow);
    ibz_init(&tmp);
    quat_alg_elem_init(&theta);

    // TODO make a clean constant

    // TODO we could get rid of this
    ibz_set(adjust, 1);
    ibz_pow(&two_pow, &ibz_const_two, length);

    ibz_mul(&tmp, adjust, adjust);
    ibz_mul(&tmp, &tmp, u);

    assert(ibz_cmp(&two_pow, &tmp) > 0);
    assert(!ibz_is_even(&tmp));

    // computing the endomorphism theta of norm u * (2^(length) - adjust^2 * u)
    ibz_sub(&tmp, &two_pow, &tmp);
    ibz_mul(&tmp, &tmp, u);

    // TODO make this a clean constant
    found = 0;
    int count = 0;
    while (!found && count < 1) {
        found = represent_integer_non_diag(&theta, &tmp, &QUATALG_PINFTY);
        count++;
    }

    if (!found) {
        printf("represent integer failed for a target of size %d for a u of size %d with length = "
               "%d \n",
               ibz_bitsize(&tmp),
               ibz_bitsize(u),
               length);
        return 0;
    }
    quat_lideal_create_from_primitive(lideal, &theta, u, &MAXORD_O0, &QUATALG_PINFTY);

    ec_basis_t B0_two;
    // copying the basis
    copy_point(&B0_two.P, &BASIS_EVEN.P);
    copy_point(&B0_two.Q, &BASIS_EVEN.Q);
    copy_point(&B0_two.PmQ, &BASIS_EVEN.PmQ);

    ec_dbl_iter(&B0_two.P, TORSION_PLUS_EVEN_POWER - length - 2, &E0, &B0_two.P);
    ec_dbl_iter(&B0_two.Q, TORSION_PLUS_EVEN_POWER - length - 2, &E0, &B0_two.Q);
    ec_dbl_iter(&B0_two.PmQ, TORSION_PLUS_EVEN_POWER - length - 2, &E0, &B0_two.PmQ);

    // now we set-up the kernel
    theta_couple_curve_t E01;
    theta_couple_point_t T1;
    theta_couple_point_t T2, T1m2;
    E01.E1 = E0;
    E01.E2 = E0;

    copy_point(&T1.P1, &B0_two.P);
    copy_point(&T2.P1, &B0_two.Q);
    copy_point(&T1m2.P1, &B0_two.PmQ);

    assert(test_point_order_twof(&B0_two.P, &E0, length + 2));
    assert(test_point_order_twof(&B0_two.Q, &E0, length + 2));
    assert(test_point_order_twof(&B0_two.PmQ, &E0, length + 2));

    // multiplication of theta by (adjust*u)^-1 mod 2^(length+2)
    ibz_mul(&two_pow, &two_pow, &ibz_const_two);
    ibz_mul(&two_pow, &two_pow, &ibz_const_two);
    ibz_mul(&tmp, u, adjust);
    ibz_invmod(&tmp, &tmp, &two_pow);
    assert(!ibz_is_even(&tmp));
    ibz_mul(&theta.coord[0], &theta.coord[0], &tmp);
    ibz_mul(&theta.coord[1], &theta.coord[1], &tmp);
    ibz_mul(&theta.coord[2], &theta.coord[2], &tmp);
    ibz_mul(&theta.coord[3], &theta.coord[3], &tmp);

    // applying theta
    endomorphism_application_even_basis(&B0_two, &E0, &theta, length + 2);

    T1.P2 = B0_two.P;
    T2.P2 = B0_two.Q;
    T1m2.P2 = B0_two.PmQ;

    // ec_mul_ibz(&T1.P2,&E0,&tmp,&T1.P2);
    // ec_mul_ibz(&T2.P2,&E0,&tmp,&T2.P2);
    // ec_mul_ibz(&T1m2.P2,&E0,&tmp,&T1m2.P2);

    assert(test_point_order_twof(&B0_two.P, &E0, length + 2));
    assert(test_point_order_twof(&B0_two.Q, &E0, length + 2));
    assert(test_point_order_twof(&B0_two.PmQ, &E0, length + 2));

    assert(test_point_order_twof(&T1.P1, &E01.E1, length + 2));
    assert(test_point_order_twof(&T2.P2, &E01.E2, length + 2));

    theta_chain_comput_strategy(isog,
                                length,
                                &E01,
                                &T1,
                                &T2,
                                &T1m2,
                                strategies[TORSION_PLUS_EVEN_POWER - length],
                                extra_info);
#ifndef NDEBUG
    theta_chain_t second_isog;
    ec_dbl_iter(&T1.P1, 2, &E01.E1, &T1.P1);
    ec_dbl_iter(&T2.P1, 2, &E01.E1, &T2.P1);
    ec_dbl_iter(&T1m2.P1, 2, &E01.E1, &T1m2.P1);
    ec_dbl_iter(&T1.P2, 2, &E01.E2, &T1.P2);
    ec_dbl_iter(&T2.P2, 2, &E01.E2, &T2.P2);
    ec_dbl_iter(&T1m2.P2, 2, &E01.E2, &T1m2.P2);

    assert(test_point_order_twof(&T1.P1, &E01.E1, length));
    assert(test_point_order_twof(&T2.P2, &E01.E2, length));
    theta_chain_comput_strategy(&second_isog,
                                length,
                                &E01,
                                &T1,
                                &T2,
                                &T1m2,
                                strategies[TORSION_PLUS_EVEN_POWER - length + 2],
                                0);
#endif

    // if (!small) {
    //     // computing the isogeny

    // }
    // else {

    //     // need to adjust
    //     // t = tic();
    //     assert(TORSION_PLUS_EVEN_POWER - length >= 2);
    //     theta_chain_comput_strategy(isog,length,&E01,&T1,&T2,&T1m2,special_small_strategy,extra_info);
    //     // TOC(t,"theta chain inside fixed");
    // }

    // var finalize
    ibz_finalize(&two_pow);
    ibz_finalize(&tmp);
    quat_alg_elem_finalize(&theta);

    return 1;
}

/**
 * @brief Find good equivalent ideals
 *
 * @param u Output : integer
 * @param v Output : integer
 * @param beta1 Output : quaternion element
 * @param beta2 Output : quaternion element
 * @param d1 Output : integer
 * @param d2 Output : integer
 * @param number_sum_square : int
 * @param lideal : left quaternion ideal
 */
int
find_uv(ibz_t *u,
        ibz_t *v,
        ibz_vec_4_t *coeffs,
        quat_alg_elem_t *beta1,
        quat_alg_elem_t *beta2,
        ibz_t *d1,
        ibz_t *d2,
        const ibz_t *target,
        int number_sum_square,
        const quat_left_ideal_t *lideal,
        const quat_alg_t *Bpoo,
        int num_rerun)
{

    // variable declaration
    ibz_vec_4_t vec;
    ibz_t n;
    ibz_t au, bu, av, bv;
    ibz_init(&au);
    ibz_init(&bu);
    ibz_init(&av);
    ibz_init(&bv);

    ibz_t remain, adjusted_norm;
    ibz_mat_4x4_t gram, reduced;

    ibz_init(&n);
    // TODO this could be much simpler (use ibz_set_str for a start)
    int prime_list_length;
    ibz_t prod_bad_primes; // TODO make this a precomputation
    short prime_list[12] = { 2, 5, 13, 17, 29, 37, 41, 53, 61, 73, 89, 97 };
    prime_list_length = 12;
    ibz_init(&prod_bad_primes);
    ibz_copy(&prod_bad_primes, &ibz_const_one);
    ibz_set(&n, 140227657289781369);
    ibz_mul(&prod_bad_primes, &prod_bad_primes, &n);
    ibz_set(&n, 8695006970070847579);
    ibz_mul(&prod_bad_primes, &prod_bad_primes, &n);
    ibz_set(&n, 4359375434796427649);
    ibz_mul(&prod_bad_primes, &prod_bad_primes, &n);
    ibz_set(&n, 221191130330393351);
    ibz_mul(&prod_bad_primes, &prod_bad_primes, &n);
    ibz_set(&n, 1516192381681334191);
    ibz_mul(&prod_bad_primes, &prod_bad_primes, &n);
    ibz_set(&n, 5474546011261709671);
    ibz_mul(&prod_bad_primes, &prod_bad_primes, &n);

    ibz_mat_4x4_init(&gram);
    ibz_mat_4x4_init(&reduced);

    ibz_vec_4_init(&vec);

    ibz_init(&remain);
    ibz_init(&adjusted_norm);

    // multiplying the norm by the denominator squared
    ibz_set(&adjusted_norm, 1);
    ibz_mul(&adjusted_norm, &adjusted_norm, &lideal->lattice.denom);
    ibz_mul(&adjusted_norm, &adjusted_norm, &lideal->lattice.denom);

    // computing a reduced basis
    clock_t t = tic();
    quat_lideal_reduce_basis(&reduced, &gram, lideal, Bpoo);
    // printf("%d-bit norm",ibz_bitsize(&lideal->norm));
    // TOC_clock(t,"\nbasis reduction");

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            ibz_div(&gram[i][j], &remain, &gram[i][j], &lideal->norm);
            assert(ibz_is_zero(&remain));
        }
    }

    // reordering the basis if needed
    if (ibz_cmp(&gram[0][0], &gram[2][2]) == 0) {
        for (int i = 0; i < 4; i++) {
            ibz_swap(&reduced[i][1], &reduced[i][2]);
        }
        ibz_swap(&gram[0][2], &gram[0][1]);
        ibz_swap(&gram[2][0], &gram[1][0]);
        ibz_swap(&gram[3][2], &gram[3][1]);
        ibz_swap(&gram[2][3], &gram[1][3]);
        ibz_swap(&gram[2][2], &gram[1][1]);
    } else if (ibz_cmp(&gram[0][0], &gram[3][3]) == 0) {
        for (int i = 0; i < 4; i++) {
            ibz_swap(&reduced[i][1], &reduced[i][3]);
        }
        ibz_swap(&gram[0][3], &gram[0][1]);
        ibz_swap(&gram[3][0], &gram[1][0]);
        ibz_swap(&gram[2][3], &gram[2][1]);
        ibz_swap(&gram[3][2], &gram[1][2]);
        ibz_swap(&gram[3][3], &gram[1][1]);
    } else if (ibz_cmp(&gram[1][1], &gram[3][3]) == 0) {
        // in this case it seems that we need to swap the second and third element, and then
        // recompute entirely the second element from the first printf("third case \n");
        // ibz_printf("%Zd %Zd %Zd %Zd \n",gram[0][0],gram[1][1],gram[2][2],gram[3][3]);

        // first we swap the second and third element
        for (int i = 0; i < 4; i++) {
            ibz_swap(&reduced[i][1], &reduced[i][2]);
        }
        ibz_swap(&gram[0][2], &gram[0][1]);
        ibz_swap(&gram[2][0], &gram[1][0]);
        ibz_swap(&gram[3][2], &gram[3][1]);
        ibz_swap(&gram[2][3], &gram[1][3]);
        ibz_swap(&gram[2][2], &gram[1][1]);
        // and now we compute
        // assert(0);
    }

    // if (ibz_cmp(&gram[0][0],&gram[1][1])!=0 || ibz_cmp(&gram[2][2],&gram[3][3])!=0) {
    // ibz_printf("%Zd %Zd %Zd %Zd \n",gram[0][0],gram[1][1],gram[2][2],gram[3][3]);
    // }

    // TODO : sometimes this fail, needs to be fixed
    // assert(ibz_cmp(&gram[0][0],&gram[1][1])==0);
    // assert(ibz_cmp(&gram[2][2],&gram[3][3])==0);
    // TODO ensure this
    // assert(ibz_cmp(&gram[0][0],&gram[2][2])<0);

    // adjusting the sign if needed
    if (ibz_cmp(&reduced[0][0], &reduced[1][1]) != 0) {
        for (int i = 0; i < 4; i++) {
            ibz_neg(&reduced[i][1], &reduced[i][1]);
            ibz_neg(&gram[i][1], &gram[i][1]);
            ibz_neg(&gram[1][i], &gram[1][i]);
        }
        // assert(ibz_cmp(&reduced[0][0],&reduced[1][1])==0);
    }
    if (ibz_cmp(&reduced[0][2], &reduced[1][3]) != 0) {
        for (int i = 0; i < 4; i++) {
            ibz_neg(&reduced[i][3], &reduced[i][3]);
            ibz_neg(&gram[i][3], &gram[i][3]);
            ibz_neg(&gram[3][i], &gram[3][i]);
        }
        // assert(ibz_cmp(&reduced[0][2],&reduced[1][3])==0);
    }

    // printing the size of the elements
    // printf("p : %d n : %d adj_n : %d
    // \n",ibz_bitsize(&QUATALG_PINFTY.p),ibz_bitsize(&lideal->norm),ibz_bitsize(&adjusted_norm));
    // printf("n1 : %d n2 : %d n3 :  %d n4 : %d
    // \n",ibz_bitsize(&gram[0][0]),ibz_bitsize(&gram[1][1])-ibz_bitsize(&adjusted_norm),ibz_bitsize(&gram[2][2])-ibz_bitsize(&adjusted_norm),ibz_bitsize(&gram[3][3])-ibz_bitsize(&adjusted_norm));

    // we start by enumerating a set of small vectors
    // global parameters for the enumerate
    // TODO may be overshot and we may improve this by sampling inside an hyperball instead
    // TODO make this a proper constant
    int m = 1 + (ibz_bitsize(&QUATALG_PINFTY.p) / 120) + 2 * number_sum_square + 2 * num_rerun;
    int m4 = (2 * m + 1) * (2 * m + 1) * (2 * m + 1) * (2 * m + 1) - 1;
    int m3 = (2 * m + 1) * (2 * m + 1) * (2 * m + 1);
    int m2 = (2 * m + 1) * (2 * m + 1);
    int m1 = 2 * m + 1;
    m4 = m4 / 4;
    ibz_vec_4_t small_vecs[m4];
    ibz_t small_norms[m4];
    for (int i = 0; i < m4; i++) {
        ibz_init(&small_norms[i]);
        ibz_vec_4_init(&small_vecs[i]);
    }

    t = tic();
    int index = 0;
    for (int i1 = -m; i1 < m; i1++) {
        for (int i2 = -m; i2 < m + 1; i2++) {
            for (int i3 = -m; i3 < m + 1; i3++) {
                for (int i4 = -m; i4 < m + 1; i4++) {
                    // we ensure that we don't record the same norm in the list
                    if (!(i1 > 0) && !(i1 == 0 && i2 > 0) && !(i1 == 0 && i2 == 0 && i3 > 0) &&
                        !(i1 == 0 && i2 == 0 & i3 == 0 && i4 >= 0)
                        // &&!(i1>i2 && i2>-i1 && i3>i4 && i4>-i3)&&!(i1>-i2 && i2>i1 && i3>-i4 &&
                        // i4>i3)
                        && !((m + i4) + m1 * (m + i3) + m2 * (m + i2) + m3 * (m + i1) >
                             (m - i3) + m1 * (m + i4) + m2 * (m - i1) + m3 * (m + i2)) &&
                        !((m + i4) + m1 * (m + i3) + m2 * (m + i2) + m3 * (m + i1) >
                          (m + i3) + m1 * (m - i4) + m2 * (m + i1) + m3 * (m - i2)) &&
                        !(i1 % 2 == 0 && i2 % 2 == 0 && i3 % 2 == 0 && i4 % 2 == 0) &&
                        !(i1 % 3 == 0 && i2 % 3 == 0 && i3 % 3 == 0 && i4 % 3 == 0)) {

                        ibz_set(&vec[0], i1);
                        ibz_set(&vec[1], i2);
                        ibz_set(&vec[2], i3);
                        ibz_set(&vec[3], i4);
                        quat_qf_eval(&n, &gram, &vec);
                        ibz_div(&n, &remain, &n, &adjusted_norm);
                        // TODECIDE : do we use only odd norms ?
                        if (ibz_mod_ui(&n, 2) == 1) {
                            ibz_set(&small_vecs[index][0], i1);
                            ibz_set(&small_vecs[index][1], i2);
                            ibz_set(&small_vecs[index][2], i3);
                            ibz_set(&small_vecs[index][3], i4);
                            ibz_copy(&small_norms[index], &n);
                            index++;
                        }
                    }
                }
            }
        }
    }
    index--;
    // printf("number of elements = %d / %d (value of m=%d) \n",index,m4,m);
    // TOC(t,"\nenum");

    // sorting the list
    quicksort(small_norms, small_vecs, 0, index);
    // TOC(t,"sorting + enum");

    int found = 0;
    int count = 0;
    // starting to try solutions

    // TODO try to go through couples (d1,d2) by increasing size of the products d1*d2

    // this was a failed attempt to speed-up this algorithm.
    // since we set it to one, we could simply remove it
    int overstretch = 1;

    // precomputing a list of small_norms[i]/target
    ibz_t quotients[index][overstretch];
    for (int i = 0; i < index; i++) {
        for (int j = 0; j < overstretch; j++) {
            ibz_init(&quotients[i][j]);
        }
    }

    ibz_copy(&n, target);
    for (int i = 0; i < index; i++) {
        ibz_div(&quotients[i][0], &remain, &n, &small_norms[i]);
        for (int j = 1; j < overstretch; j++) {
            ibz_mul(&quotients[i][j], &quotients[i][j - 1], &ibz_const_two);
        }
    }

    int cmp;

    for (int i1 = 0; i1 < index; i1++) {
        ibz_mod(&adjusted_norm, &n, &small_norms[i1]);
        for (int i2 = i1; i2 < index; i2++) {
            if (ibz_is_even(&small_norms[i1]) && ibz_is_even(&small_norms[i2])) {
                break;
            }
            // u = target / d1 mod d2
            // TODO we could use batched inversion to speed-up this part
            if (!ibz_invmod(&remain, &small_norms[i2], &small_norms[i1])) {
                continue;
            }
            ibz_mul(v, &remain, &adjusted_norm);
            for (int i3 = 0; i3 < overstretch; i3++) {
                if (i3 > 0) {
                    ibz_mul(v, v, &ibz_const_two);
                }
                ibz_mod(v, v, &small_norms[i1]);
                cmp = ibz_cmp(v, &quotients[i2][i3]);
                while (!found && cmp < 0) {
                    int size = ibz_bitsize(v);
                    if (number_sum_square > 0) {
                        found = ibz_cornacchia_extended(&av,
                                                        &bv,
                                                        v,
                                                        prime_list,
                                                        prime_list_length,
                                                        KLPT_primality_num_iter,
                                                        &prod_bad_primes);
                    } else if (number_sum_square == 0) {
                        found = 1;
                    }

                    if (found) {
                        ibz_mul(&remain, v, &small_norms[i2]);
                        ibz_pow(&au, &ibz_const_two, i3);
                        ibz_mul(&au, &au, &n);
                        ibz_sub(u, &au, &remain);
                        assert(ibz_cmp(u, &ibz_const_zero) > 0);
                        ibz_div(u, &remain, u, &small_norms[i1]);
                        assert(ibz_is_zero(&remain));
                        if (number_sum_square == 2) {
                            found = ibz_cornacchia_extended(&au,
                                                            &bu,
                                                            u,
                                                            prime_list,
                                                            prime_list_length,
                                                            KLPT_primality_num_iter,
                                                            &prod_bad_primes);
                        }
                    }
                    if (!found) {
                        ibz_add(v, v, &small_norms[i1]);
                        cmp = ibz_cmp(v, &quotients[i2][i3]);
                    }
                }
                if (found) {

                    // recording the solution that we found
                    ibz_copy(&beta1->denom, &lideal->lattice.denom);
                    ibz_copy(&beta2->denom, &lideal->lattice.denom);
                    ibz_copy(d1, &small_norms[i1]);
                    ibz_copy(d2, &small_norms[i2]);
                    ibz_mat_4x4_eval(&beta1->coord, &reduced, &small_vecs[i1]);
                    ibz_mat_4x4_eval(&beta2->coord, &reduced, &small_vecs[i2]);
#ifndef NDEBUG
                    ibq_t norm;
                    ibq_init(&norm);
                    quat_alg_norm(&norm, beta1, &QUATALG_PINFTY);
                    ibq_to_ibz(&remain, &norm);
                    ibz_mul(&n, d1, &lideal->norm);
                    assert(ibz_cmp(&n, &remain) == 0);
                    quat_alg_norm(&norm, beta2, &QUATALG_PINFTY);
                    ibq_to_ibz(&remain, &norm);
                    ibz_mul(&n, d2, &lideal->norm);
                    assert(ibz_cmp(&n, &remain) == 0);
                    ibq_finalize(&norm);

                    // testing the values of coeffs
                    if (number_sum_square == 2) {
                        ibz_mul(&n, &au, &au);
                        ibz_mul(&remain, &bu, &bu);
                        ibz_add(&n, &n, &remain);
                        assert(ibz_cmp(&n, u) == 0);
                        ibz_mul(&n, &av, &av);
                        ibz_mul(&remain, &bv, &bv);
                        ibz_add(&n, &n, &remain);
                        assert(ibz_cmp(&n, v) == 0);
                    }
#endif
                    if (number_sum_square > 0) {
                        ibz_copy(&((*coeffs)[0]), &au);
                        ibz_copy(&((*coeffs)[1]), &bu);
                        ibz_copy(&((*coeffs)[2]), &av);
                        ibz_copy(&((*coeffs)[3]), &bv);
                    }

                    // printf("Found after %d attempts and %d deg one hit out of %d
                    // \n",i1*index*overstretch + overstretch*i2 +
                    // i3,count,overstretch*index*(index-1)/2);
                    break;
                }
            }

            if (found) {
                break;
            }
        }
        if (found) {
            break;
        }
    }
    // printf("Number of missed attemtps by considering only 2^248 : %d \n ",cnt_missed);
    // printf("Number of missed attempts by not removing power of 2 from v : %d
    // \n",second_cnt_missed); clock_print(tot_cor,"cornacchias :"); clock_print(tot_ops,"all other
    // ops :"); clock_print(tot_inv,"among which modular inversions :"); clock_print(tot_spec,"among
    // which cmp");
    // // TOC(t,"total time searching for u,v");
    // clock_to_time(dclock(t)-(tot_ops+tot_cor),"difference between total and recorded ");

    // var finalize
    ibz_mat_4x4_finalize(&gram);
    ibz_mat_4x4_finalize(&reduced);

    ibz_finalize(&n);
    ibz_vec_4_finalize(&vec);
    ibz_finalize(&au);
    ibz_finalize(&bu);
    ibz_finalize(&av);
    ibz_finalize(&bv);
    ibz_finalize(&remain);
    ibz_finalize(&adjusted_norm);
    for (int i = 0; i < index; i++) {
        for (int j = 0; j < overstretch; j++) {
            ibz_finalize(&quotients[i][j]);
        }
    }
    for (int i = 0; i < m4; i++) {

        ibz_finalize(&small_norms[i]);
        ibz_vec_4_finalize(&small_vecs[i]);
    }

    return found;
}

/**
 * @brief Translating an ideal into a representation of the corresponding isogeny
 *
 * @param isog Output : dim 2 isogeny
 * @param beta1 Output : quaternion element
 * @param beta2 Output : quaternion element
 * @param u Output : integer
 * @param v Output : integer
 * @param phiv Output : dim2 isogeny representation of an isogeny of degree v
 * @param d1 Output : integer
 * @param d2 Output : integer
 * @param lideal : ideal
 * @param Bpoo : the quaternion algebra
 * @returns a bit indicating if the computation succeeded
 *
 * beta1 and beta2 are elements in lideal of norm n(lideal)d1 and n(lideal) d2 respectively
 * u,v are integers such that 2^e = d1 u + d2 v and u = au^2 + bu^2
 * phiv is a dim 2 isogeny representing an isogeny of degree v : E0 -> Ev
 * F is a dim2 2^e - isogeny between E0 x Ev -> E_I x E
 * that encodes an isogeny E0 -> E_I corresponding to the ideal lideal
 */
int
dim2id2iso_ideal_to_isogeny_clapotis(theta_chain_t *isog,
                                     quat_alg_elem_t *beta1,
                                     quat_alg_elem_t *beta2,
                                     ibz_t *u,
                                     ibz_t *v,
                                     ibz_vec_4_t *coeffs,
                                     theta_chain_t *phiu,
                                     theta_chain_t *phiv,
                                     ibz_t *d1,
                                     ibz_t *d2,
                                     ec_curve_t *codomain,
                                     ec_basis_t *basis,
                                     const quat_left_ideal_t *lideal,
                                     const quat_alg_t *Bpoo)
{

    ibz_t target, tmp, two_pow, adjust_u, adjust_v;
    quat_alg_elem_t theta;
    quat_alg_elem_t quat_tmp;
    quat_alg_elem_t quat_gcd_remove;

    ibq_t norm;
    ibq_init(&norm);
    ibz_t test1, test2;
    ibz_init(&test1);
    ibz_init(&test2);
    ibz_init(&adjust_u);
    ibz_init(&adjust_v);

    ibz_init(&target);
    ibz_init(&tmp);
    ibz_init(&two_pow);
    int exp = TORSION_PLUS_EVEN_POWER;
    quat_alg_elem_init(&theta);
    quat_alg_elem_init(&quat_tmp);
    quat_alg_elem_init(&quat_gcd_remove);

    // TODO set-up this parameter somewhere clean
    // TODECIDE the final value of this parameter
    int number_sum_square = 0;

    // clock_t t = tic();
    // first, we find u,v,d1,d2,beta1,beta2
    // such that u*d1 + v*d2 = 2^TORSION_PLUS_EVEN_POWER and there are ideals of norm d1,d2
    // equivalent to ideal beta1 and beta2 are elements of norm nd1, nd2 where n=n(lideal)
    int found = 0;
    int num_iter_find_uv = 0;
    while (!found & (num_iter_find_uv < 3)) {
        found = find_uv(u,
                        v,
                        coeffs,
                        beta1,
                        beta2,
                        d1,
                        d2,
                        &TORSION_PLUS_2POWER,
                        number_sum_square,
                        lideal,
                        Bpoo,
                        num_iter_find_uv);
        num_iter_find_uv++;
    }
    // TOC(t,"\n \ntotal time to find u,v");

    if (!found) {
        printf("didn't find uv \n");
        return 0;
    }

    assert(ibz_get(d1) % 2 == 1 && ibz_get(d2) % 2 == 1);
    // compute the valuation of the GCD of u,v
    ibz_gcd(&tmp, u, v);
    assert(ibz_get(&tmp) != 0);
    int exp_gcd = two_adic_valuation(ibz_get(&tmp));
    exp = TORSION_PLUS_EVEN_POWER - exp_gcd;
    // removing the power of 2 from u and v
    ibz_div(u, &test1, u, &tmp);
    assert(ibz_cmp(&test1, &ibz_const_zero) == 0);
    ibz_div(v, &test1, v, &tmp);
    assert(ibz_cmp(&test1, &ibz_const_zero) == 0);

#ifndef NDEBUG
    // checking that u+v =2^exp
    ibz_t pow_check, tmp_check;
    ibz_init(&pow_check);
    ibz_init(&tmp_check);
    ibz_pow(&pow_check, &ibz_const_two, exp);
    ibz_mul(&tmp_check, d1, u);
    ibz_sub(&pow_check, &pow_check, &tmp_check);
    ibz_mul(&tmp_check, v, d2);
    ibz_sub(&pow_check, &pow_check, &tmp_check);
    assert(ibz_cmp(&pow_check, &ibz_const_zero) == 0);
    ibz_finalize(&tmp_check);
    ibz_finalize(&pow_check);
#endif

    // setting-up the element to remove the power of two from phiu, phiv
    // quat_gcd_remove = (i+1)^(exp_gcd)
    ibz_set(&quat_gcd_remove.denom, 1);
    ibz_set(&quat_tmp.denom, 1);
    if (exp_gcd > 0) {
        ibz_set(&quat_tmp.coord[1], 1);
    } else {
        ibz_set(&quat_tmp.coord[1], 0);
    }
    ibz_set(&quat_tmp.coord[0], 1);
    ibz_set(&quat_tmp.coord[2], 0);
    ibz_set(&quat_tmp.coord[3], 0);
    ibz_set(&quat_gcd_remove.coord[0], 1);
    ibz_set(&quat_gcd_remove.coord[1], 0);
    ibz_set(&quat_gcd_remove.coord[2], 0);
    ibz_set(&quat_gcd_remove.coord[3], 0);
    for (int i = 0; i < exp_gcd; i++) {
        quat_alg_mul(&quat_gcd_remove, &quat_gcd_remove, &quat_tmp, &QUATALG_PINFTY);
    }
    ibz_pow(&quat_gcd_remove.denom, &ibz_const_two, exp_gcd);

#ifndef NDEBUG
    quat_alg_norm(&norm, &quat_gcd_remove, &QUATALG_PINFTY);
    ibq_denom(&test1, &norm);
    assert(ibz_cmp(&test1, &quat_gcd_remove.denom) == 0);
#endif

    // now we compute the dimension 2 isogeny
    // F : Eu x Ev -> E x E'
    // where we have phi_u : Eu -> E0 and phi_v : Ev -> E0
    // if we have phi1 : E0 -> E of degree d1
    // and phi2 : E0 -> E of degree d2
    // we can define theta = phi2 o hat{phi1}
    // and the kernel of F is given by
    // ( [ud1](P), phiv o theta o hat{phiu} (P)),( [ud1](Q), phiv o theta o hat{phiu} (Q)) where P,Q
    // is a basis of E0[2e]

    // now we set-up the kernel
    ec_curve_t E0 = CURVE_E0;
    ec_curve_init(&E0);

    ec_basis_t bas;
    theta_couple_curve_t E01;
    theta_couple_point_t T1;
    theta_couple_point_t T2, T1m2;

    ec_basis_t bas_u;

    copy_point(&bas.P, &BASIS_EVEN.P);
    copy_point(&bas.Q, &BASIS_EVEN.Q);
    copy_point(&bas.PmQ, &BASIS_EVEN.PmQ);

    // we start by computing theta = beta2 \hat{beta1}/n
    ibz_set(&theta.denom, 1);
    quat_alg_conj(&theta, beta1);
    quat_alg_mul(&theta, beta2, &theta, &QUATALG_PINFTY);
    ibz_mul(&theta.denom, &theta.denom, &lideal->norm);

    // now we perform the actual computation
    if (number_sum_square == 0) {
        quat_left_ideal_t idealu, idealv;
        quat_left_ideal_init(&idealu);
        quat_left_ideal_init(&idealv);
        theta_chain_t Fu, Fv;
        theta_couple_point_t V1, V2, V1m2;
        theta_couple_curve_t E00;
        E00.E1 = E0;
        E00.E2 = E0;

        // we perform the computation of phiu with a fixed degree isogeny
        // t = tic();
        fixed_degree_isogeny(&Fu, &idealu, u, &adjust_u, 1);
        // TOC(t,"1st fixed deg");
        // pushing the torsion points through Fu
        // first we lift the basis
        theta_couple_point_t P, Q, PmQ;

        copy_point(&P.P1, &bas.P);
        copy_point(&PmQ.P1, &bas.PmQ);
        copy_point(&Q.P1, &bas.Q);
        ec_set_zero(&P.P2);
        ec_set_zero(&Q.P2);
        ec_set_zero(&PmQ.P2);
        theta_chain_eval_special_case(&V1, &Fu, &P, &E00);
        theta_chain_eval_special_case(&V2, &Fu, &Q, &E00);
        theta_chain_eval_special_case(&V1m2, &Fu, &PmQ, &E00);

        assert(test_point_order_twof(&V1.P1, &Fu.codomain.E1, TORSION_PLUS_EVEN_POWER));
        assert(test_point_order_twof(&V1.P2, &Fu.codomain.E2, TORSION_PLUS_EVEN_POWER));

#ifndef NDEBUG
        // presumably the correct curve is the the second one, we check this
        ibz_t two_pow;
        ibz_init(&two_pow);
        fp2_t w0, w1, w2;
        ec_point_t AC, A24;
        copy_point(&AC, &CURVE_E0_A24); // Warning, this is AC, not A24!
        A24_from_AC(&A24, &AC);
        weil(&w0, TORSION_PLUS_EVEN_POWER, &bas.P, &bas.Q, &bas.PmQ, &A24);
        // Changing the AC
        fp2_copy(&AC.x, &Fu.codomain.E1.A);
        fp2_copy(&AC.z, &Fu.codomain.E1.C);
        A24_from_AC(&A24, &AC);
        weil(&w1, TORSION_PLUS_EVEN_POWER, &V1.P1, &V2.P1, &V1m2.P1, &A24);
        fp2_copy(&AC.x, &Fu.codomain.E2.A);
        fp2_copy(&AC.z, &Fu.codomain.E2.C);
        A24_from_AC(&A24, &AC);
        weil(&w2, TORSION_PLUS_EVEN_POWER, &V1.P2, &V2.P2, &V1m2.P2, &A24);
        ibz_pow(&two_pow, &ibz_const_two, Fu.length);
        ibz_mul(&tmp, &adjust_u, &adjust_u);
        ibz_mul(&tmp, &tmp, u);
        ibz_sub(&two_pow, &two_pow, &tmp);
        // now we are checking that one of the two is equal to the correct value
        digit_t digit_u[NWORDS_ORDER] = { 0 };
        ibz_to_digit_array(digit_u, &tmp);
        fp2_t test_pow;
        fp2_pow_vartime(&test_pow, &w0, digit_u, NWORDS_ORDER);

        assert(fp2_is_equal(&test_pow, &w2));
        ibz_to_digit_array(digit_u, &two_pow);
        fp2_pow_vartime(&test_pow, &w0, digit_u, NWORDS_ORDER);
        assert(fp2_is_equal(&test_pow, &w1));
#endif

        // copying the basis images
        copy_point(&bas_u.P, &V1.P2);
        copy_point(&bas_u.Q, &V2.P2);
        copy_point(&bas_u.PmQ, &V1m2.P2);

        // copying the points to the first part of the kernel
        copy_point(&T1.P1, &V1.P2);
        copy_point(&T2.P1, &V2.P2);
        copy_point(&T1m2.P1, &V1m2.P2);
        fp2_copy(&E01.E1.A, &Fu.codomain.E2.A);
        fp2_copy(&E01.E1.C, &Fu.codomain.E2.C);

        // computation of phiv
        // t = tic();
        int bv = fixed_degree_isogeny(&Fv, &idealv, v, &adjust_v, 1);
        assert(bv);
        // TOC(t,"2nd fixed deg");

        // pushing the torsion points through Fv
        theta_chain_eval_special_case(&V1, &Fv, &P, &E00);
        theta_chain_eval_special_case(&V2, &Fv, &Q, &E00);
        theta_chain_eval_special_case(&V1m2, &Fv, &PmQ, &E00);

        assert(test_point_order_twof(&V1.P1, &Fv.codomain.E1, TORSION_PLUS_EVEN_POWER));
        assert(test_point_order_twof(&V1.P2, &Fv.codomain.E2, TORSION_PLUS_EVEN_POWER));

#ifndef NDEBUG
        // presumably the correct curve is the the second one, we check this
        // Changing the AC
        fp2_copy(&AC.x, &Fv.codomain.E1.A);
        fp2_copy(&AC.z, &Fv.codomain.E1.C);
        A24_from_AC(&A24, &AC);
        weil(&w1, TORSION_PLUS_EVEN_POWER, &V1.P1, &V2.P1, &V1m2.P1, &A24);
        fp2_copy(&AC.x, &Fv.codomain.E2.A);
        fp2_copy(&AC.z, &Fv.codomain.E2.C);
        A24_from_AC(&A24, &AC);
        weil(&w2, TORSION_PLUS_EVEN_POWER, &V1.P2, &V2.P2, &V1m2.P2, &A24);
        ibz_mul(&tmp, &adjust_v, &adjust_v);
        ibz_mul(&tmp, &tmp, v);
        ibz_pow(&two_pow, &ibz_const_two, Fv.length);
        ibz_sub(&two_pow, &two_pow, &tmp);
        // now we are checking that one of the two is equal to the correct value
        ibz_to_digit_array(digit_u, &tmp);
        fp2_pow_vartime(&test_pow, &w0, digit_u, NWORDS_ORDER);
        if (!fp2_is_equal(&test_pow, &w2)) {
            assert(fp2_is_equal(&test_pow, &w1));
        }
        ibz_to_digit_array(digit_u, &two_pow);
        fp2_pow_vartime(&test_pow, &w0, digit_u, NWORDS_ORDER);
        assert(fp2_is_equal(&test_pow, &w1));
#endif

        copy_point(&bas.P, &V1.P2);
        copy_point(&bas.Q, &V2.P2);
        copy_point(&bas.PmQ, &V1m2.P2);

        // multiplying theta by adjust_u / (d1 * adjust_v)
        ibz_pow(&two_pow, &ibz_const_two, TORSION_PLUS_EVEN_POWER);
        ibz_mul(&tmp, d1, &adjust_v);
        ibz_invmod(&tmp, &tmp, &two_pow);
        ibz_mul(&tmp, &tmp, &adjust_u);
        ibz_mul(&theta.coord[0], &theta.coord[0], &tmp);
        ibz_mul(&theta.coord[1], &theta.coord[1], &tmp);
        ibz_mul(&theta.coord[2], &theta.coord[2], &tmp);
        ibz_mul(&theta.coord[3], &theta.coord[3], &tmp);
        // applying theta
        endomorphism_application_even_basis(&bas, &Fv.codomain.E2, &theta, TORSION_PLUS_EVEN_POWER);

        // copying points to the second part of the kernel
        copy_point(&T1.P2, &bas.P);
        copy_point(&T2.P2, &bas.Q);
        copy_point(&T1m2.P2, &bas.PmQ);

        // copying the points to the first part of the kernel
        fp2_copy(&E01.E2.A, &Fv.codomain.E2.A);
        fp2_copy(&E01.E2.C, &Fv.codomain.E2.C);

        quat_left_ideal_finalize(&idealu);
        quat_left_ideal_finalize(&idealv);
    } else if (number_sum_square == 1) {
        // for this one v is a sum of squares while u is not so we will need to compute phiu with
        // fixed degree isogeny
        quat_left_ideal_t idealu;
        quat_left_ideal_init(&idealu);
        theta_chain_t Fu;
        theta_couple_point_t V1, V2, V1m2;
        theta_couple_curve_t E00;
        E00.E1 = E0;
        E00.E2 = E0;

        // we perform the computation of phiu with a fixed degree isogeny
        fixed_degree_isogeny(&Fu, &idealu, u, &adjust_u, 1);
        ibz_set(&adjust_v, 1);

        // pushing the torsion points through Fu

        theta_couple_point_t P, Q, PmQ;
        copy_point(&P.P1, &bas.P);
        copy_point(&PmQ.P1, &bas.PmQ);
        copy_point(&Q.P1, &bas.Q);
        ec_set_zero(&P.P2);
        ec_set_zero(&Q.P2);
        ec_set_zero(&PmQ.P2);
        theta_chain_eval_special_case(&V1, &Fu, &P, &E00);
        theta_chain_eval_special_case(&V2, &Fu, &Q, &E00);
        theta_chain_eval_special_case(&V1m2, &Fu, &PmQ, &E00);

        assert(test_point_order_twof(&V1.P1, &Fu.codomain.E1, TORSION_PLUS_EVEN_POWER));
        assert(test_point_order_twof(&V1.P2, &Fu.codomain.E2, TORSION_PLUS_EVEN_POWER));

#ifndef NDEBUG
        // presumably the correct curve is the the second one, we check this
        ibz_t two_pow;
        ibz_init(&two_pow);
        fp2_t w0, w1, w2;
        ec_point_t AC, A24;
        copy_point(&AC, &CURVE_E0_A24); // Warning, this is AC, not A24!
        A24_from_AC(&A24, &AC);
        weil(&w0, TORSION_PLUS_EVEN_POWER, &bas.P, &bas.Q, &bas.PmQ, &A24);
        // Changing the AC
        fp2_copy(&AC.x, &Fu.codomain.E1.A);
        fp2_copy(&AC.z, &Fu.codomain.E1.C);
        A24_from_AC(&A24, &AC);
        weil(&w1, TORSION_PLUS_EVEN_POWER, &V1.P1, &V2.P1, &V1m2.P1, &A24);
        fp2_copy(&AC.x, &Fu.codomain.E2.A);
        fp2_copy(&AC.z, &Fu.codomain.E2.C);
        A24_from_AC(&A24, &AC);
        weil(&w2, TORSION_PLUS_EVEN_POWER, &V1.P2, &V2.P2, &V1m2.P2, &A24);
        ibz_mul(&tmp, &adjust_u, &adjust_u);
        ibz_mul(&tmp, &tmp, u);
        ibz_pow(&two_pow, &ibz_const_two, Fu.length);
        ibz_sub(&two_pow, &two_pow, &tmp);
        ;
        // now we are checking that one of the two is equal to the correct value
        digit_t digit_u[NWORDS_ORDER] = { 0 };
        ibz_to_digit_array(digit_u, &tmp);
        fp2_t test_pow;
        fp2_pow_vartime(&test_pow, &w0, digit_u, NWORDS_ORDER);

        assert(fp2_is_equal(&test_pow, &w2));
        ibz_to_digit_array(digit_u, &two_pow);
        fp2_pow_vartime(&test_pow, &w0, digit_u, NWORDS_ORDER);
        assert(fp2_is_equal(&test_pow, &w1));
#endif

        // copying the points to the first part of the kernel
        copy_point(&T1.P1, &V1.P2);
        copy_point(&T2.P1, &V2.P2);
        copy_point(&T1m2.P1, &V1m2.P2);
        fp2_copy(&E01.E1.A, &Fu.codomain.E2.A);
        fp2_copy(&E01.E1.C, &Fu.codomain.E2.C);

        // multiplying by d1 /adjust_u
        digit_t digit_d1[4] = { 0 };
        ibz_mul(&tmp, d1, &adjust_u);
        ibz_mod(&tmp, &tmp, &TORSION_PLUS_2POWER);
        ibz_to_digit_array(digit_d1, &tmp);
        ec_mul(&T1.P1, &E01.E1, digit_d1, &T1.P1);
        ec_mul(&T2.P1, &E01.E1, digit_d1, &T2.P1);
        ec_mul(&T1m2.P1, &E01.E1, digit_d1, &T1m2.P1);

        // now we deal with the second part of the kernel
        // phiv
        ibz_set(&quat_tmp.denom, 1);
        ibz_copy(&quat_tmp.coord[0], &((*coeffs)[2]));
        ibz_copy(&quat_tmp.coord[1], &((*coeffs)[3]));
        ibz_set(&quat_tmp.coord[2], 0);
        ibz_set(&quat_tmp.coord[3], 0);
        quat_alg_mul(&quat_tmp, &quat_tmp, &quat_gcd_remove, &QUATALG_PINFTY);

        // theta <- theta * phiv
        quat_alg_mul(&theta, &theta, &quat_tmp, &QUATALG_PINFTY);
        quat_alg_normalize(&theta);

        // applying theta
        endomorphism_application_even_basis(&bas, &E0, &theta, TORSION_PLUS_EVEN_POWER);

        // copying points to the second part of the kernel
        copy_point(&T1.P2, &bas.P);
        copy_point(&T2.P2, &bas.Q);
        copy_point(&T1m2.P2, &bas.PmQ);

        // copying the points to the first part of the kernel
        fp2_copy(&E01.E2.A, &E0.A);
        fp2_copy(&E01.E2.C, &E0.C);

        quat_left_ideal_finalize(&idealu);
    } else {
        assert(number_sum_square == 2);
        E01.E1 = E0;
        E01.E2 = E0;
        // in this case we have phiu = coeffs[0] + i coeffs[1] and phiv = coeffs[2] + i*coeffs[3]

#ifndef NDEBUG

        quat_alg_norm(&norm, &theta, &QUATALG_PINFTY);
        ibq_to_ibz(&test1, &norm);
        ibz_mul(&test2, d1, d2);
        assert(ibz_cmp(&test1, &test2) == 0);

#endif

        // multiplication by u*d1
        // we mught have a more efficient way to do this
        ibz_mul(&target, u, d1);
        ec_biscalar_mul_ibz(&T1.P1, &E0, &target, &ibz_const_zero, &bas, TORSION_PLUS_EVEN_POWER);
        ec_biscalar_mul_ibz(&T2.P1, &E0, &ibz_const_zero, &target, &bas, TORSION_PLUS_EVEN_POWER);
        ibz_neg(&tmp, &target);
        ibz_pow(&two_pow, &ibz_const_two, exp);
        ibz_mod(&tmp, &tmp, &two_pow);
        ec_biscalar_mul_ibz(&T1m2.P1, &E0, &target, &tmp, &bas, TORSION_PLUS_EVEN_POWER);

        // phiu
        ibz_set(&quat_tmp.denom, 1);
        ibz_copy(&quat_tmp.coord[0], &((*coeffs)[0]));
        ibz_copy(&quat_tmp.coord[1], &((*coeffs)[1]));
        ibz_set(&quat_tmp.coord[2], 0);
        ibz_set(&quat_tmp.coord[3], 0);
        quat_alg_mul(&quat_tmp, &quat_tmp, &quat_gcd_remove, &QUATALG_PINFTY);

#ifndef NDEBUG

        quat_alg_norm(&norm, &quat_tmp, &QUATALG_PINFTY);
        assert(ibq_to_ibz(&test1, &norm));
        assert(ibz_cmp(&test1, u) == 0);

#endif

        // theta <- phiu * theta
        quat_alg_mul(&theta, &quat_tmp, &theta, &QUATALG_PINFTY);

        // phiv
        ibz_set(&quat_tmp.denom, 1);
        ibz_copy(&quat_tmp.coord[0], &((*coeffs)[2]));
        ibz_copy(&quat_tmp.coord[1], &((*coeffs)[3]));
        ibz_set(&quat_tmp.coord[2], 0);
        ibz_set(&quat_tmp.coord[3], 0);
        quat_alg_mul(&quat_tmp, &quat_tmp, &quat_gcd_remove, &QUATALG_PINFTY);

        // theta <- theta * phiv
        quat_alg_mul(&theta, &theta, &quat_tmp, &QUATALG_PINFTY);
        quat_alg_normalize(&theta);
        assert(test_point_order_twof(&bas.P, &E0, exp));
        assert(test_point_order_twof(&bas.Q, &E0, exp));
        assert(test_point_order_twof(&bas.PmQ, &E0, exp));

#ifndef NDEBUG
        quat_alg_norm(&norm, &theta, &QUATALG_PINFTY);
        ibq_to_ibz(&test1, &norm);
        ibz_mul(&test2, &test2, u);
        ibz_mul(&test2, &test2, v);
        assert(ibz_cmp(&test1, &test2) == 0);
        assert(ibz_get(&test1) % 2 == 1);
#endif

        // applying theta
        endomorphism_application_even_basis(&bas, &E0, &theta, exp);

        // copying
        copy_point(&T1.P2, &bas.P);
        copy_point(&T2.P2, &bas.Q);
        copy_point(&T1m2.P2, &bas.PmQ);

        assert(test_point_order_twof(&bas.P, &E0, exp));
        assert(test_point_order_twof(&bas.Q, &E0, exp));
        assert(test_point_order_twof(&bas.PmQ, &E0, exp));
    }

    for (int i = 0; i < TORSION_PLUS_EVEN_POWER - exp; i++) {
        ec_dbl(&T1.P1, &E01.E1, &T1.P1);
        ec_dbl(&T2.P1, &E01.E1, &T2.P1);
        ec_dbl(&T1m2.P1, &E01.E1, &T1m2.P1);
        ec_dbl(&T1.P2, &E01.E2, &T1.P2);
        ec_dbl(&T2.P2, &E01.E2, &T2.P2);
        ec_dbl(&T1m2.P2, &E01.E2, &T1m2.P2);
    }

    assert(test_point_order_twof(&T1.P1, &E01.E1, exp));
    assert(test_point_order_twof(&T1m2.P2, &E01.E2, exp));

    assert(ibz_get(u) % 2 == 1);

    // t=tic();
    theta_chain_comput_strategy(
        isog, exp, &E01, &T1, &T2, &T1m2, strategies[TORSION_PLUS_EVEN_POWER - exp + 2], 0);
    // TOC(t,"final theta chain computation");

    // now we evaluate the basis points through the isogeny
    assert(test_point_order_twof(&bas_u.P, &E01.E1, TORSION_PLUS_EVEN_POWER));
    assert(test_point_order_twof(&bas_u.Q, &E01.E1, TORSION_PLUS_EVEN_POWER));

    // evaluating the basis through the isogeny of degree u*d1

    theta_couple_point_t P, Q, PmQ;
    copy_point(&P.P1, &bas_u.P);
    copy_point(&PmQ.P1, &bas_u.PmQ);
    copy_point(&Q.P1, &bas_u.Q);
    ec_set_zero(&P.P2);
    ec_set_zero(&Q.P2);
    ec_set_zero(&PmQ.P2);
    theta_chain_eval_special_case(&T1, isog, &P, &E01);
    theta_chain_eval_special_case(&T2, isog, &Q, &E01);
    theta_chain_eval_special_case(&T1m2, isog, &PmQ, &E01);

    assert(test_point_order_twof(&T1.P1, &isog->codomain.E1, TORSION_PLUS_EVEN_POWER));
    assert(test_point_order_twof(&T1m2.P2, &isog->codomain.E2, TORSION_PLUS_EVEN_POWER));

    copy_point(&basis->P, &T1.P2);
    copy_point(&basis->Q, &T2.P2);
    copy_point(&basis->PmQ, &T1m2.P2);
    copy_curve(codomain, &isog->codomain.E2);

    // using weil pairing to verify that we selected the correct curve
    fp2_t w0, w1;
    ec_point_t AC, A24;
    ec_basis_t bas0 = BASIS_EVEN;
    copy_point(&AC, &CURVE_E0_A24); // Warning, this is AC, not A24!
    A24_from_AC(&A24, &AC);
    weil(&w0, TORSION_PLUS_EVEN_POWER, &bas0.P, &bas0.Q, &bas0.PmQ, &A24);
    // Changing the AC
    fp2_copy(&AC.x, &codomain->A);
    fp2_copy(&AC.z, &codomain->C);
    A24_from_AC(&A24, &AC);
    weil(&w1, TORSION_PLUS_EVEN_POWER, &basis->P, &basis->Q, &basis->PmQ, &A24);

    digit_t digit_d[NWORDS_ORDER] = { 0 };
    ibz_mul(&tmp, d1, u);
    ibz_mul(&tmp, &tmp, u);
    ibz_mul(&tmp, &tmp, &adjust_u);
    ibz_mul(&tmp, &tmp, &adjust_u);
    ibz_mod(&tmp, &tmp, &TORSION_PLUS_2POWER);

    ibz_to_digit_array(digit_d, &tmp);
    fp2_t test_pow;
    fp2_pow_vartime(&test_pow, &w0, digit_d, NWORDS_ORDER);

    // then we have selected the wrong one
    if (!fp2_is_equal(&w1, &test_pow)) {
        copy_point(&basis->P, &T1.P1);
        copy_point(&basis->Q, &T2.P1);
        copy_point(&basis->PmQ, &T1m2.P1);
        copy_curve(codomain, &isog->codomain.E1);

// verifying that the other one is the good one
#ifndef NDEBUG
        fp2_copy(&AC.x, &codomain->A);
        fp2_copy(&AC.z, &codomain->C);
        A24_from_AC(&A24, &AC);
        weil(&w1, TORSION_PLUS_EVEN_POWER, &basis->P, &basis->Q, &basis->PmQ, &A24);

        ibz_mul(&tmp, d1, u);
        ibz_mul(&tmp, &tmp, u);
        ibz_mul(&tmp, &tmp, &adjust_u);
        ibz_mul(&tmp, &tmp, &adjust_u);
        ibz_mod(&tmp, &tmp, &TORSION_PLUS_2POWER);
        ibz_to_digit_array(digit_d, &tmp);
        fp2_pow_vartime(&test_pow, &w0, digit_d, NWORDS_ORDER);
        assert(fp2_is_equal(&test_pow, &w1));
#endif
    }

    // now we apply M / (u * d1 * adjust_u) where M is the matrix corresponding to the endomorphism
    // beta1 = phi o dual(phi1) we multiply beta1 by the inverse of (u*d1 * adjust_u) mod
    // 2^TORSION_PLUS_EVEN_POWER
    ibz_mul(&tmp, u, d1);
    ibz_mul(&tmp, &tmp, &adjust_u);
    ibz_invmod(&tmp, &tmp, &TORSION_PLUS_2POWER);
    ibz_mul(&beta1->coord[0], &beta1->coord[0], &tmp);
    ibz_mul(&beta1->coord[1], &beta1->coord[1], &tmp);
    ibz_mul(&beta1->coord[2], &beta1->coord[2], &tmp);
    ibz_mul(&beta1->coord[3], &beta1->coord[3], &tmp);

    endomorphism_application_even_basis(basis, codomain, beta1, TORSION_PLUS_EVEN_POWER);

#ifndef NDEBUG

    fp2_copy(&AC.x, &codomain->A);
    fp2_copy(&AC.z, &codomain->C);
    A24_from_AC(&A24, &AC);
    weil(&w1, TORSION_PLUS_EVEN_POWER, &basis->P, &basis->Q, &basis->PmQ, &A24);

    ibz_to_digit_array(digit_d, &lideal->norm);
    fp2_pow_vartime(&test_pow, &w0, digit_d, NWORDS_ORDER);
    assert(fp2_is_equal(&test_pow, &w1));

#endif

    ibq_finalize(&norm);
    ibz_finalize(&test1);
    ibz_finalize(&test2);
    ibz_finalize(&adjust_u);
    ibz_finalize(&adjust_v);
    ibz_finalize(&target);
    ibz_finalize(&tmp);
    ibz_finalize(&two_pow);
    quat_alg_elem_finalize(&theta);
    quat_alg_elem_finalize(&quat_tmp);
    quat_alg_elem_finalize(&quat_gcd_remove);
    return found;
}

/**
 * @brief Translating an ideal into a representation of the corresponding isogeny
 *
 * @param basis Output : evaluation of the canonical basis of E0 through the ideal corresponding to
 * lideal
 * @param lideal : ideal in input
 *
 * This is a wrapper around the ideal to isogeny clapotis function
 */
int
dim2id2iso_arbitrary_isogeny_evaluation(ec_basis_t *basis,
                                        ec_curve_t *codomain,
                                        const quat_left_ideal_t *lideal)
{
    int found;
    ibz_vec_4_t coeffs;

    quat_alg_elem_t beta1, beta2;
    ibz_t u, v, au, bu, d1, d2;

    // theta stuff
    theta_chain_t Phi;
    theta_chain_t phiu, phiv;

    quat_alg_elem_init(&beta1);
    quat_alg_elem_init(&beta2);

    ibz_vec_4_init(&coeffs);

    ibz_init(&u);
    ibz_init(&v);
    ibz_init(&d1);
    ibz_init(&d2);

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
                                                 codomain,
                                                 basis,
                                                 lideal,
                                                 &QUATALG_PINFTY);

    ibz_vec_4_finalize(&coeffs);

    quat_alg_elem_finalize(&beta1);
    quat_alg_elem_finalize(&beta2);

    ibz_finalize(&u);
    ibz_finalize(&v);
    ibz_finalize(&d1);
    ibz_finalize(&d2);

    return found;
}
