#include "NTT.h"

// used in the number-theoretic transform, with montgomery 2^32
#if q == 32257
#define mont_r    12260 // 2^32 mod q
#define mont_qinv 1174635009 // q^-1 mod 2^32

static const int32_t zetas[n / 2] = {
    8994, 11130, 29371, 14482, 15919, 19214, 21832, 876, 2027, 31966, 5327, 4073, 23920, 24399, 24574, 3681,
    10068, 30557, 1745, 10714, 27449, 5528, 5331, 9865, 328, 23346, 25560, 12101, 6751, 1577, 6825, 11958,
    1056, 13009, 20137, 30305, 31176, 15305, 32201, 15683, 5763, 22518, 21097, 1077, 31581, 21119, 16556, 6137,
    30009, 2853, 19936, 29570, 24539, 17515, 13804, 21109, 14857, 29774, 9095, 8704, 5349, 3672, 15585, 19437,
    830, 8331, 30062, 15083, 31835, 1827, 15107, 4690, 31105, 9268, 1492, 31454, 18774, 24358, 5926, 486,
    21204, 26985, 8827, 7724, 7252, 17371, 1808, 5167, 25912, 5685, 19306, 20526, 17217, 27812, 23063, 9229,
    18910, 27744, 10230, 7077, 11372, 15586, 27833, 13191, 3679, 4787, 21556, 20569, 11110, 23294, 17644, 968,
    15950, 31845, 26608, 13526, 3161, 28891, 26035, 22504, 12451, 29642, 8851, 10219, 3230, 32032, 5449, 19444
};
#elif q == 64513
#define mont_r    14321 // 2^32 mod q
#define mont_qinv 940508161 // q^-1 mod 2^32

static const int32_t zetas[n / 2] = {
    34793, 26964, 48008, 22229, 30746, 20243, 19064, 33295, 9395, 33528, 22859, 55662, 32144, 13744, 21408, 17599,
    48474, 41567, 6241, 44960, 10681, 22935, 22431, 35409, 28147, 36986, 35380, 44478, 20143, 53152, 30820, 25252,
    41951, 57724, 54464, 9383, 16304, 52217, 16446, 18239, 63217, 44788, 32437, 11782, 46572, 29643, 55936, 7893,
    43049, 44867, 49383, 62122, 30608, 40543, 47905, 19616, 56572, 26533, 45384, 27690, 7597, 53054, 10615, 55083,
    11591, 7814, 12697, 32114, 60752, 54909, 19813, 20353, 17456, 48246, 44958, 598, 34571, 4538, 835, 15546,
    3970, 36828, 1488, 8311, 52071, 31352, 46882, 1806, 59171, 9790, 29068, 16507, 35462, 22131, 6759, 15510,
    49572, 28710, 1160, 33186, 24985, 11261, 53890, 36786, 21502, 18731, 48327, 60386, 45681, 12050, 50012, 7929,
    29563, 33449, 5913, 5322, 48108, 2844, 29439, 5876, 54991, 45927, 54639, 23844, 30362, 43071, 9560, 17671,
    36524, 3350, 787, 50656, 1657, 43289, 57139, 55323, 2464, 25555, 60984, 35741, 16588, 48774, 23475, 13666,
    5764, 30980, 13633, 57112, 34196, 28847, 7682, 52705, 55717, 14864, 40351, 45319, 689, 63202, 33181, 48194,
    1025, 10971, 41497, 61865, 42613, 51970, 38592, 28254, 28521, 48353, 12380, 51631, 34181, 47883, 23439, 7742,
    17182, 17494, 5920, 13642, 7382, 46347, 21422, 34239, 36323, 13283, 44197, 54574, 10672, 21454, 6080, 47139,
    34778, 38601, 54343, 3808, 10639, 37528, 53648, 25636, 17261, 37662, 56260, 61209, 18282, 62311, 33145, 42270,
    13882, 12069, 53271, 56784, 54287, 1761, 37215, 59713, 46776, 41708, 60985, 65, 10770, 8908, 40762, 26934,
    21921, 37503, 42569, 8889, 63478, 23224, 55025, 58690, 63519, 44307, 7655, 48262, 41693, 36773, 15822, 23078,
    13803, 56414, 2931, 9217, 43387, 50310, 25492, 51682, 7947, 17463, 51534, 29003, 31612, 26554, 8241, 44338,
};
#else 
#error q must be in {32257, 64513}
#endif

