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

/* Cursed global constant for odd degree isogenies */
extern ec_point_t K[500000]; // Finite subsets of the kernel

void kps_t(uint64_t const i, ec_point_t const P, ec_point_t const A); // tvelu formulae

void xisog_2(ec_kps2_t *kps, ec_point_t *B, const ec_point_t P); // degree-2 isogeny construction
void xisog_2_singular(ec_kps2_t *kps, ec_point_t *B24, ec_point_t A24);

void xisog_4(ec_kps4_t *kps, ec_point_t *B, const ec_point_t P); // degree-4 isogeny construction
void xisog_4_singular(ec_kps4_t *kps, ec_point_t *B24, const ec_point_t P, ec_point_t A24);

void xeval_2(ec_point_t *R, ec_point_t *const Q, const int lenQ, const ec_kps2_t *kps);
void xeval_2_singular(ec_point_t *R, const ec_point_t *Q, const int lenQ, const ec_kps2_t *kps);

void xeval_4(ec_point_t *R, const ec_point_t *Q, const int lenQ, const ec_kps4_t *kps);
void xeval_4_singular(ec_point_t *R,
                      const ec_point_t *Q,
                      const int lenQ,
                      const ec_point_t P,
                      const ec_kps4_t *kps);

// Strategy-based 4-isogeny chain
static void ec_eval_even_strategy(ec_curve_t *image,
                                  ec_point_t *points,
                                  unsigned short points_len,
                                  ec_point_t *A24,
                                  const ec_point_t *kernel,
                                  const int isog_len);

void xisog_t(ec_point_t *B, uint64_t const i, ec_point_t const A); // tvelu formulae
void xeval_t(ec_point_t *Q, uint64_t const i, ec_point_t const P); // tvelu formulae

// hybrid velu formulae
static inline void
kps(uint64_t const i, ec_point_t const P, ec_point_t const A)
{
    kps_t(i, P, A);
}

static inline void
xisog(ec_point_t *B, uint64_t const i, ec_point_t const A)
{
    xisog_t(B, i, A);
}

static inline void
xeval(ec_point_t *Q, uint64_t const i, ec_point_t const P, ec_point_t const A)
{
    xeval_t(Q, i, P);
}

#endif
