#include "curve_extras.h"
#include <ec_params.h>
#include <assert.h>

/*
 * We implement the biextension arithmetic by using the cubical torsor representation. For now only
 * implement the 2^e-ladder.
 *
 * Warning: both cubicalDBL and cubicalADD are off by a factor x4 with respect
 * to the cubical arithmetic.
 * Since this factor is the same, this means that the biextension
 * arithmetic is correct, so the pairings are ok (they only rely on the
 * biextension arithmetic).
 * In the special case where Q=P (self pairings), we use the cubical ladder
 * rather than the biextension ladder because this is faster. In that case,
 * when we do a ladder we are off by a factor 4^m, m the number of bits.
 * This factor thus disappear in the Weil pairing since we take a quotient,
 * and also in the Tate pairing due to the final exponentiation; so
 * everything is ok too.
 * (Note that when the curves are supersingular as in our case, the Tate
 * self pairing is always trivial anyway because the Galois structure of the
 * isogeneous curves are all the same, so the étale torsor representing the
 * Tate pairing has to be trivial).
 */

/* return the *normalised* point (A+2C)/4C */
void
A24_from_AC(ec_point_t *A24, ec_point_t const *AC)
{
    fp2_add(&A24->z, &AC->z, &AC->z);
    fp2_add(&A24->x, &AC->x, &A24->z);
    fp2_add(&A24->z, &A24->z, &A24->z); //(A+2C: 4C)
    ec_normalize_point(A24);
}

// this is exactly like xDBLv2, except we use the fact that P is normalised
// to gain a multiplication
// Warning: for now we need to assume that A24 is normalised, ie C24=1.
// (maybe add an assert?)
void
cubicalDBL(ec_point_t *Q, ec_point_t const *P, ec_point_t const *A24)
{
    // A24 = (A+2C:4C)
    fp2_t t0, t1, t2;

    assert(fp2_is_one(&A24->z));
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

// this would be exactly like xADD if PQ was 'antinormalised' as (1,z)
void
cubicalADD(ec_point_t *R, ec_point_t const *P, ec_point_t const *Q, fp2_t const *ixPQ)
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
    fp2_sqr(&R->z, &t3);
    fp2_sqr(&t2, &t2);
    fp2_mul(&R->x, ixPQ, &t2);
}

// given cubical reps of P+Q, Q, P, return P+2Q, 2Q
void
biextDBL(ec_point_t *PQQ,
         ec_point_t *QQ,
         ec_point_t const *PQ,
         ec_point_t const *Q,
         fp2_t const *ixP,
         ec_point_t const *A24)
{
    cubicalADD(PQQ, PQ, Q, ixP);
    cubicalDBL(QQ, Q, A24);
}

// iterative biextension doubling
void
biext_ladder_2e(uint64_t e,
                ec_point_t *PnQ,
                ec_point_t *nQ,
                ec_point_t const *PQ,
                ec_point_t const *Q,
                fp2_t const *ixP,
                ec_point_t const *A24)
{
    copy_point(PnQ, PQ);
    copy_point(nQ, Q);
    for (uint64_t i = 0; i < e; i++) {
        biextDBL(PnQ, nQ, PnQ, nQ, ixP, A24);
    }
}

// compute the monodromy ratio of cubical points [(P+nQ)/P] / [(nQ)/0]
// Compute the ratio X/Z above as a (X:Z) point to avoid a division
void
point_ratio(ec_point_t *R, ec_point_t const *PnQ, ec_point_t const *nQ, ec_point_t const *P)
{
    // Sanity tests
    assert(ec_is_zero(nQ));
    assert(is_point_equal(PnQ, P));

    fp2_mul(&R->x, &nQ->x, &P->x);
    fp2_copy(&R->z, &PnQ->x);
}



// compute the cubical translation of P by a point of 2-torsion T
void
translate(ec_point_t *P, ec_point_t const *T)
{
    fp2_t t0, t1, t2;
    if (fp2_is_zero(&T->z)) {
        // do nothing
    } else if (fp2_is_zero(&T->x)) {
        fp2_copy(&t0, &P->x);
        fp2_copy(&P->x, &P->z);
        fp2_copy(&P->z, &t0);
    } else {
        fp2_mul(&t0, &T->x, &P->x);
        fp2_mul(&t1, &T->z, &P->z);
        fp2_sub(&t2, &t0, &t1);
        fp2_mul(&t0, &T->z, &P->x);
        fp2_mul(&t1, &T->x, &P->z);
        fp2_sub(&P->z, &t0, &t1);
        fp2_copy(&P->x, &t2);
    }
}

// Compute the monodromy P+2^e Q (in level 1)
// The suffix _i means that we are given 1/x(P) as parameter.
// Warning: to get meaningful result when using the monodromy to compute
// pairings, we need P, Q, PQ, A24 to be normalised
// (this is not strictly necessary, but care need to be taken when they are not normalised. Only
// handle the normalised case for now)
void
monodromy_i(ec_point_t *r,
            uint64_t e,
            ec_point_t const *PQ,
            ec_point_t const *Q,
            ec_point_t const *P,
            fp2_t const *ixP,
            ec_point_t const *A24)
{
    ec_point_t PnQ, nQ;
    biext_ladder_2e(e - 1, &PnQ, &nQ, PQ, Q, ixP, A24);
    translate(&PnQ, &nQ);
    translate(&nQ, &nQ);
    point_ratio(r, &PnQ, &nQ, P);
}



// TODO: use only one inversion
// And normalize A24 at the same time (if needed), to save another inversion
void
to_cubical(ec_point_t *Q, ec_point_t *P)
{
    // ec_normalize_point(A24);
    ec_normalize_point(P);
    ec_normalize_point(Q);
    // ec_normalize_point(PQ);
}

