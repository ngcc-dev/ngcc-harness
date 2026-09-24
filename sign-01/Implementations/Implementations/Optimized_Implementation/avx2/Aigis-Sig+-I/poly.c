#include <stdint.h>
#include <immintrin.h>
#include "fips202.h"
#include "fips202x4.h"
#include "params.h"
#include "ntt.h"
#include "poly.h"

#include <string.h>

#include "hashkdf.h"
#include "api.h"

static inline uint32_t load_to32(const uint8_t *src, unsigned int len)
{
	uint32_t value = 0;

	for (unsigned int i = 0; i < len; ++i)
		value |= (uint32_t)src[i] << (8 * i);

	return value;
}

void poly_reduce(poly *a)
{
	int i;
	int shift = 22;
	__m256i half = _mm256_set1_epi32(1 << 21);

	__m256i *pa = (__m256i *)a->coeffs;
	__m256i q8x = _mm256_set1_epi32(PARAM_Q);
	__m256i tmp0, tmp1;

	for (i = 0; i < PARAM_N / 8; ++i)
	{
		tmp0 = _mm256_loadu_si256(&pa[i]);
		tmp1 = _mm256_add_epi32(tmp0, half);
		tmp1 = _mm256_srai_epi32(tmp1, shift);
		tmp1 = _mm256_mullo_epi32(tmp1, q8x);
		tmp0 = _mm256_sub_epi32(tmp0, tmp1);
		_mm256_storeu_si256(&pa[i], tmp0);
	}
}
inline __m256i _mm256_mulhi_epi32(__m256i a, __m256i b) {
	__m256i u = _mm256_mul_epi32(a, b);
	__m256i v = _mm256_mul_epi32(_mm256_srli_epi64(a, 32), b);
	return _mm256_blend_epi32(_mm256_shuffle_epi32(u,0xf5) ,v, 0xaa);
}
void poly_g_reduce_avx(poly *a) {
	__m256i f, g, h;
	const __m256i v1 = _mm256_set1_epi32(1079539876);
	const __m256i v2 = _mm256_set1_epi32(1079539877);
	const __m256i q = _mm256_set1_epi32(PARAM_Q);
	for (int i = 0; i < PARAM_N / 8; ++i) {
		f = _mm256_load_si256(a->coeffs + i * 8);
		g = _mm256_mulhi_epi32(f, v1);
		h = _mm256_mulhi_epi32(f, v2);
		g = _mm256_add_epi32(g,h);
		g = _mm256_srai_epi32(g,21);
		g = _mm256_mullo_epi32(g,q);
		g = _mm256_sub_epi32(f,g);
		_mm256_store_si256(a->coeffs + i * 8, g);
	}
};

// void poly_g_reduce_avx(poly *a) {
// 	__m256i f, g, h;
// 	const __m256i v1 = _mm256_set1_epi32(1079539876);
// 	const __m256i v2 = _mm256_set1_epi32(1079539877);
// 	const __m256i q = _mm256_set1_epi32(PARAM_Q);
// 	for (int i = 0; i < PARAM_N / 8; ++i) {
// 		f = _mm256_load_si256(a->coeffs + i * 8);
// 		g = _mm256_mulhi_2159079753(f);
// 		g = _mm256_add_epi32(g,h);
// 		g = _mm256_srai_epi32(g,21);
// 		g = _mm256_mullo_epi32(g,q);
// 		g = _mm256_sub_epi32(f,g);
// 		_mm256_store_si256(a->coeffs + i * 8, g);
// 	}
// };

void poly_amodq(poly *a)
{
	int i;
	__m256i *pa = (__m256i *)a->coeffs;
	__m256i q8x = _mm256_set1_epi32(PARAM_Q);
	__m256i tmp0, tmp1;

	for (i = 0; i < PARAM_N / 8; ++i)
	{
		tmp0 = _mm256_loadu_si256(&pa[i]);
		tmp1 = _mm256_srai_epi32(tmp0, 31);
		tmp1 = _mm256_and_si256(tmp1, q8x);
		pa[i] = _mm256_add_epi32(tmp0, tmp1);
	}
}

void poly_cmodq(poly *a)
{
	int i;
	__m256i *pa = (__m256i *)a->coeffs;
	__m256i q8x = _mm256_set1_epi32(PARAM_Q);
	__m256i hq8x = _mm256_set1_epi32(PARAM_Q / 2);
	__m256i tmp0, tmp1;
	for (i = 0; i < PARAM_N / 8; ++i)
	{
		tmp0 = _mm256_loadu_si256(&pa[i]);
		tmp1 = _mm256_srai_epi32(tmp0, 31);
		tmp1 = _mm256_and_si256(tmp1, q8x);
		tmp0 = _mm256_add_epi32(tmp0, tmp1);
		tmp1 = _mm256_sub_epi32(hq8x, tmp0);
		tmp1 = _mm256_srai_epi32(tmp1, 31);
		tmp1 = _mm256_and_si256(tmp1, q8x);
		pa[i] = _mm256_sub_epi32(tmp0, tmp1);
	}
}


void poly_decompose(poly *r1, poly *r0, const poly *a)
{
	int i;
	__m256i *pa  = (__m256i *)a->coeffs;
	__m256i *pr0 = (__m256i *)r0->coeffs;
	__m256i *pr1 = (__m256i *)r1->coeffs;

	__m256i alpha8x = _mm256_set1_epi32(ALPHA);
	__m256i bound   = _mm256_set1_epi32(PARAM_Q / ALPHA);

#if ALPHA == 695296
	__m256i barrat_const8x = _mm256_set1_epi32(3);
	__m256i gamma2x8 = _mm256_set1_epi32(GAMMA2);
#elif ALPHA == 1042944
	__m256i gamma2x8 = _mm256_set1_epi32(GAMMA2);
#elif ALPHA == 347648
	__m256i div_const8x = _mm256_set1_epi32(6177);
	__m256i div_round8x = _mm256_set1_epi32(1 << 23);
	__m256i shift_round8x = _mm256_set1_epi32(127);
#elif ALPHA == 86912
	__m256i div_const8x = _mm256_set1_epi32(24709);
	__m256i div_round8x = _mm256_set1_epi32(16752312);
	__m256i shift_round8x = _mm256_set1_epi32(63);
#endif

	__m256i u, v, ta, t, t1;

	for (i = 0; i < PARAM_N / 8; ++i)
	{
		ta = _mm256_load_si256(&pa[i]);

#if ALPHA == 695296
		t  = _mm256_mullo_epi32(ta, barrat_const8x);
		t1 = _mm256_srli_epi32(t, 21);

		u = _mm256_mullo_epi32(t1, alpha8x);
		v = _mm256_sub_epi32(ta, u);
		t = _mm256_cmpgt_epi32(v, gamma2x8);
		t1 = _mm256_sub_epi32(t1, t);
		v = _mm256_and_si256(t, alpha8x);
		u = _mm256_add_epi32(u, v);

#elif ALPHA == 1042944

		t1 = _mm256_srli_epi32(ta, 20);

		u = _mm256_mullo_epi32(t1, alpha8x);
		v = _mm256_sub_epi32(ta, u);
		t = _mm256_cmpgt_epi32(v, gamma2x8);
		t1 = _mm256_sub_epi32(t1, t);
		v = _mm256_and_si256(t, alpha8x);
		u = _mm256_add_epi32(u, v);

#elif ALPHA == 347648

		t  = _mm256_add_epi32(ta, shift_round8x);
		t  = _mm256_srli_epi32(t, 7);
		t  = _mm256_mullo_epi32(t, div_const8x);
		t  = _mm256_add_epi32(t, div_round8x);
		t1 = _mm256_srli_epi32(t, 24);

		u = _mm256_mullo_epi32(t1, alpha8x);

#elif ALPHA == 86912

		t  = _mm256_add_epi32(ta, shift_round8x);
		t  = _mm256_srli_epi32(t, 6);
		t  = _mm256_mullo_epi32(t, div_const8x);
		t  = _mm256_add_epi32(t, div_round8x);
		t1 = _mm256_srli_epi32(t, 25);

		u = _mm256_mullo_epi32(t1, alpha8x);

#endif

		v = _mm256_cmpeq_epi32(t1, bound);
		t = _mm256_and_si256(t1, v);
		pr1[i] = _mm256_sub_epi32(t1, t);

		u = _mm256_sub_epi32(u, v);
		pr0[i] = _mm256_sub_epi32(ta, u);
	}
}

void poly_power2round(poly *r1, poly *r0, const poly *a)
{
	int i;
	__m256i t0, t1;
	__m256i *pa = (__m256i *)a->coeffs;
	__m256i *pr0 = (__m256i *)r0->coeffs;
	__m256i *pr1 = (__m256i *)r1->coeffs;
	const __m256i mask = _mm256_set1_epi32(-(1 << PARAM_D));
	const __m256i powerdm1m1 = _mm256_set1_epi32((1 << (PARAM_D - 1)) - 1);

	for (i = 0; i < PARAM_N / 8; ++i)
	{
		t0 = _mm256_load_si256(&pa[i]);
		t1 = _mm256_add_epi32(t0, powerdm1m1);
		pr1[i] = _mm256_srai_epi32(t1, PARAM_D);
		t1 = _mm256_and_si256(t1, mask);
		pr0[i] = _mm256_sub_epi32(t0, t1);
	}
}

void poly_add(poly *c, const poly *a, const poly *b)
{
	int i;

	__m256i *pa = (__m256i *)a->coeffs;
	__m256i *pb = (__m256i *)b->coeffs;
	__m256i *pc = (__m256i *)c->coeffs;

	for (i = 0; i < PARAM_N / 8; ++i)
		pc[i] = _mm256_add_epi32(pa[i], pb[i]);
}

void poly_sub(poly *c, const poly *a, const poly *b)
{
	int i;

	__m256i *pa = (__m256i *)a->coeffs;
	__m256i *pb = (__m256i *)b->coeffs;
	__m256i *pc = (__m256i *)c->coeffs;

	for (i = 0; i < PARAM_N / 8; ++i)
		pc[i] = _mm256_sub_epi32(pa[i], pb[i]);
}

void poly_subw(poly *c, const poly *a, const poly *w)
{
	int i;
	__m256i *pa = (__m256i *)a->coeffs;
	__m256i *pw = (__m256i *)w->coeffs;
	__m256i *pc = (__m256i *)c->coeffs;
	__m256i a8x = _mm256_set1_epi32(ALPHA);
	__m256i tmp;

	for (i = 0; i < PARAM_N / 8; ++i)
	{
		tmp = _mm256_mullo_epi32(pw[i], a8x);
		pc[i] = _mm256_sub_epi32(pa[i], tmp);
	}
}
void poly_shiftl(poly *a, int k)
{
	int i;
	__m256i *pa = (__m256i *)a->coeffs;
	for (i = 0; i < PARAM_N / 8; ++i)
		pa[i] = _mm256_slli_epi32(pa[i], k);
}

void poly_ntt(poly *a)
{
	ntt(a->coeffs);
}

void poly_invntt_montgomery(poly *a)
{
	invntt(a->coeffs);
}

void poly_pointwise_montgomery(poly *c, const poly *a, const poly *b)
{
	int i;
	__m256i *pa = (__m256i *)a->coeffs;
	__m256i *pb = (__m256i *)b->coeffs;
	__m256i *pc = (__m256i *)c->coeffs;
	__m256i q8x = _mm256_set1_epi32(PARAM_Q);
	__m256i qinv8x = _mm256_set1_epi32(QINV);
	__m256i r0, r1, t0, t1;

	for (i = 0; i < PARAM_N / 8; ++i)
	{
		// mul
		t0 = _mm256_load_si256(&pa[i]);
		t1 = _mm256_load_si256(&pb[i]);
		r0 = _mm256_mul_epi32(t0, t1);

		t0 = _mm256_srli_epi64(t0, 32);
		t1 = _mm256_srli_epi64(t1, 32);
		r1 = _mm256_mul_epi32(t0, t1);
		// reduce
		t0 = _mm256_mul_epi32(r0, qinv8x);
		t1 = _mm256_mul_epi32(r1, qinv8x);

		t0 = _mm256_mul_epi32(t0, q8x);
		t1 = _mm256_mul_epi32(t1, q8x);

		r0 = _mm256_sub_epi64(r0, t0);
		r1 = _mm256_sub_epi64(r1, t1);

		// store
		r0 = _mm256_srli_epi64(r0, 32);
		pc[i] = _mm256_blend_epi32(r0, r1, 0xAA);
	}
}

int poly_chknorm(const poly *a, uint32_t bound)
{
	int i;
	__m256i b, c;
	__m256i boundx8 = _mm256_set1_epi32(bound);
	__m256i onex8 = _mm256_set1_epi32(1);
	__m256i r = _mm256_setzero_si256();
	__m256i t;

	for (i = 0; i < PARAM_N / 8; ++i)
	{
		b = _mm256_load_si256((__m256i *)&a->coeffs[8 * i]);
		c = _mm256_srai_epi32(b, 31);
		c = _mm256_and_si256(c, onex8);
		b = _mm256_abs_epi32(b);
		b = _mm256_add_epi32(b,c);
		b = _mm256_cmpgt_epi32(b, boundx8);
		r = _mm256_or_si256(r, b);
	}
	return _mm256_movemask_epi8(r); // the value is non-zero if some a[i]> B or a[i] < -B + 1
}

