#include "params.h"
#include "reduce.h"
#include "ntt.h"
#include "assert.h"
#include "stdio.h"
const int16_t zetas[] = {
	 1375, 
	  661,-3144, -246, -953, 
	
	 3144, 2794, 3168, 3240, 376, 3104, 2709, -376, 3504, -237, 1280, -2709, 

	  728, 3454,  749, 2861,  -782,   806, -1000, -1363, -2076, -2806, -1579, -2764,
	-2475, 2489, -674, 1556, -2304, -2513, -1297, -2032, -2861, -2711, -1001,  1579, 
	 2311, 1000, -1217,  45,  -517, -1602,  2639,   604,  1800, -2967,   -32,  2304, 
	 
	   81,-1149,  3321,-2172,   740,  1770, -2510, -1312,  -330,   982, -2456, -2599, 
	-2074, 3237, -3022,  870,  -966,  2699,  1733,  -190, -2243, -2053, -1264,  2638, 
	 3227,  371,   365,    6,  -240,   582,   342,  -471, -2957, -3428,   657,  1415, 
	  758, 1332, -3186,-2611,   490,  -594, -1084, -1058, -3494, -2577,  2975, -2588, 
	 1566,  313,   845,-1158,  1970, -2995, 
	  
	  1025, 2342,  2038,  2268,  409,  2519, 2447,  -163,   747, -3154,  896, -2991, 
	  149, -1027, -3307,  2752,  497,  -530, 1070, -3221, -2602, -2273, -926,  1635, 
	  1676, 1174,  -218, -1380, 2248, -1074, 1598, -1688, -1569,  1872, -794,   775, 
	  -184, -1651, 1181,  -634, 1818, -3469,  547, -1108, -3547,  3094,  866,  -488, 
	  1974, 2374,  623,   1066,  538,  1689, -1836, -139,  1468,  2515, -1379, 2847, 
	  2654,  671, -932,   3143, 2041,  2973,  2472, 1040,  -351, -3530, -1482, 2522, 
	  3248, -1232, 1010,   343, 1096,  -667,   136, 3345, -1990,  2511,   311, 3473, 
	   521,  1020, 1438,  2111,  446,  1091,  992, -1667, -3091, -3259, -415, -2082, 
	   168, 2574, -2874,   253, -3390,-2321, -516, -1769, -1809,  -509,  452, -1260, 1357
};

/*************************************************
* Name:        fqmul
*
* Description: Multiplication followed by Montgomery reduction
*
* Arguments:   - int16_t a: first factor
*              - int16_t b: second factor
*
* Returns 16-bit integer congruent to a*b*R^{-1} mod q
**************************************************/
static int16_t fqmul(int16_t a, int16_t b)
{
	return montgomery_reduce((int32_t)a*b);
}


