#include "quaternion_tests.h"
#include <stdlib.h>
#include <assert.h>

int
quat_test_qlapoti_lideal_short_equivalent()
{
    int res = 0;
    quat_left_ideal_t lideal;
    quat_left_ideal_t test;
    quat_left_ideal_t equiv;
    quat_alg_t alg;
    quat_alg_elem_t elem;
    quat_lattice_t O;
    ibz_t n;
    quat_lattice_init(&O);
    quat_alg_elem_init(&elem);
    ibz_init(&n);
    quat_left_ideal_init(&lideal);
    quat_left_ideal_init(&test);
    quat_left_ideal_init(&equiv);
    quat_alg_init_set_ui(&alg, 103);
    quat_lattice_O0_set(&O);

    quat_alg_elem_set(&elem, 1, 1, 0, -2, 1);
    ibz_set(&n, 3);
    quat_lideal_create(&lideal, &elem, &n, &O, &alg);
    quat_lideal_create(&test, &elem, &n, &O, &alg);
    assert(quat_lideal_equals(&test, &lideal, &alg));
    quat_alg_elem_set(&elem, 1, 200, 5192, 2397665, -12233);
    quat_lideal_mul(&lideal, &lideal, &elem, &alg);
    quat_lideal_shortest_equivalent(&equiv, &elem, &lideal, &alg);
    res = res || !quat_lideal_equals(&equiv, &test, &alg);
    quat_alg_conj(&elem, &elem);
    ibz_mul(&elem.denom, &elem.denom, &lideal.norm);
    quat_lideal_mul(&test, &lideal, &elem, &alg);
    res = res || !quat_lideal_equals(&equiv, &test, &alg);

    if (res != 0) {
        printf("Quaternion unit test qlapoti_lideal_short_equivalent failed\n");
    }
    quat_left_ideal_finalize(&lideal);
    quat_left_ideal_finalize(&test);
    quat_left_ideal_finalize(&equiv);
    quat_alg_finalize(&alg);
    quat_alg_elem_finalize(&elem);
    ibz_finalize(&n);
    quat_lattice_finalize(&O);
    return (res);
}

// int quat_lideal_generator_coprime(quat_alg_elem_t *gen, const quat_left_ideal_t *lideal, const ibz_t *n, const
// quat_alg_t *alg, int bound);
int
quat_test_qlapoti_lideal_generator_small_coprime()
{
    int res = 0;

    quat_alg_t alg;
    quat_lattice_t order, order2;
    quat_alg_elem_t gen, test;
    ibz_t N, M, gcd, norm_int, prod;
    ibz_t norm_d;
    quat_left_ideal_t lideal, lideal2;
    quat_alg_init_set_ui(&alg, 103);
    quat_lattice_init(&order);
    quat_lattice_init(&order2);
    quat_alg_elem_init(&gen);
    quat_alg_elem_init(&test);
    ibz_init(&N);
    ibz_init(&M);
    ibz_init(&norm_int);
    ibz_init(&prod);
    ibz_init(&gcd);
    ibz_init(&norm_d);
    quat_left_ideal_init(&lideal);
    quat_left_ideal_init(&lideal2);
    quat_lattice_O0_set(&order);

    ibz_set(&gen.coord[0], 3);
    ibz_set(&gen.coord[1], 5);
    ibz_set(&gen.coord[2], 7);
    ibz_set(&gen.coord[3], 11);
    ibz_set(&N, 51);
    quat_alg_norm(&norm_int, &norm_d, &gen, &alg);

    // product of primes < 50
    ibz_set_from_str(&M, "614889782588491410", 10);

    quat_lideal_create(&lideal, &gen, &N, &order, &alg);

    quat_lideal_generator_small_coprime(&gen, &lideal, &alg, 16);
    quat_alg_norm(&norm_int, &norm_d, &gen, &alg);
    res |= !ibz_is_one(&norm_d);

    quat_lideal_create(&lideal2, &gen, &N, &order, &alg);
    res |= !quat_lideal_equals(&lideal, &lideal2, &alg);
    // This is not a requirement actually
    // quat_alg_make_primitive(&test.coord, &test.denom, &gen, &(lideal.lattice));
    // ibz_abs(&test.denom, &test.denom);
    // res |= !ibz_is_one(&test.denom);
    // ibz_printf("res: %u\n", res);

    // No need to test for non-maximal orders
    quat_alg_elem_set(&gen, 1, 2, 4, 2, 6);
    quat_alg_norm(&norm_int, &norm_d, &gen, &alg);
    ibz_set(&N, 23);

    quat_lideal_create(&lideal, &gen, &N, &order, &alg);

    ibz_set(&M, 15);
    quat_lideal_generator_small_coprime(&gen, &lideal, &alg, 16);
    quat_alg_norm(&norm_int, &norm_d, &gen, &alg);
    res |= !ibz_is_one(&norm_d);

    quat_lideal_create(&lideal2, &gen, &N, &order, &alg);
    res |= !quat_lideal_equals(&lideal, &lideal2, &alg);

    if (res != 0) {
        printf("Quaternion unit test qlapoti_lideal_generator_small_coprime failed\n");
    }
    quat_alg_finalize(&alg);
    quat_lattice_finalize(&order);
    quat_lattice_finalize(&order2);
    quat_alg_elem_finalize(&gen);
    quat_alg_elem_finalize(&test);
    ibz_finalize(&N);
    ibz_finalize(&M);
    ibz_finalize(&norm_int);
    ibz_finalize(&prod);
    ibz_finalize(&gcd);
    ibz_finalize(&norm_d);
    quat_left_ideal_finalize(&lideal);
    quat_left_ideal_finalize(&lideal2);
    return (res);
}

