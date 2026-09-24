#include "curve_extras.h"
#include <ec_params.h>
#include <assert.h>
#include <stdio.h>

void
ec_normalize_point(ec_point_t *P)
{
    fp2_inv(&P->z);
    fp2_mul(&P->x, &P->x, &P->z);
    fp2_set_one(&(P->z));
}

void
ec_normalize_curve(ec_curve_t *E)
{
    fp2_inv(&E->C);
    fp2_mul(&E->A, &E->A, &E->C);
    fp2_set_one(&E->C);
}

void
ec_curve_normalize_A24(ec_curve_t *E)
{
    if (E->is_A24_computed_and_normalized == 0) {
        AC_to_A24(&E->A24, E);
        ec_normalize_point(&E->A24);
        E->is_A24_computed_and_normalized = 1;
    }
}

void
ec_normalize_curve_and_A24(ec_curve_t *E)
{ // Neither the curve or A24 are guaranteed to be normalized.
  // First we normalize (A/C : 1) and conditionally compute
	if (!fp2_is_one(&E->C)) {
		ec_normalize_curve(E);
	}

	if (!E->is_A24_computed_and_normalized) {
		AC_to_A24(&E->A24, E);
		ec_normalize_point(&E->A24);
		E->is_A24_computed_and_normalized = 1;
		//// Now compute A24 = ((A + 2) / 4 : 1)
		//fp2_add_one(&E->A24.x, &E->A);     // re(A24.x) = re(A) + 1
		//fp2_add_one(&E->A24.x, &E->A24.x); // re(A24.x) = re(A) + 2
		//fp_copy(&E->A24.x.im, &E->A.im);   // im(A24.x) = im(A)

		//fp2_half(&E->A24.x, &E->A24.x); // (A + 2) / 2
		//fp2_half(&E->A24.x, &E->A24.x); // (A + 2) / 4
		//fp2_set_one(&E->A24.z);

		//E->is_A24_computed_and_normalized = true;
	}
}

bool
ec_is_zero(ec_point_t const *P)
{
    return fp2_is_zero(&P->z);
}

void
ec_point_init(ec_point_t *P)
{ // Initialize point as identity element (1:0)
    fp2_set_one(&(P->x));
    fp2_set_zero(&(P->z));
}

// Initialise the curve struct
void
ec_curve_init(ec_curve_t *E)
{
    // Initialise the constants
    fp2_set_zero(&(E->A));
    fp2_set_one(&(E->C));

    // Initalise the point (A+2 : 4C)
    ec_point_init(&(E->A24));

    // Set the bool to be false by default
    E->is_A24_computed_and_normalized = 0;
}

// TODO: this is simply ec_point_init, we don't need both?
void
ec_set_zero(ec_point_t *P)
{
    fp2_set_one(&(P->x));
    fp2_set_zero(&(P->z));
    return;
}

void
copy_curve(ec_curve_t *E1, ec_curve_t const *E2)
{
    fp2_copy(&(E1->A), &(E2->A));
    fp2_copy(&(E1->C), &(E2->C));
    E1->is_A24_computed_and_normalized = E2->is_A24_computed_and_normalized;
    copy_point(&E1->A24, &E2->A24);
}

void
xDBL(ec_point_t *Q, ec_point_t const *P, ec_point_t const *AC)
{
    // This version computes the coefficient values A+2C and 4C on-the-fly
    // The curve coefficients are passed via AC = (A:C)
    fp2_t t0, t1, t2, t3;

    fp2_add(&t0, &P->x, &P->z);
    fp2_sqr(&t0, &t0);
    fp2_sub(&t1, &P->x, &P->z);
    fp2_sqr(&t1, &t1);
    fp2_sub(&t2, &t0, &t1);
    fp2_add(&t3, &AC->z, &AC->z);
    fp2_mul(&t1, &t1, &t3);
    fp2_add(&t1, &t1, &t1);
    fp2_mul(&Q->x, &t0, &t1);
    fp2_add(&t0, &t3, &AC->x);
    fp2_mul(&t0, &t0, &t2);
    fp2_add(&t0, &t0, &t1);
    fp2_mul(&Q->z, &t0, &t2);
}

void
xDBL_A24(ec_point_t *Q, ec_point_t const *P, ec_point_t const *A24)
{
    // This version receives the coefficient value A24 = (A+2C:4C)
    fp2_t t0, t1, t2;

    fp2_add(&t0, &P->x, &P->z);
    fp2_sqr(&t0, &t0);
    fp2_sub(&t1, &P->x, &P->z);
    fp2_sqr(&t1, &t1);
    fp2_sub(&t2, &t0, &t1);
    fp2_mul(&t1, &t1, &A24->z);
    fp2_mul(&Q->x, &t0, &t1);
    fp2_mul(&t0, &t2, &A24->x);
    fp2_add(&t0, &t0, &t1);
    fp2_mul(&Q->z, &t0, &t2);
}

void
xDBL_A24_normalized(ec_point_t *Q, ec_point_t const *P, ec_point_t const *A24)
{
    // This version receives the coefficient value A24 = (A+2C/4C:1)
    fp2_t t0, t1, t2;

    fp2_add(&t0, &P->x, &P->z);
    fp2_sqr(&t0, &t0);
    fp2_sub(&t1, &P->x, &P->z);
    fp2_sqr(&t1, &t1);
    fp2_sub(&t2, &t0, &t1);
    // fp2_mul(&t1, &t1, &A24->z);
    fp2_mul(&Q->x, &t0, &t1);
    fp2_mul(&t0, &t2, &A24->x);
    fp2_add(&t0, &t0, &t1);
    fp2_mul(&Q->z, &t0, &t2);
}

void
xADD(ec_point_t *R, ec_point_t const *P, ec_point_t const *Q, ec_point_t const *PQ)
{
    fp2_t t0, t1, t2, t3;

    fp2_add(&t0, &P->x, &P->z);
    fp2_sub(&t1, &P->x, &P->z);
    fp2_add(&t2, &Q->x, &Q->z);
    fp2_sub(&t3, &Q->x, &Q->z);
    fp2_mul(&t0, &t0, &t3);
    fp2_mul(&t1, &t1, &t2);
    fp2_add(&t2, &t0, &t1);
    fp2_sub(&t3, &t0, &t1);
    fp2_sqr(&t2, &t2);
    fp2_sqr(&t3, &t3);
    fp2_mul(&t2, &PQ->z, &t2);
    fp2_mul(&R->z, &PQ->x, &t3);
    fp2_copy(&R->x, &t2);
}

void
xDBLADD(ec_point_t *R,
        ec_point_t *S,
        ec_point_t const *P,
        ec_point_t const *Q,
        ec_point_t const *PQ,
        ec_point_t const *A24)
{
    // Requires precomputation of A24 = (A+2C:4C)
    fp2_t t0, t1, t2;

    fp2_add(&t0, &P->x, &P->z);
    fp2_sub(&t1, &P->x, &P->z);
    fp2_sqr(&R->x, &t0);
    fp2_sub(&t2, &Q->x, &Q->z);
    fp2_add(&S->x, &Q->x, &Q->z);
    fp2_mul(&t0, &t0, &t2);
    fp2_sqr(&R->z, &t1);
    fp2_mul(&t1, &t1, &S->x);
    fp2_sub(&t2, &R->x, &R->z);
    fp2_mul(&R->z, &R->z, &A24->z);
    fp2_mul(&R->x, &R->x, &R->z);
    fp2_mul(&S->x, &A24->x, &t2);
    fp2_sub(&S->z, &t0, &t1);
    fp2_add(&R->z, &R->z, &S->x);
    fp2_add(&S->x, &t0, &t1);
    fp2_mul(&R->z, &R->z, &t2);
    fp2_sqr(&S->z, &S->z);
    fp2_sqr(&S->x, &S->x);
    fp2_mul(&S->z, &S->z, &PQ->x);
    fp2_mul(&S->x, &S->x, &PQ->z);
}