// Normalize the points and also store 1/x(P), 1/x(Q)
void
to_cubical_i(ec_point_t *P, ec_point_t *Q, fp2_t *ixP, fp2_t *ixQ)
{
    fp2_t t[4];
    fp2_copy(&t[0], &P->x);
    fp2_copy(&t[1], &P->z);
    fp2_copy(&t[2], &Q->x);
    fp2_copy(&t[3], &Q->z);
    fp2_batched_inv(t, 4);
    fp2_mul(ixP, &P->z, &t[0]);
    fp2_mul(ixQ, &Q->z, &t[2]);
    fp2_mul(&P->x, &P->x, &t[1]);
    fp2_mul(&Q->x, &Q->x, &t[3]);
    fp2_set_one(&P->z);
    fp2_set_one(&Q->z);
}




// Weil pairing, PQ should be P+Q in (X:Z) coordinates
// We assume the points are normalised correctly
// Do we need a weil_c version?
void
weil_n(fp2_t *r,
       uint64_t e,
       ec_point_t const *P,
       ec_point_t const *Q,
       ec_point_t const *PQ,
       fp2_t const *ixP,
       fp2_t const *ixQ,
       ec_point_t const *A24)
{
    ec_point_t R0, R1;
    monodromy_i(&R0, e, PQ, Q, P, ixP, A24);
    monodromy_i(&R1, e, PQ, P, Q, ixQ, A24);
    // TODO: check if that's the Weil pairing or its inverse
    fp2_mul(r, &R0.x, &R1.z);
    fp2_inv(r);
    fp2_mul(r, r, &R0.z);
    fp2_mul(r, r, &R1.x);
}

// Weil pairing, PQ should be P+Q in (X:Z) coordinates
// Normalise the points and call the code above
// The code will crash (division by 0) if either P or Q is (0:1)
void
weil(fp2_t *r, uint64_t e, ec_point_t *P, ec_point_t *Q, ec_point_t *PQ, ec_point_t *A24)
{
    fp2_t ixP, ixQ;
    to_cubical_i(P, Q, &ixP, &ixQ);
    weil_n(r, e, P, Q, PQ, &ixP, &ixQ, A24);
}

// recursive dlog function
bool
fp2_dlog_2e_rec(digit_t *a, long len, fp2_t *pows_f, fp2_t *pows_g, long stacklen)
{
    if (len == 0) {
        // *a = 0;
        for (int i = 0; i < NWORDS_ORDER_2; i++) {
            a[i] = 0;
        }
        return true;
    } else if (len == 1) {
        if (fp2_is_one(&pows_f[stacklen - 1])) {
            // a = 0;
            for (int i = 0; i < NWORDS_ORDER_2; i++) {
                a[i] = 0;
            }
            for (int i = 0; i < stacklen - 1; ++i) {
                fp2_sqr(&pows_g[i], &pows_g[i]); // new_g = g^2
            }
            return true;
        } else if (fp2_is_equal(&pows_f[stacklen - 1], &pows_g[stacklen - 1])) {
            // a = 1;
            a[0] = 1;
            for (int i = 1; i < NWORDS_ORDER_2; i++) {
                a[i] = 0;
            }
            fp2_t tmp;
            for (int i = 0; i < stacklen - 1; ++i) {
                fp2_mul(&pows_f[i], &pows_f[i], &pows_g[i]); // new_f = f*g
                fp2_sqr(&pows_g[i], &pows_g[i]);             // new_g = g^2
            }
            return true;
        } else {
            return false;
        }
    } else {
        long right = (double)len * 0.5;
        long left = len - right;
        pows_f[stacklen] = pows_f[stacklen - 1];
        pows_g[stacklen] = pows_g[stacklen - 1];
        for (int i = 0; i < left; i++) {
            fp2_sqr(&pows_f[stacklen], &pows_f[stacklen]);
            fp2_sqr(&pows_g[stacklen], &pows_g[stacklen]);
        }
        // uint64_t dlp1 = 0, dlp2 = 0;
        digit_t dlp1[NWORDS_ORDER_2], dlp2[NWORDS_ORDER_2];
        bool ok;
        ok = fp2_dlog_2e_rec(dlp1, right, pows_f, pows_g, stacklen + 1);
        if (!ok)
            return false;
        ok = fp2_dlog_2e_rec(dlp2, left, pows_f, pows_g, stacklen);
        if (!ok)
            return false;
        // a = dlp1 + 2^right * dlp2
        multiple_mp_shiftl(dlp2, right, NWORDS_ORDER_2);
        mp_add(a, dlp2, dlp1, NWORDS_ORDER_2);

        return true;
    }
}

// compute DLP
bool
fp2_dlog_2e(digit_t *scal, const fp2_t *f, const fp2_t *g, int e)
{
    long log, len = e;
    for (log = 0; len > 1; len >>= 1)
        log++;
    log += 1;

    fp2_t *pows_f=(fp2_t *)malloc(sizeof(fp2_t)*log);
    fp2_t *pows_g=(fp2_t *)malloc(sizeof(fp2_t)*log);
    pows_f[0] = *f;
    pows_g[0] = *g;
    fp2_inv(&pows_g[0]);

    for (int i = 0; i < NWORDS_ORDER_2; i++) {
        scal[i] = 0;
    }

    bool ok = fp2_dlog_2e_rec(scal, e, pows_f, pows_g, 1);
    assert(ok);
    (void)ok;

    return ok;
}


// Normalize a "basis" (P, Q), (P1, P2) and store their inverse
void
to_cubical_basis_i(ec_point_t *P,
                   ec_point_t *Q,
                   ec_point_t *P1,
                   ec_point_t *P2,
                   fp2_t *ixP,
                   fp2_t *ixQ,
                   fp2_t *ixP1,
                   fp2_t *ixP2)
{
    fp2_t t[8];
    fp2_copy(&t[0], &P->x);
    fp2_copy(&t[1], &P->z);
    fp2_copy(&t[2], &Q->x);
    fp2_copy(&t[3], &Q->z);
    fp2_copy(&t[4], &P1->x);
    fp2_copy(&t[5], &P1->z);
    fp2_copy(&t[6], &P2->x);
    fp2_copy(&t[7], &P2->z);
    fp2_batched_inv(t, 8);
    fp2_mul(ixP, &P->z, &t[0]);
    fp2_mul(&P->x, &P->x, &t[1]);
    fp2_set_one(&P->z);
    fp2_mul(ixQ, &Q->z, &t[2]);
    fp2_mul(&Q->x, &Q->x, &t[3]);
    fp2_set_one(&Q->z);
    fp2_mul(ixP1, &P1->z, &t[4]);
    fp2_mul(&P1->x, &P1->x, &t[5]);
    fp2_set_one(&P1->z);
    fp2_mul(ixP2, &P2->z, &t[6]);
    fp2_mul(&P2->x, &P2->x, &t[7]);
    fp2_set_one(&P2->z);
}

