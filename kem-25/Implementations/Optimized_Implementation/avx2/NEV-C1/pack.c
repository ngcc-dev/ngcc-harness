#include "pack.h"
#include <immintrin.h>

//conditinonal add PARAM_Q
//reduce from [-PARAM_Q,PARAM_Q) to [0,PARAM_Q)
int16_t caddq(int16_t x)
{
    int16_t r;
    r = x + ((x >> 15)& PARAM_Q);
    return r;
}


static inline __m256i flipabs(__m256i v,
								  const __m256i q16x,
								  const __m256i hfq)
{
	__m256i mask;

	mask = _mm256_srai_epi16(v, 15);
	mask = _mm256_and_si256(mask, q16x);
	v = _mm256_add_epi16(v, mask);

	v = _mm256_sub_epi16(v, hfq);
	mask = _mm256_srai_epi16(v, 15);
	v = _mm256_add_epi16(v, mask);
	v = _mm256_xor_si256(v, mask);

	return v;
}

#if NTT_DIM == 64

void poly_tomsg(uint8_t msg[32], const poly* r)
{
    int i, j, small;
    __m256i u, v, mask;
    const __m256i q16x = _mm256_set1_epi16(PARAM_Q);
    const __m256i hfq = _mm256_set1_epi16(PARAM_Q / 2);

    for (i = 0; i < SEED_BYTES / 2; i++)
    {
        v = _mm256_load_si256((__m256i*) & r->coeffs[64 * i]);
        mask = _mm256_srai_epi16(v, 15);
        mask = _mm256_and_si256(mask, q16x);
        v = _mm256_add_epi16(v, mask);

        v = _mm256_sub_epi16(v, hfq);
        mask = _mm256_srai_epi16(v, 15);
        v = _mm256_add_epi16(v, mask);
        u = _mm256_xor_si256(v, mask);

        v = _mm256_load_si256((__m256i*) & r->coeffs[64 * i + 16]);
        mask = _mm256_srai_epi16(v, 15);
        mask = _mm256_and_si256(mask, q16x);
        v = _mm256_add_epi16(v, mask);

        v = _mm256_sub_epi16(v, hfq);
        mask = _mm256_srai_epi16(v, 15);
        v = _mm256_add_epi16(v, mask);
        v = _mm256_xor_si256(v, mask);
        u = _mm256_add_epi16(u, v);

        v = _mm256_load_si256((__m256i*) & r->coeffs[64 * i + 32]);
        mask = _mm256_srai_epi16(v, 15);
        mask = _mm256_and_si256(mask, q16x);
        v = _mm256_add_epi16(v, mask);

        v = _mm256_sub_epi16(v, hfq);
        mask = _mm256_srai_epi16(v, 15);
        v = _mm256_add_epi16(v, mask);
        v = _mm256_xor_si256(v, mask);
        u = _mm256_add_epi16(u, v);

        v = _mm256_load_si256((__m256i*) & r->coeffs[64 * i + 48]);
        mask = _mm256_srai_epi16(v, 15);
        mask = _mm256_and_si256(mask, q16x);
        v = _mm256_add_epi16(v, mask);

        v = _mm256_sub_epi16(v, hfq);
        mask = _mm256_srai_epi16(v, 15);
        v = _mm256_add_epi16(v, mask);
        v = _mm256_xor_si256(v, mask);
        u = _mm256_add_epi16(u, v);

        v = _mm256_sub_epi16(u, q16x);

        small = _mm256_movemask_epi8(v);
        small = _pext_u32(small, 0xAAAAAAAA);
        msg[2 * i + 0] = small;
        msg[2 * i + 1] = small >> 8;
    }
}
#elif NTT_DIM == 128
void poly_tomsg(uint8_t msg[SEED_BYTES], const poly *x)
{
	int i;
	uint32_t bits0, bits1;
	__m256i u0, u1, v;
	const __m256i q16x = _mm256_set1_epi16(PARAM_Q);
	const __m256i hfq = _mm256_set1_epi16(PARAM_Q / 2);

	for (i = 0; i < SEED_BYTES / 4; i++) {
		const int16_t *p = &x->coeffs[128 * i];

		u0 = flipabs(_mm256_load_si256((const __m256i *)(p + 0)), q16x, hfq);
		v = flipabs(_mm256_load_si256((const __m256i *)(p + 32)), q16x, hfq);
		u0 = _mm256_add_epi16(u0, v);
		v = flipabs(_mm256_load_si256((const __m256i *)(p + 64)), q16x, hfq);
		u0 = _mm256_add_epi16(u0, v);
		v = flipabs(_mm256_load_si256((const __m256i *)(p + 96)), q16x, hfq);
		u0 = _mm256_add_epi16(u0, v);

		u1 = flipabs(_mm256_load_si256((const __m256i *)(p + 16)), q16x, hfq);
		v = flipabs(_mm256_load_si256((const __m256i *)(p + 48)), q16x, hfq);
		u1 = _mm256_add_epi16(u1, v);
		v = flipabs(_mm256_load_si256((const __m256i *)(p + 80)), q16x, hfq);
		u1 = _mm256_add_epi16(u1, v);
		v = flipabs(_mm256_load_si256((const __m256i *)(p + 112)), q16x, hfq);
		u1 = _mm256_add_epi16(u1, v);

		u0 = _mm256_sub_epi16(u0, q16x);
		u1 = _mm256_sub_epi16(u1, q16x);

		bits0 = _pext_u32((uint32_t)_mm256_movemask_epi8(u0), 0xAAAAAAAAu);
		bits1 = _pext_u32((uint32_t)_mm256_movemask_epi8(u1), 0xAAAAAAAAu);

		msg[4 * i + 0] = (uint8_t)bits0;
		msg[4 * i + 1] = (uint8_t)(bits0 >> 8);
		msg[4 * i + 2] = (uint8_t)bits1;
		msg[4 * i + 3] = (uint8_t)(bits1 >> 8);
	}
}