/*************************************************
* Name:        ntt
*
* Description: number-theoretic transform (NTT) in Rq.  
*
* Arguments:   - int16_t r[NTRUOAEP_N]: pointer to output vector of elements of Zq
*              - int16_t a[NTRUOAEP_N]: pointer to input vector of elements of Zq
**************************************************/
void ntt(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N])
{
	int16_t t1,t2,t3;
	int16_t zeta1,zeta2;
	
	int k = 1;

	/* = \omega_6 = \omega_{648}^{108}=31^{108}=1250, where \omega_n denotes an n-th unit root*/
	zeta1 = zetas[k++]; 

	for(int i = 0; i < NTRUOAEP_N/2; i++)
	{
		t1 = fqmul(zeta1, a[i + NTRUOAEP_N/2]);

		r[i + NTRUOAEP_N/2] = a[i] + a[i + NTRUOAEP_N/2] - t1;
		r[i               ] = a[i]                       + t1;
	}


	

	for(int step = NTRUOAEP_N/6; step >= 12; step = step/3)
	{
		for(int start = 0; start < NTRUOAEP_N; start += 3*step)
		{
			zeta1 = zetas[k++];
			zeta2 = zetas[k++];

			for(int i = start; i < start + step; i++)
			{
				t1 = fqmul(zeta1, r[i +   step]);
				t2 = fqmul(zeta2, r[i + 2*step]);
				t3 = fqmul(-714, t1 - t2);
				// -714 = 1249(omega3)x 2^{16} mod Q


				/**
				 * As \omega3+\omega3^2=-1,
				 * 	r[i] + xi1\omega3^2 + xi2\omega3
				 * =r[i] + xi1(-1-\omega3) + xi2\omega3
				 * =r[i] - \omega3(xi1-xi2) - xi1
				 * =r[i] - t3 - t1
				 */
				if (step == 12)
				{
					r[i + 2*step] = barrett_reduce(r[i] - t1 - t3);
					r[i +   step] = barrett_reduce(r[i] - t2 + t3);  
					r[i         ] = barrett_reduce(r[i] + t1 + t2); 
				}
				else
				{
					r[i + 2*step] = r[i] - t1 - t3; 
					r[i +   step] = r[i] - t2 + t3;  
					r[i         ] = r[i] + t1 + t2; 
				}
				

			}
		}
		
	}


	for(int step = 6; step >= 3; step >>= 1)
	{
		for(int start = 0; start < NTRUOAEP_N; start += (step << 1))
		{
			zeta1 = zetas[k++];
		
			for(int i = start; i < start + step; i++)
			{
				t1 = fqmul(zeta1, r[i + step]);
				
				if (step==3)
				{
					r[i + step] = barrett_reduce(r[i] - t1);
					r[i       ] = barrett_reduce(r[i] + t1);
				}
				else
				{
					r[i + step] = r[i] - t1;
					r[i       ] = r[i] + t1;
				}
			}
		}
	}
}


/*************************************************
* Name:        invntt
*
* Description: inverse number-theoretic transform in Rq and
*              multiplication by Montgomery factor R = 2^16.
*
* Arguments:   - int16_t r[NTRUOAEP_N]: pointer to output vector of elements of Zq
*              - int16_t a[NTRUOAEP_N]: pointer to input vector of elements of Zq
**************************************************/
void invntt(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N])
{
	int16_t t1, t2, t3;
	int16_t zeta1, zeta2;
	int k = 215;

	for(int i = 0; i < NTRUOAEP_N; i++)
	{
		r[i] = a[i];
	}

	for(int step = 3; step <= 6; step <<= 1)
	{
		for(int start = 0; start < NTRUOAEP_N; start += (step << 1))
		{
			zeta1 = zetas[k--];

			for(int i = start; i < start + step; i++)
			{
				t1 = r[i + step];

				r[i + step] = fqmul(zeta1,  t1 - r[i]);
				if(step==3)
				{
					r[i       ] = r[i] + t1;
				}
				else
				{
					r[i       ] = barrett_reduce(r[i] + t1);
				}
			}
		}

	}



	for(int step = 12; step <= NTRUOAEP_N/6; step = 3*step)
	{
		for(int start = 0; start < NTRUOAEP_N; start += 3*step)
		{
			zeta2 = zetas[k--];
			zeta1 = zetas[k--];

			for(int i = start; i < start + step; i++)
			{
				t1 = fqmul(-714,  r[i +   step] - r[i]);
				t2 = fqmul(zeta1, r[i + 2*step] - r[i]        + t1);
																     // = -(2+omega3)t1 - (1+2omega3)t1 + (1+2omega3)t2 -(2omega3+1)t2 = (-3-3omega3)t1 = 3(omega3^2)\xi_3 r[i+step]
																	 // \xi'_3 = \xi_3^-1 \omega3^-2 = \xi_3^-1 \omega3

				t3 = fqmul(zeta2, r[i + 2*step] - r[i + step] - t1); 
				
				r[i         ] = barrett_reduce(r[i] + r[i + step] + r[i + 2*step]);
				r[i +   step] = t2;			
				r[i + 2*step] = t3;
			}
		}

	}

	for(int i = 0; i < NTRUOAEP_N/2; i++)
	{
		t1 = r[i] + r[i + NTRUOAEP_N/2];
		t2 = fqmul(2394, r[i] - r[i + NTRUOAEP_N/2]);

		r[i               ] = fqmul(2383, t1 - t2);
		r[i + NTRUOAEP_N/2] = fqmul(-2363, t2);
	}
}

