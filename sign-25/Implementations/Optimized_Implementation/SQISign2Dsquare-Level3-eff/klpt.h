/** @file
 *
 * @authors Antonin Leroux
 *
 * @brief The norm equation algorithms
 */

#ifndef KLPT_H
#define KLPT_H

#include <intbig.h>
#include <quaternion.h>
#include <klpt_constants.h>
#include <quaternion_data.h>
#include <torsion_constants.h>
#include <stdio.h>

/*************************** Functions *****************************/

/** @defgroup klpt_klpt Functions and types for KLPT
 * @{
 */

/** @defgroup klpt_extremals Init for extremal orders
 * @{
 */
static int
generate_random_prime(ibz_t *p, int is3mod4, int bitsize)
{
    int found = 0;
    ibz_t two_pow, two_powp;

    ibz_init(&two_pow);
    ibz_init(&two_powp);
    ibz_pow(&two_pow, &ibz_const_two, bitsize);
    ibz_pow(&two_powp, &ibz_const_two, bitsize + 1);

    int cnt = 0;
    while (!found && cnt < KLPT_random_prime_attempts * bitsize) {
        cnt++;
        ibz_rand_interval(p, &two_pow, &two_powp);

        found = ibz_probab_prime(p, 30) && (!is3mod4 || (ibz_get(p) % 4 == 3));
    }
    ibz_finalize(&two_pow);
    ibz_finalize(&two_powp);
    return found;
}

static int generate_random_prime_kygen(ibz_t* p, ibz_t* bound)
{
    int found = 0;
    int bitsize = ibz_bitsize(bound);

    int cnt = 0;
    while (!found && cnt < KLPT_random_prime_attempts * bitsize) {
        cnt++;
        ibz_rand_interval(p, &ibz_const_two, bound);

        found = ibz_probab_prime(p, KLPT_primality_num_iter);
    }
    return found;
}

void quat_alg_elem_copy(quat_alg_elem_t *copy, const quat_alg_elem_t *copied);
void quat_left_ideal_copy(quat_left_ideal_t *copy, const quat_left_ideal_t *copied);

/**
 * @brief Representing an integer by the quadratic norm form of a maximal extremal order
 *
 * @param gamma Output: a quaternion element
 * @param n_gamma Outut: norm of gamma (also part of the input, it is the target norm a multiple of
 * the final norm)
 * @param Bpoo the quaternion algebra
 * @return 1 if the computation succeeded
 *
 * This algorithm finds a primitive quaternion element gamma of n_gamma inside the standard maximal
 * extremal order with a special property used in fixed degree isogeny
 */
int represent_integer(quat_alg_elem_t *gamma, ibz_t *n_gamma, const quat_alg_t *Bpoo);
int represent_integer_non_diag(quat_alg_elem_t *gamma, ibz_t *n_gamma, const quat_alg_t *Bpoo);
int represent_integer_exact(quat_alg_elem_t *gamma, ibz_t *n_gamma, ibz_t *content, const quat_alg_t *Bpoo);

#endif