/*************************************************
* Name:        MontMultMod
*
* Description: Montgomery reduction; given two 32-bit integer a and b computes
*              32-bit integer congruent to a * b * R^-1 mod q, where R=2^32
*
* Arguments:   - int32_t a: input integer to be reduced;
*                           has to be in {-q2^31,...,q2^31-1}
* Arguments:   - int32_t b: input integer to be reduced;
*                           has to be in {-q2^31,...,q2^31-1}
*
* Returns:     integer in {-q+1,...,q-1} congruent to a * b * R^-1 modulo q.
**************************************************/
static int32_t MontMultMod(const int32_t a, const int32_t b)
{
    int64_t t;
    int32_t u;

    t = (int64_t)a * b;
    u = (int32_t)t * mont_qinv;
    u = (t - (int64_t)u * q) >> 32;
    return u;
}

/*************************************************
* Name:        NTT
*
* Description: Forward NTT, in-place. No modular reduction is performed after
*              additions or subtractions. Output vector is in bitreversed order.
*
* Arguments:   - uint32_t p[n]: input/output coefficient array
**************************************************/
poly NTT(poly* r)
{
    poly w;
    int32_t t, zeta;
    int len, start, i, j;

    for (len = n / 2, i = 0; i < n / 2; i++)
    {
        t = MontMultMod(zetas[1], r->coeffs[i + len]);
        w.coeffs[i + len] = r->coeffs[i] - t;
        w.coeffs[i] = r->coeffs[i] + t;
    }

    for (j = 2, len = n / 4; len >= 2; len >>= 1)
    {
        for (start = 0; start < n; start += 2 * len)
        {
            zeta = zetas[j++];
            for (i = start; i < start + len; ++i)
            {
                t = MontMultMod(zeta, w.coeffs[i + len]);
                w.coeffs[i + len] = w.coeffs[i] - t;
                w.coeffs[i] = w.coeffs[i] + t;
            }
        }
    }

    return w;
}

/*************************************************
* Name:        INTT
*
* Description: Inverse NTT and multiplication by Montgomery factor 2^32.
*              In-place. No modular reductions after additions or
*              subtractions; input coefficients need to be smaller than
*              q in absolute value. Output coefficient are smaller than q in
*              absolute value.
*
* Arguments:   - uint32_t p[n]: input/output coefficient array
**************************************************/
void INTT(poly* r)
{
    int32_t t, zetainv;
    int start, len, i, j = n / 2;

    for (len = 2; len < n; len <<= 1)
    {
        for (start = 0; start < n; start += 2 * len)
        {
            zetainv = zetas[--j];
            for (i = start; i < start + len; ++i)
            {
                t = r->coeffs[i + len] - r->coeffs[i];
                r->coeffs[i] = r->coeffs[i + len] + r->coeffs[i];
                r->coeffs[i + len] = MontMultMod(zetainv, t);
            }
        }
    }

    for (i = 0; i < n; ++i)
        r->coeffs[i] = MontMultMod(r->coeffs[i], zetas[0]);
}

/*************************************************
* Name:        AddPoly
*
* Description: Add two polynomials
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void AddPoly(poly* r, const poly* a, const poly* b)
{
    int i;

    for (i = 0; i < n; i++)
        r->coeffs[i] = a->coeffs[i] + b->coeffs[i];
}

/*************************************************
* Name:        AddNTT
*
* Description: Add two polynomials in NTT domain representation
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void AddNTT(poly* r, const poly* a, const poly* b)
{
    int i;

    for (i = 0; i < n; i++)
        r->coeffs[i] = a->coeffs[i] + b->coeffs[i];
}

/*************************************************
* Name:        SubPoly
*
* Description: Subtract polynomial a by polynomial b
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
**************************************************/
void SubPoly(poly* r, const poly* a, const poly* b)
{
    int i;

    for (i = 0; i < n; i++)
        r->coeffs[i] = a->coeffs[i] - b->coeffs[i];
}

/*************************************************
* Name:        AddorPoly
*
* Description: Add or Subtract polynomial a by polynomial b with controls parameter
*
* Arguments: - poly *r:       pointer to output polynomial
*            - const poly *a: pointer to first input polynomial
*            - const poly *b: pointer to second input polynomial
*            - int32_t cp:    pointer to controls parameter
**************************************************/
void AddorSubPoly(poly* r, const poly* a, const poly* b, int32_t cp)
{
    int i;

    if (cp) {
        for (i = 0; i < n; i++)
            r->coeffs[i] = a->coeffs[i] - b->coeffs[i];
    }
    else {
        for (i = 0; i < n; i++)
            r->coeffs[i] = a->coeffs[i] + b->coeffs[i];
    }
}

