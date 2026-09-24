#include <quaternion.h>
#include "internal.h"
#include <stdlib.h>
// get shortest equivalent ideal

#define DEBUG_PRINTS 1

void
quat_lideal_shortest_equivalent(quat_left_ideal_t *equiv,
                                quat_alg_elem_t *elem,
                                const quat_left_ideal_t *lideal,
                                const quat_alg_t *alg)
{

    ibz_mat_4x4_t gram, red;
    quat_alg_elem_t new_alpha;
    quat_alg_elem_init(&new_alpha);
    ibz_mat_4x4_init(&gram);
    ibz_mat_4x4_init(&red);

    // computing the reduced basis
    quat_lideal_reduce_basis(&red, &gram, lideal, alg);

    ibz_set(&(new_alpha.coord[0]), 1);
    ibz_mat_4x4_eval(&new_alpha.coord, &red, &new_alpha.coord);
    ibz_copy(&new_alpha.denom, &lideal->lattice.denom);
    assert(quat_lattice_contains(NULL, &lideal->lattice, &new_alpha));
#ifndef NDEBUG
    ibz_t n, d;
    ibz_init(&n);
    ibz_init(&d);
    quat_alg_norm(&n, &d, &new_alpha, alg);
    assert(ibz_is_one(&d));
    ibz_div(&n, &d, &n, &lideal->norm);
    assert(ibz_is_zero(&d));
    ibz_finalize(&n);
    ibz_finalize(&d);
#endif
    quat_alg_elem_copy(elem, &new_alpha);
    ibz_mul(&new_alpha.denom, &new_alpha.denom, &lideal->norm);
    ibz_neg(&new_alpha.coord[0], &new_alpha.coord[0]);
    quat_lideal_mul(equiv, lideal, &new_alpha, alg);

    quat_alg_elem_finalize(&new_alpha);
    ibz_mat_4x4_finalize(&gram);
    ibz_mat_4x4_finalize(&red);
}

void
quat_lideal_generator_small_coprime(quat_alg_elem_t *gen,
                                    const quat_left_ideal_t *lideal,
                                    const quat_alg_t *alg,
                                    int sampling_bound_bits)
{
    int found = 0;
    ibz_t n, d, n2, gcd;
    ibz_vec_4_t coeffs;
    ibz_init(&n);
    ibz_init(&d);
    ibz_init(&n2);
    ibz_init(&gcd);
    ibz_vec_4_init(&coeffs);
    ibz_copy(&gen->denom, &lideal->lattice.denom);
    ibz_mul(&n2, &lideal->norm, &lideal->norm);
    while (!found) {
        for (int i = 0; i < 4; i++) {
            ibz_rand_interval_bits(&(coeffs[i]), sampling_bound_bits);
        }
        ibz_mat_4x4_eval(&(gen->coord), &(lideal->lattice.basis), &coeffs);

        // check a_alpha invertible
        if (ibz_is_one(&(gen->denom))) {
            ibz_mul(&gcd, &(gen->coord[0]), &ibz_const_two);
            ibz_gcd(&gcd, &lideal->norm, &gcd);
            found = ibz_is_one(&gcd);
        } else {
            found = 1;
            if (0 == ibz_cmp(&ibz_const_two, &(gen->denom))) {
                ibz_gcd(&gcd, &lideal->norm, &(gen->coord[0]));
                found = ibz_is_one(&gcd);
            }
        }
        // check generator
        if (found) {
            quat_alg_norm(&n, &d, gen, alg);
            assert(ibz_is_one(&d));
            ibz_gcd(&gcd, &n2, &n);
            found = (ibz_cmp(&gcd, &lideal->norm) == 0);
        }
    }

    ibz_vec_4_finalize(&coeffs);
    ibz_finalize(&n);
    ibz_finalize(&d);
    ibz_finalize(&n2);
    ibz_finalize(&gcd);
}

// enumerate in dim 2

