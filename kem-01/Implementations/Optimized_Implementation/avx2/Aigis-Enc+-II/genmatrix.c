#include <immintrin.h>
#include <string.h>
#include "genmatrix.h"
#include "polyvec.h"
#include "hashkdf.h"
#include "fips202x4.h"

static int rej_uniform_avx(uint16_t *r, int *cur, int n, const uint8_t *buf, int buflen){
    unsigned int ctr, pos;
    uint16_t val0, val1;
    uint32_t good;
    uint64_t idx0, idx1, idx2, idx3;
	const __m256i bound  = _mm256_set1_epi16(PARAM_Q);
    const __m256i ones   = _mm256_set1_epi8(1);
    const __m256i mask  = _mm256_set1_epi16(0xFFF);
    const __m256i idx8  = _mm256_set_epi8(15, 14, 14, 13, 12, 11, 11, 10,
                                          9, 8, 8, 7, 6, 5, 5, 4,
                                          11, 10, 10, 9, 8, 7, 7, 6,
                                          5, 4, 4, 3, 2, 1, 1, 0);
    __m256i f0, f1, g0, g1, g2, g3;

	pos = 0;
	ctr = *cur;
    while (ctr <= n - 32 && pos <= buflen - 48) {
        f0 = _mm256_loadu_si256((__m256i *)&buf[pos]);
        f1 = _mm256_loadu_si256((__m256i *)&buf[pos + 24]);
        f0 = _mm256_permute4x64_epi64(f0, 0x94);
        f1 = _mm256_permute4x64_epi64(f1, 0x94);
        f0 = _mm256_shuffle_epi8(f0, idx8);
        f1 = _mm256_shuffle_epi8(f1, idx8);
        g0 = _mm256_srli_epi16(f0, 4);
        g1 = _mm256_srli_epi16(f1, 4);
        f0 = _mm256_blend_epi16(f0, g0, 0xAA);
        f1 = _mm256_blend_epi16(f1, g1, 0xAA);
        f0 = _mm256_and_si256(f0, mask);
        f1 = _mm256_and_si256(f1, mask);
        pos += 48;

        g0 = _mm256_cmpgt_epi16(bound, f0);
        g1 = _mm256_cmpgt_epi16(bound, f1);

        g0 = _mm256_packs_epi16(g0, g1);
        good = _mm256_movemask_epi8(g0);

        idx0 = _pdep_u64(good >>  0, 0x0101010101010101);
        idx1 = _pdep_u64(good >>  8, 0x0101010101010101);
        idx2 = _pdep_u64(good >> 16, 0x0101010101010101);
        idx3 = _pdep_u64(good >> 24, 0x0101010101010101);
        idx0 = (idx0 << 8) - idx0;
        idx0  = _pext_u64(0x0E0C0A0806040200, idx0);
        idx1 = (idx1 << 8) - idx1;
        idx1  = _pext_u64(0x0E0C0A0806040200, idx1);
        idx2 = (idx2 << 8) - idx2;
        idx2  = _pext_u64(0x0E0C0A0806040200, idx2);
        idx3 = (idx3 << 8) - idx3;
        idx3  = _pext_u64(0x0E0C0A0806040200, idx3);

        g0 = _mm256_castsi128_si256(_mm_cvtsi64_si128(idx0));
        g1 = _mm256_castsi128_si256(_mm_cvtsi64_si128(idx1));
        g0 = _mm256_inserti128_si256(g0, _mm_cvtsi64_si128(idx2), 1);
        g1 = _mm256_inserti128_si256(g1, _mm_cvtsi64_si128(idx3), 1);

        g2 = _mm256_add_epi8(g0, ones);
        g3 = _mm256_add_epi8(g1, ones);
        g0 = _mm256_unpacklo_epi8(g0, g2);
        g1 = _mm256_unpacklo_epi8(g1, g3);

        f0 = _mm256_shuffle_epi8(f0, g0);
        f1 = _mm256_shuffle_epi8(f1, g1);

        _mm_storeu_si128((__m128i *)&r[ctr], _mm256_castsi256_si128(f0));
        ctr += _mm_popcnt_u32((good >>  0) & 0xFF);
        _mm_storeu_si128((__m128i *)&r[ctr], _mm256_extracti128_si256(f0, 1));
        ctr += _mm_popcnt_u32((good >> 16) & 0xFF);
        _mm_storeu_si128((__m128i *)&r[ctr], _mm256_castsi256_si128(f1));
        ctr += _mm_popcnt_u32((good >>  8) & 0xFF);
        _mm_storeu_si128((__m128i *)&r[ctr], _mm256_extracti128_si256(f1, 1));
        ctr += _mm_popcnt_u32((good >> 24) & 0xFF);
    }

    while (ctr < n && pos <= buflen - 3) {
        val0 = ((buf[pos + 0] >> 0) | ((uint16_t)buf[pos + 1] << 8)) & 0xFFF;
        val1 = ((buf[pos + 1] >> 4) | ((uint16_t)buf[pos + 2] << 4));
        pos += 3;

        if (val0 < PARAM_Q) {
            r[ctr++] = val0;
        }
        if (val1 < PARAM_Q && ctr < n) {
            r[ctr++] = val1;
        }
    }
	*cur = ctr;
    return pos;
}