// cvp helper functions
// void ibz_rounded_div(ibz_t *q, const ibz_t *a, const ibz_t *b);
int
quat_test_qlapoti_ibz_rounded_div()
{
    int res = 0;
    ibz_t q, a, b;
    ibz_init(&a);
    ibz_init(&b);
    ibz_init(&q);

    // basic tests
    ibz_set(&a, 15);
    ibz_set(&b, 3);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == 5);
    ibz_set(&a, 16);
    ibz_set(&b, 3);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == 5);
    ibz_set(&a, 17);
    ibz_set(&b, 3);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == 6);
    ibz_set(&a, 37);
    ibz_set(&b, 5);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == 7);
    // test sign combination
    ibz_set(&a, 149);
    ibz_set(&b, 12);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == 12);
    ibz_set(&a, 149);
    ibz_set(&b, -12);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == -12);
    ibz_set(&a, -149);
    ibz_set(&b, -12);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == 12);
    ibz_set(&a, -149);
    ibz_set(&b, 12);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == -12);
    ibz_set(&a, 151);
    ibz_set(&b, 12);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == 13);
    ibz_set(&a, -151);
    ibz_set(&b, -12);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == 13);
    ibz_set(&a, 151);
    ibz_set(&b, -12);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == -13);
    ibz_set(&a, -151);
    ibz_set(&b, 12);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == -13);
    // divisibles with sign
    ibz_set(&a, 144);
    ibz_set(&b, 12);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == 12);
    ibz_set(&a, -144);
    ibz_set(&b, -12);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == 12);
    ibz_set(&a, 144);
    ibz_set(&b, -12);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == -12);
    ibz_set(&a, -144);
    ibz_set(&b, 12);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == -12);
    // tests close to 0
    ibz_set(&a, -12);
    ibz_set(&b, -25);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == 0);
    ibz_set(&a, 12);
    ibz_set(&b, 25);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == 0);
    ibz_set(&a, -12);
    ibz_set(&b, 25);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == 0);
    ibz_set(&a, 12);
    ibz_set(&b, -25);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == 0);
    ibz_set(&a, -12);
    ibz_set(&b, -23);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == 1);
    ibz_set(&a, 12);
    ibz_set(&b, 23);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == 1);
    ibz_set(&a, -12);
    ibz_set(&b, 23);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == -1);
    ibz_set(&a, 12);
    ibz_set(&b, -23);
    ibz_rounded_div(&q, &a, &b);
    res = res || !(ibz_get(&q) == -1);
    // test output equal input
    ibz_set(&a, -151);
    ibz_set(&b, 12);
    ibz_rounded_div(&a, &a, &b);
    res = res || !(ibz_get(&a) == -13);
    ibz_set(&a, -151);
    ibz_set(&b, 12);
    ibz_rounded_div(&b, &a, &b);
    res = res || !(ibz_get(&b) == -13);

    if (res != 0) {
        printf("Quaternion unit test integer_ibz_rounded_div failed\n");
    }
    ibz_finalize(&a);
    ibz_finalize(&b);
    ibz_finalize(&q);
    return (res);
}

// int quat_dim2_lattice_contains(ibz_mat_2x2_t *basis, ibz_t *coord1, ibz_t *coord2);
int
quat_test_qlapoti_dim2_lattice_contains()
{
    int res = 0;
    ibz_mat_2x2_t basis;
    ibz_t c1, c2;
    ibz_mat_2x2_init(&basis);
    ibz_init(&c1);
    ibz_init(&c2);
    ibz_mat_2x2_set(&basis, -1, 2, 5, 0);
    ibz_set(&c1, 0);
    ibz_set(&c2, 0);
    res = res || !quat_dim2_lattice_contains(&basis, &c1, &c2);
    ibz_set(&c1, 1);
    ibz_set(&c2, 5);
    res = res || !quat_dim2_lattice_contains(&basis, &c1, &c2);
    ibz_set(&c1, 1);
    ibz_set(&c2, 4);
    res = res || quat_dim2_lattice_contains(&basis, &c1, &c2);

    if (res != 0) {
        printf("Quaternion unit test qlapoti_dim2_lattice_contains failed\n");
    }
    ibz_mat_2x2_finalize(&basis);
    ibz_finalize(&c1);
    ibz_finalize(&c2);
    return (res);
}

// void quat_dim2_lattice_norm(ibz_t *norm, const ibz_t *coord1, const ibz_t *coord2, const ibz_t *norm_q)
int
quat_test_qlapoti_dim2_lattice_norm()
{
    int res = 0;
    ibz_t norm, cmp, q, a, b;
    ibz_init(&a);
    ibz_init(&b);
    ibz_init(&q);
    ibz_init(&norm);
    ibz_init(&cmp);
    ibz_set(&a, 1);
    ibz_set(&b, 2);
    ibz_set(&q, 3);
    ibz_set(&cmp, 1 * 1 + 3 * 2 * 2);
    quat_dim2_lattice_norm(&norm, &a, &b, &q);
    res = res || ibz_cmp(&norm, &cmp);
    ibz_set(&a, 7);
    ibz_set(&b, -2);
    ibz_set(&q, 5);
    ibz_set(&cmp, 7 * 7 + 5 * 2 * 2);
    quat_dim2_lattice_norm(&norm, &a, &b, &q);
    res = res || ibz_cmp(&norm, &cmp);
    ibz_set(&cmp, 7 * 7 + 7 * 7 * 7);
    quat_dim2_lattice_norm(&a, &a, &a, &a);
    res = res || ibz_cmp(&a, &cmp);
    if (res != 0) {
        printf("Quaternion unit test qlapoti_dim2_lattice_norm failed\n");
    }
    ibz_finalize(&a);
    ibz_finalize(&b);
    ibz_finalize(&q);
    ibz_finalize(&norm);
    ibz_finalize(&cmp);
    return (res);
}

