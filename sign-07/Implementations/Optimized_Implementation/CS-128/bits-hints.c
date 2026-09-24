#include "bits-hints.h"

/*************************************************
* Name:        Power2Round
*
* Description: For finite field element a, compute low, hi such that
*              a mod^+ q = hi*2^d + low with -2^{d-1} < low <= 2^{d-1}.
*              Assumes a to be standard representative.
*
* Arguments:   - int32_t *low: pointer to output element low
*              - int32_t a: input element
*
* Returns hi.
**************************************************/
static int32_t Power2Round(int32_t* low, const int32_t a)
{
    int32_t hi;

    hi = (a + (1 << (beta - 1)) - 1) >> beta;
    *low = a - (hi << beta);
    return hi;
}

/*************************************************
* Name:        Power2RoundVector
*
* Description: For all coefficients c of the input polynomial,
*              compute c0, c1 such that c mod q = c1*2^d + c0
*              with -2^{d-1} < c0 <= 2^{d-1}. Assumes coefficients to be
*              standard representatives.
*
* Arguments:   - poly *hi: pointer to output polynomial with coefficients c1
*              - poly *low: pointer to output polynomial with coefficients c0
*              - const poly *a: pointer to input polynomial
**************************************************/
void Power2RoundVector(poly* hi, poly* low, const poly* a)
{
    int i, j;

    for (i = 0; i < k; i++)
    {
        for (j = 0; j < n; ++j)
            hi[i].coeffs[j] = Power2Round(&low[i].coeffs[j], a[i].coeffs[j]);
    }
}

/*************************************************
* Name:        CheckNormPoly
*
* Description: Check infinity norm of poly against given bound.
*              Assumes input coefficients were reduced.
*
* Arguments:   - const poly a: pointer to poly
*              - int_t bound: norm bound
*
* Returns 0 if norm is strictly smaller than bound and 1 otherwise.
**************************************************/
int CheckNormPoly(const poly* a, int bnd)
{
    int32_t i, t;

    /* It is ok to leak which coefficient violates the bound since
       the probability for each coefficient is independent of secret
       data but we must not leak the sign of the centralized representative. */
    for (i = 0; i < n; ++i) 
    {
        /* Absolute value */
        t = a->coeffs[i] >> 31;
        t = a->coeffs[i] - (t & 2 * a->coeffs[i]);

        if (t > bnd) { return 1; }
    }

    return 0;
}

/*************************************************
* Name:        CheckParityPoly
*
* Description: Check parity of polynomial.
*
* Arguments:   - poly* r: pointer to output polynomial
*              - const poly a: pointer to poly a
*
**************************************************/
void CheckParityPoly(poly* r, const poly a)
{
    int i;

    for (i = 0; i < n; ++i)
    {
        r->coeffs[i] = a.coeffs[i] & 1;
    }
}

/*************************************************
* Name:        CheckNormVector
*
* Description: Check infinity norm of polynomial against given bound.
*              Assumes input coefficients were reduced.
*
* Arguments:   - const poly *a: pointer to polynomial
*              - int_t dim: the dimension of polynomials vector 
*              - int_t bnd: norm bound
*
* Returns 0 if norm is strictly smaller than bound and 1 otherwise.
**************************************************/
int CheckNormVector(const poly* a, int dim, int bnd) 
{
    int i;

    for (i = 0; i < dim; i++)
    {
        if (CheckNormPoly(&a[i], bnd)) { 
            return 1; }
    }
    return 0;
}

/*************************************************
* Name:        HighBits
*
* Description: For finite field element a, compute low, hi such that
*              a mod^+ 2q = hi*alpha + low with -alpha/2 <= low < alpha/2.
*              Assumes a to be standard representative.
*
* Arguments:   - int32_t *low: pointer to output element low
*              - int32_t a: input element
*
* Returns hi.
**************************************************/
static int32_t HighBits(const int32_t a)
{
#if alpha == 2016 && q == 32257
    int32_t hi, lo;

    hi = a >> 11;
    lo = (a & 2047) + (hi << 5);
    hi -= (1007 - lo) >> 31;
    hi &= (hi - 32) >> 31;
#elif alpha == 4032 && q == 64513
    int32_t hi, lo;

    hi = a >> 12;
    lo = (a & 4095) + (hi << 6);
    hi -= (2015 - lo) >> 31;
    hi &= (hi - 32) >> 31;
#elif alpha == 8064 && q == 64513
    int32_t hi, lo;

    hi = a >> 13;
    lo = (a & 8191) + (hi << 7);
    hi -= (4031 - lo) >> 31;
    hi &= (hi - 16) >> 31;
#endif
    return hi;
}