static const uint64_t idx[256][4] = {
	{0x800000008, 0x800000008, 0x800000008, 0x800000008},
	{0x800000000, 0x800000008, 0x800000008, 0x800000008},
	{0x800000001, 0x800000008, 0x800000008, 0x800000008},
	{0x100000000, 0x800000008, 0x800000008, 0x800000008},
	{0x800000002, 0x800000008, 0x800000008, 0x800000008},
	{0x200000000, 0x800000008, 0x800000008, 0x800000008},
	{0x200000001, 0x800000008, 0x800000008, 0x800000008},
	{0x100000000, 0x800000002, 0x800000008, 0x800000008},
	{0x800000003, 0x800000008, 0x800000008, 0x800000008},
	{0x300000000, 0x800000008, 0x800000008, 0x800000008},
	{0x300000001, 0x800000008, 0x800000008, 0x800000008},
	{0x100000000, 0x800000003, 0x800000008, 0x800000008},
	{0x300000002, 0x800000008, 0x800000008, 0x800000008},
	{0x200000000, 0x800000003, 0x800000008, 0x800000008},
	{0x200000001, 0x800000003, 0x800000008, 0x800000008},
	{0x100000000, 0x300000002, 0x800000008, 0x800000008},
	{0x800000004, 0x800000008, 0x800000008, 0x800000008},
	{0x400000000, 0x800000008, 0x800000008, 0x800000008},
	{0x400000001, 0x800000008, 0x800000008, 0x800000008},
	{0x100000000, 0x800000004, 0x800000008, 0x800000008},
	{0x400000002, 0x800000008, 0x800000008, 0x800000008},
	{0x200000000, 0x800000004, 0x800000008, 0x800000008},
	{0x200000001, 0x800000004, 0x800000008, 0x800000008},
	{0x100000000, 0x400000002, 0x800000008, 0x800000008},
	{0x400000003, 0x800000008, 0x800000008, 0x800000008},
	{0x300000000, 0x800000004, 0x800000008, 0x800000008},
	{0x300000001, 0x800000004, 0x800000008, 0x800000008},
	{0x100000000, 0x400000003, 0x800000008, 0x800000008},
	{0x300000002, 0x800000004, 0x800000008, 0x800000008},
	{0x200000000, 0x400000003, 0x800000008, 0x800000008},
	{0x200000001, 0x400000003, 0x800000008, 0x800000008},
	{0x100000000, 0x300000002, 0x800000004, 0x800000008},
	{0x800000005, 0x800000008, 0x800000008, 0x800000008},
	{0x500000000, 0x800000008, 0x800000008, 0x800000008},
	{0x500000001, 0x800000008, 0x800000008, 0x800000008},
	{0x100000000, 0x800000005, 0x800000008, 0x800000008},
	{0x500000002, 0x800000008, 0x800000008, 0x800000008},
	{0x200000000, 0x800000005, 0x800000008, 0x800000008},
	{0x200000001, 0x800000005, 0x800000008, 0x800000008},
	{0x100000000, 0x500000002, 0x800000008, 0x800000008},
	{0x500000003, 0x800000008, 0x800000008, 0x800000008},
	{0x300000000, 0x800000005, 0x800000008, 0x800000008},
	{0x300000001, 0x800000005, 0x800000008, 0x800000008},
	{0x100000000, 0x500000003, 0x800000008, 0x800000008},
	{0x300000002, 0x800000005, 0x800000008, 0x800000008},
	{0x200000000, 0x500000003, 0x800000008, 0x800000008},
	{0x200000001, 0x500000003, 0x800000008, 0x800000008},
	{0x100000000, 0x300000002, 0x800000005, 0x800000008},
	{0x500000004, 0x800000008, 0x800000008, 0x800000008},
	{0x400000000, 0x800000005, 0x800000008, 0x800000008},
	{0x400000001, 0x800000005, 0x800000008, 0x800000008},
	{0x100000000, 0x500000004, 0x800000008, 0x800000008},
	{0x400000002, 0x800000005, 0x800000008, 0x800000008},
	{0x200000000, 0x500000004, 0x800000008, 0x800000008},
	{0x200000001, 0x500000004, 0x800000008, 0x800000008},
	{0x100000000, 0x400000002, 0x800000005, 0x800000008},
	{0x400000003, 0x800000005, 0x800000008, 0x800000008},
	{0x300000000, 0x500000004, 0x800000008, 0x800000008},
	{0x300000001, 0x500000004, 0x800000008, 0x800000008},
	{0x100000000, 0x400000003, 0x800000005, 0x800000008},
	{0x300000002, 0x500000004, 0x800000008, 0x800000008},
	{0x200000000, 0x400000003, 0x800000005, 0x800000008},
	{0x200000001, 0x400000003, 0x800000005, 0x800000008},
	{0x100000000, 0x300000002, 0x500000004, 0x800000008},
	{0x800000006, 0x800000008, 0x800000008, 0x800000008},
	{0x600000000, 0x800000008, 0x800000008, 0x800000008},
	{0x600000001, 0x800000008, 0x800000008, 0x800000008},
	{0x100000000, 0x800000006, 0x800000008, 0x800000008},
	{0x600000002, 0x800000008, 0x800000008, 0x800000008},
	{0x200000000, 0x800000006, 0x800000008, 0x800000008},
	{0x200000001, 0x800000006, 0x800000008, 0x800000008},
	{0x100000000, 0x600000002, 0x800000008, 0x800000008},
	{0x600000003, 0x800000008, 0x800000008, 0x800000008},
	{0x300000000, 0x800000006, 0x800000008, 0x800000008},
	{0x300000001, 0x800000006, 0x800000008, 0x800000008},
	{0x100000000, 0x600000003, 0x800000008, 0x800000008},
	{0x300000002, 0x800000006, 0x800000008, 0x800000008},
	{0x200000000, 0x600000003, 0x800000008, 0x800000008},
	{0x200000001, 0x600000003, 0x800000008, 0x800000008},
	{0x100000000, 0x300000002, 0x800000006, 0x800000008},
	{0x600000004, 0x800000008, 0x800000008, 0x800000008},
	{0x400000000, 0x800000006, 0x800000008, 0x800000008},
	{0x400000001, 0x800000006, 0x800000008, 0x800000008},
	{0x100000000, 0x600000004, 0x800000008, 0x800000008},
	{0x400000002, 0x800000006, 0x800000008, 0x800000008},
	{0x200000000, 0x600000004, 0x800000008, 0x800000008},
	{0x200000001, 0x600000004, 0x800000008, 0x800000008},
	{0x100000000, 0x400000002, 0x800000006, 0x800000008},
	{0x400000003, 0x800000006, 0x800000008, 0x800000008},
	{0x300000000, 0x600000004, 0x800000008, 0x800000008},
	{0x300000001, 0x600000004, 0x800000008, 0x800000008},
	{0x100000000, 0x400000003, 0x800000006, 0x800000008},
	{0x300000002, 0x600000004, 0x800000008, 0x800000008},
	{0x200000000, 0x400000003, 0x800000006, 0x800000008},
	{0x200000001, 0x400000003, 0x800000006, 0x800000008},
	{0x100000000, 0x300000002, 0x600000004, 0x800000008},
	{0x600000005, 0x800000008, 0x800000008, 0x800000008},
	{0x500000000, 0x800000006, 0x800000008, 0x800000008},
	{0x500000001, 0x800000006, 0x800000008, 0x800000008},
	{0x100000000, 0x600000005, 0x800000008, 0x800000008},
	{0x500000002, 0x800000006, 0x800000008, 0x800000008},
	{0x200000000, 0x600000005, 0x800000008, 0x800000008},
	{0x200000001, 0x600000005, 0x800000008, 0x800000008},
	{0x100000000, 0x500000002, 0x800000006, 0x800000008},
	{0x500000003, 0x800000006, 0x800000008, 0x800000008},
	{0x300000000, 0x600000005, 0x800000008, 0x800000008},
	{0x300000001, 0x600000005, 0x800000008, 0x800000008},
	{0x100000000, 0x500000003, 0x800000006, 0x800000008},
	{0x300000002, 0x600000005, 0x800000008, 0x800000008},
	{0x200000000, 0x500000003, 0x800000006, 0x800000008},
	{0x200000001, 0x500000003, 0x800000006, 0x800000008},
	{0x100000000, 0x300000002, 0x600000005, 0x800000008},
	{0x500000004, 0x800000006, 0x800000008, 0x800000008},
	{0x400000000, 0x600000005, 0x800000008, 0x800000008},
	{0x400000001, 0x600000005, 0x800000008, 0x800000008},
	{0x100000000, 0x500000004, 0x800000006, 0x800000008},
	{0x400000002, 0x600000005, 0x800000008, 0x800000008},
	{0x200000000, 0x500000004, 0x800000006, 0x800000008},
	{0x200000001, 0x500000004, 0x800000006, 0x800000008},
	{0x100000000, 0x400000002, 0x600000005, 0x800000008},
	{0x400000003, 0x600000005, 0x800000008, 0x800000008},
	{0x300000000, 0x500000004, 0x800000006, 0x800000008},
	{0x300000001, 0x500000004, 0x800000006, 0x800000008},
	{0x100000000, 0x400000003, 0x600000005, 0x800000008},
	{0x300000002, 0x500000004, 0x800000006, 0x800000008},
	{0x200000000, 0x400000003, 0x600000005, 0x800000008},
	{0x200000001, 0x400000003, 0x600000005, 0x800000008},
	{0x100000000, 0x300000002, 0x500000004, 0x800000006},
	{0x800000007, 0x800000008, 0x800000008, 0x800000008},
	{0x700000000, 0x800000008, 0x800000008, 0x800000008},
	{0x700000001, 0x800000008, 0x800000008, 0x800000008},
	{0x100000000, 0x800000007, 0x800000008, 0x800000008},
	{0x700000002, 0x800000008, 0x800000008, 0x800000008},
	{0x200000000, 0x800000007, 0x800000008, 0x800000008},
	{0x200000001, 0x800000007, 0x800000008, 0x800000008},
	{0x100000000, 0x700000002, 0x800000008, 0x800000008},
	{0x700000003, 0x800000008, 0x800000008, 0x800000008},
	{0x300000000, 0x800000007, 0x800000008, 0x800000008},
	{0x300000001, 0x800000007, 0x800000008, 0x800000008},
	{0x100000000, 0x700000003, 0x800000008, 0x800000008},
	{0x300000002, 0x800000007, 0x800000008, 0x800000008},
	{0x200000000, 0x700000003, 0x800000008, 0x800000008},
	{0x200000001, 0x700000003, 0x800000008, 0x800000008},
	{0x100000000, 0x300000002, 0x800000007, 0x800000008},
	{0x700000004, 0x800000008, 0x800000008, 0x800000008},
	{0x400000000, 0x800000007, 0x800000008, 0x800000008},
	{0x400000001, 0x800000007, 0x800000008, 0x800000008},
	{0x100000000, 0x700000004, 0x800000008, 0x800000008},
	{0x400000002, 0x800000007, 0x800000008, 0x800000008},
	{0x200000000, 0x700000004, 0x800000008, 0x800000008},
	{0x200000001, 0x700000004, 0x800000008, 0x800000008},
	{0x100000000, 0x400000002, 0x800000007, 0x800000008},
	{0x400000003, 0x800000007, 0x800000008, 0x800000008},
	{0x300000000, 0x700000004, 0x800000008, 0x800000008},
	{0x300000001, 0x700000004, 0x800000008, 0x800000008},
	{0x100000000, 0x400000003, 0x800000007, 0x800000008},
	{0x300000002, 0x700000004, 0x800000008, 0x800000008},
	{0x200000000, 0x400000003, 0x800000007, 0x800000008},
	{0x200000001, 0x400000003, 0x800000007, 0x800000008},
	{0x100000000, 0x300000002, 0x700000004, 0x800000008},
	{0x700000005, 0x800000008, 0x800000008, 0x800000008},
	{0x500000000, 0x800000007, 0x800000008, 0x800000008},
	{0x500000001, 0x800000007, 0x800000008, 0x800000008},
	{0x100000000, 0x700000005, 0x800000008, 0x800000008},
	{0x500000002, 0x800000007, 0x800000008, 0x800000008},
	{0x200000000, 0x700000005, 0x800000008, 0x800000008},
	{0x200000001, 0x700000005, 0x800000008, 0x800000008},
	{0x100000000, 0x500000002, 0x800000007, 0x800000008},
	{0x500000003, 0x800000007, 0x800000008, 0x800000008},
	{0x300000000, 0x700000005, 0x800000008, 0x800000008},
	{0x300000001, 0x700000005, 0x800000008, 0x800000008},
	{0x100000000, 0x500000003, 0x800000007, 0x800000008},
	{0x300000002, 0x700000005, 0x800000008, 0x800000008},
	{0x200000000, 0x500000003, 0x800000007, 0x800000008},
	{0x200000001, 0x500000003, 0x800000007, 0x800000008},
	{0x100000000, 0x300000002, 0x700000005, 0x800000008},
	{0x500000004, 0x800000007, 0x800000008, 0x800000008},
	{0x400000000, 0x700000005, 0x800000008, 0x800000008},
	{0x400000001, 0x700000005, 0x800000008, 0x800000008},
	{0x100000000, 0x500000004, 0x800000007, 0x800000008},
	{0x400000002, 0x700000005, 0x800000008, 0x800000008},
	{0x200000000, 0x500000004, 0x800000007, 0x800000008},
	{0x200000001, 0x500000004, 0x800000007, 0x800000008},
	{0x100000000, 0x400000002, 0x700000005, 0x800000008},
	{0x400000003, 0x700000005, 0x800000008, 0x800000008},
	{0x300000000, 0x500000004, 0x800000007, 0x800000008},
	{0x300000001, 0x500000004, 0x800000007, 0x800000008},
	{0x100000000, 0x400000003, 0x700000005, 0x800000008},
	{0x300000002, 0x500000004, 0x800000007, 0x800000008},
	{0x200000000, 0x400000003, 0x700000005, 0x800000008},
	{0x200000001, 0x400000003, 0x700000005, 0x800000008},
	{0x100000000, 0x300000002, 0x500000004, 0x800000007},
	{0x700000006, 0x800000008, 0x800000008, 0x800000008},
	{0x600000000, 0x800000007, 0x800000008, 0x800000008},
	{0x600000001, 0x800000007, 0x800000008, 0x800000008},
	{0x100000000, 0x700000006, 0x800000008, 0x800000008},
	{0x600000002, 0x800000007, 0x800000008, 0x800000008},
	{0x200000000, 0x700000006, 0x800000008, 0x800000008},
	{0x200000001, 0x700000006, 0x800000008, 0x800000008},
	{0x100000000, 0x600000002, 0x800000007, 0x800000008},
	{0x600000003, 0x800000007, 0x800000008, 0x800000008},
	{0x300000000, 0x700000006, 0x800000008, 0x800000008},
	{0x300000001, 0x700000006, 0x800000008, 0x800000008},
	{0x100000000, 0x600000003, 0x800000007, 0x800000008},
	{0x300000002, 0x700000006, 0x800000008, 0x800000008},
	{0x200000000, 0x600000003, 0x800000007, 0x800000008},
	{0x200000001, 0x600000003, 0x800000007, 0x800000008},
	{0x100000000, 0x300000002, 0x700000006, 0x800000008},
	{0x600000004, 0x800000007, 0x800000008, 0x800000008},
	{0x400000000, 0x700000006, 0x800000008, 0x800000008},
	{0x400000001, 0x700000006, 0x800000008, 0x800000008},
	{0x100000000, 0x600000004, 0x800000007, 0x800000008},
	{0x400000002, 0x700000006, 0x800000008, 0x800000008},
	{0x200000000, 0x600000004, 0x800000007, 0x800000008},
	{0x200000001, 0x600000004, 0x800000007, 0x800000008},
	{0x100000000, 0x400000002, 0x700000006, 0x800000008},
	{0x400000003, 0x700000006, 0x800000008, 0x800000008},
	{0x300000000, 0x600000004, 0x800000007, 0x800000008},
	{0x300000001, 0x600000004, 0x800000007, 0x800000008},
	{0x100000000, 0x400000003, 0x700000006, 0x800000008},
	{0x300000002, 0x600000004, 0x800000007, 0x800000008},
	{0x200000000, 0x400000003, 0x700000006, 0x800000008},
	{0x200000001, 0x400000003, 0x700000006, 0x800000008},
	{0x100000000, 0x300000002, 0x600000004, 0x800000007},
	{0x600000005, 0x800000007, 0x800000008, 0x800000008},
	{0x500000000, 0x700000006, 0x800000008, 0x800000008},
	{0x500000001, 0x700000006, 0x800000008, 0x800000008},
	{0x100000000, 0x600000005, 0x800000007, 0x800000008},
	{0x500000002, 0x700000006, 0x800000008, 0x800000008},
	{0x200000000, 0x600000005, 0x800000007, 0x800000008},
	{0x200000001, 0x600000005, 0x800000007, 0x800000008},
	{0x100000000, 0x500000002, 0x700000006, 0x800000008},
	{0x500000003, 0x700000006, 0x800000008, 0x800000008},
	{0x300000000, 0x600000005, 0x800000007, 0x800000008},
	{0x300000001, 0x600000005, 0x800000007, 0x800000008},
	{0x100000000, 0x500000003, 0x700000006, 0x800000008},
	{0x300000002, 0x600000005, 0x800000007, 0x800000008},
	{0x200000000, 0x500000003, 0x700000006, 0x800000008},
	{0x200000001, 0x500000003, 0x700000006, 0x800000008},
	{0x100000000, 0x300000002, 0x600000005, 0x800000007},
	{0x500000004, 0x700000006, 0x800000008, 0x800000008},
	{0x400000000, 0x600000005, 0x800000007, 0x800000008},
	{0x400000001, 0x600000005, 0x800000007, 0x800000008},
	{0x100000000, 0x500000004, 0x700000006, 0x800000008},
	{0x400000002, 0x600000005, 0x800000007, 0x800000008},
	{0x200000000, 0x500000004, 0x700000006, 0x800000008},
	{0x200000001, 0x500000004, 0x700000006, 0x800000008},
	{0x100000000, 0x400000002, 0x600000005, 0x800000007},
	{0x400000003, 0x600000005, 0x800000007, 0x800000008},
	{0x300000000, 0x500000004, 0x700000006, 0x800000008},
	{0x300000001, 0x500000004, 0x700000006, 0x800000008},
	{0x100000000, 0x400000003, 0x600000005, 0x800000007},
	{0x300000002, 0x500000004, 0x700000006, 0x800000008},
	{0x200000000, 0x400000003, 0x600000005, 0x800000007},
	{0x200000001, 0x400000003, 0x600000005, 0x800000007},
	{0x100000000, 0x300000002, 0x500000004, 0x700000006},
};
static const uint32_t popcount[256] = {
	0, 1, 1, 2, 1, 2, 2, 3,
	1, 2, 2, 3, 2, 3, 3, 4,
	1, 2, 2, 3, 2, 3, 3, 4,
	2, 3, 3, 4, 3, 4, 4, 5,
	1, 2, 2, 3, 2, 3, 3, 4,
	2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5,
	3, 4, 4, 5, 4, 5, 5, 6,
	1, 2, 2, 3, 2, 3, 3, 4,
	2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5,
	3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5,
	3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6,
	4, 5, 5, 6, 5, 6, 6, 7,
	1, 2, 2, 3, 2, 3, 3, 4,
	2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5,
	3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5,
	3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6,
	4, 5, 5, 6, 5, 6, 6, 7,
	2, 3, 3, 4, 3, 4, 4, 5,
	3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6,
	4, 5, 5, 6, 5, 6, 6, 7,
	3, 4, 4, 5, 4, 5, 5, 6,
	4, 5, 5, 6, 5, 6, 6, 7,
	4, 5, 5, 6, 5, 6, 6, 7,
	5, 6, 6, 7, 6, 7, 7, 8};