// void quat_dim2_lattice_bilinear(ibz_t *res, const ibz_t *v11, const ibz_t *v12,const ibz_t *v21, const ibz_t *v22,
// const ibz_t *norm_q);
int
quat_test_qlapoti_dim2_lattice_bilinear()
{
    int res = 0;
    ibz_t prod, cmp, q, a, b, c, d;
    ibz_init(&a);
    ibz_init(&b);
    ibz_init(&c);
    ibz_init(&d);
    ibz_init(&q);
    ibz_init(&prod);
    ibz_init(&cmp);
    ibz_set(&a, 1);
    ibz_set(&b, 2);
    ibz_set(&c, 3);
    ibz_set(&d, 4);
    ibz_set(&q, 3);
    ibz_set(&cmp, 1 * 3 + 3 * 2 * 4);
    quat_dim2_lattice_bilinear(&prod, &a, &b, &c, &d, &q);
    res = res || ibz_cmp(&prod, &cmp);
    ibz_set(&a, 7);
    ibz_set(&b, -2);
    ibz_set(&c, 4);
    ibz_set(&d, 7);
    ibz_set(&q, 5);
    ibz_set(&cmp, 7 * 4 - 5 * 2 * 7);
    quat_dim2_lattice_bilinear(&prod, &a, &b, &c, &d, &q);
    res = res || ibz_cmp(&prod, &cmp);
    ibz_set(&cmp, 7 * 7 + 7 * 7 * 7);
    quat_dim2_lattice_bilinear(&a, &a, &a, &a, &a, &a);
    res = res || ibz_cmp(&a, &cmp);
    if (res != 0) {
        printf("Quaternion unit test qlapoti_dim2_lattice_bilinear failed\n");
    }
    ibz_finalize(&a);
    ibz_finalize(&b);
    ibz_finalize(&c);
    ibz_finalize(&d);
    ibz_finalize(&q);
    ibz_finalize(&prod);
    ibz_finalize(&cmp);
    return (res);
}

// according to conditions from https://cseweb.ucsd.edu/classes/wi12/cse206A-a/lec3.pdf
int
quat_dim2_lattice_verify_lll(const ibz_mat_2x2_t *mat, const ibz_t *norm_q)
{
    ibz_t prod, norm_a, norm_bstar, norm_b, b_ab, b_abstar, b_bbstar, p01_denom, p00_denom, norm_p00, norm_p01;
    ibz_vec_2_t p00, p01, bstar;
    ibz_vec_2_init(&p00);
    ibz_vec_2_init(&p01);
    ibz_vec_2_init(&bstar);
    ibz_init(&prod);
    ibz_init(&norm_a);
    ibz_init(&norm_bstar);
    ibz_init(&norm_b);
    ibz_init(&b_ab);
    ibz_init(&b_bbstar);
    ibz_init(&b_abstar);
    ibz_init(&p00_denom);
    ibz_init(&p01_denom);
    ibz_init(&norm_p00);
    ibz_init(&norm_p01);
    quat_dim2_lattice_norm(&norm_a, &((*mat)[0][0]), &((*mat)[1][0]), norm_q);
    quat_dim2_lattice_norm(&norm_b, &((*mat)[0][1]), &((*mat)[1][1]), norm_q);
    quat_dim2_lattice_bilinear(&b_ab, &((*mat)[0][0]), &((*mat)[1][0]), &((*mat)[0][1]), &((*mat)[1][1]), norm_q);
    ibz_mul(&(bstar[0]), &((*mat)[0][1]), &norm_b);
    ibz_mul(&prod, &((*mat)[0][0]), &b_ab);
    ibz_sub(&(bstar[0]), &(bstar[0]), &prod);
    ibz_mul(&(bstar[1]), &((*mat)[1][1]), &norm_b);
    ibz_mul(&prod, &((*mat)[1][0]), &b_ab);
    ibz_sub(&(bstar[1]), &(bstar[1]), &prod);
    quat_dim2_lattice_norm(&norm_bstar, &(bstar[0]), &(bstar[1]), norm_q);
    quat_dim2_lattice_bilinear(&b_bbstar, &(bstar[0]), &(bstar[1]), &((*mat)[0][1]), &((*mat)[1][1]), norm_q);
    quat_dim2_lattice_bilinear(&b_abstar, &((*mat)[0][0]), &((*mat)[1][0]), &(bstar[0]), &(bstar[1]), norm_q);
    // first projection
    ibz_mul(&(p00[0]), &((*mat)[0][0]), &norm_bstar);
    ibz_mul(&prod, &(bstar[0]), &b_abstar);
    ibz_add(&(p00[0]), &(p00[0]), &prod);
    ibz_mul(&(p00[1]), &((*mat)[1][0]), &norm_bstar);
    ibz_mul(&prod, &(bstar[1]), &b_abstar);
    ibz_add(&(p00[1]), &(p00[1]), &prod);
    ibz_copy(&p00_denom, &norm_bstar);
    // second projection
    ibz_mul(&(p01[0]), &((*mat)[0][0]), &b_ab);
    ibz_mul(&(p01[0]), &(p01[0]), &norm_bstar);
    ibz_mul(&prod, &(bstar[0]), &b_bbstar);
    ibz_mul(&prod, &prod, &norm_a);
    ibz_add(&(p01[0]), &(p01[0]), &prod);
    ibz_mul(&(p01[1]), &((*mat)[1][0]), &b_ab);
    ibz_mul(&(p01[1]), &(p01[1]), &norm_bstar);
    ibz_mul(&prod, &(bstar[1]), &b_bbstar);
    ibz_mul(&prod, &prod, &norm_a);
    ibz_add(&(p01[1]), &(p01[1]), &prod);
    ibz_mul(&p01_denom, &norm_a, &norm_bstar);
    // compute norms
    quat_dim2_lattice_norm(&norm_p00, &(p00[0]), &(p00[1]), norm_q);
    quat_dim2_lattice_norm(&norm_p01, &(p01[0]), &(p01[1]), norm_q);
    // compare on same denom
    ibz_mul(&norm_p00, &norm_p00, &norm_a);
    ibz_mul(&norm_p00, &norm_p00, &norm_a);
    ibz_mul(&p00_denom, &p00_denom, &norm_a);
    int res = (ibz_cmp(&norm_p00, &norm_p01) <= 0);
    // also check 2times bilinear against norm
    ibz_set(&prod, 2);
    ibz_mul(&prod, &b_ab, &prod);
    res = res && (ibz_cmp(&prod, &norm_a) < 0);

    ibz_finalize(&prod);
    ibz_finalize(&norm_a);
    ibz_finalize(&norm_bstar);
    ibz_finalize(&norm_b);
    ibz_finalize(&norm_p00);
    ibz_finalize(&norm_p01);
    ibz_finalize(&b_ab);
    ibz_finalize(&b_abstar);
    ibz_finalize(&b_bbstar);
    ibz_finalize(&p00_denom);
    ibz_finalize(&p01_denom);
    ibz_vec_2_finalize(&p00);
    ibz_vec_2_finalize(&p01);
    ibz_vec_2_finalize(&bstar);
    return (res);
}