// Inline all the Weil pairing computations done in ec_dlog_2_weil
void
weil_dlog(digit_t *scalarP1,
          digit_t *scalarQ1,
          digit_t *scalarP2,
          digit_t *scalarQ2,
          uint64_t e,
          ec_point_t *P,
          ec_point_t *Q,
          ec_point_t *P1,
          ec_point_t *P2,
          ec_point_t *PQ,
          ec_point_t *PP1,
          ec_point_t *PP2,
          ec_point_t *P1Q,
          ec_point_t *P2Q,
          ec_point_t *A24)
{

    fp2_t ixP, ixQ, ixP1, ixP2, w0, w;
    ec_point_t nP, nQ, nP1, nP2, nPQ, PnQ, nPP1, PnP1, nPP2, PnP2, nP1Q, P1nQ, nP2Q, P2nQ, R0, R1;

    to_cubical_basis_i(P, Q, P1, P2, &ixP, &ixQ, &ixP1, &ixP2);

    copy_point(&nP, P);
    copy_point(&nQ, Q);
    copy_point(&nP1, P1);
    copy_point(&nP2, P2);
    copy_point(&nPQ, PQ);
    copy_point(&PnQ, PQ);
    copy_point(&nPP1, PP1);
    copy_point(&nPP2, PP2);
    copy_point(&PnP1, PP1);
    copy_point(&PnP2, PP2);
    copy_point(&nP1Q, P1Q);
    copy_point(&nP2Q, P2Q);
    copy_point(&P1nQ, P1Q);
    copy_point(&P2nQ, P2Q);

    for (uint64_t i = 0; i < e - 1; i++) {
        cubicalADD(&nPQ, &nPQ, &nP, &ixQ);
        cubicalADD(&nPP1, &nPP1, &nP, &ixP1);
        cubicalADD(&nPP2, &nPP2, &nP, &ixP2);

        cubicalADD(&PnQ, &PnQ, &nQ, &ixP);
        cubicalADD(&P1nQ, &P1nQ, &nQ, &ixP1);
        cubicalADD(&P2nQ, &P2nQ, &nQ, &ixP2);

        cubicalADD(&PnP1, &PnP1, &nP1, &ixP);
        cubicalADD(&nP1Q, &nP1Q, &nP1, &ixQ);

        cubicalADD(&PnP2, &PnP2, &nP2, &ixP);
        cubicalADD(&nP2Q, &nP2Q, &nP2, &ixQ);

        cubicalDBL(&nP, &nP, A24);
        cubicalDBL(&nQ, &nQ, A24);
        cubicalDBL(&nP1, &nP1, A24);
        cubicalDBL(&nP2, &nP2, A24);
    }

    // weil(&w0,e,&PQ->P,&PQ->Q,&PQ->PmQ,&A24);
    translate(&nPQ, &nP);
    translate(&nPP1, &nP);
    translate(&nPP2, &nP);
    translate(&PnQ, &nQ);
    translate(&P1nQ, &nQ);
    translate(&P2nQ, &nQ);
    translate(&PnP1, &nP1);
    translate(&nP1Q, &nP1);
    translate(&PnP2, &nP2);
    translate(&nP2Q, &nP2);

    translate(&nP, &nP);
    translate(&nQ, &nQ);
    translate(&nP1, &nP1);
    translate(&nP2, &nP2);

    // TODO: we could batch these 5 inversions

    // computation of the reference weil pairing
    // weil(&w0,e,&PQ->P,&PQ->Q,&PQ->PmQ,&A24);
    point_ratio(&R0, &nPQ, &nP, Q);
    point_ratio(&R1, &PnQ, &nQ, P);
    fp2_mul(&w0, &R0.x, &R1.z);
    fp2_inv(&w0);
    fp2_mul(&w0, &w0, &R0.z);
    fp2_mul(&w0, &w0, &R1.x);

    // e(P,P1) = w0^scalarQ1
    // weil(&w,e,&PQ->P,&basis->P,&PmP1,&A24);
    point_ratio(&R0, &nPP1, &nP, P1);
    point_ratio(&R1, &PnP1, &nP1, P);
    fp2_mul(&w, &R0.x, &R1.z);
    fp2_inv(&w);
    fp2_mul(&w, &w, &R0.z);
    fp2_mul(&w, &w, &R1.x);
    fp2_dlog_2e(scalarQ1, &w, &w0, e);

    // e(P1,Q) = w0^scalarP1
    // weil(&w,e,&basis->P,&PQ->Q,&P1mQ,&A24);
    point_ratio(&R0, &nP1Q, &nP1, Q);
    point_ratio(&R1, &P1nQ, &nQ, P1);
    fp2_mul(&w, &R0.x, &R1.z);
    fp2_inv(&w);
    fp2_mul(&w, &w, &R0.z);
    fp2_mul(&w, &w, &R1.x);
    fp2_dlog_2e(scalarP1, &w, &w0, e);

    // e(P,P2) = w0^scalarQ2
    // weil(&w,e,&PQ->P,&basis->Q,&PmP2,&A24);
    point_ratio(&R0, &nPP2, &nP, P2);
    point_ratio(&R1, &PnP2, &nP2, P);
    fp2_mul(&w, &R0.x, &R1.z);
    fp2_inv(&w);
    fp2_mul(&w, &w, &R0.z);
    fp2_mul(&w, &w, &R1.x);
    fp2_dlog_2e(scalarQ2, &w, &w0, e);

    // e(P2,Q) = w0^scalarP2
    // weil(&w,e,&basis->Q,&PQ->Q,&P2mQ,&A24);
    point_ratio(&R0, &nP2Q, &nP2, Q);
    point_ratio(&R1, &P2nQ, &nQ, P2);
    fp2_mul(&w, &R0.x, &R1.z);
    fp2_inv(&w);
    fp2_mul(&w, &w, &R0.z);
    fp2_mul(&w, &w, &R1.x);
    fp2_dlog_2e(scalarP2, &w, &w0, e);
}