// assume that A24 has been normalized
void
xDBLADD_normalized(ec_point_t *R,
                   ec_point_t *S,
                   ec_point_t const *P,
                   ec_point_t const *Q,
                   ec_point_t const *PQ,
                   ec_point_t const *A24)
{
    // Requires precomputation of A24 = (A+2C:4C)
    fp2_t t0, t1, t2;

    fp2_add(&t0, &P->x, &P->z);
    fp2_sub(&t1, &P->x, &P->z);
    fp2_sqr(&R->x, &t0);
    fp2_sub(&t2, &Q->x, &Q->z);
    fp2_add(&S->x, &Q->x, &Q->z);
    fp2_mul(&t0, &t0, &t2);
    fp2_sqr(&R->z, &t1);
    fp2_mul(&t1, &t1, &S->x);
    fp2_sub(&t2, &R->x, &R->z);
    // fp2_mul(&R->z, &R->z, &A24->z);
    fp2_mul(&R->x, &R->x, &R->z);
    fp2_mul(&S->x, &A24->x, &t2);
    fp2_sub(&S->z, &t0, &t1);
    fp2_add(&R->z, &R->z, &S->x);
    fp2_add(&S->x, &t0, &t1);
    fp2_mul(&R->z, &R->z, &t2);
    fp2_sqr(&S->z, &S->z);
    fp2_sqr(&S->x, &S->x);
    fp2_mul(&S->z, &S->z, &PQ->x);
    fp2_mul(&S->x, &S->x, &PQ->z);
}

bool
is_point_equal(const ec_point_t *P, const ec_point_t *Q)
{ // Evaluate if two points in Montgomery coordinates (X:Z) are equal
  // Returns 1 (true) if P=Q, 0 (false) otherwise
    fp2_t t0, t1;

    if ((fp2_is_zero(&P->x) && fp2_is_zero(&P->z)) || (fp2_is_zero(&Q->x) && fp2_is_zero(&Q->z))) {
        return fp2_is_zero(&P->x) && fp2_is_zero(&P->z) && fp2_is_zero(&Q->x) && fp2_is_zero(&Q->z);
    }

    fp2_mul(&t0, &P->x, &Q->z);
    fp2_mul(&t1, &Q->x, &P->z);
    fp2_sub(&t0, &t0, &t1);

    return fp2_is_zero(&t0);
}

void
select_point(ec_point_t *Q, ec_point_t const *P1, ec_point_t const *P2, const digit_t option)
{ // Select points
  // If option = 0 then Q <- P1, else if option = 0xFF...FF then Q <- P2
    fp2_select(&(Q->x), &(P1->x), &(P2->x), option);
    fp2_select(&(Q->z), &(P1->z), &(P2->z), option);
}

void
swap_points(ec_point_t *P, ec_point_t *Q, const digit_t option)
{ // Swap points
  // If option = 0 then P <- P and Q <- Q, else if option = 0xFF...FF then P <- Q and Q <- P
    fp2_cswap(&(P->x), &(Q->x), option);
    fp2_cswap(&(P->z), &(Q->z), option);
}

void
copy_point(ec_point_t *P, ec_point_t const *Q)
{
    fp2_copy(&(P->x), &(Q->x));
    fp2_copy(&(P->z), &(Q->z));
}

void
ec_neg(ec_point_t *res, const ec_point_t *P)
{
    // DOES NOTHING
    copy_point(res, P);
}

void
xMUL(ec_point_t *Q, ec_point_t const *P, digit_t const *k, ec_curve_t const *curve)
{
    ec_point_t R0, R1, A24;
    digit_t mask;
    unsigned int bit = 0, prevbit = 0, swap;

    fp2_add(&A24.x, &curve->C, &curve->C); // Precomputation of A24=(A+2C:4C)
    fp2_add(&A24.z, &A24.x, &A24.x);
    fp2_add(&A24.x, &A24.x, &curve->A);

    // R0 <- (1:0), R1 <- P
    ec_point_init(&R0);
    fp2_copy(&R1.x, &P->x);
    fp2_copy(&R1.z, &P->z);

    // Main loop
    for (int i = BITS - 1; i >= 0; i--) {
        bit = (k[i >> LOG2RADIX] >> (i & (RADIX - 1))) & 1;
        swap = bit ^ prevbit;
        prevbit = bit;
        mask = 0 - (digit_t)swap;

        swap_points(&R0, &R1, mask);
        xDBLADD(&R0, &R1, &R0, &R1, P, &A24);
    }
    swap = 0 ^ prevbit;
    mask = 0 - (digit_t)swap;
    swap_points(&R0, &R1, mask);

    fp2_copy(&Q->x, &R0.x);
    fp2_copy(&Q->z, &R0.z);
}

void
xMULv2(ec_point_t *Q, ec_point_t const *P, digit_t const *k, const int kbits, ec_point_t const *A24)
{
    // This version receives the coefficient value A24 = (A+2C:4C)
    ec_point_t R0, R1;
    digit_t mask;
    unsigned int bit = 0, prevbit = 0, swap;

    // R0 <- (1:0), R1 <- P
    ec_point_init(&R0);
    fp2_copy(&R1.x, &P->x);
    fp2_copy(&R1.z, &P->z);

    // Main loop
    for (int i = kbits - 1; i >= 0; i--) {
        bit = (k[i >> LOG2RADIX] >> (i & (RADIX - 1))) & 1;
        swap = bit ^ prevbit;
        prevbit = bit;
        mask = 0 - (digit_t)swap;

        swap_points(&R0, &R1, mask);
        xDBLADD(&R0, &R1, &R0, &R1, P, A24);
    }
    swap = 0 ^ prevbit;
    mask = 0 - (digit_t)swap;
    swap_points(&R0, &R1, mask);

    fp2_copy(&Q->x, &R0.x);
    fp2_copy(&Q->z, &R0.z);
}