/*************************************************
* Name:        invntt_normalized
*
* Description: inverse number-theoretic transform in Rq and
*              without multiplication by Montgomery factor R = 2^16.
*
* Arguments:   - int16_t r[NTRUOAEP_N]: pointer to output vector of elements of Zq
*              - int16_t a[NTRUOAEP_N]: pointer to input vector of elements of Zq
**************************************************/
void invntt_normalized(int16_t r[NTRUOAEP_N], const int16_t a[NTRUOAEP_N])
{
	int16_t t1, t2, t3;
	int16_t zeta1, zeta2;
	int k = 215;

	for(int i = 0; i < NTRUOAEP_N; i++)
	{
		r[i] = a[i];
	}

	for(int step = 3; step <= 6; step <<= 1)
	{
		for(int start = 0; start < NTRUOAEP_N; start += (step << 1))
		{
			zeta1 = zetas[k--];

			for(int i = start; i < start + step; i++)
			{
				t1 = r[i + step];

				r[i + step] = fqmul(zeta1,  t1 - r[i]);
				if(step==3)
				{
					r[i       ] = r[i] + t1;
				}
				else
				{
					r[i       ] = barrett_reduce(r[i] + t1);
				}
			}
		}

	}



	for(int step = 12; step <= NTRUOAEP_N/6; step = 3*step)
	{
		for(int start = 0; start < NTRUOAEP_N; start += 3*step)
		{
			zeta2 = zetas[k--];
			zeta1 = zetas[k--];

			for(int i = start; i < start + step; i++)
			{
				t1 = fqmul(-714,  r[i +   step] - r[i]);
				t2 = fqmul(zeta1, r[i + 2*step] - r[i]        + t1);
																     // = -(2+omega3)t1 - (1+2omega3)t1 + (1+2omega3)t2 -(2omega3+1)t2 = (-3-3omega3)t1 = 3(omega3^2)\xi_3 r[i+step]
																	 // \xi'_3 = \xi_3^-1 \omega3^-2 = \xi_3^-1 \omega3

				t3 = fqmul(zeta2, r[i + 2*step] - r[i + step] - t1); 

				r[i         ] = barrett_reduce(r[i] + r[i + step] + r[i + 2*step]);
				r[i +   step] = t2;			
				r[i + 2*step] = t3;
			}
		}

	}

	for(int i = 0; i < NTRUOAEP_N/2; i++)
	{
		t1 = r[i] + r[i + NTRUOAEP_N/2];
		t2 = fqmul(2394, r[i] - r[i + NTRUOAEP_N/2]);

		r[i               ] = fqmul(-2601, t1 - t2);
		r[i + NTRUOAEP_N/2] = fqmul(1927, t2);
	}
}


/*************************************************
* Name:        fqinv
*
* Description: Inversion via exponentiated by ord-1
*
* Arguments:   - int16_t a: first factor a = x mod q
*
* Returns 16-bit integer congruent to x^{-1} * R^2 mod q
**************************************************/
static int16_t fqinv(int16_t a)
{
	int x1, x2, x3, x4;

	x1 = fqmul(a, a);
	x2 = fqmul(x1, a);
	x3 = fqmul(x2, x2);
	x3 = fqmul(x3, x3);
	x3 = fqmul(x3, x3);
	x1 = fqmul(x3, x1);

	x3 = fqmul(x3, x2);
	x4 = fqmul(x3, x3);
	x4 = fqmul(x4, x4);
	x4 = fqmul(x4, x4);
	x4 = fqmul(x4, x4);
	x4 = fqmul(x4, x4);
	x4 = fqmul(x4, x1);
	
	x4 = fqmul(x4, x4);
	x4 = fqmul(x4, x4);
	x4 = fqmul(x4, x2);
	x4 = fqmul(x4, x4);
	x4 = fqmul(x4, a);

	return x4;

}