// Like ec_dlog_2_weil_old but all weil pairing computation to factor
// the same arithmetic operations
void
ec_dlog_2_weil(digit_t *scalarP1,
               digit_t *scalarQ1,
               digit_t *scalarP2,
               digit_t *scalarQ2,
               ec_basis_t *PQ,
               ec_basis_t *basis,
               ec_curve_t *curve,
               int e)
{
#if _DEBUG
    assert(test_point_order_twof(&PQ->Q, curve, e));
#endif

    fp2_t w0, w;
    ec_point_t AC, A24;
    ec_point_t PmP1, P1mQ, PmP2, P2mQ;
    jac_point_t xyP, xyQ, xyP1, xyP2, temp;

    // we start by computing the different weil pairings

    // precomputing the correct curve data
    fp2_copy(&AC.x, &curve->A);
    fp2_copy(&AC.z, &curve->C);
    A24_from_AC(&A24, &AC);

    // lifting the two basis points
    lift_basis(&xyP, &xyQ, PQ, curve);
    lift_basis(&xyP1, &xyP2, basis, curve);

    // computation of the differences
    jac_neg(&temp, &xyP1);
    ADD(&temp, &temp, &xyP, curve);
    jac_to_xz(&PmP1, &temp);
    jac_neg(&temp, &xyP2);
    ADD(&temp, &temp, &xyP, curve);
    jac_to_xz(&PmP2, &temp);
    jac_neg(&temp, &xyQ);
    ADD(&temp, &temp, &xyP1, curve);
    jac_to_xz(&P1mQ, &temp);
    jac_neg(&temp, &xyQ);
    ADD(&temp, &temp, &xyP2, curve);
    jac_to_xz(&P2mQ, &temp);

    weil_dlog(scalarP1,
              scalarQ1,
              scalarP2,
              scalarQ2,
              e,
              &PQ->P,
              &PQ->Q,
              &basis->P,
              &basis->Q,
              &PQ->PmQ,
              &PmP1,
              &PmP2,
              &P1mQ,
              &P2mQ,
              &A24);

#if _DEBUG
    ec_point_t test_comput;
    ec_biscalar_mul(&test_comput, curve, scalarP1, scalarQ1, PQ);
    assert(ec_is_equal(&test_comput, &basis->P));
    ec_biscalar_mul(&test_comput, curve, scalarP2, scalarQ2, PQ);
    assert(ec_is_equal(&test_comput, &basis->Q));
#endif
}


void
biext_ladder_3(ec_point_t* PnQ,
    ec_point_t* nQ,
    ec_point_t* PQ,
    ec_point_t* Q,
    ec_point_t* P,
    fp2_t const* ixPQ,
    fp2_t const* ixQ,
    fp2_t const* ixP,
    ec_point_t const* A24)
{
    int location, i, j, bit;
    ec_point_t nm1Q;
    copy_point(nQ, Q);
    copy_point(PnQ, PQ);

    for (i = NWORDS_ORDER_3 - 1; i >= 0; i--) {
        if (i == (NWORDS_ORDER_3 - 1)) {
            location = LOCATION;
        }
        else {
            location = RADIX;
        }
        for (j = location - 1; j >= 0; j--) {
            bit = (THREEpFm1[i] >> j) & 1;
            if (bit) {
                cubicalADD(PnQ, PnQ, nQ, ixP);
                if ((i == NWORDS_ORDER_3 - 1) && (j == location - 1)) {
                    copy_point(&nm1Q, nQ);
                }
                else {
                    cubicalADD(&nm1Q, nQ, &nm1Q, ixQ);
                }
                cubicalDBL(nQ, nQ, A24);
            }
            else {
                cubicalADD(PnQ, PnQ, &nm1Q, ixPQ);
                cubicalADD(nQ, nQ, &nm1Q, ixQ);
                cubicalDBL(&nm1Q, &nm1Q, A24);
            }
        }
    }
}

void
to_cubical_triple(ec_point_t* PQ,
    ec_point_t* Q,
    ec_point_t* P,
    fp2_t* ixPQ,
    fp2_t* ixQ,
    fp2_t* ixP)
{
    fp2_t T[6];
    fp2_copy(&T[0], &PQ->x);
    fp2_copy(&T[1], &PQ->z);
    fp2_copy(&T[2], &Q->x);
    fp2_copy(&T[3], &Q->z);
    fp2_copy(&T[4], &P->x);
    fp2_copy(&T[5], &P->z);
    fp2_batched_inv(T, 6);
    fp2_mul(ixPQ, &T[0], &PQ->z);
    fp2_mul(ixQ, &T[2], &Q->z);
    fp2_mul(ixP, &T[4], &P->z);
    fp2_mul(&PQ->x, &PQ->x, &T[1]);
    fp2_set_one(&PQ->z);
    fp2_mul(&Q->x, &Q->x, &T[3]);
    fp2_set_one(&Q->z);
    fp2_mul(&P->x, &P->x, &T[5]);
    fp2_set_one(&P->z);
}

void
monodromy_3(ec_point_t* R,
    ec_point_t* P,
    ec_point_t* Q,
    ec_point_t* PQ,
    ec_point_t* A24)
{
    fp2_t ixP, ixQ, ixPQ;
    to_cubical_triple(PQ, Q, P, &ixPQ, &ixQ, &ixP);
    ec_point_t PnQ, nQ;
    biext_ladder_3(&PnQ, &nQ, PQ, Q, P, &ixPQ, &ixQ, &ixP, A24);
    point_ratio(R, &PnQ, &nQ, P);
}

// applies frobenius a + ib --> a - ib to an fp2 element
void
fp2_frob(fp2_t* out, const fp2_t* in)
{
    fp_copy(&(out->re), &(in->re));
    fp_neg(&(out->im), &(in->im));
}

