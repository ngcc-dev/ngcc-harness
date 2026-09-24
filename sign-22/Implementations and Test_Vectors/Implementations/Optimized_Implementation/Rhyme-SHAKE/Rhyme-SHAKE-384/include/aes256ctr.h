#ifndef AES256CTR_H
#define AES256CTR_H
#include <stddef.h>
#include <stdint.h>
#include <wmmintrin.h>
#define AES256CTR_BLOCKBYTES 64
typedef struct { __m128i rk[15]; __m128i ctr; } aes256ctr_ctx;
void aes256ctr_init(aes256ctr_ctx *state, const uint8_t key[32], uint64_t nonce);
void aes256ctr_squeezeblocks(uint8_t *out, size_t nblocks, aes256ctr_ctx *state);
void aes256ctr_prf(uint8_t *out, size_t outlen, const uint8_t seed[32], uint64_t nonce);
void aes256ctr_free(aes256ctr_ctx *state);
void aes256ctr_set_nonce(aes256ctr_ctx *state, uint64_t nonce);
#endif