/*************************************************
* Name:        HighBitsVector
*
* Description: For all coefficients c of the input polynomial,
*              compute low, hi such that a mod^+ 2q = hi*2^alpha + low
*              with -2^(alpha-1) < low < 2^(alpha-1). Assumes coefficients to be
*              standard representatives.
*
* Arguments:   - int32_t* hi: pointer to output polynomial with high part coefficients
*              - poly *low: pointer to output polynomial with low part coefficients
*              - const poly *a: pointer to input polynomial
**************************************************/
void HighBitsVector(poly* hi, const poly* a)
{
    int i, j;

    for (i = 0; i < k; i++)
    {
        for (j = 0; j < n; ++j)
            hi[i].coeffs[j] = HighBits(a[i].coeffs[j]);
    }
}


/*************************************************
* Name:       HighBitsZ2Vector
*
* Description: compute w - 2*z2, w in [0,2q), and z2 in (-bound1, bound1).
*
* Arguments:   - poly *r: pointer to input/output polynomial
*              - const poly* w: pointer to input polynomial
*              - const poly* z2: pointer to input polynomial
*
**************************************************/
void HighBitsZ2Vector(poly* r, const poly* w, const poly* z2)
{
    int i, j;

    for (i = 0; i < k; i++)
    {
        for (j = 0; j < n; j++)
        {
            r[i].coeffs[j] = w[i].coeffs[j] - (z2[i].coeffs[j] << 1);
            r[i].coeffs[j] += ((r[i].coeffs[j] >> 31) & dq)- dq;
            r[i].coeffs[j] += ((r[i].coeffs[j] >> 31) & dq);
            r[i].coeffs[j] = HighBits(r[i].coeffs[j]);
        }
    }
}

/*************************************************
* Name:       hModpVector
*
* Description: compute w1 - w0 mod p, p = 2 * (q-1) / alpha.
*
* Arguments:   - poly *r: pointer to input/output polynomial
*              - const poly* w1: pointer to input polynomial
*              - const poly* w0: pointer to input polynomial
*
**************************************************/
void hModpVector(poly* r, const poly* w1, const poly* w0, int sgn)
{
    int i, j, tmp;
    const p = 2 * q / alpha, hp = p / 2, mask = p - 1;

    if (sgn)
    {
        for (i = 0; i < k; i++)
        {
            for (j = 0; j < n; j++)
            {
                tmp = (w1[i].coeffs[j] - w0[i].coeffs[j]) & mask;
                r[i].coeffs[j] = tmp - (((hp - 1 - tmp) >> 31) & p);//mod +- p
            }
        }
    }
    else
    {
        for (i = 0; i < k; i++)
        {
            for (j = 0; j < n; j++)
            {
                r[i].coeffs[j] = (w1[i].coeffs[j] + w0[i].coeffs[j]) & mask;//mod + p
            }
        }
    }
}

/*************************************************
* Name:        DecomposePoly
*
* Description: For all coefficients c of the input polynomial,
*              compute low, hi such that a mod q = hi*2^d + low
*              with 0 <= low < 2^d. Assumes coefficients to be
*              standard representatives.
*
* Arguments:   - int32_t* hi: pointer to output polynomial with high part coefficients
*              - poly *low: pointer to output polynomial with low part coefficients
*              - const poly a: pointer to input polynomial
**************************************************/
void DecomposePoly(int32_t* hi, poly* low, const poly a, int dc)
{
    int i;

    for (i = 0; i < n; ++i)
    {
        hi[i] = (a.coeffs[i] + (1 << (dc - 1)) - 1) >> dc;
        low->coeffs[i] = a.coeffs[i] - (hi[i] << dc);
    }
}

/*************************************************
* Name:        DecomposeVector
*
* Description: For all coefficients c of the input polynomial,
*              compute low, hi such that a mod q = hi*2^d + low
*              with 0 <= low < 2^d. Assumes coefficients to be
*              standard representatives.
*
* Arguments:   - int32_t* hi: pointer to output polynomial with high part coefficients
*              - poly *low: pointer to output polynomial with low part coefficients
*              - const poly *a: pointer to input polynomial
**************************************************/
void DecomposeVector(int32_t* hi, poly* low, const poly* a, int dim, int dc)
{
    int i;

    for (i = 0; i < dim; i++)
    {
        DecomposePoly(hi + i * n, &low[i], a[i], dc);
    }
}

/*************************************************
* Name:        ComposeVector
*
* Description: For all coefficients a of the output polynomial, compute a = hi*2^d + low
*
* Arguments:   - poly *a: pointer to output polynomial
*              - const poly *low: pointer to input polynomial with low part coefficients
*              - const int32_t* hi: pointer to input all high part coefficients
**************************************************/
void ComposeVector(poly* a, const poly* low, const int32_t* hi, int dim, int dc)
{
    int i, j;

    for (i = 0; i < dim; i++)
    {
        for (j = 0; j < n; ++j)
            a[i].coeffs[j] = (hi[i * n + j] << dc) + low[i].coeffs[j];
    }
}