static int rej_eta1_ref(int32_t *a, int32_t *cur, int32_t n, const uint8_t *buf, int32_t blen)
{
#if ETA1 > 7
#error "rej_eta1() assumes 1<= ETA1 <= 7"
#endif
	int ctr, pos;
	int32_t t[4];

	ctr = *cur;
	pos = 0;

#if ETA1 == 1
	while (ctr < n && pos < blen)
	{
		t[0] = buf[pos] & 0x03;
		t[1] = (buf[pos] >> 2) & 0x03;
		t[2] = (buf[pos] >> 4) & 0x03;
		t[3] = (buf[pos++] >> 6) & 0x03;

		if (t[0] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[0];
		if (ctr < n && t[1] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[1];
		if (ctr < n && t[2] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[2];
		if (ctr < n && t[3] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[3];
	}
#elif ETA1 <= 3
	while (ctr < n && pos < blen)
	{
		t[0] = buf[pos] & 0x07;
		t[1] = (buf[pos] >> 3) & 0x07;
		if (t[0] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[0];
		if (ctr < n && t[1] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[1];
		if (ctr >= n || pos + 2 > blen)
		{
			*cur = ctr;
			return pos + 1;
		}

		t[0] = (buf[pos] >> 6) | ((buf[pos + 1] & 0x1) << 2);
		t[1] = (buf[++pos] >> 1) & 0x07;
		t[2] = (buf[pos] >> 4) & 0x07;

		if (t[0] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[0];
		if (ctr < n && t[1] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[1];
		if (ctr < n && t[2] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[2];
		if (ctr >= n || pos + 2 > blen)
		{
			*cur = ctr;
			return pos + 1;
		}

		t[0] = (buf[pos] >> 7) | ((buf[pos + 1] & 0x3) << 1);
		t[1] = (buf[++pos] >> 2) & 0x07;
		t[2] = buf[pos++] >> 5;

		if (t[0] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[0];
		if (ctr < n && t[1] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[1];
		if (ctr < n && t[2] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[2];
	}
#elif ETA1 >= 4
	while (ctr < n && pos < blen)
	{
		t[0] = buf[pos] & 0x0f;
		t[1] = (buf[pos++] >> 4) & 0x0f;
		if (t[0] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[0];
		if (ctr < n && t[1] <= 2 * ETA2)
			a[ctr++] = ETA1 - t[1];
	}
#endif
	*cur = ctr;
	return pos;
}
static int rej_eta1_avx(int32_t *a, int32_t *cur, int32_t n, const uint8_t *buf, int32_t blen)
{
#if ETA1 > 7
#error "rej_eta1() assumes 1<= ETA1 <= 7"
#endif
	int ctr, pos = 0, offset;
	uint32_t t;
	__m256i tmp, index;
	__m256i teta8x = _mm256_set1_epi32(2 * ETA1 + 1);
	__m256i eta8x = _mm256_set1_epi32(ETA1);
#if SETA1BITS == 2
	__m128i tmp0, index0;
	const __m256i shift = _mm256_set_epi32(14, 12, 10, 8, 6, 4, 2, 0);
	__m256i mask = _mm256_set1_epi32(0x3);
#elif SETA1BITS == 3
	const __m256i shift = _mm256_set_epi32(21, 18, 15, 12, 9, 6, 3, 0);
	__m256i mask = _mm256_set1_epi32(0x7);
#elif SETA1BITS == 4
	const __m256i shift = _mm256_set_epi32(28, 24, 20, 16, 12, 8, 4, 0);
	__m256i mask = _mm256_set1_epi32(0xf);
#endif

	ctr = *cur;
	while (pos + SETA1BITS <= blen && ctr + 8 <= n)
	{
		tmp = _mm256_set1_epi32(load_to32(&buf[pos], SETA1BITS));
		tmp = _mm256_srlv_epi32(tmp, shift);
		tmp = _mm256_and_si256(tmp, mask);

		index = _mm256_cmpgt_epi32(teta8x, tmp);
		t = _mm256_movemask_ps(_mm256_castsi256_ps(index));

		index = _mm256_loadu_si256((__m256i *)idx[t]);
		tmp = _mm256_permutevar8x32_epi32(tmp, index);
		tmp = _mm256_sub_epi32(eta8x, tmp);
		_mm256_storeu_si256((__m256i *)&a[ctr], tmp);

		offset = popcount[t];

		ctr += offset;
		pos += SETA1BITS;
	}
#if SETA1BITS == 2
	while (pos + SETA1BITS / 2 < blen && ctr + 4 <= n)
	{
		tmp0 = _mm_set1_epi32(buf[pos]);
		tmp0 = _mm_srlv_epi32(tmp0, _mm256_castsi256_si128(shift));
		tmp0 = _mm_and_si128(tmp0, _mm256_castsi256_si128(mask));

		index0 = _mm_cmpgt_epi32(_mm256_castsi256_si128(teta8x), tmp0);
		t = _mm_movemask_ps(_mm_castsi128_ps(index0));
		index0 = _mm_loadu_si128((__m128i *)idx[t]);
		tmp0 = _mm_castps_si128(_mm_permutevar_ps(_mm_castsi128_ps(tmp0), index0));
		tmp0 = _mm_sub_epi32(_mm256_castsi256_si128(eta8x), tmp0);
		_mm_storeu_si128((__m128i *)&a[ctr], tmp0);
		offset = popcount[t];
		ctr += offset;
		pos += SETA1BITS / 2;
	}
#endif
	*cur = ctr;
	return pos;
}
static int rej_eta2_ref(int32_t *a, int32_t *cur, int32_t n, const uint8_t *buf, int32_t blen)
{
	int ctr, pos;
	int32_t t[4];

	ctr = *cur;
	pos = 0;

#if ETA2 == 1
	while (ctr < n && pos < blen)
	{
		t[0] = buf[pos] & 0x03;
		t[1] = (buf[pos] >> 2) & 0x03;
		t[2] = (buf[pos] >> 4) & 0x03;
		t[3] = (buf[pos++] >> 6) & 0x03;

		if (t[0] <= 2 * ETA2)
			a[ctr++] = ETA2 - t[0];
		if (ctr < n && t[1] <= 2 * ETA2)
			a[ctr++] = ETA2 - t[1];
		if (ctr < n && t[2] <= 2 * ETA2)
			a[ctr++] = ETA2 - t[2];
		if (ctr < n && t[3] <= 2 * ETA2)
			a[ctr++] = ETA2 - t[3];
	}
#elif ETA2 <= 3
	while (ctr < n && pos < blen)
	{
		t[0] = buf[pos] & 0x07;
		t[1] = (buf[pos] >> 3) & 0x07;
		if (t[0] <= 2 * ETA1)
			a[ctr++] = ETA2 - t[0];
		if (ctr < n && t[1] <= 2 * ETA2)
			a[ctr++] = ETA2 - t[1];
		if (ctr >= n || pos + 2 > blen)
		{
			*cur = ctr;
			return pos + 1;
		}

		t[0] = (buf[pos] >> 6) | ((buf[pos + 1] & 0x1) << 2);
		t[1] = (buf[++pos] >> 1) & 0x07;
		t[2] = (buf[pos] >> 4) & 0x07;

		if (t[0] <= 2 * ETA2)
			a[ctr++] = ETA2 - t[0];
		if (ctr < n && t[1] <= 2 * ETA2)
			a[ctr++] = ETA2 - t[1];
		if (ctr < n && t[2] <= 2 * ETA2)
			a[ctr++] = ETA2 - t[2];
		if (ctr >= n || pos + 2 > blen)
		{
			*cur = ctr;
			return pos + 1;
		}

		t[0] = (buf[pos] >> 7) | ((buf[pos + 1] & 0x3) << 1);
		t[1] = (buf[++pos] >> 2) & 0x07;
		t[2] = buf[pos++] >> 5;

		if (t[0] <= 2 * ETA2)
			a[ctr++] = ETA2 - t[0];
		if (ctr < n && t[1] <= 2 * ETA2)
			a[ctr++] = ETA2 - t[1];
		if (ctr < n && t[2] <= 2 * ETA2)
			a[ctr++] = ETA2 - t[2];
	}
#elif ETA2 >= 4
	while (ctr < n && pos < blen)
	{
		t[0] = buf[pos] & 0x0f;
		t[1] = (buf[pos++] >> 4) & 0x0f;
		if (t[0] <= 2 * ETA2)
			a[ctr++] = ETA2 - t[0];
		if (ctr < n && t[1] <= 2 * ETA2)
			a[ctr++] = ETA2 - t[1];
	}
#endif
	*cur = ctr;
	return pos;
}
static int rej_eta2_avx(int32_t *a, int32_t *cur, int32_t n, const uint8_t *buf, int32_t blen)
{

	unsigned int ctr, pos, offset;
	uint32_t t;
	__m256i tmp, index;
	__m256i teta8x = _mm256_set1_epi32(2 * ETA2 + 1);
	__m256i eta8x = _mm256_set1_epi32(ETA2);
#if SETA2BITS == 2
	__m128i tmp0, index0;
	const __m256i shift = _mm256_set_epi32(14, 12, 10, 8, 6, 4, 2, 0);
	__m256i mask = _mm256_set1_epi32(0x3);
#elif SETA2BITS == 3
	const __m256i shift = _mm256_set_epi32(21, 18, 15, 12, 9, 6, 3, 0);
	__m256i mask = _mm256_set1_epi32(0x7);
#elif SETA2BITS == 4
	const __m256i shift = _mm256_set_epi32(28, 24, 20, 16, 12, 8, 4, 0);
	__m256i mask = _mm256_set1_epi32(0xf);
#endif

	ctr = *cur;
	pos = 0;
	while (pos + SETA2BITS <= blen && ctr + 8 <= n)
	{
		tmp = _mm256_set1_epi32(load_to32(&buf[pos], SETA2BITS));
		tmp = _mm256_srlv_epi32(tmp, shift);
		tmp = _mm256_and_si256(tmp, mask);

		index = _mm256_cmpgt_epi32(teta8x, tmp);
		t = _mm256_movemask_ps(_mm256_castsi256_ps(index));

		index = _mm256_loadu_si256((__m256i *)idx[t]);
		tmp = _mm256_permutevar8x32_epi32(tmp, index);
		tmp = _mm256_sub_epi32(eta8x, tmp);
		_mm256_storeu_si256((__m256i *)&a[ctr], tmp);

		offset = popcount[t];

		ctr += offset;
		pos += SETA2BITS;
	}
#if SETA2BITS == 2
	while (pos + SETA2BITS / 2 < blen && ctr + 4 <= n)
	{
		tmp0 = _mm_set1_epi32(buf[pos]);
		tmp0 = _mm_srlv_epi32(tmp0, _mm256_castsi256_si128(shift));
		tmp0 = _mm_and_si128(tmp0, _mm256_castsi256_si128(mask));

		index0 = _mm_cmpgt_epi32(_mm256_castsi256_si128(teta8x), tmp0);
		t = _mm_movemask_ps(_mm_castsi128_ps(index0));
		index0 = _mm_loadu_si128((__m128i *)idx[t]);
		tmp0 = _mm_castps_si128(_mm_permutevar_ps(_mm_castsi128_ps(tmp0), index0));
		tmp0 = _mm_sub_epi32(_mm256_castsi256_si128(eta8x), tmp0);
		_mm_storeu_si128((__m128i *)&a[ctr], tmp0);
		offset = popcount[t];
		ctr += offset;
		pos += SETA2BITS / 2;
	}
#endif
	*cur = ctr;
	return pos;
}

static int32_t rej_eta_1(int32_t *a, int32_t *cur, int32_t n, const uint8_t *buf, int32_t blen)
{
	int32_t ctr, pos;
	int32_t t[8];

	ctr = *cur;
	pos = 0;

	while (pos + 2 <= blen && ctr + 8 <= n)
	{
		t[0] = buf[pos] & 0x03;
		t[1] = (buf[pos] >> 2) & 0x03;
		t[2] = (buf[pos] >> 4) & 0x03;
		t[3] = (buf[pos++] >> 6) & 0x03;
		t[4] = buf[pos] & 0x03;
		t[5] = (buf[pos] >> 2) & 0x03;
		t[6] = (buf[pos] >> 4) & 0x03;
		t[7] = (buf[pos++] >> 6) & 0x03;

		if (t[0] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[0];
		if (t[1] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[1];
		if (t[2] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[2];
		if (t[3] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[3];
		if (t[4] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[4];
		if (t[5] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[5];
		if (t[6] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[6];
		if (t[7] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[7];
	}

	while (pos < blen && ctr < n)
	{
		t[0] = buf[pos] & 0x03;
		t[1] = (buf[pos] >> 2) & 0x03;
		t[2] = (buf[pos] >> 4) & 0x03;
		t[3] = (buf[pos++] >> 6) & 0x03;

		if (t[0] <= 2 * ETA1)
			a[ctr++] = ETA1 - t[0];
		if (t[1] <= 2 * ETA1 && ctr < n)
			a[ctr++] = ETA1 - t[1];
		if (t[2] <= 2 * ETA1 && ctr < n)
			a[ctr++] = ETA1 - t[2];
		if (t[3] <= 2 * ETA1 && ctr < n)
			a[ctr++] = ETA1 - t[3];
	}
	*cur = ctr;
	return pos;
}

#ifdef USE_ICCS

void poly_uniform_eta1_seed(poly *a, const uint8_t *seed, int32_t seedbytes)
{
	uint8_t i;
	int32_t cur = 0, pos, step;
	int32_t nblock = (REJ_ETA1_BYTES + KDF_RATE - 1) / KDF_RATE;
	uint8_t outbuf[nblock * KDF_RATE];
	int32_t len;

	kdfstate ctx;
	kdf_init(&ctx, seed, seedbytes);
	kdf_squeezeblocks(outbuf, nblock, &ctx);

	len = nblock * KDF_RATE;

	rej_eta1_avx(a->coeffs, &cur, PARAM_N, outbuf, len);

	while (cur < PARAM_N)
	{
		kdf_squeezeblocks(outbuf, 1, &ctx);
		rej_eta_1(a->coeffs, &cur, PARAM_N, outbuf, KDF_RATE);
	}
}
#else
void poly_uniform_eta1_seed(poly *a, const uint8_t *seed, int32_t seedbytes)
{
	uint8_t i;
	int32_t cur = 0, pos, step;
	int32_t nblock = (REJ_ETA1_BYTES + KDF_RATE - 1) / KDF_RATE;
	ALIGN(32)
	uint8_t outbuf[nblock * KDF_RATE];
	int32_t len;

	kdfstate ctx;
	KDF_ABSORB(&ctx, seed, seedbytes);
	KDF_SQUEEZEBLOCK(outbuf, nblock, &ctx);
	len = nblock * KDF_RATE;

	pos = rej_eta1_avx(a->coeffs, &cur, PARAM_N, outbuf, len);

	while (cur < PARAM_N)
	{
		len = len - pos;
		memcpy(outbuf, outbuf + pos, len);
		KDF_SQUEEZEBLOCK(outbuf + len, 1, &ctx);
		rej_eta_1(a->coeffs, &cur, PARAM_N, outbuf + len, KDF_RATE);
	}
}
#endif

int poly_uniform_eta1(poly *a, const uint8_t *buf, int32_t buflen)
{
	int cur = 0, pos;
	pos = rej_eta1_avx(a->coeffs, &cur, PARAM_N, buf, buflen);
	buflen -= pos;
	if (cur < PARAM_N && ((PARAM_N - cur) * SETA1BITS + 7) / 8 <= buflen)
		rej_eta1_ref(a->coeffs, &cur, PARAM_N, &buf[pos], buflen);
	return cur;
}

int poly_uniform_eta2(poly *a, const uint8_t *buf, int32_t buflen)
{
	int cur = 0, pos;
	pos = rej_eta2_avx(a->coeffs, &cur, PARAM_N, buf, buflen);
	buflen -= pos;
	if (cur < PARAM_N && ((PARAM_N - cur) * SETA2BITS + 7) / 8 <= buflen)
		rej_eta2_ref(a->coeffs, &cur, PARAM_N, &buf[pos], buflen);
	return cur;
}

#ifdef USE_ICCS

void poly_uniform_eta2_seed(poly *a, const uint8_t *seed, int32_t seedbytes)
{
	int cur = 0, pos, step;
	ALIGN(32)
	uint8_t buf[REJ_ETA2_BYTES + KDF_RATE];
	int nblock = (REJ_ETA2_BYTES + KDF_RATE - 1) / KDF_RATE;
	int len;
	kdfstate state;
	kdf_init(&state, seed, seedbytes);
	kdf_squeezeblocks(buf, nblock, &state);
	len = nblock * KDF_RATE;
	pos = rej_eta2_avx(a->coeffs, &cur, PARAM_N, buf, len);

	while (cur < PARAM_N)
	{
		len = len - pos;
		memcpy(buf, buf + pos, len);
		kdf_squeezeblocks(buf + len, 1, &state);
		len += KDF128RATE;
		pos = rej_eta2_ref(a->coeffs, &cur, PARAM_N, buf, KDF_RATE);
	}
}

#else

void poly_uniform_eta2_seed(poly *a, const uint8_t *seed, int32_t seedbytes)
{
	int cur = 0, pos, step;
	ALIGN(32)
	uint8_t buf[REJ_ETA2_BYTES + KDF_RATE];
	int nblock = (REJ_ETA2_BYTES + KDF_RATE - 1) / KDF_RATE;
	int len;
	kdfstate state;
	KDF_ABSORB(&state, seed, seedbytes);
	KDF_SQUEEZEBLOCK(buf, nblock, &state);
	len = nblock * KDF_RATE;
	pos = rej_eta2_avx(a->coeffs, &cur, PARAM_N, buf, len);

	while (cur < PARAM_N)
	{
		len = len - pos;
		memcpy(buf, buf + pos, len);
		KDF_SQUEEZEBLOCK(buf + len, 1, &state);
		len += KDF128RATE;
		pos = rej_eta2_ref(a->coeffs, &cur, PARAM_N, buf, KDF_RATE);
	}
}

#endif
void poly_uniform_gamma1(poly *a, const uint8_t seed[SEEDBYTES + CRHBYTES], uint16_t nonce)
{
	int i;
	uint8_t inbuf[SEEDBYTES + CRHBYTES + 2];
	uint8_t outbuf[SZBITS * PARAM_N / 8];

	for (i = 0; i < SEEDBYTES + CRHBYTES; ++i)
		inbuf[i] = seed[i];
	inbuf[SEEDBYTES + CRHBYTES] = nonce & 0xFF;
	inbuf[SEEDBYTES + CRHBYTES + 1] = nonce >> 8;

	KDF(outbuf, sizeof(outbuf), inbuf, SEEDBYTES + CRHBYTES + 2);

	polyz_unpack(a, outbuf);
}

void polyeta1_pack(uint8_t r[POLETA1_SIZE_PACKED + 1], const poly *a)
{
#if ETA1 > 7
#error "polyeta1_pack() assumes 1 <= ETA1 <= 7"
#endif
	int i;
	__m256i d0, d1, d2, d3;
	const __m256i eta1 = _mm256_set1_epi8(ETA1);
#if SETA1BITS == 2
	__m128i t;
	const __m256i lomask16 = _mm256_set1_epi16(0x3);
	const __m256i himask16 = _mm256_set1_epi16(0x3 << 8);
	const __m256i lomask32 = _mm256_set1_epi32(0xf);
	const __m256i himask32 = _mm256_set1_epi32(0xf << 16);
	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
										 12, 15, 8, 15, 4, 15, 0, 15,
										 15, 15, 15, 15, 15, 15, 15, 15,
										 15, 12, 15, 8, 15, 4, 15, 0);

	for (i = 0; i < PARAM_N / 32; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i]);
		d1 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 8]);
		d2 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 16]);
		d3 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 24]);

		d0 = _mm256_packs_epi32(d0, d1);
		d1 = _mm256_packs_epi32(d2, d3);
		d0 = _mm256_packs_epi16(d0, d1);

		d0 = _mm256_sub_epi8(eta1, d0);

		d1 = _mm256_and_si256(d0, himask16);
		d0 = _mm256_and_si256(d0, lomask16);
		d1 = _mm256_srli_epi16(d1, 6);
		d0 = _mm256_or_si256(d0, d1);

		d1 = _mm256_and_si256(d0, himask32);
		d0 = _mm256_and_si256(d0, lomask32);
		d1 = _mm256_srli_epi32(d1, 12);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_shuffle_epi8(d1, idx8);

		t = _mm256_extracti128_si256(d0, 1);
		t = _mm_or_si128(_mm256_castsi256_si128(d0), t);
		_mm_storel_epi64((__m128i *)&r[8 * i], t);
	}
#elif SETA1BITS == 3
	const __m256i zero = _mm256_setzero_si256();
	const __m256i permu = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);
	const __m256i permu2 = _mm256_set_epi32(7, 7, 7, 7, 6, 4, 2, 0);
	const __m256i lomask16 = _mm256_set1_epi16(0x7);
	const __m256i himask16 = _mm256_set1_epi16(0x7 << 8);

	const __m256i lomask32 = _mm256_set1_epi32(0x3f);
	const __m256i himask32 = _mm256_set1_epi32(0x3f << 16);

	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 14, 13, 12, 10,
		9, 8, 6, 5, 4, 2, 1, 0);

	for (i = 0; i < PARAM_N / 32; i++)
	{
		d0 = _mm256_loadu_si256((__m256i*) & a->coeffs[32 * i]);
		d1 = _mm256_loadu_si256((__m256i*) & a->coeffs[32 * i + 8]);
		d2 = _mm256_loadu_si256((__m256i*) & a->coeffs[32 * i + 16]);
		d3 = _mm256_loadu_si256((__m256i*) & a->coeffs[32 * i + 24]);
		d0 = _mm256_packs_epi32(d0, d1);
		d1 = _mm256_packs_epi32(d2, d3);
		d0 = _mm256_packs_epi16(d0, d1);

		d0 = _mm256_permutevar8x32_epi32(d0, permu);
		d0 = _mm256_sub_epi8(eta1, d0);

		d1 = _mm256_and_si256(d0, himask16);
		d0 = _mm256_and_si256(d0, lomask16);
		d1 = _mm256_srli_epi16(d1, 5);
		d0 = _mm256_or_si256(d0, d1);

		d1 = _mm256_and_si256(d0, himask32);
		d0 = _mm256_and_si256(d0, lomask32);
		d1 = _mm256_srli_epi32(d1, 10);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_blend_epi32(d1, zero, 0xAA);
		d1 = _mm256_blend_epi32(zero, d1, 0xAA);
		d1 = _mm256_srli_epi64(d1, 20);
		d1 = _mm256_or_si256(d0, d1);

		d1 = _mm256_permutevar8x32_epi32(d1, permu2);
		d0 = _mm256_shuffle_epi8(d1, idx8);

		_mm_storeu_si128((__m128i*) & r[12 * i], _mm256_castsi256_si128(d0));
	}
#elif SETA1BITS == 4
	const __m256i zero = _mm256_setzero_si256();
	const __m256i permu = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);
	const __m256i permu2 = _mm256_set_epi32(3, 3, 3, 3, 7, 6, 3, 3);
	const __m256i lomask16 = _mm256_set1_epi16(0xf);
	const __m256i himask16 = _mm256_set1_epi16(0xf << 8);
	const __m256i idx8 = _mm256_set_epi8(14, 12, 10, 8, 6, 4, 2, 0,
										 15, 15, 15, 15, 15, 15, 15, 15,
										 15, 15, 15, 15, 15, 15, 15, 15,
										 14, 12, 10, 8, 6, 4, 2, 0);

	for (i = 0; i < PARAM_N / 32; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i]);
		d1 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 8]);
		d2 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 16]);
		d3 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 24]);
		d0 = _mm256_packs_epi32(d0, d1);
		d1 = _mm256_packs_epi32(d2, d3);
		d0 = _mm256_packs_epi16(d0, d1);

		d0 = _mm256_permutevar8x32_epi32(d0, permu);
		d0 = _mm256_sub_epi8(eta1, d0);

		d1 = _mm256_and_si256(d0, himask16);
		d0 = _mm256_and_si256(d0, lomask16);
		d1 = _mm256_srli_epi16(d1, 4);
		d0 = _mm256_or_si256(d0, d1);

		d0 = _mm256_shuffle_epi8(d0, idx8);
		d1 = _mm256_permutevar8x32_epi32(d0, permu2);
		d0 = _mm256_or_si256(d0, d1);

		_mm_storeu_si128((__m128i *)&r[16 * i], _mm256_castsi256_si128(d0));
	}
#endif
}
void polyeta2_pack(unsigned char r[POLETA2_SIZE_PACKED + 1], const poly *a)
{
#if ETA2 > 7
#error "polyeta2_pack() assumes ETA2 <= 7"
#endif
	int i;
	__m256i d0, d1, d2, d3;
	const __m256i eta2 = _mm256_set1_epi8(ETA2);
#if SETA2BITS == 2
	__m128i t;
	const __m256i lomask16 = _mm256_set1_epi16(0x3);
	const __m256i himask16 = _mm256_set1_epi16(0x3 << 8);
	const __m256i lomask32 = _mm256_set1_epi32(0xf);
	const __m256i himask32 = _mm256_set1_epi32(0xf << 16);
	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
										 12, 15, 8, 15, 4, 15, 0, 15,
										 15, 15, 15, 15, 15, 15, 15, 15,
										 15, 12, 15, 8, 15, 4, 15, 0);

	for (i = 0; i < PARAM_N / 32; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i]);
		d1 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 8]);
		d2 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 16]);
		d3 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 24]);

		d0 = _mm256_packs_epi32(d0, d1);
		d1 = _mm256_packs_epi32(d2, d3);
		d0 = _mm256_packs_epi16(d0, d1);

		d0 = _mm256_sub_epi8(eta2, d0);

		d1 = _mm256_and_si256(d0, himask16);
		d0 = _mm256_and_si256(d0, lomask16);
		d1 = _mm256_srli_epi16(d1, 6);
		d0 = _mm256_or_si256(d0, d1);

		d1 = _mm256_and_si256(d0, himask32);
		d0 = _mm256_and_si256(d0, lomask32);
		d1 = _mm256_srli_epi32(d1, 12);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_shuffle_epi8(d1, idx8);

		t = _mm256_extracti128_si256(d0, 1);
		t = _mm_or_si128(_mm256_castsi256_si128(d0), t);
		_mm_storel_epi64((__m128i *)&r[8 * i], t);
	}
