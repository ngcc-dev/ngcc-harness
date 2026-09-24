#include <stdio.h>
#include <immintrin.h>
#include "poly.h"
#include "ntt.h"
#include "symmetrics/hashkdf.h"
#include "api.h"
#include "string.h"


#if PARAM_Q == 641
int16_t NTT_Y[NTT_DIM] = { 29,-29,-21,21,268,-268,248,-248,305,-305,177,-177,122,-122,199,-199,-210,210,-290,290,-84,84,-116,116,-153,153,155,-155,67,-67,62,-62,-31,31,-287,287,244,-244,-243,243,-105,105,-145,145,-42,42,-58,58,-306,306,310,-310,134,-134,124,-124,-168,168,-232,232,61,-61,-221,221 };
#elif PARAM_Q == 1409
int16_t NTT_Y[NTT_DIM] = {-337,337,-152,152,-328,328,-311,311,299,-299,-116,116,-102,102,393,-393,-462,462,-292,292,-111,111,552,-552,389,-389,-297,297,249,-249,-172,172,-672,672,600,-600,479,-479,-478,478,-587,587,-432,432,106,-106,6,-6,563,-563,-553,553,364,-364,-325,325,-349,349,60,-60,-93,93,234,-234};
#elif PARAM_Q == 3329
int16_t NTT_Y[NTT_DIM] = {
	-1103, 1103, 430, -430, 555, -555, 843, -843,
	-1251, 1251, 871, -871, 1550, -1550, 105, -105,
	422, -422, 587, -587, 177, -177, -235, 235,
	-291, 291, -460, 460, 1574, -1574, 1653, -1653,
	-246, 246, 778, -778, 1159, -1159, -147, 147,
	-777, 777, 1483, -1483, -602, 602, 1119, -1119,
	-1590, 1590, 644, -644, -872, 872, 349, -349,
	418, -418, 329, -329, -156, 156, -75, 75,
	817, -817, 1097, -1097, 603, -603, 610, -610,
	1322, -1322, -1285, 1285, -1465, 1465, 384, -384,
	-1215, 1215, -136, 136, 1218, -1218, -1335, 1335,
	-874, 874, 220, -220, -1187, 1187, -1659, 1659,
	-1185, 1185, -1530, 1530, -1278, 1278, 794, -794,
	-1510, 1510, -854, 854, -870, 870, 478, -478,
	-108, 108, -308, 308, 996, -996, 991, -991,
	958, -958, -1460, 1460, 1522, -1522, 1628, -1628
};
#elif PARAM_Q == 769
int16_t NTT_Y[NTT_DIM] = {
	-341, 341, -379, 379, 202, -202, 220, -220,
	236, -236, 21, -21, 212, -212, 71, -71,
	-134, 134, 151, -151, 23, -23, -112, 112,
	-232, 232, 227, -227, -52, 52, -148, 148,
	244, -244, -252, 252, -237, 237, -83, 83,
	-117, 117, -333, 333, -66, 66, -247, 247,
	-292, 292, 352, -352, -145, 145, 238, -238,
	-276, 276, -194, 194, -274, 274, -70, 70,
	209, -209, -115, 115, -99, 99, 14, -14,
	29, -29, 260, -260, -378, 378, -366, 366,
	355, -355, -291, 291, 358, -358, -105, 105,
	167, -167, 357, -357, -241, 241, -331, 331,
	-348, 348, -44, 44, -78, 78, -222, 222,
	-350, 350, -168, 168, -158, 158, 201, -201,
	303, -303, 330, -330, -184, 184, 127, -127,
	318, -318, -278, 278, -353, 353, -354, 354
};
#endif

int16_t montgomery_reduce(int32_t a)
{
	int16_t t;
	t = (int16_t)a*QINV;
	t = (a - (int32_t)t*PARAM_Q) >> 16;
	return t;
}

void poly_reduce(poly *r)
{
	int i;

#if PARAM_Q == 1409
	__m256i c16x = _mm256_set1_epi16(23814);//((1 << 25) + PARAM_Q / 2) / PARAM_Q
	__m256i one16x = _mm256_set1_epi16(1 << 8);
#define SHIFT_BIT 9
#elif PARAM_Q == 641
	__m256i c16x = _mm256_set1_epi16(204);
	__m256i one16x = _mm256_set1_epi16(1);
#define SHIFT_BIT 1
#elif PARAM_Q == 3329
	__m256i c16x = _mm256_set1_epi16(20159);
	__m256i one16x = _mm256_set1_epi16(1 << 9);
#define SHIFT_BIT 10
#elif PARAM_Q == 769
	__m256i c16x = _mm256_set1_epi16(21817);
	__m256i one16x = _mm256_set1_epi16(1 << 7);
#define SHIFT_BIT 8
#endif
	__m256i q16x = _mm256_set1_epi16(PARAM_Q);
	__m256i t, d;

	for (i = 0; i < PARAM_N / 16; i++)
	{
		t = _mm256_loadu_si256((__m256i*)&r->coeffs[16 * i]);
		d = _mm256_mulhi_epi16(t, c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, SHIFT_BIT);
		d = _mm256_mullo_epi16(d, q16x);
		t = _mm256_sub_epi16(t, d);
		_mm256_storeu_si256((__m256i*)&r->coeffs[16 * i], t);
	}
}
void poly_getmontgomery(poly *r)
{
	int i;
	const __m256i vq16x = _mm256_set1_epi16((int16_t)PARAM_Q);
	const __m256i vqinv16x = _mm256_set1_epi16((int16_t)QINV);
	__m256i f, g;

	for (i = 0; i < PARAM_N; i += 16) {
		f = _mm256_loadu_si256((const __m256i *)&r->coeffs[i]);
		g = _mm256_mullo_epi16(f, vqinv16x);
		g = _mm256_mulhi_epi16(g, vq16x);
		f = _mm256_srai_epi16(f, 15);
		f = _mm256_sub_epi16(f, g);
		_mm256_storeu_si256((__m256i *)&r->coeffs[i], f);
	}
}



#define Q_HALF (PARAM_Q + 1) / 2
void poly_add_vinv(poly *f) {
	f->coeffs[0] += Q_HALF;
	f->coeffs[NTT_DIM / 2] += Q_HALF;
	f->coeffs[NTT_DIM / 4] += Q_HALF;
	f->coeffs[NTT_DIM * 3 / 4] += Q_HALF;
}

void poly_rotv(poly* r, poly* a)
{
	int i, j;
	__m256i v;
	__m256i zero = _mm256_setzero_si256();
	for (i = 0; i < PARAM_N / NTT_DIM; i++)
	{
		memcpy(&r->coeffs[i * NTT_DIM],&a->coeffs[i * NTT_DIM + NTT_DIM - SEED_BYTES * 8 * NTT_DIM / PARAM_N], SEED_BYTES * 16 * NTT_DIM / PARAM_N);
		for(j = 0; j < NTT_DIM/16 - SEED_BYTES * NTT_DIM/(2*PARAM_N); j++)
		{
			v = _mm256_load_si256((__m256i*) & a->coeffs[i * NTT_DIM + 16 * j]);
			v = _mm256_sub_epi16(zero, v);
			_mm256_store_si256((__m256i*) & r->coeffs[i * NTT_DIM + SEED_BYTES * 8 * NTT_DIM / PARAM_N + 16*j],v);

		}
	}
}

void poly_caddq(poly *r)
{
	int i;
	__m256i * pr = (__m256i *) r->coeffs;
	__m256i q16x = _mm256_set1_epi16(PARAM_Q);
	__m256i t, d;

	for (i = 0; i < PARAM_N / 16; i++)
	{
		t = _mm256_load_si256((__m256i*)&r->coeffs[16 * i]);
		d = _mm256_srai_epi16(t, 15);
		d = _mm256_and_si256(q16x, d);
		t = _mm256_add_epi16(t, d);
		_mm256_store_si256((__m256i*)&r->coeffs[16 * i], t);
	}
}

void poly_add(poly *r, const poly *a, const poly *b)
{
	int i;
	__m256i * pa = (__m256i *) a->coeffs;
	__m256i * pb = (__m256i *) b->coeffs;
	__m256i * pr = (__m256i *) r->coeffs;

	for (i = 0; i < PARAM_N / 16; i++)
		pr[i] = _mm256_add_epi16(pa[i], pb[i]);
}

void poly_sub(poly *r, const poly *a, const poly *b)
{
	int i;
	__m256i * pa = (__m256i *) a->coeffs;
	__m256i * pb = (__m256i *) b->coeffs;
	__m256i * pr = (__m256i *) r->coeffs;

	for (i = 0; i < PARAM_N / 16; i++)
		pr[i] = _mm256_sub_epi16(pa[i], pb[i]);
}


void poly_ntt(poly *r)
{
	int i = 0;
	for (i = 0; i < PARAM_N / NTT_DIM; i++)
		ntt(&r->coeffs[i*NTT_DIM]);
}
void poly_invntt(poly *r)
{
	int i = 0;
	for (i = 0; i < PARAM_N / NTT_DIM; i++)
		invntt(&r->coeffs[i*NTT_DIM]);
}


