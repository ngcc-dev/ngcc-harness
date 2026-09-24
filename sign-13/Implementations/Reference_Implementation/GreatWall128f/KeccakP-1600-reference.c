#include "KeccakP-1600-SnP.h"

#include <stdint.h>
#include <string.h>

static uint64_t rotl64(uint64_t x, unsigned int n)
{
	return n ? ((x << n) | (x >> (64 - n))) : x;
}

static void keccakp1600_permute(uint64_t st[25], unsigned int nrounds)
{
	static const uint64_t round_constants[24] = {
		UINT64_C(0x0000000000000001), UINT64_C(0x0000000000008082),
		UINT64_C(0x800000000000808a), UINT64_C(0x8000000080008000),
		UINT64_C(0x000000000000808b), UINT64_C(0x0000000080000001),
		UINT64_C(0x8000000080008081), UINT64_C(0x8000000000008009),
		UINT64_C(0x000000000000008a), UINT64_C(0x0000000000000088),
		UINT64_C(0x0000000080008009), UINT64_C(0x000000008000000a),
		UINT64_C(0x000000008000808b), UINT64_C(0x800000000000008b),
		UINT64_C(0x8000000000008089), UINT64_C(0x8000000000008003),
		UINT64_C(0x8000000000008002), UINT64_C(0x8000000000000080),
		UINT64_C(0x000000000000800a), UINT64_C(0x800000008000000a),
		UINT64_C(0x8000000080008081), UINT64_C(0x8000000000008080),
		UINT64_C(0x0000000080000001), UINT64_C(0x8000000080008008),
	};
	static const unsigned int rho[24] = {
		1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14,
		27, 41, 56, 8, 25, 43, 62, 18, 39, 61, 20, 44,
	};
	static const unsigned int pi[24] = {
		10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4,
		15, 23, 19, 13, 12, 2, 20, 14, 22, 9, 6, 1,
	};

	if (nrounds > 24)
		nrounds = 24;

	for (unsigned int round = 24 - nrounds; round < 24; ++round) {
		uint64_t c[5];
		uint64_t t;

		for (unsigned int x = 0; x < 5; ++x)
			c[x] = st[x] ^ st[x + 5] ^ st[x + 10] ^ st[x + 15] ^ st[x + 20];

		for (unsigned int x = 0; x < 5; ++x) {
			t = c[(x + 4) % 5] ^ rotl64(c[(x + 1) % 5], 1);
			for (unsigned int y = 0; y < 25; y += 5)
				st[y + x] ^= t;
		}

		t = st[1];
		for (unsigned int x = 0; x < 24; ++x) {
			const unsigned int y = pi[x];
			c[0] = st[y];
			st[y] = rotl64(t, rho[x]);
			t = c[0];
		}

		for (unsigned int y = 0; y < 25; y += 5) {
			for (unsigned int x = 0; x < 5; ++x)
				c[x] = st[y + x];
			for (unsigned int x = 0; x < 5; ++x)
				st[y + x] ^= (~c[(x + 1) % 5]) & c[(x + 2) % 5];
		}

		st[0] ^= round_constants[round];
	}
}

void KeccakP1600_Initialize(void* state)
{
	memset(state, 0, KeccakP1600_stateSizeInBytes);
}

void KeccakP1600_AddByte(void* state, unsigned char data, unsigned int offset)
{
	((unsigned char*)state)[offset] ^= data;
}

void KeccakP1600_AddBytes(
	void* state, const unsigned char* data, unsigned int offset, unsigned int length)
{
	unsigned char* bytes = (unsigned char*)state + offset;
	for (unsigned int i = 0; i < length; ++i)
		bytes[i] ^= data[i];
}

void KeccakP1600_OverwriteBytes(
	void* state, const unsigned char* data, unsigned int offset, unsigned int length)
{
	memcpy((unsigned char*)state + offset, data, length);
}

void KeccakP1600_OverwriteWithZeroes(void* state, unsigned int byteCount)
{
	memset(state, 0, byteCount);
}

void KeccakP1600_Permute_Nrounds(void* state, unsigned int nrounds)
{
	keccakp1600_permute((uint64_t*)state, nrounds);
}

void KeccakP1600_Permute_12rounds(void* state)
{
	KeccakP1600_Permute_Nrounds(state, 12);
}

void KeccakP1600_Permute_24rounds(void* state)
{
	KeccakP1600_Permute_Nrounds(state, 24);
}

void KeccakP1600_ExtractBytes(
	const void* state, unsigned char* data, unsigned int offset, unsigned int length)
{
	memcpy(data, (const unsigned char*)state + offset, length);
}

void KeccakP1600_ExtractAndAddBytes(
	const void* state, const unsigned char* input, unsigned char* output,
	unsigned int offset, unsigned int length)
{
	const unsigned char* bytes = (const unsigned char*)state + offset;
	for (unsigned int i = 0; i < length; ++i)
		output[i] = input[i] ^ bytes[i];
}