// Compute S = k*P + l*Q, with PQ = P+Q
void
xDBLMUL(ec_point_t *S,
        ec_point_t const *P,
        digit_t const *k,
        ec_point_t const *Q,
        digit_t const *l,
        ec_point_t const *PQ,
        const ec_curve_t *curve)
{

    int i;
    digit_t evens, mevens, bitk0, bitl0, maskk, maskl, temp, bs1_ip1, bs2_ip1, bs1_i, bs2_i, h;
    digit_t sigma[2] = { 0 }, pre_sigma = 0;
    digit_t k_t[NWORDS_ORDER_2], l_t[NWORDS_ORDER_2], one[NWORDS_ORDER_2] = { 0 }, r[2 * BITS_2] = { 0 };
    ec_point_t A24, DIFF1a, DIFF1b, DIFF2a, DIFF2b, R[3] = { 0 }, T[3];

    // Derive sigma according to parity
    bitk0 = (k[0] & 1);
    bitl0 = (l[0] & 1);
    maskk = 0 - bitk0; // Parity masks: 0 if even, otherwise 1...1
    maskl = 0 - bitl0;
    sigma[0] = (bitk0 ^ 1);
    sigma[1] = (bitl0 ^ 1);
    evens = sigma[0] + sigma[1]; // Count number of even scalars
    mevens =
        0 - (evens & 1); // Mask mevens <- 0 if # even scalars = 0 or 2, otherwise mevens = 1...1

    // If k and l are both even or both odd, pick sigma = (0,1)
    sigma[0] = (sigma[0] & mevens);
    sigma[1] = (sigma[1] & mevens) | (1 & ~mevens);

    // Convert even scalars to odd
    one[0] = 1;
    mp_sub(k_t, k, one, NWORDS_ORDER_2);
    mp_sub(l_t, l, one, NWORDS_ORDER_2);
    select_ct(k_t, k_t, k, maskk, NWORDS_ORDER_2);
    select_ct(l_t, l_t, l, maskl, NWORDS_ORDER_2);

    // Scalar recoding
    for (i = 0; i < BITS_2; i++) {
        // If sigma[0] = 1 swap k_t and l_t
        maskk = 0 - (sigma[0] ^ pre_sigma);
        swap_ct(k_t, l_t, maskk, NWORDS_ORDER_2);

        if (i == BITS_2 - 1) {
            bs1_ip1 = 0;
            bs2_ip1 = 0;
        } else {
            bs1_ip1 = mp_shiftr(k_t, 1, NWORDS_ORDER_2);
            bs2_ip1 = mp_shiftr(l_t, 1, NWORDS_ORDER_2);
        }
        bs1_i = k_t[0] & 1;
        bs2_i = l_t[0] & 1;

        r[2 * i] = bs1_i ^ bs1_ip1;
        r[2 * i + 1] = bs2_i ^ bs2_ip1;

        // Revert sigma if second bit, r_(2i+1), is 1
        pre_sigma = sigma[0];
        maskk = 0 - r[2 * i + 1];
        select_ct(&temp, &sigma[0], &sigma[1], maskk, 1);
        select_ct(&sigma[1], &sigma[1], &sigma[0], maskk, 1);
        sigma[0] = temp;
    }

    // Point initialization
    ec_point_init(&R[0]);
    maskk = 0 - sigma[0];
    select_point(&R[1], P, Q, maskk);
    select_point(&R[2], Q, P, maskk);

    fp2_copy(&DIFF1a.x, &R[1].x);
    fp2_copy(&DIFF1a.z, &R[1].z);
    fp2_copy(&DIFF1b.x, &R[2].x);
    fp2_copy(&DIFF1b.z, &R[2].z);

    // Initialize DIFF2a <- P+Q, DIFF2b <- P-Q
    xADD(&R[2], &R[1], &R[2], PQ);
    fp2_copy(&DIFF2a.x, &R[2].x);
    fp2_copy(&DIFF2a.z, &R[2].z);
    fp2_copy(&DIFF2b.x, &PQ->x);
    fp2_copy(&DIFF2b.z, &PQ->z);

    // fp2_add(&A24.x, &curve->C, &curve->C);    // Precomputation of A24=(A+2C:4C)
    // fp2_add(&A24.z, &A24.x, &A24.x);
    // fp2_add(&A24.x, &A24.x, &curve->A);

    // normalizing
    ec_curve_t E;
    copy_curve(&E, curve);
    ec_curve_normalize_A24(&E);
    copy_point(&A24, &E.A24);

    // Main loop
    for (i = BITS_2 - 1; i >= 0; i--) {
        h = r[2 * i] + r[2 * i + 1]; // in {0, 1, 2}
        maskk = 0 - (h & 1);
        select_point(&T[0], &R[0], &R[1], maskk);
        maskk = 0 - (h >> 1);
        select_point(&T[0], &T[0], &R[2], maskk);
        xDBL_A24_normalized(&T[0], &T[0], &A24);

        maskk = 0 - r[2 * i + 1]; // in {0, 1}
        select_point(&T[1], &R[0], &R[1], maskk);
        select_point(&T[2], &R[1], &R[2], maskk);

        swap_points(&DIFF1a, &DIFF1b, maskk);
        xADD(&T[1], &T[1], &T[2], &DIFF1a);
        xADD(&T[2], &R[0], &R[2], &DIFF2a);

        // If hw (mod 2) = 1 then swap DIFF2a and DIFF2b
        maskk = 0 - (h & 1);
        swap_points(&DIFF2a, &DIFF2b, maskk);

        // R <- T
        copy_point(&R[0], &T[0]);
        copy_point(&R[1], &T[1]);
        copy_point(&R[2], &T[2]);
    }

    // Output R[evens]
    select_point(S, &R[0], &R[1], mevens);

    maskk = 0 - (bitk0 & bitl0);
    select_point(S, S, &R[2], maskk);
}

