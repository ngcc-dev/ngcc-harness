#include "quaternion_tests.h"

// int ibz_generate_random_prime(ibz_t *p, int is3mod4, int bitsize);
int
quat_test_ibz_generate_random_prime()
{
    int res = 0;
    int bitsize, is3mod4, primality_test_attempts;
    ibz_t p;
    ibz_init(&p);
    bitsize = 20;
    primality_test_attempts = 30;
    is3mod4 = 1;
    res = res || !ibz_generate_random_prime(&p, is3mod4, bitsize, primality_test_attempts);
    res = res || (ibz_probab_prime(&p, 20) == 0);
    res = res || (ibz_bitsize(&p) < bitsize);
    res = res || (is3mod4 && (ibz_get(&p) % 4 != 3));
    bitsize = 30;
    is3mod4 = 0;
    res = res || !ibz_generate_random_prime(&p, is3mod4, bitsize, primality_test_attempts);
    res = res || (ibz_probab_prime(&p, 20) == 0);
    res = res || (ibz_bitsize(&p) < bitsize);
    res = res || (is3mod4 && (ibz_get(&p) % 4 != 3));
    is3mod4 = 1;
    res = res || !ibz_generate_random_prime(&p, is3mod4, bitsize, primality_test_attempts);
    res = res || (ibz_probab_prime(&p, 20) == 0);
    res = res || (ibz_bitsize(&p) < bitsize);
    res = res || (is3mod4 && (ibz_get(&p) % 4 != 3));
    if (res) {
        printf("Quaternion unit test ibz_generate_random_prime failed\n");
    }
    ibz_finalize(&p);
    return (res);
}

// int ibz_cornacchia_prime(ibz_t *x, ibz_t *y, const ibz_t *n, const ibz_t *p);
int
quat_test_integer_ibz_cornacchia_prime(void)
{
    int res = 0;
    ibz_t x, y, n, prod, c_res, p;
    ibz_init(&x);
    ibz_init(&y);
    ibz_init(&n);
    ibz_init(&p);
    ibz_init(&prod);
    ibz_init(&c_res);

    ibz_set(&n, 1);
    // there is a solution in these cases
    ibz_set(&p, 5);
    if (ibz_cornacchia_prime(&x, &y, &n, &p)) {
        ibz_mul(&c_res, &x, &x);
        ibz_mul(&prod, &y, &y);
        ibz_mul(&prod, &prod, &n);
        ibz_add(&c_res, &c_res, &prod);
        res = res || ibz_cmp(&p, &c_res);
    } else {
        res = 1;
    }
    ibz_set(&p, 2);
    if (ibz_cornacchia_prime(&x, &y, &n, &p)) {
        ibz_mul(&c_res, &x, &x);
        ibz_mul(&prod, &y, &y);
        ibz_mul(&prod, &prod, &n);
        ibz_add(&c_res, &c_res, &prod);
        res = res || ibz_cmp(&p, &c_res);
    } else {
        res = 1;
    }
    ibz_set(&p, 41);
    if (ibz_cornacchia_prime(&x, &y, &n, &p)) {
        ibz_mul(&c_res, &x, &x);
        ibz_mul(&prod, &y, &y);
        ibz_mul(&prod, &prod, &n);
        ibz_add(&c_res, &c_res, &prod);
        res = res || ibz_cmp(&p, &c_res);
    } else {
        res = 1;
    }
    ibz_set(&n, 2);
    ibz_set(&p, 3);
    if (ibz_cornacchia_prime(&x, &y, &n, &p)) {
        ibz_mul(&c_res, &x, &x);
        ibz_mul(&prod, &y, &y);
        ibz_mul(&prod, &prod, &n);
        ibz_add(&c_res, &c_res, &prod);
        res = res || ibz_cmp(&p, &c_res);
    } else {
        res = 1;
    }
    ibz_set(&n, 3);
    ibz_set(&p, 7);
    if (ibz_cornacchia_prime(&x, &y, &n, &p)) {
        ibz_mul(&c_res, &x, &x);
        ibz_mul(&prod, &y, &y);
        ibz_mul(&prod, &prod, &n);
        ibz_add(&c_res, &c_res, &prod);
        res = res || ibz_cmp(&p, &c_res);
    } else {
        res = 1;
    }

    ibz_set(&n, 1);
    // there is no solution in these cases
    ibz_set(&p, 7);
    res = res || ibz_cornacchia_prime(&x, &y, &n, &p);
    ibz_set(&p, 3);
    res = res || ibz_cornacchia_prime(&x, &y, &n, &p);
    ibz_set(&n, 3);
    ibz_set(&p, 5);
    res = res || ibz_cornacchia_prime(&x, &y, &n, &p);
    // This should be solved
    ibz_set(&n, 3);
    ibz_set(&p, 3);
    res = res || !ibz_cornacchia_prime(&x, &y, &n, &p);
    if (res != 0) {
        printf("Quaternion unit test integer_ibz_cornacchia_prime failed\n");
    }
    ibz_finalize(&x);
    ibz_finalize(&y);
    ibz_finalize(&n);
    ibz_finalize(&p);
    ibz_finalize(&prod);
    ibz_finalize(&c_res);
    return res;
}

