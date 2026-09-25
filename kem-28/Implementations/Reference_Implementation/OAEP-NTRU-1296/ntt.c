#include "params.h"
#include "reduce.h"
#include "ntt.h"
#include <assert.h>

const int16_t zetas[] = {
    -4452,
    457, 1997, 3730, 3618,
	-1997, 7864, -3311, -1893, 8427, -5990, -5827, -8427, 921, -212, -8342, 5827,

	6007, -6405, 6384, 7182, 8552, -5357, 1151, -342, 8510, -7588, -2919, 4541, -6, -5471, -1673, -3549, -8689, 7515, 178, -8510, -169, -6680, 6405, -7541, -1875, 8650, -6651, 8691, 5471, 3738, 3134, 3740, -4541, -178, 999, 888, -4516, 6603, -3957, 1660, -2867, -4360, 8338, 1857, 8363, -4245, -8021, 7421, -8263, -2588, 6010, 4075, 2688, -3422, -6529, -3069, -6089, -2058, -2602, -2993, -7196, -6523, -4460, 2266, 8672, 7540, -162, 8563, 4002, -5837, -3876, -5539, -8208, -5586, -2387, 1180, 7296, -7483, -7433, -7707, -5006, 3651, 4257, 4264, -1380, -7283, -1912, -330, -6014, -3072, 144, 6089, -1910, -3443, -1857, 2602, -8202, -3402, 4003, 6479, -2728, -6175, 5116, 2315, 3422, -2151, 4797, 5228, -134, -2688, -4264, -6010, -1825, -4663, -777, -7716, 5586, 5088, 6683, 8235, 5554, 4607, 330, 4192, 4369, 3957, -8245, -2275, -8563, 4272, -1977, 2387, 4525, 1912, -98, -144, 4853, -5946, 6160, -2227, 2993, 5006, -2925, 3336, -4905, -1275, -968, -2243, 6184, 3484, 2700, -1487, 2343, 856, 4209, -7385, -3176, -479, -3767, 3288, -8219, 2831, 5388, 940, 7438, 8378, 8166, 2371, 6960, -904, 2388, 3292, 2091, -1212, -879, 4314, -4428, 8742, -6303, 5260, 1043, -7955, 962, 8580, 3109, 5503, -8612, -4406, 7613, 5478, -2400, 7441, -5041, 2705, 5210, 7915, 5882, 4699, -6916, -5127, 1089, 4038, -4829, 5711, 6957, -963, -3860, -4823, -3699, 2726, -6425, -2685, 5372, 2687, -2548, 6121, 3573, 5051, -7691, -4755, 7830, 3936, -3894, -5045, 1017, 6062, -4199, 4821, -8477, 5063, -3542, -8605, 3517, 4413, 7930, 3047, -694, 3741, -4394, -5735, 1341, -8440, -3869, 5188, -3966, 2270, -6236, 2923, 8573, -6001, 4789, 2044, -6833, -5068, -2561, -2507, 1192, -7794, -8511, -3614, -1495, 5109, -3599, -2307, -5906, 156, -1446, 1290, -6201, -3761, 7535, 1547, -5591, 4044, -3276, -4628, -7904, -8504, 762, 7742, 1399, 6911, 5512, 1255, 7883, 8359, 4547, -6979, -5971, 569, -8639, 8070, 2912, 8002, -6583, -5615, 5155, 6727, -1733, -8661, 6928, 1291, -4564, -3273, -5548, 6449, 5500, 8016, 1150, -1010, 2780, -3930, -7006, 3496, -4772, 429, -2047, 5201, -1449, -5523, 3595, 7493, 7285, -3690, -1970, -662, 263, -7985, -1190, 927, 7323, 7117, -3278, -1534, -8380, 4812, -2000, -69, -6080, 6832, 3560, -6763, 2520, -120, -4488, 6376, 7502, 5507, 6496, -4955, -7150, 6653, -6634, -5889, 3713, 4565, -4239, 4770, 3428, -7993, -8488, -5049, 135, -4683, 7173, 366, -7308, -1990, 4438, 987, -5079, -3451, -3089, -4892, 992, -6805, 7948, -5800, 6956, -5668, -8479, -3238, -5621, -5241, -47, -3380, -3933, -7044, 4490, -3664, 557, 2517, 348, -2506, -2740, 2158, -223, 3335, -2250, -8062, 2929, 6100, -679, -4005, -7686, -2835, 1047, -6639, 6840, 2483, 5119, 5713, -6796, 1677, -3230, 1103, -4240, 8448, 8008, -7946, -3768, 2951, 1886, -5939, 1843, -4053, -1108, 4476, -6430, 3365, -3372, -7695, -7841, 2679, -1288, -6613, -8130, 7901, -5451, 1116, 3245, -193, 3280, 6525, 923, 3493, -2339, 1293, 2866, 527, -4786, -824, 677, -7188, 4122, -6511, 4946, -2438, 6802, 8357, -8104, 5666, 1555, 4612, 8017, 3709, 2240, 2372, 5771, -157, -1127, 775, -6009, 6166, -352, -3722, -7772, -2356, -2870, 852, 7369, -3505, -8608, 5430, -6882, -1726, -8562, -721, -5969, -2459, -7981, -3510, 8702, 2241, 7171, -751, -7091, 7922, -8165, -8295, 4713, 3316, 1960, -6673, 5886, -4546, 8449, -4832, -1252, -3617, 5798, 3297, -6170, -3710, 1222, -7392, -7007, -7779, -3015, -395, 2724, 8174, -5739, -6117, -5684, 105, 3927, 5579, -2190, -7318, 740, 7340, 5436, -6600, 1882, -7427, 8316, -1522, 2567, -8548, -5749, 6103, -7790, 5, 187, 7603, 6098, 3368, 7014, -6316, 5258, -7813, -1756, -3059, 4573, -396, 6186, -6738, -3455, -334, -1506, 1416, -3032, 1750, 4538, -5320, -6501, -3115, 5978, -5018, 2205, -4528, -1376, -3795, 1957, 8323, 3333, 1593, -3411, 731, 6343, 4750, 2680, 8455, 1271, -5985, 3622, -3057, 2351, 5094, 1548, -8734, 2292, -7186, -2802, 2633, 491, 2257, 7425, -4890, -7916, -868, -4468, -1794, -607, 5075, -2662, 2354, -4054, -6605, -2069, -6838, 285, 4359, 8444, 6567, -6351, -2486, 1992, 3789, 5232, -1643, -1541, -6773, -2146, 5791, 3120, 8303, -2585, 6074, -3206
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

		r[i + NTRUOAEP_N/2] = barrett_reduce(a[i] + a[i + NTRUOAEP_N/2] - t1);
		r[i               ] = barrett_reduce(a[i]                       + t1);
		// r[i]-r[i+xx] = -xx +2 zeta1 xx = (2zeta1 - 1) xx
		// xx =(r[i]-r[i+NTRUOAEP_N/2]) / (2zeta1-1)
		// a[i] = (r[i] + r[i+xx] - xx)/2
	}


	for(int step = NTRUOAEP_N/6; step >= 8; step = step/3)
	{
		for(int start = 0; start < NTRUOAEP_N; start += 3*step)
		{
			zeta1 = zetas[k++];
			zeta2 = zetas[k++];

			for(int i = start; i < start + step; i++)
			{
				t1 = fqmul(zeta1, r[i +   step]);
				t2 = fqmul(zeta2, r[i + 2*step]);
				t3 = fqmul(4909, t1 - t2);
                //   4719 x 2^{16} mod Q = 4909

				/**
				 * As \omega3+\omega3^2=-1,
				 * 	r[i] + xi1\omega3^2 + xi2\omega3
				 * =r[i] + xi1(-1-\omega3) + xi2\omega3
				 * =r[i] - \omega3(xi1-xi2) - xi1
				 * =r[i] - t3 - t1
				 */

				r[i + 2*step] = barrett_reduce(r[i] - t1 - t3);
				r[i +   step] = barrett_reduce(r[i] - t2 + t3);
				r[i         ] = barrett_reduce(r[i] + t1 + t2);



			}
		}

	}


	for(int step = 4; step >= 2; step >>= 1)
	{
		for(int start = 0; start < NTRUOAEP_N; start += (step << 1))
		{
			zeta1 = zetas[k++];

			for(int i = start; i < start + step; i++)
			{
				t1 = fqmul(zeta1, r[i + step]);

				r[i + step] = barrett_reduce(r[i] - t1);
				r[i       ] = barrett_reduce(r[i] + t1);

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
	int k = 647;



	for(int i = 0; i < NTRUOAEP_N; i++)
	{
		r[i] = a[i];
	}

	for(int step = 2; step <= 4; step <<= 1)
	{
		for(int start = 0; start < NTRUOAEP_N; start += (step << 1))
		{
			zeta1 = zetas[k--];

			for(int i = start; i < start + step; i++)
			{
				t1 = r[i + step];

				r[i + step] = fqmul(zeta1,  t1 - r[i]);
				r[i       ] = barrett_reduce(r[i] + t1);
			}
		}

	}



	for(int step = 8; step <= NTRUOAEP_N/6; step = 3*step)
	{
		for(int start = 0; start < NTRUOAEP_N; start += 3*step)
		{
			zeta2 = zetas[k--];
			zeta1 = zetas[k--];

			for(int i = start; i < start + step; i++)
			{

				t1 = fqmul(4909,  r[i +   step] - r[i]);
				t2 = fqmul(zeta1, r[i + 2*step] - r[i]        + t1);
				// -2t1 - t3 -t2  + omega3(-2t2+t3-t1) = -(2+omega3)t1+(omega3-1)(omega3)(t1-t2) -(2omega3+1)t2
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
		t1 = barrett_reduce(r[i] + r[i + NTRUOAEP_N/2]);
		t2 = fqmul(-7621, r[i] - r[i + NTRUOAEP_N/2]);
		// (2*zeta6-1)^-1 = (2*4720-1)^-1 = 2686
		// mont form 2686 * 2^{16} mod Q = -7621

		r[i               ] = fqmul(-2463, (int32_t) t1 - t2);
		r[i + NTRUOAEP_N/2] = fqmul(-4926, t2);
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
	int k = 647;



	for(int i = 0; i < NTRUOAEP_N; i++)
	{
		r[i] = a[i];
	}

	for(int step = 2; step <= 4; step <<= 1)
	{
		for(int start = 0; start < NTRUOAEP_N; start += (step << 1))
		{
			zeta1 = zetas[k--];

			for(int i = start; i < start + step; i++)
			{
				t1 = r[i + step];

				r[i + step] = fqmul(zeta1,  t1 - r[i]);
				r[i       ] = barrett_reduce(r[i] + t1);
			}
		}

	}



	for(int step = 8; step <= NTRUOAEP_N/6; step = 3*step)
	{
		for(int start = 0; start < NTRUOAEP_N; start += 3*step)
		{
			zeta2 = zetas[k--];
			zeta1 = zetas[k--];

			for(int i = start; i < start + step; i++)
			{

				t1 = fqmul(4909,  r[i +   step] - r[i]);
				t2 = fqmul(zeta1, r[i + 2*step] - r[i]        + t1);
				// -2t1 - t3 -t2  + omega3(-2t2+t3-t1) = -(2+omega3)t1+(omega3-1)(omega3)(t1-t2) -(2omega3+1)t2
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
		t1 = barrett_reduce(r[i] + r[i + NTRUOAEP_N/2]);
		t2 = fqmul(-7621, r[i] - r[i + NTRUOAEP_N/2]);
		// (2*zeta6-1)^-1 = (2*4720-1)^-1 = 2686
		// mont form 2686 * 2^{16} mod Q = -7621

		r[i               ] = fqmul(-2275, (int32_t) t1 - t2);
		r[i + NTRUOAEP_N/2] = fqmul(-4550, t2);
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
	int16_t x1, x2, x3, x4, x5, x6, x7, x8;

	x8 = fqmul(a, a);
	x8 = fqmul(x8, x8);
	x8 = fqmul(x8, x8);
	x1 = fqmul(x8, x8);

	x8 = fqmul(x1, x1);
	x8 = fqmul(x8, x8);
	x8 = fqmul(x8, x8);
	x8 = fqmul(x8, x8);
	x8 = fqmul(x8, x8);
	x2 = fqmul(x8, x8);
	x8 = fqmul(x2, x2);

	x3 = fqmul(x8, a);
	x4 = fqmul(x3, x1);
	x5 = fqmul(x4, x2);
	x6 = fqmul(x5, x4);
	x7 = fqmul(x6, x3);
	x8 = fqmul(x7, x7);
	return fqmul(x8, x5);

}


/*************************************************
* Name:        baseinv
*
* Description: Inversion of polynomial in Zq[X]/(X^2-zeta)
*              used for inversion of element in Rq in NTT domain
*
* Arguments:   - int16_t r[3]: pointer to the output polynomial
*              - const int16_t a[3]: pointer to the input polynomial
*              - int16_t zeta: integer defining the reduction polynomial
**************************************************/
int  baseinv(int16_t r[2], const int16_t a[2], int16_t zeta)
{

	int16_t t0;

	t0 = montgomery_reduce(a[1]*zeta);
	t0 = montgomery_reduce(t0*a[1]-a[0]*a[0]);

	if(t0 == 0) return 1;

	t0 = fqinv(t0);

	r[0] = montgomery_reduce(-a[0]*t0);
	r[1] = montgomery_reduce(a[1]*t0);

	return 0;
}

/* a-> a/R montgomery form */
/*************************************************
* Name:        basemul
*
* Description: Multiplication of polynomials in Zq[X]/(X^2-zeta)
*              used for multiplication of elements in Rq in NTT domain.
*
* Arguments:   - int16_t r[3]: pointer to the output polynomial
*              - const int16_t a[3]: pointer to the first factor
*              - const int16_t b[3]: pointer to the second factor
*              - int16_t zeta: integer defining the reduction polynomial
*
* a,b,zeta are all in montgomery form, returns in montgomery form
**************************************************/
void basemul(int16_t r[2], const int16_t a[2], const int16_t b[2], int16_t zeta)
{
	int16_t t;

	/*
	 *   (a0 + a1*x)(b0 + b1*x)
	 * = (a0*b0 + zeta*a1*b1) + (a0*b1 + a1*b0)*x.
	 */
	t = montgomery_reduce((int32_t)a[1] * b[1]);

	r[0] = montgomery_reduce((int32_t)t * zeta + (int32_t)a[0] * b[0]);
	r[1] = montgomery_reduce((int32_t)a[0] * b[1] + (int32_t)a[1] * b[0]);
}


/*************************************************
* Name:        basemul_add
*
* Description: Multiplication then addition of polynomials in Zq[X]/(X^2-zeta)
*              used for multiplication of elements in Rq in NTT domain
*
* Arguments:   - int16_t c[3]: pointer to the output polynomial
*              - const int16_t a[3]: pointer to the first factor
*              - const int16_t b[3]: pointer to the second factor
*              - const int16_t c[3]: pointer to the addend
*              - int16_t zeta: integer defining the reduction polynomial
*
* a,b,c,zeta are all in montgomery form, returns in montgomery form
**************************************************/
void basemul_add(int16_t r[2], const int16_t a[2], const int16_t b[2], const int16_t c[2], int16_t zeta)
{
	int16_t t;

	/*
	 *   (a0 + a1*x)(b0 + b1*x)
	 * = (a0*b0 + zeta*a1*b1) + (a0*b1 + a1*b0)*x.
	 */
	t = montgomery_reduce((int32_t)a[1] * b[1]);

	r[0] = montgomery_reduce(13045*c[0] + (int32_t)t * zeta + (int32_t)a[0] * b[0]);
	r[1] = montgomery_reduce(13045*c[1] + (int32_t)a[0] * b[1] + (int32_t)a[1] * b[0]);
}