void
xDBLMUL_3(ec_point_t* S,
    ec_point_t const* P,
    digit_t const* k,
    ec_point_t const* Q,
    digit_t const* l,
    ec_point_t const* PQ,
    const ec_curve_t* curve)
{

    int i;
    digit_t evens, mevens, bitk0, bitl0, maskk, maskl, temp, bs1_ip1, bs2_ip1, bs1_i, bs2_i, h;
    digit_t sigma[2] = { 0 }, pre_sigma = 0;
    digit_t k_t[NWORDS_ORDER_3], l_t[NWORDS_ORDER_3], one[NWORDS_ORDER_3] = { 0 }, r[2 * BITS_3] = { 0 };
    ec_point_t A24, DIFF1a, DIFF1b, DIFF2a, DIFF2b, R[3] = { 0 }, T[3];

    // Derive sigma according to parity
    bitk0 = (k[0] & 1);
    bitl0 = (l[0] & 1);
    maskk = 0 - bitk0; // Parity masks: 0 if even, otherwise 1...1
    maskl = 0 - bitl0;
    sigma[0] = (bitk0 ^ 1);
    sigma[1] = (bitl0 ^ 1);
    evens = sigma[0] + sigma[1]; // Count number of even scalars
    mevens =
        0 - (evens & 1); // Mask mevens <- 0 if # even scalars = 0 or 2, otherwise mevens = 1...1

    // If k and l are both even or both odd, pick sigma = (0,1)
    sigma[0] = (sigma[0] & mevens);
    sigma[1] = (sigma[1] & mevens) | (1 & ~mevens);

    // Convert even scalars to odd
    if (memcmp(k, one, NWORDS_ORDER_3 * 8) == 0) {
        memcpy(k_t, THREEpF, NWORDS_ORDER_3 * 8);
        maskk = -1;
    }
    else {
        one[0] = 1;
        mp_sub(k_t, k, one, NWORDS_ORDER_3);
        one[0] = 0;
    }
    if (memcmp(l, one, NWORDS_ORDER_3 * 8) == 0) {
        memcpy(l_t, THREEpF, NWORDS_ORDER_3 * 8);
        maskl = -1;
    }
    else {
        one[0] = 1;
        mp_sub(l_t, l, one, NWORDS_ORDER_3);
    }

    select_ct(k_t, k_t, k, maskk, NWORDS_ORDER_3);
    select_ct(l_t, l_t, l, maskl, NWORDS_ORDER_3);

    // Scalar recoding
    for (i = 0; i < BITS_3; i++) {
        // If sigma[0] = 1 swap k_t and l_t
        maskk = 0 - (sigma[0] ^ pre_sigma);
        swap_ct(k_t, l_t, maskk, NWORDS_ORDER_3);

        if (i == BITS_3 - 1) {
            bs1_ip1 = 0;
            bs2_ip1 = 0;
        }
        else {
            bs1_ip1 = mp_shiftr(k_t, 1, NWORDS_ORDER_3);
            bs2_ip1 = mp_shiftr(l_t, 1, NWORDS_ORDER_3);
        }
        bs1_i = k_t[0] & 1;
        bs2_i = l_t[0] & 1;

        r[2 * i] = bs1_i ^ bs1_ip1;
        r[2 * i + 1] = bs2_i ^ bs2_ip1;

        // Revert sigma if second bit, r_(2i+1), is 1
        pre_sigma = sigma[0];
        maskk = 0 - r[2 * i + 1];
        select_ct(&temp, &sigma[0], &sigma[1], maskk, 1);
        select_ct(&sigma[1], &sigma[1], &sigma[0], maskk, 1);
        sigma[0] = temp;
    }

    // Point initialization
    ec_point_init(&R[0]);
    maskk = 0 - sigma[0];
    select_point(&R[1], P, Q, maskk);
    select_point(&R[2], Q, P, maskk);

    fp2_copy(&DIFF1a.x, &R[1].x);
    fp2_copy(&DIFF1a.z, &R[1].z);
    fp2_copy(&DIFF1b.x, &R[2].x);
    fp2_copy(&DIFF1b.z, &R[2].z);

    // Initialize DIFF2a <- P+Q, DIFF2b <- P-Q
    xADD(&R[2], &R[1], &R[2], PQ);
    fp2_copy(&DIFF2a.x, &R[2].x);
    fp2_copy(&DIFF2a.z, &R[2].z);
    fp2_copy(&DIFF2b.x, &PQ->x);
    fp2_copy(&DIFF2b.z, &PQ->z);

    // fp2_add(&A24.x, &curve->C, &curve->C);    // Precomputation of A24=(A+2C:4C)
    // fp2_add(&A24.z, &A24.x, &A24.x);
    // fp2_add(&A24.x, &A24.x, &curve->A);

    // normalizing
    ec_curve_t E;
    copy_curve(&E, curve);
    ec_curve_normalize_A24(&E);
    copy_point(&A24, &E.A24);

    // Main loop
    for (i = BITS_3 - 1; i >= 0; i--) {
        h = r[2 * i] + r[2 * i + 1]; // in {0, 1, 2}
        maskk = 0 - (h & 1);
        select_point(&T[0], &R[0], &R[1], maskk);
        maskk = 0 - (h >> 1);
        select_point(&T[0], &T[0], &R[2], maskk);
        xDBL_A24_normalized(&T[0], &T[0], &A24);

        maskk = 0 - r[2 * i + 1]; // in {0, 1}
        select_point(&T[1], &R[0], &R[1], maskk);
        select_point(&T[2], &R[1], &R[2], maskk);

        swap_points(&DIFF1a, &DIFF1b, maskk);
        xADD(&T[1], &T[1], &T[2], &DIFF1a);
        xADD(&T[2], &R[0], &R[2], &DIFF2a);

        // If hw (mod 2) = 1 then swap DIFF2a and DIFF2b
        maskk = 0 - (h & 1);
        swap_points(&DIFF2a, &DIFF2b, maskk);

        // R <- T
        copy_point(&R[0], &T[0]);
        copy_point(&R[1], &T[1]);
        copy_point(&R[2], &T[2]);
    }

    select_point(S, &R[0], &R[1], mevens);

    maskk = 0 - (bitk0 & bitl0);
    select_point(S, S, &R[2], maskk);
}

void
xDBLMUL_bounded(ec_point_t *S,
                ec_point_t const *P,
                digit_t const *k,
                ec_point_t const *Q,
                digit_t const *l,
                ec_point_t const *PQ,
                const ec_curve_t *curve,
                int f)
{

    int i;
    digit_t evens, mevens, bitk0, bitl0, maskk, maskl, temp, bs1_ip1, bs2_ip1, bs1_i, bs2_i, h;
    digit_t sigma[2] = { 0 }, pre_sigma = 0;
    digit_t k_t[NWORDS_ORDER_2], l_t[NWORDS_ORDER_2], one[NWORDS_ORDER_2] = { 0 }, r[2 * BITS_2] = { 0 };
    ec_point_t A24, DIFF1a, DIFF1b, DIFF2a, DIFF2b, R[3] = { 0 }, T[3];

    // Derive sigma according to parity
    bitk0 = (k[0] & 1);
    bitl0 = (l[0] & 1);
    maskk = 0 - bitk0; // Parity masks: 0 if even, otherwise 1...1
    maskl = 0 - bitl0;
    sigma[0] = (bitk0 ^ 1);
    sigma[1] = (bitl0 ^ 1);
    evens = sigma[0] + sigma[1]; // Count number of even scalars
    mevens =
        0 - (evens & 1); // Mask mevens <- 0 if # even scalars = 0 or 2, otherwise mevens = 1...1

    // If k and l are both even or both odd, pick sigma = (0,1)
    sigma[0] = (sigma[0] & mevens);
    sigma[1] = (sigma[1] & mevens) | (1 & ~mevens);

    // Convert even scalars to odd
    one[0] = 1;
    mp_sub(k_t, k, one, NWORDS_ORDER_2);
    mp_sub(l_t, l, one, NWORDS_ORDER_2);
    select_ct(k_t, k_t, k, maskk, NWORDS_ORDER_2);
    select_ct(l_t, l_t, l, maskl, NWORDS_ORDER_2);

    // Scalar recoding
    for (i = 0; i < BITS_2; i++) {
        // If sigma[0] = 1 swap k_t and l_t
        maskk = 0 - (sigma[0] ^ pre_sigma);
        swap_ct(k_t, l_t, maskk, NWORDS_ORDER_2);

        if (i == BITS_2 - 1) {
            bs1_ip1 = 0;
            bs2_ip1 = 0;
        } else {
            bs1_ip1 = mp_shiftr(k_t, 1, NWORDS_ORDER_2);
            bs2_ip1 = mp_shiftr(l_t, 1, NWORDS_ORDER_2);
        }
        bs1_i = k_t[0] & 1;
        bs2_i = l_t[0] & 1;

        r[2 * i] = bs1_i ^ bs1_ip1;
        r[2 * i + 1] = bs2_i ^ bs2_ip1;

        // Revert sigma if second bit, r_(2i+1), is 1
        pre_sigma = sigma[0];
        maskk = 0 - r[2 * i + 1];
        select_ct(&temp, &sigma[0], &sigma[1], maskk, 1);
        select_ct(&sigma[1], &sigma[1], &sigma[0], maskk, 1);
        sigma[0] = temp;
    }

    // Point initialization
    ec_point_init(&R[0]);
    maskk = 0 - sigma[0];
    select_point(&R[1], P, Q, maskk);
    select_point(&R[2], Q, P, maskk);

    fp2_copy(&DIFF1a.x, &R[1].x);
    fp2_copy(&DIFF1a.z, &R[1].z);
    fp2_copy(&DIFF1b.x, &R[2].x);
    fp2_copy(&DIFF1b.z, &R[2].z);

    // Initialize DIFF2a <- P+Q, DIFF2b <- P-Q
    xADD(&R[2], &R[1], &R[2], PQ);
    fp2_copy(&DIFF2a.x, &R[2].x);
    fp2_copy(&DIFF2a.z, &R[2].z);
    fp2_copy(&DIFF2b.x, &PQ->x);
    fp2_copy(&DIFF2b.z, &PQ->z);

    // fp2_add(&A24.x, &curve->C, &curve->C);    // Precomputation of A24=(A+2C:4C)
    // fp2_add(&A24.z, &A24.x, &A24.x);
    // fp2_add(&A24.x, &A24.x, &curve->A);

    // normalizing
    ec_curve_t E;
    copy_curve(&E, curve);
    ec_curve_normalize_A24(&E);
    copy_point(&A24, &E.A24);

    // T <- R
    copy_point(&T[0], &R[0]);
    copy_point(&T[1], &R[1]);
    copy_point(&T[2], &R[2]);

    // Main loop
    for (i = BITS_2 - 1; i >= 0; i--) {

        // TODO : clean this
        // this is an ugly fix to avoid unnecessary operations
        bool apply = (i <= f + 2 + (BITS_2 - TORSION_PLUS_EVEN_POWER));

        h = r[2 * i] + r[2 * i + 1]; // in {0, 1, 2}
        maskk = 0 - (h & 1);
        if (apply) {
            select_point(&T[0], &R[0], &R[1], maskk);
        }

        maskk = 0 - (h >> 1);

        if (apply) {
            select_point(&T[0], &T[0], &R[2], maskk);
            xDBL_A24_normalized(&T[0], &T[0], &A24);
        }

        maskk = 0 - r[2 * i + 1]; // in {0, 1}
        if (apply) {
            select_point(&T[1], &R[0], &R[1], maskk);
            select_point(&T[2], &R[1], &R[2], maskk);
        }

        swap_points(&DIFF1a, &DIFF1b, maskk);
        if (apply) {
            xADD(&T[1], &T[1], &T[2], &DIFF1a);
            xADD(&T[2], &R[0], &R[2], &DIFF2a);
        }

        // If hw (mod 2) = 1 then swap DIFF2a and DIFF2b
        maskk = 0 - (h & 1);
        swap_points(&DIFF2a, &DIFF2b, maskk);

        // R <- T
        copy_point(&R[0], &T[0]);
        copy_point(&R[1], &T[1]);
        copy_point(&R[2], &T[2]);

    }

    // Output R[evens]
    select_point(S, &R[0], &R[1], mevens);

    maskk = 0 - (bitk0 & bitl0);
    select_point(S, S, &R[2], maskk);
}