// tests for cornacchia helper functions
// void ibz_complex_mul(ibz_t *re_res, ibz_t *im_res, const ibz_t *re_a, const ibz_t *im_a, const
// ibz_t *re_b, const ibz_t *im_b, const ibz_t *q);
int
quat_test_integer_ibz_complex_mul(void)
{
    int res = 0;
    ibz_t re_res, re_a, re_b, re_cmp, im_res, im_a, im_b, im_cmp, q;
    ibz_init(&re_res);
    ibz_init(&re_a);
    ibz_init(&re_b);
    ibz_init(&re_cmp);
    ibz_init(&im_res);
    ibz_init(&im_a);
    ibz_init(&im_b);
    ibz_init(&im_cmp);
    ibz_init(&q);

    ibz_set(&q, 1);
    ibz_set(&re_a, 1);
    ibz_set(&im_a, 2);
    ibz_set(&re_b, 3);
    ibz_set(&im_b, -4);
    ibz_set(&re_cmp, 11);
    ibz_set(&im_cmp, 2);
    ibz_complex_mul(&re_res, &im_res, &re_a, &im_a, &re_b, &im_b, &q);
    res = res || ibz_cmp(&re_res, &re_cmp);
    res = res || ibz_cmp(&im_res, &im_cmp);

    ibz_set(&re_a, -3);
    ibz_set(&im_a, -5);
    ibz_set(&re_b, 2);
    ibz_set(&im_b, 4);
    ibz_set(&re_cmp, 14);
    ibz_set(&im_cmp, -22);
    ibz_complex_mul(&re_res, &im_res, &re_a, &im_a, &re_b, &im_b, &q);
    res = res || ibz_cmp(&re_res, &re_cmp);
    res = res || ibz_cmp(&im_res, &im_cmp);

    ibz_set(&q, 3);
    ibz_set(&re_a, -3);
    ibz_set(&im_a, -5);
    ibz_set(&re_b, 2);
    ibz_set(&im_b, 4);
    ibz_set(&re_cmp, 54);
    ibz_set(&im_cmp, -22);
    ibz_complex_mul(&re_res, &im_res, &re_a, &im_a, &re_b, &im_b, &q);
    res = res || ibz_cmp(&re_res, &re_cmp);
    res = res || ibz_cmp(&im_res, &im_cmp);

    if (res != 0) {
        printf("Quaternion unit test integer_ibz_complex_mul failed\n");
    }
    ibz_finalize(&re_res);
    ibz_finalize(&re_cmp);
    ibz_finalize(&re_a);
    ibz_finalize(&re_b);
    ibz_finalize(&im_res);
    ibz_finalize(&im_cmp);
    ibz_finalize(&im_a);
    ibz_finalize(&im_b);
    ibz_finalize(&q);
    return (res);
}