static int rej_uniform(uint16_t *r, int *cur, int n, const uint8_t *buf, int buflen)
{
	int ctr, pos;
	int16_t val0, val1;
	ctr = *cur;
	pos = 0;

	while (((ctr + 2) < n) && (pos + 3 <= buflen))
	{
		val0 = (buf[pos] | ((uint16_t)buf[pos + 1] << 8)) & 0xfff;
		val1 = (buf[pos + 1] >> 4) | ((uint16_t)buf[pos + 2] << 4);
		pos += 3;

		if (val0 < PARAM_Q)
			r[ctr++] = val0;
		if (val1 < PARAM_Q)
			r[ctr++] = val1;
	}
	while ((ctr < n) && (pos + 3 <= buflen))
	{
		val0 = (buf[pos] | ((uint16_t)buf[pos + 1] << 8)) & 0xfff;
		val1 = (buf[pos + 1] >> 4) | ((uint16_t)buf[pos + 2] << 4);
		pos += 3;

		if (val0 < PARAM_Q && ctr < n)
			r[ctr++] = val0;
		if (val1 < PARAM_Q && ctr < n)
			r[ctr++] = val1;
	}
	*cur = ctr;
	return pos;
}
#ifdef USE_NICCS_API

void poly_uniform_seed(uint16_t *r,int n, const uint8_t *seed, int seedbytes)
{
	int cur = 0, off;
	int buflen = 3 * KDF128RATE;
	uint8_t buf[buflen];

	int len = buflen;
	kdfstate ctx;
	kdf_init(&ctx, seed, seedbytes);

	while (cur + 21 < n)
	{
		kdf_squeezeblocks(buf, 3, &ctx);
		rej_uniform(r, &cur, n, buf, len);
	}

	while (cur < n)
	{
		off = len & 0x3;
		for (int i = 0; i < off; i++)
			buf[i] = buf[len - off + 1];
		kdf_squeezeblocks(&buf[off], 1, &ctx);
		len = off + KDF128RATE;
		rej_uniform(r, &cur, n, buf, len);
	}
}

void gen_a(poly *a, const uint8_t *seed)
{
	int quarter_n = PARAM_N/4;
	uint8_t buf[SEED_BYTES + 1];
	memcpy(buf, seed, SEED_BYTES);
	for (int i = 0; i < 4; ++i) {
		buf[SEED_BYTES] = i;
		poly_uniform_seed(a->coeffs + i * quarter_n, quarter_n, buf, SEED_BYTES + 1);
	}
}