// helper for cvp
void
ibz_rounded_div(ibz_t *q, const ibz_t *a, const ibz_t *b)
{
    ibz_t r, sign_q, abs_b;
    ibz_init(&r);
    ibz_init(&sign_q);
    ibz_init(&abs_b);

    // assumed to round towards 0
    ibz_abs(&abs_b, b);
    // q is of same sign as a*b (and 0 if a is 0)
    ibz_mul(&sign_q, a, b);
    ibz_div(q, &r, a, b);
    ibz_abs(&r, &r);
    ibz_add(&r, &r, &r);
    ibz_set(&sign_q, (1 - 2 * (ibz_cmp(&sign_q, &ibz_const_zero) < 0)) * (ibz_cmp(&r, &abs_b) > 0));
    ibz_add(q, q, &sign_q);
    ibz_finalize(&r);
    ibz_finalize(&sign_q);
    ibz_finalize(&abs_b);
}

int
quat_dim2_lattice_contains(const ibz_mat_2x2_t *basis, const ibz_t *coord1, const ibz_t *coord2)
{
    int res = 1;
    ibz_t prod, sum, det, r;
    ibz_init(&det);
    ibz_init(&r);
    ibz_init(&sum);
    ibz_init(&prod);
    // compute det, then both coordinates (inverse*det)*vec, where vec is (coord1, coord2) and check wthether det
    // divides both results
    ibz_mat_2x2_det_from_ibz(&det, &((*basis)[0][0]), &((*basis)[0][1]), &((*basis)[1][0]), &((*basis)[1][1]));
    ibz_mul(&sum, coord1, &((*basis)[1][1]));
    ibz_mul(&prod, coord2, &((*basis)[0][1]));
    ibz_sub(&sum, &sum, &prod);
    ibz_div(&prod, &r, &sum, &det);
    res = res & ibz_is_zero(&r);
    ibz_mul(&sum, coord2, &((*basis)[0][0]));
    ibz_mul(&prod, coord1, &((*basis)[1][0]));
    ibz_sub(&sum, &sum, &prod);
    ibz_div(&prod, &r, &sum, &det);
    res = res & ibz_is_zero(&r);
    ibz_finalize(&det);
    ibz_finalize(&r);
    ibz_finalize(&sum);
    ibz_finalize(&prod);
    return (res);
}

void
quat_dim2_lattice_norm(ibz_t *norm, const ibz_t *coord1, const ibz_t *coord2, const ibz_t *norm_q)
{
    ibz_t prod, sum;
    ibz_init(&prod);
    ibz_init(&sum);
    ibz_mul(&sum, coord1, coord1);
    ibz_mul(&prod, coord2, coord2);
    ibz_mul(&prod, &prod, norm_q);
    ibz_add(norm, &sum, &prod);
    ibz_finalize(&prod);
    ibz_finalize(&sum);
}

void
quat_dim2_lattice_bilinear(ibz_t *res,
                           const ibz_t *v11,
                           const ibz_t *v12,
                           const ibz_t *v21,
                           const ibz_t *v22,
                           const ibz_t *norm_q)
{
    ibz_t prod, sum;
    ibz_init(&prod);
    ibz_init(&sum);
    ibz_mul(&sum, v11, v21);
    ibz_mul(&prod, v12, v22);
    ibz_mul(&prod, &prod, norm_q);
    ibz_add(res, &sum, &prod);
    ibz_finalize(&prod);
    ibz_finalize(&sum);
}