void
pow_to_pm1(ec_point_t* R)
{
    fp2_t frob, tmp;

    fp2_copy(&tmp, &R->x);
    fp2_frob(&frob, &R->x);
    fp2_mul(&R->x, &R->z, &frob);
    fp2_frob(&frob, &R->z);
    fp2_mul(&R->z, &tmp, &frob);
}

// compute the square of the reduced tate-pairing e_{T,3^b}(P,Q)，where b is maximal
// PQ should be P+Q in (X:Z) coordinates
// A24 must be normalized
void
reduced_tate_3(fp2_t* r,
    ec_point_t* P,
    ec_point_t* Q,
    ec_point_t* PQ,
    ec_point_t* A24)
{
    fp2_t frob, tmp;
    ec_point_t R;

    monodromy_3(&R, P, Q, PQ, A24);   

    fp2_copy(&tmp, &R.x);
    fp2_frob(&frob, &R.x);
    fp2_mul(&R.x, &R.z, &frob);
    fp2_frob(&frob, &R.z);
    fp2_mul(&R.z, &tmp, &frob);
    fp2_inv(&R.z);
    fp2_mul(r, &R.x, &R.z);

    fp2_pow_vartime(r, r, p_cofactor_for_3g, NWORDS_ORDER_2);
}

bool
fp2_dlog_3e_rec(digit_t* a, long len, fp2_t* pows_f, fp2_t* pows_g, long stacklen)
{
    if (len == 0) {
        // *a = 0;
        for (int i = 0; i < NWORDS_ORDER_3; i++) {
            a[i] = 0;
        }
        return true;
    }
    else if (len == 1) {
        fp2_t sqr_g;
        fp2_sqr(&sqr_g, &pows_g[stacklen - 1]);
        if (fp2_is_one(&pows_f[stacklen - 1])) {
            for (int i = 0; i < NWORDS_ORDER_3; i++) {
                a[i] = 0;
            }
            fp2_t temp;
            for (int i = 0; i < stacklen - 1; ++i) {
                fp2_sqr(&temp, &pows_g[i]);
                fp2_mul(&pows_g[i], &pows_g[i], &temp);  // new_g = g^3
            }
            return true;
        }
        else if (fp2_is_equal(&pows_f[stacklen - 1], &pows_g[stacklen - 1])) {
            a[0] = 2;
            for (int i = 1; i < NWORDS_ORDER_3; i++) {
                a[i] = 0;
            }
            fp2_t temp;
            for (int i = 0; i < stacklen - 1; ++i) {

                fp2_sqr(&temp, &pows_g[i]);
                fp2_mul(&pows_f[i], &pows_f[i], &temp); // new_f = f*g^2
                fp2_mul(&pows_g[i], &pows_g[i], &temp); // new_g = g^3
            }
            return true;
        }
        else if (fp2_is_equal(&pows_f[stacklen - 1], &sqr_g)) {
            a[0] = 1;
            for (int i = 1; i < NWORDS_ORDER_3; i++) {
                a[i] = 0;
            }
            fp2_t temp;
            for (int i = 0; i < stacklen - 1; ++i) {
                fp2_sqr(&temp, &pows_g[i]);
                fp2_mul(&pows_f[i], &pows_f[i], &pows_g[i]); // new_f = f*g
                fp2_mul(&pows_g[i], &pows_g[i], &temp); // new_g = g^3
            }
            return true;
        }
        else {
            return false;
        }
    }
    else {
        long right = (double)len * 0.5;
        long left = len - right;
        pows_f[stacklen] = pows_f[stacklen - 1];
        pows_g[stacklen] = pows_g[stacklen - 1];
        // This can be optimized later.
        fp2_t temp;
        for (int i = 0; i < left; i++) {
            fp2_sqr(&temp, &pows_f[stacklen]);
            fp2_mul(&pows_f[stacklen], &pows_f[stacklen], &temp);
            fp2_sqr(&temp, &pows_g[stacklen]);
            fp2_mul(&pows_g[stacklen], &pows_g[stacklen], &temp);
        }
        digit_t dlp1[NWORDS_ORDER_3], dlp2[NWORDS_ORDER_3], dlp_temp[NWORDS_ORDER_3];
        bool ok;
        ok = fp2_dlog_3e_rec(dlp1, right, pows_f, pows_g, stacklen + 1);
        if (!ok)
            return false;
        ok = fp2_dlog_3e_rec(dlp2, left, pows_f, pows_g, stacklen);
        if (!ok)
            return false;
        // a = dlp1 + 3^right * dlp2
        for (int i = 0; i < right; i++) {
            mp_add(dlp_temp, dlp2, dlp2, NWORDS_ORDER_3);
            mp_add(dlp2, dlp2, dlp_temp, NWORDS_ORDER_3);
        }
        mp_add(a, dlp2, dlp1, NWORDS_ORDER_3);

        return true;
    }
}

bool
fp2_dlog_3e(digit_t* scal, const fp2_t* f, const fp2_t* g, int e)
{
    long log, len = e;
    for (log = 0; len > 1; len >>= 1)
        log++;
    log += 1;

    fp2_t* pows_f = (fp2_t*)malloc(sizeof(fp2_t) * log);
    fp2_t* pows_g = (fp2_t*)malloc(sizeof(fp2_t) * log);
    pows_f[0] = *f;
    pows_g[0] = *g;

    fp2_inv(&pows_g[0]);

    for (int i = 0; i < NWORDS_ORDER_3; i++) {
        scal[i] = 0;
    }

    bool ok = fp2_dlog_3e_rec(scal, e, pows_f, pows_g, 1);
    assert(ok);
    (void)ok;

    return ok;
}