// void quat_dim2_lattice_short_basis(ibz_mat_2x2_t *reduced, const ibz_mat_2x2_t *basis, const ibz_t *norm_q);
int
quat_test_qlapoti_dim2_lattice_short_basis()
{
    int res = 0;
    ibz_mat_2x2_t basis, cmp, red;
    ibz_t prod, sum, norm_q, bound;
    ibz_init(&prod);
    ibz_init(&sum);
    ibz_init(&norm_q);
    ibz_init(&bound);
    ibz_mat_2x2_init(&basis);
    ibz_mat_2x2_init(&cmp);
    ibz_mat_2x2_init(&red);
    // first test
    ibz_mat_2x2_set(&basis, 3, 5, 7, 9);
    ibz_set(&norm_q, 2);
    quat_dim2_lattice_short_basis(&red, &basis, &norm_q);
    // check second basis vector larger (or at least equal) than 1st
    res = res || (ibz_cmp(&sum, &prod) > 0);
    // check mutual inclusion of lattices
    res = res || (!quat_dim2_lattice_contains(&red, &(basis[0][0]), &(basis[1][0])));
    res = res || (!quat_dim2_lattice_contains(&red, &(basis[0][1]), &(basis[1][1])));
    res = res || (!quat_dim2_lattice_contains(&basis, &(red[0][0]), &(red[1][0])));
    res = res || (!quat_dim2_lattice_contains(&basis, &(red[0][1]), &(red[1][1])));
    // check bilinear form value is small
    ibz_set(&bound, 12);
    quat_dim2_lattice_bilinear(&sum, &(red[0][0]), &(red[0][1]), &(red[0][1]), &(red[1][1]), &norm_q);
    res = res || (ibz_cmp(&sum, &bound) > 0);
    // check norm smaller than original
    ibz_set(&bound, 12);
    quat_dim2_lattice_norm(&sum, &(red[0][0]), &(red[1][0]), &norm_q);
    res = res || (ibz_cmp(&sum, &bound) > 0);
    quat_dim2_lattice_norm(&prod, &(red[0][1]), &(red[1][1]), &norm_q);
    res = res || (ibz_cmp(&prod, &bound) > 0);
    // check lll
    res = res || !quat_dim2_lattice_verify_lll(&red, &norm_q);

    // second test
    ibz_mat_2x2_set(&basis, 48, 4, 81, 9);
    ibz_set(&norm_q, 9);
    quat_dim2_lattice_short_basis(&red, &basis, &norm_q);
    // check second basis vector larger (or at least equal) than 1st
    res = res || (ibz_cmp(&sum, &prod) > 0);
    // check mutual inclusion of lattices
    res = res || (!quat_dim2_lattice_contains(&red, &(basis[0][0]), &(basis[1][0])));
    res = res || (!quat_dim2_lattice_contains(&red, &(basis[0][1]), &(basis[1][1])));
    res = res || (!quat_dim2_lattice_contains(&basis, &(red[0][0]), &(red[1][0])));
    res = res || (!quat_dim2_lattice_contains(&basis, &(red[0][1]), &(red[1][1])));
    // check bilinear form value is small
    ibz_set(&bound, 50 * 100);
    quat_dim2_lattice_bilinear(&sum, &(red[0][0]), &(red[0][1]), &(red[0][1]), &(red[1][1]), &norm_q);
    res = res || (ibz_cmp(&sum, &bound) > 0);
    // check norm smaller than original
    ibz_set(&bound, 50 * 50);
    quat_dim2_lattice_norm(&sum, &(red[0][0]), &(red[1][0]), &norm_q);
    res = res || (ibz_cmp(&sum, &bound) > 0);
    quat_dim2_lattice_norm(&prod, &(red[0][1]), &(red[1][1]), &norm_q);
    res = res || (ibz_cmp(&prod, &bound) > 0);
    // check lll
    res = res || !quat_dim2_lattice_verify_lll(&red, &norm_q);

    // third test
    ibz_mat_2x2_set(&basis, -1, 2, 3, 2);
    ibz_set(&norm_q, 2);
    quat_dim2_lattice_short_basis(&red, &basis, &norm_q);
    // check second basis vector larger (or at least equal) than 1st
    res = res || (ibz_cmp(&sum, &prod) > 0);
    // check mutual inclusion of lattices
    res = res || (!quat_dim2_lattice_contains(&red, &(basis[0][0]), &(basis[1][0])));
    res = res || (!quat_dim2_lattice_contains(&red, &(basis[0][1]), &(basis[1][1])));
    res = res || (!quat_dim2_lattice_contains(&basis, &(red[0][0]), &(red[1][0])));
    res = res || (!quat_dim2_lattice_contains(&basis, &(red[0][1]), &(red[1][1])));
    // check bilinear form value is small
    ibz_set(&bound, 12);
    quat_dim2_lattice_bilinear(&sum, &(red[0][0]), &(red[0][1]), &(red[0][1]), &(red[1][1]), &norm_q);
    res = res || (ibz_cmp(&sum, &bound) > 0);
    // check norm smaller than original
    ibz_set(&bound, 12);
    quat_dim2_lattice_norm(&sum, &(red[0][0]), &(red[1][0]), &norm_q);
    res = res || (ibz_cmp(&sum, &bound) > 0);
    quat_dim2_lattice_norm(&prod, &(red[0][1]), &(red[1][1]), &norm_q);
    res = res || (ibz_cmp(&prod, &bound) > 0);
    // check lll
    res = res || !quat_dim2_lattice_verify_lll(&red, &norm_q);

    // 4th test
    ibz_set(&norm_q, 12165);
    ibz_mat_2x2_set(&basis, 364, 0, 1323546, 266606);
    quat_dim2_lattice_short_basis(&red, &basis, &norm_q);
    // check second basis vector larger (or at least equal) than 1st
    res = res || (ibz_cmp(&sum, &prod) > 0);
    // check mutual inclusion of lattices
    res = res || (!quat_dim2_lattice_contains(&red, &(basis[0][0]), &(basis[1][0])));
    res = res || (!quat_dim2_lattice_contains(&red, &(basis[0][1]), &(basis[1][1])));
    res = res || (!quat_dim2_lattice_contains(&basis, &(red[0][0]), &(red[1][0])));
    res = res || (!quat_dim2_lattice_contains(&basis, &(red[0][1]), &(red[1][1])));
    // check bilinear form value is small
    quat_dim2_lattice_norm(&bound, &(basis[0][1]), &(basis[1][1]), &norm_q);
    quat_dim2_lattice_bilinear(&sum, &(red[0][0]), &(red[0][1]), &(red[0][1]), &(red[1][1]), &norm_q);
    res = res || (ibz_cmp(&sum, &bound) > 0);
    // check norm smaller than original
    quat_dim2_lattice_norm(&bound, &(basis[0][1]), &(basis[1][1]), &norm_q);
    quat_dim2_lattice_norm(&sum, &(red[0][0]), &(red[1][0]), &norm_q);
    res = res || (ibz_cmp(&sum, &bound) > 0);
    quat_dim2_lattice_norm(&prod, &(red[0][1]), &(red[1][1]), &norm_q);
    res = res || (ibz_cmp(&prod, &bound) > 0);
    // check lll
    res = res || !quat_dim2_lattice_verify_lll(&red, &norm_q);

    if (res != 0) {
        printf("Quaternion unit test qlapoti_dim2_lattice_short_basis failed\n");
    }
    ibz_finalize(&prod);
    ibz_finalize(&sum);
    ibz_finalize(&norm_q);
    ibz_finalize(&bound);
    ibz_mat_2x2_finalize(&basis);
    ibz_mat_2x2_finalize(&cmp);
    ibz_mat_2x2_finalize(&red);
    return res;
}

