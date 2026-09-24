#include "ec.h"
#include "isog.h"
#include <quaternion_data.h>
#include <assert.h>

// This method is used only for short isogeny chains, e.g. length below 10.
static void
ec_eval_even_using_2steps_and_no_strategy(ec_curve_t *image,
	ec_point_t *points,
	unsigned short points_len,
	ec_point_t *A24,
	const ec_point_t *kernel,
	const int isog_len) 
{
	ec_kps2_t kps;
	ec_point_t *SPLITTING_POINTS = (ec_point_t*)malloc(sizeof(ec_point_t)*(1 + isog_len));

	copy_point(&SPLITTING_POINTS[isog_len - 1], kernel);
	for (int i = 0; i < isog_len - 1; i++) {
		xDBL_A24(&SPLITTING_POINTS[isog_len - 2 - i], &SPLITTING_POINTS[isog_len - 1 - i], A24);
	}

	for (int i = 0; i < isog_len; i++) {
		if ((i == 0) && fp2_is_zero(&SPLITTING_POINTS[i].x)) {
			xisog_2_singular(&kps, A24, *A24);
			xeval_2_singular(SPLITTING_POINTS + i + 1, SPLITTING_POINTS + i + 1, isog_len - i, &kps);
			if (points_len)
				xeval_2_singular(points, points, points_len, &kps);
		}
		else {
			xisog_2(&kps, A24, SPLITTING_POINTS[i]);
			xeval_2(SPLITTING_POINTS + i + 1, SPLITTING_POINTS + i + 1, isog_len - i, &kps);
			if (points_len)
				xeval_2(points, points, points_len, &kps);
		}
	}

	// Output curve in the form (A:C)
	A24_to_AC(image, A24);
	image->is_A24_computed_and_normalized = false;
	free(SPLITTING_POINTS);
}

