#include "hash.h"

extern inline int hash_init(hash_state* ctx);
extern inline int hash_update(hash_state* ctx, const void* input, size_t bytes);
extern inline int hash_update_byte(hash_state* ctx, uint8_t b);
extern inline int hash_final(hash_state* ctx, void* digest, size_t bytes);
extern inline void hash_init_x4(hash_state_x4* ctx);
extern inline void hash_update_x4(hash_state_x4* ctx, const void** data, size_t size);
extern inline void hash_update_x4_4(
	hash_state_x4* ctx,
	const void* data0, const void* data1, const void* data2, const void* data3, size_t size);
extern inline void hash_update_x4_1(hash_state_x4* ctx, const void* data, size_t size);
extern inline void hash_update_x4_1_byte(hash_state_x4* ctx, uint8_t b);
extern inline void hash_init_prefix_x4(hash_state_x4* ctx, const uint8_t prefix);
extern inline void hash_final_x4(hash_state_x4* ctx, void** buffer, size_t buflen);
extern inline void hash_final_x4_4(
	hash_state_x4* ctx, void* buffer0, void* buffer1, void* buffer2, void* buffer3, size_t buflen);
extern inline void shake_prg(
	const block_secpar* restrict keys, const block128* restrict ivs,
	size_t num_keys, size_t num_bytes, uint8_t* restrict output);