void
ec_ladder3pt(ec_point_t *R,
             digit_t *const m,
             ec_point_t const *P,
             ec_point_t const *Q,
             ec_point_t const *PQ,
             ec_curve_t const *A)
{

    // Curve constant in the form A24=(A+2C:4C)
    // ec_point_t A24;
    // fp2_add(&A24.z, &A->C, &A->C);
    // fp2_add(&A24.x, &A->A, &A24.z);
    // fp2_add(&A24.z, &A24.z, &A24.z);

    ec_point_t X0, X1, X2;
    copy_point(&X0, Q);
    copy_point(&X1, P);
    copy_point(&X2, PQ);

    int i, j;
    uint64_t t;
    for (i = 0; i < NWORDS_ORDER_2; i++) {
        t = 1;
        for (j = 0; j < 64; j++) {
            swap_points(&X1, &X2, -((t & m[i]) == 0));
            xDBLADD_normalized(&X0, &X1, &X0, &X1, &X2, &A->A24);
            swap_points(&X1, &X2, -((t & m[i]) == 0));
            t <<= 1;
        };
    };
    copy_point(R, &X1);
}

void
ec_j_inv(fp2_t *j_inv, const ec_curve_t *curve)
{
    /* j-invariant computation for montgommery coefficient A2=(A+2C:4C) */
    fp2_t t0, t1;

    fp2_sqr(&t1, &curve->C);
    fp2_sqr(j_inv, &curve->A);
    fp2_add(&t0, &t1, &t1);
    fp2_sub(&t0, j_inv, &t0);
    fp2_sub(&t0, &t0, &t1);
    fp2_sub(j_inv, &t0, &t1);
    fp2_sqr(&t1, &t1);
    fp2_mul(j_inv, j_inv, &t1);
    fp2_add(&t0, &t0, &t0);
    fp2_add(&t0, &t0, &t0);
    fp2_sqr(&t1, &t0);
    fp2_mul(&t0, &t0, &t1);
    fp2_add(&t0, &t0, &t0);
    fp2_add(&t0, &t0, &t0);
    fp2_inv(j_inv);
    fp2_mul(j_inv, &t0, j_inv);
}

static void
jac_init(jac_point_t *P)
{ // Initialize Montgomery in Jacobian coordinates as identity element (0:1:0)
    fp2_set_zero(&(P->x));
    fp2_set_one(&(P->y));
    fp2_set_zero(&(P->z));
}

bool
is_jac_equal(const jac_point_t *P, const jac_point_t *Q)
{ // Evaluate if two points in Jacobian coordinates (X:Y:Z) are equal
  // Returns 1 (true) if P=Q, 0 (false) otherwise
    fp2_t t0, t1, t2, t3;

    fp2_sqr(&t0, &Q->z);
    fp2_mul(&t2, &P->x, &t0); // x1*z2^2
    fp2_sqr(&t1, &P->z);
    fp2_mul(&t3, &Q->x, &t1); // x2*z1^2
    fp2_sub(&t2, &t2, &t3);

    fp2_mul(&t0, &t0, &Q->z);
    fp2_mul(&t0, &P->y, &t0); // y1*z2^3
    fp2_mul(&t1, &t1, &P->z);
    fp2_mul(&t1, &Q->y, &t1); // y2*z1^3
    fp2_sub(&t0, &t0, &t1);

    return fp2_is_zero(&t0) && fp2_is_zero(&t2);
}

void
jac_to_xz(ec_point_t *P, const jac_point_t *xyP)
{
    fp2_copy(&P->x, &xyP->x);
    fp2_copy(&P->z, &xyP->z);
    fp2_sqr(&P->z, &P->z);
}

bool
is_jac_xz_equal(const jac_point_t *P, const ec_point_t *Q)
{ // Evaluate if point P in Jacobian coordinates is equal to Q in homogeneous projective coordinates
  // (X:Z) Comparison is up to sign (only compares X and Z coordinates) Returns 1 (true) if P=Q, 0
  // (false) otherwise
    fp2_t t0, t1;

    fp2_mul(&t0, &P->x, &Q->z); // x1*z2
    fp2_sqr(&t1, &P->z);
    fp2_mul(&t1, &Q->x, &t1); // x2*z1^2
    fp2_sub(&t0, &t0, &t1);

    return fp2_is_zero(&t0);
}

void
copy_jac_point(jac_point_t *P, jac_point_t const *Q)
{
    fp2_copy(&(P->x), &(Q->x));
    fp2_copy(&(P->y), &(Q->y));
    fp2_copy(&(P->z), &(Q->z));
}

void
jac_neg(jac_point_t *Q, jac_point_t const *P)
{
    fp2_copy(&Q->x, &P->x);
    fp2_neg(&Q->y, &P->y);
    fp2_copy(&Q->z, &P->z);
}