// since we use degree 4 isogeny steps, we need to handle the odd case with care
static void
ec_eval_even_optimal_strategy(ec_curve_t *image,
                      ec_point_t *points,
                      unsigned short points_len,
                      ec_point_t *A24,
                      const ec_point_t *kernel,
                      const int isog_len)
{

    ec_kps4_t kps;

    uint8_t log2_of_e, tmp;
    digit_t e_half = (isog_len) >> 1;
    for (tmp = e_half, log2_of_e = 0; tmp > 0; tmp >>= 1, ++log2_of_e)
        ;
    log2_of_e *= 2; // In order to ensure each splits is at most size log2_of_e

    ec_point_t K2;
    ec_point_t *SPLITTING_POINTS = (ec_point_t*)malloc(sizeof(ec_point_t)*log2_of_e);
    copy_point(&SPLITTING_POINTS[0], kernel);

    int strategy = 0, // Current element of the strategy to be used
        i, j;

    int BLOCK = 0,        // Keeps track of point order
        current = 0;      // Number of points being carried
    int *XDBLs = (int*)malloc(sizeof(int)*log2_of_e); // Number of doubles performed

    // If walk length is odd, we start with a 2-isogeny
    int is_odd = isog_len % 2;

    // if the length is long enough we normalize the first step
    // TODO: should we normalise throughout the chain too? This only
    // helps for the first step
    if (isog_len > 50) {
        ec_normalize_point(A24);
    }

    // Chain of 4-isogenies
    for (j = 0; j < (e_half - 1); j++) {
        // Get the next point of order 4
        while (BLOCK != (e_half - 1 - j)) {
            // A new split will be added
            current += 1;
            // We set the seed of the new split to be computed and saved
            copy_point(&SPLITTING_POINTS[current], &SPLITTING_POINTS[current - 1]);
            // if we copied from the very first element, then we perform one additional doubling
            if (is_odd && current == 1) {
                if (j == 0) {
                    assert(fp2_is_one(&A24->z));
                    xDBL_A24_normalized(
                        &SPLITTING_POINTS[current], &SPLITTING_POINTS[current], A24);
                } else {
                    xDBL_A24(&SPLITTING_POINTS[current], &SPLITTING_POINTS[current], A24);
                }
            }
            for (i = 0; i < 2 * STRATEGY4[MAXIMAL_2ISOGENY_CHAIN_LENGTH - isog_len][strategy]; i++)
                if (j == 0) {
                    assert(fp2_is_one(&A24->z));
                    xDBL_A24_normalized(
                        &SPLITTING_POINTS[current], &SPLITTING_POINTS[current], A24);
                } else {
                    xDBL_A24(&SPLITTING_POINTS[current], &SPLITTING_POINTS[current], A24);
                }
            XDBLs[current] = STRATEGY4[MAXIMAL_2ISOGENY_CHAIN_LENGTH - isog_len]
                                      [strategy]; // The number of doublings performed is saved
            BLOCK += STRATEGY4[MAXIMAL_2ISOGENY_CHAIN_LENGTH - isog_len]
                              [strategy]; // BLOCK is increased by the number of doublings performed
            strategy += 1;                // Next, we move to the next element of the strategy
        }
        if (j == 0) {
            assert(current > 0);
            ec_point_t T;
            assert(fp2_is_one(&A24->z));
            xDBL_A24_normalized(&T, &SPLITTING_POINTS[current], A24);
            if (fp2_is_zero(&T.x)) {
                xisog_4_singular(&kps, A24, SPLITTING_POINTS[current], *A24);
                xeval_4_singular(
                    SPLITTING_POINTS, SPLITTING_POINTS, current, SPLITTING_POINTS[current], &kps);

                // Evaluate points
                if (points_len)
                    xeval_4_singular(points, points, points_len, SPLITTING_POINTS[current], &kps);
            } else {
                xisog_4(&kps, A24, SPLITTING_POINTS[current]);
                xeval_4(SPLITTING_POINTS, SPLITTING_POINTS, current, &kps);

                // Evaluate points
                if (points_len)
                    xeval_4(points, points, points_len, &kps);
            }
        } else {
            if (is_odd && current == 0) {
                xDBL_A24(&SPLITTING_POINTS[current], &SPLITTING_POINTS[current], A24);
            }
#if _DEBUG
            // printf("%d \n",current);
            assert(!fp2_is_zero(&SPLITTING_POINTS[current].z));
            ec_point_t test;
            copy_point(&test, &SPLITTING_POINTS[current]);
            xDBL_A24(&test, &test, A24);
            assert(!fp2_is_zero(&test.z));
            xDBL_A24(&test, &test, A24);
            assert(fp2_is_zero(&test.z));
#endif
            // Evaluate 4-isogeny
            xisog_4(&kps, A24, SPLITTING_POINTS[current]);
            xeval_4(SPLITTING_POINTS, SPLITTING_POINTS, current, &kps);
            if (points_len)
                xeval_4(points, points, points_len, &kps);
        }

        BLOCK -= XDBLs[current];
        XDBLs[current] = 0;
        current -= 1;
    }
    // Final 4-isogeny
    if (is_odd) {
        current = 1;
        copy_point(&SPLITTING_POINTS[1], &SPLITTING_POINTS[0]);
        xDBL_A24(&SPLITTING_POINTS[current], &SPLITTING_POINTS[current], A24);
    }
    xisog_4(&kps, A24, SPLITTING_POINTS[current]);
    if (points_len)
        xeval_4(points, points, points_len, &kps);

    // current-=1;
    // final 2-isogeny
    if (is_odd) {
        xeval_4(SPLITTING_POINTS, SPLITTING_POINTS, 1, &kps);

#if _DEBUG
        assert(!fp2_is_zero(&SPLITTING_POINTS[0].z));
        ec_point_t test;
        copy_point(&test, &SPLITTING_POINTS[0]);
        xDBL_A24(&test, &test, A24);
        assert(fp2_is_zero(&test.z));

#endif

        ec_kps2_t kps;
        xisog_2(&kps, A24, SPLITTING_POINTS[0]);
        if (points_len)
            xeval_2(points, points, points_len, &kps);
    }

    // Output curve in the form (A:C)
    A24_to_AC(image, A24);

    // TODO:
    // The curve does not have A24 normalised though
    // should we normalise it here, or do it later?
    image->is_A24_computed_and_normalized = false;
}