/*************************************************
* Name:        MultiplyPoly
*
* Description: Multiplication of two polynomials.
*
* Arguments:   - poly *r:       pointer to output polynomial
*              - const poly *a: pointer to first input polynomial
*              - const int32_t* coordinates: pointer to num coordinates of the polynomial a
*              - int32_t num:   pointer to num of non-zero coordinates of the polynomial a
*              - const poly *b: pointer to second input polynomial
**************************************************/
void MultiplyPoly(poly* r, const poly* a, const int32_t* coordinates, int32_t num, const poly* b)
{
    int i, j, s, t;

    for (i = 0; i < n; i++) { r->coeffs[i] = 0; }

    for (i = 0; i < num; i++)
    {
        s = coordinates[i];
        t = (-a->coeffs[s]) >> 31;
        for (j = 0; j < s; j++)
        {
            r->coeffs[j] += (b->coeffs[n - s + j] ^ t) - t;
        }
        t = -t - 1;
        for (j = s; j < n; j++)
        {
            r->coeffs[j] += (b->coeffs[j - s] ^ t) - t;
        }
    }
}

/*************************************************
* Name:        TruncPoly
*
* Description: Truncate a polynomial.
*
* Arguments:   - poly *r:       pointer to output polynomial
*              - const poly *a: pointer to input polynomial
*              - const int32_t* coordinates: pointer to num coordinates of the output polynomial
*              - int32_t num:   pointer to num of non-zero coordinates of the output polynomial
**************************************************/
void TruncPoly(poly* r, const poly* a, const int32_t* coordinates, int32_t num)
{
    int i;

    for (i = 0; i < n; i++) { r->coeffs[i] = 0; }

    for (i = 0; i < num; i++)
    {
        r->coeffs[coordinates[i]] = a->coeffs[coordinates[i]];
    }
}

/*************************************************
* Name:        BaseCaseMontMultiply
*
* Description: Multiplication of polynomials in Zq[X]/(X^2-zeta)
*              used for multiplication of elements in Rq in NTT domain,
*              and multiplication by Montgomery factor 2^-32.
*
* Arguments:   - int32_t r[2]:       pointer to the output polynomial
*              - const int32_t a[2]: pointer to the first factor
*              - const int32_t b[2]: pointer to the second factor
*              - int32_t zeta:       integer defining the reduction polynomial
**************************************************/
static void BaseCaseMontMultiply(int32_t r[2], const int32_t a[2], const int32_t b[2], int32_t zeta)
{
    int32_t s, t;

    s = MontMultMod(a[0], b[0]);
    t = MontMultMod(a[1], b[1]);

    r[1] = MontMultMod(a[0] + a[1], b[0] + b[1]) - s - t;
    r[0] = s + MontMultMod(t, zeta);
}

/*************************************************
* Name:        MultiplyNTT
*
* Description: Multiplication of two polynomials in NTT domain
*
* Arguments:   - poly *r:       pointer to output polynomial
*              - const poly *a: pointer to first input polynomial
*              - const poly *b: pointer to second input polynomial
**************************************************/
static void MultiplyNTT(poly* r, const poly* a, const poly* b)
{
    int i, quat = n / 4;

    for (i = 0; i < quat; i++)
    {
        BaseCaseMontMultiply(&r->coeffs[4 * i], &a->coeffs[4 * i], &b->coeffs[4 * i], zetas[quat + i]);
        BaseCaseMontMultiply(&r->coeffs[4 * i + 2], &a->coeffs[4 * i + 2], &b->coeffs[4 * i + 2], -zetas[quat + i]);
    }
}

/*************************************************
* Name:        AddVector
*
* Description: Add elements of a and b, into r.
*
* Arguments: - poly *u:          pointer to output polynomial
*            - const poly *v: pointer to first input vector of polynomials
*            - const poly *w: pointer to second input vector of polynomials
* 
**************************************************/
void AddVector(poly* u, const poly* v, const poly* w)
{
    int i;

    for (i = 0; i < k; i++)
    {
        AddPoly(&u[i], &v[i], &w[i]);
    }
}

/*************************************************
* Name:        SubVector
*
* Description: Sub elements of a and b, into r.
*
* Arguments: - poly *u:          pointer to output polynomial
*            - const poly *v: pointer to first input vector of polynomials
*            - const poly *w: pointer to second input vector of polynomials
*
**************************************************/
void SubVector(poly* u, const poly* v, const poly* w)
{
    int i, j;

    for (i = 0; i < k; i++)
    {
        for (j = 0; j < n; j++)
        {
            u[i].coeffs[j] = v[i].coeffs[j] - w[i].coeffs[j];
        }
    }
}

