#include "params.h"
#include "reduce.h"
#include "ntt.h"
#include <assert.h>
#include <stdint.h>

const int16_t zetas[] = {
    8510,
    7214, 102, 3660, -784, -102, 4724, 11193, -2458, 2861, -9427, -7057, 8798, -12743, 9475, 9427, 7057, 2458, -8847, 10494, 8352, 4183, -3042, -5875, -6664, 6887, -4769, -7436, 13204, 1818, 9255, 3917, -14047, -2656, -7695, -13966, 7436, -8352, 4654, -13343, 8097, 4578, -3202, 13910, -10494, 10756, 12, 11298, -9703, 6664, -13119, -13204, 7744, -867, -12711, 1414, 14125, -8073, -9956, 10484, -2871, -10057, -12928, -6995, -1810, 8805, -13883, -12794, 1089, -2855, -8402, 11257, 7569, 593, 8162, 8912, 9419, -10182, -10915, -8103, -9495, -11907, 3339, 8568, -2627, 6203, -8830, -7203, -5188, -2015, 2366, 13065, 13082, 6496, 12382, 5886, -6881, 13546, -6665, -1747, 717, -2464, -2306, 13987, 12220, -5985, 7826, 13811, -2579, -7037, 2499, -3817, 1238, 4538, -5665, 2312, 8904, 901, -3213, -3239, 863, 5174, 11448, 9305, 4131, -12311, 3833, -12438, 2909, 1912, -924, 14163, -1615, -12553, 1188, -10063, -8448, -11365, -13578, -1681, 10223, 7993, 3355, 9674, 6241, 6327, 12034, -3195, -3132, 5793, -8112, 11399, -8193, -8845, 8269, -12208, -4921, 1686, 2485, 12817, -2436, -14010, 11926, -2904, 1642, -3900, 10284, 996, -14182, -941, 6866, 9149, 7807, 5033, 12901, -11763, -6471, 1891, -10279, -11010, -6985, -10325, 10060, -10523, -11468, -7665, 6598, 9855, -13275, -13054, -8861, 5383, 4848, -7656, 6921, 13047, 11769, -7810, 7923, -7854, 10189, 8329, -10470, 12261, -14253, 10098, -3193, -13731, -6905, -529, -3342, 12900, 14252, 3310, 32, -1361, 9871, 6978, 4489, -2431, -9905, 7314, -12526, 7038, 9009, -11604, 14119, -9745, -9181, 8227, -4995, 4077, 10872, 13320, -5474, -2930, 479, 7007, 1691, 5093, -5447, 11583, -10360, 8456, 8618, 13045, 5021, 2375, -12735, -13477, -3171, -3885, -7677, -1286, -12518, -13826, 1148, 4636, 8041, -6075, -12313, -2434, -6646, 6443, 12600, -1275, -2001, -1325, -5087, -3400, -5336, 5971, 9942, 4061, -4725, -3086, -7948, -13703, -9800, -10496, 11884, -10028, -2186, 1476, 3936, 3675, 11276, -12682, 8289, 4762, -13894, 13190, -2844, 10471, -6310, -6409, 7584, 8914, 2182, -1918, 1471, 7828, 3765, -13110, 6447, 10040, 1866, 13427, -11321, -11244, -4976, -2212, 2466, -10952, -759, -2469, 2024, -6584, -8812, -6576, -10977, -4490, 4107, -8053, 11567, -8129, 2857, -13697, -1492, -11390, -7172, 12173, -9621, 5556, -7644, 13483, -3559, -11690, 5183, -7483, 9518, -6844, 4317, -946, 12027, 11512, 758, -12636, -13262, 9828, -13223, -150, -2652, -2305, 2756, 400, 10571, -2155, 12862, -7072, 10967, -3995, 13656, 10838, -9116, 7903, -1149, 8772, 3064, -5121, 13708, 2066, -7411, -1117, 11698, -3229, -754, -12483, 10398, -12186, -11515, -4775, -785, 3983, -5788, -6882, 13283, 6266, -7205, 2596, 10161, -3574, 9478, -1417, 9709, -12086, -4441, -9784, 3497, -12404, -7082, -7166, 4940, 179, 3669, 9027, -9381, 9605, -828, -10267, -6197, -2494, -8370, 2208, -7021, 12358, 13946, -286, -6193, 5888, -644, -4687, -8276, -7988, 7787, 6510, 11797, 12565, 11153, -11261, -14015, 12450, -342, -5057, 3019, -9708, -11361, -11842, -13352, 9511, -14254, 5007, 6432, -11569, 8796, 7000, 10616, -2432, -3981, 912, -2625, 10958, -6354, -2412, -12570, 1783, 5492, 12343, -1709, -10180, -4984, 9000, -9093, -2644, -10439, -4205, 13192, -12197, 13265, 6974, 1869, -3375, 4265, -11958, 4513, 5718, -8138, -4947, 13906, 5141, -7228, 2663, -13949, 11425, -14006, 5830, -5610, -1586, 6408, -8795, 11546, 11691, 4942, -1876, 7723, -9232, 13553, 5275, 668, 3462, -820, 11458, 266, -2403, -11839, -5441, 3910, -10127, -8858, 11419, 13678, -992, 13381, -8652, -14023, -5662, -10450, 718, 372, -1999, -8582, -11012, 5005, 12562, 14117, -11442, -6859, -11047, -7346, -11266, 1151, 7075, 6535, -831, 10927, 9422, -13723, -3595, -13143, 11004, 11353, -9883, 911, -11124, -6435, -142, -10085, -11034, -1582, -2216, 10130, 12892, -605, -14148, 11671, 5424, 11733, 9843, 2902, 10229, -13444, -2034, 3791, -8951, -127, 7964, 7400, -6040, -12114, 14049, -7891, 9215, -10740, 11270, -2265, 2775, -10887, 13664, 13195, -4605, -5246, 3203, -6848, -6124, -13738, 5124, -1384, 5291, -11960, -2568, 5161, -2363, 2678, -8757, 4485, -963, -12335, -12280, -1580, 519, 11548, 9994, 10945, -5851, -12175, 5475, -658, -11947, 6588, 1370, -10876, 9926, -13255, 1511, 916, -7375, 12850, 11259, 13458, -13913, -3406, -10178, 11786, -7642, -2221, -11596, 5519, -7517, 7507, 2966, 846, -5006, 12762, -6383, -9908, 4397, -6811, 5251, -6016, 749, -13086, -11914, 10541, -5213, 1010, -1595, -2256, -3845, 2261, -6169, 6970, -11854, -5678, -914, -2068, 13108, -4412, 8379, 9742, 2683, -4999, -7471, -9341, -13481, -3475, -2558, -422, -12602, -7067, 5637, 13494, 12567, -13395, -3227, 560, -9561, -11405, -3021, -215, 2907, 4697, -7841, -10773, -2474, 7107, -210, 1459, -2354, -7752, -8931, 7604, 8056, -3017, 8011, 7207, 899, 12147, 720, -929, 4149, -12409, 6517, 8423, 4409, -9039, 13851, 6039, -6008, 270, -991, 5120, 10344, 2253, 12957, -14082, 1630, 7027, 11064, 1920, -3879, 11920, -7995, -11663, 12212, -3295, -12723, 6513, -9015, 566, 4470, -13447, -9677, 8250, 14073, -1207, -11928, -5452, -6916, 7193, 12778, 11145, -4473, -10222, 5415, -5874, 5686, -13177, 10668, 4801, -10546, 11224, -222, 11083, 8892, 4209, 7045, 4996, -9331, -5751, -10256, -12849, -3846, 12383, -65, 10922, -592, -6206, -9114, 836, 3479, -13716, -4722, 2211, 8432, 2734, 10246, 9113, 8899, 11997, -13943, 3162, -2735, 6103, 3286, 227, -7275, 8063, 12592, 11718, 8314, -12981, -5896, 9337, 10087, -8585, -699, 10197, 1541, -12783, -2821, -7388, -4142, -4622, -13027, -3302, 7473, -14038, 7191, -5575, 11486, 1321, -5395, -13389, 1864, 5890, -7890, -9672, -5052, -13430, 9845, 3152, 5750, -6071, 4647, -12969, 12435, 4972, -1182, 7256, -2092, 3627, 12362, 6685, 12392, -5829, -1099, -11764, -2204, -13472, 2721, 12619, -590, -6763, 6964, 3768, 3692, 1754, 5593, -7786, 8595, 12872, -1413, 11645, -1028, -6907, 1168, 438, -974, 7931, 13871, -341, -10048, -4094, 4827
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


	for(int step = NTRUOAEP_N/6; step >= 48; step = step/3)
	{
		for(int start = 0; start < NTRUOAEP_N; start += 3*step)
		{
			zeta1 = zetas[k++];
			zeta2 = zetas[k++];

			for(int i = start; i < start + step; i++)
			{
				t1 = fqmul(zeta1, r[i +   step]);
				t2 = fqmul(zeta2, r[i + 2*step]);
				t3 = montgomery_reduce(-1296 * (t1 - t2));
                //   12370 x 2^{16} mod Q = -1296

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


	for(int step = 24; step >= 3; step >>= 1)
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
	int k = 863;

	for(int i = 0; i < NTRUOAEP_N; i++)
	{
		r[i] = a[i];
	}

	for(int step = 3; step <= 24; step <<= 1)
	{
		for(int start = 0; start < NTRUOAEP_N; start += (step << 1))
		{
			zeta1 = zetas[k--];

			for(int i = start; i < start + step; i++)
			{
				t1 = r[i + step];

				r[i + step] = montgomery_reduce((int32_t)zeta1 * ((int32_t)t1 - (int32_t)r[i]));
				r[i       ] = barrett_reduce(r[i] + t1);
			}
		}

	}



	for(int step = 48; step <= NTRUOAEP_N/6; step = 3*step)
	{
		for(int start = 0; start < NTRUOAEP_N; start += 3*step)
		{
			zeta2 = zetas[k--];
			zeta1 = zetas[k--];

			for(int i = start; i < start + step; i++)
			{

				t1 = montgomery_reduce((int32_t)(-1296) * ((int32_t)r[i + step] - (int32_t)r[i]));
				t2 = montgomery_reduce((int32_t)zeta1 * ((int32_t)r[i + 2*step] - (int32_t)r[i] + (int32_t)t1));
				// -2t1 - t3 -t2  + omega3(-2t2+t3-t1) = -(2+omega3)t1+(omega3-1)(omega3)(t1-t2) -(2omega3+1)t2
    		     // = -(2+omega3)t1 - (1+2omega3)t1 + (1+2omega3)t2 -(2omega3+1)t2 = (-3-3omega3)t1 = 3(omega3^2)\xi_3 r[i+step]
    			 // \xi'_3 = \xi_3^-1 \omega3^-2 = \xi_3^-1 \omega3

				t3 = montgomery_reduce((int32_t)zeta2 * ((int32_t)r[i + 2*step] - (int32_t)r[i + step] - (int32_t)t1));

				r[i         ] = barrett_reduce(r[i] + r[i + step] + r[i + 2*step]);
				r[i +   step] = t2;
				r[i + 2*step] = t3;
			}
		}

	}


	for(int i = 0; i < NTRUOAEP_N/2; i++)
	{
		t1 = barrett_reduce(r[i] + r[i + NTRUOAEP_N/2]);
		t2 = montgomery_reduce((int32_t)(-11477) * ((int32_t)r[i] - (int32_t)r[i + NTRUOAEP_N/2]));
		// (2*zeta6-1)^-1 = (2*12371-1)^-1 = -8247
		// mont form -8247 * 2^{16} mod Q = -11477

		r[i               ] = fqmul(10821, (int32_t) t1 - t2);
		r[i + NTRUOAEP_N/2] = fqmul(-6871, t2);
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
	int k = 863;

	for(int i = 0; i < NTRUOAEP_N; i++)
	{
		r[i] = a[i];
	}

	for(int step = 3; step <= 24; step <<= 1)
	{
		for(int start = 0; start < NTRUOAEP_N; start += (step << 1))
		{
			zeta1 = zetas[k--];

			for(int i = start; i < start + step; i++)
			{
				t1 = r[i + step];

				r[i + step] = montgomery_reduce((int32_t)zeta1 * ((int32_t)t1 - (int32_t)r[i]));
				r[i       ] = barrett_reduce(r[i] + t1);
			}
		}

	}



	for(int step = 48; step <= NTRUOAEP_N/6; step = 3*step)
	{
		for(int start = 0; start < NTRUOAEP_N; start += 3*step)
		{
			zeta2 = zetas[k--];
			zeta1 = zetas[k--];

			for(int i = start; i < start + step; i++)
			{

				t1 = montgomery_reduce((int32_t)(-1296) * ((int32_t)r[i + step] - (int32_t)r[i]));
				t2 = montgomery_reduce((int32_t)zeta1 * ((int32_t)r[i + 2*step] - (int32_t)r[i] + (int32_t)t1));
				// -2t1 - t3 -t2  + omega3(-2t2+t3-t1) = -(2+omega3)t1+(omega3-1)(omega3)(t1-t2) -(2omega3+1)t2
       		     // = -(2+omega3)t1 - (1+2omega3)t1 + (1+2omega3)t2 -(2omega3+1)t2 = (-3-3omega3)t1 = 3(omega3^2)\xi_3 r[i+step]
       			 // \xi'_3 = \xi_3^-1 \omega3^-2 = \xi_3^-1 \omega3

				t3 = montgomery_reduce((int32_t)zeta2 * ((int32_t)r[i + 2*step] - (int32_t)r[i + step] - (int32_t)t1));

				r[i         ] = barrett_reduce(r[i] + r[i + step] + r[i + 2*step]);
				r[i +   step] = t2;
				r[i + 2*step] = t3;
			}
		}

	}


	for(int i = 0; i < NTRUOAEP_N/2; i++)
	{
		t1 = barrett_reduce(r[i] + r[i + NTRUOAEP_N/2]);
		t2 = montgomery_reduce((int32_t)(-11477) * ((int32_t)r[i] - (int32_t)r[i + NTRUOAEP_N/2]));
		// (2*zeta6-1)^-1 = (2*4720-1)^-1 = 2686
		// mont form 2686 * 2^{16} mod Q = -7621

		r[i               ] = fqmul(4300, (int32_t) t1 - t2);
		r[i + NTRUOAEP_N/2] = fqmul(8600, t2);
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
	int16_t x9, x10, x11, x12, x13, x14, x15;

	x15 = fqmul(a, a);
	x15 = fqmul(x15, x15);
	x1 = fqmul(x15, x15);
	x15 = fqmul(x1, x1);
	x15 = fqmul(x15, x15);
	x2 = fqmul(x15, x15);
	x15 = fqmul(x2, x2);
	x3 = fqmul(x15, x15);
	x4 = fqmul(x3, x3);

	x5 = fqmul(x4, x3);
	x6 = fqmul(x5, x1);
	x7 = fqmul(x6, x2);
	x8 = fqmul(x7, x6);
	x9 = fqmul(x8, x8);
	x10 = fqmul(x9, x7);
	x11 = fqmul(x10, a);
	x12 = fqmul(x11, x11);
	x13 = fqmul(x12, x12);
	x14 = fqmul(x13, x12);
	return fqmul(x14, x11);
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


	int16_t s0 = montgomery_reduce(a[1]*a[1]);
	int16_t s1 = montgomery_reduce(a[0]*a[2]);
	t0 = barrett_reduce(s0 - fqmul(s1, -2983));

	t4 = montgomery_reduce(a[2]*zeta);
	t1 = montgomery_reduce(t4*t4);
	t2 = montgomery_reduce(t0*a[1]);

	t3 = montgomery_reduce(a[0]*a[0]);

	int32_t t5;
	t5  = (int32_t)montgomery_reduce(t1*a[2]);
	t5 += (int32_t)montgomery_reduce(t3*a[0]);
	t5 += (int32_t)montgomery_reduce(t2*zeta);
	t3 = barrett_reduce(t5);


	if(t3 == 0) return 1;

	t3 = fqinv(t3);
	t0 = barrett_reduce(s0-s1);


	t2 = fqmul(t4,t3);
	t1 = fqmul(a[0],t3);

	r[0] = barrett_reduce(montgomery_reduce(a[0]*t1)-montgomery_reduce(a[1]*t2));
	r[1] = barrett_reduce(montgomery_reduce(a[2]*t2)-montgomery_reduce(a[1]*t1));
	r[2] = fqmul(t0,t3);

	return 0;
}

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
	r[0] = barrett_reduce(montgomery_reduce(a[1] * b[2]) + montgomery_reduce(a[2] * b[1]));
	r[1] = montgomery_reduce(a[2] * b[2]);

	r[0] = barrett_reduce(montgomery_reduce(r[0]*zeta) + montgomery_reduce(a[0]*b[0]));
	r[1] = barrett_reduce(montgomery_reduce(r[1]*zeta)+montgomery_reduce(a[0]*b[1])+montgomery_reduce(a[1]*b[0]));
	r[2] = barrett_reduce(montgomery_reduce(a[2]*b[0]) + montgomery_reduce(a[1]*b[1]) + montgomery_reduce(a[0]*b[2]));
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
*              - const int16_t c[3]: pointer to the addend
*              - int16_t zeta: integer defining the reduction polynomial
*
* a,b,c,zeta are all in montgomery form, returns in montgomery form
**************************************************/
void basemul_add(int16_t r[3], const int16_t a[3], const int16_t b[3], const int16_t c[3], int16_t zeta)
{
	r[0] = barrett_reduce(montgomery_reduce(a[1] * b[2]) + montgomery_reduce(a[2] * b[1]));
	r[1] = montgomery_reduce(a[2] * b[2]);

	r[0] = barrett_reduce(montgomery_reduce(c[0]*8510) + montgomery_reduce(r[0]*zeta) + montgomery_reduce(a[0]*b[0]));
	r[1] = barrett_reduce(montgomery_reduce(c[1]*8510) + montgomery_reduce(r[1]*zeta) + montgomery_reduce(a[0]*b[1]) + montgomery_reduce(a[1]*b[0]));
	r[2] = barrett_reduce(montgomery_reduce(c[2]*8510) + montgomery_reduce(a[2]*b[0]) + montgomery_reduce(a[1]*b[1]) + montgomery_reduce(a[0]*b[2]));
}