static void
ec_eval_even_balanced_strategy(ec_curve_t *image,
					ec_point_t *points,
					unsigned points_len,
					ec_point_t *A24,
					const ec_point_t *kernel,
					const int isog_len)
{
	ec_kps4_t kps;

	int space = 1;
	for (int i = 1; i < isog_len; i *= 2)
		++space;

	// Stack of remaining kernel points and their associated orders
	ec_point_t *splits = (ec_point_t*)malloc(sizeof(ec_point_t)*space);
	uint16_t *todo = (uint16_t*)malloc(sizeof(uint16_t)*space);
	splits[0] = *kernel;
	todo[0] = isog_len;

	int current = 0; // Pointer to current top of stack

	// Chain of 4-isogenies
	for (int j = 0; j < isog_len / 2; j++) {
		assert(current >= 0);
		assert(todo[current] >= 1);
		// Get the next point of order 4
		while (todo[current] != 2) {
			assert(todo[current] >= 3);
			// A new split will be added
			++current;
			assert(current < space);
			// We set the seed of the new split to be computed and saved
			copy_point(&splits[current], &splits[current - 1]);
			// if we copied from the very first element, then we perform one additional doubling
			unsigned num_dbls = todo[current - 1] / 4 * 2 + todo[current - 1] % 2;
			todo[current] = todo[current - 1] - num_dbls;
			while (num_dbls--)
				xDBL_A24(&splits[current], &splits[current], A24);
		}

		if (j == 0) {
			ec_point_t T;
			xDBL_A24(&T, &splits[current], A24);
			if (fp2_is_zero(&T.x)) {
				xisog_4_singular(&kps, A24, splits[current], *A24);
				xeval_4_singular(
					splits, splits, current, splits[current], &kps);
				// Evaluate points
				if (points_len)
					xeval_4_singular(points, points, points_len, splits[current], &kps);
			}
			else {
				xisog_4(&kps, A24, splits[current]);
				xeval_4(splits, splits, current, &kps);

				// Evaluate points
				if (points_len)
					xeval_4(points, points, points_len, &kps);
			}
		}
		else {
#if _DEBUG
			// printf("%d \n",current);
			assert(!fp2_is_zero(&splits[current].z));
			ec_point_t test;
			copy_point(&test, &splits[current]);
			xDBL_A24(&test, &test, A24);
			assert(!fp2_is_zero(&test.z));
			xDBL_A24(&test, &test, A24);
			assert(fp2_is_zero(&test.z));
#endif
			// Evaluate 4-isogeny
			xisog_4(&kps, A24, splits[current]);
			xeval_4(splits, splits, current, &kps);
			if (points_len)
				xeval_4(points, points, points_len, &kps);
		}
		
		for (int i = 0; i < current; ++i)
			todo[i] -= 2;

		--current;
	}
	assert(isog_len % 2 ? !current : current == -1);

	// Final 2-isogeny
	if (isog_len % 2) {
#if _DEBUG
		assert(!fp2_is_zero(&splits[0].z));
		assert(!fp2_is_zero(&splits[0].x));
		ec_point_t test;
		copy_point(&test, &splits[0]);
		xDBL_A24(&test, &test, A24);
		assert(fp2_is_zero(&test.z));
#endif
		ec_kps2_t kps2;
		xisog_2(&kps2, A24, splits[0]);
		if (points_len)
			xeval_2(points, points, points_len, &kps2);
	}

	// Output curve in the form (A:C)
	A24_to_AC(image, A24);

	image->is_A24_computed_and_normalized = false;
}