#endif

static inline void decode6(uint16_t* x, const uint8_t* buf)
{
	/* useful facts:
	 *    for any x in 2^28, (6700417 * x) >> 32 = floor(x / 641)
	 */

	uint64_t t, r;
	t = (uint32_t)((buf[3] & 0x0f) << 24) | (uint32_t)(buf[2] << 16) | (uint32_t)(buf[1] << 8) | buf[0];
	r = (t * 6700417) >> 32;
	x[0] = t - r * 641;

	x[2] = (r * 6700417) >> 32;
	x[1] = r - x[2] * 641;

	t = (uint32_t)(buf[6] << 20) | (uint32_t)(buf[5] << 12) | (uint32_t)(buf[4] << 4) | (buf[3] >> 4);

	r = (t * 6700417) >> 32;
	x[3] = t - r * 641;

	x[5] = (r * 6700417) >> 32;
	x[4] = r - x[5] * 641;

}
static inline void encode6(uint8_t* buf, const uint16_t x[6])
{
	uint32_t t;

	t = x[2];
	t = t * 641 + x[1];
	t = t * 641 + x[0];

	buf[0] = t;
	buf[1] = t >> 8;
	buf[2] = t >> 16;
	buf[3] = t >> 24;

	t = x[5];
	t = t * 641 + x[4];
	t = t * 641 + x[3];

	buf[3] = buf[3] | ((t & 0x0f) << 4);
	buf[4] = t >> 4;
	buf[5] = t >> 12;
	buf[6] = t >> 20;
}

static inline void encode16(uint8_t* buf, const uint16_t x[16]) {
	uint32_t t[8];

	for (int i = 0; i < 8; ++i) {
		t[i] = x[2*i+1];
		t[i] = t[i] * 1409 + x[2*i];
	}

	buf[0]  =  t[0] & 0xff;
	buf[1]  = (t[0] >> 8) & 0xff;
	buf[2]  = ((t[0] >> 16) & 0xff) | ((t[1] << 5) & 0xff);
	buf[3]  = (t[1] >> 3) & 0xff;
	buf[4]  = (t[1] >> 11) & 0xff;
	buf[5]  = ((t[1] >> 19) & 0xff) | ((t[2] << 2) & 0xff);
	buf[6]  = (t[2] >> 6) & 0xff;
	buf[7]  = ((t[2] >> 14) & 0xff) | ((t[3] << 7) & 0xff);
	buf[8]  = (t[3] >> 1) & 0xff;
	buf[9]  = (t[3] >> 9) & 0xff;
	buf[10] = ((t[3] >> 17) & 0xff) | ((t[4] << 4) & 0xff);
	buf[11] = (t[4] >> 4) & 0xff;
	buf[12] = (t[4] >> 12) & 0xff;
	buf[13] = ((t[4] >> 20) & 0xff) | ((t[5] << 1) & 0xff);
	buf[14] = (t[5] >> 7) & 0xff;
	buf[15] = ((t[5] >> 15) & 0xff) | ((t[6] << 6) & 0xff);
	buf[16] = (t[6] >> 2) & 0xff;
	buf[17] = (t[6] >> 10) & 0xff;
	buf[18] = ((t[6] >> 18) & 0xff) | ((t[7] << 3) & 0xff);
	buf[19] = (t[7] >> 5) & 0xff;
	buf[20] = (t[7] >> 13) & 0xff;
}

static inline void decode16(uint16_t* x, const uint8_t* buf) {
	uint32_t t[8];

	t[0] = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) | ((uint32_t)(buf[2] & 0x1f) << 16);
	t[1] = ((uint32_t)buf[2] >> 5) | ((uint32_t)buf[3] << 3) | ((uint32_t)buf[4] << 11) | ((uint32_t)(buf[5] & 0x03) << 19);
	t[2] = ((uint32_t)buf[5] >> 2) | ((uint32_t)buf[6] << 6) | ((uint32_t)(buf[7] & 0x7f) << 14);
	t[3] = ((uint32_t)buf[7] >> 7) | ((uint32_t)buf[8] << 1) | ((uint32_t)buf[9] << 9) | ((uint32_t)(buf[10] & 0x0f) << 17);
	t[4] = ((uint32_t)buf[10] >> 4) | ((uint32_t)buf[11] << 4) | ((uint32_t)buf[12] << 12) | ((uint32_t)(buf[13] & 0x01) << 20);
	t[5] = ((uint32_t)buf[13] >> 1) | ((uint32_t)buf[14] << 7) | ((uint32_t)(buf[15] & 0x3f) << 15);
	t[6] = ((uint32_t)buf[15] >> 6) | ((uint32_t)buf[16] << 2) | ((uint32_t)buf[17] << 10) | ((uint32_t)(buf[18] & 0x07) << 18);
	t[7] = ((uint32_t)buf[18] >> 3) | ((uint32_t)buf[19] << 5) | ((uint32_t)buf[20] << 13);

	for (int i = 0; i < 8; ++i) {
		// x[2*i] = t[i] % 1409;
		// x[2*i+1] = t[i] / 1409;
		x[2*i+1] = ((uint64_t)t[i] * 3048238) >> 32;
		x[2*i] = t[i] - x[2*i+1] * 1409;
	}
}