void
to_cubical_basis_6(ec_point_t* P,
	ec_point_t* Q,
	ec_point_t* R,
	ec_point_t* PQ,
	ec_point_t* RQ,
	ec_point_t* PR,
	fp2_t* ixP,
	fp2_t* ixQ,
	fp2_t* ixR,
	fp2_t* ixPQ,
	fp2_t* ixRQ,
	fp2_t* ixPR)
{
	fp2_t t[12];
	fp2_copy(&t[0], &P->x);
	fp2_copy(&t[1], &P->z);
	fp2_copy(&t[2], &Q->x);
	fp2_copy(&t[3], &Q->z);
	fp2_copy(&t[4], &R->x);
	fp2_copy(&t[5], &R->z);
	fp2_copy(&t[6], &RQ->x);
	fp2_copy(&t[7], &RQ->z);
	fp2_copy(&t[8], &PR->x);
	fp2_copy(&t[9], &PR->z);
	fp2_copy(&t[10], &PQ->x);
	fp2_copy(&t[11], &PQ->z);
	fp2_batched_inv(t, 12);
	fp2_mul(ixP, &P->z, &t[0]);
	fp2_mul(&P->x, &P->x, &t[1]);
	fp2_set_one(&P->z);
	fp2_mul(ixQ, &Q->z, &t[2]);
	fp2_mul(&Q->x, &Q->x, &t[3]);
	fp2_set_one(&Q->z);
	fp2_mul(ixR, &R->z, &t[4]);
	fp2_mul(&R->x, &R->x, &t[5]);
	fp2_set_one(&R->z);
	fp2_mul(ixRQ, &RQ->z, &t[6]);
	fp2_mul(&RQ->x, &RQ->x, &t[7]);
	fp2_set_one(&RQ->z);
	fp2_mul(ixPR, &PR->z, &t[8]);
	fp2_mul(&PR->x, &PR->x, &t[9]);
	fp2_set_one(&PR->z);
	fp2_mul(ixPQ, &PQ->z, &t[10]);
	fp2_mul(&PQ->x, &PQ->x, &t[11]);
	fp2_set_one(&PQ->z);
}

void
to_cubical_basis_9(ec_point_t* P,
    ec_point_t* Q,
    ec_point_t* P1,
    ec_point_t* P2,
    ec_point_t* PQ,
    ec_point_t* P1Q,
    ec_point_t* P2Q,
    ec_point_t* PP1,
    ec_point_t* PP2,
    fp2_t* ixP,
    fp2_t* ixQ,
    fp2_t* ixP1,
    fp2_t* ixP2,
    fp2_t* ixPQ,
    fp2_t* ixP1Q,
    fp2_t* ixP2Q,
    fp2_t* ixPP1,
    fp2_t* ixPP2)
{
    fp2_t t[18];
    fp2_copy(&t[0], &P->x);
    fp2_copy(&t[1], &P->z);
    fp2_copy(&t[2], &Q->x);
    fp2_copy(&t[3], &Q->z);
    fp2_copy(&t[4], &P1->x);
    fp2_copy(&t[5], &P1->z);
    fp2_copy(&t[6], &P2->x);
    fp2_copy(&t[7], &P2->z);
    fp2_copy(&t[8], &P1Q->x);
    fp2_copy(&t[9], &P1Q->z);
    fp2_copy(&t[10], &P2Q->x);
    fp2_copy(&t[11], &P2Q->z);
    fp2_copy(&t[12], &PP1->x);
    fp2_copy(&t[13], &PP1->z);
    fp2_copy(&t[14], &PP2->x);
    fp2_copy(&t[15], &PP2->z);
    fp2_copy(&t[16], &PQ->x);
    fp2_copy(&t[17], &PQ->z);
    fp2_batched_inv(t, 18);
    fp2_mul(ixP, &P->z, &t[0]);
    fp2_mul(&P->x, &P->x, &t[1]);
    fp2_set_one(&P->z);
    fp2_mul(ixQ, &Q->z, &t[2]);
    fp2_mul(&Q->x, &Q->x, &t[3]);
    fp2_set_one(&Q->z);
    fp2_mul(ixP1, &P1->z, &t[4]);
    fp2_mul(&P1->x, &P1->x, &t[5]);
    fp2_set_one(&P1->z);
    fp2_mul(ixP2, &P2->z, &t[6]);
    fp2_mul(&P2->x, &P2->x, &t[7]);
    fp2_set_one(&P2->z);
    fp2_mul(ixP1Q, &P1Q->z, &t[8]);
    fp2_mul(&P1Q->x, &P1Q->x, &t[9]);
    fp2_set_one(&P1Q->z);
    fp2_mul(ixP2Q, &P2Q->z, &t[10]);
    fp2_mul(&P2Q->x, &P2Q->x, &t[11]);
    fp2_set_one(&P2Q->z);
    fp2_mul(ixPP1, &PP1->z, &t[12]);
    fp2_mul(&PP1->x, &PP1->x, &t[13]);
    fp2_set_one(&PP1->z);
    fp2_mul(ixPP2, &PP2->z, &t[14]);
    fp2_mul(&PP2->x, &PP2->x, &t[15]);
    fp2_set_one(&PP2->z);
    fp2_mul(ixPQ, &PQ->z, &t[16]);
    fp2_mul(&PQ->x, &PQ->x, &t[17]);
    fp2_set_one(&PQ->z);
}