#elif defined(USE_SM3)

void poly_uniform_seed(poly *r, const uint8_t *seed, int seedbytes)
{
	int cur = 0, off;
	int buflen = 3 * KDF128RATE;
	uint8_t buf[buflen];

	int len = buflen;
	kdfstate ctx;
	kdf128_absorb(&ctx, seed, seedbytes);

	while (cur + 21 < PARAM_N)
	{
		kdf128_squeezeblocks(buf, 3, &ctx);
		rej_uniform(r->coeffs, &cur, PARAM_N, buf, len);
	}

	while (cur < PARAM_N)
	{
		off = len & 0x3;
		for (int i = 0; i < off; i++)
			buf[i] = buf[len - off + 1];
		kdf128_squeezeblocks(&buf[off], 1, &ctx);
		len = off + KDF128RATE;
		rej_uniform(r->coeffs, &cur, PARAM_N, buf, len);
	}
}

#elif defined(USE_SHA3)

void poly_uniform_seed(poly *r, const uint8_t *seed, int seedbytes)
{
	int cur = 0;
	uint8_t buf[KDF128RATE];
	int len;
	kdfstate state;
	kdf128_absorb(&state, seed, seedbytes);

	while (cur < PARAM_N)
	{
		kdf128_squeezeblocks(buf, 1, &state);
		len = KDF128RATE;
		rej_uniform(r->coeffs, &cur, PARAM_N, buf, len);
	}
}

void poly_uniform_seedx4(int16_t *r, const uint8_t *seed)
{
#define  QUARTER_N  (PARAM_N/4)
	ALIGN(32) uint8_t buf[4][KDF128RATE];
	keccakx4_state state;
	int ctr0,ctr1,ctr2,ctr3;
	ctr0=ctr1=ctr2=ctr3 = 0;

	memcpy(buf[0],seed,SEED_BYTES);
	memcpy(buf[1],seed,SEED_BYTES);
	memcpy(buf[2],seed,SEED_BYTES);
	memcpy(buf[3],seed,SEED_BYTES);
	buf[0][SEED_BYTES] = 0;
	buf[1][SEED_BYTES] = 1;
	buf[2][SEED_BYTES] = 2;
	buf[3][SEED_BYTES] = 3;

	shake128_absorb4x(state.s, buf[0],buf[1],buf[2],buf[3],SEED_BYTES+1);
	shake128_squeezeblocks4x(buf[0],buf[1],buf[2],buf[3],1,state.s);

	rej_uniform_avx(r, &ctr0, QUARTER_N, buf[0], KDF128RATE);
	rej_uniform_avx(r + QUARTER_N, &ctr1, QUARTER_N, buf[1], KDF128RATE);
	rej_uniform_avx(r + 2 * QUARTER_N, &ctr2, QUARTER_N, buf[2], KDF128RATE);
	rej_uniform_avx(r + 3 * QUARTER_N, &ctr3, QUARTER_N, buf[3], KDF128RATE);

	while (ctr0 < QUARTER_N || ctr1 < QUARTER_N || ctr2 < QUARTER_N || ctr3 < QUARTER_N  )
	{
		shake128_squeezeblocks4x(buf[0],buf[1],buf[2],buf[3],1,state.s);

		rej_uniform_avx(r, &ctr0, QUARTER_N, buf[0], KDF128RATE);
		rej_uniform_avx(r + QUARTER_N, &ctr1, QUARTER_N, buf[1], KDF128RATE);
		rej_uniform_avx(r + 2 * QUARTER_N, &ctr2, QUARTER_N, buf[2], KDF128RATE);
		rej_uniform_avx(r + 3 * QUARTER_N, &ctr3, QUARTER_N, buf[3], KDF128RATE);
	}
}

void gen_a(poly *a, const uint8_t *seed)
{
	poly_uniform_seedx4(a->coeffs, seed);
}

#endif


