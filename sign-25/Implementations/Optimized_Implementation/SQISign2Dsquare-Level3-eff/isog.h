#ifndef _ISOG_H_
#define _ISOG_H_

#include <ec.h>
#include "curve_extras.h"

/* KPS structure for isogenies of degree 2 or 4 */
typedef struct
{
    ec_point_t K;
} ec_kps2_t;
typedef struct
{
    ec_point_t K[3];
} ec_kps4_t;
typedef struct
{
    ec_point_t K;
} ec_kps3_t;

/* Cursed global constant for odd degree isogenies */
extern ec_point_t K[83]; // Finite subsets of the kernel


void xisog_2(ec_kps2_t *kps, ec_point_t *B, const ec_point_t P); // degree-2 isogeny construction
void xisog_2_singular(ec_kps2_t *kps, ec_point_t *B24, ec_point_t A24);

void xisog_4(ec_kps4_t *kps, ec_point_t *B, const ec_point_t P); // degree-4 isogeny construction
void xisog_4_singular(ec_kps4_t *kps, ec_point_t *B24, const ec_point_t P, ec_point_t A24);

void xisog_3(ec_kps3_t* kps, ec_point_t* B, const ec_point_t P); // degree-3 isogeny construction
void xisog_3_with_dual(ec_kps3_t* kps, ec_point_t* B, ec_point_t* dual, const ec_point_t P);

void xeval_2(ec_point_t *R, ec_point_t *const Q, const int lenQ, const ec_kps2_t *kps);
void xeval_2_singular(ec_point_t *R, const ec_point_t *Q, const int lenQ, const ec_kps2_t *kps);

void xeval_4(ec_point_t *R, const ec_point_t *Q, const int lenQ, const ec_kps4_t *kps);
void xeval_4_singular(ec_point_t *R,
                      const ec_point_t *Q,
                      const int lenQ,
                      const ec_point_t P,
                      const ec_kps4_t *kps);

void xeval_3(ec_point_t* R, ec_point_t* const Q, const int lenQ, const ec_kps3_t* kps);

static void
ec_eval_even_using_2steps_and_no_strategy(ec_curve_t *image,
										  ec_point_t *points,
										  unsigned short points_len,
										  ec_point_t *A24,
										  const ec_point_t *kernel,
										  const int isog_len);
// Strategy-based 4-isogeny chain
static void ec_eval_even_optimal_strategy(ec_curve_t *image,
                                  ec_point_t *points,
                                  unsigned short points_len,
                                  ec_point_t *A24,
                                  const ec_point_t *kernel,
                                  const int isog_len);
// Strategy-based 3-isogeny chain
static void ec_eval_three_optimal_strategy(ec_curve_t* image,
	ec_point_t* points,
	unsigned short points_len,
	ec_point_t* dual,
    ec_point_t* A3,
    const ec_point_t* kernel,
    const int isog_len);

#endif
