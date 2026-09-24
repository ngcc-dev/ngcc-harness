
#include "sample.h"
#include <immintrin.h>
#include "fips202x4.h"

ALIGN(32) static const uint64_t idx[256][4] = {
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

int32_t rej_uniform_s(int32_t * a, int32_t *cur, uint8_t *buf, int32_t buflen)
{
	int32_t ctr = *cur, pos = 0;

    if(ctr >= PARAM_N) 
        return buflen;

	uint32_t t[4];
    uint32_t v;
	__m256i tmp, index, d;
	__m256i q8x = _mm256_set1_epi32(PARAM_Q);

	__m128i tmp0, index0, d0;
	__m256i mask = _mm256_set1_epi32(0x3FFFFF);
	const __m256i idx8 = _mm256_set_epi8(14, 13, 12, 11, 11, 10, 9, 8,
										 8, 7, 6, 5, 6, 5, 4, 3,
										 11, 10, 9, 8, 8, 7, 6, 5,
										 5, 4, 3, 2, 3, 2, 1, 0);
	const __m256i shift = _mm256_set_epi32(2, 4, 6, 0, 2, 4, 6, 0);
	const __m256i permu = _mm256_set_epi32(5, 4, 3, 2, 3, 2, 1, 0);

	while (pos + 22 <= buflen)
	{
		d = _mm256_loadu_si256((__m256i *)&buf[pos]);
		tmp = _mm256_permutevar8x32_epi32(d, permu);
		tmp = _mm256_shuffle_epi8(tmp, idx8);
		tmp = _mm256_srlv_epi32(tmp, shift);
		tmp = _mm256_and_si256(tmp, mask);

		index = _mm256_cmpgt_epi32(q8x, tmp);
		v = _mm256_movemask_ps(_mm256_castsi256_ps(index));
		if (v == 255)
		{
			_mm256_storeu_si256((__m256i *)&a[ctr], tmp);
			ctr += 8;
		}
		else
		{
			index = _mm256_load_si256((__m256i *)idx[v]);
			tmp = _mm256_permutevar8x32_epi32(tmp, index);
			_mm256_storeu_si256((__m256i *)&a[ctr], tmp);
			ctr += _mm_popcnt_u32(v);
		}

		pos += 22;
	}

    *cur = ctr;
	return pos;
}

int32_t rej_uniform_avx2(int32_t * a, int32_t *cur, uint8_t *buf, int32_t buflen)
{
	int32_t ctr = *cur, pos = 0;

    if(ctr >= PARAM_N) 
        return buflen;

	uint32_t t[4];
    uint32_t v;
	__m256i tmp, index, d;
	__m256i q8x = _mm256_set1_epi32(PARAM_Q);

	__m128i tmp0, index0, d0;
	__m256i mask = _mm256_set1_epi32(0x3FFFFF);
	const __m256i idx8 = _mm256_set_epi8(14, 13, 12, 11, 11, 10, 9, 8,
										 8, 7, 6, 5, 6, 5, 4, 3,
										 11, 10, 9, 8, 8, 7, 6, 5,
										 5, 4, 3, 2, 3, 2, 1, 0);
	const __m256i shift = _mm256_set_epi32(2, 4, 6, 0, 2, 4, 6, 0);
	const __m256i permu = _mm256_set_epi32(5, 4, 3, 2, 3, 2, 1, 0);

	while (ctr + 8 <= PARAM_N && pos + 22 <= buflen)
	{
		d = _mm256_loadu_si256((__m256i *)&buf[pos]);
		tmp = _mm256_permutevar8x32_epi32(d, permu);
		tmp = _mm256_shuffle_epi8(tmp, idx8);
		tmp = _mm256_srlv_epi32(tmp, shift);
		tmp = _mm256_and_si256(tmp, mask);

		index = _mm256_cmpgt_epi32(q8x, tmp);
		v = _mm256_movemask_ps(_mm256_castsi256_ps(index));
		if (v == 255)
		{
			_mm256_storeu_si256((__m256i *)&a[ctr], tmp);
			ctr += 8;
		}
		else
		{
			index = _mm256_load_si256((__m256i *)idx[v]);
			tmp = _mm256_permutevar8x32_epi32(tmp, index);
			_mm256_storeu_si256((__m256i *)&a[ctr], tmp);
			ctr += _mm_popcnt_u32(v);
		}

		pos += 22;
	}
	while (pos + 11 <= buflen && ctr + 4 <= PARAM_N)
	{
		d0 = _mm_loadu_si128((__m128i *)&buf[pos]);
		tmp0 = _mm_shuffle_epi8(d0, _mm256_castsi256_si128(idx8));
		tmp0 = _mm_srlv_epi32(tmp0, _mm256_castsi256_si128(shift));
		tmp0 = _mm_and_si128(tmp0, _mm256_castsi256_si128(mask));

		index0 = _mm_cmpgt_epi32(_mm256_castsi256_si128(q8x), tmp0);

		v = _mm_movemask_ps(_mm_castsi128_ps(index0));

		index0 = _mm_loadu_si128((__m128i *)idx[v]);
		tmp0 = _mm_castps_si128(_mm_permutevar_ps(_mm_castsi128_ps(tmp0), index0));

		_mm_storeu_si128((__m128i *)&a[ctr], tmp0);

		ctr += _mm_popcnt_u32(v);

		pos += 11;
	}
	while ((ctr > PARAM_N - 4) && (ctr < PARAM_N) && (pos + 11 <= buflen))
	{
		t[0] = buf[pos++];
		t[0] |= (uint32_t)buf[pos++] << 8;
		t[0] |= (uint32_t)buf[pos] << 16;
		t[0] &= 0x3FFFFF;
		if (t[0] < PARAM_Q)
			a[ctr++] = t[0];
		if (ctr == PARAM_N)
		{
			*cur = ctr;
			return pos;
		}

		t[1] = buf[pos++] >> 6;
		t[1] |= (uint32_t)buf[pos++] << 2;
		t[1] |= (uint32_t)buf[pos++] << 10;
		t[1] |= (uint32_t)buf[pos] << 18;
		t[1] &= 0x3FFFFF;
		if (t[1] < PARAM_Q)
			a[ctr++] = t[1];
		if (ctr == PARAM_N)
		{
			*cur = ctr;
			return pos;
		}

		t[2] = buf[pos++] >> 4;
		t[2] |= (uint32_t)buf[pos++] << 4;
		t[2] |= (uint32_t)buf[pos++] << 12;
		t[2] |= (uint32_t)buf[pos] << 20;
		t[2] &= 0x3FFFFF;
		if (t[2] < PARAM_Q)
			a[ctr++] = t[2];
		if (ctr == PARAM_N)
		{
			*cur = ctr;
			return pos;
		}

		t[3] = buf[pos++] >> 2;
		t[3] |= (uint32_t)buf[pos++] << 6;
		t[3] |= (uint32_t)buf[pos++] << 14;
		t[3] &= 0x3FFFFF;
		if (t[3] < PARAM_Q)
			a[ctr++] = t[3];
		if (ctr == PARAM_N)
		{
			*cur = ctr;
			return pos;
		}

	}
    *cur = ctr;
	return pos;
}


static int reject_uniform_ref(int32_t *a, int32_t *cur, int32_t n, const uint8_t *buf, int32_t buflen)
{
	int ctr, pos;
	uint32_t t;

	ctr = *cur;
	pos = 0;
	while (ctr < n && pos + 3 <= buflen)
	{
		t = buf[pos++];
		t |= (uint32_t)buf[pos++] << 8;
		t |= (uint32_t)buf[pos] << 16;
		t &= 0x3FFFFF;
		if (t < PARAM_Q)
			a[ctr++] = t;

		if (ctr == n || pos + 4 > buflen)
		{
			*cur = ctr;
			return pos + 1;
		}

		t = buf[pos++] >> 6;
		t |= (uint32_t)buf[pos++] << 2;
		t |= (uint32_t)buf[pos++] << 10;
		t |= (uint32_t)buf[pos] << 18;
		t &= 0x3FFFFF;
		if (t < PARAM_Q)
			a[ctr++] = t;

		if (ctr == n || pos + 4 > buflen)
		{
			*cur = ctr;
			return pos + 1;
		}

		t = buf[pos++] >> 4;
		t |= (uint32_t)buf[pos++] << 4;
		t |= (uint32_t)buf[pos++] << 12;
		t |= (uint32_t)buf[pos] << 20;
		t &= 0x3FFFFF;
		if (t < PARAM_Q)
			a[ctr++] = t;

		if (ctr == n || pos + 3 > buflen)
		{
			*cur = ctr;
			return pos + 1;
		}

		t = buf[pos++] >> 2;
		t |= (uint32_t)buf[pos++] << 6;
		t |= (uint32_t)buf[pos++] << 14;
		t &= 0x3FFFFF;
		if (t < PARAM_Q)
			a[ctr++] = t;
	}
	*cur = ctr;
	return pos;
}

#define REJ_UNIFORM_BYTES 1440 // fail with prob. less than 2^-14

#ifdef USE_ICCS

void poly_uniform_seed(poly *a, const uint8_t *seed, int32_t seedbytes)
{
	int cur = 0, pos, step;
	ALIGN(32)
	uint8_t buf[REJ_UNIFORM_BYTES + KDF128RATE];
	int nblock = (REJ_UNIFORM_BYTES + KDF128RATE - 1) / KDF128RATE;

	int len;
	kdfstate state;
	kdf_init(&state, seed, seedbytes);
	kdf_squeezeblocks(buf, nblock, &state);
	len = nblock * KDF128RATE;
	pos = rej_uniform_avx2(a->coeffs, &cur, buf, len);

	while (cur < PARAM_N)
	{
		len = len - pos;
		memmove(buf, buf + pos, len);
		kdf_squeezeblocks(buf + len, 1, &state);
		len += KDF128RATE;
		pos = reject_uniform_ref(a->coeffs, &cur, PARAM_N, buf, len);
	}
}

#else

void poly_uniform_seed(poly *a, const uint8_t *seed, int32_t seedbytes)
{
	int cur = 0, pos, step;
	ALIGN(32)
	uint8_t buf[REJ_UNIFORM_BYTES + KDF128RATE];
	int nblock = (REJ_UNIFORM_BYTES + KDF128RATE - 1) / KDF128RATE;

	int len;
	kdfstate state;
	kdf128_absorb(&state, seed, seedbytes);
	kdf128_squeezeblocks(buf, nblock, &state);
	len = nblock * KDF128RATE;
	pos = rej_uniform_avx2(a->coeffs, &cur,  buf, len);

	while (cur < PARAM_N)
	{
		len = len - pos;
		memcpy(buf, buf + pos, len);
		kdf128_squeezeblocks(buf + len, 1, &state);
		len += KDF128RATE;
		pos = reject_uniform_ref(a->coeffs, &cur, PARAM_N, buf, len);
	}
}

#endif

#ifdef USE_SHA3

static int32_t reject_uniform(int32_t * a, int32_t *cur, int32_t n, uint8_t *buf, int32_t buflen)
{
	int32_t ctr, pos;
	uint32_t t[8];

	ctr = *cur;
    if(ctr >= n) {
        return buflen;
    }
	pos = 0;

	while (ctr + 4 <= n && pos + 11 <= buflen)
	{
        t[0] = buf[pos] | ((uint32_t)buf[pos + 1] << 8) | ((uint32_t)buf[pos + 2] << 16);
        t[0] &= 0x3FFFFF;

        t[1] = (buf[pos + 2] >> 6) | ((uint32_t)buf[pos + 3] << 2) | ((uint32_t)buf[pos + 4] << 10) | ((uint32_t)buf[pos + 5] << 18);
        t[1] &= 0x3FFFFF;

        t[2] = (buf[pos + 5] >> 4) | ((uint32_t)buf[pos + 6] << 4) | ((uint32_t)buf[pos + 7] << 12) | ((uint32_t)buf[pos + 8] << 20);
        t[2] &= 0x3FFFFF;

        t[3] = (buf[pos + 8] >> 2) | ((uint32_t)buf[pos + 9] << 6) | ((uint32_t)buf[pos + 10] << 14);
        t[3] &= 0x3FFFFF;

        pos += 11;

		if (t[0] < PARAM_Q)
			a[ctr++] = t[0];
		if (t[1] < PARAM_Q)
			a[ctr++] = t[1];
		if (t[2] < PARAM_Q)
			a[ctr++] = t[2];
		if (t[3] < PARAM_Q)
			a[ctr++] = t[3];
	}
	while (ctr < n && pos + 11 <= buflen)
	{
		t[0] = buf[pos++];
		t[0] |= (uint32_t)buf[pos++] << 8;
		t[0] |= (uint32_t)buf[pos] << 16;
		t[0] &= 0x3FFFFF;
		if (t[0] < PARAM_Q)
			a[ctr++] = t[0];
		if (ctr == n)
		{
			*cur = ctr;
			return pos;
		}

		t[1] = buf[pos++] >> 6;
		t[1] |= (uint32_t)buf[pos++] << 2;
		t[1] |= (uint32_t)buf[pos++] << 10;
		t[1] |= (uint32_t)buf[pos] << 18;
		t[1] &= 0x3FFFFF;
		if (t[1] < PARAM_Q)
			a[ctr++] = t[1];
		if (ctr == n)
		{
			*cur = ctr;
			return pos;
		}

		t[2] = buf[pos++] >> 4;
		t[2] |= (uint32_t)buf[pos++] << 4;
		t[2] |= (uint32_t)buf[pos++] << 12;
		t[2] |= (uint32_t)buf[pos] << 20;
		t[2] &= 0x3FFFFF;
		if (t[2] < PARAM_Q)
			a[ctr++] = t[2];
		if (ctr == n)
		{
			*cur = ctr;
			return pos;
		}

		t[3] = buf[pos++] >> 2;
		t[3] |= (uint32_t)buf[pos++] << 6;
		t[3] |= (uint32_t)buf[pos++] << 14;
		t[3] &= 0x3FFFFF;
		if (t[3] < PARAM_Q)
			a[ctr++] = t[3];
		if (ctr == n)
		{
			*cur = ctr;
			return pos;
		}

	}

	*cur = ctr;
	return pos;

}


static void uniform_x4(keccakx4_state *state, poly *r0, poly *r1, poly *r2, poly *r3,
     const uint8_t rho[SEEDBYTES], 
    uint8_t nonce0, uint8_t nonce1, uint8_t nonce2,uint8_t nonce3) {
    int ctr0 = 0, ctr1 = 0, ctr2 = 0, ctr3 = 0;
    int len0, len1 , len2, len3;
    int pos0, pos1 , pos2, pos3;
	int nblock = (REJ_UNIFORM_BYTES) / KDF128RATE;
	ALIGN(32) uint8_t buf[4][nblock * SHAKE128_RATE];
    const int nonce_pos = SEEDBYTES >> 3;

    uint64_t *rho64 = rho;
    state->s[0] = _mm256_set1_epi64x(rho64[0]);
    state->s[1] = _mm256_set1_epi64x(rho64[1]);
    state->s[2] = _mm256_set1_epi64x(rho64[2]);
    state->s[3] = _mm256_set1_epi64x(rho64[3]);
#if SEEDBYTES == 64
    state->s[4] = _mm256_set1_epi64x(rho64[4]);
    state->s[5] = _mm256_set1_epi64x(rho64[5]);
    state->s[6] = _mm256_set1_epi64x(rho64[6]);
    state->s[7] = _mm256_set1_epi64x(rho64[7]);
#endif
    state->s[nonce_pos] = _mm256_set_epi64x((0x1f << 8) | nonce3, (0x1f << 8) | nonce2, (0x1f << 8) | nonce1, (0x1f << 8) | nonce0);
    for (int i = nonce_pos + 1; i < 25; i++)
        state->s[i] = _mm256_setzero_si256();

    state->s[20] = _mm256_set1_epi64x(0x1ULL << 63);

    shake128_squeezeblocks4x(buf[0], buf[1], buf[2],  buf[3], nblock, state->s);

    len0 = nblock * SHAKE128_RATE;
    len1 = nblock * SHAKE128_RATE;
    len2 = nblock * SHAKE128_RATE;
    len3 = nblock * SHAKE128_RATE;

    pos0 = rej_uniform_s(r0->coeffs,  &ctr0, buf[0], len0);
    pos1 = rej_uniform_s(r1->coeffs,  &ctr1, buf[1], len1);
    pos2 = rej_uniform_s(r2->coeffs,  &ctr2, buf[2], len2);
    pos3 = rej_uniform_s(r3->coeffs,  &ctr3, buf[3], len3);

    while (ctr0 < PARAM_N || ctr1 < PARAM_N || ctr2 < PARAM_N || ctr3 < PARAM_N)
    {
        len0 -= pos0;
        len1 -= pos1;
        len2 -= pos2;
        len3 -= pos3;

        memmove(buf[0],buf[0]+pos0, len0);
        memmove(buf[1],buf[1]+pos1, len1);
        memmove(buf[2],buf[2]+pos2, len2);
        memmove(buf[3],buf[3]+pos3, len3);

        shake128_squeezeblocks4x(buf[0] + len0, buf[1] + len1, buf[2] + len2,  buf[3] + len3, 1, state->s);

        len0 += SHAKE128_RATE;
        len1 += SHAKE128_RATE;
        len2 += SHAKE128_RATE;
        len3 += SHAKE128_RATE;

        pos0 = rej_uniform_avx2(r0->coeffs,  &ctr0,  buf[0], len0);
        pos1 = rej_uniform_avx2(r1->coeffs,  &ctr1,  buf[1], len1);
        pos2 = rej_uniform_avx2(r2->coeffs,  &ctr2,  buf[2], len2);
        pos3 = rej_uniform_avx2(r3->coeffs,  &ctr3,  buf[3], len3);
    }
    
    
}


#endif

#ifdef USE_SHA3


#if PARAM_K == 2 && PARAM_L == 2


void expand_mat(polyvecl mat[PARAM_K], const unsigned char rho[SEEDBYTES])
{
	keccakx4_state state;
    uniform_x4(&state, &mat[0].vec[0], &mat[0].vec[1], &mat[1].vec[0], &mat[1].vec[1],
    rho, 0, 1, 1 << 4 , (1 << 4) | 1);
}

#elif PARAM_K == 4

void expand_mat(polyvecl mat[PARAM_K], const unsigned char rho[SEEDBYTES])
{
	keccakx4_state state;
	uniform_x4(&state, &mat[0].vec[0], &mat[0].vec[1], &mat[0].vec[2], &mat[0].vec[3],
			   rho, 0, 1, 2, 3);
	uniform_x4(&state, &mat[1].vec[0], &mat[1].vec[1], &mat[1].vec[2], &mat[1].vec[3],
			   rho, (1 << 4) | 0, (1 << 4) | 1, (1 << 4) | 2, (1 << 4) | 3);
	uniform_x4(&state, &mat[2].vec[0], &mat[2].vec[1], &mat[2].vec[2], &mat[2].vec[3],
			   rho, (2 << 4) | 0, (2 << 4) | 1, (2 << 4) | 2, (2 << 4) | 3);
	uniform_x4(&state, &mat[3].vec[0], &mat[3].vec[1], &mat[3].vec[2], &mat[3].vec[3],
			   rho, (3 << 4) | 0, (3 << 4) | 1, (3 << 4) | 2, (3 << 4) | 3);
}
#elif PARAM_K == 8

void expand_mat(polyvecl mat[PARAM_K], const unsigned char rho[SEEDBYTES])
{
	keccakx4_state state;
	uniform_x4(&state, &mat[0].vec[0], &mat[0].vec[1], &mat[0].vec[2], &mat[0].vec[3],
			   rho, 0, 1, 2, 3);
	uniform_x4(&state, &mat[0].vec[4], &mat[0].vec[5], &mat[0].vec[6], &mat[1].vec[0],
			   rho, 4, 5, 6, (1 << 4) | 0);
	uniform_x4(&state, &mat[1].vec[1], &mat[1].vec[2], &mat[1].vec[3], &mat[1].vec[4],
			   rho, (1 << 4) | 1, (1 << 4) | 2, (1 << 4) | 3, (1 << 4) | 4);
	uniform_x4(&state, &mat[1].vec[5], &mat[1].vec[6], &mat[2].vec[0], &mat[2].vec[1],
			   rho, (1 << 4) | 5, (1 << 4) | 6, (2 << 4) | 0, (2 << 4) | 1);
	uniform_x4(&state, &mat[2].vec[2], &mat[2].vec[3], &mat[2].vec[4], &mat[2].vec[5],
			   rho, (2 << 4) | 2, (2 << 4) | 3, (2 << 4) | 4, (2 << 4) | 5);
	uniform_x4(&state, &mat[2].vec[6], &mat[3].vec[0], &mat[3].vec[1], &mat[3].vec[2],
			   rho, (2 << 4) | 6, (3 << 4) | 0, (3 << 4) | 1, (3 << 4) | 2);
	uniform_x4(&state, &mat[3].vec[3], &mat[3].vec[4], &mat[3].vec[5], &mat[3].vec[6],
			   rho, (3 << 4) | 3, (3 << 4) | 4, (3 << 4) | 5, (3 << 4) | 6);

	uniform_x4(&state, &mat[4].vec[0], &mat[4].vec[1], &mat[4].vec[2], &mat[4].vec[3],
			   rho, (4 << 4) | 0, (4 << 4) | 1, (4 << 4) | 2, (4 << 4) | 3);
	uniform_x4(&state, &mat[4].vec[4], &mat[4].vec[5], &mat[4].vec[6], &mat[5].vec[0],
			   rho, (4 << 4) | 4, (4 << 4) | 5, (4 << 4) | 6, (5 << 4) | 0);
	uniform_x4(&state, &mat[5].vec[1], &mat[5].vec[2], &mat[5].vec[3], &mat[5].vec[4],
			   rho, (5 << 4) | 1, (5 << 4) | 2, (5 << 4) | 3, (5 << 4) | 4);
	uniform_x4(&state, &mat[5].vec[5], &mat[5].vec[6], &mat[6].vec[0], &mat[6].vec[1],
			   rho, (5 << 4) | 5, (5 << 4) | 6, (6 << 4) | 0, (6 << 4) | 1);
	uniform_x4(&state, &mat[6].vec[2], &mat[6].vec[3], &mat[6].vec[4], &mat[6].vec[5],
			   rho, (6 << 4) | 2, (6 << 4) | 3, (6 << 4) | 4, (6 << 4) | 5);
	uniform_x4(&state, &mat[6].vec[6], &mat[7].vec[0], &mat[7].vec[1], &mat[7].vec[2],
			   rho, (6 << 4) | 6, (7 << 4) | 0, (7 << 4) | 1, (7 << 4) | 2);
	uniform_x4(&state, &mat[7].vec[3], &mat[7].vec[4], &mat[7].vec[5], &mat[7].vec[6],
			   rho, (7 << 4) | 3, (7 << 4) | 4, (7 << 4) | 5,(7 << 4) | 6);
}
#endif

#else

static int32_t reject_uniform(int32_t * a, int32_t *cur, int32_t n, uint8_t *buf, int32_t buflen)
{
	int32_t ctr, pos;
	uint32_t t[4];
	ctr = *cur;
	pos = 0;

	while (ctr + 4 <= n && pos + 11 <= buflen)
	{
        t[0] = buf[pos] | ((uint32_t)buf[pos + 1] << 8) | ((uint32_t)buf[pos + 2] << 16);
        t[0] &= 0x3FFFFF;

        t[1] = (buf[pos + 2] >> 6) | ((uint32_t)buf[pos + 3] << 2) | ((uint32_t)buf[pos + 4] << 10) | ((uint32_t)buf[pos + 5] << 18);
        t[1] &= 0x3FFFFF;

        t[2] = (buf[pos + 5] >> 4) | ((uint32_t)buf[pos + 6] << 4) | ((uint32_t)buf[pos + 7] << 12) | ((uint32_t)buf[pos + 8] << 20);
        t[2] &= 0x3FFFFF;

        t[3] = (buf[pos + 8] >> 2) | ((uint32_t)buf[pos + 9] << 6) | ((uint32_t)buf[pos + 10] << 14);
        t[3] &= 0x3FFFFF;

        pos += 11;

		if (t[0] < PARAM_Q)
			a[ctr++] = t[0];
		if (t[1] < PARAM_Q)
			a[ctr++] = t[1];
		if (t[2] < PARAM_Q)
			a[ctr++] = t[2];
		if (t[3] < PARAM_Q)
			a[ctr++] = t[3];
	}
	while ( ctr < n && pos + 11 <= buflen)
	{

        t[0] = buf[pos] | ((uint32_t)buf[pos + 1] << 8) | ((uint32_t)buf[pos + 2] << 16);
        t[0] &= 0x3FFFFF;

        t[1] = (buf[pos + 2] >> 6) | ((uint32_t)buf[pos + 3] << 2) | ((uint32_t)buf[pos + 4] << 10) | ((uint32_t)buf[pos + 5] << 18);
        t[1] &= 0x3FFFFF;

        t[2] = (buf[pos + 5] >> 4) | ((uint32_t)buf[pos + 6] << 4) | ((uint32_t)buf[pos + 7] << 12) | ((uint32_t)buf[pos + 8] << 20);
        t[2] &= 0x3FFFFF;

        t[3] = (buf[pos + 8] >> 2) | ((uint32_t)buf[pos + 9] << 6) | ((uint32_t)buf[pos + 10] << 14);
        t[3] &= 0x3FFFFF;

		t[0] = buf[pos++];
		t[0] |= (uint32_t)buf[pos++] << 8;
		t[0] |= (uint32_t)buf[pos] << 16;
		t[0] &= 0x3FFFFF;

        pos += 11;

		if (t[0] < PARAM_Q)
			a[ctr++] = t[0];
		if (ctr == n)
			break;

		if (t[1] < PARAM_Q)
			a[ctr++] = t[1];
		if (ctr == n)
			break;

		if (t[2] < PARAM_Q)
			a[ctr++] = t[2];
		if (ctr== n)
			break;

		if (t[3] < PARAM_Q)
			a[ctr++] = t[3];
		if (ctr == n)
			break;
	}
	*cur = ctr;
	return pos;

}

void poly_uniform(poly *a, uint8_t *seed, uint32_t seedbytes)
{
	int cur = 0, pos, step, off;
	uint8_t buf[352];//32 * 11 bytes
	kdfstate ctx;
	kdf_init(&ctx, seed, seedbytes);

	int len;
	len = 352;

	kdf_squeezeblocks(buf, 11, &ctx);
	reject_uniform(a->coeffs, &cur, PARAM_N, buf, len);

	kdf_squeezeblocks(buf, 11, &ctx);
	reject_uniform(a->coeffs, &cur, PARAM_N, buf, len);

	kdf_squeezeblocks(buf, 11, &ctx);
	reject_uniform(a->coeffs, &cur, PARAM_N, buf, len);

	kdf_squeezeblocks(buf, 11, &ctx);
	pos = reject_uniform(a->coeffs, &cur, PARAM_N, buf, len);
	while (cur < PARAM_N)
	{
		len = len - pos;
		memcpy(buf, buf + pos, len);
		kdf_squeezeblocks(buf + len, 1, &ctx);
		len += KDF128RATE;
		pos = reject_uniform(a->coeffs, &cur, PARAM_N, buf, len);

	}
}

void expand_mat(polyvecl mat[PARAM_K], const unsigned char rho[SEEDBYTES])
{
	unsigned int i, j;
	unsigned char inbuf[SEEDBYTES + 1];
	for (i = 0; i < SEEDBYTES; ++i)
		inbuf[i] = rho[i];

	for (i = 0; i < PARAM_K; i++)
	{
		for (j = 0; j < PARAM_L; j++)
		{
			inbuf[SEEDBYTES] = (i<<4) | j;
			poly_uniform(&mat[i].vec[j], inbuf, SEEDBYTES + 1);

		}
	}
}
#endif





/*generate the the challenge c*/
#if PARAM_C <= 64
void challenge(uint8_t *seed, const unsigned char mu[CRHBYTES],
	const polyveck *w1)
{
	int i;
	ALIGN(32) uint8_t buf[CRHBYTES + PARAM_K * POLW1_SIZE_PACKED];
	for (i = 0; i < CRHBYTES; ++i)
		buf[i] = mu[i];
	for (i = 0; i < PARAM_K; ++i)
		polyw1_pack(buf + CRHBYTES + i * POLW1_SIZE_PACKED, w1->vec + i);
	Hash(seed, buf, sizeof(buf));
}

void unpack_c(poly *c, const uint8_t seed[SEEDBYTES])
{
	int32_t b;
	unsigned char outbuf[128 + KDF_RATE];
	int i, j, pos, buflen = 128;
	int nblocks = buflen / KDF_RATE;
	if (buflen % KDF_RATE != 0)
		nblocks++;
	kdfstate state;
	uint64_t signs;

	unsigned char extmask[8] = { 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff };

#ifdef USE_ICCS
  kdf_init(&state, seed, 32);
  kdf_squeezeblocks(outbuf, nblocks, &state);
#else
	KDF_ABSORB(&state, seed, 32);
	KDF_SQUEEZEBLOCK(outbuf, nblocks, &state);
#endif

	signs = 0;
	for (i = 0; i < (PARAM_C + 7) / 8; ++i)
		signs |= (uint64_t)outbuf[i] << 8 * i;

	pos = (PARAM_C + 7) / 8;

	for (i = 0; i < PARAM_N; ++i)
		c->coeffs[i] = 0;

	j = 0;
	for (i = PARAM_N - PARAM_C; i < PARAM_N; ++i) {
		do {
			if (pos >= buflen - 2) {
        #ifdef USE_ICCS
        kdf_squeezeblocks(outbuf, 1, &state);
        #else 
				KDF_SQUEEZEBLOCK(outbuf, 1, &state);
        #endif
				pos = 0;
				buflen = KDF_RATE;
			}

			b = outbuf[pos++] >> j;
			b |= (uint32_t)(outbuf[pos] & extmask[j]) << (8 - j);
			j = (j + 1) % 8;
			if (j == 0)
				pos++;
		} while (b > i);

		c->coeffs[i] = c->coeffs[b];
		c->coeffs[b] = 1 - 2 * (signs & 1);
		signs = 0;
	}
}

#elif PARAM_C > 64
void challenge(uint8_t* seed, const unsigned char mu[CRHBYTES],
	const polyveck* w1)
{
	int i;
	ALIGN(32) uint8_t buf[CRHBYTES + PARAM_K * POLW1_SIZE_PACKED];
	for (i = 0; i < CRHBYTES; ++i)
		buf[i] = mu[i];
	for (i = 0; i < PARAM_K; ++i)
		polyw1_pack(buf + CRHBYTES + i * POLW1_SIZE_PACKED, w1->vec + i);

	hash512(seed, buf, sizeof(buf));

}
void unpack_c(poly* c, const uint8_t seed[64])
{
	int32_t b;
	unsigned char outbuf[256 + KDF_RATE];
	int i, j, k, pos, buflen = 256;
	int nblocks = buflen / KDF_RATE;
	if (buflen % KDF_RATE != 0)
		nblocks++;
	kdfstate state;
	unsigned char signs[(PARAM_C + 7) / 8];

	unsigned char extmask[8] = { 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff };

  #ifdef USE_ICCS
  kdf_init(&state, seed, 64);
  kdf_squeezeblocks(outbuf, nblocks, &state);
#else
	KDF_ABSORB(&state, seed, 64);
	KDF_SQUEEZEBLOCK(outbuf, nblocks, &state);
#endif

	pos = (PARAM_C + 7) / 8;
	memcpy(signs, outbuf, pos);

	for (i = 0; i < PARAM_N; ++i)
		c->coeffs[i] = 0;

	j = 0;
	i = PARAM_N - PARAM_C;
	for (k = 0; k < PARAM_C; k++)
	{
		do {
			if (pos >= buflen - 2) {
        #ifdef USE_ICCS
        kdf_squeezeblocks(outbuf, 1, &state);
        #else 
				KDF_SQUEEZEBLOCK(outbuf, 1, &state);
        #endif
				pos = 0;
				buflen = KDF_RATE;
			}

			b = outbuf[pos++] >> j;
			b |= (uint32_t)(outbuf[pos] & extmask[j]) << (8 - j);
			j = (j + 1) % 8;
			if (j == 0)
				pos++;
		} while (b > i);
		c->coeffs[i++] = c->coeffs[b];
		c->coeffs[b] = 1 - 2 * (((uint32_t)signs[k / 8] >> (k % 8)) & 1);
	}
}
#endif