#if PARAM_Q == 641
void poly_frombytes(poly* r, const uint8_t* a)
{
	/* useful facts:
		 *    for any x in 2^28, (6700417 * x) >> 32 = floor(x / 641)
		 */
	__m256i v[4], u, w, t;
	const __m256i q8x = _mm256_set1_epi32(PARAM_Q);
	const __m256i cdiv = _mm256_set1_epi32(6700417);
	const __m256i mask28 = _mm256_set1_epi32(0xFFFFFFF);
	const __m256i permu = _mm256_set_epi32(3, 6, 5, 4, 3, 2, 1, 0);
	const __m256i idx8 = _mm256_set_epi8(12, 11, 10, 9, 8, 7, 6, 5,
		5, 4, 3, 2, 1, 0, 15, 14,
		14, 13, 12, 11, 10, 9, 8, 7,
		7, 6, 5, 4, 3, 2, 1, 0);

	const __m256i idx0 = _mm256_set_epi8(5, 4, 128, 128, 3, 2, 1, 0,
		128, 128, 128, 128, 128, 128, 128, 128,
		11, 10, 9, 8, 128, 128, 7, 6,
		5, 4, 128, 128, 3, 2, 1, 0);

	const __m256i idx1 = _mm256_set_epi8(128, 128, 11, 10, 128, 128, 128, 128,
		7, 6, 15, 14, 13, 12, 3, 2,
		128, 128, 128, 128, 5, 4, 128, 128,
		128, 128, 1, 0, 128, 128, 128, 128);

	const __m256i idx2 = _mm256_set_epi8(128, 128, 128, 128, 128, 128, 128, 128,
		128, 128, 128, 128, 128, 128, 128, 128,
		128, 128, 15, 14, 13, 12, 128, 128,
		11, 10, 9, 8, 128, 128, 7, 6);

	const __m256i idx3 = _mm256_set_epi8(128, 128, 128, 128, 128, 128, 128, 128,
		128, 128, 128, 128, 128, 128, 128, 128,
		15, 14, 128, 128, 128, 128, 11, 10,
		128, 128, 128, 128, 7, 6, 128, 128);

	int j;
	for (j = 0; j < PARAM_N / 24; j++)
	{
		t = _mm256_loadu_si256((__m256i*) & a[28 * j]);
		t = _mm256_permutevar8x32_epi32(t, permu);
		t = _mm256_shuffle_epi8(t, idx8);
		w = _mm256_slli_epi64(t, 4);
		t = _mm256_blend_epi32(t, w, 0xAA);
		t = _mm256_and_si256(t, mask28);


		u = _mm256_mul_epi32(t, cdiv);
		u = _mm256_srli_epi64(u, 32);

		w = _mm256_srli_epi64(t, 32);
		w = _mm256_mul_epi32(w, cdiv);
		u = _mm256_blend_epi32(u, w, 0xAA);


		//multiplying PARAM_Q
		w = _mm256_slli_epi32(u, 2);
		w = _mm256_add_epi32(w, u);
		w = _mm256_slli_epi32(w, 7);
		w = _mm256_add_epi32(w, u);

		v[0] = _mm256_sub_epi32(t, w);


		w = _mm256_mul_epi32(u, cdiv);
		t = _mm256_srli_epi64(w, 32);

		w = _mm256_srli_epi64(u, 32);
		w = _mm256_mul_epi32(w, cdiv);
		v[2] = _mm256_blend_epi32(t, w, 0xAA);

		//multiplying PARAM_Q
		w = _mm256_slli_epi32(v[2], 2);
		w = _mm256_add_epi32(w, v[2]);
		w = _mm256_slli_epi32(w, 7);
		w = _mm256_add_epi32(w, v[2]);

		v[1] = _mm256_sub_epi32(u, w);


		//rearrange the positions
		v[1] = _mm256_slli_epi32(v[1], 16);
		v[0] = _mm256_or_si256(v[0], v[1]);
		v[1] = _mm256_permute4x64_epi64(v[0], 0x4E);
		v[3] = _mm256_permute4x64_epi64(v[2], 0x9E);
		v[3] = _mm256_slli_epi32(v[3], 16);
		v[2] = _mm256_or_si256(v[2], v[3]);
		v[2] = _mm256_blend_epi32(v[2], v[1], 0x80);

		v[0] = _mm256_shuffle_epi8(v[0], idx0);
		v[3] = _mm256_shuffle_epi8(v[2], idx1);
		v[0] = _mm256_or_si256(v[0], v[3]);

		v[1] = _mm256_shuffle_epi8(v[1], idx2);
		v[3] = _mm256_shuffle_epi8(v[2], idx3);
		v[1] = _mm256_or_si256(v[1], v[3]);

		_mm256_storeu_si256((__m256i*) & r->coeffs[24 * j], v[0]);
		_mm_storeu_si128((__m128i*) & r->coeffs[24 * j + 16], _mm256_castsi256_si128(v[1]));
	}
#if PARAM_N%24 == 8
	decode6(&r->coeffs[24 * j], &a[28 * j]);
	r->coeffs[24 * j + 6] = a[28 * j + 7] | (((uint16_t)a[28 * j + 8] & 0x03) << 8);
	r->coeffs[24 * j + 7] = (a[28 * j + 8] >> 2) | (((uint16_t)a[28 * j + 9] & 0x0f) << 6);

#elif PARAM_N%24 == 16
	decode6(&r->coeffs[24 * j], &a[28 * j]);
	decode6(&r->coeffs[24 * j + 6], &a[28 * j + 7]);
	r->coeffs[24 * j + 12] = a[28 * j + 14] | (((uint16_t)a[28 * j + 15] & 0x03) << 8);
	r->coeffs[24 * j + 13] = (a[28 * j + 15] >> 2) | (((uint16_t)a[28 * j + 16] & 0x0f) << 6);
	r->coeffs[24 * j + 14] = (a[28 * j + 16] >> 4) | (((uint16_t)a[28 * j + 17] & 0x3f) << 4);
	r->coeffs[24 * j + 15] = (a[28 * j + 17] >> 6) | (((uint16_t)a[28 * j + 18]) << 2);
#endif
}

