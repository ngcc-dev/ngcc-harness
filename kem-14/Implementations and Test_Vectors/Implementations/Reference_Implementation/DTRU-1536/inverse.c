#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "params.h"
#include "inverse.h"
#include "reduce.h"
#include "ntt.h"

/* Based on the reference implementation of NTRU Prime (NIST 3rd round submission)
 * by Daniel J. Bernstein, Chitchanok Chuengsatiansup, Tanja Lange, Christine van Vredendaal.
 * It can be used for q = 641.
 * */

void uint32_divmod_uint14(uint32_t *y, uint16_t *r, uint32_t x, uint16_t m)
{
    uint32_t w = 0x80000000;
    uint32_t qpart;
    uint32_t mask;

    w /= m;

    *y = 0;
    qpart = (x * (uint64_t)w) >> 31;
    x -= qpart * m;
    *y += qpart;

    qpart = (x * (uint64_t)w) >> 31;
    x -= qpart * m;
    *y += qpart;

    x -= m;
    *y += 1;
    mask = -(x >> 31);
    x += mask & (uint32_t)m;
    *y += mask;

    *r = x;
}

void int32_divmod_uint14(int32_t *y, uint16_t *r, int32_t x, uint16_t m)
{
    uint32_t uq, uq2;
    uint16_t ur, ur2;
    uint32_t mask;

    uint32_divmod_uint14(&uq, &ur, 0x80000000 + (uint32_t)x, m);
    uint32_divmod_uint14(&uq2, &ur2, 0x80000000, m);

    ur -= ur2;
    uq -= uq2;

    mask = -(uint32_t)(ur >> 15);
    ur += mask & m;
    uq += mask;
    *r = ur;
    *y = uq;
}

uint16_t int32_mod_uint14(int32_t x, uint16_t m)
{
    int32_t y;
    uint16_t r;
    int32_divmod_uint14(&y, &r, x, m);
    return r;
}

int16_t fq_freeze(int32_t x)
{

    const int16_t half_q = (DTRU_Q - 1) >> 1;
    return int32_mod_uint14(x + half_q, DTRU_Q) - half_q;
}

int int16_nonzero_mask(int16_t x)
{
    uint16_t u = x;
    uint32_t w = u;
    w = -w;
    w >>= 31;
    return -w;
}

int int16_negative_mask(int16_t x)
{
    uint16_t u = x;
    u >>= 15;
    return -(int)u;
}

int rq_inverse(int16_t finv[ROOT_DIMENSION], const int16_t f[ROOT_DIMENSION], const int16_t zeta)
{
    int16_t Phi[ROOT_DIMENSION + 1] = {1, 0, zeta};
    int16_t V[ROOT_DIMENSION + 1] = {0};
    int16_t S[ROOT_DIMENSION + 1] = {1, 0, 0};
    int16_t F[ROOT_DIMENSION + 1] = {f[1], f[0], 0};
    int i, loop, swap, t;
    int Delta = 1;
    int32_t Phi0, F0;
    int16_t scale;

    for (loop = 0; loop < 2 * ROOT_DIMENSION - 1; ++loop)
    {
        for (i = ROOT_DIMENSION; i > 0; --i)
            V[i] = V[i - 1];
        V[0] = 0;

        swap = int16_negative_mask(-Delta) & int16_nonzero_mask(F[0]);

        for (i = 0; i < ROOT_DIMENSION + 1; ++i)
        {
            t = swap & (Phi[i] ^ F[i]);
            Phi[i] ^= t;
            F[i] ^= t;
            t = swap & (V[i] ^ S[i]);
            V[i] ^= t;
            S[i] ^= t;
        }

        Delta ^= swap & (Delta ^ -Delta);
        Delta++;

        Phi0 = Phi[0];
        F0 = F[0];

        for (i = 0; i < ROOT_DIMENSION + 1; ++i)
            F[i] = fq_freeze(Phi0 * F[i] - F0 * Phi[i]);
        for (i = 0; i < ROOT_DIMENSION; ++i)
            F[i] = F[i + 1];
        F[ROOT_DIMENSION] = 0;

        for (i = 0; i < ROOT_DIMENSION + 1; ++i)
            S[i] = fq_freeze(Phi0 * S[i] - F0 * V[i]);
    }

    scale = Phi[0];
    scale += (scale >> 15) & DTRU_Q;
    // scale = fq_inverse_table[scale];
    scale = fqinv(scale);
    for (i = 0; i < ROOT_DIMENSION; ++i)
        finv[i] = fq_freeze(scale * (int32_t)V[ROOT_DIMENSION - 1 - i]);

    return int16_nonzero_mask(Delta);
}