void tate_dlog_3(digit_t *scalarP,
	digit_t *scalarQ,
	ec_point_t *R,
	ec_point_t *P,
	ec_point_t *Q,
	ec_point_t *PQ,
	ec_point_t *PmR,
	ec_point_t *RmQ,
	ec_point_t *A24)
{
	int location, i, j, bit;
	fp2_t ixP, ixQ, ixR, ixPQ, ixPR, ixRQ, w0, w;
	ec_point_t nQ, nR, PnQ, PnR, RnQ, nm1Q, nm1R;
	ec_point_t R0, R1, R3;
	fp2_t t[3];

	to_cubical_basis_6(P, Q, R, PQ, RmQ, PmR, &ixP, &ixQ, &ixR, &ixPQ, &ixRQ, &ixPR);

	copy_point(&nQ, Q);
	copy_point(&nR, R);
	copy_point(&PnQ, PQ);
	copy_point(&PnR, PmR);
	copy_point(&RnQ, RmQ);

	for (i = NWORDS_ORDER_3 - 1; i >= 0; i--) {
		if (i == (NWORDS_ORDER_3 - 1)) {
			location = LOCATION;
		}
		else {
			location = RADIX;
		}
		for (j = location - 1; j >= 0; j--) {
			bit = (THREEpFm1[i] >> j) & 1;
			if (bit) {
				cubicalADD(&PnQ, &PnQ, &nQ, &ixP);
				cubicalADD(&RnQ, &RnQ, &nQ, &ixR);
				cubicalADD(&PnR, &PnR, &nR, &ixP);
				if ((i == NWORDS_ORDER_3 - 1) && (j == location - 1)) {
					copy_point(&nm1Q, &nQ);
					copy_point(&nm1R, &nR);
				}
				else {
					cubicalADD(&nm1Q, &nQ, &nm1Q, &ixQ);
					cubicalADD(&nm1R, &nR, &nm1R, &ixR);
				}
				cubicalDBL(&nQ, &nQ, A24);
				cubicalDBL(&nR, &nR, A24);
			}
			else {
				cubicalADD(&PnQ, &PnQ, &nm1Q, &ixPQ);
				cubicalADD(&RnQ, &RnQ, &nm1Q, &ixRQ);
				cubicalADD(&PnR, &PnR, &nm1R, &ixPR);
				cubicalADD(&nQ, &nQ, &nm1Q, &ixQ);
				cubicalADD(&nR, &nR, &nm1R, &ixR);
				cubicalDBL(&nm1Q, &nm1Q, A24);
				cubicalDBL(&nm1R, &nm1R, A24);
			}
		}
	}

	point_ratio(&R0, &PnQ, &nQ, P);
	point_ratio(&R1, &RnQ, &nQ, R);
	point_ratio(&R3, &PnR, &nR, P);


	pow_to_pm1(&R0);
	pow_to_pm1(&R1);
	pow_to_pm1(&R3);

	fp2_copy(&t[0], &R0.z);
	fp2_copy(&t[1], &R1.z);
	fp2_copy(&t[2], &R3.z);
	fp2_batched_inv(t, 3);

	fp2_mul(&w0, &R0.x, &t[0]);
	fp2_pow_vartime(&w0, &w0, p_cofactor_for_3g, NWORDS_ORDER_2);

	fp2_mul(&w, &R1.x, &t[1]);
	fp2_pow_vartime(&w, &w, p_cofactor_for_3g, NWORDS_ORDER_2);
	fp2_dlog_3e(scalarP, &w, &w0, TORSION_PLUS_THREE_POWER);

	fp2_mul(&w, &R3.x, &t[2]);
	fp2_pow_vartime(&w, &w, p_cofactor_for_3g, NWORDS_ORDER_2);
	fp2_dlog_3e(scalarQ, &w, &w0, TORSION_PLUS_THREE_POWER);
}

void
tate_basis_dlog_3(digit_t* scalarP1,
    digit_t* scalarQ1,
    digit_t* scalarP2,
    digit_t* scalarQ2,
    ec_point_t* P,
    ec_point_t* Q,
    ec_point_t* P1,
    ec_point_t* P2,
    ec_point_t* PQ,
    ec_point_t* PP1,
    ec_point_t* PP2,
    ec_point_t* P1Q,
    ec_point_t* P2Q,
    ec_point_t* A24)
{
    int location, i, j, bit;
    fp2_t ixP, ixQ, ixP1, ixP2, ixPQ, ixPP1, ixPP2, ixP1Q, ixP2Q, w0, w;
    ec_point_t nQ, nP1, nP2, PnQ, PnP1, PnP2, P1nQ, P2nQ, nm1Q, nm1P1, nm1P2;
    ec_point_t R0, R1, R2, R3, R4;
    fp2_t t[5];

    to_cubical_basis_9(P, Q, P1, P2, PQ, P1Q, P2Q, PP1, PP2,
        &ixP, &ixQ, &ixP1, &ixP2, &ixPQ, &ixP1Q, &ixP2Q, &ixPP1, &ixPP2);

    copy_point(&nQ, Q);
    copy_point(&nP1, P1);
    copy_point(&nP2, P2);
    copy_point(&PnQ, PQ);
    copy_point(&PnP1, PP1);
    copy_point(&PnP2, PP2);
    copy_point(&P1nQ, P1Q);
    copy_point(&P2nQ, P2Q);

    for (i = NWORDS_ORDER_3 - 1; i >= 0; i--) {
        if (i == (NWORDS_ORDER_3 - 1)) {
            location = LOCATION;
        }
        else {
            location = RADIX;
        }
        for (j = location - 1; j >= 0; j--) {
            bit = (THREEpFm1[i] >> j) & 1;
            if (bit) {
                cubicalADD(&PnQ, &PnQ, &nQ, &ixP);
                cubicalADD(&P1nQ, &P1nQ, &nQ, &ixP1);
                cubicalADD(&P2nQ, &P2nQ, &nQ, &ixP2);
                cubicalADD(&PnP1, &PnP1, &nP1, &ixP);
                cubicalADD(&PnP2, &PnP2, &nP2, &ixP);
                if ((i == NWORDS_ORDER_3 - 1) && (j == location - 1)) {
                    copy_point(&nm1Q, &nQ);
                    copy_point(&nm1P1, &nP1);
                    copy_point(&nm1P2, &nP2);
                }
                else {
                    cubicalADD(&nm1Q, &nQ, &nm1Q, &ixQ);
                    cubicalADD(&nm1P1, &nP1, &nm1P1, &ixP1);
                    cubicalADD(&nm1P2, &nP2, &nm1P2, &ixP2);
                }
                cubicalDBL(&nQ, &nQ, A24);
                cubicalDBL(&nP1, &nP1, A24);
                cubicalDBL(&nP2, &nP2, A24);
            }
            else {
                cubicalADD(&PnQ, &PnQ, &nm1Q, &ixPQ);
                cubicalADD(&P1nQ, &P1nQ, &nm1Q, &ixP1Q);
                cubicalADD(&P2nQ, &P2nQ, &nm1Q, &ixP2Q);
                cubicalADD(&PnP1, &PnP1, &nm1P1, &ixPP1);
                cubicalADD(&PnP2, &PnP2, &nm1P2, &ixPP2);
                cubicalADD(&nQ, &nQ, &nm1Q, &ixQ);
                cubicalADD(&nP1, &nP1, &nm1P1, &ixP1);
                cubicalADD(&nP2, &nP2, &nm1P2, &ixP2);
                cubicalDBL(&nm1Q, &nm1Q, A24);
                cubicalDBL(&nm1P1, &nm1P1, A24);
                cubicalDBL(&nm1P2, &nm1P2, A24);
            }
        }
    }

    point_ratio(&R0, &PnQ, &nQ, P);
    point_ratio(&R1, &P1nQ, &nQ, P1);
    point_ratio(&R2, &P2nQ, &nQ, P2);
    point_ratio(&R3, &PnP1, &nP1, P);
    point_ratio(&R4, &PnP2, &nP2, P);

    pow_to_pm1(&R0);
    pow_to_pm1(&R1);
    pow_to_pm1(&R2);
    pow_to_pm1(&R3);
    pow_to_pm1(&R4);

    fp2_copy(&t[0], &R0.z);
    fp2_copy(&t[1], &R1.z);
    fp2_copy(&t[2], &R2.z);
    fp2_copy(&t[3], &R3.z);
    fp2_copy(&t[4], &R4.z);
    fp2_batched_inv(t, 5);

    fp2_mul(&w0, &R0.x, &t[0]);
    fp2_pow_vartime(&w0, &w0, p_cofactor_for_3g, NWORDS_ORDER_2);

    fp2_mul(&w, &R1.x, &t[1]);
    fp2_pow_vartime(&w, &w, p_cofactor_for_3g, NWORDS_ORDER_2);
    fp2_dlog_3e(scalarP1, &w, &w0, TORSION_PLUS_THREE_POWER);

    fp2_mul(&w, &R2.x, &t[2]);
    fp2_pow_vartime(&w, &w, p_cofactor_for_3g, NWORDS_ORDER_2);
    fp2_dlog_3e(scalarP2, &w, &w0, TORSION_PLUS_THREE_POWER);

    fp2_mul(&w, &R3.x, &t[3]);
    fp2_pow_vartime(&w, &w, p_cofactor_for_3g, NWORDS_ORDER_2);
    fp2_dlog_3e(scalarQ1, &w, &w0, TORSION_PLUS_THREE_POWER);

    fp2_mul(&w, &R4.x, &t[4]);
    fp2_pow_vartime(&w, &w, p_cofactor_for_3g, NWORDS_ORDER_2);
    fp2_dlog_3e(scalarQ2, &w, &w0, TORSION_PLUS_THREE_POWER);
}

