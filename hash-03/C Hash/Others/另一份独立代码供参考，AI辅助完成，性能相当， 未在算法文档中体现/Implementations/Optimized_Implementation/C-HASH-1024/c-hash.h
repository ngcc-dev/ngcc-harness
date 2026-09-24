#ifndef C_HASH_H
#define C_HASH_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* C-Engine permutation: 1536-bit -> 1536-bit (static inline for zero call overhead) */
static inline void c_engine_permute(const uint8_t *in, uint8_t *out);

/* C-Engine permutation with FeedForward: P(x) XOR x */
static inline void c_engine_permute_ff(const uint8_t *in, uint8_t *out);

/* C-Hash-1024: byte-length API (msg_len is BYTES) */
int c_hash_1024(const uint8_t *msg, size_t msg_len, uint8_t digest[128]);

/* C-Hash-1024: bit-length API (msg_len_bits is BITS, supports non-byte-aligned) */
int c_hash_1024_bits(const uint8_t *msg, unsigned long long msg_len_bits, uint8_t digest[128]);

#ifdef __cplusplus
}
#endif

#endif