int
quat_test_lattice_qlapoti_cvp_condition()
{
    int res = 0;
    int tmp_res;
    ibz_vec_2_t ab;
    quat_alg_elem_t out, cmp;
    ibz_t m, n, a_alpha, b_alpha;
    qlapoti_enumeration_parameters_t params;
    ibz_cornacchia_extended_params_t cornacchia_params;
    short primes[82] = { 2,   5,   13,  17,  29,  37,  41,  53,  61,  73,  89,  97,  101, 109, 113, 137, 149,
                         157, 173, 181, 193, 197, 229, 233, 241, 257, 269, 277, 281, 293, 313, 317, 337, 349,
                         353, 373, 389, 397, 401, 409, 421, 433, 449, 457, 461, 509, 521, 541, 557, 569, 577,
                         593, 601, 613, 617, 641, 653, 661, 673, 677, 701, 709, 733, 757, 761, 769, 773, 797,
                         809, 821, 829, 853, 857, 877, 881, 929, 937, 941, 953, 977, 997, 1009 };
    ibz_init(&m);
    ibz_init(&n);
    ibz_init(&a_alpha);
    ibz_init(&b_alpha);
    ibz_vec_2_init(&ab);
    quat_alg_elem_init(&out);
    quat_alg_elem_init(&cmp);
    params.a_alpha = &a_alpha;
    params.b_alpha = &b_alpha;
    params.m = &m;
    params.n = &n;
    cornacchia_params.bad_primes_prod = &ibz_const_one;
    cornacchia_params.prime_list = primes;
    cornacchia_params.primality_test_iterations = 100;
    cornacchia_params.q = 1;
    cornacchia_params.prime_list_length = 82;
    params.cornacchia_params = &cornacchia_params;

    ibz_set_from_str(&n, "3588068757273373623184041115224326680", 10);
    ibz_set_from_str(&m, "113078212145816597093331034492692987310554024929624970589366518801641159342", 10);
    ibz_set_from_str(&a_alpha, "55802309567322429328887817479381622545473", 10); // denom 2
    ibz_neg(&a_alpha, &a_alpha);
    ibz_set_from_str(&b_alpha, "164832325035431386247229290667677491931168", 10); // denom 2
    ibz_set_from_str(&(ab[0]), "3324930064024875210", 10);
    ibz_set_from_str(&(ab[1]), "2166920436366097869", 10);
    ibz_neg(&(ab[0]), &(ab[0]));
    ibz_neg(&(ab[1]), &(ab[1]));
    ibz_set_from_str(&(cmp.coord[0]), "5098956283653089832", 10);
    ibz_set_from_str(&(cmp.coord[1]), "1774026219628214622", 10);
    ibz_neg(&(cmp.coord[1]), &(cmp.coord[1]));
    ibz_set_from_str(&(cmp.coord[2]), "1185358405318603462", 10);
    ibz_set_from_str(&(cmp.coord[3]), "981562031047494407", 10);
    tmp_res = quat_dim2_lattice_qlapoti_cvp_condition(&out, &ab, &params);
    res = res || !tmp_res;
    if (tmp_res) {
        res = res || !(quat_alg_elem_equal(&out, &cmp));
    }

    ibz_set_from_str(&(ab[0]), "6588358983801282790", 10);
    ibz_set_from_str(&(ab[1]), "4139758051024177521", 10);
    ibz_set_from_str(&(cmp.coord[0]), "2743025951076238849", 10);
    ibz_neg(&(cmp.coord[0]), &(cmp.coord[0]));
    ibz_set_from_str(&(cmp.coord[1]), "3845333032725043941", 10);
    ibz_neg(&(cmp.coord[1]), &(cmp.coord[1]));
    ibz_set_from_str(&(cmp.coord[2]), "1506191648044069223", 10);
    ibz_neg(&(cmp.coord[2]), &(cmp.coord[2]));
    ibz_set_from_str(&(cmp.coord[3]), "2633566402980108298", 10);
    ibz_neg(&(cmp.coord[3]), &(cmp.coord[3]));
    tmp_res = quat_dim2_lattice_qlapoti_cvp_condition(&out, &ab, &params);
    res = res || !tmp_res;
    if (tmp_res) {
        res = res || !(quat_alg_elem_equal(&out, &cmp));
    }

    if (res != 0) {
        printf("Quaternion unit test lattice_qlapoti_cvp_condition failed\n");
    }
    ibz_finalize(&m);
    ibz_finalize(&n);
    ibz_finalize(&a_alpha);
    ibz_finalize(&b_alpha);
    ibz_vec_2_finalize(&ab);
    quat_alg_elem_finalize(&out);
    quat_alg_elem_finalize(&cmp);
    return res;
}