#elif SETA2BITS == 3
	const __m256i zero = _mm256_setzero_si256();
	const __m256i permu = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);
	const __m256i permu2 = _mm256_set_epi32(7, 7, 7, 7, 6, 4, 2, 0);
	const __m256i lomask16 = _mm256_set1_epi16(0x7);
	const __m256i himask16 = _mm256_set1_epi16(0x7 << 8);

	const __m256i lomask32 = _mm256_set1_epi32(0x3f);
	const __m256i himask32 = _mm256_set1_epi32(0x3f << 16);

	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 14, 13, 12, 10,
		9, 8, 6, 5, 4, 2, 1, 0);

	for (i = 0; i < PARAM_N / 32; i++)
	{
		d0 = _mm256_loadu_si256((__m256i*) & a->coeffs[32 * i]);
		d1 = _mm256_loadu_si256((__m256i*) & a->coeffs[32 * i + 8]);
		d2 = _mm256_loadu_si256((__m256i*) & a->coeffs[32 * i + 16]);
		d3 = _mm256_loadu_si256((__m256i*) & a->coeffs[32 * i + 24]);
		d0 = _mm256_packs_epi32(d0, d1);
		d1 = _mm256_packs_epi32(d2, d3);
		d0 = _mm256_packs_epi16(d0, d1);

		d0 = _mm256_permutevar8x32_epi32(d0, permu);
		d0 = _mm256_sub_epi8(eta2, d0);

		d1 = _mm256_and_si256(d0, himask16);
		d0 = _mm256_and_si256(d0, lomask16);
		d1 = _mm256_srli_epi16(d1, 5);
		d0 = _mm256_or_si256(d0, d1);

		d1 = _mm256_and_si256(d0, himask32);
		d0 = _mm256_and_si256(d0, lomask32);
		d1 = _mm256_srli_epi32(d1, 10);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_blend_epi32(d1, zero, 0xAA);
		d1 = _mm256_blend_epi32(zero, d1, 0xAA);
		d1 = _mm256_srli_epi64(d1, 20);
		d1 = _mm256_or_si256(d0, d1);

		d1 = _mm256_permutevar8x32_epi32(d1, permu2);
		d0 = _mm256_shuffle_epi8(d1, idx8);

		_mm_storeu_si128((__m128i*) & r[12 * i], _mm256_castsi256_si128(d0));
	}