void poly_tobytes(uint8_t* r, const poly* a)
{
	__m256i v[6], w, t;
	ALIGN(32) int32_t buf[16];
	const __m256i mask16 = _mm256_set1_epi32(0xFFFF);
	const __m256i zero = _mm256_setzero_si256();
	const __m256i permu = _mm256_set_epi32(7, 6, 5, 4, 7, 6, 5, 4);
	const __m256i idx8 = _mm256_set_epi8(1, 0, 15, 15, 14, 13, 12, 11,
		10, 9, 8, 6, 5, 4, 3, 2,
		15, 15, 14, 13, 12, 11, 10, 9,
		8, 6, 5, 4, 3, 2, 1, 0);

	const __m256i idx0 = _mm256_set_epi8(128, 128, 11, 10, 128, 128, 5, 4,
		128, 128, 15, 14, 128, 128, 9, 8,
		128, 128, 3, 2, 128, 128, 13, 12,
		128, 128, 7, 6, 128, 128, 1, 0);

	const __m256i idx1 = _mm256_set_epi8(128, 128, 13, 12, 128, 128, 7, 6,
		128, 128, 1, 0, 128, 128, 11, 10,
		128, 128, 5, 4, 128, 128, 15, 14,
		128, 128, 9, 8, 128, 128, 3, 2);

	const __m256i idx2 = _mm256_set_epi8(128, 128, 15, 14, 128, 128, 9, 8,
		128, 128, 3, 2, 128, 128, 13, 12,
		128, 128, 7, 6, 128, 128, 1, 0,
		128, 128, 11, 10, 128, 128, 5, 4);

	int j;

	for (j = 0; j < PARAM_N / 24; j++)
	{
		v[0] = _mm256_loadu_si256((__m256i*) & a->coeffs[24 * j]);
		v[1] = _mm256_loadu_si256((__m256i*) & a->coeffs[24 * j + 8]);

		v[3] = _mm256_blend_epi16(v[0], v[1], 0x26);
		v[4] = _mm256_blend_epi16(v[0], v[1], 0x4D);
		v[5] = _mm256_blend_epi16(v[0], v[1], 0x9B);

		v[3] = _mm256_shuffle_epi8(v[3], idx0);
		v[4] = _mm256_shuffle_epi8(v[4], idx1);
		v[5] = _mm256_shuffle_epi8(v[5], idx2);

		//multiplying Q by shift and plus
		t = _mm256_slli_epi32(v[5], 2);
		t = _mm256_add_epi32(t, v[5]);
		t = _mm256_slli_epi32(t, 7);
		t = _mm256_add_epi32(t, v[5]);

		w = _mm256_add_epi32(t, v[4]);

		t = _mm256_slli_epi32(w, 2);
		t = _mm256_add_epi32(t, w);
		t = _mm256_slli_epi32(t, 7);
		t = _mm256_add_epi32(t, w);

		t = _mm256_add_epi32(t, v[3]);

		w = _mm256_blend_epi32(t, zero, 0xAA);
		t = _mm256_blend_epi32(zero, t, 0xAA);
		t = _mm256_srli_epi64(t, 4);
		t = _mm256_or_si256(w, t);


		t = _mm256_shuffle_epi8(t, idx8);
		w = _mm256_permutevar8x32_epi32(t, permu);
		w = _mm256_or_si256(w, t);
		t = _mm256_blend_epi32(t, w, 0xF8);

		_mm256_storeu_si256((__m256i*) & r[28 * j], t);
	}
#if PARAM_N%24 == 8
	encode6(&r[28 * j], &a->coeffs[24 * j]);
	r[28 * j + 7] = a->coeffs[24 * j + 6] & 0xff;
	r[28 * j + 8] = (a->coeffs[24 * j + 6] >> 8) | ((a->coeffs[24 * j + 7] & 0x3f) << 2);
	r[28 * j + 9] = a->coeffs[24 * j + 7] >> 6;
#elif PARAM_N%24 == 16
	encode6(&r[28 * j], &a->coeffs[24 * j]);
	encode6(&r[28 * j + 7], &a->coeffs[24 * j + 6]);
	r[28 * j + 14] = a->coeffs[24 * j + 12] & 0xff;
	r[28 * j + 15] = (a->coeffs[24 * j + 12] >> 8) | ((a->coeffs[24 * j + 13] & 0x3f) << 2);
	r[28 * j + 16] = (a->coeffs[24 * j + 13] >> 6) | ((a->coeffs[24 * j + 14] & 0x0f) << 4);
	r[28 * j + 17] = (a->coeffs[24 * j + 14] >> 4) | ((a->coeffs[24 * j + 15] & 0x03) << 6);
	r[28 * j + 18] = (a->coeffs[24 * j + 15] >> 2) & 0xff;
#endif
}

#elif  PARAM_Q == 1409