int rq_inverse_det(int16_t b[4], const int16_t a[4], int16_t zeta)
{
    int16_t det;
    int16_t f0_sq, f1_sq, f2_sq, f3_sq, zeta_sq;
    int16_t f0_f, f1_f, f2_f, f3_f;
    int16_t tmp, tmp2;
    int r;
    f0_sq = fqmul(a[0], a[0]);
     
    f1_sq = fqmul(a[1], a[1]);
    f2_sq = fqmul(a[2], a[2]);
    f3_sq = fqmul(a[3], a[3]);
    zeta_sq = fqmul(zeta, zeta);
   
    f0_f = fqmul(f0_sq, f0_sq);
    
    f1_f = fqmul(f1_sq, f1_sq);
    f2_f = fqmul(f2_sq, f2_sq);
    f3_f = fqmul(f3_sq, f3_sq);
   

    // compute determinant
   
    det = 4*f0_sq;
    
    det = fqmul(a[1], det);
   
    det = fqmul(a[3], det);
   
    det = fqmul(det, zeta);
    
    det = f0_f - det;
 
    tmp = 2*f0_sq;
     
    tmp = fqmul(tmp, f2_sq);
    
    tmp = fqmul(tmp, zeta);
    
    det -= tmp;

    
    tmp = 4*a[0];
    
    tmp2 = fqmul(tmp, a[2]);
    
    tmp2 = fqmul(tmp2, f3_sq);
   
    tmp2 = fqmul(tmp2, zeta_sq);
     
    tmp = fqmul(tmp, f1_sq);
   
    tmp = fqmul(tmp, a[2]);
   
    tmp = fqmul(tmp, zeta);
     

     
    det += tmp;
    
    det += tmp2;
     
    tmp =  fqmul(f1_f, zeta);
    
    det -= tmp;
     
    tmp = 2*f1_sq;
    tmp = fqmul(tmp, f3_sq);
    tmp = fqmul(tmp, zeta_sq);
    
    det += tmp;
    
    tmp = 4*a[1];
    tmp = fqmul(tmp, f2_sq);
    tmp = fqmul(tmp, a[3]);
    tmp = fqmul(tmp, zeta_sq);
    
    det -= tmp;
     
    tmp = fqmul(f2_f,zeta_sq);
   
    det += tmp;
   
    tmp = fqmul(zeta_sq, zeta);
    tmp = fqmul(tmp, f3_f);
  
  
    det -= tmp; //
     
    // compute inverse of determinant
    det = fqinv(det);
    
    b[0] =  fqmul(f0_sq, a[0]);
   
    tmp = 2*a[0];
    tmp = fqmul(tmp, a[1]);
    tmp = fqmul(tmp, a[3]);
    tmp = fqmul(tmp, zeta);
 
    b[0] -= tmp;
 
    tmp = fqmul(a[0], f2_sq);
    tmp = fqmul(tmp, zeta);
 
    b[0] -= tmp;
 
    tmp = fqmul(a[2], f1_sq);
    tmp = fqmul(tmp, zeta);
 
    b[0] += tmp;


    tmp = fqmul(a[2], f3_sq);
    tmp = fqmul(tmp, zeta_sq);


    b[0] += tmp;
    
    b[1]= 2*a[0];
    b[1] = fqmul(b[1], a[2]);
    b[1] = fqmul(b[1], a[3]);
    b[1] = fqmul(b[1], zeta);

    tmp = fqmul(f0_sq, a[1]);
    b[1] -= tmp;

    tmp = fqmul(f1_sq, a[3]);
    tmp = fqmul(tmp, zeta);
    b[1] += tmp;

    tmp = fqmul(a[1], f2_sq);
    tmp = fqmul(tmp, zeta);
    b[1] -= tmp;
    tmp = fqmul(f3_sq, a[3]);
    tmp = fqmul(tmp, zeta_sq);
    b[1] -= tmp;
    b[2] =fqmul(a[0], f1_sq);
    tmp = fqmul(f0_sq, a[2]);
    b[2] -= tmp;
    tmp = fqmul(a[0], f3_sq);
    tmp = fqmul(tmp, zeta);
    b[2] += tmp;
    tmp = 2*a[1];
    tmp = fqmul(tmp, a[2]);
    tmp = fqmul(tmp, a[3]);
    tmp = fqmul(tmp, zeta);
    b[2] -= tmp;

    tmp = fqmul(f2_sq,a[2]);
    tmp = fqmul(tmp, zeta);
    b[2] += tmp;
    b[3] = 2*a[0];
    b[3] = fqmul(b[3],a[1]);
    b[3] = fqmul(b[3],a[2]);
     

    tmp = fqmul(f0_sq,a[3]);
    b[3] -= tmp;
    tmp = fqmul(f1_sq,a[1]);
    b[3] -= tmp;
    tmp = fqmul(f3_sq,a[1]);
    tmp = fqmul(tmp, zeta);
    b[3] += tmp;
    tmp = fqmul(f2_sq,a[3]);
    tmp = fqmul(tmp, zeta);
    b[3] -= tmp;
    b[0] = fqmul(b[0], det);
    b[1] = fqmul(b[1], det);
    b[2] = fqmul(b[2], det);
    b[3] = fqmul(b[3], det);
    r = (uint16_t)det;
    r = (uint32_t)(-r) >> 31;
    return r - 1;
}