// algo 3.1.14 Cohen (exact solution for shortest vector in dimension 2, than take a second, orthogonal vector)
void
quat_dim2_lattice_short_basis(ibz_mat_2x2_t *reduced, const ibz_mat_2x2_t *basis, const ibz_t *norm_q)
{
    ibz_vec_2_t a, b, t;
    ibz_t prod, sum, norm_a, norm_b, r, norm_t, n;
    ibz_vec_2_init(&a);
    ibz_vec_2_init(&b);
    ibz_vec_2_init(&t);
    ibz_init(&prod);
    ibz_init(&sum);
    ibz_init(&r);
    ibz_init(&n);
    ibz_init(&norm_t);
    ibz_init(&norm_a);
    ibz_init(&norm_b);
    // init a,b
    ibz_copy(&(a[0]), &((*basis)[0][0]));
    ibz_copy(&(a[1]), &((*basis)[1][0]));
    ibz_copy(&(b[0]), &((*basis)[0][1]));
    ibz_copy(&(b[1]), &((*basis)[1][1]));
    // compute initial norms
    quat_dim2_lattice_norm(&norm_a, &(a[0]), &(a[1]), norm_q);
    quat_dim2_lattice_norm(&norm_b, &(b[0]), &(b[1]), norm_q);
    // exchange if needed
    if (ibz_cmp(&norm_a, &norm_b) < 0) {
        ibz_copy(&sum, &(a[0]));
        ibz_copy(&(a[0]), &(b[0]));
        ibz_copy(&(b[0]), &sum);
        ibz_copy(&sum, &(a[1]));
        ibz_copy(&(a[1]), &(b[1]));
        ibz_copy(&(b[1]), &sum);
        ibz_copy(&sum, &norm_a);
        ibz_copy(&norm_a, &norm_b);
        ibz_copy(&norm_b, &sum);
    }
    int test = 1;
    while (test) {
        // compute n
        quat_dim2_lattice_bilinear(&n, &(a[0]), &(a[1]), &(b[0]), &(b[1]), norm_q);
        // set r
        ibz_rounded_div(&r, &n, &norm_b);
        // compute t_norm
        ibz_set(&prod, 2);
        ibz_mul(&prod, &prod, &n);
        ibz_mul(&prod, &prod, &r);
        ibz_sub(&sum, &norm_a, &prod);
        ibz_mul(&prod, &r, &r);
        ibz_mul(&prod, &prod, &norm_b);
        ibz_add(&norm_t, &sum, &prod);
        // test:
        if (ibz_cmp(&norm_b, &norm_t) > 0) {
            // compute t, a, b
            ibz_copy(&norm_a, &norm_b);
            ibz_copy(&norm_b, &norm_t);
            // t is a -rb, a is b, b is t
            ibz_mul(&prod, &r, &(b[0]));
            ibz_sub(&(t[0]), &(a[0]), &prod);
            ibz_mul(&prod, &r, &(b[1]));
            ibz_sub(&(t[1]), &(a[1]), &prod);
            ibz_copy(&(a[0]), &(b[0]));
            ibz_copy(&(a[1]), &(b[1]));
            ibz_copy(&(b[0]), &(t[0]));
            ibz_copy(&(b[1]), &(t[1]));
        } else {
            test = 0;
        }
    }
    // output : now b is short: need to get 2nd short vector: idea: take shortest among t and a
    if (ibz_cmp(&norm_t, &norm_a) < 0) {
        ibz_mul(&prod, &r, &(b[0]));
        ibz_sub(&(a[0]), &(a[0]), &prod);
        ibz_mul(&prod, &r, &(b[1]));
        ibz_sub(&(a[1]), &(a[1]), &prod);
    }
    ibz_copy(&((*reduced)[0][0]), &(b[0]));
    ibz_copy(&((*reduced)[1][0]), &(b[1]));
    ibz_copy(&((*reduced)[0][1]), &(a[0]));
    ibz_copy(&((*reduced)[1][1]), &(a[1]));

    ibz_finalize(&prod);
    ibz_finalize(&sum);
    ibz_finalize(&norm_a);
    ibz_finalize(&norm_b);
    ibz_finalize(&norm_t);
    ibz_vec_2_finalize(&a);
    ibz_vec_2_finalize(&b);
    ibz_vec_2_finalize(&t);
    ibz_finalize(&r);
    ibz_finalize(&n);
}

// qlapoti
int
quat_qlapoti_check_mod_condition(const ibz_t *m, const ibz_t *a, const ibz_t *b)
{
    int ok = 1;
    int A = ibz_get(a) & 1;
    int B = ibz_get(b) & 1;
    int M8 = ibz_get(m) & 7;
    int M = M8 & 3;
#ifndef NDEBUG
    if (DEBUG_PRINTS > 3)
        ibz_printf("m %d a %d b %d\n", M, A, B);
#endif
    if (M8 == 0)
        ok = 0;
    if (ok) {
        if ((A == B) && (A == 0)) {
            if (M != 0) {
                ok = 0;
#ifndef NDEBUG
                if (DEBUG_PRINTS > 2)
                    ibz_printf("case 0\n");
#endif
            }
        } else {
            if (((A == B) && (A == 1))) {
                if (M != 2) {
                    ok = 0;
#ifndef NDEBUG
                    if (DEBUG_PRINTS > 2)
                        ibz_printf("case 1\n");
#endif
                }
            } else {
                if (M != 1) {
#ifndef NDEBUG
                    if (DEBUG_PRINTS > 2)
                        ibz_printf("mod 4\n");
#endif
                    ok = 0;
                }
            }
        }
    }

    return (ok);
}