void ec_dlog_3_tate(digit_t *scalarP,
	digit_t *scalarQ,
	ec_point_t *R,
	ec_basis_t *PQ,
	ec_curve_t *curve) 
{
	ec_point_t AC, A24;
	ec_point_t PmR, RmQ;
	jac_point_t xyP, xyQ, xyR, temp;

	// precomputing the correct curve data
	fp2_copy(&AC.x, &curve->A);
	fp2_copy(&AC.z, &curve->C);
	A24_from_AC(&A24, &AC);

	// lifting the points
	lift_basis(&xyP, &xyQ, PQ, curve);
	lift_point(&xyR, R, curve);

	// computation of the differences
	jac_neg(&temp, &xyR);
	ADD(&temp, &temp, &xyP, curve);
	jac_to_xz(&PmR, &temp);
	jac_neg(&temp, &xyQ);
	ADD(&temp, &temp, &xyR, curve);
	jac_to_xz(&RmQ, &temp);

	tate_dlog_3(scalarP,
		scalarQ,
		R,
		&PQ->P,
		&PQ->Q,
		&PQ->PmQ,
		&PmR,
		&RmQ,
		&A24);

#if _DEBUG
	ec_point_t test_comput;
	ec_biscalar_mul_3(&test_comput, curve, scalarP, scalarQ, PQ);
	assert(ec_is_equal(&test_comput, R));
#endif
}

void ec_basis_dlog_3_tate(digit_t* scalarP1,
    digit_t* scalarQ1,
    digit_t* scalarP2,
    digit_t* scalarQ2,
    ec_basis_t* PQ,
    ec_basis_t* basis,
    ec_curve_t* curve)
{
    fp2_t w0, w;
    ec_point_t AC, A24;
    ec_point_t PmP1, P1mQ, PmP2, P2mQ;
    jac_point_t xyP, xyQ, xyP1, xyP2, temp;

    // we start by computing the different tate pairings

    // precomputing the correct curve data
    fp2_copy(&AC.x, &curve->A);
    fp2_copy(&AC.z, &curve->C);
    A24_from_AC(&A24, &AC);

    // lifting the two basis points
    lift_basis(&xyP, &xyQ, PQ, curve);
    lift_basis(&xyP1, &xyP2, basis, curve);

    // computation of the differences
    jac_neg(&temp, &xyP1);
    ADD(&temp, &temp, &xyP, curve);
    jac_to_xz(&PmP1, &temp);
    jac_neg(&temp, &xyP2);
    ADD(&temp, &temp, &xyP, curve);
    jac_to_xz(&PmP2, &temp);
    jac_neg(&temp, &xyQ);
    ADD(&temp, &temp, &xyP1, curve);
    jac_to_xz(&P1mQ, &temp);
    jac_neg(&temp, &xyQ);
    ADD(&temp, &temp, &xyP2, curve);
    jac_to_xz(&P2mQ, &temp);

    tate_basis_dlog_3(scalarP1,
        scalarQ1,
        scalarP2,
        scalarQ2,
        &PQ->P,
        &PQ->Q,
        &basis->P,
        &basis->Q,
        &PQ->PmQ,
        &PmP1,
        &PmP2,
        &P1mQ,
        &P2mQ,
        &A24);

#if _DEBUG
    ec_point_t test_comput;
    ec_biscalar_mul_3(&test_comput, curve, scalarP1, scalarQ1, PQ);
    assert(ec_is_equal(&test_comput, &basis->P));
    ec_biscalar_mul_3(&test_comput, curve, scalarP2, scalarQ2, PQ);
    assert(ec_is_equal(&test_comput, &basis->Q));
#endif
}