/*************************************************
* Name:        ShiftLeftVector
*
* Description: Multiply polynomial by 2^value without modular reduction. Assumes
*              input coefficients to be less than 2^{31-d} in absolute value.
*
* Arguments:   - poly *r: pointer to input/output polynomial
*              - int value: pointer to input value
**************************************************/
void ShiftLeftVector(poly* r, int value)
{
    int i, j;

    for (i = 0; i < k; i++)
    {
        for (j = 0; j < n; j++)
        {
            r[i].coeffs[j] <<= value;
        }
    }
}

/*************************************************
* Name:        ScalarVector
*
* Description: Multiply elements of w by a scalar number.
*
* Arguments: - poly *w:          pointer to input/output polynomial
*
**************************************************/
void ScalarVector(poly* w, const int scalar)
{
    int i, j;

    for (i = 0; i < k; i++)
    {
        for (j = 0; j < n; j++)
        {
            w[i].coeffs[j] *= scalar;
        }
    }
}

/*************************************************
* Name:        HalfCenterModVector
*
* Description: Divide polynomial by 2 with center modular reduction. 
*
* Arguments:   - poly *r: pointer to input/output polynomial
*
**************************************************/
void HalfCenterModVector(poly* r)
{
    int32_t i, j, tmp, hq = q >> 1;

    for (i = 0; i < k; i++)
    {
        for (j = 0; j < n; j++)
        {
            r[i].coeffs[j] >>= 1;
            r[i].coeffs[j] += (r[i].coeffs[j] >> 31) & q;
            tmp = hq - r[i].coeffs[j];
            r[i].coeffs[j] -= (tmp >> 31) & q;
        }
    }
}

/*************************************************
* Name:        ScalarMulVector
*
* Description: Multiply elements of v by a scalar c, into w.
*
* Arguments: - poly *w:          pointer to output polynomial
*            - const poly c:     pointer to first input poly
*            - const int32_t* coordinates: pointer to num coordinates of the polynomial a
*            - const poly *v:    pointer to second input vector of polynomials
*
**************************************************/
void ScalarMulVector(poly* w, const poly c, const int32_t* coordinates, const poly* v)
{
    int i;

    for (i = 0; i < k; i++)
    {
        MultiplyPoly(&w[i], &c, coordinates, tau, &v[i]);
    }
}

/*************************************************
* Name:        ScalarVectorNTT
*
* Description: Multiply elements of v by a scalar polynomial c, into w.
*
* Arguments: - poly *w:          pointer to output polynomial
*            - const poly c:     pointer to first input poly
*            - const poly *v:    pointer to second input vector of polynomials
* 
**************************************************/
void ScalarVectorNTT(poly* w, const poly c, const poly* v)
{
    int i;

    for (i = 0; i < k; i++)
    {
        MultiplyNTT(&w[i], &c, &v[i]);
    }
}

/*************************************************
* Name:        MatrixVectorNTT
*
* Description: Pointwise multiply vectors of polynomials of length L, multiply
*              resulting vector by 2^{-32} and add (accumulate) polynomials
*              in it. Input/output vectors are in NTT domain representation.
*
* Arguments:   - poly *w: output polynomial
*              - const poly **M: pointer to input matrix
*              - const poly *v: pointer to input vector
**************************************************/
void MatrixVectorNTT(poly* w,
                     const poly M[k][l],
                     const poly* v)
{
    poly t;
    int i, j;

    for (i = 0; i < k; i++)
    {
        MultiplyNTT(&w[i], &M[i][0], &v[0]);
        for (j = 1; j < l; j++)
        {
            MultiplyNTT(&t, &M[i][j], &v[j]);
            AddNTT(&w[i], &w[i], &t);
        }
    }
}

/*************************************************
* Name:       LazyReductionVector
*
* Description: Add q if input coefficient is negative.
*
* Arguments:   - poly *r: pointer to input/output polynomial
*
**************************************************/
void LazyReductionVector(poly* r)
{
    int i, j;

    for (i = 0; i < k; i++)
    {
        for (j = 0; j < n; j++)
        {
            r[i].coeffs[j] += (r[i].coeffs[j] >> 31) & q;
        }
    }
}

/*************************************************
* Name:       ExchangeModulus
*
* Description: Transform modulus 2 and modulus q to modulus 2q.
*
* Arguments:   - poly *r: pointer to input/output polynomial
*              - const poly y: pointer to input polynomial
*
**************************************************/
void ExchangeModulus(poly* b, const poly y)
{
    int i, j;

    for (i = 0; i < k; i++)
    {
        for (j = 0; j < n; j++)
        {
            b[i].coeffs[j] <<= 1;
        }
    }
    
    for (i = 0; i < n; i++)
    {
        b[0].coeffs[i] -= (y.coeffs[i] & 1) * q;
        b[0].coeffs[i] += (b[0].coeffs[i] >> 31) & dq;
    }
}