void
ec_eval_even_test(ec_curve_t *image, ec_isog_even_t *phi, ec_point_t *points, unsigned short length)
{
	if (phi->length > STRATEGY_BOUND) {
		// Use the optimized strategy for longer chains.
		ec_curve_normalize_A24(&phi->curve);
		ec_eval_even_optimal_strategy(image, points, length, &phi->curve.A24, &phi->kernel, phi->length);
	}
	else {
		// Use the trivial strategy for short chains.
		AC_to_A24(&phi->curve.A24, &phi->curve);
		ec_eval_even_using_2steps_and_no_strategy(image, points, length, &phi->curve.A24, &phi->kernel, phi->length);
	}
    
}

void ec_eval_even(ec_curve_t *image, ec_isog_even_t *phi, ec_point_t *points, unsigned short length)
{
	AC_to_A24(&phi->curve.A24, &phi->curve);
	ec_eval_even_balanced_strategy(image, points, length, &phi->curve.A24, &phi->kernel, phi->length);
}

static void
ec_eval_three_optimal_strategy(ec_curve_t* image,
	ec_point_t* points,
	unsigned short points_len,
	ec_point_t* dual,
    ec_point_t* A3,
    const ec_point_t* kernel,
    const int isog_len)
{
    ec_kps3_t kps;

    uint8_t log2_of_e, tmp;

    for (tmp = isog_len, log2_of_e = 0; tmp > 0; tmp >>= 1, ++log2_of_e)
        ;
    log2_of_e *= 2; // In order to ensure each splits is at most size log2_of_e

    ec_point_t* SPLITTING_POINTS = (ec_point_t*)malloc(sizeof(ec_point_t) * log2_of_e);
    copy_point(&SPLITTING_POINTS[0], kernel);

    int strategy = 0, // Current element of the strategy to be used
        i, j;

    int BLOCK = 0,        // Keeps track of point order
        current = 0;      // Number of points being carried

    int* XTPLs = (int*)malloc(sizeof(int) * log2_of_e); // Number of triples performed

    // Chain of 3-isogenies
    for (j = 0; j < (isog_len - 1); j++) {
        // Get the next point of order 3
        while (BLOCK != (isog_len - 1 - j)) {
            // A new split will be added
            current += 1;
            // We set the seed of the new split to be computed and saved
            copy_point(&SPLITTING_POINTS[current], &SPLITTING_POINTS[current - 1]);
            for (i = 0; i < STRATEGY3[TORSION_PLUS_THREE_POWER - isog_len][strategy]; i++)
                xTPL(&SPLITTING_POINTS[current], &SPLITTING_POINTS[current], A3);
            XTPLs[current] = STRATEGY3[TORSION_PLUS_THREE_POWER - isog_len]
                [strategy]; // The number of triplings performed is saved
            BLOCK += STRATEGY3[TORSION_PLUS_THREE_POWER - isog_len]
                [strategy]; // BLOCK is increased by the number of triplings performed
            strategy += 1;                // Next, we move to the next element of the strategy
        }
        // Evaluate 3-isogeny
        xisog_3(&kps, A3, SPLITTING_POINTS[current]);
        xeval_3(SPLITTING_POINTS, SPLITTING_POINTS, current, &kps);
        if (points_len)
            xeval_3(points, points, points_len, &kps);

        BLOCK -= XTPLs[current];
        XTPLs[current] = 0;
        current -= 1;
    }
    // Final 3-isogeny
	if (dual) {
		xisog_3_with_dual(&kps, A3, dual, SPLITTING_POINTS[current]);
		fp2_copy(&image->A, &A3->x);
		fp2_copy(&image->C, &A3->z);
	}
	else {
		xisog_3(&kps, A3, SPLITTING_POINTS[current]);
		A3_to_AC(image, A3);
	}

    if (points_len)
        xeval_3(points, points, points_len, &kps);

    image->is_A24_computed_and_normalized = false;
}

