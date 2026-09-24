#include "aes.h"

unsigned char aes_round_constants[] = {
	0x01, 0x02, 0x04, 0x08,
	0x10, 0x20, 0x40, 0x80,
	0x1b, 0x36, 0x6c, 0xd8,
	0xab, 0x4d
};

extern inline void aes_round_function(
	const aes_round_keys* restrict round_keys, block128* restrict block,
	block128* restrict after_sbox, int round);

extern ALWAYS_INLINE void aes_keygen_ctr(
	aes_round_keys* restrict aeses, const block_secpar* restrict keys, const block128* restrict ivs,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block128* restrict output);

extern inline void aes_ctr(
	const aes_round_keys* restrict aeses,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block128* restrict output);

extern inline void aes_fixed_key_ctr(
	const aes_round_keys* restrict fixed_key, const block128* restrict keys,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block128* restrict output);
extern inline void rijndael192_fixed_key_ctr(
	const rijndael192_round_keys* restrict fixed_key, const block192* restrict keys,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block192* restrict output);
extern inline void rijndael256_fixed_key_ctr(
	const rijndael256_round_keys* restrict fixed_key, const block256* restrict keys,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block256* restrict output);

#if SECURITY_PARAM == 128
extern inline void rijndael_keygen(rijndael_round_keys* round_keys, block_secpar key);
extern inline void rijndael_fixed_key_ctr(
	const rijndael_round_keys* restrict fixed_key, const block_secpar* restrict keys,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block_secpar* restrict output);

#elif SECURITY_PARAM == 192
extern inline void rijndael_keygen(rijndael_round_keys* round_keys, block_secpar key);
extern inline void rijndael_fixed_key_ctr(
	const rijndael_round_keys* restrict fixed_key, const block_secpar* restrict keys,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block_secpar* restrict output);

#elif SECURITY_PARAM == 256
extern inline void rijndael_keygen(rijndael_round_keys* round_keys, block_secpar key);
extern inline void rijndael_fixed_key_ctr(
	const rijndael_round_keys* restrict fixed_key, const block_secpar* restrict keys,
	size_t num_keys, uint32_t num_blocks, uint32_t counter, block_secpar* restrict output);

#endif