#elif SETA2BITS == 4
	const __m256i permu = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);
	const __m256i permu2 = _mm256_set_epi32(3, 3, 3, 3, 7, 6, 3, 3);
	const __m256i lomask16 = _mm256_set1_epi16(0xf);
	const __m256i himask16 = _mm256_set1_epi16(0xf << 8);
	const __m256i idx8 = _mm256_set_epi8(14, 12, 10, 8, 6, 4, 2, 0,
										 15, 15, 15, 15, 15, 15, 15, 15,
										 15, 15, 15, 15, 15, 15, 15, 15,
										 14, 12, 10, 8, 6, 4, 2, 0);

	for (i = 0; i < PARAM_N / 32; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i]);
		d1 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 8]);
		d2 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 16]);
		d3 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 24]);
		d0 = _mm256_packs_epi32(d0, d1);
		d1 = _mm256_packs_epi32(d2, d3);
		d0 = _mm256_packs_epi16(d0, d1);

		d0 = _mm256_permutevar8x32_epi32(d0, permu);
		d0 = _mm256_sub_epi8(eta2, d0);

		d1 = _mm256_and_si256(d0, himask16);
		d0 = _mm256_and_si256(d0, lomask16);
		d1 = _mm256_srli_epi16(d1, 4);
		d0 = _mm256_or_si256(d0, d1);

		d0 = _mm256_shuffle_epi8(d0, idx8);
		d1 = _mm256_permutevar8x32_epi32(d0, permu2);
		d0 = _mm256_or_si256(d0, d1);

		_mm_storeu_si128((__m128i *)&r[16 * i], _mm256_castsi256_si128(d0));
	}
#endif
}