#define CAL_S(a, i, j) fqmul(a[i], a[j])

int rq_inverse_recursive(int16_t b[4], const int16_t a[4], int16_t zeta)
{
    int16_t a0[2], a1[2], d[4], det;
    int r;

    a0[0] = CAL_S(a, 0, 0) + fqmul(CAL_S(a, 2, 2) - 2 * CAL_S(a, 1, 3), zeta);
    a0[1] = -CAL_S(a, 1, 1) - fqmul(CAL_S(a, 3, 3), zeta) + 2 * CAL_S(a, 0, 2);

    det = CAL_S(a0, 1, 1);
    det = fqmul(det, -zeta);
    det += CAL_S(a0, 0, 0); // with mont^-2
    det = barrett_reduce(det);
    det = fqinv(det); // det^-1 with mont^2
    
    a1[0] = fqmul(a0[0], det);
    a1[1] = fqmul(-a0[1], det);

    for (int i = 0; i < 4; i++)
        d[i] = fqmul(a1[i >> 1], a[i]);

    b[0] = d[0] + fqmul(d[2], zeta);
    b[1] = -d[1] + fqmul(-d[3], zeta);
    b[2] = fqmul(a1[0] + a1[1], a[0] + a[2]) - d[0] - d[2];
    b[3] = fqmul(a1[0] + a1[1], -a[1] - a[3]) + d[1] + d[3];

    r = (uint16_t)det;
    r = (uint32_t)(-r) >> 31;

    return r - 1;
}

int baseinv_opt(int16_t r[4], const int16_t a[4], int16_t zeta)
{
	int16_t t0, t1, t2, t3;
	
	t0 = montgomery_reduce((int32_t)a[2]*a[2] - 2*(int32_t)a[1]*a[3]);                     // R^-1
	t1 = montgomery_reduce((int32_t)a[3]*a[3]);                                            // R^-1
	t0 = montgomery_reduce((int32_t)a[0]*a[0] + (int32_t)t0*zeta);                         // R^-1
	t1 = montgomery_reduce((int32_t)a[1]*a[1] + (int32_t)t1*zeta - 2*(int32_t)a[0]*a[2]);  // R^-1
	t2 = montgomery_reduce((int32_t)t1*zeta);                                              // R^-1
	
	t3 = montgomery_reduce((int32_t)t0*t0 - (int32_t)t1*t2);  // R^-3

	if (t3 == 0) return 1;

	r[0] = montgomery_reduce((int32_t)a[0]*t0 + (int32_t)a[2]*t2); // R^-2
	r[1] = montgomery_reduce((int32_t)a[3]*t2 + (int32_t)a[1]*t0); // R^-2
	r[2] = montgomery_reduce((int32_t)a[2]*t0 + (int32_t)a[0]*t1); // R^-2
	r[3] = montgomery_reduce((int32_t)a[1]*t1 + (int32_t)a[3]*t0); // R^-2

	t3 = fqinv(t3); // R^3

	r[0] =  montgomery_reduce((int32_t)r[0]*t3); // R^0
	r[1] = -montgomery_reduce((int32_t)r[1]*t3); // R^0
	r[2] =  montgomery_reduce((int32_t)r[2]*t3); // R^0
	r[3] = -montgomery_reduce((int32_t)r[3]*t3); // R^0

	return 0;
}
