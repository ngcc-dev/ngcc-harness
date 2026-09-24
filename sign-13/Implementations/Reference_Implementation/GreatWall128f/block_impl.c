#include "block.h"

extern inline block128 block128_xor(block128 x, block128 y);
extern inline block256 block256_xor(block256 x, block256 y);
extern inline block128 block128_and(block128 x, block128 y);
extern inline block256 block256_and(block256 x, block256 y);
extern inline block128 block128_set_zero();
extern inline block256 block256_set_zero();
extern inline block128 block128_set_all_8(uint8_t x);
extern inline block256 block256_set_all_8(uint8_t x);
extern inline block128 block128_set_low32(uint32_t x);
extern inline block256 block256_set_low32(uint32_t x);
extern inline block128 block128_set_low64(uint64_t x);
extern inline block256 block256_set_low64(uint64_t x);
extern inline block256 block256_set_128(block128 x0, block128 x1);
extern inline block256 block256_set_low128(block128 x);
extern inline block384 block384_xor(block384 x, block384 y);
extern inline block512 block512_xor(block512 x, block512 y);
extern inline block1024 block1024_xor(block1024 x, block1024 y);
extern inline block384 block384_and(block384 x, block384 y);
extern inline block512 block512_and(block512 x, block512 y);
extern inline block1024 block1024_and(block1024 x, block1024 y);
extern inline block384 block384_set_zero();
extern inline block512 block512_set_zero();
extern inline block1024 block1024_set_zero();
extern inline block384 block384_set_all_8(uint8_t x);
extern inline block512 block512_set_all_8(uint8_t x);
extern inline block1024 block1024_set_all_8(uint8_t x);
extern inline block384 block384_set_low32(uint32_t x);
extern inline block512 block512_set_low32(uint32_t x);
extern inline block1024 block1024_set_low32(uint32_t x);
extern inline block384 block384_set_low64(uint64_t x);
extern inline block512 block512_set_low64(uint64_t x);
extern inline block1024 block1024_set_low64(uint64_t x);
extern inline bool block128_any_zeros(block128 x);
extern inline bool block256_any_zeros(block256 x);
extern inline bool block192_any_zeros(block192 x);
extern inline block128 block128_byte_reverse(block128 x);
extern inline block256 block256_from_2_block128(block128 x, block128 y);
extern inline vole_block vole_block_set_zero();
extern inline vole_block vole_block_xor(vole_block x, vole_block y);
extern inline vole_block vole_block_and(vole_block x, vole_block y);
extern inline vole_block vole_block_set_all_8(uint8_t x);
extern inline vole_block vole_block_set_low32(uint32_t x);
extern inline vole_block vole_block_set_low64(uint64_t x);
extern inline clmul_block clmul_block_xor(clmul_block x, clmul_block y);
extern inline clmul_block clmul_block_and(clmul_block x, clmul_block y);
extern inline clmul_block clmul_block_set_all_8(uint8_t x);
extern inline clmul_block clmul_block_set_zero();
extern inline clmul_block clmul_block_clmul_ll(clmul_block x, clmul_block y);
extern inline clmul_block clmul_block_clmul_lh(clmul_block x, clmul_block y);
extern inline clmul_block clmul_block_clmul_hl(clmul_block x, clmul_block y);
extern inline clmul_block clmul_block_clmul_hh(clmul_block x, clmul_block y);
extern inline clmul_block clmul_block_shift_left_64(clmul_block x);
extern inline clmul_block clmul_block_shift_right_64(clmul_block x);
extern inline clmul_block clmul_block_broadcast_low64(clmul_block x);
extern inline clmul_block clmul_block_mix_64(clmul_block x, clmul_block y);



extern inline void block128_inverse(block128* block);
extern inline void block128_mersenne_inverse(block128* block);
extern inline void block128_multiply_with_GF2_matrix(block128* block, const uint64_t* matrix);
extern inline void block128_multiply_with_transposed_GF2_matrix(block128* block, const uint64_t* matrix);

extern inline void block192_inverse(block192* block);
extern inline void block192_mersenne_inverse(block192* block);
extern inline void block192_multiply_with_GF2_matrix(block192* block, const uint64_t* matrix);
extern inline void block192_multiply_with_transposed_GF2_matrix(block192* block, const uint64_t* matrix);

extern inline void block256_inverse(block256* block);
extern inline void block256_mersenne_inverse(block256* block);
extern inline void block256_multiply_with_GF2_matrix(block256* block, const uint64_t* matrix);
extern inline void block256_multiply_with_transposed_GF2_matrix(block256* block, const uint64_t* matrix);