void polyeta1_unpack(poly *r, const uint8_t *a)
{
#if ETA1 > 7
#error "polyeta1_unpack() assumes 1 <= ETA1 <= 7"
#endif

	int i;
	unsigned int pos;
	__m256i tmp;
	__m256i eta8x = _mm256_set1_epi32(ETA1);
#if SETA1BITS == 2
	__m256i mask = _mm256_set1_epi32(0x3);
	const __m256i shift = _mm256_set_epi32(14, 12, 10, 8, 6, 4, 2, 0);
#elif SETA1BITS == 3
	__m256i mask = _mm256_set1_epi32(0x7);
	const __m256i shift = _mm256_set_epi32(21, 18, 15, 12, 9, 6, 3, 0);
#elif SETA1BITS == 4
	__m256i mask = _mm256_set1_epi32(0xf);
	const __m256i shift = _mm256_set_epi32(28, 24, 20, 16, 12, 8, 4, 0);
#endif

	pos = 0;
	for (i = 0; i < PARAM_N; i += 8)
	{
		tmp = _mm256_set1_epi32(load_to32(&a[pos], SETA1BITS));
		tmp = _mm256_srlv_epi32(tmp, shift);
		tmp = _mm256_and_si256(tmp, mask);
		tmp = _mm256_sub_epi32(eta8x, tmp);
		_mm256_store_si256((__m256i *)&r->coeffs[i], tmp);
		pos += SETA1BITS;
	}
}
void polyeta2_unpack(poly *r, const uint8_t *a)
{
#if ETA2 > 7
#error "polyeta2_unpack() assumes ETA2 <= 7"
#endif
	int i;
	unsigned int pos;
	__m256i tmp;
	__m256i eta8x = _mm256_set1_epi32(ETA2);
#if SETA2BITS == 2
	__m256i mask = _mm256_set1_epi32(0x3);
	const __m256i shift = _mm256_set_epi32(14, 12, 10, 8, 6, 4, 2, 0);
#elif SETA2BITS == 3
	__m256i mask = _mm256_set1_epi32(0x7);
	const __m256i shift = _mm256_set_epi32(21, 18, 15, 12, 9, 6, 3, 0);
#elif SETA2BITS == 4
	__m256i mask = _mm256_set1_epi32(0xf);
	const __m256i shift = _mm256_set_epi32(28, 24, 20, 16, 12, 8, 4, 0);
#endif

	pos = 0;
	for (i = 0; i < PARAM_N; i += 8)
	{
		tmp = _mm256_set1_epi32(load_to32(&a[pos], SETA2BITS));
		tmp = _mm256_srlv_epi32(tmp, shift);
		tmp = _mm256_and_si256(tmp, mask);
		tmp = _mm256_sub_epi32(eta8x, tmp);
		_mm256_store_si256((__m256i *)&r->coeffs[i], tmp);
		pos += SETA2BITS;
	}
}

void polyt1_pack(unsigned char r[POLT1_SIZE_PACKED + 7], const poly *a)
{
	int i;
#if QBITS - PARAM_D == 7
	__m256i d0, d1, d2, d3;
	const __m256i zero = _mm256_setzero_si256();
	const __m256i permu = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);
	const __m256i permu2 = _mm256_set_epi32(7, 6, 5, 4, 7, 7, 7, 4);
	const __m256i lomask16 = _mm256_set1_epi16(0x7f);
	const __m256i himask16 = _mm256_set1_epi16(0x7f << 8);

	const __m256i lomask32 = _mm256_set1_epi32(0x3fff);
	const __m256i himask32 = _mm256_set1_epi32(0x3fff << 16);

	const __m256i idx8 = _mm256_set_epi8(15, 15, 14, 13, 12, 11, 10, 9,
										 8, 6, 5, 4, 3, 2, 1, 0,
										 15, 15, 14, 13, 12, 11, 10, 9,
										 8, 6, 5, 4, 3, 2, 1, 0);
	const __m256i idx82 = _mm256_set_epi8(15, 15, 15, 15, 13, 12, 11, 10,
										  9, 8, 7, 6, 5, 4, 3, 2,
										  1, 0, 15, 15, 15, 15, 15, 15,
										  15, 15, 15, 15, 15, 15, 15, 15);
	for (i = 0; i < PARAM_N / 32; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i]);
		d1 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 8]);
		d2 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 16]);
		d3 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 24]);
		d0 = _mm256_packus_epi32(d0, d1);
		d1 = _mm256_packus_epi32(d2, d3);
		d0 = _mm256_packus_epi16(d0, d1);

		d0 = _mm256_permutevar8x32_epi32(d0, permu);

		d1 = _mm256_and_si256(d0, himask16);
		d0 = _mm256_and_si256(d0, lomask16);
		d1 = _mm256_srli_epi16(d1, 1);
		d0 = _mm256_or_si256(d0, d1);

		d1 = _mm256_and_si256(d0, himask32);
		d0 = _mm256_and_si256(d0, lomask32);
		d1 = _mm256_srli_epi32(d1, 2);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_blend_epi32(d1, zero, 0xAA);
		d1 = _mm256_blend_epi32(zero, d1, 0xAA);
		d1 = _mm256_srli_epi64(d1, 4);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_shuffle_epi8(d1, idx8);

		d1 = _mm256_permutevar8x32_epi32(d0, permu2);
		d1 = _mm256_shuffle_epi8(d1, idx82);
		d0 = _mm256_blend_epi32(d0, zero, 0xF0);
		d0 = _mm256_or_si256(d0, d1);

		_mm256_storeu_si256((__m256i *)&r[28 * i], d0);
	}
#elif QBITS - PARAM_D == 8
	__m256i d0, d1, d2, d3;
	const __m256i permu = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);
	for (i = 0; i < PARAM_N / 32; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i]);
		d1 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 8]);
		d2 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 16]);
		d3 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 24]);
		d0 = _mm256_packus_epi32(d0, d1);
		d1 = _mm256_packus_epi32(d2, d3);
		d0 = _mm256_packus_epi16(d0, d1);

		d0 = _mm256_permutevar8x32_epi32(d0, permu);

		_mm256_storeu_si256((__m256i*)&r[32 * i], d0);
	}
#elif QBITS - PARAM_D == 9
	__m256i d0, d1;
	const __m256i zero = _mm256_setzero_si256();
	const __m256i permu = _mm256_set_epi32(7, 6, 5, 4, 7, 7, 5, 4);
	const __m256i lomask32 = _mm256_set1_epi32(0x1ff);
	const __m256i himask32 = _mm256_set1_epi32(0x1ff << 16);
	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 12,
										 11, 10, 9, 8, 15, 15, 15, 15,
										 15, 15, 15, 15, 15, 15, 15, 12,
										 11, 10, 9, 8, 15, 15, 15, 15);

	const __m256i idx82 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
										  15, 15, 15, 15, 15, 15, 8, 7,
										  6, 5, 4, 3, 2, 1, 0, 15,
										  15, 15, 15, 15, 15, 15, 15, 15);

	for (i = 0; i < PARAM_N / 16 - 1; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i]);
		d1 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i + 8]);

		d0 = _mm256_packus_epi32(d0, d1);

		d1 = _mm256_and_si256(d0, himask32);
		d0 = _mm256_and_si256(d0, lomask32);
		d1 = _mm256_srli_epi32(d1, 7);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_blend_epi32(d1, zero, 0xAA);
		d1 = _mm256_blend_epi32(zero, d1, 0xAA);
		d1 = _mm256_srli_epi64(d1, 14);
		d1 = _mm256_or_si256(d0, d1);

		d1 = _mm256_permute4x64_epi64(d1, 0xD8);
		d0 = _mm256_blend_epi32(d1, zero, 0xCC);
		d1 = _mm256_slli_epi64(d1, 4);
		d1 = _mm256_shuffle_epi8(d1, idx8);
		d1 = _mm256_or_si256(d0, d1);

		_mm_storeu_si128((__m128i *)&r[18 * i], _mm256_castsi256_si128(d1));
		_mm_storeu_si128((__m128i *)&r[18 * i + 9], _mm256_extracti128_si256(d1,1));
	}
	d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i]);
	d1 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i + 8]);

	d0 = _mm256_packus_epi32(d0, d1);

	d1 = _mm256_and_si256(d0, himask32);
	d0 = _mm256_and_si256(d0, lomask32);
	d1 = _mm256_srli_epi32(d1, 7);
	d1 = _mm256_or_si256(d0, d1);

	d0 = _mm256_blend_epi32(d1, zero, 0xAA);
	d1 = _mm256_blend_epi32(zero, d1, 0xAA);
	d1 = _mm256_srli_epi64(d1, 14);
	d1 = _mm256_or_si256(d0, d1);

	d1 = _mm256_permute4x64_epi64(d1, 0xD8);
	d0 = _mm256_blend_epi32(d1, zero, 0xCC);
	d1 = _mm256_slli_epi64(d1, 4);
	d1 = _mm256_shuffle_epi8(d1, idx8);
	d1 = _mm256_or_si256(d0, d1);

	ALIGN(32) uint8_t t[32];
	_mm_storeu_si128((__m128i *)&r[18 * i], _mm256_castsi256_si128(d1));
	_mm_storeu_si128(t, _mm256_extracti128_si256(d1,1));
	memcpy(&r[18 * i + 9], t, 9);

#else
#error "polyt1_pack() assumes QBITS - PARAM_D == 7 or 9"
#endif
}

void polyt1_unpack(poly *r, const uint8_t *a)
{
	int i, pos;
	__m256i tmp;

#if QBITS - PARAM_D == 6
	__m256i mask = _mm256_set1_epi32(0x3F);
	const __m256i idx8 = _mm256_set_epi8(8, 7, 6, 5, 7, 6, 5, 4,
										 6, 5, 4, 3, 6, 5, 4, 3,
										 5, 4, 3, 2, 4, 3, 2, 1,
										 3, 2, 1, 0, 3, 2, 1, 0);
	const __m256i shift = _mm256_set_epi32(2, 4, 6, 0, 2, 4, 6, 0);
	pos = 0;
	for (i = 0; i < PARAM_N; i += 8)
	{
		tmp = _mm256_set1_epi64x(*(uint64_t *)&a[pos]);
		tmp = _mm256_shuffle_epi8(tmp, idx8);
		tmp = _mm256_srlv_epi32(tmp, shift);
		tmp = _mm256_and_si256(tmp, mask);
		_mm256_store_si256((__m256i *)&r->coeffs[i], tmp);
		pos += 6;
	}

#elif QBITS - PARAM_D == 7
	__m256i mask = _mm256_set1_epi32(0x7F);
	const __m256i idx8 = _mm256_set_epi8(9, 8, 7, 6, 8, 7, 6, 5,
										 7, 6, 5, 4, 6, 5, 4, 3,
										 5, 4, 3, 2, 4, 3, 2, 1,
										 3, 2, 1, 0, 3, 2, 1, 0);
	const __m256i shift = _mm256_set_epi32(1, 2, 3, 4, 5, 6, 7, 0);
	pos = 0;
	for (i = 0; i < PARAM_N; i += 8)
	{
		tmp = _mm256_set1_epi64x(*(uint64_t *)&a[pos]);
		tmp = _mm256_shuffle_epi8(tmp, idx8);
		tmp = _mm256_srlv_epi32(tmp, shift);
		tmp = _mm256_and_si256(tmp, mask);
		_mm256_store_si256((__m256i *)&r->coeffs[i], tmp);
		pos += 7;
	}

#elif QBITS - PARAM_D == 8
	__m256i mask = _mm256_set1_epi32(0xFF);
	const __m256i idx8 = _mm256_set_epi8(0, 0, 0, 7, 0, 0, 0, 6,
										 0, 0, 0, 5, 0, 0, 0, 4,
										 0, 0, 0, 3, 0, 0, 0, 2,
										 0, 0, 0, 1, 0, 0, 0, 0);
	for (i = 0; i < PARAM_N; i += 8)
	{
		tmp = _mm256_set1_epi64x(*(uint64_t *)&a[i]);
		tmp = _mm256_shuffle_epi8(tmp, idx8);
		tmp = _mm256_and_si256(tmp, mask);
		_mm256_store_si256((__m256i *)&r->coeffs[i], tmp);
	}
#elif QBITS - PARAM_D == 9
	const __m256i permu = _mm256_set_epi32(0, 3, 2, 1, 3, 2, 1, 0);
	__m256i mask = _mm256_set1_epi32(0x1FF);
	const __m256i idx8 = _mm256_set_epi8(6, 5, 4, 3, 5, 4, 3, 2,
										 4, 3, 2, 1, 3, 2, 1, 0,
										 6, 5, 4, 3, 5, 4, 3, 2,
										 4, 3, 2, 1, 3, 2, 1, 0);
	const __m256i shift = _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0);

	pos = 0;
	for (i = 0; i < PARAM_N; i += 8)
	{
		tmp = _mm256_loadu_si256((__m256i *)&a[pos]);
		tmp = _mm256_permutevar8x32_epi32(tmp, permu);
		tmp = _mm256_shuffle_epi8(tmp, idx8);
		tmp = _mm256_srlv_epi32(tmp, shift);
		tmp = _mm256_and_si256(tmp, mask);
		_mm256_store_si256((__m256i *)&r->coeffs[i], tmp);
		pos += 9;
	}
#endif
}