/*************************************************
* Name:        baseinv
*
* Description: Inversion of polynomial in Zq[X]/(X^3-zeta)
*              used for inversion of element in Rq in NTT domain
*
* Arguments:   - int16_t r[3]: pointer to the output polynomial
*              - const int16_t a[3]: pointer to the input polynomial
*              - int16_t zeta: integer defining the reduction polynomial
**************************************************/
int  baseinv(int16_t r[3], const int16_t a[3], int16_t zeta)
{

	int16_t t0,t1,t2,t3,t4;

	t0 = montgomery_reduce(a[1]*a[1]-3*a[0]*a[2]);
	t4 = montgomery_reduce(a[2]*zeta);
	t1 = montgomery_reduce(t4*t4);
	t2 = montgomery_reduce(t0*a[1]); 

	t3 = montgomery_reduce(a[0]*a[0]);
	t3 = montgomery_reduce(t1*a[2]+t3*a[0]+t2*zeta);

	if(t3 == 0) return 1;

	t3 = fqinv(t3); 
	t0 = montgomery_reduce(a[1]*a[1]-a[0]*a[2]);
	
	
	t2 = fqmul(t4,t3);
	t1 = fqmul(a[0],t3);

	r[0] = montgomery_reduce(-a[1]*t2+a[0]*t1);
	r[1] = montgomery_reduce(a[2]*t2-a[1]*t1);
	r[2] = fqmul(t0,t3);
	
	return 0;
}

/* a-> a/R montgomery form */
/*************************************************
* Name:        basemul
*
* Description: Multiplication of polynomials in Zq[X]/(X^3-zeta)
*              used for multiplication of elements in Rq in NTT domain.
*
* Arguments:   - int16_t r[3]: pointer to the output polynomial
*              - const int16_t a[3]: pointer to the first factor
*              - const int16_t b[3]: pointer to the second factor
*              - int16_t zeta: integer defining the reduction polynomial
*
* a,b,zeta are all in montgomery form, returns in montgomery form
**************************************************/
void basemul(int16_t r[3], const int16_t a[3], const int16_t b[3], int16_t zeta)
{
	r[0] = montgomery_reduce(a[1] * b[2] + a[2] * b[1]);
	r[1] = montgomery_reduce(a[2] * b[2]);

	r[0] = montgomery_reduce(r[0]*zeta+a[0]*b[0]);
	r[1] = montgomery_reduce(r[1]*zeta+a[0]*b[1]+a[1]*b[0]);
	r[2] = montgomery_reduce(a[2]*b[0]+a[1]*b[1]+a[0]*b[2]);
}


/*************************************************
* Name:        basemul_add
*
* Description: Multiplication then addition of polynomials in Zq[X]/(X^3-zeta)
*              used for multiplication of elements in Rq in NTT domain
*
* Arguments:   - int16_t c[3]: pointer to the output polynomial
*              - const int16_t a[3]: pointer to the first factor
*              - const int16_t b[3]: pointer to the second factor
*              - const int16_t c[3]: pointer to the third factor
*              - int16_t zeta: integer defining the reduction polynomial
*
* a,b,c,zeta are all in montgomery form, returns in montgomery form
**************************************************/
void basemul_add(int16_t r[3], const int16_t a[3], const int16_t b[3], const int16_t c[3], int16_t zeta)
{
	r[0] = montgomery_reduce(a[1] * b[2] + a[2] * b[1]);
	r[1] = montgomery_reduce(a[2] * b[2]);

	r[0] = montgomery_reduce(c[0]*1375+r[0]*zeta+a[0]*b[0]);
	r[1] = montgomery_reduce(c[1]*1375+r[1]*zeta+a[0]*b[1]+a[1]*b[0]);
	r[2] = montgomery_reduce(c[2]*1375+a[2]*b[0]+a[1]*b[1]+a[0]*b[2]);
}