void
DBL(jac_point_t *Q, jac_point_t const *P, ec_curve_t const *AC)
{ // Doubling on a Montgomery curve, representation in Jacobian coordinates (X:Y:Z) corresponding to
  // (X/Z^2,Y/Z^3) This version receives the coefficient value A
    fp2_t t0, t1, t2, t3;

    if (fp2_is_zero(&P->x) && fp2_is_zero(&P->z)) {
        jac_init(Q);
        return;
    }

    fp2_sqr(&t0, &P->x); // t0 = x1^2
    fp2_add(&t1, &t0, &t0);
    fp2_add(&t0, &t0, &t1); // t0 = 3x1^2
    fp2_sqr(&t1, &P->z);    // t1 = z1^2
    fp2_mul(&t2, &P->x, &AC->A);
    fp2_add(&t2, &t2, &t2); // t2 = 2Ax1
    fp2_add(&t2, &t1, &t2); // t2 = 2Ax1+z1^2
    fp2_mul(&t2, &t1, &t2); // t2 = z1^2(2Ax1+z1^2)
    fp2_add(&t2, &t0, &t2); // t2 = alpha = 3x1^2 + z1^2(2Ax1+z1^2)
    fp2_mul(&Q->z, &P->y, &P->z);
    fp2_add(&Q->z, &Q->z, &Q->z); // z2 = 2y1z1
    fp2_sqr(&t0, &Q->z);
    fp2_mul(&t0, &t0, &AC->A); // t0 = 4Ay1^2z1^2
    fp2_sqr(&t1, &P->y);
    fp2_add(&t1, &t1, &t1);     // t1 = 2y1^2
    fp2_add(&t3, &P->x, &P->x); // t3 = 2x1
    fp2_mul(&t3, &t1, &t3);     // t3 = 4x1y1^2
    fp2_sqr(&Q->x, &t2);        // x2 = alpha^2
    fp2_sub(&Q->x, &Q->x, &t0); // x2 = alpha^2 - 4Ay1^2z1^2
    fp2_sub(&Q->x, &Q->x, &t3);
    fp2_sub(&Q->x, &Q->x, &t3); // x2 = alpha^2 - 4Ay1^2z1^2 - 8x1y1^2
    fp2_sub(&Q->y, &t3, &Q->x); // y2 = 4x1y1^2 - x2
    fp2_mul(&Q->y, &Q->y, &t2); // y2 = alpha(4x1y1^2 - x2)
    fp2_sqr(&t1, &t1);          // t1 = 4y1^4
    fp2_sub(&Q->y, &Q->y, &t1);
    fp2_sub(&Q->y, &Q->y, &t1); // y2 = alpha(4x1y1^2 - x2) - 8y1^4
}

void
ADD(jac_point_t *R, jac_point_t const *P, jac_point_t const *Q, ec_curve_t const *AC)
{ // Addition on a Montgomery curve, representation in Jacobian coordinates (X:Y:Z) corresponding to
  // (X/Z^2,Y/Z^3) This version receives the coefficient value A
    fp2_t t0, t1, t2, t3, t4, t5, t6;
    jac_point_t T;

    if (is_jac_equal(P, Q)) {
        DBL(R, P, AC);
        return;
    }
    jac_neg(&T, P);
    if (is_jac_equal(&T, Q)) {
        jac_init(R);
        return;
    }
    if (fp2_is_zero(&P->x) && fp2_is_zero(&P->z)) {
        copy_jac_point(R, Q);
        return;
    } else if (fp2_is_zero(&Q->x) && fp2_is_zero(&Q->z)) {
        copy_jac_point(R, P);
        return;
    }

    fp2_sqr(&t0, &P->z);        // t0 = z1^2
    fp2_mul(&t1, &t0, &P->z);   // t1 = z1^3
    fp2_sqr(&t2, &Q->z);        // t2 = z2^2
    fp2_mul(&t3, &t2, &Q->z);   // t3 = z2^3
    fp2_mul(&t1, &t1, &Q->y);   // t1 = y2z1^3
    fp2_mul(&t3, &t3, &P->y);   // t3 = y1z2^3
    fp2_sub(&t1, &t1, &t3);     // t1 = lambda1 = y2z1^3 - y1z2^3
    fp2_mul(&t0, &t0, &Q->x);   // t0 = x2z1^2
    fp2_mul(&t2, &t2, &P->x);   // t2 = x1z2^2
    fp2_sub(&t4, &t0, &t2);     // t4 = lambda3 = x2z1^2 - x1z2^2
    fp2_add(&t0, &t0, &t2);     // t0 = lambda2 = x2z1^2 + x1z2^2
    fp2_mul(&t5, &P->z, &Q->z); // t5 = z1z2
    fp2_mul(&R->z, &t4, &t5);   // z3 = z1z2*lambda3
    fp2_sqr(&t5, &t5);          // t5 = z1^2z2^2
    fp2_mul(&t5, &AC->A, &t5);  // t5 = Az1^2z2^2
    fp2_add(&t0, &t0, &t5);     // t0 = Az1^2z2^2 + lambda2
    fp2_sqr(&t6, &t4);          // t6 = lambda3^2
    fp2_mul(&t5, &t0, &t6);     // t5 = lambda3^2(Az1^2z2^2 + lambda2)
    fp2_sqr(&R->x, &t1);        // x3 = lambda1^2
    fp2_sub(&R->x, &R->x, &t5); // x3 = lambda1^2 - lambda3^2(Az1^2z2^2 + lambda2)
    fp2_mul(&t3, &t3, &t4);     // t3 = y1z2^3*lambda3
    fp2_mul(&t3, &t3, &t6);     // t3 = y1z2^3*lambda3^3
    fp2_mul(&t2, &t2, &t6);     // t2 = x1z2^2*lambda3^2
    fp2_sub(&R->y, &t2, &R->x); // y3 = x1z2^2*lambda3^2 - x3
    fp2_mul(&R->y, &R->y, &t1); // y3 = lambda1(x1z2^2*lambda3^2 - x3)
    fp2_sub(&R->y, &R->y, &t3); // y3 = lambda1(x1z2^2*lambda3^2 - x3) - y1z2^3*lambda3^3
}

void
TPL(jac_point_t *Q, jac_point_t const *P, ec_curve_t const *AC)
{ // Naive tripling on a Montgomery curve, representation in Jacobian coordinates (X:Y:Z)
  // corresponding to (X/Z^2,Y/Z^3) This version receives the coefficient value A
    jac_point_t R;

    DBL(&R, P, AC);
    ADD(Q, &R, P, AC);
}

void
recover_y(fp2_t *y, fp2_t const *Px, ec_curve_t const *curve)
{ // Recover y-coordinate of a point on the Montgomery curve y^2 = x^3 + Ax^2 + x
    fp2_t t0;

    fp2_sqr(&t0, Px);
    fp2_mul(y, &t0, &curve->A); // Ax^2
    fp2_add(y, y, Px);          // Ax^2 + x
    fp2_mul(&t0, &t0, Px);
    fp2_add(y, y, &t0); // x^3 + Ax^2 + x
    fp2_sqrt(y);
}