static inline void mont_mul4(int16_t* r, const int16_t* a, const int16_t* b)//multiplication for degree 3 polynomial/4 coefficients
{
	__m256i t[14];
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i dh, d;
	int i, j;
	for (i = 0; i < NTT_DIM / 16; i++)
	{
		for (j = 0; j < 4; j++)
			t[j] = _mm256_load_si256((__m256i*) & a[j * NTT_DIM + 16 * i]);//a0,a1,a2,a3
		for (j = 0; j < 4; j++)
			t[4 + j] = _mm256_load_si256((__m256i*) & b[j * NTT_DIM + 16 * i]);//b0,b1,b2,b3


		dh = _mm256_mulhi_epi16(t[0], t[4]);
		d = _mm256_mullo_epi16(t[0], t[4]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[8] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		t[9] = _mm256_add_epi16(t[0], t[1]);
		t[10] = _mm256_add_epi16(t[4], t[5]);

		dh = _mm256_mulhi_epi16(t[9], t[10]);
		d = _mm256_mullo_epi16(t[9], t[10]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[9] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		dh = _mm256_mulhi_epi16(t[1], t[5]);
		d = _mm256_mullo_epi16(t[1], t[5]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[10] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		t[9] = _mm256_sub_epi16(t[9], t[8]);
		t[9] = _mm256_sub_epi16(t[9], t[10]);


		dh = _mm256_mulhi_epi16(t[2], t[6]);
		d = _mm256_mullo_epi16(t[2], t[6]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[11] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		t[12] = _mm256_add_epi16(t[2], t[3]);
		t[13] = _mm256_add_epi16(t[6], t[7]);

		dh = _mm256_mulhi_epi16(t[12], t[13]);
		d = _mm256_mullo_epi16(t[12], t[13]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[12] = _mm256_sub_epi16(dh, d);//[-Q,Q]



		dh = _mm256_mulhi_epi16(t[3], t[7]);
		d = _mm256_mullo_epi16(t[3], t[7]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[13] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		t[12] = _mm256_sub_epi16(t[12], t[11]);
		t[12] = _mm256_sub_epi16(t[12], t[13]);

		t[0] = _mm256_add_epi16(t[0], t[2]);
		t[1] = _mm256_add_epi16(t[1], t[3]);
		t[2] = _mm256_add_epi16(t[4], t[6]);
		t[3] = _mm256_add_epi16(t[5], t[7]);


		dh = _mm256_mulhi_epi16(t[0], t[2]);
		d = _mm256_mullo_epi16(t[0], t[2]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[4] = _mm256_sub_epi16(dh, d);//[-Q,Q]




		t[5] = _mm256_add_epi16(t[0], t[1]);
		t[6] = _mm256_add_epi16(t[2], t[3]);

		dh = _mm256_mulhi_epi16(t[5], t[6]);
		d = _mm256_mullo_epi16(t[5], t[6]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[5] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		dh = _mm256_mulhi_epi16(t[1], t[3]);
		d = _mm256_mullo_epi16(t[1], t[3]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[6] = _mm256_sub_epi16(dh, d);//[-Q,Q]



		t[5] = _mm256_sub_epi16(t[5], t[4]);
		t[5] = _mm256_sub_epi16(t[5], t[6]);

		t[4] = _mm256_sub_epi16(t[4], t[8]);
		t[4] = _mm256_sub_epi16(t[4], t[11]);

		t[5] = _mm256_sub_epi16(t[5], t[9]);
		t[5] = _mm256_sub_epi16(t[5], t[12]);

		t[6] = _mm256_sub_epi16(t[6], t[10]);
		t[6] = _mm256_sub_epi16(t[6], t[13]);

		t[10] = _mm256_add_epi16(t[4], t[10]);
		t[11] = _mm256_add_epi16(t[6], t[11]);

		_mm256_store_si256((__m256i*) & r[16 * i], t[8]);
		_mm256_store_si256((__m256i*) & r[NTT_DIM + 16 * i], t[9]);
		_mm256_store_si256((__m256i*) & r[2 * NTT_DIM + 16 * i], t[10]);
		_mm256_store_si256((__m256i*) & r[3 * NTT_DIM + 16 * i], t[5]);
		_mm256_store_si256((__m256i*) & r[4 * NTT_DIM + 16 * i], t[11]);
		_mm256_store_si256((__m256i*) & r[5 * NTT_DIM + 16 * i], t[12]);
		_mm256_store_si256((__m256i*) & r[6 * NTT_DIM + 16 * i], t[13]);
	}
}
static inline void mont_double_mul4(int16_t* r, const int16_t* a, const int16_t* b)//multiplication for degree 3 polynomial/4 coefficients
{
	__m256i t[15];
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i dh, d;
	int i, j;
	for (i = 0; i < NTT_DIM / 16; i++)
	{
		for (j = 0; j < 4; j++)
		{
			t[j] = _mm256_load_si256((__m256i*) & a[j * NTT_DIM + 16 * i]);//a0,a1,a2,a3
			t[4 + j] = _mm256_load_si256((__m256i*) & b[j * NTT_DIM + 16 * i]);//b0,b1,b2,b3
			t[j] = _mm256_slli_epi16(t[j], 1);
		}


		dh = _mm256_mulhi_epi16(t[0], t[4]);
		d = _mm256_mullo_epi16(t[0], t[4]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[8] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		t[9] = _mm256_add_epi16(t[0], t[1]);
		t[10] = _mm256_add_epi16(t[4], t[5]);

		dh = _mm256_mulhi_epi16(t[9], t[10]);
		d = _mm256_mullo_epi16(t[9], t[10]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[9] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		dh = _mm256_mulhi_epi16(t[1], t[5]);
		d = _mm256_mullo_epi16(t[1], t[5]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[10] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		t[9] = _mm256_sub_epi16(t[9], t[8]);
		t[9] = _mm256_sub_epi16(t[9], t[10]);


		dh = _mm256_mulhi_epi16(t[2], t[6]);
		d = _mm256_mullo_epi16(t[2], t[6]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[11] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		t[12] = _mm256_add_epi16(t[2], t[3]);
		t[13] = _mm256_add_epi16(t[6], t[7]);

		dh = _mm256_mulhi_epi16(t[12], t[13]);
		d = _mm256_mullo_epi16(t[12], t[13]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[12] = _mm256_sub_epi16(dh, d);//[-Q,Q]



		dh = _mm256_mulhi_epi16(t[3], t[7]);
		d = _mm256_mullo_epi16(t[3], t[7]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[13] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		t[12] = _mm256_sub_epi16(t[12], t[11]);
		t[12] = _mm256_sub_epi16(t[12], t[13]);

		t[0] = _mm256_add_epi16(t[0], t[2]);
		t[1] = _mm256_add_epi16(t[1], t[3]);
		t[2] = _mm256_add_epi16(t[4], t[6]);
		t[3] = _mm256_add_epi16(t[5], t[7]);


		dh = _mm256_mulhi_epi16(t[0], t[2]);
		d = _mm256_mullo_epi16(t[0], t[2]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[4] = _mm256_sub_epi16(dh, d);//[-Q,Q]




		t[5] = _mm256_add_epi16(t[0], t[1]);
		t[6] = _mm256_add_epi16(t[2], t[3]);

		dh = _mm256_mulhi_epi16(t[5], t[6]);
		d = _mm256_mullo_epi16(t[5], t[6]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[5] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		dh = _mm256_mulhi_epi16(t[1], t[3]);
		d = _mm256_mullo_epi16(t[1], t[3]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[6] = _mm256_sub_epi16(dh, d);//[-Q,Q]



		t[5] = _mm256_sub_epi16(t[5], t[4]);
		t[5] = _mm256_sub_epi16(t[5], t[6]);

		t[4] = _mm256_sub_epi16(t[4], t[8]);
		t[4] = _mm256_sub_epi16(t[4], t[11]);

		t[5] = _mm256_sub_epi16(t[5], t[9]);
		t[5] = _mm256_sub_epi16(t[5], t[12]);

		t[6] = _mm256_sub_epi16(t[6], t[10]);
		t[6] = _mm256_sub_epi16(t[6], t[13]);

		t[10] = _mm256_add_epi16(t[4], t[10]);
		t[11] = _mm256_add_epi16(t[6], t[11]);

		_mm256_store_si256((__m256i*) & r[16 * i], t[8]);
		_mm256_store_si256((__m256i*) & r[NTT_DIM + 16 * i], t[9]);
		_mm256_store_si256((__m256i*) & r[2 * NTT_DIM + 16 * i], t[10]);
		_mm256_store_si256((__m256i*) & r[3 * NTT_DIM + 16 * i], t[5]);
		_mm256_store_si256((__m256i*) & r[4 * NTT_DIM + 16 * i], t[11]);
		_mm256_store_si256((__m256i*) & r[5 * NTT_DIM + 16 * i], t[12]);
		_mm256_store_si256((__m256i*) & r[6 * NTT_DIM + 16 * i], t[13]);
	}
}


static inline void mont_mul8(int16_t* r, const int16_t* a, const int16_t* b)
{
	ALIGN(32) int16_t t1[7 * NTT_DIM], t2[7 * NTT_DIM];
	__m256i v[6],d;

#if PARAM_Q == 1409
	__m256i c16x = _mm256_set1_epi16(23814);//((1 << 25) + PARAM_Q / 2) / PARAM_Q
	__m256i one16x = _mm256_set1_epi16(1 << 8);
	#define SHIFT_BIT 9
#elif PARAM_Q == 641
	__m256i c16x = _mm256_set1_epi16(204);
	__m256i one16x = _mm256_set1_epi16(1);
	#define SHIFT_BIT 1
#elif PARAM_Q == 3329
	__m256i c16x = _mm256_set1_epi16(20159);
	__m256i one16x = _mm256_set1_epi16(1 << 9);
	#define SHIFT_BIT 10
#elif PARAM_Q == 769
	__m256i c16x = _mm256_set1_epi16(21817);
	__m256i one16x = _mm256_set1_epi16(1 << 7);
#define SHIFT_BIT 8
#endif
	__m256i q16x = _mm256_set1_epi16(PARAM_Q);

	int i;
	for (i = 0; i < 4 * NTT_DIM / 16; i++)
	{
		v[0] = _mm256_load_si256((__m256i*) & a[16 * i]);//a0,a1,a2,a3
		v[1] = _mm256_load_si256((__m256i*) & a[4 * NTT_DIM + 16 * i]);
		v[0] = _mm256_add_epi16(v[0], v[1]);

		v[1] = _mm256_load_si256((__m256i*) & b[16 * i]);//a0,a1,a2,a3
		v[2] = _mm256_load_si256((__m256i*) & b[4 * NTT_DIM + 16 * i]);
		v[1] = _mm256_add_epi16(v[1], v[2]);

		_mm256_store_si256((__m256i*) & t1[16 * i], v[0]);
		_mm256_store_si256((__m256i*) & t2[16 * i], v[1]);
	}
	mont_mul4(t1, t1, t2);
	mont_mul4(&r[8 * NTT_DIM], &a[4 * NTT_DIM], &b[4 * NTT_DIM]);
	mont_mul4(r, a, b);

	for (i = 0; i < NTT_DIM / 16; i++)
	{
		v[0] = _mm256_load_si256((__m256i*) & t1[3 * NTT_DIM + 16 * i]);
		v[1] = _mm256_load_si256((__m256i*) & r[3 * NTT_DIM + 16 * i]);
		v[2] = _mm256_load_si256((__m256i*) & r[11 * NTT_DIM + 16 * i]);
		v[0] = _mm256_sub_epi16(v[0], v[1]);
		v[0] = _mm256_sub_epi16(v[0], v[2]);

		//lazy reduction, barrat reduction
		d = _mm256_mulhi_epi16(v[0], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, SHIFT_BIT);
		d = _mm256_mullo_epi16(d, q16x);
		v[0] = _mm256_sub_epi16(v[0], d);


		_mm256_store_si256((__m256i*) & r[7 * NTT_DIM + 16 * i], v[0]);
	}
	for (i = 0; i < 3 * NTT_DIM / 16; i++)
	{
		v[0] = _mm256_load_si256((__m256i*) & r[4 * NTT_DIM + 16 * i]);
		v[1] = _mm256_load_si256((__m256i*) & t1[16 * i]);
		v[2] = _mm256_load_si256((__m256i*) & r[16 * i]);
		v[3] = _mm256_load_si256((__m256i*) & r[8 * NTT_DIM + 16 * i]);
		v[4] = _mm256_load_si256((__m256i*) & t1[4 * NTT_DIM + 16 * i]);
		v[5] = _mm256_load_si256((__m256i*) & r[12 * NTT_DIM + 16 * i]);

		v[1] = _mm256_add_epi16(v[0], v[1]);
		v[1] = _mm256_sub_epi16(v[1], v[2]);
		v[1] = _mm256_sub_epi16(v[1], v[3]);

		//lazy reduction, barrat reduction
		d = _mm256_mulhi_epi16(v[1], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, SHIFT_BIT);
		d = _mm256_mullo_epi16(d, q16x);
		v[1] = _mm256_sub_epi16(v[1], d);


		v[3] = _mm256_add_epi16(v[3], v[4]);
		v[3] = _mm256_sub_epi16(v[3], v[0]);
		v[3] = _mm256_sub_epi16(v[3], v[5]);

		//lazy reduction, barrat reduction
		d = _mm256_mulhi_epi16(v[3], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, SHIFT_BIT);
		d = _mm256_mullo_epi16(d, q16x);
		v[3] = _mm256_sub_epi16(v[3], d);

		_mm256_store_si256((__m256i*) & r[4 * NTT_DIM + 16 * i], v[1]);
		_mm256_store_si256((__m256i*) & r[8 * NTT_DIM + 16 * i], v[3]);
	}
}
static inline void mont_double_mul8(int16_t* r, const int16_t* a, const int16_t* b)
{
	ALIGN(32) int16_t t1[7 * NTT_DIM], t2[8 * NTT_DIM], t3[4 * NTT_DIM];
	__m256i v[6],d;
#if PARAM_Q == 1409
	__m256i c16x = _mm256_set1_epi16(23814);//((1 << 25) + PARAM_Q / 2) / PARAM_Q
	__m256i one16x = _mm256_set1_epi16(1 << 8);
#define SHIFT_BIT 9
#elif PARAM_Q == 641
	__m256i c16x = _mm256_set1_epi16(204);
	__m256i one16x = _mm256_set1_epi16(1);
#define SHIFT_BIT 1
#elif PARAM_Q == 3329
	__m256i c16x = _mm256_set1_epi16(20159);
	__m256i one16x = _mm256_set1_epi16(1 << 9);
#define SHIFT_BIT 10
#elif PARAM_Q == 769
	__m256i c16x = _mm256_set1_epi16(21817);
	__m256i one16x = _mm256_set1_epi16(1 << 7);
#define SHIFT_BIT 8
#endif
	__m256i q16x = _mm256_set1_epi16(PARAM_Q);

	int i;
	for (i = 0; i < 4 * NTT_DIM / 16; i++)
	{
		v[0] = _mm256_load_si256((__m256i*) & a[16 * i]);//a0,a1,a2,a3
		v[1] = _mm256_load_si256((__m256i*) & a[4 * NTT_DIM + 16 * i]);
		v[0] = _mm256_slli_epi16(v[0], 1);
		v[1] = _mm256_slli_epi16(v[1], 1);
		v[2] = _mm256_add_epi16(v[0], v[1]);

		v[3] = _mm256_load_si256((__m256i*) & b[16 * i]);//a0,a1,a2,a3
		v[4] = _mm256_load_si256((__m256i*) & b[4 * NTT_DIM + 16 * i]);
		v[3] = _mm256_add_epi16(v[3], v[4]);

		_mm256_store_si256((__m256i*) & t2[16 * i], v[0]);
		_mm256_store_si256((__m256i*) & t2[4 * NTT_DIM + 16 * i], v[1]);

		_mm256_store_si256((__m256i*) & t1[16 * i], v[2]);
		_mm256_store_si256((__m256i*) & t3[16 * i], v[3]);
	}
	mont_mul4(t1, t1, t3);
	mont_mul4(&r[8 * NTT_DIM], &t2[4 * NTT_DIM], &b[4 * NTT_DIM]);
	mont_mul4(r, t2, b);

	for (i = 0; i < NTT_DIM / 16; i++)
	{
		v[0] = _mm256_load_si256((__m256i*) & t1[3 * NTT_DIM + 16 * i]);
		v[1] = _mm256_load_si256((__m256i*) & r[3 * NTT_DIM + 16 * i]);
		v[2] = _mm256_load_si256((__m256i*) & r[11 * NTT_DIM + 16 * i]);
		v[0] = _mm256_sub_epi16(v[0], v[1]);
		v[0] = _mm256_sub_epi16(v[0], v[2]);

		//lazy reduction, barrat reduction
		d = _mm256_mulhi_epi16(v[0], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, SHIFT_BIT);
		d = _mm256_mullo_epi16(d, q16x);
		v[0] = _mm256_sub_epi16(v[0], d);

		_mm256_store_si256((__m256i*) & r[7 * NTT_DIM + 16 * i], v[0]);
	}
	for (i = 0; i < 3 * NTT_DIM / 16; i++)
	{
		v[0] = _mm256_load_si256((__m256i*) & r[4 * NTT_DIM + 16 * i]);
		v[1] = _mm256_load_si256((__m256i*) & t1[16 * i]);
		v[2] = _mm256_load_si256((__m256i*) & r[16 * i]);
		v[3] = _mm256_load_si256((__m256i*) & r[8 * NTT_DIM + 16 * i]);
		v[4] = _mm256_load_si256((__m256i*) & t1[4 * NTT_DIM + 16 * i]);
		v[5] = _mm256_load_si256((__m256i*) & r[12 * NTT_DIM + 16 * i]);

		v[1] = _mm256_add_epi16(v[0], v[1]);
		v[1] = _mm256_sub_epi16(v[1], v[2]);
		v[1] = _mm256_sub_epi16(v[1], v[3]);

		//lazy reduction, barrat reduction
		d = _mm256_mulhi_epi16(v[1], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, SHIFT_BIT);
		d = _mm256_mullo_epi16(d, q16x);
		v[1] = _mm256_sub_epi16(v[1], d);


		v[3] = _mm256_add_epi16(v[3], v[4]);
		v[3] = _mm256_sub_epi16(v[3], v[0]);
		v[3] = _mm256_sub_epi16(v[3], v[5]);

		//lazy reduction, barrat reduction
		d = _mm256_mulhi_epi16(v[3], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, SHIFT_BIT);
		d = _mm256_mullo_epi16(d, q16x);
		v[3] = _mm256_sub_epi16(v[3], d);


		_mm256_store_si256((__m256i*) & r[4 * NTT_DIM + 16 * i], v[1]);
		_mm256_store_si256((__m256i*) & r[8 * NTT_DIM + 16 * i], v[3]);
	}
}
static inline void mont_mul16(int16_t* r, const int16_t* a, const int16_t* b)
{
	ALIGN(32) int16_t t1[15 * NTT_DIM], t2[15 * NTT_DIM];
	__m256i v[6];
	int i;
	for (i = 0; i < 8 * NTT_DIM / 16; i++)
	{
		v[0] = _mm256_load_si256((__m256i*) & a[16 * i]);//a0,a1,a2,a3
		v[1] = _mm256_load_si256((__m256i*) & a[8 * NTT_DIM + 16 * i]);
		v[0] = _mm256_add_epi16(v[0], v[1]);

		v[1] = _mm256_load_si256((__m256i*) & b[16 * i]);//a0,a1,a2,a3
		v[2] = _mm256_load_si256((__m256i*) & b[8 * NTT_DIM + 16 * i]);
		v[1] = _mm256_add_epi16(v[1], v[2]);

		_mm256_store_si256((__m256i*) & t1[16 * i], v[0]);
		_mm256_store_si256((__m256i*) & t2[16 * i], v[1]);
	}
	mont_mul8(t1, t1, t2);
	mont_mul8(&r[16 * NTT_DIM], &a[8 * NTT_DIM], &b[8 * NTT_DIM]);//t2
	mont_mul8(r, a, b);//t0

	for (i = 0; i < NTT_DIM / 16; i++)
	{
		v[0] = _mm256_load_si256((__m256i*) & t1[7 * NTT_DIM + 16 * i]);
		v[1] = _mm256_load_si256((__m256i*) & r[7 * NTT_DIM + 16 * i]);
		v[2] = _mm256_load_si256((__m256i*) & r[23 * NTT_DIM + 16 * i]);
		v[0] = _mm256_sub_epi16(v[0], v[1]);
		v[0] = _mm256_sub_epi16(v[0], v[2]);
		_mm256_store_si256((__m256i*) & r[15 * NTT_DIM + 16 * i], v[0]);
	}
	for (i = 0; i < 7 * NTT_DIM / 16; i++)
	{
		v[0] = _mm256_load_si256((__m256i*) & r[8 * NTT_DIM + 16 * i]);
		v[1] = _mm256_load_si256((__m256i*) & t1[16 * i]);
		v[2] = _mm256_load_si256((__m256i*) & r[16 * i]);
		v[3] = _mm256_load_si256((__m256i*) & r[16 * NTT_DIM + 16 * i]);
		v[4] = _mm256_load_si256((__m256i*) & t1[8 * NTT_DIM + 16 * i]);
		v[5] = _mm256_load_si256((__m256i*) & r[24 * NTT_DIM + 16 * i]);

		v[1] = _mm256_add_epi16(v[0], v[1]);
		v[1] = _mm256_sub_epi16(v[1], v[2]);
		v[1] = _mm256_sub_epi16(v[1], v[3]);

		v[3] = _mm256_add_epi16(v[3], v[4]);
		v[3] = _mm256_sub_epi16(v[3], v[0]);
		v[3] = _mm256_sub_epi16(v[3], v[5]);

		_mm256_store_si256((__m256i*) & r[8 * NTT_DIM + 16 * i], v[1]);
		_mm256_store_si256((__m256i*) & r[16 * NTT_DIM + 16 * i], v[3]);
	}
}
static inline void mont_square4(int16_t* r, const int16_t* a)//multiplication for degree 3 polynomial/4 coefficients
{

	__m256i t[12],dh,d;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	int i, j;
	for (i = 0; i < NTT_DIM / 16; i++)
	{
		for (j = 0; j < 4; j++)
			t[j] = _mm256_load_si256((__m256i*) & a[j * NTT_DIM + 16 * i]);//a0,a1,a2,a3


		dh = _mm256_mulhi_epi16(t[0], t[0]);
		d = _mm256_mullo_epi16(t[0], t[0]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[4] = _mm256_sub_epi16(dh, d);//[-Q,Q]



		dh = _mm256_mulhi_epi16(t[1], t[1]);
		d = _mm256_mullo_epi16(t[1], t[1]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[6] = _mm256_sub_epi16(dh, d);//[-Q,Q]



		t[0] = _mm256_slli_epi16(t[0], 1);

		dh = _mm256_mulhi_epi16(t[0], t[1]);
		d = _mm256_mullo_epi16(t[0], t[1]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[5] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		t[1] = _mm256_slli_epi16(t[1], 1);

		dh = _mm256_mulhi_epi16(t[0], t[2]);
		d = _mm256_mullo_epi16(t[0], t[2]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[9] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		t[0] = _mm256_add_epi16(t[0], t[1]);
		t[7] = _mm256_add_epi16(t[2], t[3]);

		dh = _mm256_mulhi_epi16(t[0], t[7]);
		d = _mm256_mullo_epi16(t[0], t[7]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[7] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		dh = _mm256_mulhi_epi16(t[1], t[3]);
		d = _mm256_mullo_epi16(t[1], t[3]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[8] = _mm256_sub_epi16(dh, d);//[-Q,Q]



		t[7] = _mm256_sub_epi16(t[7], t[8]);
		t[7] = _mm256_sub_epi16(t[7], t[9]);
		t[6] = _mm256_add_epi16(t[6], t[9]);


		dh = _mm256_mulhi_epi16(t[2], t[2]);
		d = _mm256_mullo_epi16(t[2], t[2]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[11] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		t[2] = _mm256_slli_epi16(t[2], 1);


		dh = _mm256_mulhi_epi16(t[2], t[3]);
		d = _mm256_mullo_epi16(t[2], t[3]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[9] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		dh = _mm256_mulhi_epi16(t[3], t[3]);
		d = _mm256_mullo_epi16(t[3], t[3]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[10] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		t[8] = _mm256_add_epi16(t[8], t[11]);//[-Q,Q]

		for (j = 0; j < 7; j++)
			_mm256_store_si256((__m256i*) & r[j * NTT_DIM + 16 * i], t[4 + j]);
	}
}
static inline void mont_square8(int16_t* r, const int16_t* a)//multiplication for degree 3 polynomial/4 coefficients
{
	ALIGN(32) int16_t t1[7 * NTT_DIM];
	__m256i v[4], d;

#if PARAM_Q == 1409
	__m256i c16x = _mm256_set1_epi16(23814);//((1 << 25) + PARAM_Q / 2) / PARAM_Q
	__m256i one16x = _mm256_set1_epi16(1 << 8);
#define SHIFT_BIT 9
#elif PARAM_Q == 641
	__m256i c16x = _mm256_set1_epi16(204);
	__m256i one16x = _mm256_set1_epi16(1);
#define SHIFT_BIT 1
#elif PARAM_Q == 3329
	__m256i c16x = _mm256_set1_epi16(20159);
	__m256i one16x = _mm256_set1_epi16(1 << 9);
#define SHIFT_BIT 10
#elif PARAM_Q == 769
	__m256i c16x = _mm256_set1_epi16(21817);
	__m256i one16x = _mm256_set1_epi16(1 << 7);
#define SHIFT_BIT 8
#endif
	__m256i q16x = _mm256_set1_epi16(PARAM_Q);

	int i;

	mont_double_mul4(t1, a, &a[4 * NTT_DIM]);
	mont_square4(&r[8 * NTT_DIM], &a[4 * NTT_DIM]);
	mont_square4(r, a);

	for (i = 0; i < NTT_DIM; i++)
		r[7 * NTT_DIM + i] = t1[3 * NTT_DIM + i];

	for (i = 0; i < 3 * NTT_DIM / 16; i++)
	{
		v[0] = _mm256_load_si256((__m256i*) & r[4 * NTT_DIM + 16 * i]);
		v[1] = _mm256_load_si256((__m256i*) & t1[16 * i]);
		v[2] = _mm256_load_si256((__m256i*) & t1[4 * NTT_DIM + 16 * i]);
		v[3] = _mm256_load_si256((__m256i*) & r[8 * NTT_DIM + 16 * i]);

		v[0] = _mm256_add_epi16(v[0], v[1]);
		v[1] = _mm256_add_epi16(v[2], v[3]);

		//lazy reduction, barrat reduction
		d = _mm256_mulhi_epi16(v[0], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, SHIFT_BIT);
		d = _mm256_mullo_epi16(d, q16x);
		v[0] = _mm256_sub_epi16(v[0], d);

		//lazy reduction, barrat reduction
		d = _mm256_mulhi_epi16(v[1], c16x);
		d = _mm256_add_epi16(d, one16x);
		d = _mm256_srai_epi16(d, SHIFT_BIT);
		d = _mm256_mullo_epi16(d, q16x);
		v[1] = _mm256_sub_epi16(v[1], d);

		_mm256_store_si256((__m256i*) & r[4 * NTT_DIM + 16 * i], v[0]);
		_mm256_store_si256((__m256i*) & r[8 * NTT_DIM + 16 * i], v[1]);
	}
}
static inline void mont_square16(int16_t* r, const int16_t* a)//multiplication for degree 3 polynomial/4 coefficients
{
	int16_t t1[15 * NTT_DIM];
	__m256i v[4];
	int i;
	mont_double_mul8(t1, a, &a[8 * NTT_DIM]);
	mont_square8(&r[16 * NTT_DIM], &a[8 * NTT_DIM]);
	mont_square8(r, a);

	for (i = 0; i < NTT_DIM; i++)
		r[15 * NTT_DIM + i] = t1[7 * NTT_DIM + i];

	for (i = 0; i < 7 * NTT_DIM / 16; i++)
	{
		v[0] = _mm256_load_si256((__m256i*) & r[8 * NTT_DIM + 16 * i]);
		v[1] = _mm256_load_si256((__m256i*) & t1[16 * i]);
		v[2] = _mm256_load_si256((__m256i*) & t1[8 * NTT_DIM + 16 * i]);
		v[3] = _mm256_load_si256((__m256i*) & r[16 * NTT_DIM + 16 * i]);

		v[0] = _mm256_add_epi16(v[0], v[1]);
		v[1] = _mm256_add_epi16(v[2], v[3]);

		_mm256_store_si256((__m256i*) & r[8 * NTT_DIM + 16 * i], v[0]);
		_mm256_store_si256((__m256i*) & r[16 * NTT_DIM + 16 * i], v[1]);
	}
}

inline __m256i _mm256_mulhi_epi32(__m256i a, __m256i b) {
	__m256i u = _mm256_mul_epi32(a, b);
	__m256i v = _mm256_mul_epi32(_mm256_srli_epi64(a, 32), b);
	return _mm256_blend_epi32(_mm256_shuffle_epi32(u,0xf5) ,v, 0xaa);
}

static inline __m256i fqmul_epi16(__m256i a,
								  __m256i b,
								  __m256i vq16x,
								  __m256i vqinv16x)
{
	__m256i dh, d;

	dh = _mm256_mulhi_epi16(a, b);
	d = _mm256_mullo_epi16(a, b);

	d = _mm256_mullo_epi16(d, vqinv16x);
	d = _mm256_mulhi_epi16(d, vq16x);

	return _mm256_sub_epi16(dh, d);
}

static inline __m256i mont2_inv_epi16(__m256i s)
{
    __m256i vq16x = _mm256_set1_epi16((int16_t)PARAM_Q);
    __m256i vqinv16x = _mm256_set1_epi16((int16_t)QINV);

#if PARAM_Q == 641

    /*
     * Addition chain:
     * 1, 2, 3, 4, 7, 8, 16, 32, 39, 71,
     * 142, 284, 568, 639
     *
     * 639 = q - 2
     */
    __m256i y0, y1;

    y1 = fqmul_epi16(s, s, vq16x, vqinv16x);       /* 2 */
    y0 = fqmul_epi16(y1, s, vq16x, vqinv16x);      /* 3 */

    y1 = fqmul_epi16(y1, y1, vq16x, vqinv16x);     /* 4 */
    y0 = fqmul_epi16(y0, y1, vq16x, vqinv16x);     /* 7 */

    y1 = fqmul_epi16(y1, y1, vq16x, vqinv16x);     /* 8 */
    y1 = fqmul_epi16(y1, y1, vq16x, vqinv16x);     /* 16 */
    y1 = fqmul_epi16(y1, y1, vq16x, vqinv16x);     /* 32 */

    y0 = fqmul_epi16(y1, y0, vq16x, vqinv16x);     /* 39 */
    y0 = fqmul_epi16(y1, y0, vq16x, vqinv16x);     /* 71 */

    y1 = fqmul_epi16(y0, y0, vq16x, vqinv16x);     /* 142 */
    y1 = fqmul_epi16(y1, y1, vq16x, vqinv16x);     /* 284 */
    y1 = fqmul_epi16(y1, y1, vq16x, vqinv16x);     /* 568 */
    y1 = fqmul_epi16(y1, y0, vq16x, vqinv16x);     /* 639 */

    return y1;

#elif PARAM_Q == 1409

    /*
     * Addition chain:
     * 1, 2, 4, 8, 16, 32, 64, 128,
     * 192, 200, 201, 402, 804, 1206, 1407
     *
     * 1407 = q - 2
     */
    __m256i y0, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10, y11, y12, y13;

    y0  = fqmul_epi16(s, s, vq16x, vqinv16x);        /* 2 */
    y1  = fqmul_epi16(y0, y0, vq16x, vqinv16x);      /* 4 */
    y2  = fqmul_epi16(y1, y1, vq16x, vqinv16x);      /* 8 */
    y3  = fqmul_epi16(y2, y2, vq16x, vqinv16x);      /* 16 */
    y4  = fqmul_epi16(y3, y3, vq16x, vqinv16x);      /* 32 */
    y5  = fqmul_epi16(y4, y4, vq16x, vqinv16x);      /* 64 */
    y6  = fqmul_epi16(y5, y5, vq16x, vqinv16x);      /* 128 */

    y7  = fqmul_epi16(y5, y6, vq16x, vqinv16x);      /* 192 */
    y8  = fqmul_epi16(y7, y2, vq16x, vqinv16x);      /* 200 */
    y9  = fqmul_epi16(y8, s, vq16x, vqinv16x);       /* 201 */

    y10 = fqmul_epi16(y9, y9, vq16x, vqinv16x);      /* 402 */
    y11 = fqmul_epi16(y10, y10, vq16x, vqinv16x);    /* 804 */
    y12 = fqmul_epi16(y10, y11, vq16x, vqinv16x);    /* 1206 */
    y13 = fqmul_epi16(y12, y9, vq16x, vqinv16x);     /* 1407 */

    return y13;

#elif PARAM_Q == 3329

    /*
     * Addition chain:
     * 1, 2, 4, 8, 16, 32, 64, 128,
     * 144, 145, 273, 418, 836, 1109, 2218, 3327
     *
     * 3327 = q - 2
     */
    __m256i y0, y1, y2, y3, y4, y5, y6, y7, y8, y9, y10, y11, y12, y13, y14;

    y0  = fqmul_epi16(s, s, vq16x, vqinv16x);        /* 2 */
    y1  = fqmul_epi16(y0, y0, vq16x, vqinv16x);      /* 4 */
    y2  = fqmul_epi16(y1, y1, vq16x, vqinv16x);      /* 8 */
    y3  = fqmul_epi16(y2, y2, vq16x, vqinv16x);      /* 16 */
    y4  = fqmul_epi16(y3, y3, vq16x, vqinv16x);      /* 32 */
    y5  = fqmul_epi16(y4, y4, vq16x, vqinv16x);      /* 64 */
    y6  = fqmul_epi16(y5, y5, vq16x, vqinv16x);      /* 128 */

    y7  = fqmul_epi16(y3, y6, vq16x, vqinv16x);      /* 144 */
    y8  = fqmul_epi16(y7, s, vq16x, vqinv16x);       /* 145 */
    y9  = fqmul_epi16(y8, y6, vq16x, vqinv16x);      /* 273 */
    y10 = fqmul_epi16(y9, y8, vq16x, vqinv16x);      /* 418 */
    y11 = fqmul_epi16(y10, y10, vq16x, vqinv16x);    /* 836 */
    y12 = fqmul_epi16(y11, y9, vq16x, vqinv16x);     /* 1109 */
    y13 = fqmul_epi16(y12, y12, vq16x, vqinv16x);    /* 2218 */
    y14 = fqmul_epi16(y13, y12, vq16x, vqinv16x);    /* 3327 */

    return y14;

#elif PARAM_Q == 769
	__m256i y0, y1;

	y0 = fqmul_epi16(s, s, vq16x, vqinv16x);       /* 2 */
	y0 = fqmul_epi16(y0, y0, vq16x, vqinv16x);     /* 4 */
	y0 = fqmul_epi16(y0, y0, vq16x, vqinv16x);     /* 8 */

	y1 = fqmul_epi16(y0, y0, vq16x, vqinv16x);     /* 16 */
	y1 = fqmul_epi16(y1, y1, vq16x, vqinv16x);     /* 32 */
	y1 = fqmul_epi16(y1, y1, vq16x, vqinv16x);     /* 64 */

	y0 = fqmul_epi16(y1, y0, vq16x, vqinv16x);     /* 72*/
	y0 = fqmul_epi16(y0, s, vq16x, vqinv16x);      /* 73*/

	y1 = fqmul_epi16(y0, y1, vq16x, vqinv16x);     /* 137 */
	y1 = fqmul_epi16(y1, y1, vq16x, vqinv16x);     /* 274 */
	y1 = fqmul_epi16(y1, y0, vq16x, vqinv16x);     /* 347 */
	y1 = fqmul_epi16(y1, y1, vq16x, vqinv16x);     /* 694 */
	y1 = fqmul_epi16(y1, y0, vq16x, vqinv16x);     /* 767 */

	return y1;
#else

#error "Unsupported PARAM_Q"

#endif
}

static inline int mont2_inverse4(int16_t *r, const int16_t *a)
{
    ALIGN(32) __m256i t[9];

    __m256i vq16x = _mm256_set1_epi16((int16_t)PARAM_Q);
    __m256i vqinv16x = _mm256_set1_epi16((int16_t)QINV);
    __m256i zero16x = _mm256_setzero_si256();

    __m256i vntty;
    int i;

    for (i = 0; i < NTT_DIM / 16; i++)
    {
        vntty = _mm256_load_si256((const __m256i *)&NTT_Y[16 * i]);

    	t[0] = _mm256_load_si256((const __m256i *)&a[16 * i]);
    	t[1] = _mm256_load_si256((const __m256i *)&a[NTT_DIM + 16 * i]);
    	t[2] = _mm256_load_si256((const __m256i *)&a[2 * NTT_DIM + 16 * i]);
    	t[3] = _mm256_load_si256((const __m256i *)&a[3 * NTT_DIM + 16 * i]);

        t[4] = _mm256_slli_epi16(t[1], 1);
        t[4] = fqmul_epi16(t[4], t[3], vq16x, vqinv16x);

        t[5] = fqmul_epi16(t[2], t[2], vq16x, vqinv16x);

        t[4] = _mm256_sub_epi16(t[5], t[4]);
        t[4] = fqmul_epi16(t[4], vntty, vq16x, vqinv16x);

        t[5] = fqmul_epi16(t[0], t[0], vq16x, vqinv16x);
        t[4] = _mm256_add_epi16(t[4], t[5]);          /* a2_0 */

        t[5] = _mm256_slli_epi16(t[0], 1);
        t[5] = fqmul_epi16(t[5], t[2], vq16x, vqinv16x);

        t[6] = fqmul_epi16(t[1], t[1], vq16x, vqinv16x);
        t[5] = _mm256_sub_epi16(t[5], t[6]);

        t[6] = fqmul_epi16(t[3], t[3], vq16x, vqinv16x);
        t[6] = fqmul_epi16(t[6], vntty, vq16x, vqinv16x);

        t[5] = _mm256_sub_epi16(t[5], t[6]);          /* a2_1 */

        t[6] = fqmul_epi16(t[4], t[4], vq16x, vqinv16x);

        t[7] = fqmul_epi16(t[5], t[5], vq16x, vqinv16x);
        t[7] = fqmul_epi16(t[7], vntty, vq16x, vqinv16x);

        t[6] = _mm256_sub_epi16(t[6], t[7]);

        t[7] = _mm256_srai_epi16(t[6], 15);
        t[7] = _mm256_and_si256(t[7], vq16x);
        t[6] = _mm256_add_epi16(t[6], t[7]);

        t[7] = _mm256_srai_epi16(t[6], 15);
        t[7] = _mm256_and_si256(t[7], vq16x);
        t[6] = _mm256_add_epi16(t[6], t[7]);

        t[6] = _mm256_sub_epi16(t[6], vq16x);

        t[7] = _mm256_srai_epi16(t[6], 15);
        t[7] = _mm256_and_si256(t[7], vq16x);
        t[6] = _mm256_add_epi16(t[6], t[7]);

        t[7] = _mm256_cmpeq_epi16(t[6], zero16x);

        if (!_mm256_testz_si256(t[7], t[7]))
            return 0;

        t[6] = mont2_inv_epi16(t[6]);

        t[4] = fqmul_epi16(t[4], t[6], vq16x, vqinv16x);
        t[5] = fqmul_epi16(t[5], t[6], vq16x, vqinv16x);

        t[6] = fqmul_epi16(t[0], t[4], vq16x, vqinv16x);
        t[7] = fqmul_epi16(t[1], t[4], vq16x, vqinv16x);

        t[0] = _mm256_add_epi16(t[0], t[2]);
        t[8] = _mm256_sub_epi16(t[4], t[5]);
        t[0] = fqmul_epi16(t[0], t[8], vq16x, vqinv16x);

        t[1] = _mm256_sub_epi16(t[1], t[3]);
        t[4] = _mm256_add_epi16(t[4], t[5]);
        t[1] = fqmul_epi16(t[1], t[4], vq16x, vqinv16x);

        t[2] = fqmul_epi16(t[2], t[5], vq16x, vqinv16x);
        t[3] = fqmul_epi16(t[3], t[5], vq16x, vqinv16x);

        t[4] = fqmul_epi16(t[2], vntty, vq16x, vqinv16x);
        t[5] = fqmul_epi16(t[3], vntty, vq16x, vqinv16x);

        t[4] = _mm256_sub_epi16(t[6], t[4]);

        t[5] = _mm256_sub_epi16(t[5], t[7]);

        t[6] = _mm256_sub_epi16(t[0], t[6]);
        t[6] = _mm256_add_epi16(t[6], t[2]);

        t[7] = _mm256_sub_epi16(t[1], t[7]);
        t[7] = _mm256_add_epi16(t[7], t[3]);

    	_mm256_store_si256((__m256i *)&r[16 * i], t[4]);
    	_mm256_store_si256((__m256i *)&r[NTT_DIM + 16 * i], t[5]);
    	_mm256_store_si256((__m256i *)&r[2 * NTT_DIM + 16 * i], t[6]);
    	_mm256_store_si256((__m256i *)&r[3 * NTT_DIM + 16 * i], t[7]);
    }

    return 1;
}

int16_t caddq1 (int16_t x)
{
	int16_t r;
	r = x + ((x >> 15)& PARAM_Q);
	return r;
}

static inline int mont2_inverse4_judge(int16_t* r, const int16_t* a)
{
	ALIGN(32) __m256i t[9];
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i zero16x = _mm256_setzero_si256();
	__m256i one16x = _mm256_set1_epi16(1);
	__m256i vntty, dh, d;
	int i, j;

	for (i = 0; i < NTT_DIM / 16; i++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * i]);
		for (j = 0; j < 4; j++)
			t[j] = _mm256_load_si256((__m256i*) & a[j * NTT_DIM + 16 * i]);

		t[4] = _mm256_slli_epi16(t[1], 1);
		dh = _mm256_mulhi_epi16(t[4], t[3]);
		d = _mm256_mullo_epi16(t[4], t[3]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[4] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		dh = _mm256_mulhi_epi16(t[2], t[2]);
		d = _mm256_mullo_epi16(t[2], t[2]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[5] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		t[4] = _mm256_sub_epi16(t[5], t[4]);//[-Q,Q]

		dh = _mm256_mulhi_epi16(t[4], vntty);
		d = _mm256_mullo_epi16(t[4], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[4] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		dh = _mm256_mulhi_epi16(t[0], t[0]);
		d = _mm256_mullo_epi16(t[0], t[0]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[5] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		t[4] = _mm256_add_epi16(t[4], t[5]);//a2_0

		t[5] = _mm256_slli_epi16(t[0], 1);
		dh = _mm256_mulhi_epi16(t[5], t[2]);
		d = _mm256_mullo_epi16(t[5], t[2]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[5] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		dh = _mm256_mulhi_epi16(t[1], t[1]);
		d = _mm256_mullo_epi16(t[1], t[1]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[6] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		t[5] = _mm256_sub_epi16(t[5], t[6]);

		dh = _mm256_mulhi_epi16(t[3], t[3]);
		d = _mm256_mullo_epi16(t[3], t[3]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[6] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		dh = _mm256_mulhi_epi16(t[6], vntty);
		d = _mm256_mullo_epi16(t[6], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[6] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		t[5] = _mm256_sub_epi16(t[5], t[6]);//a2_1

		dh = _mm256_mulhi_epi16(t[4], t[4]);
		d = _mm256_mullo_epi16(t[4], t[4]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[6] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		dh = _mm256_mulhi_epi16(t[5], t[5]);
		d = _mm256_mullo_epi16(t[5], t[5]);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[7] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		dh = _mm256_mulhi_epi16(t[7], vntty);
		d = _mm256_mullo_epi16(t[7], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		t[7] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		t[6] = _mm256_sub_epi16(t[6], t[7]);//a3_0

		//reducing from [-2!,2Q) to [0,Q)
		t[6] = _mm256_sub_epi16(t[6], vq16x);
		t[7] = _mm256_srai_epi16(t[6], 15);
		t[7] = _mm256_and_si256(t[7], vq16x);//conditinal adding Q
		t[6] = _mm256_add_epi16(t[6], t[7]);

		t[7] = _mm256_srai_epi16(t[6], 15);
		t[7] = _mm256_and_si256(t[7], vq16x);//conditinal adding Q
		t[6] = _mm256_add_epi16(t[6], t[7]);

		t[7] = _mm256_srai_epi16(t[6], 15);
		t[7] = _mm256_and_si256(t[7], vq16x);//conditinal adding Q
		t[6] = _mm256_add_epi16(t[6], t[7]);

		t[7] = _mm256_cmpgt_epi16(t[6], zero16x);
		t[7] = _mm256_add_epi16(t[7], one16x);

		if (!_mm256_testz_si256(t[7], t[7]))//no inverse!
			return 0;
	}
	return 1;
}
static inline int mont2_inverse8(int16_t* r, const int16_t* a)
{
	ALIGN(32) int16_t t0[4 * NTT_DIM], t1[4 * NTT_DIM];
	ALIGN(32) int16_t b0[7 * NTT_DIM], b1[7 * NTT_DIM];
	__m256i v[4];
	__m256i vntty, dh, d;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i zero16x = _mm256_setzero_si256();

	int i, j;
	for (j = 0; j < 4; j++)
	{
		memcpy(&t0[j * NTT_DIM], &a[2 * j * NTT_DIM], NTT_DIM * 2);
		memcpy(&t1[j * NTT_DIM], &a[(2 * j + 1) * NTT_DIM], NTT_DIM * 2);
	}

	mont_square4(b0, t0);
	mont_square4(b1, t1);


	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);


		v[0] = _mm256_load_si256((__m256i*) & b0[4 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		v[1] = _mm256_load_si256((__m256i*) & b1[3 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[16 * j]);
		v[2] = _mm256_add_epi16(v[2], v[0]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);

		_mm256_store_si256((__m256i*) & b0[16 * j], v[2]);


		for (i = 1; i < 3; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & b0[(4 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b1[(3 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[2] = _mm256_load_si256((__m256i*) & b0[i * NTT_DIM + 16 * j]);
			v[3] = _mm256_load_si256((__m256i*) & b1[(i - 1) * NTT_DIM + 16 * j]);
			v[2] = _mm256_add_epi16(v[2], v[0]);
			v[2] = _mm256_sub_epi16(v[2], v[1]);
			v[2] = _mm256_sub_epi16(v[2], v[3]);

			_mm256_store_si256((__m256i*) & b0[i * NTT_DIM + 16 * j], v[2]);
		}


		v[1] = _mm256_load_si256((__m256i*) & b1[6 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[3 * NTT_DIM + 16 * j]);
		v[3] = _mm256_load_si256((__m256i*) & b1[2 * NTT_DIM + 16 * j]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);
		v[2] = _mm256_sub_epi16(v[2], v[3]);

		_mm256_store_si256((__m256i*) & b0[3 * NTT_DIM + 16 * j], v[2]);
	}

	if (!mont2_inverse4(b1, b0))
		return 0;

	mont_mul4(b0, b1, t0);
	mont_mul4(b1, b1, t1);

	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);

		for (i = 0; i < 3; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & b0[(4 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b0[i * NTT_DIM + 16 * j]);
			v[1] = _mm256_add_epi16(v[0], v[1]);//[-Q,Q]
			_mm256_store_si256((__m256i*) & r[2 * i * NTT_DIM + 16 * j], v[1]);


			v[0] = _mm256_load_si256((__m256i*) & b1[(4 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b1[i * NTT_DIM + 16 * j]);
			v[1] = _mm256_sub_epi16(zero16x, v[1]);
			v[1] = _mm256_sub_epi16(v[1], v[0]);//[-Q,Q]
			_mm256_store_si256((__m256i*) & r[(2 * i + 1) * NTT_DIM + 16 * j], v[1]);
		}
	}
	memcpy(&r[6 * NTT_DIM], &b0[3 * NTT_DIM], NTT_DIM * 2);
	for (j = 0; j < NTT_DIM / 16; j++)
	{
		v[0] = _mm256_load_si256((__m256i*) & b1[3 * NTT_DIM + 16 * j]);
		v[0] = _mm256_sub_epi16(zero16x, v[0]);
		_mm256_store_si256((__m256i*) & r[7 * NTT_DIM + 16 * j], v[0]);

	}
	return 1;
}
static inline int mont2_inverse8_judge(int16_t* r, const int16_t* a)
{
	ALIGN(32) int16_t t0[4 * NTT_DIM], t1[4 * NTT_DIM];
	ALIGN(32) int16_t b0[7 * NTT_DIM], b1[7 * NTT_DIM];
	__m256i v[4];
	__m256i vntty, dh, d;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i zero16x = _mm256_setzero_si256();

	int i, j;
	for (j = 0; j < 4; j++)
	{
		memcpy(&t0[j * NTT_DIM], &a[2 * j * NTT_DIM], NTT_DIM * 2);
		memcpy(&t1[j * NTT_DIM], &a[(2 * j + 1) * NTT_DIM], NTT_DIM * 2);
	}

	mont_square4(b0, t0);
	mont_square4(b1, t1);


	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);


		v[0] = _mm256_load_si256((__m256i*) & b0[4 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		v[1] = _mm256_load_si256((__m256i*) & b1[3 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[16 * j]);
		v[2] = _mm256_add_epi16(v[2], v[0]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);

		_mm256_store_si256((__m256i*) & b0[16 * j], v[2]);


		for (i = 1; i < 3; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & b0[(4 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b1[(3 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[2] = _mm256_load_si256((__m256i*) & b0[i * NTT_DIM + 16 * j]);
			v[3] = _mm256_load_si256((__m256i*) & b1[(i - 1) * NTT_DIM + 16 * j]);
			v[2] = _mm256_add_epi16(v[2], v[0]);
			v[2] = _mm256_sub_epi16(v[2], v[1]);
			v[2] = _mm256_sub_epi16(v[2], v[3]);

			_mm256_store_si256((__m256i*) & b0[i * NTT_DIM + 16 * j], v[2]);
		}


		v[1] = _mm256_load_si256((__m256i*) & b1[6 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[3 * NTT_DIM + 16 * j]);
		v[3] = _mm256_load_si256((__m256i*) & b1[2 * NTT_DIM + 16 * j]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);
		v[2] = _mm256_sub_epi16(v[2], v[3]);

		_mm256_store_si256((__m256i*) & b0[3 * NTT_DIM + 16 * j], v[2]);
	}

	return mont2_inverse4_judge(b1, b0);
}
static inline int mont2_inverse16(int16_t* r, const int16_t* a)
{
	ALIGN(32) int16_t t0[8 * NTT_DIM], t1[8 * NTT_DIM];
	ALIGN(32) int16_t b0[15 * NTT_DIM], b1[15 * NTT_DIM];
	int i, j;
	__m256i v[4];
	__m256i vntty, dh, d;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i zero16x = _mm256_setzero_si256();

	for (j = 0; j < 8; j++)
	{
		memcpy(&t0[j * NTT_DIM], &a[2 * j * NTT_DIM], NTT_DIM * 2);
		memcpy(&t1[j * NTT_DIM], &a[(2 * j + 1) * NTT_DIM], NTT_DIM * 2);
	}

	mont_square8(b0, t0);
	mont_square8(b1, t1);


	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);


		v[0] = _mm256_load_si256((__m256i*) & b0[8 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		v[1] = _mm256_load_si256((__m256i*) & b1[7 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[16 * j]);
		v[2] = _mm256_add_epi16(v[2], v[0]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);

		_mm256_store_si256((__m256i*) & b0[16 * j], v[2]);


		for (i = 1; i < 7; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & b0[(8 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b1[(7 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[2] = _mm256_load_si256((__m256i*) & b0[i * NTT_DIM + 16 * j]);
			v[3] = _mm256_load_si256((__m256i*) & b1[(i - 1) * NTT_DIM + 16 * j]);
			v[2] = _mm256_add_epi16(v[2], v[0]);
			v[2] = _mm256_sub_epi16(v[2], v[1]);
			v[2] = _mm256_sub_epi16(v[2], v[3]);

			_mm256_store_si256((__m256i*) & b0[i * NTT_DIM + 16 * j], v[2]);
		}


		v[1] = _mm256_load_si256((__m256i*) & b1[14 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[7 * NTT_DIM + 16 * j]);
		v[3] = _mm256_load_si256((__m256i*) & b1[6 * NTT_DIM + 16 * j]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);
		v[2] = _mm256_sub_epi16(v[2], v[3]);

		_mm256_store_si256((__m256i*) & b0[7 * NTT_DIM + 16 * j], v[2]);
	}

	if (!mont2_inverse8(b1, b0))
		return 0;

	mont_mul8(b0, b1, t0);
	mont_mul8(b1, b1, t1);

	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);

		for (i = 0; i < 7; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & b0[(8 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b0[i * NTT_DIM + 16 * j]);
			v[1] = _mm256_add_epi16(v[0], v[1]);//[-Q,Q]
			_mm256_store_si256((__m256i*) & r[2 * i * NTT_DIM + 16 * j], v[1]);


			v[0] = _mm256_load_si256((__m256i*) & b1[(8 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b1[i * NTT_DIM + 16 * j]);
			v[1] = _mm256_sub_epi16(zero16x, v[1]);
			v[1] = _mm256_sub_epi16(v[1], v[0]);//[-Q,Q]
			_mm256_store_si256((__m256i*) & r[(2 * i + 1) * NTT_DIM + 16 * j], v[1]);
		}
	}
	memcpy(&r[14 * NTT_DIM], &b0[7 * NTT_DIM], NTT_DIM * 2);
	for (j = 0; j < NTT_DIM / 16; j++)
	{
		v[0] = _mm256_load_si256((__m256i*) & b1[7 * NTT_DIM + 16 * j]);
		v[0] = _mm256_sub_epi16(zero16x, v[0]);
		_mm256_store_si256((__m256i*) & r[15 * NTT_DIM + 16 * j], v[0]);

	}

	return 1;
}
static inline int mont2_inverse16_judge(int16_t* r, const int16_t* a)
{
	ALIGN(32) int16_t t0[8 * NTT_DIM], t1[8 * NTT_DIM];
	ALIGN(32) int16_t b0[15 * NTT_DIM], b1[15 * NTT_DIM];
	int i, j;
	__m256i v[4];
	__m256i vntty, dh, d;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i zero16x = _mm256_setzero_si256();

	for (j = 0; j < 8; j++)
	{
		memcpy(&t0[j * NTT_DIM], &a[2 * j * NTT_DIM], NTT_DIM * 2);
		memcpy(&t1[j * NTT_DIM], &a[(2 * j + 1) * NTT_DIM], NTT_DIM * 2);
	}

	mont_square8(b0, t0);
	mont_square8(b1, t1);

	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);

		v[0] = _mm256_load_si256((__m256i*) & b0[8 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[1] = _mm256_load_si256((__m256i*) & b1[7 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[16 * j]);
		v[2] = _mm256_add_epi16(v[2], v[0]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);

		_mm256_store_si256((__m256i*) & b0[16 * j], v[2]);

		for (i = 1; i < 7; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & b0[(8 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b1[(7 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[2] = _mm256_load_si256((__m256i*) & b0[i * NTT_DIM + 16 * j]);
			v[3] = _mm256_load_si256((__m256i*) & b1[(i - 1) * NTT_DIM + 16 * j]);
			v[2] = _mm256_add_epi16(v[2], v[0]);
			v[2] = _mm256_sub_epi16(v[2], v[1]);
			v[2] = _mm256_sub_epi16(v[2], v[3]);

			_mm256_store_si256((__m256i*) & b0[i * NTT_DIM + 16 * j], v[2]);
		}

		v[1] = _mm256_load_si256((__m256i*) & b1[14 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[7 * NTT_DIM + 16 * j]);
		v[3] = _mm256_load_si256((__m256i*) & b1[6 * NTT_DIM + 16 * j]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);
		v[2] = _mm256_sub_epi16(v[2], v[3]);

		_mm256_store_si256((__m256i*) & b0[7 * NTT_DIM + 16 * j], v[2]);
	}

	return mont2_inverse8_judge(b1, b0);
}
#if PARAM_N/NTT_DIM == 4
void poly_mont_mul(poly* r, const poly* a, const poly* b) {
	ALIGN(32) int16_t t[7 * NTT_DIM];
	int i, j;

	mont_mul4(t, a->coeffs, b->coeffs);

	for (i = 0; i < 3; i++) {
		for (j = 0; j < NTT_DIM; j++) {
			r->coeffs[i * NTT_DIM + j] =
				t[i * NTT_DIM + j] +
				montgomery_reduce(t[(4 + i) * NTT_DIM + j] * NTT_Y[j]);
		}
	}
	for (j = 0; j < NTT_DIM; j++)
		r->coeffs[3 * NTT_DIM + j] = t[3 * NTT_DIM + j];
}
int poly_mont2_inverse(poly* r, const poly*a) {
	return mont2_inverse4(r->coeffs, a->coeffs);
}
int poly_mont2_inverse_judge(const poly* a) {
	ALIGN(32) int16_t t[PARAM_N];
	return mont2_inverse4_judge(t, a->coeffs);
}


#elif PARAM_N/NTT_DIM == 8
void poly_mont_mul(poly* r, const poly* a, const poly* b)
{
	ALIGN(32) int16_t t0[7 * NTT_DIM], t1[7 * NTT_DIM], t2[7 * NTT_DIM], x;
	int i, j;
	__m256i v[6];
	__m256i vntty, dh, d;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)

	for (i = 0; i < NTT_DIM / 4; i++)
	{
		v[0] = _mm256_load_si256((__m256i*) & a->coeffs[16 * i]);
		v[1] = _mm256_load_si256((__m256i*) & a->coeffs[4 * NTT_DIM + 16 * i]);
		v[0] = _mm256_add_epi16(v[0], v[1]);

		v[1] = _mm256_load_si256((__m256i*) & b->coeffs[16 * i]);
		v[2] = _mm256_load_si256((__m256i*) & b->coeffs[4 * NTT_DIM + 16 * i]);
		v[1] = _mm256_add_epi16(v[1], v[2]);

		_mm256_store_si256((__m256i*) & t0[16 * i], v[0]);
		_mm256_store_si256((__m256i*) & t1[16 * i], v[1]);
	}

	mont_mul4(t1, t0, t1);
	mont_mul4(t0, a->coeffs, b->coeffs);
	mont_mul4(t2, &a->coeffs[PARAM_N / 2], &b->coeffs[PARAM_N / 2]);

	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);
		for (i = 0; i < 3; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & t0[i * NTT_DIM + 16 * j]);
			v[1] = _mm256_load_si256((__m256i*) & t1[i * NTT_DIM + 16 * j]);
			v[2] = _mm256_load_si256((__m256i*) & t2[i * NTT_DIM + 16 * j]);
			v[3] = _mm256_load_si256((__m256i*) & t0[(4 + i) * NTT_DIM + 16 * j]);
			v[4] = _mm256_load_si256((__m256i*) & t1[(4 + i) * NTT_DIM + 16 * j]);
			v[5] = _mm256_load_si256((__m256i*) & t2[(4 + i) * NTT_DIM + 16 * j]);

			v[1] = _mm256_add_epi16(v[1], v[3]);
			v[1] = _mm256_sub_epi16(v[1], v[0]);
			v[1] = _mm256_sub_epi16(v[1], v[2]);

			v[2] = _mm256_add_epi16(v[2], v[4]);
			v[2] = _mm256_sub_epi16(v[2], v[3]);
			v[2] = _mm256_sub_epi16(v[2], v[5]);

			dh = _mm256_mulhi_epi16(v[2], vntty);
			d = _mm256_mullo_epi16(v[2], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[2] = _mm256_sub_epi16(dh, d);
			v[0] = _mm256_add_epi16(v[0], v[2]);


			dh = _mm256_mulhi_epi16(v[5], vntty);
			d = _mm256_mullo_epi16(v[5], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[5] = _mm256_sub_epi16(dh, d);
			v[1] = _mm256_add_epi16(v[1], v[5]);


			_mm256_store_si256((__m256i*) & r->coeffs[i * NTT_DIM + 16 * j], v[0]);
			_mm256_store_si256((__m256i*) & r->coeffs[(4 + i) * NTT_DIM + 16 * j], v[1]);
		}

		v[0] = _mm256_load_si256((__m256i*) & t0[3 * NTT_DIM + 16 * j]);
		v[1] = _mm256_load_si256((__m256i*) & t1[3 * NTT_DIM + 16 * j]);
		v[2] = _mm256_load_si256((__m256i*) & t2[3 * NTT_DIM + 16 * j]);

		v[1] = _mm256_sub_epi16(v[1], v[0]);
		v[1] = _mm256_sub_epi16(v[1], v[2]);

		dh = _mm256_mulhi_epi16(v[2], vntty);
		d = _mm256_mullo_epi16(v[2], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[2] = _mm256_sub_epi16(dh, d);//[-Q,Q]
		v[0] = _mm256_add_epi16(v[0], v[2]);

		_mm256_store_si256((__m256i*) & r->coeffs[3 * NTT_DIM + 16 * j], v[0]);
		_mm256_store_si256((__m256i*) & r->coeffs[7 * NTT_DIM + 16 * j], v[1]);
	}
}
int poly_mont2_inverse(poly* r, const poly* a)
{
	ALIGN(32) int16_t t0[4 * NTT_DIM], t1[4 * NTT_DIM];
	ALIGN(32) int16_t b0[7 * NTT_DIM], b1[7 * NTT_DIM];
	__m256i v[4];
	__m256i vntty, dh, d;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i zero16x = _mm256_setzero_si256();

	int i, j;
	for (j = 0; j < 4; j++)
	{
		memcpy(&t0[j * NTT_DIM], &a->coeffs[2 * j * NTT_DIM], NTT_DIM * 2);
		memcpy(&t1[j * NTT_DIM], &a->coeffs[(2 * j + 1) * NTT_DIM], NTT_DIM * 2);
	}

	mont_square4(b0, t0);
	mont_square4(b1, t1);


	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);


		v[0] = _mm256_load_si256((__m256i*) & b0[4 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		v[1] = _mm256_load_si256((__m256i*) & b1[3 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[16 * j]);
		v[2] = _mm256_add_epi16(v[2], v[0]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);

		_mm256_store_si256((__m256i*) & b0[16 * j], v[2]);


		for (i = 1; i < 3; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & b0[(4 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b1[(3 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[2] = _mm256_load_si256((__m256i*) & b0[i * NTT_DIM + 16 * j]);
			v[3] = _mm256_load_si256((__m256i*) & b1[(i - 1) * NTT_DIM + 16 * j]);
			v[2] = _mm256_add_epi16(v[2], v[0]);
			v[2] = _mm256_sub_epi16(v[2], v[1]);
			v[2] = _mm256_sub_epi16(v[2], v[3]);

			_mm256_store_si256((__m256i*) & b0[i * NTT_DIM + 16 * j], v[2]);
		}


		v[1] = _mm256_load_si256((__m256i*) & b1[6 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[3 * NTT_DIM + 16 * j]);
		v[3] = _mm256_load_si256((__m256i*) & b1[2 * NTT_DIM + 16 * j]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);
		v[2] = _mm256_sub_epi16(v[2], v[3]);

		_mm256_store_si256((__m256i*) & b0[3 * NTT_DIM + 16 * j], v[2]);
	}

	if (!mont2_inverse4(b1, b0))
		return 0;

	mont_mul4(b0, b1, t0);
	mont_mul4(b1, b1, t1);

	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);

		for (i = 0; i < 3; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & b0[(4 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b0[i * NTT_DIM + 16 * j]);
			v[1] = _mm256_add_epi16(v[0], v[1]);//[-Q,Q]
			_mm256_store_si256((__m256i*) & r->coeffs[2 * i * NTT_DIM + 16 * j], v[1]);


			v[0] = _mm256_load_si256((__m256i*) & b1[(4 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b1[i * NTT_DIM + 16 * j]);
			v[1] = _mm256_sub_epi16(zero16x, v[1]);
			v[1] = _mm256_sub_epi16(v[1], v[0]);//[-Q,Q]
			_mm256_store_si256((__m256i*) & r->coeffs[(2 * i + 1) * NTT_DIM + 16 * j], v[1]);
		}
	}
	memcpy(&r->coeffs[6 * NTT_DIM], &b0[3 * NTT_DIM], NTT_DIM * 2);
	for (j = 0; j < NTT_DIM / 16; j++)
	{
		v[0] = _mm256_load_si256((__m256i*) & b1[3 * NTT_DIM + 16 * j]);
		v[0] = _mm256_sub_epi16(zero16x, v[0]);
		_mm256_store_si256((__m256i*) & r->coeffs[7 * NTT_DIM + 16 * j], v[0]);

	}
	return 1;
}
int poly_mont2_inverse_judge(const poly*a)
{
	ALIGN(32) int16_t t0[4 * NTT_DIM], t1[4 * NTT_DIM];
	ALIGN(32) int16_t b0[7 * NTT_DIM], b1[7 * NTT_DIM];
	__m256i v[4];
	__m256i vntty, dh, d;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i zero16x = _mm256_setzero_si256();

	int i, j;
	for (j = 0; j < 4; j++)
	{
		memcpy(&t0[j * NTT_DIM], &a->coeffs[2 * j * NTT_DIM], NTT_DIM * 2);
		memcpy(&t1[j * NTT_DIM], &a->coeffs[(2 * j + 1) * NTT_DIM], NTT_DIM * 2);
	}

	mont_square4(b0, t0);
	mont_square4(b1, t1);

	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);

		v[0] = _mm256_load_si256((__m256i*) & b0[4 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[1] = _mm256_load_si256((__m256i*) & b1[3 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[16 * j]);
		v[2] = _mm256_add_epi16(v[2], v[0]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);

		_mm256_store_si256((__m256i*) & b0[16 * j], v[2]);

		for (i = 1; i < 3; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & b0[(4 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b1[(3 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[2] = _mm256_load_si256((__m256i*) & b0[i * NTT_DIM + 16 * j]);
			v[3] = _mm256_load_si256((__m256i*) & b1[(i - 1) * NTT_DIM + 16 * j]);
			v[2] = _mm256_add_epi16(v[2], v[0]);
			v[2] = _mm256_sub_epi16(v[2], v[1]);
			v[2] = _mm256_sub_epi16(v[2], v[3]);

			_mm256_store_si256((__m256i*) & b0[i * NTT_DIM + 16 * j], v[2]);
		}

		v[1] = _mm256_load_si256((__m256i*) & b1[6 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[3 * NTT_DIM + 16 * j]);
		v[3] = _mm256_load_si256((__m256i*) & b1[2 * NTT_DIM + 16 * j]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);
		v[2] = _mm256_sub_epi16(v[2], v[3]);

		_mm256_store_si256((__m256i*) & b0[3 * NTT_DIM + 16 * j], v[2]);
	}

	return mont2_inverse4_judge(b1, b0);
}
#elif PARAM_N/NTT_DIM == 16
void poly_mont_mul(poly* r, const poly* a, const poly* b)
{
	ALIGN(32) int16_t t0[15 * NTT_DIM];
	ALIGN(32) int16_t t1[15 * NTT_DIM]; 
	ALIGN(32) int16_t t2[15 * NTT_DIM];
	int16_t  x;
	int i, j;
	__m256i v[6];
	__m256i vntty, dh, d;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)

#if PARAM_Q == 3329
	__m256i vbarrett16x = _mm256_set1_epi16(20159);
	__m256i vround16x = _mm256_set1_epi16(1 << 9);
#endif

	for (i = 0; i < NTT_DIM / 2; i++)
	{
		v[0] = _mm256_load_si256((__m256i*) & a->coeffs[16 * i]);
		v[1] = _mm256_load_si256((__m256i*) & a->coeffs[8 * NTT_DIM + 16 * i]);
		v[0] = _mm256_add_epi16(v[0], v[1]);

#if PARAM_Q == 3329
		d = _mm256_mulhi_epi16(v[0], vbarrett16x);
		d = _mm256_add_epi16(d, vround16x);
		d = _mm256_srai_epi16(d, 10);
		d = _mm256_mullo_epi16(d, vq16x);
		v[0] = _mm256_sub_epi16(v[0], d);
#endif

		v[1] = _mm256_load_si256((__m256i*) & b->coeffs[16 * i]);
		v[2] = _mm256_load_si256((__m256i*) & b->coeffs[8 * NTT_DIM + 16 * i]);
		v[1] = _mm256_add_epi16(v[1], v[2]);

#if PARAM_Q == 3329
		d = _mm256_mulhi_epi16(v[1], vbarrett16x);
		d = _mm256_add_epi16(d, vround16x);
		d = _mm256_srai_epi16(d, 10);
		d = _mm256_mullo_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(v[1], d);
#endif

		_mm256_store_si256((__m256i*) & t0[16 * i], v[0]);
		_mm256_store_si256((__m256i*) & t1[16 * i], v[1]);
	}

	mont_mul8(t1, t0, t1);
	mont_mul8(t0, a->coeffs, b->coeffs);
	mont_mul8(t2, &a->coeffs[PARAM_N / 2], &b->coeffs[PARAM_N / 2]);

	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);
		for (i = 0; i < 7; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & t0[i * NTT_DIM + 16 * j]);
			v[1] = _mm256_load_si256((__m256i*) & t1[i * NTT_DIM + 16 * j]);
			v[2] = _mm256_load_si256((__m256i*) & t2[i * NTT_DIM + 16 * j]);
			v[3] = _mm256_load_si256((__m256i*) & t0[(8 + i) * NTT_DIM + 16 * j]);
			v[4] = _mm256_load_si256((__m256i*) & t1[(8 + i) * NTT_DIM + 16 * j]);
			v[5] = _mm256_load_si256((__m256i*) & t2[(8 + i) * NTT_DIM + 16 * j]);

			v[1] = _mm256_add_epi16(v[1], v[3]);
			v[1] = _mm256_sub_epi16(v[1], v[0]);
			v[1] = _mm256_sub_epi16(v[1], v[2]);

			v[2] = _mm256_add_epi16(v[2], v[4]);
			v[2] = _mm256_sub_epi16(v[2], v[3]);
			v[2] = _mm256_sub_epi16(v[2], v[5]);

			dh = _mm256_mulhi_epi16(v[2], vntty);
			d = _mm256_mullo_epi16(v[2], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[2] = _mm256_sub_epi16(dh, d);
			v[0] = _mm256_add_epi16(v[0], v[2]);


			dh = _mm256_mulhi_epi16(v[5], vntty);
			d = _mm256_mullo_epi16(v[5], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[5] = _mm256_sub_epi16(dh, d);
			v[1] = _mm256_add_epi16(v[1], v[5]);


			_mm256_store_si256((__m256i*) & r->coeffs[i * NTT_DIM + 16 * j], v[0]);
			_mm256_store_si256((__m256i*) & r->coeffs[(8 + i) * NTT_DIM + 16 * j], v[1]);
		}

		v[0] = _mm256_load_si256((__m256i*) & t0[7 * NTT_DIM + 16 * j]);
		v[1] = _mm256_load_si256((__m256i*) & t1[7 * NTT_DIM + 16 * j]);
		v[2] = _mm256_load_si256((__m256i*) & t2[7 * NTT_DIM + 16 * j]);

		v[1] = _mm256_sub_epi16(v[1], v[0]);
		v[1] = _mm256_sub_epi16(v[1], v[2]);

		dh = _mm256_mulhi_epi16(v[2], vntty);
		d = _mm256_mullo_epi16(v[2], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[2] = _mm256_sub_epi16(dh, d);//[-Q,Q]
		v[0] = _mm256_add_epi16(v[0], v[2]);

		_mm256_store_si256((__m256i*) & r->coeffs[7 * NTT_DIM + 16 * j], v[0]);
		_mm256_store_si256((__m256i*) & r->coeffs[15 * NTT_DIM + 16 * j], v[1]);
	}
}
int poly_mont2_inverse(poly* r, const poly* a)
{
	ALIGN(32) int16_t t0[8 * NTT_DIM]; 
	ALIGN(32) int16_t t1[8 * NTT_DIM];
	ALIGN(32) int16_t b0[15 * NTT_DIM];
	ALIGN(32) int16_t b1[15 * NTT_DIM];
	int i, j;
	__m256i v[4];
	__m256i vntty, dh, d;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i zero16x = _mm256_setzero_si256();

	for (j = 0; j < 8; j++)
	{
		memcpy(&t0[j * NTT_DIM], &a->coeffs[2 * j * NTT_DIM], NTT_DIM * 2);
		memcpy(&t1[j * NTT_DIM], &a->coeffs[(2 * j + 1) * NTT_DIM], NTT_DIM * 2);
	}

	mont_square8(b0, t0);
	mont_square8(b1, t1);


	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);


		v[0] = _mm256_load_si256((__m256i*) & b0[8 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		v[1] = _mm256_load_si256((__m256i*) & b1[7 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[16 * j]);
		v[2] = _mm256_add_epi16(v[2], v[0]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);

		_mm256_store_si256((__m256i*) & b0[16 * j], v[2]);


		for (i = 1; i < 7; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & b0[(8 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b1[(7 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[2] = _mm256_load_si256((__m256i*) & b0[i * NTT_DIM + 16 * j]);
			v[3] = _mm256_load_si256((__m256i*) & b1[(i - 1) * NTT_DIM + 16 * j]);
			v[2] = _mm256_add_epi16(v[2], v[0]);
			v[2] = _mm256_sub_epi16(v[2], v[1]);
			v[2] = _mm256_sub_epi16(v[2], v[3]);

			_mm256_store_si256((__m256i*) & b0[i * NTT_DIM + 16 * j], v[2]);
		}


		v[1] = _mm256_load_si256((__m256i*) & b1[14 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[7 * NTT_DIM + 16 * j]);
		v[3] = _mm256_load_si256((__m256i*) & b1[6 * NTT_DIM + 16 * j]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);
		v[2] = _mm256_sub_epi16(v[2], v[3]);

		_mm256_store_si256((__m256i*) & b0[7 * NTT_DIM + 16 * j], v[2]);
	}

	if (!mont2_inverse8(b1, b0))
		return 0;

	mont_mul8(b0, b1, t0);
	mont_mul8(b1, b1, t1);

	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);

		for (i = 0; i < 7; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & b0[(8 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b0[i * NTT_DIM + 16 * j]);
			v[1] = _mm256_add_epi16(v[0], v[1]);//[-Q,Q]
			_mm256_store_si256((__m256i*) & r->coeffs[2 * i * NTT_DIM + 16 * j], v[1]);


			v[0] = _mm256_load_si256((__m256i*) & b1[(8 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b1[i * NTT_DIM + 16 * j]);
			v[1] = _mm256_sub_epi16(zero16x, v[1]);
			v[1] = _mm256_sub_epi16(v[1], v[0]);//[-Q,Q]
			_mm256_store_si256((__m256i*) & r->coeffs[(2 * i + 1) * NTT_DIM + 16 * j], v[1]);
		}
	}
	memcpy(&r->coeffs[14 * NTT_DIM], &b0[7 * NTT_DIM], NTT_DIM * 2);
	for (j = 0; j < NTT_DIM / 16; j++)
	{
		v[0] = _mm256_load_si256((__m256i*) & b1[7 * NTT_DIM + 16 * j]);
		v[0] = _mm256_sub_epi16(zero16x, v[0]);
		_mm256_store_si256((__m256i*) & r->coeffs[15 * NTT_DIM + 16 * j], v[0]);

	}

	return 1;
}
int poly_mont2_inverse_judge(const poly*a)
{
	ALIGN(32) int16_t t0[8 * NTT_DIM];
	ALIGN(32) int16_t t1[8 * NTT_DIM];
	ALIGN(32) int16_t b0[15 * NTT_DIM];
	ALIGN(32) int16_t b1[15 * NTT_DIM];
	int i, j;
	__m256i v[4];
	__m256i vntty, dh, d;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i zero16x = _mm256_setzero_si256();

	for (j = 0; j < 8; j++)
	{
		memcpy(&t0[j * NTT_DIM], &a->coeffs[2 * j * NTT_DIM], NTT_DIM * 2);
		memcpy(&t1[j * NTT_DIM], &a->coeffs[(2 * j + 1) * NTT_DIM], NTT_DIM * 2);
	}

	mont_square8(b0, t0);
	mont_square8(b1, t1);

	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);

		v[0] = _mm256_load_si256((__m256i*) & b0[8 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		v[1] = _mm256_load_si256((__m256i*) & b1[7 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[16 * j]);
		v[2] = _mm256_add_epi16(v[2], v[0]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);

		_mm256_store_si256((__m256i*) & b0[16 * j], v[2]);

		for (i = 1; i < 7; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & b0[(8 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b1[(7 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[2] = _mm256_load_si256((__m256i*) & b0[i * NTT_DIM + 16 * j]);
			v[3] = _mm256_load_si256((__m256i*) & b1[(i - 1) * NTT_DIM + 16 * j]);
			v[2] = _mm256_add_epi16(v[2], v[0]);
			v[2] = _mm256_sub_epi16(v[2], v[1]);
			v[2] = _mm256_sub_epi16(v[2], v[3]);

			_mm256_store_si256((__m256i*) & b0[i * NTT_DIM + 16 * j], v[2]);
		}

		v[1] = _mm256_load_si256((__m256i*) & b1[14 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[7 * NTT_DIM + 16 * j]);
		v[3] = _mm256_load_si256((__m256i*) & b1[6 * NTT_DIM + 16 * j]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);
		v[2] = _mm256_sub_epi16(v[2], v[3]);

		_mm256_store_si256((__m256i*) & b0[7 * NTT_DIM + 16 * j], v[2]);
	}

	if (!mont2_inverse8_judge(b1, b0))
		return 0;

	return 1;
}
#elif PARAM_N/NTT_DIM == 32
void poly_mont_mul(poly* r, const poly* a, const poly* b)
{
	ALIGN(32) int16_t t0[31 * NTT_DIM], t1[31 * NTT_DIM], t2[31 * NTT_DIM], x;
	int i, j;
	__m256i v[6];
	__m256i vntty, dh, d;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)

	for (i = 0; i < NTT_DIM; i++)
	{
		v[0] = _mm256_load_si256((__m256i*) & a->coeffs[16 * i]);
		v[1] = _mm256_load_si256((__m256i*) & a->coeffs[16 * NTT_DIM + 16 * i]);
		v[0] = _mm256_add_epi16(v[0], v[1]);

		v[1] = _mm256_load_si256((__m256i*) & b->coeffs[16 * i]);
		v[2] = _mm256_load_si256((__m256i*) & b->coeffs[16 * NTT_DIM + 16 * i]);
		v[1] = _mm256_add_epi16(v[1], v[2]);

		_mm256_store_si256((__m256i*) & t0[16 * i], v[0]);
		_mm256_store_si256((__m256i*) & t1[16 * i], v[1]);
	}

	mont_mul16(t1, t0, t1);
	mont_mul16(t0, a->coeffs, b->coeffs);
	mont_mul16(t2, &a->coeffs[PARAM_N / 2], &b->coeffs[PARAM_N / 2]);

	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);
		for (i = 0; i < 15; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & t0[i * NTT_DIM + 16 * j]);
			v[1] = _mm256_load_si256((__m256i*) & t1[i * NTT_DIM + 16 * j]);
			v[2] = _mm256_load_si256((__m256i*) & t2[i * NTT_DIM + 16 * j]);
			v[3] = _mm256_load_si256((__m256i*) & t0[(16 + i) * NTT_DIM + 16 * j]);
			v[4] = _mm256_load_si256((__m256i*) & t1[(16 + i) * NTT_DIM + 16 * j]);
			v[5] = _mm256_load_si256((__m256i*) & t2[(16 + i) * NTT_DIM + 16 * j]);

			v[1] = _mm256_add_epi16(v[1], v[3]);
			v[1] = _mm256_sub_epi16(v[1], v[0]);
			v[1] = _mm256_sub_epi16(v[1], v[2]);

			v[2] = _mm256_add_epi16(v[2], v[4]);
			v[2] = _mm256_sub_epi16(v[2], v[3]);
			v[2] = _mm256_sub_epi16(v[2], v[5]);

			dh = _mm256_mulhi_epi16(v[2], vntty);
			d = _mm256_mullo_epi16(v[2], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[2] = _mm256_sub_epi16(dh, d);
			v[0] = _mm256_add_epi16(v[0], v[2]);


			dh = _mm256_mulhi_epi16(v[5], vntty);
			d = _mm256_mullo_epi16(v[5], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[5] = _mm256_sub_epi16(dh, d);
			v[1] = _mm256_add_epi16(v[1], v[5]);


			_mm256_store_si256((__m256i*) & r->coeffs[i * NTT_DIM + 16 * j], v[0]);
			_mm256_store_si256((__m256i*) & r->coeffs[(16 + i) * NTT_DIM + 16 * j], v[1]);
		}

		v[0] = _mm256_load_si256((__m256i*) & t0[15 * NTT_DIM + 16 * j]);
		v[1] = _mm256_load_si256((__m256i*) & t1[15 * NTT_DIM + 16 * j]);
		v[2] = _mm256_load_si256((__m256i*) & t2[15 * NTT_DIM + 16 * j]);

		v[1] = _mm256_sub_epi16(v[1], v[0]);
		v[1] = _mm256_sub_epi16(v[1], v[2]);

		dh = _mm256_mulhi_epi16(v[2], vntty);
		d = _mm256_mullo_epi16(v[2], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[2] = _mm256_sub_epi16(dh, d);//[-Q,Q]
		v[0] = _mm256_add_epi16(v[0], v[2]);

		_mm256_store_si256((__m256i*) & r->coeffs[15 * NTT_DIM + 16 * j], v[0]);
		_mm256_store_si256((__m256i*) & r->coeffs[31 * NTT_DIM + 16 * j], v[1]);
	}
}
int poly_mont2_inverse(poly* r, const poly* a)
{
	ALIGN(32) int16_t t0[16 * NTT_DIM], t1[16 * NTT_DIM];
	ALIGN(32) int16_t b0[31 * NTT_DIM], b1[31 * NTT_DIM];
	int i, j;
	__m256i v[4];
	__m256i vntty, dh, d;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i zero16x = _mm256_setzero_si256();

	for (j = 0; j < 16; j++)
	{
		memcpy(&t0[j * NTT_DIM], &a->coeffs[2 * j * NTT_DIM], NTT_DIM * 2);
		memcpy(&t1[j * NTT_DIM], &a->coeffs[(2 * j + 1) * NTT_DIM], NTT_DIM * 2);
	}

	mont_square16(b0, t0);
	mont_square16(b1, t1);


	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);


		v[0] = _mm256_load_si256((__m256i*) & b0[16 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		v[1] = _mm256_load_si256((__m256i*) & b1[15 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[16 * j]);
		v[2] = _mm256_add_epi16(v[2], v[0]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);

		_mm256_store_si256((__m256i*) & b0[16 * j], v[2]);


		for (i = 1; i < 15; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & b0[(16 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b1[(15 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[2] = _mm256_load_si256((__m256i*) & b0[i * NTT_DIM + 16 * j]);
			v[3] = _mm256_load_si256((__m256i*) & b1[(i - 1) * NTT_DIM + 16 * j]);
			v[2] = _mm256_add_epi16(v[2], v[0]);
			v[2] = _mm256_sub_epi16(v[2], v[1]);
			v[2] = _mm256_sub_epi16(v[2], v[3]);

			_mm256_store_si256((__m256i*) & b0[i * NTT_DIM + 16 * j], v[2]);
		}


		v[1] = _mm256_load_si256((__m256i*) & b1[30 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[15 * NTT_DIM + 16 * j]);
		v[3] = _mm256_load_si256((__m256i*) & b1[14 * NTT_DIM + 16 * j]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);
		v[2] = _mm256_sub_epi16(v[2], v[3]);

		_mm256_store_si256((__m256i*) & b0[15 * NTT_DIM + 16 * j], v[2]);
	}

	if (!mont2_inverse16(b1, b0))
		return 0;

	mont_mul16(b0, b1, t0);
	mont_mul16(b1, b1, t1);

	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);

		for (i = 0; i < 15; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & b0[(16 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b0[i * NTT_DIM + 16 * j]);
			v[1] = _mm256_add_epi16(v[0], v[1]);//[-Q,Q]
			_mm256_store_si256((__m256i*) & r->coeffs[2 * i * NTT_DIM + 16 * j], v[1]);


			v[0] = _mm256_load_si256((__m256i*) & b1[(16 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b1[i * NTT_DIM + 16 * j]);
			v[1] = _mm256_sub_epi16(zero16x, v[1]);
			v[1] = _mm256_sub_epi16(v[1], v[0]);//[-Q,Q]
			_mm256_store_si256((__m256i*) & r->coeffs[(2 * i + 1) * NTT_DIM + 16 * j], v[1]);
		}
	}
	memcpy(&r->coeffs[30 * NTT_DIM], &b0[15 * NTT_DIM], NTT_DIM * 2);
	for (j = 0; j < NTT_DIM / 16; j++)
	{
		v[0] = _mm256_load_si256((__m256i*) & b1[15 * NTT_DIM + 16 * j]);
		v[0] = _mm256_sub_epi16(zero16x, v[0]);
		_mm256_store_si256((__m256i*) & r->coeffs[31 * NTT_DIM + 16 * j], v[0]);

	}

	return 1;
}
int poly_mont2_inverse_judge(const poly* a)
{
	ALIGN(32) int16_t t0[16 * NTT_DIM], t1[16 * NTT_DIM];
	ALIGN(32) int16_t b0[31 * NTT_DIM], b1[31 * NTT_DIM];
	int i, j;
	__m256i v[4];
	__m256i vntty, dh, d;
	__m256i vq16x = _mm256_set1_epi16(PARAM_Q);
	__m256i vqinv16x = _mm256_set1_epi16(QINV); //inverse_mod(q,2^16)
	__m256i zero16x = _mm256_setzero_si256();

	for (j = 0; j < 16; j++)
	{
		memcpy(&t0[j * NTT_DIM], &a->coeffs[2 * j * NTT_DIM], NTT_DIM * 2);
		memcpy(&t1[j * NTT_DIM], &a->coeffs[(2 * j + 1) * NTT_DIM], NTT_DIM * 2);
	}

	mont_square16(b0, t0);
	mont_square16(b1, t1);


	for (j = 0; j < NTT_DIM / 16; j++)
	{
		vntty = _mm256_load_si256((__m256i*) & NTT_Y[16 * j]);


		v[0] = _mm256_load_si256((__m256i*) & b0[16 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(v[0], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]


		v[1] = _mm256_load_si256((__m256i*) & b1[15 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[16 * j]);
		v[2] = _mm256_add_epi16(v[2], v[0]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);

		_mm256_store_si256((__m256i*) & b0[16 * j], v[2]);


		for (i = 1; i < 15; i++)
		{
			v[0] = _mm256_load_si256((__m256i*) & b0[(16 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(v[0], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[0] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[1] = _mm256_load_si256((__m256i*) & b1[(15 + i) * NTT_DIM + 16 * j]);
			dh = _mm256_mulhi_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(v[1], vntty);
			d = _mm256_mullo_epi16(d, vqinv16x);
			d = _mm256_mulhi_epi16(d, vq16x);
			v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

			v[2] = _mm256_load_si256((__m256i*) & b0[i * NTT_DIM + 16 * j]);
			v[3] = _mm256_load_si256((__m256i*) & b1[(i - 1) * NTT_DIM + 16 * j]);
			v[2] = _mm256_add_epi16(v[2], v[0]);
			v[2] = _mm256_sub_epi16(v[2], v[1]);
			v[2] = _mm256_sub_epi16(v[2], v[3]);

			_mm256_store_si256((__m256i*) & b0[i * NTT_DIM + 16 * j], v[2]);
		}


		v[1] = _mm256_load_si256((__m256i*) & b1[30 * NTT_DIM + 16 * j]);
		dh = _mm256_mulhi_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(v[1], vntty);
		d = _mm256_mullo_epi16(d, vqinv16x);
		d = _mm256_mulhi_epi16(d, vq16x);
		v[1] = _mm256_sub_epi16(dh, d);//[-Q,Q]

		v[2] = _mm256_load_si256((__m256i*) & b0[15 * NTT_DIM + 16 * j]);
		v[3] = _mm256_load_si256((__m256i*) & b1[14 * NTT_DIM + 16 * j]);
		v[2] = _mm256_sub_epi16(v[2], v[1]);
		v[2] = _mm256_sub_epi16(v[2], v[3]);

		_mm256_store_si256((__m256i*) & b0[15 * NTT_DIM + 16 * j], v[2]);
	}

	return mont2_inverse16_judge(b1, b0);
}
#endif


#if NTT_DIM == 64
void poly_extract(uint8_t *key, poly *m) {
	uint16_t t;
	int p0,p1;
	for (int i = 0; i < PARAM_N / 64; ++i) {
		for (int j = 0; j < 2; ++j) {
			key[2 * i + j] = 0;
			p0 = 64 * i + 8 * j;
			for (int k = 0; k < 8; ++k) {
				p1 = p0 + k;
				t = m->coeffs[p1] + m->coeffs[p1 + 16]+ m->coeffs[p1 + 32] + m->coeffs[p1 + 48];
				t &= 1;
				key[2 * i + j] ^= t << k;
			}
		}
	}
}
#elif NTT_DIM == 128
void poly_extract(uint8_t *key, poly *m)
{
	uint16_t t;
	int p0, p1;

	for (int i = 0; i < PARAM_N / 128; ++i) {
		for (int j = 0; j < 4; ++j) {
			key[4 * i + j] = 0;
			p0 = 128 * i + 8 * j;

			for (int k = 0; k < 8; ++k) {
				p1 = p0 + k;
				t = m->coeffs[p1]
				  + m->coeffs[p1 + 32]
				  + m->coeffs[p1 + 64]
				  + m->coeffs[p1 + 96];
				key[4 * i + j] |= (t & 1) << k;
			}
		}
	}
}
#endif