void poly_tobytes(uint8_t *r, const poly *a)
{
	int i;
	__m256i v, w, t;
	__m128i tail;

	const __m256i vmul = _mm256_setr_epi16(
		1, PARAM_Q, 1, PARAM_Q, 1, PARAM_Q, 1, PARAM_Q,
		1, PARAM_Q, 1, PARAM_Q, 1, PARAM_Q, 1, PARAM_Q);

	const __m256i idx0 = _mm256_setr_epi32(0, 3, 6, 0, 0, 0, 0, 0);
	const __m256i idx1 = _mm256_setr_epi32(1, 4, 7, 0, 0, 0, 0, 0);
	const __m256i idx2 = _mm256_setr_epi32(2, 5, 0, 0, 0, 0, 0, 0);
	const __m256i idx3 = _mm256_setr_epi32(3, 6, 0, 0, 0, 0, 0, 0);

	const __m256i shr0 = _mm256_setr_epi64x(0, 1, 2, 64);
	const __m256i shl1 = _mm256_setr_epi64x(21, 20, 19, 64);
	const __m256i shl2 = _mm256_setr_epi64x(42, 41, 64, 64);
	const __m256i shl3 = _mm256_setr_epi64x(63, 62, 64, 64);

	for (i = 0; i < PARAM_N / 16; ++i)
	{
		v = _mm256_loadu_si256((const __m256i *)&a->coeffs[16 * i]);
		
		v = _mm256_madd_epi16(v, vmul);

		w = _mm256_permutevar8x32_epi32(v, idx0);
		w = _mm256_cvtepu32_epi64(_mm256_castsi256_si128(w));
		w = _mm256_srlv_epi64(w, shr0);

		t = _mm256_permutevar8x32_epi32(v, idx1);
		t = _mm256_cvtepu32_epi64(_mm256_castsi256_si128(t));
		w = _mm256_or_si256(w, _mm256_sllv_epi64(t, shl1));

		t = _mm256_permutevar8x32_epi32(v, idx2);
		t = _mm256_cvtepu32_epi64(_mm256_castsi256_si128(t));
		w = _mm256_or_si256(w, _mm256_sllv_epi64(t, shl2));

		t = _mm256_permutevar8x32_epi32(v, idx3);
		t = _mm256_cvtepu32_epi64(_mm256_castsi256_si128(t));
		w = _mm256_or_si256(w, _mm256_sllv_epi64(t, shl3));

		_mm_storeu_si128((__m128i *)&r[21 * i], _mm256_castsi256_si128(w));

		tail = _mm256_extracti128_si256(w, 1);
		_mm_storeu_si32((void *)&r[21 * i + 16], tail);
		r[21 * i + 20] = (uint8_t)_mm_cvtsi128_si32(_mm_srli_si128(tail, 4));
	}
}

void poly_frombytes(poly *r, const uint8_t *a)
{
	int i;
	__m256i t, u, w;
	__m128i lo, hi;

	const __m256i idx = _mm256_setr_epi8(
		 0,  1,  2,  3,  2,  3,  4,  5,
		 5,  6,  7,  8,  7,  8,  9, 10,
		 5,  6,  7,  8,  8,  9, 10, 11,
		10, 11, 12, 13, 13, 14, 15, -1);

	const __m256i shift = _mm256_setr_epi32(0, 5, 2, 7, 4, 1, 6, 3);
	const __m256i mask21 = _mm256_set1_epi32(0x1fffff);
	const __m256i cdiv = _mm256_set1_epi32(3048238);
	const __m256i q8x = _mm256_set1_epi32(PARAM_Q);

	for (i = 0; i < PARAM_N / 16; ++i)
	{

		lo = _mm_loadu_si128((const __m128i *)&a[21 * i]);
		hi = _mm_loadu_si128((const __m128i *)&a[21 * i + 5]);

		t = _mm256_set_m128i(hi, lo);
		t = _mm256_shuffle_epi8(t, idx);
		t = _mm256_srlv_epi32(t, shift);
		t = _mm256_and_si256(t, mask21);

		u = _mm256_mul_epu32(t, cdiv);
		u = _mm256_srli_epi64(u, 32);

		w = _mm256_srli_epi64(t, 32);
		w = _mm256_mul_epu32(w, cdiv);

		u = _mm256_blend_epi32(u, w, 0xaa);

		w = _mm256_mullo_epi32(u, q8x);
		t = _mm256_sub_epi32(t, w);
		t = _mm256_or_si256(t, _mm256_slli_epi32(u, 16));

		_mm256_storeu_si256((__m256i *)&r->coeffs[16 * i], t);
	}
}
#elif  PARAM_Q == 3329

void poly_tobytes(uint8_t *r, const poly *a)
{
	int i;
	__m256i v, u, t;
	__m128i lo, hi;

	const __m256i masklo = _mm256_set1_epi32(0x0000ffff);
	const __m256i maskhi = _mm256_set1_epi32(0x00fff000);
	const __m256i idx = _mm256_set_epi8(-1, -1, -1, -1, 14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0, -1, -1, -1, -1, 14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0);

	for (i = 0; i < PARAM_N / 16; ++i)
	{
		v = _mm256_loadu_si256((const __m256i *)&a->coeffs[16 * i]);

		t = _mm256_and_si256(v, masklo);
		u = _mm256_srli_epi32(v, 4);
		u = _mm256_and_si256(u, maskhi);
		t = _mm256_or_si256(t, u);

		t = _mm256_shuffle_epi8(t, idx);

		lo = _mm256_castsi256_si128(t);
		hi = _mm256_extracti128_si256(t, 1);

		lo = _mm_or_si128(lo, _mm_slli_si128(hi, 12));
		_mm_storeu_si128((__m128i *)&r[24 * i], lo);

		hi = _mm_srli_si128(hi, 4);
		_mm_storel_epi64((__m128i *)&r[24 * i + 16], hi);
	}
}