// Double-scalar multiplication R <- k*P + l*Q, fixed for (64*size)-bit scalars
void
DBLMUL_generic(jac_point_t *R,
               const jac_point_t *P,
               const digit_t *k,
               const jac_point_t *Q,
               const digit_t *l,
               const ec_curve_t *curve,
               int size)
{

    digit_t k_t, l_t;
    jac_point_t PQ;

    ADD(&PQ, P, Q, curve);
    jac_init(R);
    for (int j = size; j > 0; j--) {
        for (int i = 0; i < 64; i++) {
            k_t = k[j - 1] >> (63 - i);
            k_t &= 0x01;
            l_t = l[j - 1] >> (63 - i);
            l_t &= 0x01;
            DBL(R, R, curve);
            if (k_t == 1 && l_t == 1) {
                ADD(R, R, &PQ, curve);
            } else if (k_t == 1) {
                ADD(R, R, P, curve);
            } else if (l_t == 1) {
                ADD(R, R, Q, curve);
            }
        }
    }
}

void
DBLMUL(jac_point_t *R,
       const jac_point_t *P,
       const digit_t k,
       const jac_point_t *Q,
       const digit_t l,
       const ec_curve_t *curve)
{ // Double-scalar multiplication R <- k*P + l*Q, fixed for 64-bit scalars
    digit_t k_t, l_t;
    jac_point_t PQ;

    ADD(&PQ, P, Q, curve);
    jac_init(R);

    for (int i = 0; i < 64; i++) {
        k_t = k >> (63 - i);
        k_t &= 0x01;
        l_t = l >> (63 - i);
        l_t &= 0x01;
        DBL(R, R, curve);
        if (k_t == 1 && l_t == 1) {
            ADD(R, R, &PQ, curve);
        } else if (k_t == 1) {
            ADD(R, R, P, curve);
        } else if (l_t == 1) {
            ADD(R, R, Q, curve);
        }
    }
}

#define SCALAR_BITS 128 //// TODO: this could be defined by level

void
DBLMUL2(jac_point_t *R,
        const jac_point_t *P,
        const digit_t *k,
        const jac_point_t *Q,
        const digit_t *l,
        const ec_curve_t *curve)
{ // Double-scalar multiplication R <- k*P + l*Q, fixed for 128-bit scalars
    digit_t k_t[2], l_t[2];
    jac_point_t PQ;

    ADD(&PQ, P, Q, curve);
    jac_init(R);

    for (int i = 0; i < SCALAR_BITS - 64; i++) {
        k_t[1] = k[1] >> (SCALAR_BITS - 64 - 1 - i);
        k_t[1] &= 0x01;
        l_t[1] = l[1] >> (SCALAR_BITS - 64 - 1 - i);
        l_t[1] &= 0x01;
        DBL(R, R, curve);
        if (k_t[1] == 1 && l_t[1] == 1) {
            ADD(R, R, &PQ, curve);
        } else if (k_t[1] == 1) {
            ADD(R, R, P, curve);
        } else if (l_t[1] == 1) {
            ADD(R, R, Q, curve);
        }
    }

    for (int i = 0; i < 64; i++) {
        k_t[0] = k[0] >> (64 - 1 - i);
        k_t[0] &= 0x01;
        l_t[0] = l[0] >> (64 - 1 - i);
        l_t[0] &= 0x01;
        DBL(R, R, curve);
        if (k_t[0] == 1 && l_t[0] == 1) {
            ADD(R, R, &PQ, curve);
        } else if (k_t[0] == 1) {
            ADD(R, R, P, curve);
        } else if (l_t[0] == 1) {
            ADD(R, R, Q, curve);
        }
    }
}

void
lift_point(jac_point_t *P, ec_point_t *Q, ec_curve_t *E)
{
    fp2_t inverses[2];
    if (fp2_is_zero(&Q->z)) {
        fp2_set_one(&P->x);
        fp2_set_one(&P->y);
        fp2_set_zero(&P->z);
    } else {
        fp2_copy(&inverses[0], &Q->z);
        fp2_copy(&inverses[1], &E->C);
        fp2_batched_inv(inverses, 2);

        fp2_set_one(&Q->z);
        fp2_set_one(&E->C);

        fp2_mul(&Q->x, &Q->x, &inverses[0]);
        fp2_mul(&E->A, &E->A, &inverses[1]);

        fp2_copy(&P->x, &Q->x);
        fp2_set_one(&P->z);
        recover_y(&P->y, &P->x, E);
    }
}

void
lift_basis(jac_point_t *P, jac_point_t *Q, ec_basis_t *B, ec_curve_t *E)
{
    fp2_t inverses[2];
    fp2_copy(&inverses[0], &B->P.z);
    // fp2_copy(&inverses[1],&B->Q.z);
    fp2_copy(&inverses[1], &E->C);

    fp2_batched_inv(inverses, 2);
    fp2_set_one(&B->P.z);
    fp2_set_one(&E->C);

    fp2_mul(&B->P.x, &B->P.x, &inverses[0]);
    fp2_mul(&E->A, &E->A, &inverses[1]);

    fp2_copy(&P->x, &B->P.x);
    fp2_copy(&Q->x, &B->Q.x);
    fp2_copy(&Q->z, &B->Q.z);
    fp2_set_one(&P->z);
    recover_y(&P->y, &P->x, E);

    // Algorithm of Okeya-Sakurai to recover y.Q in the montgomery model
    fp2_t v1, v2, v3, v4;
    fp2_mul(&v1, &P->x, &Q->z);
    fp2_add(&v2, &Q->x, &v1);
    fp2_sub(&v3, &Q->x, &v1);
    fp2_sqr(&v3, &v3);
    fp2_mul(&v3, &v3, &B->PmQ.x);
    fp2_add(&v1, &E->A, &E->A);
    fp2_mul(&v1, &v1, &Q->z);
    fp2_add(&v2, &v2, &v1);
    fp2_mul(&v4, &P->x, &Q->x);
    fp2_add(&v4, &v4, &Q->z);
    fp2_mul(&v2, &v2, &v4);
    fp2_mul(&v1, &v1, &Q->z);
    fp2_sub(&v2, &v2, &v1);
    fp2_mul(&v2, &v2, &B->PmQ.z);
    fp2_sub(&Q->y, &v3, &v2);
    fp2_add(&v1, &P->y, &P->y);
    fp2_mul(&v1, &v1, &Q->z);
    fp2_mul(&v1, &v1, &B->PmQ.z);
    fp2_mul(&Q->x, &Q->x, &v1);
    fp2_mul(&Q->z, &Q->z, &v1);

    // Transforming to a jacobian coordinate
    fp2_sqr(&v1, &Q->z);
    fp2_mul(&Q->y, &Q->y, &v1);
    fp2_mul(&Q->x, &Q->x, &Q->z);
}

// WRAPPERS

void
ec_dbl(ec_point_t *res, const ec_curve_t *curve, const ec_point_t *P)
{
    xDBL(res, P, (ec_point_t const *)curve);
}

void
ec_dbl_iter(ec_point_t *res, int n, ec_curve_t *curve, const ec_point_t *P)
{
    if (n > 0) {

        // Chosen according to the relative cost of multiplication and inversion.
        if (n > 300) {
            ec_curve_normalize_A24(curve);
            xDBL_A24_normalized(res, P, &curve->A24);
            for (int i = 0; i < n - 1; i++) {
				xDBL_A24_normalized(res, res, &curve->A24);
            }
        } else {

            // TODO might still be worth it to compute the A24 and use xDBLv2
            ec_dbl(res, curve, P);
            for (int i = 0; i < n - 1; i++) {
                ec_dbl(res, curve, res);
            }
        }
    }
}