static void
ec_eval_three_balanced_strategy(ec_curve_t* image,
	ec_point_t* points,
	unsigned short points_len,
	ec_point_t* dual,
	ec_point_t* A3,
	const ec_point_t* kernel,
	const int isog_len)
{
	ec_kps3_t kps;

	int space = 1;
	for (int i = 1; i < isog_len; i *= 2)
		++space;

	// Stack of remaining kernel points and their associated orders
	ec_point_t *splits = (ec_point_t*)malloc(sizeof(ec_point_t)*space);
	uint16_t *todo = (uint16_t*)malloc(sizeof(uint16_t)*space);
	splits[0] = *kernel;
	todo[0] = isog_len;

	int current = 0; // Pointer to current top of stack

	// Chain of 3-isogenies
	for (int j = 0; j < (isog_len - 1); j++) {
		assert(current >= 0);
		assert(todo[current] >= 1);
		// Get the next point of order 3
		while (todo[current] != 1) {
			assert(todo[current] >= 2);
			// A new split will be added
			current += 1;
			// We set the seed of the new split to be computed and saved
			copy_point(&splits[current], &splits[current - 1]);
			unsigned num_tpls = todo[current - 1] / 2;
			todo[current] = todo[current - 1] - num_tpls;
			for (int i = 0; i < num_tpls; i++)
				xTPL(&splits[current], &splits[current], A3);
		}
#if _DEBUG
		{
			assert(!fp2_is_zero(&splits[current].z));
			ec_point_t test;
			xTPL(&test, &splits[current], A3);
			assert(fp2_is_zero(&test.z));
		}
#endif
		// Evaluate 3-isogeny
		xisog_3(&kps, A3, splits[current]);
		xeval_3(splits, splits, current, &kps);
		if (points_len)
			xeval_3(points, points, points_len, &kps);

		for (int i = 0; i < current; ++i)
			todo[i] -= 1;

		--current;
	}
	// Final 3-isogeny
	if (dual) {
		xisog_3_with_dual(&kps, A3, dual, splits[current]);
		fp2_copy(&image->A, &A3->x);
		fp2_copy(&image->C, &A3->z);
	}
	else {
		xisog_3(&kps, A3, splits[current]);
		A3_to_AC(image, A3);
	}

	if (points_len)
		xeval_3(points, points, points_len, &kps);

	image->is_A24_computed_and_normalized = false;
}

void
ec_eval_three_test(ec_curve_t* image,
    const ec_isog_three_t* phi,
    ec_point_t* points,
    unsigned short length,
	ec_point_t* dual)
{
    ec_point_t A3;

    AC_to_A3(&A3, &phi->curve);
    ec_eval_three_optimal_strategy(image, points, length, dual, &A3, &phi->kernel, phi->length);
}

void
ec_eval_three(ec_curve_t* image,
	const ec_isog_three_t* phi,
	ec_point_t* points,
	unsigned short length,
	ec_point_t* dual)
{
	ec_point_t A3;

	AC_to_A3(&A3, &phi->curve);
	ec_eval_three_balanced_strategy(image, points, length, dual, &A3, &phi->kernel, phi->length);
}