void polyt0_pack(unsigned char r[POLT0_SIZE_PACKED + 8], const poly *a)
{
	int i;
	__m256i d0, d1;
	const __m256i zero = _mm256_setzero_si256();

#if  PARAM_D == 13
	const __m256i pdm1 = _mm256_set1_epi16(1 << (PARAM_D - 1));
	const __m256i permu = _mm256_set_epi32(7, 6, 5, 4, 7, 7, 7, 4);
	const __m256i lomask32 = _mm256_set1_epi32(0x1fff);
	const __m256i himask32 = _mm256_set1_epi32(0x1fff << 16);

	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 14, 13, 12, 11, 10,
										 9, 8, 15, 15, 15, 15, 15, 15,
										 15, 15, 15, 14, 13, 12, 11, 10,
										 9, 8, 15, 15, 15, 15, 15, 15);
	const __m256i idx82 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 12, 11,
										  10, 9, 8, 7, 6, 5, 4, 3,
										  2, 1, 0, 15, 15, 15, 15, 15,
										  15, 15, 15, 15, 15, 15, 15, 15);

	for (i = 0; i < PARAM_N / 16; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i]);
		d1 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i + 8]);

		d0 = _mm256_packs_epi32(d0, d1);
		d0 = _mm256_sub_epi16(pdm1, d0);

		d1 = _mm256_and_si256(d0, himask32);
		d0 = _mm256_and_si256(d0, lomask32);
		d1 = _mm256_srli_epi32(d1, 3);
		d1 = _mm256_or_si256(d0, d1); // 26/32

		d0 = _mm256_blend_epi32(d1, zero, 0xAA);
		d1 = _mm256_blend_epi32(zero, d1, 0xAA);
		d1 = _mm256_srli_epi64(d1, 6);
		d1 = _mm256_or_si256(d0, d1); // 52/64

		d1 = _mm256_permute4x64_epi64(d1, 0xD8);

		d0 = _mm256_blend_epi32(d1, zero, 0xCC);
		d1 = _mm256_slli_epi64(d1, 4);
		d1 = _mm256_shuffle_epi8(d1, idx8);
		d0 = _mm256_or_si256(d0, d1);

		d1 = _mm256_permutevar8x32_epi32(d0, permu);
		d1 = _mm256_shuffle_epi8(d1, idx82);
		d0 = _mm256_blend_epi32(d0, zero, 0xF0);
		d0 = _mm256_or_si256(d0, d1);

		_mm256_storeu_si256((__m256i *)&r[26 * i], d0);
	}
#elif PARAM_D==14
	const __m256i pdm1 = _mm256_set1_epi16(1 << (PARAM_D - 1));
	const __m256i permu = _mm256_set_epi32(7, 6, 5, 4, 7, 7, 7, 4);
	const __m256i lomask32 = _mm256_set1_epi32(0x3fff);
	const __m256i himask32 = _mm256_set1_epi32(0x3fff << 16);

	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
		15, 15, 15, 15, 15, 15, 15, 15,
		9, 8, 15, 15, 15, 15, 15, 15,
		15, 6, 5, 4, 3, 2, 1, 0);
	const __m256i idx82 = _mm256_set_epi8(15, 15, 15, 15, 14, 13, 12, 11,
		10, 9, 8, 6, 5, 4, 3, 2,
		15, 15, 14, 13, 12, 11, 10, 9,
		8, 15, 15, 15, 15, 15, 15, 15);

	for (i = 0; i < PARAM_N / 16; i++)
	{
		d0 = _mm256_loadu_si256((__m256i*) & a->coeffs[16 * i]);
		d1 = _mm256_loadu_si256((__m256i*) & a->coeffs[16 * i + 8]);

		d0 = _mm256_packs_epi32(d0, d1);
		d0 = _mm256_sub_epi16(pdm1, d0);

		d1 = _mm256_and_si256(d0, himask32);
		d0 = _mm256_and_si256(d0, lomask32);
		d1 = _mm256_srli_epi32(d1, 2);
		d1 = _mm256_or_si256(d0, d1);//28/32

		d0 = _mm256_blend_epi32(d1, zero, 0xAA);
		d1 = _mm256_blend_epi32(zero, d1, 0xAA);
		d1 = _mm256_srli_epi64(d1, 4);
		d0 = _mm256_or_si256(d0, d1);//56/64

		d1 = _mm256_permute4x64_epi64(d0, 0xD8);
		d0 = _mm256_shuffle_epi8(d0, idx8);
		d1 = _mm256_shuffle_epi8(d1, idx82);

		d0 = _mm256_or_si256(d0, d1);

		_mm256_storeu_si256(&r[28 * i], d0);
	}
#elif PARAM_D == 15
	const __m256i pdm1 = _mm256_set1_epi16(1 << (PARAM_D - 1));
	// const __m256i permu = _mm256_set_epi32(7, 6, 5, 4, 7, 7, 7, 4);
	const __m256i lomask32 = _mm256_set1_epi32(0x7fff);
	const __m256i himask32 = _mm256_set1_epi32(0x7fff << 16);

	const __m256i idx8 = _mm256_set_epi8(128, 15, 14, 13, 12, 11, 10, 9,
										 8, 128, 128, 128, 128, 128, 128, 128,
										 128, 15, 14, 13, 12, 11, 10, 9,
										 8, 128, 128, 128, 128, 128, 128, 128);
	const __m256i idx82 = _mm256_set_epi8(15, 15, 14, 13, 12, 11, 10, 9,
										  8, 7, 6, 5, 4, 3, 2, 1,
										  0, 15, 15, 15, 15, 15, 15, 15,
										  15, 15, 15, 15, 15, 15, 15, 15);

	for (i = 0; i < PARAM_N / 16; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i]);
		d1 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i + 8]);

		d0 = _mm256_packs_epi32(d0, d1);
		d0 = _mm256_sub_epi16(pdm1, d0);

		d1 = _mm256_and_si256(d0, himask32);
		d0 = _mm256_and_si256(d0, lomask32);
		d1 = _mm256_srli_epi32(d1, 1);
		d1 = _mm256_or_si256(d0, d1); // 30/32

		d0 = _mm256_blend_epi32(d1, zero, 0xAA);
		d1 = _mm256_blend_epi32(zero, d1, 0xAA);
		d1 = _mm256_srli_epi64(d1, 2);
		d0 = _mm256_or_si256(d0, d1); // 60/64

		d0 = _mm256_permute4x64_epi64(d0, 0xD8);

		d1 = _mm256_slli_epi64(d0, 4);
		d0 = _mm256_blend_epi32(d0, zero, 0xCC);
		d1 = _mm256_shuffle_epi8(d1, idx8);
		d0 = _mm256_or_si256(d0, d1); // 120//128

		d1 = _mm256_permute4x64_epi64(d0, 0xE6);
		d1 = _mm256_shuffle_epi8(d1, idx82);
		d0 = _mm256_or_si256(d0, d1);
		d0 = _mm256_blend_epi32(d0, d1, 0xF0);
		_mm256_storeu_si256((__m256i *)&r[30 * i], d0);
	}
#else
#error "polyt0_unpack() assumes PARAM_D== 12 or 13"
#endif
}

void polyz_pack(unsigned char r[SZBITS * PARAM_N + 16], const poly *a)
{
#if GAMMA1 != 16384 && GAMMA1 != 32768 && GAMMA1 != 65536 && GAMMA1 != 131072 && GAMMA1 != 262144 && GAMMA1 != 524288
#error "poly_uniform_gamma1() assumes GAMMA1 == 16384 32768 65536 or 131072"
#endif
	int i;
	__m256i d0, d1;
	const __m256i gamma1 = _mm256_set1_epi32(GAMMA1);
	const __m256i zero = _mm256_setzero_si256();
#if GAMMA1 == 16384
	const __m256i permu = _mm256_set_epi32(7, 7, 7, 7, 5, 4, 7, 7);
	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
										 12, 11, 10, 9, 8, 15, 15, 15,
										 15, 15, 15, 15, 15, 15, 15, 15,
										 12, 11, 10, 9, 8, 15, 15, 15);
	const __m256i idx82 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
										  15, 15, 15, 15, 15, 15, 15, 15,
										  0, 15, 14, 13, 12, 11, 10, 9,
										  8, 0, 0, 0, 0, 0, 0, 0);

	for (i = 0; i < PARAM_N / 8; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[8 * i]);
		d1 = _mm256_sub_epi32(gamma1, d0);

		d0 = _mm256_blend_epi32(d1, zero, 0xAA); // lower 32 bits
		d1 = _mm256_blend_epi32(zero, d1, 0xAA); // higher 32 bits
		d1 = _mm256_srli_epi64(d1, 17);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_blend_epi32(d1, zero, 0xCC); // lower 64 bits
		d1 = _mm256_slli_epi64(d1, 6);
		d1 = _mm256_shuffle_epi8(d1, idx8);
		d0 = _mm256_or_si256(d0, d1);

		d1 = _mm256_permutevar8x32_epi32(d0, permu);
		d1 = _mm256_slli_epi64(d1, 4);
		d1 = _mm256_shuffle_epi8(d1, idx82);
		d0 = _mm256_or_si256(d0, d1);

		_mm256_storeu_si256((__m256i *)&r[15 * i], d0);
	}
#elif GAMMA1 == 32768
	for (i = 0; i < PARAM_N / 16; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i]);
		d1 = _mm256_loadu_si256((__m256i *)&a->coeffs[16 * i + 8]);
		d0 = _mm256_sub_epi32(gamma1, d0);
		d1 = _mm256_sub_epi32(gamma1, d1);

		d0 = _mm256_packus_epi32(d0, d1);
		d0 = _mm256_permute4x64_epi64(d0, 0xD8);
		_mm256_store_si256((__m256i *)&r[32 * i], d0);
	}
#elif GAMMA1 == 65536
	const __m256i permu = _mm256_set_epi32(6, 5, 7, 7, 5, 4, 7, 7);
	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 12,
										 11, 10, 9, 8, 15, 15, 15, 15,
										 15, 15, 15, 15, 15, 15, 15, 12,
										 11, 10, 9, 8, 15, 15, 15, 15);
	const __m256i idx82 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
										  15, 15, 15, 15, 15, 15, 15, 12,
										  15, 14, 13, 12, 11, 10, 9, 8,
										  7, 6, 5, 4, 3, 2, 1, 0);

	for (i = 0; i < PARAM_N / 8; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[8 * i]);
		d1 = _mm256_sub_epi32(gamma1, d0);

		d0 = _mm256_blend_epi32(d1, zero, 0xAA); // lower 32 bits
		d1 = _mm256_blend_epi32(zero, d1, 0xAA); // higher 32 bits
		d1 = _mm256_srli_epi64(d1, 15);
		d1 = _mm256_or_si256(d0, d1); // 34/64

		d0 = _mm256_blend_epi32(d1, zero, 0xCC); // lower 64 bits
		d1 = _mm256_slli_epi64(d1, 2);
		d1 = _mm256_shuffle_epi8(d1, idx8);
		d0 = _mm256_or_si256(d0, d1); // 68/128

		d1 = _mm256_permutevar8x32_epi32(d0, permu);
		d1 = _mm256_slli_epi64(d1, 4);
		d0 = _mm256_or_si256(d0, d1);
		d0 = _mm256_shuffle_epi8(d0, idx82);

		_mm256_storeu_si256((__m256i *)&r[17 * i], d0);
	}
#elif GAMMA1 == 131072
	const __m256i permu = _mm256_set_epi32(7, 6, 5, 4, 7, 7, 5, 4);
	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 12,
										 11, 10, 9, 8, 15, 15, 15, 15,
										 15, 15, 15, 15, 15, 15, 15, 12,
										 11, 10, 9, 8, 15, 15, 15, 15);
	const __m256i idx82 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
										  15, 15, 15, 15, 15, 15, 8, 7,
										  6, 5, 4, 3, 2, 1, 0, 15,
										  15, 15, 15, 15, 15, 15, 15, 15);

	for (i = 0; i < PARAM_N / 8; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[8 * i]);
		d1 = _mm256_sub_epi32(gamma1, d0);

		d0 = _mm256_blend_epi32(d1, zero, 0xAA);
		d1 = _mm256_blend_epi32(zero, d1, 0xAA);
		d1 = _mm256_srli_epi64(d1, 14);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_blend_epi32(d1, zero, 0xCC);
		d1 = _mm256_slli_epi64(d1, 4);
		d1 = _mm256_shuffle_epi8(d1, idx8);
		d0 = _mm256_or_si256(d0, d1);

		d1 = _mm256_permutevar8x32_epi32(d0, permu);
		d1 = _mm256_shuffle_epi8(d1, idx82);
		d0 = _mm256_blend_epi32(d0, zero, 0xF0);
		d0 = _mm256_or_si256(d0, d1);

		_mm256_storeu_si256((__m256i *)&r[18 * i], d0);
	}
#elif GAMMA1 == 262144
	const __m256i permu = _mm256_set_epi32(7, 7, 6, 5, 5, 4, 7, 7);
	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 13, 12,
										 11, 10, 9, 8, 15, 15, 15, 15,
										 15, 15, 15, 15, 15, 15, 13, 12,
										 11, 10, 9, 8, 15, 15, 15, 15);
	const __m256i idx82 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
										  15, 15, 15, 15, 15, 6, 5, 4,
										  15, 14, 13, 12, 11, 10, 9, 8,
										  7, 6, 5, 4, 3, 2, 1, 0);

	for (i = 0; i < PARAM_N / 8; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[8 * i]);
		d1 = _mm256_sub_epi32(gamma1, d0);

		d0 = _mm256_blend_epi32(d1, zero, 0xAA);
		d1 = _mm256_blend_epi32(zero, d1, 0xAA);
		d1 = _mm256_srli_epi64(d1, 13);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_blend_epi32(d1, zero, 0xCC);
		d1 = _mm256_slli_epi64(d1, 6);
		d1 = _mm256_shuffle_epi8(d1, idx8);
		d0 = _mm256_or_si256(d0, d1);

		d1 = _mm256_permutevar8x32_epi32(d0, permu);
		d1 = _mm256_slli_epi64(d1, 12);
		d1 = _mm256_shuffle_epi8(d1, idx82);
		d0 = _mm256_blend_epi32(d0, zero, 0xF0);
		d0 = _mm256_or_si256(d0, d1);

		_mm256_storeu_si256((__m256i *)&r[19 * i], d0);
	}