int
quat_dim2_lattice_qlapoti_cvp_condition(quat_alg_elem_t *elem, const ibz_vec_2_t *vec, const void *params)
{
    // remember a_alpha and b_alpha are integers
    int found = 1;
    qlapoti_enumeration_parameters_t *q_params = ((qlapoti_enumeration_parameters_t *)params);
    ibz_t m2, tmp, A, B;
    ibz_vec_2_t a, b;
    ibz_init(&m2);
    ibz_init(&tmp);
    ibz_init(&A);
    ibz_init(&B);
    ibz_vec_2_init(&a);
    ibz_vec_2_init(&b);

    // compute A,B
    // Assumes vec is target-close (up to one sign)
    ibz_neg(&A, &((*vec)[0]));
    ibz_neg(&B, &((*vec)[1]));
#ifndef NDEBUG
    if (DEBUG_PRINTS > 3)
        ibz_printf("A %Zd\nB %Zd\n", &A, &B);
    // assert (2*(a_alpha*A + b_alpha*B)) % N == M % N
    ibz_mul(&tmp, &A, q_params->a_alpha);
    ibz_mul(&m2, &B, q_params->b_alpha);
    ibz_add(&tmp, &tmp, &m2);
    ibz_mod(&tmp, &tmp, q_params->n);
    ibz_mod(&m2, q_params->m, q_params->n);
    assert(0 == ibz_cmp(&m2, &tmp));
#endif
    // compute m2
    // M2 = M - 2 *a_alpha *A - 2 *b_alpha *B; M2 = ZZ(M2 / N)
    ibz_mul(&tmp, &A, q_params->a_alpha);
    ibz_mul(&m2, &B, q_params->b_alpha);
    ibz_add(&tmp, &tmp, &m2);
    ibz_sub(&m2, q_params->m, &tmp);
    ibz_div(&m2, &tmp, &m2, q_params->n);
    assert(ibz_is_zero(&tmp));
    // Complete the square
    // Comp: M3 = M2 - A ** 2 - B ** 2
    ibz_mul(&tmp, &A, &A);
    ibz_sub(&m2, &m2, &tmp);
    ibz_mul(&tmp, &B, &B);
    ibz_sub(&m2, &m2, &tmp);

    // Comp; M4 = 2 * M3 + A * *2 + B * *2
    ibz_add(&m2, &m2, &m2);
    ibz_add(&m2, &m2, &tmp);
    ibz_mul(&tmp, &A, &A);
    ibz_add(&m2, &m2, &tmp);

#ifndef NDEBUG
    if ((DEBUG_PRINTS > 2))
        ibz_printf("M4 %Zd\n", &m2);
    // If the first one is too small, there is no point in trying others...
    // if M4 < 0:if first_vec : break else : continue
    // Must be communicated to exterior loop (enforce change of alpha)? Might be fine already since better enum (if
    // correct bound)
    // Test for unsolvable cases
    if ((DEBUG_PRINTS > 2) && found && !(ibz_cmp(&ibz_const_zero, &m2) < 0))
        ibz_printf("size\n");
#endif
    if (found)
        found = quat_qlapoti_check_mod_condition(&m2, &A, &B);
    if (found) {
        // cornacchia
        found = ibz_cornacchia_extended(&(a[0]), &(b[0]), &m2, q_params->cornacchia_params);
#ifndef NDEBUG
        if ((DEBUG_PRINTS > 2) && !found)
            ibz_printf("cor \n");
#endif
    }
    if (found) {
// treat output
#ifndef NDEBUG
        if (DEBUG_PRINTS > 3)
            ibz_printf("a_0 %Zd, b_0 %Zd\n", &(a[0]), &(b[0]));
#endif
        if (!((ibz_get(&A) & 1) == (ibz_get(&(a[0])) & 1)))
            ibz_swap(&(a[0]), &(b[0]));
        assert(((ibz_get(&A) & 1) == (ibz_get(&(a[0])) & 1)));
        assert(((ibz_get(&B) & 1) == (ibz_get(&(b[0])) & 1)));
        // a1 = ZZ((ad1 + A)/2);b1 = ZZ((bd1 + B)/2)
        ibz_add(&(a[0]), &(a[0]), &A);
        ibz_div(&(a[0]), &tmp, &(a[0]), &ibz_const_two);
        assert(ibz_is_zero(&tmp));
        ibz_add(&(b[0]), &(b[0]), &B);
        ibz_div(&(b[0]), &tmp, &(b[0]), &ibz_const_two);
        assert(ibz_is_zero(&tmp));
        // a2 = A - a1;b2 = B - b1
        ibz_sub(&(a[1]), &A, &(a[0]));
        ibz_sub(&(b[1]), &B, &(b[0]));
        ibz_copy(&(elem->coord[0]), &(a[0]));
        ibz_copy(&(elem->coord[1]), &(a[1]));
        ibz_copy(&(elem->coord[2]), &(b[0]));
        ibz_copy(&(elem->coord[3]), &(b[1]));
    }
    ibz_finalize(&m2);
    ibz_finalize(&tmp);
    ibz_finalize(&A);
    ibz_finalize(&B);
    ibz_vec_2_finalize(&a);
    ibz_vec_2_finalize(&b);
    return (found);
}