// void ibz_complex_mul_by_complex_power(ibz_t *re_res, ibz_t *im_res, const ibz_t *re_a, const
// ibz_t *im_a, const ibz_t *q, int64_t exp);
int
quat_test_integer_ibz_complex_mul_by_complex_power(void)
{
    int res = 0;
    int64_t exp;
    ibz_t re_res, re_a, re_cmp, im_res, im_a, im_cmp, q;
    ibz_init(&re_res);
    ibz_init(&re_a);
    ibz_init(&re_cmp);
    ibz_init(&im_res);
    ibz_init(&im_a);
    ibz_init(&im_cmp);
    ibz_init(&q);

    ibz_set(&q, 1);

    exp = 0;
    ibz_set(&re_a, 1);
    ibz_set(&im_a, 2);
    ibz_set(&re_res, 3);
    ibz_set(&im_res, -4);
    ibz_set(&re_cmp, 3);
    ibz_set(&im_cmp, -4);
    ibz_complex_mul_by_complex_power(&re_res, &im_res, &re_a, &im_a, &q, exp);
    res = res || ibz_cmp(&re_res, &re_cmp);
    res = res || ibz_cmp(&im_res, &im_cmp);

    exp = 1;
    ibz_set(&re_a, 1);
    ibz_set(&im_a, 2);
    ibz_set(&re_res, 3);
    ibz_set(&im_res, -4);
    ibz_set(&re_cmp, 11);
    ibz_set(&im_cmp, 2);
    ibz_complex_mul_by_complex_power(&re_res, &im_res, &re_a, &im_a, &q, exp);
    res = res || ibz_cmp(&re_res, &re_cmp);
    res = res || ibz_cmp(&im_res, &im_cmp);

    exp = 2;
    ibz_set(&re_a, 1);
    ibz_set(&im_a, 2);
    ibz_set(&re_res, 3);
    ibz_set(&im_res, -4);
    ibz_set(&re_cmp, 7);
    ibz_set(&im_cmp, 24);
    ibz_complex_mul_by_complex_power(&re_res, &im_res, &re_a, &im_a, &q, exp);
    res = res || ibz_cmp(&re_res, &re_cmp);
    res = res || ibz_cmp(&im_res, &im_cmp);

    exp = 2;
    ibz_set(&q, 3);
    ibz_set(&re_a, 1);
    ibz_set(&im_a, 2);
    ibz_set(&re_res, 3);
    ibz_set(&im_res, -4);
    ibz_set(&re_cmp, 15);
    ibz_set(&im_cmp, 56);
    ibz_complex_mul_by_complex_power(&re_res, &im_res, &re_a, &im_a, &q, exp);
    res = res || ibz_cmp(&re_res, &re_cmp);
    res = res || ibz_cmp(&im_res, &im_cmp);

    if (res != 0) {
        printf("Quaternion unit test integer_ibz_complex_mul_by_complex_power failed\n");
    }
    ibz_finalize(&re_res);
    ibz_finalize(&re_cmp);
    ibz_finalize(&re_a);
    ibz_finalize(&im_res);
    ibz_finalize(&im_cmp);
    ibz_finalize(&im_a);
    ibz_finalize(&q);
    return (res);
}

// int ibz_cornacchia_extended_prime_loop(ibz_t *re_res, ibz_t *im_res, const ibz_t *q, int64_t
// prime, int64_t val);
int
quat_test_integer_ibz_cornacchia_extended_prime_loop(void)
{
    int res = 0;
    int64_t p;
    ibz_t re_res, re_cmp, im_res, im_cmp, prod, q;
    ibz_init(&re_res);
    ibz_init(&re_cmp);
    ibz_init(&im_res);
    ibz_init(&im_cmp);
    ibz_init(&prod);
    ibz_init(&q);

    ibz_set(&q, 1);

    p = 5;
    ibz_set(&re_res, 1);
    ibz_set(&im_res, 1);
    ibz_set(&re_cmp, -1);
    ibz_set(&im_cmp, 7);
    ibz_cornacchia_extended_prime_loop(&re_res, &im_res, &q, p, 2);
    res = res || ibz_cmp(&re_res, &re_cmp);
    res = res || ibz_cmp(&im_res, &im_cmp);

    p = 7;
    ibz_set(&re_res, -1);
    ibz_set(&im_res, 7);
    ibz_set(&re_cmp, -1);
    ibz_set(&im_cmp, 7);
    ibz_cornacchia_extended_prime_loop(&re_res, &im_res, &q, p, 0);
    res = res || ibz_cmp(&re_res, &re_cmp);
    res = res || ibz_cmp(&im_res, &im_cmp);

    p = 7;
    ibz_set(&q, 3);
    ibz_set(&re_res, -1);
    ibz_set(&im_res, 7);
    ibz_set(&re_cmp, -1);
    ibz_set(&im_cmp, 7);
    ibz_cornacchia_extended_prime_loop(&re_res, &im_res, &q, p, 0);
    res = res || ibz_cmp(&re_res, &re_cmp);
    res = res || ibz_cmp(&im_res, &im_cmp);

    p = 7;
    ibz_set(&q, 3);
    ibz_set(&re_res, 0);
    ibz_set(&im_res, 1);
    ibz_set(&re_cmp, 12);
    ibz_set(&im_cmp, 1);
    ibz_cornacchia_extended_prime_loop(&re_res, &im_res, &q, p, 2);
    ibz_abs(&re_res, &re_res);
    ibz_abs(&im_res, &im_res);
    res = res || ibz_cmp(&re_res, &re_cmp);
    res = res || ibz_cmp(&im_res, &im_cmp);

    if (res != 0) {
        printf("Quaternion unit test integer_ibz_cornacchia_extended_prime_loop failed\n");
    }
    ibz_finalize(&re_res);
    ibz_finalize(&re_cmp);
    ibz_finalize(&im_res);
    ibz_finalize(&im_cmp);
    ibz_finalize(&prod);
    ibz_finalize(&q);
    return (res);
}