void 
ec_tpl(ec_point_t* res, const ec_curve_t* curve, const ec_point_t* P)
{
    ec_point_t A3;

    fp2_add(&A3.z, &curve->C, &curve->C);
    fp2_add(&A3.x, &curve->A, &A3.z);
    fp2_sub(&A3.z, &curve->A, &A3.z);

    xTPL(res, P, &A3);
}

void
ec_tpl_iter(ec_point_t* res, int n, ec_curve_t* curve, const ec_point_t* P)
{
    ec_point_t A3;

    fp2_add(&A3.z, &curve->C, &curve->C);
    fp2_add(&A3.x, &curve->A, &A3.z);
    fp2_sub(&A3.z, &curve->A, &A3.z);

    if (n > 0) {
        xTPL(res, P, &A3);
        for (int i = 0; i < n - 1; i++) {
            xTPL(res, res, &A3);
        }
    }
}

void
ec_mul(ec_point_t *res, const ec_curve_t *curve, const digit_t *scalar, const ec_point_t *P)
{
    xMUL(res, P, scalar, curve);
}

void
ec_biscalar_mul(ec_point_t *res,
                const ec_curve_t *curve,
                const digit_t *scalarP,
                const digit_t *scalarQ,
                const ec_basis_t *PQ)
{
    xDBLMUL(res, &PQ->P, scalarP, &PQ->Q, scalarQ, &PQ->PmQ, curve);
}

void
ec_biscalar_mul_3(ec_point_t* res,
    const ec_curve_t* curve,
    const digit_t* scalarP,
    const digit_t* scalarQ,
    const ec_basis_t* PQ)
{
    xDBLMUL_3(res, &PQ->P, scalarP, &PQ->Q, scalarQ, &PQ->PmQ, curve);
}

void
ec_biscalar_mul_bounded(ec_point_t *res,
                        const ec_curve_t *curve,
                        const digit_t *scalarP,
                        const digit_t *scalarQ,
                        const ec_basis_t *PQ,
                        int f)
{
    xDBLMUL_bounded(res, &PQ->P, scalarP, &PQ->Q, scalarQ, &PQ->PmQ, curve, f);
}

/*
* Description: compute a canonical representative for a Montgomery curve.
*
* @param curve Input curve.
* @param std Output canonical representative of curve.
* @param isom Output isomorphism from curve to std.
*
* Idea: let j be the j-invariant of curve and use
*       y^2 = x^3 - 3j/(j-1728)*x - 2j/(j-1728)
*       as the canonical Weierstrass form. Compute its three 2-torsion
*       x-coordinates x0, x1, x2, choose the largest one, and derive the
*       canonical Montgomery form from it.
* Warning: this algorithm fails when j is 0 or 1728, i.e. A = 0 or A^2 = 9/2.
*/
void
ec_curve_standardized(ec_isom_t *isom, ec_curve_t *std, ec_curve_t *curve)
{
	fp2_t t0, t1, t2;
	fp2_t gamma_plus, gamma_minus, z2, z3, disc, double_C;
	fp2_t x0, x1, x2, u;

	fp2_add(&double_C, &curve->C, &curve->C);   // double_C = 2*C

	fp2_sqr(&t0, &curve->A);
	fp2_sqr(&t1, &curve->C);
	fp2_add(&t2, &t1, &t1);
	fp2_add(&t1, &t1, &t2);
	fp2_add(&t2, &t2, &t2);
	fp2_sub(&disc, &t0, &t2);    
	fp2_sqrt(&disc);            // disc = sqrt(A^2-4*C^2)
	fp2_sub(&t0, &t0, &t1);
	fp2_add(&t2, &t0, &t0);
	fp2_sub(&t1, &t2, &t1);
	fp2_mul(&t1, &t1, &curve->A);
	fp2_inv(&t1);
	fp2_mul(&u, &t0, &t1);     
	fp2_neg(&u, &u);           // u = -(A^2-3*C^2) / (2*A^3-9*A*C^2)

	fp2_neg(&t0, &curve->A);
	fp2_add(&gamma_plus, &t0, &disc);   // gamma_plus = -A + sqrt(A^2-4*C^2)
	fp2_sub(&gamma_minus, &t0, &disc);  // gamma_minus = -A - sqrt(A^2-4*C^2)

	fp2_add(&disc, &disc, &disc);  // disc = 2 * sqrt(A^2-4*C^2)

	fp2_add(&t1, &curve->A, &curve->A);
	fp2_mul(&x0, &u, &t1);            // x0 = u * 2A
	fp2_add(&z2, &gamma_plus, &disc);
	fp2_mul(&x1, &z2, &u);            // x1 = u * (-A + 3*sqrt(A^2-4*C^2))
	fp2_sub(&z3, &gamma_minus, &disc);
	fp2_mul(&x2, &z3, &u);            // x2 = u * (-A - 3*sqrt(A^2-4*C^2))

	// Compare x0, x1, x2 and choose the largest one.
	if (fp2_cmp(&x0, &x1) > 0) {
		if (fp2_cmp(&x0, &x2) > 0) {
			// x0 is the largest.
			fp2_copy(&std->A, &curve->A);
			fp2_copy(&std->C, &curve->C);
			fp2_set_one(&isom->Nx);
			fp2_set_one(&isom->D);
			fp2_set_zero(&isom->Nz);
		}
		else {
			// x2 is the largest.
			fp2_copy(&std->A, &z3);
			fp2_mul(&t0, &disc, &gamma_minus);
			fp2_neg(&t0, &t0);
			fp2_sqrt(&t0);
			fp2_copy(&std->C, &t0);
			fp2_copy(&isom->Nx, &double_C);
			fp2_copy(&isom->Nz, &gamma_minus);
			fp2_copy(&isom->D, &t0);
		}
	}
	else {
		if (fp2_cmp(&x1, &x2) > 0) {
			// x1 is the largest.
			fp2_copy(&std->A, &z2);
			fp2_mul(&t0, &disc, &gamma_plus);
			fp2_sqrt(&t0);
			fp2_copy(&std->C, &t0);
			fp2_copy(&isom->Nx, &double_C);
			fp2_copy(&isom->Nz, &gamma_plus);
			fp2_copy(&isom->D, &t0);
		}
		else {
			// x2 is the largest.
			fp2_copy(&std->A, &z3);
			fp2_mul(&t0, &disc, &gamma_minus);
			fp2_neg(&t0, &t0);
			fp2_sqrt(&t0);
			fp2_copy(&std->C, &t0);
			fp2_copy(&isom->Nx, &double_C);
			fp2_copy(&isom->Nz, &gamma_minus);
			fp2_copy(&isom->D, &t0);
		}
	}

	fp2_copy(&t0, &std->C);
	fp2_inv(&t0);
	fp2_mul(&std->A, &std->A, &t0);
	fp2_set_one(&std->C);
	fp2_neg(&t0, &std->A);
	if (fp2_cmp(&t0, &std->A) > 0) {
		fp2_copy(&std->A, &t0);
		fp2_neg(&isom->D, &isom->D);
	}

	std->is_A24_computed_and_normalized = false;
}

uint32_t
ec_is_isomorphic(ec_curve_t *E1, ec_curve_t *E2)
{
	fp2_t j1, j2;
	ec_j_inv(&j1, E1);
	ec_j_inv(&j2, E2);
	return fp2_is_equal(&j1, &j2);
}