void poly_frombytes(poly *r, const uint8_t *a)
{
	int i;
	__m256i t, u;
	__m128i lo, hi, tail;

	const __m256i mask12 = _mm256_set1_epi32(0x00000fff);
	const __m256i idx = _mm256_set_epi8(-1, 11, 10, 9, -1, 8, 7, 6, -1, 5, 4, 3, -1, 2, 1, 0, -1, 11, 10, 9, -1, 8, 7, 6, -1, 5, 4, 3, -1, 2, 1, 0);

	for (i = 0; i < PARAM_N / 16; ++i)
	{
		lo = _mm_loadu_si128((const __m128i *)&a[24 * i]);
		tail = _mm_loadl_epi64((const __m128i *)&a[24 * i + 16]);

		hi = _mm_srli_si128(lo, 12);
		hi = _mm_or_si128(hi, _mm_slli_si128(tail, 4));

		t = _mm256_set_m128i(hi, lo);
		t = _mm256_shuffle_epi8(t, idx);

		u = _mm256_and_si256(t, mask12);
		t = _mm256_srli_epi32(t, 12);
		t = _mm256_slli_epi32(t, 16);
		t = _mm256_or_si256(u, t);

		_mm256_storeu_si256((__m256i *)&r->coeffs[16 * i], t);
	}
}

#elif  PARAM_Q == 769

void encode5(uint8_t *buf, const uint16_t x[5])
{
	int i;
	uint32_t wl, wh;
	uint32_t t;
	uint32_t s;

	wl = x[0] & 0x07;
	for (i = 1; i < 5; i++)
		wl |= (uint32_t)(x[i] & 0x07) << (3 * i);

	wl <<= 1;
	wh = x[4] >> 3;
	for (i = 3; i > 0; i--)
		wh = (wh * 97) + (x[i] >> 3);

	t = wh * 48;
	wl |= t >> 31;
	t <<= 1;
	wh = wh + (x[0] >> 3);
	wh = t + wh;
	wl |= ((t & ~wh) >> 31);

	buf[0] = wl;
	buf[1] = wl >> 8;
	buf[2] = wh;
	for (i = 3; i < 6; i++)
	{
		wh = wh >> 8;
		buf[i] = wh;
	}
}
void decode5(uint16_t *x, const uint8_t *buf)
{
	/*
	 * Useful facts:
	 *    512 = 27 mod 97
	 *    2^19 = 3 mod 97
	 *    for any x in 0..57519, (43241 * x) >> 22 = floor(x / 97)
	 *    97 * 1594008481 = 1 mod 2^32
	 */

	uint32_t wl, wh, z;
	int i;

	wl = (uint32_t)(buf[1] << 8) | buf[0];
	wh = buf[5];
	for (i = 4; i > 1; i--)
		wh = (wh << 8) | (uint32_t)buf[i];

	z = (((wl & 0x01) << 13) | (wh >> 19)) * 3;
	z += wh & 0x7FFFF;
	wl >>= 1;

	z = (z >> 9) * 27 + (z & 0x1FF);
	z -= ((z * 43241) >> 22) * 97;

	x[0] = (z << 3) + (wl & 0x07);
	wl >>= 3;

	wh -= z;
	wh = wh * 1594008481u;

	z = (wh >> 19) * 3 + (wh & 0x7FFFF);
	z = (z >> 9) * 27 + (z & 0x1FF);
	z -= ((z * 43241) >> 22) * 97;

	x[1] = (z << 3) + (wl & 0x07);
	wl >>= 3;

	wh -= z;
	wh = wh * 1594008481u;

	z = (wh >> 9) * 27 + (wh & 0x1FF);
	z -= ((z * 43241) >> 22) * 97;

	x[2] = (z << 3) + (wl & 0x07);
	wl >>= 3;

	wh -= z;
	z = wh * 1594008481u;

	wh = (z * 43241) >> 22;
	z -= wh * 97;

	x[3] = (z << 3) + (wl & 0x07);
	wl >>= 3;

	x[4] = (wh << 3) + wl;

}

