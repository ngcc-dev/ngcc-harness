#ifndef HASH_H
#define HASH_H

#include <inttypes.h>
#include <stddef.h>

#include "config.h"
#include "block.h"
#include "KeccakHash.h"


typedef Keccak_HashInstance hash_state;

inline int hash_init(hash_state* ctx)
{
#if SECURITY_PARAM <= 128
	return Keccak_HashInitialize_SHAKE128(ctx);
#else
	return Keccak_HashInitialize_SHAKE256(ctx);
#endif
}

inline int hash_update(hash_state* ctx, const void* input, size_t bytes)
{
	return Keccak_HashUpdate(ctx, (const uint8_t*) input, bytes * 8);
}

inline int hash_update_byte(hash_state* ctx, uint8_t b)
{
	return hash_update(ctx, &b, 1);
}

inline int hash_final(hash_state* ctx, void* digest, size_t bytes)
{
	int ret = Keccak_HashFinal(ctx, NULL);
	if (ret != KECCAK_SUCCESS)
		return ret;
	return Keccak_HashSqueeze(ctx, (uint8_t*) digest, bytes * 8);
}

typedef struct
{
	hash_state state[4];
} hash_state_x4;

inline void hash_init_x4(hash_state_x4* ctx)
{
	for (size_t i = 0; i < 4; ++i)
		hash_init(&ctx->state[i]);
}

inline void hash_update_x4(hash_state_x4* ctx, const void** data, size_t size)
{
	for (size_t i = 0; i < 4; ++i)
		hash_update(&ctx->state[i], data[i], size);
}

inline void hash_update_x4_4(
	hash_state_x4* ctx,
	const void* data0, const void* data1, const void* data2, const void* data3, size_t size)
{
	const void* data[4] = {data0, data1, data2, data3};
	hash_update_x4(ctx, data, size);
}

inline void hash_update_x4_1(hash_state_x4* ctx, const void* data, size_t size)
{
	hash_update_x4_4(ctx, data, data, data, data, size);
}

inline void hash_update_x4_1_byte(hash_state_x4* ctx, uint8_t b)
{
	hash_update_x4_1(ctx, &b, 1);
}

inline void hash_init_prefix_x4(hash_state_x4* ctx, const uint8_t prefix)
{
	hash_init_x4(ctx);
	hash_update_x4_1(ctx, &prefix, sizeof(prefix));
}

inline void hash_final_x4(hash_state_x4* ctx, void** buffer, size_t buflen)
{
	for (size_t i = 0; i < 4; ++i)
		hash_final(&ctx->state[i], buffer[i], buflen);
}

inline void hash_final_x4_4(
	hash_state_x4* ctx, void* buffer0, void* buffer1, void* buffer2, void* buffer3, size_t buflen)
{
	void* buffer[4] = {buffer0, buffer1, buffer2, buffer3};
	hash_final_x4(ctx, buffer, buflen);
}

inline void shake_prg(
	const block_secpar* restrict keys, const block128* restrict ivs,
	size_t num_keys, size_t num_bytes, uint8_t* restrict output)
{
	size_t i;
	const size_t num_keys_x4 = num_keys & ~(size_t)3;
	for (i = 0; i < num_keys_x4; i += 4)
	{
		const void* key_arr[4];
		const void* iv_arr[4];
		void* output_arr[4];
		for (size_t j = 0; j < 4; ++j)
		{
			key_arr[j] = &keys[i + j];
			iv_arr[j] = &ivs[i + j];
			output_arr[j] = output + (i + j) * num_bytes;
		}

		hash_state_x4 hasher;
		hash_init_x4(&hasher);
		hash_update_x4(&hasher, key_arr, sizeof(keys[i]));
		hash_update_x4(&hasher, iv_arr, sizeof(ivs[i]));
		hash_update_x4_1_byte(&hasher, 0);
		hash_final_x4(&hasher, output_arr, num_bytes);
	}

	for (; i < num_keys; ++i)
	{
		hash_state hasher;
		hash_init(&hasher);
		hash_update(&hasher, &keys[i], sizeof(keys[i]));
		hash_update(&hasher, &ivs[i], sizeof(ivs[i]));
		hash_update_byte(&hasher, 0);
		hash_final(&hasher, output + i * num_bytes, num_bytes);
	}
}

#endif