int
quat_elem_is_odd_norm(const quat_alg_elem_t *elem)
{
    int found = 0;
    if ((ibz_get(&elem->coord[0]) & 1) == (ibz_get(&elem->coord[2]) & 1)) {
        return found;
    }
    for (int i = 0; i < 4; i++) {
        if (found) {
            if ((ibz_get(&elem->coord[i]) & 3) == 2) {
                return 0;
            }
        } else {
            if ((ibz_get(&elem->coord[i]) & 3) == 2) {
                found = 1;
            }
        }
    }
    return found;
}

int
get_endtype(const quat_alg_elem_t *elem)
{
    int t1, t2, t3, t4;
    // transformlist = {1: [[2, 0, 1, 2], [2, 2, 3, 0], [2, 2, 1, 0], [2, 0, 3, 2]], 2: [[0, 2, 2, 1], [0, 2, 2, 3], [2,
    // 2, 0, 1], [2, 2, 0, 3]]}
    t1 = ibz_get(&elem->coord[0]) & 3;
    t2 = ibz_get(&elem->coord[1]) & 3;
    // NB! These are swapped because I computed this using j and k swapped
    t3 = ibz_get(&elem->coord[3]) & 3;
    t4 = ibz_get(&elem->coord[2]) & 3;
    // End NB!
    if (t1 == 2) {
        if (t2 == 2) {
            if (t3 == 1 && t4 == 0) {
                return 1;
            } else if (t3 == 3 && t4 == 2) {
                return 1;
            } else if (t3 == 0) {
                if (t4 == 1) {
                    return 2;
                } else if (t4 == 3) {
                    return 2;
                } else {
                    return 0;
                }
            } else {
                return 0;
            }
        } else if (t2 == 0) {
            if (t3 == 1 && t4 == 2) {
                return 1;
            } else if (t3 == 3 && t4 == 2) {
                return 1;
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    } else if (t1 == 0) {
        if (t2 == 2 && t3 == 2) {
            if (t4 == 1) {
                return 2;
            } else if (t4 == 3) {
                return 2;
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}
// document all functions
int
quat_qlapoti(quat_alg_elem_t *mu1,
             quat_alg_elem_t *mu2,
             quat_alg_elem_t *theta,
             quat_alg_elem_t *smallest,
             const quat_left_ideal_t *lideal,
             const quat_alg_t *alg,
             int max_counter_alpha,
             int gen_sampling_bound_bits,
             int two_power,
             const ibz_cornacchia_extended_params_t *cornacchia_params)
{
    int found = 0;
    int transformed = 0;
    int keep_alpha = 0;
    quat_left_ideal_t small;
    ibz_mat_4x4_t gram;
    quat_alg_elem_t alpha, encoding, alpha_0;
    ibz_mat_2x2_t L, L_red, L_inv;
    ibz_vec_2_t v_target, v_close, v_diff;
    ibz_t n, m, norm_n, norm_d, a_alpha, b_alpha, x, T, alpha0norm, psqrt;
    ibz_t tmp, lam, two_e, L_det;
    qlapoti_enumeration_parameters_t params;
    quat_alg_elem_t gamma1, gamma2, temp;
    quat_alg_elem_init(&gamma1);
    quat_alg_elem_init(&gamma2);
    ibz_init(&T);
    ibz_init(&L_det);
    ibz_vec_2_init(&v_target);
    ibz_vec_2_init(&v_close);
    ibz_vec_2_init(&v_diff);
    ibz_mat_2x2_init(&L);
    ibz_mat_2x2_init(&L_red);
    ibz_mat_2x2_init(&L_inv);
    ibz_init(&tmp);
    ibz_init(&x);
    ibz_init(&n);
    ibz_init(&m);
    ibz_init(&two_e);
    ibz_init(&lam);
    ibz_init(&norm_n);
    ibz_init(&norm_d);
    ibz_init(&a_alpha);
    ibz_init(&b_alpha);
    ibz_init(&alpha0norm);
    ibz_init(&psqrt);
    ibz_mat_4x4_init(&gram);
    quat_alg_elem_init(&alpha);
    quat_alg_elem_init(&alpha_0);
    quat_alg_elem_init(&encoding);
    quat_alg_elem_init(&temp);
    quat_left_ideal_init(&small);
    ibz_sqrt_floor(&psqrt, &alg->p);
    quat_lideal_shortest_equivalent(&small, smallest, lideal, alg);
    params.cornacchia_params = cornacchia_params;
    quat_lideal_reduce_basis(&small.lattice.basis, &gram, &small, alg);
    ibz_copy(&n, &small.norm);
    ibz_pow(&two_e, &ibz_const_two, two_power);

    // int num_alphas_tried = 0;
    // int num_loops_total = 0;
    for (int counter = 0; counter < max_counter_alpha * 20; counter++) {
        found = 1;

        // compute alpha, lam, alpha_0, alpha0norm
        {
            if (counter % max_counter_alpha == 0) {
                keep_alpha = 0;
                ibz_set(&lam, 1);
            }
            if (!keep_alpha) {
                // num_alphas_tried++;
                quat_lideal_generator_small_coprime(&alpha_0, &small, alg, gen_sampling_bound_bits);
                quat_alg_elem_copy(&alpha, &alpha_0);
                quat_alg_norm(&alpha0norm, &b_alpha, &alpha_0, alg);
                assert(ibz_is_one(&b_alpha));
                ibz_set(&lam, 1);
            } else {
                ibz_add(&lam, &lam, &ibz_const_one);
                quat_alg_add(&alpha, &alpha, &alpha_0);
                ibz_gcd(&b_alpha, &(small.norm), &lam);
                while (!ibz_is_one(&b_alpha)) {
                    ibz_add(&lam, &lam, &ibz_const_one);
                    quat_alg_add(&alpha, &alpha, &alpha_0);
                    ibz_gcd(&b_alpha, &(small.norm), &lam);
                }
            }
            assert(quat_lattice_contains(NULL, &small.lattice, &alpha));
            quat_alg_normalize(&alpha);
        }

        // compute m
        // 2^e-(2norm(alpha0)lam^2)/n
        {
            ibz_add(&norm_n, &alpha0norm, &alpha0norm);
            ibz_mul(&norm_d, &lam, &lam);
            ibz_mul(&norm_n, &norm_n, &norm_d);
            ibz_div_floor(&norm_n, &norm_d, &norm_n, &n);
            assert(ibz_is_zero(&norm_d));
            ibz_sub(&m, &two_e, &norm_n);
            // Discard bad alphas directly: this case should never happen
            if (ibz_cmp(&m, &ibz_const_zero) < 0) {
                found = 0;
                continue;
            }
            assert(ibz_cmp(&m, &ibz_const_zero) > 0);
        }
        // set a_alpha, b_alpha (to double of what they are in sage)
        {
            ibz_copy(&a_alpha, &(alpha.coord[0]));
            ibz_copy(&b_alpha, &(alpha.coord[1]));
            if (ibz_is_one(&alpha.denom)) {
                ibz_add(&a_alpha, &a_alpha, &a_alpha);
                ibz_add(&b_alpha, &b_alpha, &b_alpha);
            }
#ifndef NDEBUG
            else {
                assert(ibz_cmp(&alpha.denom, &ibz_const_two) == 0);
            }
#endif
        }
        // Prepare target vector: T, v_target
        {
            // #A + (2*b_alpha/2*a_alpha)B = M/(2*a_alpha) (mod N)
            // x = ZZ(Z_N(2*b_alpha)*(Z_N(2*a_alpha)**-1))
            ibz_invmod(&tmp, &a_alpha, &n);
            // T = ZZ(Z_N(M)*Z_N(2*a_alpha)**-1)
            ibz_mul(&T, &tmp, &m);
            ibz_mod(&T, &T, &n);
            // v = vector(ZZ, [-T, 0])
            ibz_set(&(v_target[1]), 0);
            ibz_neg(&(v_target[0]), &T);
        }
        // Prepare 2d lattice: L, L_red, L_inv. Set keep_alpha accordingly
        // tmp must contain invmod(a_alpha,n) here
        if (!keep_alpha) {
            // build lattice
            ibz_mul(&x, &b_alpha, &tmp);
            ibz_mod(&x, &x, &n);
            // L = Matrix(ZZ, [[N-x, 1], [N, 0]])
            ibz_sub(&(L[0][0]), &n, &x);
            ibz_copy(&(L[0][1]), &n);
            ibz_set(&(L[1][1]), 0);
            ibz_set(&(L[1][0]), 1);
            quat_dim2_lattice_short_basis(&L_red, &L, &ibz_const_one);
            ibz_mat_2x2_inv_with_det_as_denom(&L_inv, &L_det, &L_red);
            // set keep_alpha: test short basis is suitable
            {
                ibz_gcd(&tmp, &a_alpha, &b_alpha);
                keep_alpha = ibz_is_one(&tmp);
                ibz_mul(&norm_d, &L_red[0][1], &L_red[0][1]);
                ibz_mul(&norm_n, &L_red[1][1], &L_red[1][1]);
                ibz_add(&norm_n, &norm_d, &norm_n);
                keep_alpha = keep_alpha && (ibz_cmp(&norm_n, &psqrt) < 0);
            }
        }
        // find close vector
        {
            ibz_mat_2x2_eval(&v_close, &L_inv, &v_target);
            ibz_rounded_div(&(v_close[0]), &(v_close[0]), &L_det);
            ibz_rounded_div(&(v_close[1]), &(v_close[1]), &L_det);
            ibz_mat_2x2_eval(&v_close, &L_red, &v_close);
            ibz_sub(&(v_diff[0]), &(v_target[0]), &(v_close[0]));
            ibz_sub(&(v_diff[1]), &(v_target[1]), &(v_close[1]));
        }
        // set parameters and call condition
        params.a_alpha = &a_alpha;
        params.b_alpha = &b_alpha;
        params.m = &m;
        params.n = &n;
        found = quat_dim2_lattice_qlapoti_cvp_condition(&encoding, &v_diff, &params);

        if (!found) {
            continue;
        } else {
            // extract ideals from encoding of output
            ibz_mul(&(gamma1.coord[0]), &(encoding.coord[0]), &n);
            ibz_mul(&(gamma2.coord[0]), &(encoding.coord[1]), &n);
            ibz_mul(&(gamma1.coord[1]), &(encoding.coord[2]), &n);
            ibz_mul(&(gamma2.coord[1]), &(encoding.coord[3]), &n);
            quat_alg_add(&gamma1, &gamma1, &alpha);
            quat_alg_add(&gamma2, &gamma2, &alpha);
#ifndef NDEBUG
            ibz_t n1, n2, d;
            ibz_init(&n1);
            ibz_init(&n2);
            ibz_init(&d);
            assert(quat_lattice_contains(NULL, &small.lattice, &gamma1));
            assert(quat_lattice_contains(NULL, &small.lattice, &gamma2));
            quat_alg_norm(&n1, &d, &gamma1, alg);
            assert(ibz_is_one(&d));
            quat_alg_norm(&n2, &d, &gamma2, alg);
            assert(ibz_is_one(&d));
            ibz_div(&n1, &d, &n1, &(small.norm));
            assert(ibz_is_zero(&d));
            ibz_div(&n2, &d, &n2, &(small.norm));
            assert(ibz_is_zero(&d));
            ibz_add(&n2, &n1, &n2);
            assert(0 == ibz_cmp(&two_e, &n2));
            ibz_finalize(&n1);
            ibz_finalize(&n2);
            ibz_finalize(&d);
#endif
            quat_alg_elem_copy(mu1, &gamma1);
            quat_alg_elem_copy(mu2, &gamma2);
            quat_alg_conj(&(gamma1), &(gamma1));
            ibz_mul(&gamma1.denom, &gamma1.denom, &n);
            quat_alg_mul(theta, mu2, &gamma1, alg);
#ifndef NDEBUG
            quat_left_ideal_t I1, I2;
            quat_left_ideal_init(&I1);
            quat_left_ideal_init(&I2);
            ibz_neg(&(gamma2.coord[0]), &(gamma2.coord[0]));
            ibz_mul(&gamma2.denom, &gamma2.denom, &n);
            quat_lideal_mul(&I1, &small, &gamma1, alg);
            quat_lideal_mul(&I2, &small, &gamma2, alg);
            ibz_add(&tmp, &I1.norm, &I2.norm);
            assert(ibz_cmp(&tmp, &two_e) == 0);
            quat_left_ideal_finalize(&I1);
            quat_left_ideal_finalize(&I2);
#endif

            // enforces suitable output
            quat_alg_normalize(theta);
            if (ibz_is_one(&theta->denom)) {
                int endtype = get_endtype(theta);
                if (endtype == 1) {
                    quat_alg_add(&temp, mu1, mu2);
                    ibz_add(&temp.denom, &temp.denom, &temp.denom);
                    quat_alg_sub(mu1, mu1, mu2);
                    ibz_add(&mu1->denom, &mu1->denom, &mu1->denom);
                    quat_alg_elem_copy(mu2, &temp);
                } else if (endtype == 2) {
                    quat_alg_elem_set(&temp, 1, 0, 1, 0, 0);
                    quat_alg_mul(mu2, &temp, mu2, alg);
                    quat_alg_add(&temp, mu1, mu2);
                    ibz_add(&temp.denom, &temp.denom, &temp.denom);
                    quat_alg_sub(mu1, mu1, mu2);
                    ibz_add(&mu1->denom, &mu1->denom, &mu1->denom);
                    quat_alg_elem_copy(mu2, &temp);
                } else {
                    keep_alpha = 0;
                    ibz_set(&lam, 1);
                    ibz_vec_4_set(&gamma1.coord, 0, 0, 0, 0);
                    ibz_vec_4_set(&gamma2.coord, 0, 0, 0, 0);
                    ibz_set(&gamma1.denom, 1);
                    ibz_set(&gamma2.denom, 1);
                    found = 0;
                    transformed = 0;
                    continue;
                }

                assert(quat_lattice_contains(NULL, &small.lattice, mu1));
                assert(quat_lattice_contains(NULL, &small.lattice, mu2));
                quat_alg_conj(&(gamma1), mu1);
                ibz_mul(&gamma1.denom, &gamma1.denom, &n);
                quat_alg_mul(theta, mu2, &gamma1, alg);
                quat_alg_normalize(theta);
                transformed = 1;
            } else {
                if (!(quat_elem_is_odd_norm(theta))) {
                    keep_alpha = 0;
                    ibz_set(&lam, 1);
                    ibz_vec_4_set(&gamma1.coord, 0, 0, 0, 0);
                    ibz_vec_4_set(&gamma2.coord, 0, 0, 0, 0);
                    ibz_set(&gamma1.denom, 1);
                    ibz_set(&gamma2.denom, 1);
                    found = 0;
                    transformed = 0;
                    continue;
                }
            }
            found = 1;
            // num_loops_total = counter;
            break;
        }
    }

    quat_alg_elem_finalize(&gamma1);
    quat_alg_elem_finalize(&gamma2);
    ibz_finalize(&n);
    ibz_finalize(&m);
    ibz_finalize(&tmp);
    ibz_finalize(&x);
    ibz_finalize(&lam);
    ibz_finalize(&two_e);
    ibz_finalize(&norm_n);
    ibz_finalize(&norm_d);
    ibz_finalize(&a_alpha);
    ibz_finalize(&b_alpha);
    ibz_finalize(&alpha0norm);
    ibz_finalize(&psqrt);
    ibz_finalize(&L_det);
    quat_left_ideal_finalize(&small);
    quat_alg_elem_finalize(&alpha);
    quat_alg_elem_finalize(&alpha_0);
    quat_alg_elem_finalize(&encoding);
    quat_alg_elem_finalize(&temp);
    ibz_finalize(&T);
    ibz_vec_2_finalize(&v_target);
    ibz_vec_2_finalize(&v_close);
    ibz_vec_2_finalize(&v_diff);
    ibz_mat_2x2_finalize(&L);
    ibz_mat_2x2_finalize(&L_red);
    ibz_mat_2x2_finalize(&L_inv);
    ibz_mat_4x4_finalize(&gram);

    return (found + found * transformed * 2);
}
