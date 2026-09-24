#include "hashkdf.h"
#include <string.h>
#include "auxfunc.h"


#ifdef USE_SHA3
#include "fips202x4.h"
void kdf128x4(uint8_t *out0,
	uint8_t *out1,
	uint8_t *out2,
	uint8_t *out3, int outlen,
	const uint8_t *in0,
	const uint8_t *in1,
	const uint8_t *in2,
	const uint8_t *in3, int inlen)
{
shake128x4(out0, out1, out2, out3, outlen, in0, in1, in2, in3, inlen);
}

void kdf256x4(uint8_t *out0,
	uint8_t *out1,
	uint8_t *out2,
	uint8_t *out3, int outlen,
	const uint8_t *in0,
	const uint8_t *in1,
	const uint8_t *in2,
	const uint8_t *in3, int inlen)
{
	shake256x4(out0, out1, out2, out3, outlen, in0, in1, in2, in3, inlen);
}

void kdf512x4(uint8_t* out0,
	uint8_t* out1,
	uint8_t* out2,
	uint8_t* out3, int outlen,
	const uint8_t* in0,
	const uint8_t* in1,
	const uint8_t* in2,
	const uint8_t* in3, int inlen)
{
	shake512x4(out0, out1, out2, out3, outlen, in0, in1, in2, in3, inlen);
}
#endif


void kdf128(uint8_t *out, int outlen, uint8_t *in, int inlen)
{
#ifdef USE_ICCS
	pseudoXOF(outlen * 8, in, inlen * 8, out);
#elif defined(USE_SHA3)
	shake128(out, outlen, in, inlen);
#endif
}

void kdf256(uint8_t *out, int outlen, uint8_t *in, int inlen)
{
#ifdef USE_ICCS
	pseudoXOF(outlen * 8, in, inlen * 8, out);
#elif defined(USE_SHA3)
	shake256(out, outlen, in, inlen);
#endif
}

void kdf512(uint8_t* out, int outlen, uint8_t* in, int inlen)
{
#ifdef USE_ICCS
	pseudoXOF(outlen * 8, in, inlen * 8, out);
#elif defined(USE_SHA3)
	shake512(out, outlen, in, inlen);
#endif
}

void hash256(uint8_t *out, uint8_t *in, int inlen)
{
#ifdef USE_ICCS
	sm3hash(256, in, inlen * 8, out);
#else
	sha3_256(out, in, inlen);
#endif
}

void hash512(uint8_t *out, uint8_t *in, int inlen)
{
#ifdef USE_ICCS
	pseudohash(512, in, inlen * 8, out);
#else
	sha3_512(out, in, inlen);
#endif
}

void hash1024(uint8_t* out, uint8_t* in, int inlen)
{
#ifdef USE_ICCS
	pseudohash(1024, in, inlen * 8, out);
#else
	sha3_1024(out, in, inlen);
#endif
}

#ifdef USE_ICCS

void kdf_init(kdfstate* state, const uint8_t* input, int inlen) {
	init_random_number(state, input, inlen);
}

void kdf_squeezeblocks(uint8_t* output, int nblocks, kdfstate* state)
{
	get_random_number(state, output, nblocks * 256);
}
#endif

void kdf128_absorb(kdfstate* state, uint8_t* input, int inlen)
{
#ifdef USE_SHA3
	shake128_absorb(*state, input, inlen);
#endif
}

void kdf128_squeezeblocks(uint8_t* output, int nblocks, kdfstate* state)
{
#ifdef USE_SHA3
	shake128_squeezeblocks(output, nblocks, *state); 
#endif
}

void kdf256_absorb(kdfstate * state, uint8_t *input, int inlen)
{
#ifdef USE_SHA3
	shake256_absorb(*state, input, inlen);
#endif
}
void kdf256_squeezeblocks(uint8_t *output, int nblocks, kdfstate * state)
{
#ifdef USE_SHA3
	shake256_squeezeblocks(output, nblocks, *state);
#endif
}

void kdf512_absorb(kdfstate* state, uint8_t* input, int inlen)
{
#ifdef USE_SHA3
	shake512_absorb(*state, input, inlen);
#endif
}
void kdf512_squeezeblocks(uint8_t* output, int nblocks, kdfstate* state)
{
#ifdef USE_SHA3
	shake512_squeezeblocks(output, nblocks, *state);
#endif
}