// Generate random points SPLITTING_POINTS on the curve, where SPLITTING_POINTS[e-1] has order 3^e.
// SPLITTING_POINTS[i] = [3]SPLITTING_POINTS[i+1]
// The curve Montgomery coefficients are (A:C), and A3 = (A+2C:A-2C).
static void
random_splliting_torsion_points_3(ec_point_t* SPLITTING_POINTS,
	const unsigned int e,
	const ec_curve_t* curve)
{
	ec_point_t A24, A3;
	fp2_t x, t0, t1, t2;
	ec_point_t P;
	int EXTRA = 10;
	ibz_t re, im;

	if (e == 0)
		return;

	ibz_init(&re);
	ibz_init(&im);

	// Curve coefficient in the form A24 = (A+2C:4C)
	fp2_add(&A24.z, &curve->C, &curve->C);
	fp2_add(&A24.x, &curve->A, &A24.z);
	fp2_add(&A24.z, &A24.z, &A24.z);

	// Curve coefficient in the form A3 = (A+2C:A-2C)
	fp2_sub(&A3.z, &A24.x, &A24.z);
	fp2_copy(&A3.x, &A24.x);

	while (true) {
		// Generate a random x-coordinate.
		ibz_rand_interval(&re, &ibz_const_one, &QUATALG_PINFTY.p);
		ibz_rand_interval(&im, &ibz_const_one, &QUATALG_PINFTY.p);
		ibz_to_digit_array(x.re, &re);
		ibz_to_digit_array(x.im, &im);

		// Check if point is rational
		fp2_sqr(&t0, &curve->C);
		fp2_mul(&t1, &t0, &x);
		fp2_mul(&t2, &curve->A, &curve->C);
		fp2_add(&t1, &t1, &t2);
		fp2_mul(&t1, &t1, &x);
		fp2_add(&t1, &t1, &t0);
		fp2_mul(&t1, &t1, &x);
		if (fp2_is_square(&t1)) {
			fp2_copy(&P.x, &x);
			fp2_set_one(&P.z);
		}
		else
			continue;

		// Clear non-3 factors from the order
		xMULv2(&P, &P, p_cofactor_for_3g, (int)P_COFACTOR_FOR_3G_BITLENGTH, &A24);

		for (int i = 0; i < POWER_OF_3 - e - EXTRA; i++) {
			xTPL(&P, &P, &A3);
		}
		copy_point(&SPLITTING_POINTS[0], &P);
		for (int i = 0; i < e - 1; i++) {
			xTPL(&SPLITTING_POINTS[i + 1], &SPLITTING_POINTS[i], &A3);
		}
		copy_point(&P, &SPLITTING_POINTS[e - 1]);
		if (ec_is_zero(&P))
			continue;
		xTPL(&P, &P, &A3);
		while (!ec_is_zero(&P)) {			
			for (int i = 0; i < e - 1; i++) {
				copy_point(&SPLITTING_POINTS[i], &SPLITTING_POINTS[i + 1]);
			}
			copy_point(&SPLITTING_POINTS[e - 1], &P);
			xTPL(&P, &P, &A3);
		}
		break;
	}

	ibz_finalize(&re);
	ibz_finalize(&im);
}

// Generate a 3-isogeny path of length e from domain.
// This naive method is suitable for short chains.
void 
random_isog_chain_3(ec_curve_t* codomain, 
	const ec_curve_t* domain, 
	const unsigned int e,
	ec_point_t* points,
	const unsigned int points_len)
{
	if (e == 0)
		return;

	ec_point_t* SPLITTING_POINTS = (ec_point_t*)malloc(sizeof(ec_point_t) * e);
	ec_point_t A24, A3;
	fp2_t C2;

	random_splliting_torsion_points_3(SPLITTING_POINTS, e, domain);

#if _DEBUG
	for (int i = 0; i < e; i++) {
		assert(test_point_order_threef(&SPLITTING_POINTS[i], domain, e - i));
	}
#endif
	int current = 0;
	ec_kps3_t kps;
	AC_to_A3(&A3, domain);

	for (int i = e - 1; i >= 0; i--) {
		xisog_3(&kps, &A3, SPLITTING_POINTS[i]);
		xeval_3(SPLITTING_POINTS, SPLITTING_POINTS, i, &kps);
		if (points_len)
			xeval_3(points, points, points_len, &kps);
	}
	
	// Output curve in the form (A:C)
	A3_to_AC(codomain, &A3);
	codomain->is_A24_computed_and_normalized = false;
	
	free(SPLITTING_POINTS);
}