int
quat_test_qlapoti_qlapoti()
{
    int res = 0;
    int output = 0;
    quat_alg_t alg;
    quat_left_ideal_t lideal, I1, I2, small_equiv;
    quat_alg_elem_t elem, mu1, mu2, theta, cmp, small;
    quat_lattice_t O;
    ibz_t n, cofactor;
    ibz_cornacchia_extended_params_t cornacchia_params;
    short primes[82] = { 2,   5,   13,  17,  29,  37,  41,  53,  61,  73,  89,  97,  101, 109, 113, 137, 149,
                         157, 173, 181, 193, 197, 229, 233, 241, 257, 269, 277, 281, 293, 313, 317, 337, 349,
                         353, 373, 389, 397, 401, 409, 421, 433, 449, 457, 461, 509, 521, 541, 557, 569, 577,
                         593, 601, 613, 617, 641, 653, 661, 673, 677, 701, 709, 733, 757, 761, 769, 773, 797,
                         809, 821, 829, 853, 857, 877, 881, 929, 937, 941, 953, 977, 997, 1009 };
    quat_lattice_init(&O);
    quat_alg_elem_init(&elem);
    ibz_init(&n);
    ibz_init(&cofactor);
    quat_left_ideal_init(&lideal);
    quat_left_ideal_init(&I1);
    quat_left_ideal_init(&I2);
    quat_left_ideal_init(&small_equiv);
    quat_alg_elem_init(&mu1);
    quat_alg_elem_init(&mu2);
    quat_alg_elem_init(&theta);
    quat_alg_elem_init(&cmp);
    quat_alg_elem_init(&small);
    ibz_pow(&n, &ibz_const_two, 248);
    ibz_set(&cofactor, 5);
    ibz_mul(&n, &n, &cofactor);
    ibz_sub(&n, &n, &ibz_const_one);
    quat_alg_init_set(&alg, &n);
    quat_lattice_O0_set(&O);
    cornacchia_params.bad_primes_prod = &ibz_const_one;
    cornacchia_params.prime_list = primes;
    cornacchia_params.primality_test_iterations = 100;
    cornacchia_params.q = 1;
    cornacchia_params.prime_list_length = 82;

    // set ideal
    ibz_set_from_str(&elem.coord[0], "11679558057699548966295664199498600969", 10);
    ibz_set_from_str(&elem.coord[1], "5995913694671076569732436307214390300", 10);
    ibz_set(&elem.coord[2], 0);
    ibz_set(&elem.coord[3], 1);
    ibz_set(&elem.denom, 2);
    assert(quat_lattice_contains(NULL, &O, &elem));
    ibz_set_from_str(&n, "3588068757273373623184041115224326680", 10);
#ifndef NDEBUG
    ibz_t nd, nn;
    ibz_init(&nd);
    ibz_init(&nn);
    quat_alg_norm(&nn, &nd, &elem, &alg);
    assert(ibz_is_one(&nd));
    ibz_div(&nn, &nd, &nn, &n);
    assert(ibz_is_zero(&nd));
    ibz_finalize(&nn);
    ibz_finalize(&nd);
#endif

    quat_lideal_create(&lideal, &elem, &n, &O, &alg);
    assert(0 == ibz_cmp(&lideal.norm, &n));
#ifndef NDEBUG
    quat_lattice_t tes;
    quat_lattice_init(&tes);
    quat_lideal_inverse_lattice_without_hnf(&tes, &lideal, &alg);
    quat_lattice_mul(&tes, &lideal.lattice, &tes, &alg);
    assert(quat_lattice_inclusion(&lideal.lattice, lideal.parent_order));
    quat_lattice_mul(&tes, &lideal.lattice, &lideal.lattice, &alg);
    assert(quat_lattice_inclusion(&tes, &lideal.lattice));
    quat_lattice_mul(&tes, lideal.parent_order, &lideal.lattice, &alg);
    assert(quat_lattice_inclusion(&tes, &lideal.lattice));
    quat_lattice_finalize(&tes);
#endif

    output = quat_qlapoti(&mu1, &mu2, &theta, &small, &lideal, &alg, 20000, 17, 246, &cornacchia_params);
    res = res || !output;

    if (!res) {
        res = res || !quat_lattice_contains(NULL, &lideal.lattice, &small);
        quat_alg_conj(&small, &small);
        ibz_mul(&small.denom, &small.denom, &lideal.norm);
        quat_lideal_mul(&small_equiv, &lideal, &small, &alg);

        res = res || !quat_lattice_contains(NULL, &small_equiv.lattice, &mu1);
        res = res || !quat_lattice_contains(NULL, &small_equiv.lattice, &mu2);
        ibz_sqrt_floor(&n, &alg.p);
        res = res || !(ibz_cmp(&small_equiv.norm, &n) < 0);
        quat_alg_conj(&mu1, &mu1);
        ibz_mul(&mu1.denom, &mu1.denom, &small_equiv.norm);
        quat_lideal_mul(&I1, &small_equiv, &mu1, &alg);
        quat_alg_mul(&cmp, &mu2, &mu1, &alg);
        quat_alg_sub(&cmp, &cmp, &theta);
        res = res || !quat_alg_elem_is_zero(&cmp);
        quat_alg_conj(&mu2, &mu2);
        ibz_mul(&mu2.denom, &mu2.denom, &small_equiv.norm);
        quat_lideal_mul(&I2, &small_equiv, &mu2, &alg);
        ibz_add(&n, &I1.norm, &I2.norm);
        ibz_pow(&cofactor, &ibz_const_two, 246 - (0 != (output & 2)));
        res = res || !(ibz_cmp(&cofactor, &n) == 0);
    }

    // For now this case (2^128 norm of smallest equivalent ideal) is not handled
    // quat_lattice_O0_set(&O);
    // ibz_mat_4x4_zero(&lideal.lattice.basis);
    // ibz_set_from_str(&n, "63802943797675961899382738893456539649", 10);

    // ibz_set_from_str(&(lideal.lattice.basis[0][0]), "127605887595351923798765477786913079298", 10);
    // ibz_set_from_str(&(lideal.lattice.basis[1][1]), "127605887595351923798765477786913079298", 10);

    // ibz_set_from_str(&(lideal.lattice.basis[1][2]), "106338239662793269832304564822427566081", 10);
    // ibz_set_from_str(&(lideal.lattice.basis[2][2]), "1", 10);

    // ibz_set_from_str(&(lideal.lattice.basis[0][3]), "21267647932558653966460912964485513217 ", 10);
    // ibz_set(&lideal.lattice.denom, 2);
    // lideal.parent_order = &O;
    // ibz_set_from_str(&(lideal.lattice.basis[3][3]), "1", 10);
    // quat_lideal_norm(&lideal);
    // assert(0 == ibz_cmp(&lideal.norm, &n));

    // #ifndef NDEBUG
    // quat_lattice_t test;
    // quat_lattice_init(&test);
    // assert(quat_lattice_inclusion(&lideal.lattice, lideal.parent_order));
    // quat_lattice_mul(&test, &lideal.lattice, &lideal.lattice, &alg);
    // assert(quat_lattice_inclusion(&test, &lideal.lattice));
    // quat_lattice_mul(&test, lideal.parent_order, &lideal.lattice, &alg);
    // assert(quat_lattice_inclusion(&test, &lideal.lattice));
    // quat_lattice_finalize(&test);
    // #endif

    // output = quat_qlapoti(&mu1, &mu2, &theta, &small, &lideal, &alg, 20000, 17, 246, &cornacchia_params);
    // res = res || !output;

    // if (!res) {
    //     res = res || !quat_lattice_contains(NULL, &lideal.lattice, &small);
    //     quat_alg_conj(&small, &small);
    //     ibz_mul(&small.denom, &small.denom, &lideal.norm);
    //     quat_lideal_mul(&small_equiv, &lideal, &small, &alg);

    //    res = res || !quat_lattice_contains(NULL, &small_equiv.lattice, &mu1);
    //    res = res || !quat_lattice_contains(NULL, &small_equiv.lattice, &mu2);
    //    ibz_sqrt_floor(&n, &alg.p);
    //    res = res || !(ibz_cmp(&small_equiv.norm, &n) < 0);
    //    quat_alg_conj(&mu1, &mu1);
    //    ibz_mul(&mu1.denom, &mu1.denom, &small_equiv.norm);
    //    quat_lideal_mul(&I1, &small_equiv, &mu1, &alg);
    //    quat_alg_mul(&cmp, &mu2, &mu1, &alg);
    //    quat_alg_sub(&cmp, &cmp, &theta);
    //    res = res || !quat_alg_elem_is_zero(&cmp);
    //    quat_alg_conj(&mu2, &mu2);
    //    ibz_mul(&mu2.denom, &mu2.denom, &small_equiv.norm);
    //    quat_lideal_mul(&I2, &small_equiv, &mu2, &alg);
    //    ibz_add(&n, &I1.norm, &I2.norm);
    //    ibz_pow(&cofactor, &ibz_const_two, 246 - (0 != (output & 2)));
    //    res = res || !(ibz_cmp(&cofactor, &n) == 0);
    //}

    if (res != 0) {
        printf("Quaternion unit test qlapoti_qlapoti failed\n");
    }
    quat_lattice_finalize(&O);
    quat_alg_elem_finalize(&elem);
    ibz_finalize(&n);
    ibz_finalize(&cofactor);
    quat_left_ideal_finalize(&lideal);
    quat_left_ideal_finalize(&I1);
    quat_left_ideal_finalize(&I2);
    quat_left_ideal_finalize(&small_equiv);
    quat_alg_elem_finalize(&mu1);
    quat_alg_elem_finalize(&mu2);
    quat_alg_elem_finalize(&theta);
    quat_alg_elem_finalize(&cmp);
    quat_alg_elem_finalize(&small);
    quat_alg_finalize(&alg);
    return (res);
}