void poly_tobytes(uint8_t *r, const poly *a)
{
	int i;
	uint16_t t16[4];
	__m256i v01, v23, v34;
	__m256i x0, x1, x2, x3, x4;
	__m256i d0, d1, d2, d3;
	__m256i wl, wh, t, u, v;
	__m128i u0, u1, v0, v1;

	const __m256i idx5 = _mm256_setr_epi32(0, 5, 10, 15, 20, 25, 30, 35);
	const __m256i mask16 = _mm256_set1_epi32(0xffff);
	const __m256i mask3 = _mm256_set1_epi32(0x07);
	const __m256i c97 = _mm256_set1_epi32(97);
	const __m256i c48 = _mm256_set1_epi32(48);
	const __m256i idxpack = _mm256_setr_epi8(
		0, 1, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, -1, -1, -1, -1,
		0, 1, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, -1, -1, -1, -1);

	for (i = 0; i + 8 <= PARAM_N / 5; i += 8)
	{
		v01 = _mm256_i32gather_epi32((const int *)&a->coeffs[5 * i], idx5, 2);
		v23 = _mm256_i32gather_epi32((const int *)&a->coeffs[5 * i + 2], idx5, 2);
		v34 = _mm256_i32gather_epi32((const int *)&a->coeffs[5 * i + 3], idx5, 2);

		x0 = _mm256_and_si256(v01, mask16);
		x1 = _mm256_srli_epi32(v01, 16);
		x2 = _mm256_and_si256(v23, mask16);
		x3 = _mm256_srli_epi32(v23, 16);
		x4 = _mm256_srli_epi32(v34, 16);

		wl = _mm256_and_si256(x0, mask3);
		wl = _mm256_or_si256(wl, _mm256_slli_epi32(_mm256_and_si256(x1, mask3), 3));
		wl = _mm256_or_si256(wl, _mm256_slli_epi32(_mm256_and_si256(x2, mask3), 6));
		wl = _mm256_or_si256(wl, _mm256_slli_epi32(_mm256_and_si256(x3, mask3), 9));
		wl = _mm256_or_si256(wl, _mm256_slli_epi32(_mm256_and_si256(x4, mask3), 12));
		wl = _mm256_slli_epi32(wl, 1);

		d0 = _mm256_srli_epi32(x0, 3);
		d1 = _mm256_srli_epi32(x1, 3);
		d2 = _mm256_srli_epi32(x2, 3);
		d3 = _mm256_srli_epi32(x3, 3);

		wh = _mm256_srli_epi32(x4, 3);
		wh = _mm256_add_epi32(_mm256_mullo_epi32(wh, c97), d3);
		wh = _mm256_add_epi32(_mm256_mullo_epi32(wh, c97), d2);
		wh = _mm256_add_epi32(_mm256_mullo_epi32(wh, c97), d1);

		t = _mm256_mullo_epi32(wh, c48);
		wl = _mm256_or_si256(wl, _mm256_srli_epi32(t, 31));

		t = _mm256_slli_epi32(t, 1);
		wh = _mm256_add_epi32(t, _mm256_add_epi32(wh, d0));
		wl = _mm256_or_si256(wl, _mm256_srli_epi32(_mm256_andnot_si256(wh, t), 31));

		u = _mm256_unpacklo_epi32(wl, wh);
		v = _mm256_unpackhi_epi32(wl, wh);

		u = _mm256_shuffle_epi8(u, idxpack);
		v = _mm256_shuffle_epi8(v, idxpack);

		u0 = _mm256_castsi256_si128(u);
		u1 = _mm256_extracti128_si256(u, 1);
		v0 = _mm256_castsi256_si128(v);
		v1 = _mm256_extracti128_si256(v, 1);

		_mm_storeu_si128((__m128i *)&r[6 * i], u0);
		_mm_storeu_si128((__m128i *)&r[6 * i + 12], v0);
		_mm_storeu_si128((__m128i *)&r[6 * i + 24], u1);
		_mm_storel_epi64((__m128i *)&r[6 * i + 36], v1);
		_mm_storeu_si32((void *)&r[6 * i + 44], _mm_srli_si128(v1, 8));
	}

	for (; i < PARAM_N / 5; ++i)
		encode5(&r[6 * i], (const uint16_t *)&a->coeffs[5 * i]);

	t16[0] = (uint16_t)a->coeffs[5 * i];
	t16[1] = (uint16_t)a->coeffs[5 * i + 1];

#if PARAM_N == 512
	r[6 * i] = t16[0];
	r[6 * i + 1] = (t16[0] >> 8) | (t16[1] << 2);
	r[6 * i + 2] = t16[1] >> 6;

#elif PARAM_N == 1024
	t16[2] = (uint16_t)a->coeffs[5 * i + 2];
	t16[3] = (uint16_t)a->coeffs[5 * i + 3];

	r[6 * i] = t16[0];
	r[6 * i + 1] = (t16[0] >> 8) | (t16[1] << 2);
	r[6 * i + 2] = (t16[1] >> 6) | (t16[2] << 4);
	r[6 * i + 3] = (t16[2] >> 4) | (t16[3] << 6);
	r[6 * i + 4] = t16[3] >> 2;

#elif PARAM_N == 2048
	t16[2] = (uint16_t)a->coeffs[5 * i + 2];

	r[6 * i] = t16[0];
	r[6 * i + 1] = (t16[0] >> 8) | (t16[1] << 2);
	r[6 * i + 2] = (t16[1] >> 6) | (t16[2] << 4);
	r[6 * i + 3] = t16[2] >> 4;
#endif
}