#elif GAMMA1 == 524288
	const __m256i permu = _mm256_set_epi32(7, 7, 7, 6, 5, 4, 7, 7);
	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 12, 11, 10, 9,
										 8, 4, 3, 2, 1, 0, 15, 15,
										 15, 15, 15, 15, 15, 15, 12, 11,
										 10, 9, 8, 4, 3, 2, 1, 0);

	for (i = 0; i < PARAM_N / 8; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[8 * i]);
		d1 = _mm256_sub_epi32(gamma1, d0);

		d0 = _mm256_blend_epi32(d1, zero, 0xAA);
		d1 = _mm256_blend_epi32(zero, d1, 0xAA);
		d1 = _mm256_srli_epi64(d1, 12);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_shuffle_epi8(d1, idx8);
		d1 = _mm256_permutevar8x32_epi32(d0, permu);
		d0 = _mm256_blend_epi32(d0, zero, 0xF0);
		d0 = _mm256_or_si256(d0, d1);

		_mm256_storeu_si256((__m256i *)&r[20 * i], d0);
	}
#else
#error "polyz_pack() error"
#endif
}

void polyz_unpack(poly *r, const uint8_t *a)
{
#if GAMMA1 != 16384 && GAMMA1 != 32768 && GAMMA1 != 65536 && GAMMA1 != 131072 && GAMMA1 != 262144 && GAMMA1 != 524288
#error "poly_uniform_gamma1() assumes GAMMA1 ==16384, 32768 or 131072"
#endif
	int pos;
	int i;
	__m256i tmp;
	__m256i gamma1 = _mm256_set1_epi32(GAMMA1);

#if SZBITS == 15
	const __m256i permu = _mm256_set_epi32(4, 3, 2, 1, 3, 2, 1, 0);
	__m256i mask = _mm256_set1_epi32(0x7FFF);
	const __m256i idx8 = _mm256_set_epi8(12, 11, 10, 9, 10, 9, 8, 7,
										 8, 7, 6, 5, 6, 5, 4, 3,
										 8, 7, 6, 5, 6, 5, 4, 3,
										 4, 3, 2, 1, 3, 2, 1, 0);
	const __m256i shift = _mm256_set_epi32(1, 2, 3, 4, 5, 6, 7, 0);
#elif SZBITS == 16
	const __m256i permu = _mm256_set_epi32(5, 4, 3, 2, 3, 2, 1, 0);
	__m256i mask = _mm256_set1_epi32(0xFFFF);
	const __m256i idx8 = _mm256_set_epi8(9, 8, 7, 6, 7, 6, 5, 4,
										 5, 4, 3, 2, 3, 2, 1, 0,
										 9, 8, 7, 6, 7, 6, 5, 4,
										 5, 4, 3, 2, 3, 2, 1, 0);
#elif SZBITS == 17
	const __m256i permu = _mm256_set_epi32(5, 4, 3, 2, 3, 2, 1, 0);
	__m256i mask = _mm256_set1_epi32(0x1FFFF);
	const __m256i idx8 = _mm256_set_epi8(9, 8, 7, 6, 7, 6, 5, 4,
										 5, 4, 3, 2, 3, 2, 1, 0,
										 9, 8, 7, 6, 7, 6, 5, 4,
										 5, 4, 3, 2, 3, 2, 1, 0);
	const __m256i shift = _mm256_set_epi32(7, 6, 5, 4, 3, 2, 1, 0);
#elif SZBITS == 18
	const __m256i permu = _mm256_set_epi32(5, 4, 3, 2, 3, 2, 1, 0);
	__m256i mask = _mm256_set1_epi32(0x3FFFF);
	const __m256i idx8 = _mm256_set_epi8(10, 9, 8, 7, 8, 7, 6, 5,
										 6, 5, 4, 3, 4, 3, 2, 1,
										 9, 8, 7, 6, 7, 6, 5, 4,
										 5, 4, 3, 2, 3, 2, 1, 0);
	const __m256i shift = _mm256_set_epi32(6, 4, 2, 0, 6, 4, 2, 0);
#elif SZBITS == 19
	const __m256i permu = _mm256_set_epi32(5, 4, 3, 2, 3, 2, 1, 0);
	__m256i mask = _mm256_set1_epi32(0x7FFFF);
	const __m256i idx8 = _mm256_set_epi8(11, 10, 9, 8, 9, 8, 7, 6,
										 6, 5, 4, 3, 4, 3, 2, 1,
										 10, 9, 8, 7, 7, 6, 5, 4,
										 5, 4, 3, 2, 3, 2, 1, 0);
	const __m256i shift = _mm256_set_epi32(5, 2, 7, 4, 1, 6, 3, 0);
#elif SZBITS == 20
	const __m256i permu = _mm256_set_epi32(5, 4, 3, 2, 3, 2, 1, 0);
	__m256i mask = _mm256_set1_epi32(0xFFFFF);
	const __m256i idx8 = _mm256_set_epi8(12, 11, 10, 9, 10, 9, 8, 7,
										 7, 6, 5, 4, 5, 4, 3, 2,
										 10, 9, 8, 7, 8, 7, 6, 5,
										 5, 4, 3, 2, 3, 2, 1, 0);
	const __m256i shift = _mm256_set_epi32(4, 0, 4, 0, 4, 0, 4, 0);
#endif
	pos = 0;
	for (i = 0; i < PARAM_N; i += 8)
	{
		tmp = _mm256_loadu_si256((__m256i *)&a[pos]);
		tmp = _mm256_permutevar8x32_epi32(tmp, permu);
		tmp = _mm256_shuffle_epi8(tmp, idx8);
#if SZBITS != 16
		tmp = _mm256_srlv_epi32(tmp, shift);
#endif
		tmp = _mm256_and_si256(tmp, mask);
		tmp = _mm256_sub_epi32(gamma1, tmp);
		_mm256_store_si256((__m256i *)&r->coeffs[i], tmp);
		pos += SZBITS;
	}
}
void polyw1_pack(unsigned char r[POLW1_SIZE_PACKED + 4], const poly *a)
{
#if PARAM_Q / ALPHA > 64 || PARAM_Q / ALPHA < 3
#error "polyw1_pack() assumes 2 < PARAM_Q/ALPHA -1 <= 16"
#endif
	int i;
	__m256i d0, d1, d2, d3;
#if PARAM_Q/ALPHA == 48
	for (i = 0; i < PARAM_N / 4; ++i) {
		r[3 * i] = (uint8_t)(a->coeffs[4 * i]
		                    | (a->coeffs[4 * i + 1] << 6));
		r[3 * i + 1] = (uint8_t)((a->coeffs[4 * i + 1] >> 2)
		                        | (a->coeffs[4 * i + 2] << 4));
		r[3 * i + 2] = (uint8_t)((a->coeffs[4 * i + 2] >> 4)
		                        | (a->coeffs[4 * i + 3] << 2));
	}
#elif PARAM_Q / ALPHA == 12
	for (i = 0; i < PARAM_N / 2; ++i)
		r[i] = (uint8_t)(a->coeffs[2 * i]
		                 | (a->coeffs[2 * i + 1] << 4));
#elif PARAM_Q / ALPHA > 8
	__m128i t;
	const __m256i lomask16 = _mm256_set1_epi16(0xF);
	const __m256i himask16 = _mm256_set1_epi16(0xF << 8);
	const __m256i idx8 = _mm256_set_epi8(14, 12, 15, 15, 10, 8, 15, 15,
										 6, 4, 15, 15, 2, 0, 15, 15,
										 15, 15, 14, 12, 15, 15, 10, 8,
										 15, 15, 6, 4, 15, 15, 2, 0);

	for (i = 0; i < PARAM_N / 32; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i]);
		d1 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 8]);
		d2 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 16]);
		d3 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 24]);
		d0 = _mm256_packus_epi32(d0, d1);
		d1 = _mm256_packus_epi32(d2, d3);
		d0 = _mm256_packus_epi16(d0, d1);

		d1 = _mm256_and_si256(d0, himask16);
		d0 = _mm256_and_si256(d0, lomask16);
		d1 = _mm256_srli_epi16(d1, 4);
		d0 = _mm256_or_si256(d0, d1);

		d0 = _mm256_shuffle_epi8(d0, idx8);

		t = _mm256_extracti128_si256(d0, 1);
		t = _mm_or_si128(_mm256_castsi256_si128(d0), t);

		_mm_storeu_si128((__m128i *)&r[16 * i], t);
	}
#elif PARAM_Q / ALPHA > 4
	const __m256i permu = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);
	const __m256i permu2 = _mm256_set_epi32(7, 7, 7, 7, 6, 4, 2, 0);
	const __m256i lomask16 = _mm256_set1_epi16(0x7);
	const __m256i himask16 = _mm256_set1_epi16(0x7 << 8);
	const __m256i zero = _mm256_setzero_si256();
	const __m256i lomask32 = _mm256_set1_epi32(0x3f);
	const __m256i himask32 = _mm256_set1_epi32(0x3f << 16);

	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
										 15, 15, 15, 15, 15, 15, 15, 15,
										 15, 15, 15, 15, 14, 13, 12, 10,
										 9, 8, 6, 5, 4, 2, 1, 0);

	for (i = 0; i < PARAM_N / 32; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i]);
		d1 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 8]);
		d2 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 16]);
		d3 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 24]);
		d0 = _mm256_packus_epi32(d0, d1);
		d1 = _mm256_packus_epi32(d2, d3);
		d0 = _mm256_packus_epi16(d0, d1);

		d0 = _mm256_permutevar8x32_epi32(d0, permu);

		d1 = _mm256_and_si256(d0, himask16);
		d0 = _mm256_and_si256(d0, lomask16);
		d1 = _mm256_srli_epi16(d1, 5);
		d0 = _mm256_or_si256(d0, d1);

		d1 = _mm256_and_si256(d0, himask32);
		d0 = _mm256_and_si256(d0, lomask32);
		d1 = _mm256_srli_epi32(d1, 10);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_blend_epi32(d1, zero, 0xAA);
		d1 = _mm256_blend_epi32(zero, d1, 0xAA);
		d1 = _mm256_srli_epi64(d1, 20);
		d1 = _mm256_or_si256(d0, d1);

		d1 = _mm256_permutevar8x32_epi32(d1, permu2);
		d0 = _mm256_shuffle_epi8(d1, idx8);

		_mm_storeu_si128((__m128i *)&r[12 * i], _mm256_castsi256_si128(d0));
	}
#elif PARAM_Q / ALPHA <= 4
	const __m256i lomask16 = _mm256_set1_epi16(0x3);
	const __m256i himask16 = _mm256_set1_epi16(0x3 << 8);
	const __m256i lomask32 = _mm256_set1_epi32(0xf);
	const __m256i himask32 = _mm256_set1_epi32(0xf << 16);
	const __m256i idx8 = _mm256_set_epi8(15, 15, 15, 15, 15, 15, 15, 15,
										 12, 15, 8, 15, 4, 15, 0, 15,
										 15, 15, 15, 15, 15, 15, 15, 15,
										 15, 12, 15, 8, 15, 4, 15, 0);
	__m128i t;

	for (i = 0; i < PARAM_N / 32; i++)
	{
		d0 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i]);
		d1 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 8]);
		d2 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 16]);
		d3 = _mm256_loadu_si256((__m256i *)&a->coeffs[32 * i + 24]);

		d0 = _mm256_packs_epi32(d0, d1);
		d1 = _mm256_packs_epi32(d2, d3);
		d0 = _mm256_packs_epi16(d0, d1);

		d1 = _mm256_and_si256(d0, himask16);
		d0 = _mm256_and_si256(d0, lomask16);
		d1 = _mm256_srli_epi16(d1, 6);
		d0 = _mm256_or_si256(d0, d1);

		d1 = _mm256_and_si256(d0, himask32);
		d0 = _mm256_and_si256(d0, lomask32);
		d1 = _mm256_srli_epi32(d1, 12);
		d1 = _mm256_or_si256(d0, d1);

		d0 = _mm256_shuffle_epi8(d1, idx8);

		t = _mm256_extracti128_si256(d0, 1);
		t = _mm_or_si128(_mm256_castsi256_si128(d0), t);
		_mm_storel_epi64((__m128i *)&r[8 * i], t);
	}
#endif
}