void
ec_isomorphism(ec_isom_t *isom, const ec_curve_t *from, const ec_curve_t *to)
{
    fp2_t t0, t1, t2, t3, t4;
    fp2_mul(&t0, &from->A, &to->C);
    fp2_sqr(&t0, &t0); // fromA^2toC^2
    fp2_mul(&t1, &to->A, &from->C);
    fp2_sqr(&t1, &t1); // toA^2fromC^2
    fp2_mul(&t2, &to->C, &from->C);
    fp2_sqr(&t2, &t2); // toC^2fromC^2
    fp2_add(&t3, &t2, &t2);
    fp2_add(&t2, &t3, &t2); // 3toC^2fromC^2
    fp2_sub(&t3, &t2, &t0); // 3toC^2fromC^2-fromA^2toC^2
    fp2_sub(&t4, &t2, &t1); // 3toC^2fromC^2-toA^2fromC^2
    fp2_inv(&t3);
    fp2_mul(&t4, &t4, &t3);
    fp2_sqrt(&t4); // lambda^2 constant for SW isomorphism
    fp2_sqr(&t3, &t4);
    fp2_mul(&t3, &t3, &t4); // lambda^6

    // Check sign of lambda^2, such that lambda^6 has the right sign
    fp2_sqr(&t0, &from->C);
    fp2_add(&t1, &t0, &t0);
    fp2_add(&t1, &t1, &t1);
    fp2_add(&t1, &t1, &t1);
    fp2_add(&t0, &t0, &t1); // 9fromC^2
    fp2_sqr(&t2, &from->A);
    fp2_add(&t2, &t2, &t2); // 2fromA^2
    fp2_sub(&t2, &t2, &t0);
    fp2_mul(&t2, &t2, &from->A); // -9fromC^2fromA+2fromA^3
    fp2_sqr(&t0, &to->C);
    fp2_mul(&t0, &t0, &to->C);
    fp2_mul(&t2, &t2, &t0); // toC^3* [-9fromC^2fromA+2fromA^3]
    fp2_mul(&t3, &t3, &t2); // lambda^6*(-9fromA+2fromA^3)*toC^3
    fp2_sqr(&t0, &to->C);
    fp2_add(&t1, &t0, &t0);
    fp2_add(&t1, &t1, &t1);
    fp2_add(&t1, &t1, &t1);
    fp2_add(&t0, &t0, &t1); // 9toC^2
    fp2_sqr(&t2, &to->A);
    fp2_add(&t2, &t2, &t2); // 2toA^2
    fp2_sub(&t2, &t2, &t0);
    fp2_mul(&t2, &t2, &to->A); // -9toC^2toA+2toA^3
    fp2_sqr(&t0, &from->C);
    fp2_mul(&t0, &t0, &from->C);
    fp2_mul(&t2, &t2, &t0); // fromC^3* [-9toC^2toA+2toA^3]
    if (!fp2_is_equal(&t2, &t3))
        fp2_neg(&t4, &t4);

    // Mont -> SW -> SW -> Mont
    fp2_set_one(&t0);
    fp2_add(&isom->D, &t0, &t0);
    fp2_add(&isom->D, &isom->D, &t0);
    fp2_mul(&isom->D, &isom->D, &from->C);
    fp2_mul(&isom->D, &isom->D, &to->C);
    fp2_mul(&isom->Nx, &isom->D, &t4);
    fp2_mul(&t4, &t4, &from->A);
    fp2_mul(&t4, &t4, &to->C);
    fp2_mul(&t0, &to->A, &from->C);
    fp2_sub(&isom->Nz, &t0, &t4);
}

void
ec_iso_eval(ec_point_t *P, ec_isom_t *isom)
{
    fp2_t tmp;
    fp2_mul(&P->x, &P->x, &isom->Nx);
    fp2_mul(&tmp, &P->z, &isom->Nz);
    fp2_sub(&P->x, &P->x, &tmp);
    fp2_mul(&P->z, &P->z, &isom->D);
}