int
quat_test_qlapoti_randomized_qlapoti()
{
    int output = 0;
    int res = 0;
    quat_left_ideal_t lideal, I1, I2, small_equiv;
    quat_alg_elem_t mu1, mu2, theta, cmp, small;
    quat_alg_t alg;
    quat_p_extremal_maximal_order_t O;
    ibz_t n, cofactor;
    quat_represent_integer_params_t params;
    ibz_cornacchia_extended_params_t cornacchia_params;
    short primes[82] = { 2,   5,   13,  17,  29,  37,  41,  53,  61,  73,  89,  97,  101, 109, 113, 137, 149,
                         157, 173, 181, 193, 197, 229, 233, 241, 257, 269, 277, 281, 293, 313, 317, 337, 349,
                         353, 373, 389, 397, 401, 409, 421, 433, 449, 457, 461, 509, 521, 541, 557, 569, 577,
                         593, 601, 613, 617, 641, 653, 661, 673, 677, 701, 709, 733, 757, 761, 769, 773, 797,
                         809, 821, 829, 853, 857, 877, 881, 929, 937, 941, 953, 977, 997, 1009 };
    quat_lattice_init(&O.order);
    quat_alg_elem_init(&O.t);
    quat_alg_elem_init(&O.z);
    ibz_init(&n);
    ibz_init(&cofactor);
    quat_left_ideal_init(&lideal);
    quat_left_ideal_init(&I1);
    quat_left_ideal_init(&I2);
    quat_left_ideal_init(&small_equiv);
    quat_alg_elem_init(&mu1);
    quat_alg_elem_init(&mu2);
    quat_alg_elem_init(&theta);
    quat_alg_elem_init(&cmp);
    quat_alg_elem_init(&small);
    ibz_pow(&n, &ibz_const_two, 248);
    ibz_set(&cofactor, 5);
    ibz_mul(&n, &n, &cofactor);
    ibz_sub(&n, &n, &ibz_const_one);
    quat_alg_init_set(&alg, &n);
    cornacchia_params.bad_primes_prod = &ibz_const_one;
    cornacchia_params.prime_list = primes;
    cornacchia_params.primality_test_iterations = 100;
    cornacchia_params.q = 1;
    cornacchia_params.prime_list_length = 82;

    quat_lattice_O0_set_extremal(&O);
    params.order = &O;
    params.algebra = &alg;
    params.primality_test_iterations = 31;
    // set &n to a prime larger than p^2
    for (int i = 0; i < 10; i++) {
        ibz_set(&n, 0);
        ibz_set(&cofactor, 0);
        ibz_set(&lideal.lattice.denom, 1);
        ibz_set(&I1.lattice.denom, 1);
        ibz_set(&I2.lattice.denom, 1);
        ibz_set(&lideal.norm, 0);
        ibz_set(&I1.norm, 0);
        ibz_set(&I2.norm, 0);
        ibz_mat_4x4_zero(&(lideal.lattice.basis));
        ibz_mat_4x4_zero(&(I1.lattice.basis));
        ibz_mat_4x4_zero(&(I2.lattice.basis));
        ibz_set_from_str(
            &n,
            "4072254666723681508155138358078070932780242149161529099968578135037959387304277915989100465848"
            "257629505329846152616214434336488530208890909665449179137",
            10);
        quat_sampling_random_ideal_O0_given_norm(&lideal, &n, 1, &params, NULL);
        if (i > 0) {

            output = quat_qlapoti(&mu1, &mu2, &theta, &small, &lideal, &alg, 20000, 17, 246, &cornacchia_params);
            res = res || !output;

            if (!res) {

                res = res || !quat_lattice_contains(NULL, &lideal.lattice, &small);
                quat_alg_conj(&small, &small);
                ibz_mul(&small.denom, &small.denom, &lideal.norm);
                quat_lideal_mul(&small_equiv, &lideal, &small, &alg);

                res = res || !quat_lattice_contains(NULL, &small_equiv.lattice, &mu1);
                res = res || !quat_lattice_contains(NULL, &small_equiv.lattice, &mu2);
                ibz_sqrt_floor(&n, &alg.p);
                res = res || !(ibz_cmp(&small_equiv.norm, &n) < 0);
                quat_alg_conj(&mu1, &mu1);
                ibz_mul(&mu1.denom, &mu1.denom, &small_equiv.norm);
                quat_lideal_mul(&I1, &small_equiv, &mu1, &alg);
                quat_alg_mul(&cmp, &mu2, &mu1, &alg);
                quat_alg_sub(&cmp, &cmp, &theta);
                res = res || !quat_alg_elem_is_zero(&cmp);
                quat_alg_conj(&mu2, &mu2);
                ibz_mul(&mu2.denom, &mu2.denom, &small_equiv.norm);
                quat_lideal_mul(&I2, &small_equiv, &mu2, &alg);
                ibz_add(&n, &I1.norm, &I2.norm);
                ibz_pow(&cofactor, &ibz_const_two, 246 - (0 != (output & 2)));
                res = res || !(ibz_cmp(&cofactor, &n) == 0);
            }
            if (res) {
                break;
            }
        }
    }

    if (res != 0) {
        printf("Quaternion randomized unit test qlapoti_randomized_qlapoti failed\n");
    }
    quat_lattice_finalize(&O.order);
    ibz_finalize(&n);
    ibz_finalize(&cofactor);
    quat_left_ideal_finalize(&lideal);
    quat_left_ideal_finalize(&I1);
    quat_left_ideal_finalize(&I2);
    quat_left_ideal_finalize(&small_equiv);
    quat_alg_elem_finalize(&mu1);
    quat_alg_elem_finalize(&mu2);
    quat_alg_elem_finalize(&theta);
    quat_alg_elem_finalize(&cmp);
    quat_alg_elem_finalize(&small);
    quat_alg_finalize(&alg);
    quat_alg_elem_finalize(&O.t);
    quat_alg_elem_finalize(&O.z);
    return (res);
}

// run all previous tests
int
quat_test_qlapoti(void)
{
    int res = 0;
    printf("\nRunning quaternion tests of qlapoti (sub-)functions\n");
    res = res | quat_test_qlapoti_lideal_short_equivalent();
    res = res | quat_test_qlapoti_lideal_generator_small_coprime();
    res = res | quat_test_qlapoti_ibz_rounded_div();
    res = res | quat_test_qlapoti_dim2_lattice_contains();
    res = res | quat_test_qlapoti_dim2_lattice_norm();
    res = res | quat_test_qlapoti_dim2_lattice_bilinear();
    res = res | quat_test_qlapoti_dim2_lattice_short_basis();
    res = res | quat_test_lattice_qlapoti_cvp_condition();
    res = res | quat_test_qlapoti_qlapoti();
    res = res | quat_test_qlapoti_randomized_qlapoti();
    return (res);
}