// int ibz_cornacchia_extended(ibz_t *x, ibz_t *y, const ibz_t *n, const short *prime_list, const
// int prime_list_length, short primality_test_iterations, const ibz_t *bad_primes_prod);
int
quat_test_integer_ibz_cornacchia_extended(void)
{
    int res = 0;
    ibz_t x, y, n, prod, c_res, bad;
    ibz_cornacchia_extended_params_t params;
    short primes[26] = { 2,  3,  5,  7,  11, 13, 17, 19, 23, 29, 31, 37, 41,
                         43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101 };
    ibz_init(&x);
    ibz_init(&y);
    ibz_init(&n);
    ibz_init(&prod);
    ibz_init(&c_res);
    ibz_init(&bad);
    ibz_set(&bad, 3 * 7 * 11 * 19);
    ibz_set(&prod, 23 * 31 * 43);
    ibz_mul(&bad, &prod, &bad);
    ibz_set(&prod, 59 * 67 * 71);
    ibz_mul(&bad, &prod, &bad);
    ibz_set(&prod, 73 * 83);
    ibz_mul(&bad, &prod, &bad);
    params.prime_list = primes;
    params.prime_list_length = 26;
    params.bad_primes_prod = NULL;
    params.primality_test_iterations = 20;

    // there is a solution in these cases
    ibz_set(&n, 5);
    params.q = 1;
    params.prime_list_length = 4;
    if (ibz_cornacchia_extended(&x, &y, &n, &params)) {
        ibz_mul(&c_res, &x, &x);
        ibz_mul(&prod, &y, &y);
        ibz_add(&c_res, &c_res, &prod);
        res = res || ibz_cmp(&n, &c_res);
    } else {
        res = 1;
    }
    ibz_set(&n, 50);
    params.prime_list_length = 7;
    if (ibz_cornacchia_extended(&x, &y, &n, &params)) {
        ibz_mul(&c_res, &x, &x);
        ibz_mul(&prod, &y, &y);
        ibz_add(&c_res, &c_res, &prod);
        res = res || ibz_cmp(&n, &c_res);
    } else {
        res = 1;
    }
    ibz_set(&n, 4100);
    params.prime_list_length = 26;
    params.bad_primes_prod = &bad;
    if (ibz_cornacchia_extended(&x, &y, &n, &params)) {
        ibz_mul(&c_res, &x, &x);
        ibz_mul(&prod, &y, &y);
        ibz_add(&c_res, &c_res, &prod);
        res = res || ibz_cmp(&n, &c_res);
    } else {
        res = 1;
    }
    // test product of all small primes
    ibz_set(&n, 5 * 13 * 17 * 29 * 37 * 41);
    ibz_set(&x, 53 * 61 * 73 * 89 * 97);
    ibz_mul(&n, &n, &x);
    ibz_set(&x, 404);
    ibz_mul(&n, &n, &x);
    params.bad_primes_prod = NULL;
    if (ibz_cornacchia_extended(&x, &y, &n, &params)) {
        ibz_mul(&c_res, &x, &x);
        ibz_mul(&prod, &y, &y);
        ibz_add(&c_res, &c_res, &prod);
        res = res || ibz_cmp(&n, &c_res);
    } else {
        res = 1;
    }
    // test with large prime
    ibz_set(&n, 1381); // prime and 1 mod 4
    params.bad_primes_prod = &bad;
    if (ibz_cornacchia_extended(&x, &y, &n, &params)) {
        ibz_mul(&c_res, &x, &x);
        ibz_mul(&prod, &y, &y);
        ibz_add(&c_res, &c_res, &prod);
        res = res || ibz_cmp(&n, &c_res);
    } else {
        res = 1;
    }
    // test with large prime part
    ibz_set(&n, 5 * 13 * 17 * 29 * 37 * 97);
    ibz_set(&x, 1381); // prime and 1 mod 4
    ibz_mul(&n, &n, &x);
    params.bad_primes_prod = NULL;
    if (ibz_cornacchia_extended(&x, &y, &n, &params)) {
        ibz_mul(&c_res, &x, &x);
        ibz_mul(&prod, &y, &y);
        ibz_add(&c_res, &c_res, &prod);
        res = res || ibz_cmp(&n, &c_res);
    } else {
        res = 1;
    }

    // there is no solution in these cases
    params.bad_primes_prod = &bad;
    ibz_set(&n, 7);
    res = res || ibz_cornacchia_extended(&x, &y, &n, &params);
    ibz_set(&n, 3);
    res = res || ibz_cornacchia_extended(&x, &y, &n, &params);
    ibz_set(&n, 6);
    params.bad_primes_prod = NULL;
    res = res || ibz_cornacchia_extended(&x, &y, &n, &params);
    ibz_set(&n, 30);
    params.bad_primes_prod = &bad;
    res = res || ibz_cornacchia_extended(&x, &y, &n, &params);
    ibz_set(&n, 30 * 1381);
    res = res || ibz_cornacchia_extended(&x, &y, &n, &params);

    // tests with q=3
    ibz_set(&bad, 2 * 5 * 11 * 17);
    ibz_set(&n, 7);
    params.q = 3;
    if (ibz_cornacchia_extended(&x, &y, &n, &params)) {
        ibz_mul(&c_res, &x, &x);
        ibz_set(&prod, 3);
        ibz_mul(&prod, &prod, &y);
        ibz_mul(&prod, &prod, &y);
        ibz_add(&c_res, &c_res, &prod);
        res = res || ibz_cmp(&n, &c_res);
    } else {
        res = 1;
    }
    ibz_set(&n, 3 * 7 * 31);
    params.bad_primes_prod = NULL;
    if (ibz_cornacchia_extended(&x, &y, &n, &params)) {
        ibz_mul(&c_res, &x, &x);
        ibz_set(&prod, 3);
        ibz_mul(&prod, &prod, &y);
        ibz_mul(&prod, &prod, &y);
        ibz_add(&c_res, &c_res, &prod);
        res = res || ibz_cmp(&n, &c_res);
    } else {
        res = 1;
    }
    ibz_set(&n, 7 * 7 * 31);
    ibz_set(&prod, 547);
    ibz_mul(&n, &n, &prod);
    if (ibz_cornacchia_extended(&x, &y, &n, &params)) {
        ibz_mul(&c_res, &x, &x);
        ibz_set(&prod, 3);
        ibz_mul(&prod, &prod, &y);
        ibz_mul(&prod, &prod, &y);
        ibz_add(&c_res, &c_res, &prod);
        res = res || ibz_cmp(&n, &c_res);
    } else {
        res = 1;
    }

    ibz_set(&n, 5);
    params.bad_primes_prod = &bad;
    res = res || ibz_cornacchia_extended(&x, &y, &n, &params);
    ibz_set(&n, 5 * 7 * 31);
    res = res || ibz_cornacchia_extended(&x, &y, &n, &params);
    ibz_set(&n, 547 * 11);
    res = res || ibz_cornacchia_extended(&x, &y, &n, &params);
    ibz_set(&n, 547 * 2);
    res = res || ibz_cornacchia_extended(&x, &y, &n, &params);
    ibz_set(&n, 107);
    res = res || ibz_cornacchia_extended(&x, &y, &n, &params);

    if (res != 0) {
        printf("Quaternion unit test integer_ibz_cornacchia_extended failed\n");
    }
    ibz_finalize(&x);
    ibz_finalize(&y);
    ibz_finalize(&n);
    ibz_finalize(&prod);
    ibz_finalize(&c_res);
    ibz_finalize(&bad);
    return res;
}

// run all previous tests
int
quat_test_integers(void)
{
    int res = 0;
    printf("\nRunning quaternion tests of integer functions\n");
    res = res | quat_test_ibz_generate_random_prime();
    res = res | quat_test_integer_ibz_cornacchia_prime();
    res = res | quat_test_integer_ibz_complex_mul();
    res = res | quat_test_integer_ibz_complex_mul_by_complex_power();
    res = res | quat_test_integer_ibz_cornacchia_extended_prime_loop();
    res = res | quat_test_integer_ibz_cornacchia_prime();
    res = res | quat_test_integer_ibz_cornacchia_extended();
    return (res);
}
