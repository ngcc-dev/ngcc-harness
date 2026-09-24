#ifndef C_HASH_H
#define C_HASH_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* C-Engine permutation: 1536-bit -> 1536-bit */
int c_engine_permute(const unsigned char *in, unsigned char *out);

/* C-Hash-512: byte-length API (msg_len is BYTES) */
int c_hash_512(const unsigned char *msg, size_t msg_len, unsigned char *digest);

/* C-Hash-512: bit-length API (msg_len_bits is BITS, supports non-byte-aligned) */
int c_hash_512_bits(const unsigned char *msg, unsigned long long msg_len_bits, unsigned char *digest);

#ifdef __cplusplus
}
#endif

#endif