void poly_frombytes(poly *r, const uint8_t *a)
{
	int i, j;
	__m256i wl, wh, z, q, t;
	__m256i x0, x1, x2, x3, x4;
	__m256i p01, p23, lo, hi, t0, t1;
	__m128i x4w;

	ALIGN(32) uint64_t tmp[8];
	ALIGN(16) uint16_t last[8];

	const __m256i idx6 = _mm256_setr_epi32(0, 6, 12, 18, 24, 30, 36, 42);
	const __m256i mask16 = _mm256_set1_epi32(0xffff);
	const __m256i mask1 = _mm256_set1_epi32(0x01);
	const __m256i mask3 = _mm256_set1_epi32(0x07);
	const __m256i mask9 = _mm256_set1_epi32(0x1ff);
	const __m256i mask19 = _mm256_set1_epi32(0x7ffff);
	const __m256i c27 = _mm256_set1_epi32(27);
	const __m256i c97 = _mm256_set1_epi32(97);
	const __m256i cdiv = _mm256_set1_epi32(43241);
	const __m256i cinv = _mm256_set1_epi32(1594008481u);

	for (i = 0; i + 8 <= PARAM_N / 5; i += 8)
	{
		wl = _mm256_i32gather_epi32((const int *)&a[6 * i], idx6, 1);
		wh = _mm256_i32gather_epi32((const int *)&a[6 * i + 2], idx6, 1);
		wl = _mm256_and_si256(wl, mask16);

		z = _mm256_or_si256(_mm256_slli_epi32(_mm256_and_si256(wl, mask1), 13),
			_mm256_srli_epi32(wh, 19));
		z = _mm256_add_epi32(z, _mm256_slli_epi32(z, 1));
		z = _mm256_add_epi32(z, _mm256_and_si256(wh, mask19));
		wl = _mm256_srli_epi32(wl, 1);

		q = _mm256_srli_epi32(z, 9);
		z = _mm256_add_epi32(_mm256_mullo_epi32(q, c27), _mm256_and_si256(z, mask9));
		q = _mm256_srli_epi32(_mm256_mullo_epi32(z, cdiv), 22);
		z = _mm256_sub_epi32(z, _mm256_mullo_epi32(q, c97));

		x0 = _mm256_or_si256(_mm256_slli_epi32(z, 3), _mm256_and_si256(wl, mask3));
		wl = _mm256_srli_epi32(wl, 3);

		wh = _mm256_sub_epi32(wh, z);
		wh = _mm256_mullo_epi32(wh, cinv);

		q = _mm256_srli_epi32(wh, 19);
		z = _mm256_add_epi32(q, _mm256_slli_epi32(q, 1));
		z = _mm256_add_epi32(z, _mm256_and_si256(wh, mask19));

		q = _mm256_srli_epi32(z, 9);
		z = _mm256_add_epi32(_mm256_mullo_epi32(q, c27), _mm256_and_si256(z, mask9));
		q = _mm256_srli_epi32(_mm256_mullo_epi32(z, cdiv), 22);
		z = _mm256_sub_epi32(z, _mm256_mullo_epi32(q, c97));

		x1 = _mm256_or_si256(_mm256_slli_epi32(z, 3), _mm256_and_si256(wl, mask3));
		wl = _mm256_srli_epi32(wl, 3);

		wh = _mm256_sub_epi32(wh, z);
		wh = _mm256_mullo_epi32(wh, cinv);

		q = _mm256_srli_epi32(wh, 9);
		z = _mm256_add_epi32(_mm256_mullo_epi32(q, c27), _mm256_and_si256(wh, mask9));
		q = _mm256_srli_epi32(_mm256_mullo_epi32(z, cdiv), 22);
		z = _mm256_sub_epi32(z, _mm256_mullo_epi32(q, c97));

		x2 = _mm256_or_si256(_mm256_slli_epi32(z, 3), _mm256_and_si256(wl, mask3));
		wl = _mm256_srli_epi32(wl, 3);

		wh = _mm256_sub_epi32(wh, z);
		wh = _mm256_mullo_epi32(wh, cinv);

		q = _mm256_srli_epi32(_mm256_mullo_epi32(wh, cdiv), 22);
		z = _mm256_sub_epi32(wh, _mm256_mullo_epi32(q, c97));

		x3 = _mm256_or_si256(_mm256_slli_epi32(z, 3), _mm256_and_si256(wl, mask3));
		wl = _mm256_srli_epi32(wl, 3);
		x4 = _mm256_add_epi32(_mm256_slli_epi32(q, 3), wl);

		p01 = _mm256_or_si256(x0, _mm256_slli_epi32(x1, 16));
		p23 = _mm256_or_si256(x2, _mm256_slli_epi32(x3, 16));

		lo = _mm256_unpacklo_epi32(p01, p23);
		hi = _mm256_unpackhi_epi32(p01, p23);

		t0 = _mm256_permute2x128_si256(lo, hi, 0x20);
		t1 = _mm256_permute2x128_si256(lo, hi, 0x31);

		_mm256_store_si256((__m256i *)&tmp[0], t0);
		_mm256_store_si256((__m256i *)&tmp[4], t1);

		x4w = _mm_packus_epi32(_mm256_castsi256_si128(x4), _mm256_extracti128_si256(x4, 1));
		_mm_store_si128((__m128i *)last, x4w);

		for (j = 0; j < 8; ++j)
		{
			r->coeffs[5 * (i + j)] = (int16_t)tmp[j];
			r->coeffs[5 * (i + j) + 1] = (int16_t)(tmp[j] >> 16);
			r->coeffs[5 * (i + j) + 2] = (int16_t)(tmp[j] >> 32);
			r->coeffs[5 * (i + j) + 3] = (int16_t)(tmp[j] >> 48);
			r->coeffs[5 * (i + j) + 4] = (int16_t)last[j];
		}
	}

	for (; i < PARAM_N / 5; ++i)
		decode5((uint16_t *)&r->coeffs[5 * i], &a[6 * i]);

	r->coeffs[5 * i] =
		(int16_t)((uint16_t)a[6 * i]
		| (((uint16_t)a[6 * i + 1] & 0x03) << 8));

	r->coeffs[5 * i + 1] =
		(int16_t)(((uint16_t)a[6 * i + 1] >> 2)
		| (((uint16_t)a[6 * i + 2] & 0x0f) << 6));

#if PARAM_N == 1024
	r->coeffs[5 * i + 2] =
		(int16_t)(((uint16_t)a[6 * i + 2] >> 4)
		| (((uint16_t)a[6 * i + 3] & 0x3f) << 4));

	r->coeffs[5 * i + 3] =
		(int16_t)(((uint16_t)a[6 * i + 3] >> 6)
		| ((uint16_t)a[6 * i + 4] << 2));

#elif PARAM_N == 2048
	r->coeffs[5 * i + 2] =
		(int16_t)(((uint16_t)a[6 * i + 2] >> 4)
		| (((uint16_t)a[6 * i + 3] & 0x3f) << 4));
#endif
}

#endif


void poly_compress(uint8_t *r, const poly *a)
{
	unsigned int i;
	uint32_t t;

	for (i = 0; i < PARAM_N; ++i) {
		t = ((uint32_t)a->coeffs[i] * 341U + 469U) >> 10;
		r[i] = (uint8_t)(t & 0xff);
	}
}

void poly_decompress(poly *r, const uint8_t *a)
{
	unsigned int i;

	for (i = 0; i < PARAM_N; ++i) {
		r->coeffs[i] = (int16_t)(((uint32_t)a[i] * PARAM_Q + (1U << 7)) >> 8);
	}
}